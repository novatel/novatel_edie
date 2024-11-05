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

#include "framer.hpp"

#include "framer_manager/api/framer_manager.hpp"
#include "src/decoders/common/api/crc32.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
novatel::edie::oem::Framer::Framer(std::shared_ptr<CircularBuffer> circularBuffer) : FramerBase("novatel_framer", circularBuffer)
{
    // This static instance persists for the lifetime of the program even after leaving scope of this function
    FramerManager& clMyFramerManager = FramerManager::GetInstance();
    clMyFramerManager.RegisterFramer(FRAMER_ID::NOVATEL, std::unique_ptr<FramerBase>(this), std::make_unique<MetaDataStruct>());
}

novatel::edie::oem::Framer::Framer() : Framer::Framer(FramerManager::GetInstance().GetCircularBuffer()) {}
// -------------------------------------------------------------------------------------------------------
bool novatel::edie::oem::Framer::IsAsciiCrc(const uint32_t uiDelimiterPosition_) const
{
    return IsCrlf(uiDelimiterPosition_ + OEM4_ASCII_CRC_LENGTH);
}

// -------------------------------------------------------------------------------------------------------
bool novatel::edie::oem::Framer::IsAbbrevSeparatorCrlf(const uint32_t uiCircularBufferPosition_) const
{
    return IsCrlf(uiCircularBufferPosition_ + 1) && (*pclMyCircularDataBuffer)[uiCircularBufferPosition_] == OEM4_ABBREV_ASCII_SEPARATOR;
}

// -------------------------------------------------------------------------------------------------------
bool novatel::edie::oem::Framer::IsEmptyAbbrevLine(uint32_t uiCircularBufferPosition_) const
{
    while ((*pclMyCircularDataBuffer)[uiCircularBufferPosition_--] == OEM4_ABBREV_ASCII_SEPARATOR)
    {
        if ((*pclMyCircularDataBuffer)[uiCircularBufferPosition_] == OEM4_ABBREV_ASCII_SYNC) { return true; }
    }

    return false;
}

// -------------------------------------------------------------------------------------------------------
bool novatel::edie::oem::Framer::IsAbbrevAsciiResponse() const
{
    constexpr uint32_t errorLen = 5;
    constexpr uint32_t okLen = 2;
    char szResponse[errorLen + 1];

    if (uiMyAbbrevAsciiHeaderPosition + okLen < pclMyCircularDataBuffer->GetLength())
    {
        for (uint32_t i = 0; i < okLen; i++) { szResponse[i] = (*pclMyCircularDataBuffer)[uiMyAbbrevAsciiHeaderPosition + i]; }

        if (strstr(szResponse, "OK")) { return true; }
    }

    if (uiMyAbbrevAsciiHeaderPosition + errorLen < pclMyCircularDataBuffer->GetLength())
    {
        for (uint32_t i = 0; i < errorLen; i++) { szResponse[i] = (*pclMyCircularDataBuffer)[uiMyAbbrevAsciiHeaderPosition + i]; }

        if (strstr(szResponse, "ERROR")) { return true; }
    }

    return false;
}

// -------------------------------------------------------------------------------------------------------
void novatel::edie::oem::Framer::ResetState() { eMyFrameState = NovAtelFrameState::WAITING_FOR_SYNC; }

// -------------------------------------------------------------------------------------------------------
void novatel::edie::oem::Framer::ResetStateAndByteCount()
{
    eMyFrameState = NovAtelFrameState::WAITING_FOR_SYNC;
    uiMyByteCount = 0;
    uiMyExpectedMessageLength = 0;
    uiMyExpectedPayloadLength = 0;
}

