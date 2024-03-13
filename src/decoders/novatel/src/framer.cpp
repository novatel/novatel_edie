////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file framer.cpp
//! \brief Frame OEM messages.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "framer.hpp"

#include "crc32.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
Framer::Framer() : FramerInterface("novatel_framer") {}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsAsciiCRC(const uint32_t uiDelimiterPosition_) const
{
    return uiDelimiterPosition_ + OEM4_ASCII_CRC_LENGTH < clMyCircularDataBuffer.GetLength() && IsCRLF(uiDelimiterPosition_ + OEM4_ASCII_CRC_LENGTH);
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsCRLF(const uint32_t uiCircularBufferPosition_) const
{
    return uiCircularBufferPosition_ + 1 < clMyCircularDataBuffer.GetLength() && clMyCircularDataBuffer[uiCircularBufferPosition_] == '\r' &&
           clMyCircularDataBuffer[uiCircularBufferPosition_ + 1] == '\n';
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsSpaceCRLF(const uint32_t uiCircularBufferPosition_) const
{
    return uiCircularBufferPosition_ + 2 < clMyCircularDataBuffer.GetLength() &&
           clMyCircularDataBuffer[uiCircularBufferPosition_] == OEM4_ABBREV_ASCII_SEPARATOR && IsCRLF(uiCircularBufferPosition_ + 1);
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsEmptyLine(uint32_t uiCircularBufferPosition_) const
{
    while (clMyCircularDataBuffer[uiCircularBufferPosition_--] == ' ')
        if (clMyCircularDataBuffer[uiCircularBufferPosition_] == '<') return true;

    return false;
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsAbbrevAsciiResponse() const
{
    constexpr uint32_t ERROR_LEN = 5;
    constexpr uint32_t OK_LEN = 2;
    char szResponse[ERROR_LEN + 1]{0};

    if (uiMyAbbrevAsciiHeaderPosition + OK_LEN < clMyCircularDataBuffer.GetLength())
    {
        for (uint32_t i = 0; i < OK_LEN; i++) szResponse[i] = clMyCircularDataBuffer[uiMyAbbrevAsciiHeaderPosition + i];

        if (strstr(szResponse, "OK")) return true;
    }

    if (uiMyAbbrevAsciiHeaderPosition + ERROR_LEN < clMyCircularDataBuffer.GetLength())
    {
        for (uint32_t i = 0; i < ERROR_LEN; i++) szResponse[i] = clMyCircularDataBuffer[uiMyAbbrevAsciiHeaderPosition + i];

        if (strstr(szResponse, "ERROR")) return true;
    }

    return false;
}

// -------------------------------------------------------------------------------------------------------
void Framer::HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t uiUnknownBytes_)
{
    if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { clMyCircularDataBuffer.Copy(pucBuffer_, uiUnknownBytes_); }
    clMyCircularDataBuffer.Discard(uiUnknownBytes_);

    uiMyByteCount = 0;
    uiMyExpectedMessageLength = 0;
    uiMyExpectedPayloadLength = 0;

    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
}

// -------------------------------------------------------------------------------------------------------
void Framer::SetFrameJson(bool bFrameJson_) { bMyFrameJson = bFrameJson_; }

// -------------------------------------------------------------------------------------------------------
void Framer::SetPayloadOnly(bool bMyPayloadOnly_) { bMyPayloadOnly = bMyPayloadOnly_; }

// -------------------------------------------------------------------------------------------------------
STATUS
Framer::GetFrame(unsigned char* pucFrameBuffer_, const uint32_t uiFrameBufferSize_, MetaDataStruct& stMetaData_)
{
    STATUS eStatus = STATUS::UNKNOWN;
    uint32_t uiMessageCRC;
    OEM4BinaryHeader stOEM4BinaryHeader;
    OEM4BinaryShortHeader stOEM4BinaryShortHeader;

    if (pucFrameBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    // Loop buffer to complete NovAtel message
    while (eMyFrameState != NovAtelFrameStateEnum::COMPLETE_MESSAGE)
    {
        stMetaData_.bResponse = false;

        // Read data from circular buffer until we reach the end or we
        // didn't find a complete frame in current data buffer
        if (clMyCircularDataBuffer.GetLength() == uiMyByteCount)
        {
            if (eMyFrameState > NovAtelFrameStateEnum::WAITING_FOR_SYNC)
            {
                if (stMetaData_.eFormat == HEADERFORMAT::ABB_ASCII)
                {
                    uiMyByteCount--; // If the data lands on the ABBV header CRLF then it can be
                                     // missed unless it's tested again when there is more data
                }
                return STATUS::INCOMPLETE;
            }
            else if (uiMyByteCount == 0)
            {
                stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount;
                return STATUS::BUFFER_EMPTY;
            }
            else
            {
                stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount;
                HandleUnknownBytes(pucFrameBuffer_, uiMyByteCount);
                return STATUS::UNKNOWN;
            }
        }

        unsigned char ucDataByte = clMyCircularDataBuffer[uiMyByteCount++];
        stMetaData_.uiLength = uiMyByteCount;

        // If we see any non-ASCII characters in a ASCII message then we've encountered unknown data
        // or a corrupt log. Either way mark the data as unknown
        if ((stMetaData_.eFormat == HEADERFORMAT::ASCII || stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII ||
             stMetaData_.eFormat == HEADERFORMAT::ABB_ASCII || stMetaData_.eFormat == HEADERFORMAT::NMEA ||
             stMetaData_.eFormat == HEADERFORMAT::JSON) &&
            ucDataByte > 127)
        {
            stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
            eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
            uiMyByteCount--;
        }

        switch (eMyFrameState)
        {
        case NovAtelFrameStateEnum::WAITING_FOR_SYNC:
            uiMyCalculatedCRC32 = 0;

            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC1:
                uiMyCalculatedCRC32 = CalculateCharacterCRC32(0, ucDataByte);
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_BINARY_SYNC2;
                break;
            case OEM4_ASCII_SYNC:
                stMetaData_.eFormat = HEADERFORMAT::ASCII;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_ASCII_HEADER_AND_BODY;
                break;
            case OEM4_SHORT_ASCII_SYNC:
                stMetaData_.eFormat = HEADERFORMAT::SHORT_ASCII;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_ASCII_HEADER_AND_BODY;
                break;
            case NMEA_SYNC:
                stMetaData_.eFormat = HEADERFORMAT::NMEA;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_NMEA_BODY;
                break;
            case OEM4_ABBREV_ASCII_SYNC:
                stMetaData_.eFormat = HEADERFORMAT::ABB_ASCII;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_ABB_ASCII_SYNC2;
                uiMyAbbrevAsciiHeaderPosition = uiMyByteCount;
                break;
            case '{':
                if (bMyFrameJson)
                {
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_JSON_OBJECT;
                    stMetaData_.eFormat = HEADERFORMAT::JSON;
                    uiMyJsonObjectOpenBraces++;
                }
                break;
            default: break;
            }

            // If we have just encountered a sync byte and have read bytes before, we need to
            // handle them
            if (eMyFrameState > NovAtelFrameStateEnum::WAITING_FOR_SYNC && uiMyByteCount > 1)
            {
                stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount - 1;
                HandleUnknownBytes(pucFrameBuffer_, uiMyByteCount - 1);
                return STATUS::UNKNOWN;
            }
            else if (uiMyByteCount > uiFrameBufferSize_)
            {
                stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount - 1;
                HandleUnknownBytes(pucFrameBuffer_, uiFrameBufferSize_);
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_BINARY_SYNC2:
            if (ucDataByte == OEM4_BINARY_SYNC2)
            {
                uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_BINARY_SYNC3;
            }
            else if (ucDataByte == OEM4_PROPRIETARY_BINARY_SYNC2)
            {
                uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);
                stMetaData_.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_BINARY_SYNC3;
            }
            else
            {
                stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                uiMyByteCount--;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_BINARY_SYNC3:
            if (ucDataByte == OEM4_BINARY_SYNC3)
            {
                uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);
                if (stMetaData_.eFormat != HEADERFORMAT::PROPRIETARY_BINARY) { stMetaData_.eFormat = HEADERFORMAT::BINARY; }
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_BINARY_HEADER;
            }
            else if (ucDataByte == OEM4_SHORT_BINARY_SYNC3)
            {
                uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);
                stMetaData_.eFormat = HEADERFORMAT::SHORT_BINARY;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SHORT_BINARY_HEADER;
            }
            else
            {
                stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                uiMyByteCount--;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_ABB_ASCII_SYNC2:
            if (ucDataByte != OEM4_ABBREV_ASCII_SEPARATOR && isalpha(ucDataByte))
            {
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_ABB_ASCII_HEADER;
            }
            else
            {
                stMetaData_.eFormat = HEADERFORMAT::UNKNOWN;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                uiMyByteCount--;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_BINARY_HEADER:
            uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);
            if (uiMyByteCount == OEM4_BINARY_HEADER_LENGTH)
            {
                if (uiFrameBufferSize_ < OEM4_BINARY_HEADER_LENGTH)
                {
                    uiMyByteCount = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                    return STATUS::BUFFER_FULL;
                }

                clMyCircularDataBuffer.Copy(reinterpret_cast<unsigned char*>(&stOEM4BinaryHeader), OEM4_BINARY_HEADER_LENGTH);
                uiMyExpectedPayloadLength = static_cast<uint32_t>(stOEM4BinaryHeader.usLength);
                uiMyExpectedMessageLength = OEM4_BINARY_HEADER_LENGTH + static_cast<uint32_t>(stOEM4BinaryHeader.usLength) + OEM4_BINARY_CRC_LENGTH;

                if (uiMyExpectedPayloadLength > MAX_BINARY_MESSAGE_LENGTH || uiMyExpectedMessageLength > MAX_BINARY_MESSAGE_LENGTH)
                {
                    uiMyByteCount = OEM4_BINARY_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                    break;
                }

                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_BINARY_BODY_AND_CRC;

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetaData_.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;

                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;

                    return STATUS::BUFFER_FULL;
                }
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_SHORT_BINARY_HEADER:
            uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);
            if (uiMyByteCount == OEM4_SHORT_BINARY_HEADER_LENGTH)
            {
                if (uiFrameBufferSize_ < OEM4_SHORT_BINARY_HEADER_LENGTH)
                {
                    uiMyByteCount = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                    return STATUS::BUFFER_FULL;
                }

                clMyCircularDataBuffer.Copy(reinterpret_cast<unsigned char*>(&stOEM4BinaryShortHeader), OEM4_SHORT_BINARY_HEADER_LENGTH);
                uiMyExpectedPayloadLength = static_cast<uint32_t>(stOEM4BinaryShortHeader.ucLength);
                uiMyExpectedMessageLength =
                    OEM4_SHORT_BINARY_HEADER_LENGTH + static_cast<uint32_t>(stOEM4BinaryShortHeader.ucLength) + OEM4_BINARY_CRC_LENGTH;

                if (uiMyExpectedPayloadLength > MAX_SHORT_BINARY_MESSAGE_LENGTH || uiMyExpectedMessageLength > MAX_SHORT_BINARY_MESSAGE_LENGTH)
                {
                    uiMyByteCount = OEM4_SHORT_BINARY_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                    break;
                }

                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_BINARY_BODY_AND_CRC;

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetaData_.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;

                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;

                    return STATUS::BUFFER_FULL;
                }
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_BINARY_BODY_AND_CRC:
            uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);

            if (uiMyByteCount == uiMyExpectedMessageLength)
            {
                if (uiMyCalculatedCRC32 == 0)
                {
                    eStatus = STATUS::SUCCESS;
                    if (bMyPayloadOnly)
                    {
                        stMetaData_.uiLength = uiMyExpectedPayloadLength;
                        clMyCircularDataBuffer.Discard((uiMyExpectedMessageLength - uiMyExpectedPayloadLength) + OEM4_BINARY_CRC_LENGTH);
                        clMyCircularDataBuffer.Copy(pucFrameBuffer_, stMetaData_.uiLength);
                        clMyCircularDataBuffer.Discard(uiMyExpectedPayloadLength + OEM4_BINARY_CRC_LENGTH);
                    }
                    else
                    {
                        clMyCircularDataBuffer.Copy(pucFrameBuffer_, stMetaData_.uiLength);
                        clMyCircularDataBuffer.Discard(stMetaData_.uiLength);
                    }
                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = (stMetaData_.eFormat == HEADERFORMAT::BINARY) ? OEM4_BINARY_SYNC_LENGTH : OEM4_SHORT_BINARY_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                }
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_ASCII_HEADER_AND_BODY:

            if (ucDataByte == OEM4_ASCII_CRC_DELIMITER)
            {
                // Need to be able to check for *12345678CRLF
                if (uiMyByteCount + OEM4_ASCII_CRC_LENGTH + 2 > clMyCircularDataBuffer.GetLength())
                {
                    uiMyByteCount--; // Rewind so that we reprocess the '*' delimiter after
                                     // getting more bytes
                    return STATUS::INCOMPLETE;
                }

                // Handle RXCONFIGA logs: Normally * signifies the start of the CRC, but
                // RXCONFIG payload has an internal CRC that we need to treat as part of the log
                // payload.

                //                   |<--------------rxconfig payload--------------->|
                // #RXCONFIGA,HEADER;#command,commandheader;commandpayload*commandcrc*CRC
                //                                         internal CRC   |<------->|

                // Check for a second CRC delimiter which indicates this is RXCONFIG
                if (clMyCircularDataBuffer[uiMyByteCount + OEM4_ASCII_CRC_LENGTH] == OEM4_ASCII_CRC_DELIMITER)
                {
                    uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte);
                }
                // Look ahead for the CRLF to ensure this is a CRC delimiter and not a '*' in a
                // log payload
                else if (!IsAsciiCRC(uiMyByteCount)) { uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte); }
                else { eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_ASCII_CRC; }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
            }
            else { uiMyCalculatedCRC32 = CalculateCharacterCRC32(uiMyCalculatedCRC32, ucDataByte); }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_ABB_ASCII_HEADER:
            if (IsCRLF(uiMyByteCount - 1))
            {
                if (IsAbbrevAsciiResponse())
                {
                    uiMyByteCount++; // Add 1 to consume LF
                    stMetaData_.uiLength = uiMyByteCount;
                    stMetaData_.bResponse = true;

                    eStatus = STATUS::SUCCESS;

                    if (uiFrameBufferSize_ < stMetaData_.uiLength)
                    {
                        uiMyByteCount = 0;
                        uiMyAbbrevAsciiHeaderPosition = 0;
                        uiMyExpectedPayloadLength = 0;
                        eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                        return STATUS::BUFFER_FULL;
                    }

                    clMyCircularDataBuffer.Copy(pucFrameBuffer_, stMetaData_.uiLength);
                    clMyCircularDataBuffer.Discard(stMetaData_.uiLength);

                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::COMPLETE_MESSAGE;
                }
                // End of buffer, can't look ahead but there should be more data
                else if (uiMyByteCount + 2 >= clMyCircularDataBuffer.GetLength())
                {
                    uiMyByteCount--; // If the data lands on the header CRLF then it can be
                                     // missed unless it's tested again when there is more data
                    stMetaData_.uiLength = uiMyByteCount;
                    return STATUS::INCOMPLETE;
                }
                // New line with abbrev data
                else if (clMyCircularDataBuffer[uiMyByteCount + 1] == OEM4_ABBREV_ASCII_SYNC &&
                         clMyCircularDataBuffer[uiMyByteCount + 2] == OEM4_ABBREV_ASCII_SEPARATOR)
                {
                    uiMyByteCount++; // Add 1 to consume LF
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_ABB_ASCII_BODY;
                }
                // New line with some other data
                else
                {
                    uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_ABB_ASCII_BODY:
            // End of buffer (can't look ahead, assume incomplete message)
            if (uiMyByteCount + 3 >= clMyCircularDataBuffer.GetLength())
            {
                uiMyByteCount--; // If the data lands on the header CRLF then it can be missed
                                 // unless it's tested again when there is more data
                stMetaData_.uiLength = clMyCircularDataBuffer.GetLength();
                return STATUS::INCOMPLETE;
            }

            // Abbrev Array, more data to follow
            if (IsSpaceCRLF(uiMyByteCount - 1))
            {
                uiMyByteCount += 2; // Consume CRLF

                // New line with non abbrev data
                if (clMyCircularDataBuffer[uiMyByteCount] != OEM4_ABBREV_ASCII_SYNC
                    // Abbrev data, but is the start of a new message rather than a
                    // continuation of the current message: <NEWMESSAGE
                    || clMyCircularDataBuffer[uiMyByteCount + 1] != OEM4_ABBREV_ASCII_SEPARATOR)
                {
                    // 0 length arrays will output an empty line which suggests more data will
                    // follow In this case, this is actually the end of the log
                    if (IsEmptyLine(uiMyByteCount - 3))
                    {
                        uiMyByteCount--; // rewind back to CR so we can treat this as end of log
                    }
                    else
                    {
                        uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                        uiMyAbbrevAsciiHeaderPosition = 0;
                        uiMyExpectedPayloadLength = 0;
                        eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                    }
                }
            }

            // End of Abbrev
            if (IsCRLF(uiMyByteCount - 1))
            {
                uiMyByteCount++; // Add 1 to consume LF
                stMetaData_.uiLength = uiMyByteCount;

                eStatus = STATUS::SUCCESS;

                if (uiFrameBufferSize_ < stMetaData_.uiLength)
                {
                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                    return STATUS::BUFFER_FULL;
                }

                clMyCircularDataBuffer.Copy(pucFrameBuffer_, stMetaData_.uiLength);
                clMyCircularDataBuffer.Discard(stMetaData_.uiLength);

                uiMyByteCount = 0;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::COMPLETE_MESSAGE;
            }
            else if (uiMyByteCount >= MAX_ABB_ASCII_RESPONSE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_ASCII_CRC:
            if (IsAsciiCRC(uiMyByteCount - 1))
            {
                uiMyByteCount--; // rewind back to delimiter before copying
                char acCrc[OEM4_ASCII_CRC_LENGTH + 1];
                for (int32_t i = 0; i < OEM4_ASCII_CRC_LENGTH; i++) { acCrc[i] = clMyCircularDataBuffer[uiMyByteCount++]; }
                uiMyByteCount += 2; // Add 2 for CRLF
                stMetaData_.uiLength = uiMyByteCount;

                acCrc[OEM4_ASCII_CRC_LENGTH] = '\0';
                sscanf(acCrc, "%x", &uiMessageCRC);

                if (uiMyCalculatedCRC32 == uiMessageCRC)
                {
                    eStatus = STATUS::SUCCESS;

                    if (uiFrameBufferSize_ < stMetaData_.uiLength)
                    {
                        uiMyByteCount = 0;
                        uiMyExpectedPayloadLength = 0;
                        eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                        return STATUS::BUFFER_FULL;
                    }

                    clMyCircularDataBuffer.Copy(pucFrameBuffer_, stMetaData_.uiLength);
                    clMyCircularDataBuffer.Discard(stMetaData_.uiLength);

                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyExpectedPayloadLength = 0;
                    uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_NMEA_BODY:
            if (ucDataByte == OEM4_ASCII_CRC_DELIMITER) { eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_NMEA_CRC; }
            else if (uiMyByteCount >= MAX_NMEA_MESSAGE_LENGTH)
            {
                uiMyByteCount = NMEA_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
            }
            else { uiMyCalculatedCRC32 ^= ucDataByte; }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_NMEA_CRC:
            if (ucDataByte == '\n')
            {
                char acCrc[NMEA_CRC_LENGTH + 1];
                for (int32_t iOffset = NMEA_CRC_LENGTH; iOffset > 0; iOffset--)
                {
                    acCrc[NMEA_CRC_LENGTH - iOffset] = clMyCircularDataBuffer[uiMyByteCount - iOffset - 2];
                }
                acCrc[NMEA_CRC_LENGTH] = '\0';

                if (sscanf(acCrc, "%x", &uiMessageCRC) > 0 && uiMyCalculatedCRC32 == uiMessageCRC)
                {
                    stMetaData_.uiLength = uiMyByteCount;
                    eStatus = STATUS::SUCCESS;

                    if (uiFrameBufferSize_ < stMetaData_.uiLength)
                    {
                        uiMyByteCount = 0;
                        uiMyExpectedPayloadLength = 0;
                        eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                        return STATUS::BUFFER_FULL;
                    }

                    clMyCircularDataBuffer.Copy(pucFrameBuffer_, stMetaData_.uiLength);
                    clMyCircularDataBuffer.Discard(stMetaData_.uiLength);

                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;

                    eMyFrameState = NovAtelFrameStateEnum::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = NMEA_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                }
            }
            else if (uiMyByteCount >= MAX_NMEA_MESSAGE_LENGTH)
            {
                uiMyByteCount = NMEA_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
            }
            break;

        case NovAtelFrameStateEnum::WAITING_FOR_JSON_OBJECT:
            if (uiFrameBufferSize_ < uiMyByteCount)
            {
                uiMyByteCount = 0;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;
                return STATUS::BUFFER_FULL;
            }

            if (ucDataByte == '{') { uiMyJsonObjectOpenBraces++; }
            else if (ucDataByte == '}') { uiMyJsonObjectOpenBraces--; }

            if (uiMyJsonObjectOpenBraces == 0)
            {
                eStatus = STATUS::SUCCESS;
                stMetaData_.uiLength = uiMyByteCount;

                clMyCircularDataBuffer.Copy(pucFrameBuffer_, stMetaData_.uiLength);
                clMyCircularDataBuffer.Discard(stMetaData_.uiLength);

                uiMyByteCount = 0;
                uiMyExpectedPayloadLength = 0;

                eMyFrameState = NovAtelFrameStateEnum::COMPLETE_MESSAGE;
            }
            break;

        default:
            std::string sError = "GetFrame(): Invalid parsing state";
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
            throw std::runtime_error(sError);
        }
    }

    eMyFrameState = NovAtelFrameStateEnum::WAITING_FOR_SYNC;

    return eStatus;
}
