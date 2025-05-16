// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file framer.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/framer.hpp"

#include <charconv>

#include "novatel_edie/common/crc32.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
Framer::Framer() : FramerBase("novatel_framer") {}

// -------------------------------------------------------------------------------------------------------
Framer::Framer(std::shared_ptr<CircularBuffer> circularBuffer) : FramerBase("novatel_framer", circularBuffer) {
    pclMyLogger->info("Framer initialized");
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsAbbrevSeparatorCrlf(const uint32_t uiCircularBufferPosition_) const
{
    return IsCrlf(uiCircularBufferPosition_ + 1) && clMyBuffer[uiCircularBufferPosition_] == OEM4_ABBREV_ASCII_SEPARATOR;
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsEmptyAbbrevLine(uint32_t uiCircularBufferPosition_) const
{
    while (clMyBuffer[uiCircularBufferPosition_--] == OEM4_ABBREV_ASCII_SEPARATOR)
    {
        if (clMyBuffer[uiCircularBufferPosition_] == OEM4_ABBREV_ASCII_SYNC) { return true; }
    }

    return false;
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsAbbrevAsciiResponse() const
{
    constexpr uint32_t errorLen = 5;
    constexpr uint32_t okLen = 2;

    if (uiMyAbbrevAsciiHeaderPosition + okLen < clMyBuffer.size())
    {
        if (clMyBuffer[uiMyAbbrevAsciiHeaderPosition + 0] == 'O' && //
            clMyBuffer[uiMyAbbrevAsciiHeaderPosition + 1] == 'K')
        {
            return true;
        }
    }

    if (uiMyAbbrevAsciiHeaderPosition + errorLen < clMyBuffer.size())
    {
        if (clMyBuffer[uiMyAbbrevAsciiHeaderPosition + 0] == 'E' && //
            clMyBuffer[uiMyAbbrevAsciiHeaderPosition + 1] == 'R' && //
            clMyBuffer[uiMyAbbrevAsciiHeaderPosition + 2] == 'R' && //
            clMyBuffer[uiMyAbbrevAsciiHeaderPosition + 3] == 'O' && //
            clMyBuffer[uiMyAbbrevAsciiHeaderPosition + 4] == 'R')
        {
            return true;
        }
    }

    return false;
}

// -------------------------------------------------------------------------------------------------------
uint32_t Framer::FindSyncOffset(const uint32_t uiFrameBufferSize_, STATUS& eOffsetStatus_)
{
    auto eCurrentFrameState = NovAtelFrameState::WAITING_FOR_SYNC;
    uint32_t uiByteOffset = 0;

    while (eCurrentFrameState != NovAtelFrameState::COMPLETE_MESSAGE)
    {
        // Read data from circular buffer until end is reached
        if (pclMyCircularDataBuffer->GetLength() == uiByteOffset)
        {
            if (eCurrentFrameState != NovAtelFrameState::WAITING_FOR_SYNC)
            {
                eOffsetStatus_ = STATUS::INCOMPLETE;
                return 0;
            }

            if (uiByteOffset == 0)
            {
                eOffsetStatus_ = STATUS::BUFFER_EMPTY;
                return 0;
            }

            eOffsetStatus_ = STATUS::UNKNOWN;
            return 0;
        }

        const unsigned char ucDataByte = (*pclMyCircularDataBuffer)[uiByteOffset++];

        switch (eCurrentFrameState)
        {
        case NovAtelFrameState::WAITING_FOR_SYNC:
            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC1: eCurrentFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC2; break;
            case OEM4_ASCII_SYNC: eOffsetStatus_ = STATUS::SUCCESS; return uiByteOffset - OEM4_ASCII_SYNC_LENGTH;
            case OEM4_SHORT_ASCII_SYNC: eOffsetStatus_ = STATUS::SUCCESS; return uiByteOffset - OEM4_SHORT_ASCII_SYNC_LENGTH;
            case OEM4_ABBREV_ASCII_SYNC: eOffsetStatus_ = STATUS ::SUCCESS; return uiByteOffset - 1;
            default: break;
            }
            break;
        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC2:
            switch (ucDataByte)
            {
            case OEM4_PROPRIETARY_BINARY_SYNC2: [[fallthrough]];
            case OEM4_BINARY_SYNC2: eCurrentFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC3; break;
            default:
                uiByteOffset--;
                eCurrentFrameState = NovAtelFrameState::WAITING_FOR_SYNC;
                break;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC3:
            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC3: [[fallthrough]];
            case OEM4_SHORT_BINARY_SYNC3:
                uiByteOffset = uiByteOffset - 3;
                eOffsetStatus_ = STATUS::SUCCESS;
                return uiByteOffset;
            default:
                uiByteOffset--;
                eCurrentFrameState = NovAtelFrameState::WAITING_FOR_SYNC;
                break;
            }
            break;
        default: eOffsetStatus_ = STATUS::UNKNOWN; return 0;
        }
    }
}

// -------------------------------------------------------------------------------------------------------
STATUS
Framer::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_, bool bMetadataOnly_)
{
    if (pucFrameBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    // Loop buffer to complete NovAtel message
    while (eMyFrameState != NovAtelFrameState::COMPLETE_MESSAGE)
    {
        stMetaData_.bResponse = false;
        // Read data from circular buffer until we reach the end or we didn't find a complete frame in current data buffer
        if (clMyBuffer.size() == uiMyByteCount)
        {
            if (eMyFrameState != NovAtelFrameState::WAITING_FOR_SYNC)
            {
                // This logic should only be operated on ABB_ASCII
                if (stMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII)
                {
                    // If the data lands between ABB_ASCII SYNC1 & 2, it can be missed unless it's tested again when there is more data
                    if ((*pclMyCircularDataBuffer)[uiMyByteCount - 1] == OEM4_ABBREV_ASCII_SYNC)
                    {
                        InitAttributes();
                        ResetState();
                    }
                    // If the data lands on the abbreviated header CRLF then it can be missed unless it's tested again when there is more data
                    else { uiMyByteCount--; }
                }

                return STATUS::INCOMPLETE;
            }

            stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
            stMetaData_.uiLength = uiMyByteCount;

            if (uiMyByteCount == 0) { return STATUS::BUFFER_EMPTY; }

            HandleUnknownBytes(pucFrameBuffer_, uiMyByteCount);
            return STATUS::UNKNOWN;
        }

        const unsigned char ucDataByte = clMyBuffer[uiMyByteCount++];
        stMetaData_.uiLength = uiMyByteCount;

        // non-ASCII characters in an ASCII message indicates a corrupt log or unknown data. Either way, mark the data as unknown
        if ((stMetaData_.eFormat == HEADER_FORMAT::ASCII || stMetaData_.eFormat == HEADER_FORMAT::SHORT_ASCII ||
             stMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII || stMetaData_.eFormat == HEADER_FORMAT::NMEA ||
             stMetaData_.eFormat == HEADER_FORMAT::JSON) &&
            ucDataByte > 127)
        {
            stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
            ResetState();
            uiMyByteCount--;
        }

        switch (eMyFrameState)
        {
        case NovAtelFrameState::WAITING_FOR_SYNC:
            uiMyCalculatedCrc32 = 0;

            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC1:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC2;
                break;
            case OEM4_ASCII_SYNC:
                stMetaData_.eFormat = HEADER_FORMAT::ASCII;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY;
                break;
            case OEM4_SHORT_ASCII_SYNC:
                stMetaData_.eFormat = HEADER_FORMAT::SHORT_ASCII;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY;
                break;
            case OEM4_ABBREV_ASCII_SYNC:
                stMetaData_.eFormat = HEADER_FORMAT::ABB_ASCII;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_SYNC2;
                uiMyAbbrevAsciiHeaderPosition = uiMyByteCount;
                break;
            case '{':
                if (bMyFrameJson)
                {
                    eMyFrameState = NovAtelFrameState::WAITING_FOR_JSON_OBJECT;
                    stMetaData_.eFormat = HEADER_FORMAT::JSON;
                    uiMyJsonObjectOpenBraces++;
                }
                break;
            default: break;
            }

            // If we have just encountered a sync byte and have read bytes before, we need to handle them
            if (eMyFrameState != NovAtelFrameState::WAITING_FOR_SYNC && uiMyByteCount > 1)
            {
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount - 1;
                HandleUnknownBytes(pucFrameBuffer_, uiMyByteCount - 1);
                return STATUS::UNKNOWN;
            }
            if (uiMyByteCount > uiFrameBufferSize_)
            {
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount - 1;
                HandleUnknownBytes(pucFrameBuffer_, uiFrameBufferSize_);
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC2:
            switch (ucDataByte)
            {
            case OEM4_PROPRIETARY_BINARY_SYNC2: stMetaData_.eFormat = HEADER_FORMAT::PROPRIETARY_BINARY; [[fallthrough]];
            case OEM4_BINARY_SYNC2:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC3;
                break;
            default:
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                ResetState();
                uiMyByteCount--;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC3:
            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC3:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                if (stMetaData_.eFormat != HEADER_FORMAT::PROPRIETARY_BINARY) { stMetaData_.eFormat = HEADER_FORMAT::BINARY; }
                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_HEADER;
                break;
            case OEM4_SHORT_BINARY_SYNC3:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                stMetaData_.eFormat = HEADER_FORMAT::SHORT_BINARY;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_SHORT_BINARY_HEADER;
                break;
            default:
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                ResetState();
                uiMyByteCount--;
                break;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_SYNC2:
            if (ucDataByte != OEM4_ABBREV_ASCII_SEPARATOR && isalpha(ucDataByte) != 0)
            {
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_HEADER;
            }
            else
            {
                if (pclMyCircularDataBuffer->GetLength() == uiMyByteCount)
                {
                    // If the data lands on the abbreviated header CRLF then it can be missed unless it's tested again when there is more data
                    uiMyByteCount--;
                    return STATUS::INCOMPLETE;
                }
                uiMyExpectedPayloadLength = 0;
                uiMyExpectedMessageLength = 0;
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount;
                if (!bMetadataOnly_)
                {
                    pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                    pclMyCircularDataBuffer->Discard(uiMyByteCount);
                }
                ResetState();
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_HEADER: {
            CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);

            if (uiMyByteCount == OEM4_BINARY_HEADER_LENGTH)
            {
                if (uiFrameBufferSize_ < OEM4_BINARY_HEADER_LENGTH)
                {
                    uiMyByteCount = 0;
                    ResetState();
                    return STATUS::BUFFER_FULL;
                }

                Oem4BinaryHeader header;
                clMyBuffer.copy_out(reinterpret_cast<unsigned char*>(&header), OEM4_BINARY_HEADER_LENGTH);
                uiMyExpectedPayloadLength = static_cast<uint32_t>(header.usLength);
                uiMyExpectedMessageLength = OEM4_BINARY_HEADER_LENGTH + static_cast<uint32_t>(header.usLength) + OEM4_BINARY_CRC_LENGTH;

                if (uiMyExpectedPayloadLength > MAX_BINARY_MESSAGE_LENGTH || uiMyExpectedMessageLength > MAX_BINARY_MESSAGE_LENGTH)
                {
                    uiMyByteCount = OEM4_BINARY_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData_.uiLength = uiMyByteCount;
                    if (!bMetadataOnly_)
                    {
                        pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                        pclMyCircularDataBuffer->Discard(uiMyByteCount);
                    }
                    ResetState();
                    return STATUS::UNKNOWN;
                }

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetaData_.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;
                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    ResetState();
                    return STATUS::BUFFER_FULL;
                }

                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_BODY_AND_CRC;
            }
            break;
        }
        case NovAtelFrameState::WAITING_FOR_SHORT_BINARY_HEADER: {
            CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);

            if (uiMyByteCount == OEM4_SHORT_BINARY_HEADER_LENGTH)
            {
                if (uiFrameBufferSize_ < OEM4_SHORT_BINARY_HEADER_LENGTH)
                {
                    uiMyByteCount = 0;
                    ResetState();
                    return STATUS::BUFFER_FULL;
                }

                Oem4BinaryShortHeader header;
                clMyBuffer.copy_out(reinterpret_cast<unsigned char*>(&header), OEM4_SHORT_BINARY_HEADER_LENGTH);
                uiMyExpectedPayloadLength = static_cast<uint32_t>(header.ucLength);
                uiMyExpectedMessageLength = OEM4_SHORT_BINARY_HEADER_LENGTH + OEM4_BINARY_CRC_LENGTH + header.ucLength;

                if (uiMyExpectedPayloadLength > MAX_SHORT_BINARY_MESSAGE_LENGTH || uiMyExpectedMessageLength > MAX_SHORT_BINARY_MESSAGE_LENGTH)
                {
                    uiMyByteCount = OEM4_SHORT_BINARY_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    if (!bMetadataOnly_)
                    {
                        stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                        stMetaData_.uiLength = uiMyByteCount;
                        pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                        pclMyCircularDataBuffer->Discard(uiMyByteCount);
                    }
                    ResetState();
                    return STATUS::UNKNOWN;
                }

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetaData_.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;
                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    ResetState();
                    return STATUS::BUFFER_FULL;
                }

                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_BODY_AND_CRC;
            }
            break;
        }
        case NovAtelFrameState::WAITING_FOR_BINARY_BODY_AND_CRC:
            CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);

            if (uiMyByteCount == uiMyExpectedMessageLength)
            {
                if (uiMyCalculatedCrc32 == 0)
                {
                    if (bMyPayloadOnly)
                    {
                        stMetaData_.uiLength = uiMyExpectedPayloadLength;
                        if (!bMetadataOnly_)
                        {
                            clMyBuffer.erase_begin((uiMyExpectedMessageLength - uiMyExpectedPayloadLength) + OEM4_BINARY_CRC_LENGTH);
                            clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                            clMyBuffer.erase_begin(uiMyExpectedPayloadLength + OEM4_BINARY_CRC_LENGTH);
                        }
                    }
                    else
                    {
                        if (!bMetadataOnly_)
                        {
                            clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                            clMyBuffer.erase_begin(stMetaData_.uiLength);
                        }
                    }

                    uiMyByteCount = 0;
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = stMetaData_.eFormat == HEADER_FORMAT::BINARY ? OEM4_BINARY_SYNC_LENGTH : OEM4_SHORT_BINARY_SYNC_LENGTH;
                    ResetState();
                }

                uiMyExpectedPayloadLength = 0;
                uiMyExpectedMessageLength = 0;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY:
            if (ucDataByte == OEM4_ASCII_CRC_DELIMITER)
            {
                // Need to be able to check for *12345678CRLF
                if (uiMyByteCount > clMyBuffer.size() - OEM4_ASCII_CRC_LENGTH - 2)
                {
                    uiMyByteCount--; // Rewind so that we reprocess the '*' delimiter after getting more bytes
                    return STATUS::INCOMPLETE;
                }

                // Handle RXCONFIGA logs: Normally * signifies the start of the CRC, but
                // RXCONFIG payload has an internal CRC that we need to treat as part of the log
                // payload.

                //                   |<----------------rxconfig payload---------------->|
                // #RXCONFIGA,HEADER;#command,command header;command payload*command crc*CRC
                //                                            internal CRC  |<--------->|

                // Check for a second CRC delimiter which indicates this is RXCONFIG
                if (clMyBuffer[uiMyByteCount + OEM4_ASCII_CRC_LENGTH] == OEM4_ASCII_CRC_DELIMITER
                    // Look ahead for the CRLF to ensure this is a CRC delimiter and not a '*' in a log payload
                    || !IsAsciiCrc(uiMyByteCount))
                {
                    CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                }
                else { eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_CRC; }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                uiMyExpectedMessageLength = 0;
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount;
                if (!bMetadataOnly_)
                {
                    pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                    pclMyCircularDataBuffer->Discard(uiMyByteCount);
                }
                ResetState();
                return STATUS::UNKNOWN;
            }
            else { CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte); }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_HEADER:
            if (IsCrlf(uiMyByteCount - 1))
            {
                if (IsAbbrevAsciiResponse())
                {
                    stMetaData_.uiLength = uiMyByteCount + 1; // Add 1 to consume LF
                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    stMetaData_.bResponse = true;

                    if (uiFrameBufferSize_ < stMetaData_.uiLength)
                    {
                        ResetState();
                        return STATUS::BUFFER_FULL;
                    }

                    if (!bMetadataOnly_)
                    {
                        clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                        clMyBuffer.erase_begin(stMetaData_.uiLength);
                    }
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                // End of buffer, can't look ahead but there should be more data
                else if (uiMyByteCount + 2 >= clMyBuffer.size())
                {
                    uiMyByteCount--; // If the data lands on the header CRLF then it can be
                                     // missed unless it's tested again when there is more data
                    stMetaData_.uiLength = uiMyByteCount;
                    return STATUS::INCOMPLETE;
                }
                // New line with abbrev data
                else if (clMyBuffer[uiMyByteCount + 1] == OEM4_ABBREV_ASCII_SYNC && clMyBuffer[uiMyByteCount + 2] == OEM4_ABBREV_ASCII_SEPARATOR)
                {
                    uiMyByteCount++; // Add 1 to consume LF
                    eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_BODY;
                }
                // New line with some other data
                else
                {
                    uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData_.uiLength = uiMyByteCount;
                    if (!bMetadataOnly_)
                    {
                        pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                        pclMyCircularDataBuffer->Discard(uiMyByteCount);
                    }
                    ResetState();
                    return STATUS::UNKNOWN;
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                uiMyExpectedMessageLength = 0;
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount;
                if (!bMetadataOnly_)
                {
                    pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                    pclMyCircularDataBuffer->Discard(uiMyByteCount);
                }
                ResetState();
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_BODY:
            // End of buffer (can't look ahead, assume incomplete message)
            if (uiMyByteCount + 3 >= clMyBuffer.size())
            {
                uiMyByteCount--; // If the data lands on the header CRLF then it can be missed
                                 // unless it's tested again when there is more data
                stMetaData_.uiLength = static_cast<uint32_t>(clMyBuffer.size());
                return STATUS::INCOMPLETE;
            }

            // Abbrev Array, more data to follow
            if (IsAbbrevSeparatorCrlf(uiMyByteCount - 1))
            {
                uiMyByteCount += 2; // Consume CRLF

                // New line with non abbrev data
                if (clMyBuffer[uiMyByteCount] != OEM4_ABBREV_ASCII_SYNC
                    // Abbrev data, but is the start of a new message rather than a
                    // continuation of the current message: <NEWMESSAGE
                    || clMyBuffer[uiMyByteCount + 1] != OEM4_ABBREV_ASCII_SEPARATOR)
                {
                    // 0 length arrays will output an empty line which suggests more data will
                    // follow In this case, this is actually the end of the log
                    // rewind back to CR, so we can treat this as end of log.
                    if (IsEmptyAbbrevLine(uiMyByteCount - 3)) { uiMyByteCount--; }
                    else
                    {
                        uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                        uiMyExpectedPayloadLength = 0;
                        uiMyExpectedMessageLength = 0;
                        stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                        stMetaData_.uiLength = uiMyByteCount;
                        if (!bMetadataOnly_)
                        {
                            pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                            pclMyCircularDataBuffer->Discard(uiMyByteCount);
                        }
                        ResetState();
                        return STATUS::UNKNOWN;
                    }
                }
            }

            // End of Abbrev
            if (IsCrlf(uiMyByteCount - 1))
            {
                uiMyByteCount++; // Add 1 to consume LF
                stMetaData_.uiLength = uiMyByteCount;

                if (uiFrameBufferSize_ < stMetaData_.uiLength)
                {
                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    ResetState();
                    return STATUS::BUFFER_FULL;
                }

                if (!bMetadataOnly_)
                {
                    clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                    clMyBuffer.erase_begin(stMetaData_.uiLength);
                }

                uiMyByteCount = 0;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
            }
            else if (uiMyByteCount >= MAX_ABB_ASCII_RESPONSE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                uiMyExpectedMessageLength = 0;
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount;
                if (!bMetadataOnly_)
                {
                    pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                    pclMyCircularDataBuffer->Discard(uiMyByteCount);
                }
                ResetState();
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ASCII_CRC: {
            if (IsAsciiCrc(uiMyByteCount - 1))
            {
                uiMyByteCount--; // rewind back to delimiter before copying
                char acCrc[OEM4_ASCII_CRC_LENGTH + 1];
                for (int32_t i = 0; i < OEM4_ASCII_CRC_LENGTH; i++) { acCrc[i] = clMyBuffer[uiMyByteCount + i]; }
                uiMyByteCount += OEM4_ASCII_CRC_LENGTH + 2; // Add 2 for CRLF
                stMetaData_.uiLength = uiMyByteCount;
                acCrc[OEM4_ASCII_CRC_LENGTH] = '\0';
                uiMyExpectedPayloadLength = 0;

                uint32_t uiMessageCrc;
                auto result = std::from_chars(acCrc, acCrc + OEM4_ASCII_CRC_LENGTH, uiMessageCrc, 16);

                if (result.ec == std::errc() && uiMyCalculatedCrc32 == uiMessageCrc)
                {
                    uiMyByteCount = 0;

                    if (uiFrameBufferSize_ < stMetaData_.uiLength)
                    {
                        ResetState();
                        return STATUS::BUFFER_FULL;
                    }

                    if (!bMetadataOnly_)
                    {
                        clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                        clMyBuffer.erase_begin(stMetaData_.uiLength);
                    }
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                    stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData_.uiLength = uiMyByteCount;
                    if (!bMetadataOnly_)
                    {
                        pclMyCircularDataBuffer->Copy(pucFrameBuffer_, uiMyByteCount);
                        pclMyCircularDataBuffer->Discard(uiMyByteCount);
                    }
                    ResetState();
                    return STATUS::UNKNOWN;
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData_.uiLength = uiMyByteCount;
                if (!bMetadataOnly_)
                {
                    stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData_.uiLength = uiMyByteCount;
                    clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                    clMyBuffer.erase_begin(stMetaData_.uiLength);
                }
                ResetState();
            }
            break;
        }
        case NovAtelFrameState::WAITING_FOR_NMEA_BODY:
            if (ucDataByte == OEM4_ASCII_CRC_DELIMITER) { eMyFrameState = NovAtelFrameState::WAITING_FOR_NMEA_CRC; }
            else if (uiMyByteCount >= MAX_NMEA_MESSAGE_LENGTH)
            {
                uiMyByteCount = NMEA_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                ResetState();
            }
            else { uiMyCalculatedCrc32 ^= ucDataByte; }
            break;

        case NovAtelFrameState::WAITING_FOR_NMEA_CRC: {
            if (ucDataByte == '\n')
            {
                char acCrc[NMEA_CRC_LENGTH + 1];
                const size_t crcStartIndexInBuffer = uiMyByteCount - NMEA_CRC_LENGTH - 2;
                for (int32_t i = 0; i < NMEA_CRC_LENGTH; ++i) { acCrc[i] = clMyBuffer[crcStartIndexInBuffer + i]; }
                acCrc[NMEA_CRC_LENGTH] = '\0';
                uiMyExpectedPayloadLength = 0;

                uint32_t uiMessageCrc;
                auto result = std::from_chars(acCrc, acCrc + NMEA_CRC_LENGTH, uiMessageCrc, 16);

                if (result.ec == std::errc() && uiMyCalculatedCrc32 == uiMessageCrc)
                {
                    uiMyByteCount = 0;

                    if (uiFrameBufferSize_ < stMetaData_.uiLength)
                    {
                        ResetState();
                        return STATUS::BUFFER_FULL;
                    }

                    clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                    clMyBuffer.erase_begin(stMetaData_.uiLength);
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = NMEA_SYNC_LENGTH;
                    ResetState();
                }
                ResetState();
                return STATUS::UNKNOWN;
            }
            break;
        }
        case NovAtelFrameState::WAITING_FOR_JSON_OBJECT:
            if (uiFrameBufferSize_ < uiMyByteCount)
            {
                uiMyByteCount = 0;
                uiMyExpectedPayloadLength = 0;
                ResetState();
                return STATUS::BUFFER_FULL;
            }

            if (ucDataByte == '{') { uiMyJsonObjectOpenBraces++; }
            else if (ucDataByte == '}') { uiMyJsonObjectOpenBraces--; }

            if (uiMyJsonObjectOpenBraces == 0)
            {
                stMetaData_.uiLength = uiMyByteCount;
                if (!bMetadataOnly_)
                {
                    clMyBuffer.copy_out(pucFrameBuffer_, stMetaData_.uiLength);
                    clMyBuffer.erase_begin(stMetaData_.uiLength);
                }
                uiMyByteCount = 0;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
            }
            break;

        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "GetFrame(): Invalid parsing state");
            throw std::runtime_error("GetFrame(): Invalid parsing state");
        }
    }

    ResetState();

    return STATUS::SUCCESS;
}