// -------------------------------------------------------------------------------------------------------
STATUS novatel::edie::oem::Framer::FindNextSyncByte(unsigned char* pucFrameBuffer_, const uint32_t uiFrameBufferSize_)
{
    if (pucFrameBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    // Loop buffer to complete NovAtel message
    // TODO, this should probably not be COMPLETE_MESSAGE - Think about SYNC_BYTES_FOUND
    enum class HeaderFormat
    {
        UNKNOWN,
        ABB_ASCII
    };

    HeaderFormat headerFormat = HeaderFormat::UNKNOWN;
    while (eMyFrameState != NovAtelFrameState::COMPLETE_MESSAGE)
    {
        // Read data from circular buffer until we reach the end or we didn't find a complete frame in current data buffer
        if (pclMyCircularDataBuffer->GetLength() == uiMyByteCount)
        {
            if (eMyFrameState != NovAtelFrameState::WAITING_FOR_SYNC)
            {
                // If the data lands on the abbreviated header CRLF then it can be missed unless it's tested again when there is more data
                if (headerFormat == HeaderFormat::ABB_ASCII) { uiMyByteCount--; }
                return STATUS::INCOMPLETE;
            }

            headerFormat = HeaderFormat::UNKNOWN;
            //stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
            //stMetaData_.uiLength = uiMyByteCount;

            if (uiMyByteCount == 0) { return STATUS::BUFFER_EMPTY; }

            uiMySyncByteOffset = uiMyByteCount;
            return STATUS::UNKNOWN;
        }

        const unsigned char ucDataByte = (*pclMyCircularDataBuffer)[uiMyByteCount++];

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
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY; 
                return STATUS::SYNC_BYTES_FOUND;
            case OEM4_SHORT_ASCII_SYNC:
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY;
                return STATUS::SYNC_BYTES_FOUND;
            case OEM4_ABBREV_ASCII_SYNC:
                headerFormat = HeaderFormat::ABB_ASCII;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_SYNC2;
                uiMyAbbrevAsciiHeaderPosition = uiMyByteCount;
                break;
            case '{':
                if (bMyFrameJson)
                {
                    eMyFrameState = NovAtelFrameState::WAITING_FOR_JSON_OBJECT;
                    uiMyJsonObjectOpenBraces++;
                }
                break;
            default: break;
            }

            // If we have just encountered a sync byte and have read bytes before, we need to handle them
            if (eMyFrameState != NovAtelFrameState::WAITING_FOR_SYNC && uiMyByteCount > 1)
            {
                // stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                // stMetaData_.uiLength = uiMyByteCount - 1;
                uiMySyncByteOffset = uiMyByteCount - 1;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                return STATUS::UNKNOWN;
            }
            if (uiMyByteCount > uiFrameBufferSize_)
            {
                // stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                // stMetaData_.uiLength = uiMyByteCount - 1;
                uiMySyncByteOffset = uiFrameBufferSize_;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC2:
            switch (ucDataByte)
            {
                // TODO: Figure out how to deal with this
                // case OEM4_PROPRIETARY_BINARY_SYNC2: stMetaData_.eFormat = HEADER_FORMAT::PROPRIETARY_BINARY; [[fallthrough]];

            case OEM4_BINARY_SYNC2:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC3;
                break;
            default:
                uiMySyncByteOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                ResetState();
                uiMyByteCount--;
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC3:
            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC3:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);

                // TODO: Figure out how to deal with this
                // if (stMetaData_.eFormat != HEADER_FORMAT::PROPRIETARY_BINARY) { stMetaData_.eFormat = HEADER_FORMAT::BINARY; }
                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_HEADER;
                uiMySyncByteOffset = uiMyByteCount;
                return STATUS::SYNC_BYTES_FOUND;
            case OEM4_SHORT_BINARY_SYNC3:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);

                // TODO: Figure out how to deal with this
                // stMetaData_.eFormat = HEADER_FORMAT::SHORT_BINARY;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_SHORT_BINARY_HEADER;
                uiMySyncByteOffset = uiMyByteCount;
                return STATUS::SYNC_BYTES_FOUND;
            default:
                uiMySyncByteOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                ResetState();
                uiMyByteCount--;
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_SYNC2:
            if (ucDataByte != OEM4_ABBREV_ASCII_SEPARATOR && isalpha(ucDataByte))
            {
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_HEADER;
                uiMySyncByteOffset = uiMyByteCount - 2;
                return STATUS::SYNC_BYTES_FOUND;
            }
            else
            {
                // stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
                uiMySyncByteOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                ResetState();
                uiMyByteCount--;
            }
            break;
        }
    }
}

// -------------------------------------------------------------------------------------------------------
STATUS
novatel::edie::oem::Framer::GetFrame(unsigned char* pucFrameBuffer_, const uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_)
{
    MetaDataStruct& stMetData = dynamic_cast<MetaDataStruct&>(stMetaData_);

    if (pucFrameBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    // Loop buffer to complete NovAtel message
    while (eMyFrameState != NovAtelFrameState::COMPLETE_MESSAGE)
    {
        stMetData.bResponse = false;

        // Read data from circular buffer until we reach the end or we didn't find a complete frame in current data buffer
        if (pclMyCircularDataBuffer->GetLength() == uiMyByteCount)
        {
            if (eMyFrameState != NovAtelFrameState::WAITING_FOR_SYNC)
            {
                // If the data lands on the abbreviated header CRLF then it can be missed unless it's tested again when there is more data
                if (stMetData.eFormat == HEADER_FORMAT::ABB_ASCII) { uiMyByteCount--; }
                return STATUS::INCOMPLETE;
            }

            stMetData.eFormat = HEADER_FORMAT::UNKNOWN;
            stMetData.uiLength = uiMyByteCount;

            if (uiMyByteCount == 0) { return STATUS::BUFFER_EMPTY; }

            uiMySyncByteOffset = uiMyByteCount;
            return STATUS::UNKNOWN;
        }

        const unsigned char ucDataByte = (*pclMyCircularDataBuffer)[uiMyByteCount++];
        stMetData.uiLength = uiMyByteCount;

        // non-ASCII characters in an ASCII message indicates a corrupt log or unknown data. Either way, mark the data as unknown
        if ((stMetData.eFormat == HEADER_FORMAT::ASCII || stMetData.eFormat == HEADER_FORMAT::SHORT_ASCII ||
             stMetData.eFormat == HEADER_FORMAT::ABB_ASCII || stMetData.eFormat == HEADER_FORMAT::NMEA || stMetData.eFormat == HEADER_FORMAT::JSON) &&
            ucDataByte > 127)
        {
            stMetData.eFormat = HEADER_FORMAT::UNKNOWN;
            ResetState();
            uiMyByteCount--;
        }

        switch (eMyFrameState)
        {
            // case NovAtelFrameState::WAITING_FOR_SYNC:
            //     uiMyCalculatedCrc32 = 0;

            //    switch (ucDataByte)
            //    {
            //    case OEM4_BINARY_SYNC1:
            //        CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
            //        eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC2;
            //        break;
            //    case OEM4_ASCII_SYNC:
            //        stMetData.eFormat = HEADER_FORMAT::ASCII;
            //        eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY;
            //        break;
            //    case OEM4_SHORT_ASCII_SYNC:
            //        stMetData.eFormat = HEADER_FORMAT::SHORT_ASCII;
            //        eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY;
            //        break;
            //        // case NMEA_SYNC:
            //        //     stMetData.eFormat = HEADER_FORMAT::NMEA;
            //        //     eMyFrameState = NovAtelFrameState::WAITING_FOR_NMEA_BODY;
            //        break;
            //    case OEM4_ABBREV_ASCII_SYNC:
            //        stMetData.eFormat = HEADER_FORMAT::ABB_ASCII;
            //        eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_SYNC2;
            //        uiMyAbbrevAsciiHeaderPosition = uiMyByteCount;
            //        break;
            //    case '{':
            //        if (bMyFrameJson)
            //        {
            //            eMyFrameState = NovAtelFrameState::WAITING_FOR_JSON_OBJECT;
            //            stMetData.eFormat = HEADER_FORMAT::JSON;
            //            uiMyJsonObjectOpenBraces++;
            //        }
            //        break;
            //    default: break;
            //    }

            //    // If we have just encountered a sync byte and have read bytes before, we need to handle them
            //    if (eMyFrameState != NovAtelFrameState::WAITING_FOR_SYNC && uiMyByteCount > 1)
            //    {
            //        stMetData.eFormat = HEADER_FORMAT::UNKNOWN;
            //        stMetData.uiLength = uiMyByteCount - 1;
            //        uiMySyncByteOffset = uiMyByteCount - 1;
            //        return STATUS::UNKNOWN;
            //    }
            //    if (uiMyByteCount > uiFrameBufferSize_)
            //    {
            //        stMetData.eFormat = HEADER_FORMAT::UNKNOWN;
            //        stMetData.uiLength = uiMyByteCount - 1;
            //        uiMySyncByteOffset = uiFrameBufferSize_;
            //        return STATUS::UNKNOWN;
            //    }
            //    break;

            // case NovAtelFrameState::WAITING_FOR_BINARY_SYNC2:
            //     switch (ucDataByte)
            //     {
            //     case OEM4_PROPRIETARY_BINARY_SYNC2: stMetData.eFormat = HEADER_FORMAT::PROPRIETARY_BINARY; [[fallthrough]];
            //     case OEM4_BINARY_SYNC2:
            //         CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
            //         eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC3;
            //         break;
            //     default:
            //         stMetData.eFormat = HEADER_FORMAT::UNKNOWN;
            //         ResetState();
            //         uiMyByteCount--;
            //     }
            //     break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC3:
            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC3:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                if (stMetData.eFormat != HEADER_FORMAT::PROPRIETARY_BINARY) { stMetData.eFormat = HEADER_FORMAT::BINARY; }
                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_HEADER;
                uiMySyncByteOffset = uiMyByteCount;
                return STATUS::SYNC_BYTES_FOUND;
            case OEM4_SHORT_BINARY_SYNC3:
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
                stMetData.eFormat = HEADER_FORMAT::SHORT_BINARY;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_SHORT_BINARY_HEADER;
                uiMySyncByteOffset = uiMyByteCount;
                return STATUS::SYNC_BYTES_FOUND;
            default:
                stMetData.eFormat = HEADER_FORMAT::UNKNOWN;
                ResetState();
                uiMyByteCount--;
                break;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_SYNC2:
            if (ucDataByte != OEM4_ABBREV_ASCII_SEPARATOR && isalpha(ucDataByte))
            {
                eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_HEADER;
                uiMySyncByteOffset = uiMyByteCount - 2;
                return STATUS::SYNC_BYTES_FOUND;
            }
            else
            {
                stMetData.eFormat = HEADER_FORMAT::UNKNOWN;
                ResetState();
                uiMyByteCount--;
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

                Oem4BinaryHeader stOem4BinaryHeader;
                pclMyCircularDataBuffer->Copy(reinterpret_cast<unsigned char*>(&stOem4BinaryHeader), OEM4_BINARY_HEADER_LENGTH);
                uiMyExpectedPayloadLength = static_cast<uint32_t>(stOem4BinaryHeader.usLength);
                uiMyExpectedMessageLength = OEM4_BINARY_HEADER_LENGTH + static_cast<uint32_t>(stOem4BinaryHeader.usLength) + OEM4_BINARY_CRC_LENGTH;

                if (uiMyExpectedPayloadLength > MAX_BINARY_MESSAGE_LENGTH || uiMyExpectedMessageLength > MAX_BINARY_MESSAGE_LENGTH)
                {
                    uiMyByteCount = OEM4_BINARY_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    ResetState();
                    break;
                }

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetData.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;
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

                Oem4BinaryShortHeader stOem4BinaryShortHeader;
                pclMyCircularDataBuffer->Copy(reinterpret_cast<unsigned char*>(&stOem4BinaryShortHeader), OEM4_SHORT_BINARY_HEADER_LENGTH);
                uiMyExpectedPayloadLength = static_cast<uint32_t>(stOem4BinaryShortHeader.ucLength);
                uiMyExpectedMessageLength = OEM4_SHORT_BINARY_HEADER_LENGTH + OEM4_BINARY_CRC_LENGTH + stOem4BinaryShortHeader.ucLength;

                if (uiMyExpectedPayloadLength > MAX_SHORT_BINARY_MESSAGE_LENGTH || uiMyExpectedMessageLength > MAX_SHORT_BINARY_MESSAGE_LENGTH)
                {
                    uiMyByteCount = OEM4_SHORT_BINARY_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
                    ResetState();
                    break;
                }

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetData.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;
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
                        stMetData.uiLength = uiMyExpectedPayloadLength;
                        pclMyCircularDataBuffer->Discard((uiMyExpectedMessageLength - uiMyExpectedPayloadLength) + OEM4_BINARY_CRC_LENGTH);
                        pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetData.uiLength);
                        pclMyCircularDataBuffer->Discard(uiMyExpectedPayloadLength + OEM4_BINARY_CRC_LENGTH);
                    }
                    else
                    {
                        pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetData.uiLength);
                        pclMyCircularDataBuffer->Discard(stMetData.uiLength);
                    }

                    uiMyByteCount = 0;
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = stMetData.eFormat == HEADER_FORMAT::BINARY ? OEM4_BINARY_SYNC_LENGTH : OEM4_SHORT_BINARY_SYNC_LENGTH;
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
                if (uiMyByteCount + OEM4_ASCII_CRC_LENGTH + 2 > pclMyCircularDataBuffer->GetLength())
                {
                    uiMyByteCount--; // Rewind so that we reprocess the '*' delimiter after getting more bytes
                    return STATUS::INCOMPLETE;
                }

                // Handle RXCONFIGA logs: Normally * signifies the start of the CRC, but
                // RXCONFIG payload has an internal CRC that we need to treat as part of the log
                // payload.

                //                   |<----------------rxconfig payload---------------->|
                // #RXCONFIGA,HEADER;#command,command header;command payload*command crc*CRC
                //                                         internal CRC   |<------->|

                // Check for a second CRC delimiter which indicates this is RXCONFIG
                if ((*pclMyCircularDataBuffer)[uiMyByteCount + OEM4_ASCII_CRC_LENGTH] == OEM4_ASCII_CRC_DELIMITER
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
                ResetState();
            }
            else { CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte); }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_HEADER:
            if (IsCrlf(uiMyByteCount - 1))
            {
                if (IsAbbrevAsciiResponse())
                {
                    stMetData.uiLength = uiMyByteCount + 1; // Add 1 to consume LF
                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    stMetData.bResponse = true;

                    if (uiFrameBufferSize_ < stMetData.uiLength)
                    {
                        ResetState();
                        return STATUS::BUFFER_FULL;
                    }

                    pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetData.uiLength);
                    pclMyCircularDataBuffer->Discard(stMetData.uiLength);
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                // End of buffer, can't look ahead but there should be more data
                else if (uiMyByteCount + 2 >= pclMyCircularDataBuffer->GetLength())
                {
                    uiMyByteCount--; // If the data lands on the header CRLF then it can be
                                     // missed unless it's tested again when there is more data
                    stMetData.uiLength = uiMyByteCount;
                    return STATUS::INCOMPLETE;
                }
                // New line with abbrev data
                else if ((*pclMyCircularDataBuffer)[uiMyByteCount + 1] == OEM4_ABBREV_ASCII_SYNC &&
                         (*pclMyCircularDataBuffer)[uiMyByteCount + 2] == OEM4_ABBREV_ASCII_SEPARATOR)
                {
                    uiMyByteCount++; // Add 1 to consume LF
                    eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_BODY;
                }
                // New line with some other data
                else
                {
                    uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                    uiMyExpectedPayloadLength = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    ResetState();
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;
                ResetState();
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_BODY:
            // End of buffer (can't look ahead, assume incomplete message)
            if (uiMyByteCount + 3 >= pclMyCircularDataBuffer->GetLength())
            {
                uiMyByteCount--; // If the data lands on the header CRLF then it can be missed
                                 // unless it's tested again when there is more data
                stMetData.uiLength = pclMyCircularDataBuffer->GetLength();
                return STATUS::INCOMPLETE;
            }

            // Abbrev Array, more data to follow
            if (IsAbbrevSeparatorCrlf(uiMyByteCount - 1))
            {
                uiMyByteCount += 2; // Consume CRLF

                // New line with non abbrev data
                if ((*pclMyCircularDataBuffer)[uiMyByteCount] != OEM4_ABBREV_ASCII_SYNC
                    // Abbrev data, but is the start of a new message rather than a
                    // continuation of the current message: <NEWMESSAGE
                    || (*pclMyCircularDataBuffer)[uiMyByteCount + 1] != OEM4_ABBREV_ASCII_SEPARATOR)
                {
                    // 0 length arrays will output an empty line which suggests more data will
                    // follow In this case, this is actually the end of the log
                    // rewind back to CR, so we can treat this as end of log.
                    if (IsEmptyAbbrevLine(uiMyByteCount - 3)) { uiMyByteCount--; }
                    else
                    {
                        uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                        uiMyAbbrevAsciiHeaderPosition = 0;
                        uiMyExpectedPayloadLength = 0;
                        ResetState();
                    }
                }
            }

            // End of Abbrev
            if (IsCrlf(uiMyByteCount - 1))
            {
                uiMyByteCount++; // Add 1 to consume LF
                stMetData.uiLength = uiMyByteCount;

                if (uiFrameBufferSize_ < stMetData.uiLength)
                {
                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    ResetState();
                    return STATUS::BUFFER_FULL;
                }

                pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetData.uiLength);
                pclMyCircularDataBuffer->Discard(stMetData.uiLength);

                uiMyByteCount = 0;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;
                eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
            }
            else if (uiMyByteCount >= MAX_ABB_ASCII_RESPONSE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;
                ResetState();
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ASCII_CRC: {
            if (IsAsciiCrc(uiMyByteCount - 1))
            {
                uiMyByteCount--; // rewind back to delimiter before copying
                char acCrc[OEM4_ASCII_CRC_LENGTH + 1];
                for (int32_t i = 0; i < OEM4_ASCII_CRC_LENGTH; i++) { acCrc[i] = (*pclMyCircularDataBuffer)[uiMyByteCount++]; }
                uiMyByteCount += 2; // Add 2 for CRLF
                stMetData.uiLength = uiMyByteCount;
                acCrc[OEM4_ASCII_CRC_LENGTH] = '\0';
                uiMyExpectedPayloadLength = 0;

                uint32_t uiMessageCrc;
                if (sscanf(acCrc, "%x", &uiMessageCrc) > 0 && uiMyCalculatedCrc32 == uiMessageCrc)
                {
                    uiMyByteCount = 0;

                    if (uiFrameBufferSize_ < stMetData.uiLength)
                    {
                        ResetState();
                        return STATUS::BUFFER_FULL;
                    }

                    pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetData.uiLength);
                    pclMyCircularDataBuffer->Discard(stMetData.uiLength);
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                    ResetState();
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                ResetState();
            }
            break;
        }
        // case NovAtelFrameState::WAITING_FOR_NMEA_BODY:
        //     if (ucDataByte == OEM4_ASCII_CRC_DELIMITER) { eMyFrameState = NovAtelFrameState::WAITING_FOR_NMEA_CRC; }
        //     else if (uiMyByteCount >= MAX_NMEA_MESSAGE_LENGTH)
        //     {
        //         uiMyByteCount = NMEA_SYNC_LENGTH;
        //         uiMyExpectedPayloadLength = 0;
        //         ResetState();
        //     }
        //     else { uiMyCalculatedCrc32 ^= ucDataByte; }
        //     break;

        // case NovAtelFrameState::WAITING_FOR_NMEA_CRC: {
        //     if (ucDataByte == '\n')
        //     {
        //         uiMyExpectedPayloadLength = 0;
        //         char acCrc[NMEA_CRC_LENGTH + 1];
        //         for (int32_t iOffset = NMEA_CRC_LENGTH; iOffset > 0; iOffset--)
        //         {
        //             acCrc[NMEA_CRC_LENGTH - iOffset] = (*pclMyCircularDataBuffer)[uiMyByteCount - iOffset - 2];
        //         }
        //         acCrc[NMEA_CRC_LENGTH] = '\0';

        //        uint32_t uiMessageCrc;
        //        if (sscanf(acCrc, "%x", &uiMessageCrc) > 0 && uiMyCalculatedCrc32 == uiMessageCrc)
        //        {
        //            stMetData.uiLength = uiMyByteCount;

        //            if (uiFrameBufferSize_ < stMetData.uiLength)
        //            {
        //                uiMyByteCount = 0;
        //                ResetState();
        //                return STATUS::BUFFER_FULL;
        //            }

        //            pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetData.uiLength);
        //            pclMyCircularDataBuffer->Discard(stMetData.uiLength);
        //            uiMyByteCount = 0;
        //            eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
        //        }
        //        else
        //        {
        //            uiMyByteCount = NMEA_SYNC_LENGTH;
        //            ResetState();
        //        }
        //    }
        //    else if (uiMyByteCount >= MAX_NMEA_MESSAGE_LENGTH)
        //    {
        //        uiMyByteCount = NMEA_SYNC_LENGTH;
        //        uiMyExpectedPayloadLength = 0;
        //        ResetState();
        //    }
        //    break;
        //}
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
                stMetData.uiLength = uiMyByteCount;
                pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetData.uiLength);
                pclMyCircularDataBuffer->Discard(stMetData.uiLength);
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
