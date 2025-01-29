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

#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/common/framer_manager.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
Framer::Framer(std::shared_ptr<CircularBuffer> circularBuffer) : FramerBase("novatel_framer", circularBuffer)
{
    // This static instance persists for the lifetime of the program even after leaving scope of this function
    FramerManager& clMyFramerManager = FramerManager::GetInstance();
    auto it_registered = std::find_if(clMyFramerManager.framerRegistry.begin(), clMyFramerManager.framerRegistry.end(),
                                      [](const FramerElement& element) { return element.framerId == FRAMER_ID::NOVATEL; });
    if (it_registered != clMyFramerManager.framerRegistry.end()) { throw std::runtime_error("Framer already registered"); }

    clMyFramerManager.RegisterFramer(FRAMER_ID::NOVATEL, std::unique_ptr<Framer>(this), std::make_unique<MetaDataStruct>());
}

Framer::Framer() : Framer::Framer(FramerManager::GetInstance().GetCircularBuffer()) {}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsAsciiCrc(const uint32_t uiDelimiterPosition_) const { return IsCrlf(uiDelimiterPosition_ + OEM4_ASCII_CRC_LENGTH); }

// -------------------------------------------------------------------------------------------------------
bool Framer::IsAbbrevSeparatorCrlf(const uint32_t uiCircularBufferPosition_) const
{
    return IsCrlf(uiCircularBufferPosition_ + 1) && (*pclMyCircularDataBuffer)[uiCircularBufferPosition_] == OEM4_ABBREV_ASCII_SEPARATOR;
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsEmptyAbbrevLine(uint32_t uiCircularBufferPosition_) const
{
    while ((*pclMyCircularDataBuffer)[uiCircularBufferPosition_--] == OEM4_ABBREV_ASCII_SEPARATOR)
    {
        if ((*pclMyCircularDataBuffer)[uiCircularBufferPosition_] == OEM4_ABBREV_ASCII_SYNC) { return true; }
    }

    return false;
}

// -------------------------------------------------------------------------------------------------------
bool Framer::IsAbbrevAsciiResponse() const
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
void Framer::ResetState()
{
    eMyFrameState = NovAtelFrameState::WAITING_FOR_SYNC;
    eMyCurrentFramerStatus = STATUS::UNKNOWN;
}

// -------------------------------------------------------------------------------------------------------
void Framer::ResetStateAndByteCount()
{
    uiMyByteCount = 0;
    uiMyExpectedMessageLength = 0;
    uiMyExpectedPayloadLength = 0;
    uiMyFrameBufferOffset = 0;
    ResetState();
}

// -------------------------------------------------------------------------------------------------------
void Framer::FindNextSyncByte(const uint32_t uiFrameBufferSize_)
{
    NovAtelFrameState localFrameState = NovAtelFrameState::WAITING_FOR_SYNC;

    while (localFrameState != NovAtelFrameState::COMPLETE_MESSAGE)
    {
        // Read data from circular buffer until end is reached
        if (pclMyCircularDataBuffer->GetLength() == uiMyByteCount)
        {

            uiMyFrameBufferOffset = uiMyByteCount;
            uiMyByteCount = 0;
            // We have found a potential partial log, but we need more data to complete it
            if (localFrameState != NovAtelFrameState::WAITING_FOR_SYNC)
            {
                // If the data lands on the abbreviated header CRLF then it can be missed unless it's tested again when there is more data
                eMyCurrentFramerStatus = STATUS::INCOMPLETE;
                return;
            }
            eMyCurrentFramerStatus = STATUS::UNKNOWN;
            return;
        }

        const unsigned char ucDataByte = (*pclMyCircularDataBuffer)[uiMyByteCount++];

        switch (localFrameState)
        {
        case NovAtelFrameState::WAITING_FOR_SYNC:
            uiMyCalculatedCrc32 = 0;

            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC1: localFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC2; break;
            case OEM4_ASCII_SYNC:
                uiMyByteCount--;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::SYNC_BYTES_FOUND;
                return;
            case OEM4_SHORT_ASCII_SYNC:
                uiMyByteCount--;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::SYNC_BYTES_FOUND;
                return;
            case OEM4_ABBREV_ASCII_SYNC:
                uiMyByteCount--;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::SYNC_BYTES_FOUND;
                return;
            case '{':
                if (bMyFrameJson)
                {
                    uiMyByteCount--;
                    uiMyFrameBufferOffset = uiMyByteCount;
                    eMyCurrentFramerStatus = STATUS::SYNC_BYTES_FOUND;
                    return;
                }
                break;
            default: break;
            }

            // If we have just encountered a sync byte and have read bytes before, we need to handle them
            if (localFrameState != NovAtelFrameState::WAITING_FOR_SYNC && uiMyByteCount > 1)
            {
                uiMyByteCount--;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                return;
            }
            if (uiMyByteCount > uiFrameBufferSize_)
            {
                uiMyFrameBufferOffset = uiFrameBufferSize_;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                return;
            }
            if (uiMyByteCount == 1 && localFrameState == NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY)
            {
                uiMyByteCount = uiMyByteCount - 1;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                return;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC2:
            switch (ucDataByte)
            {
            case OEM4_PROPRIETARY_BINARY_SYNC2: [[fallthrough]];
            case OEM4_BINARY_SYNC2: localFrameState = NovAtelFrameState::WAITING_FOR_BINARY_SYNC3; break;
            default:
                uiMyFrameBufferOffset = uiMyByteCount;
                uiMyByteCount--;
                uiMyFrameBufferOffset = uiMyByteCount;
                localFrameState = NovAtelFrameState::WAITING_FOR_SYNC;
                break;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_BINARY_SYNC3:
            switch (ucDataByte)
            {
            case OEM4_BINARY_SYNC3:
                uiMyByteCount = uiMyByteCount - 3;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::SYNC_BYTES_FOUND;
                return;
            case OEM4_SHORT_BINARY_SYNC3:
                uiMyByteCount = uiMyByteCount - 3;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::SYNC_BYTES_FOUND;
                return;
            default:
                uiMyByteCount--;
                uiMyFrameBufferOffset = uiMyByteCount;
                localFrameState = NovAtelFrameState::WAITING_FOR_SYNC;
                break;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_SYNC2:
            if (ucDataByte != OEM4_ABBREV_ASCII_SEPARATOR && isalpha(ucDataByte))
            {
                uiMyByteCount = uiMyByteCount - 2;
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::SYNC_BYTES_FOUND;
                return;
            }
            else
            {
                //   If we get to a WAITING_FOR_ABB_ASCII_SYNC2, it means we got the first byte of the header
                //   and if it's not immediately followed by sync2, it's not a valid header -> therefore unknown bytes of offset = count
                uiMyFrameBufferOffset = uiMyByteCount;
                eMyCurrentFramerStatus = STATUS::UNKNOWN;
                return;
            }
            break;
        }
    }
}

// -------------------------------------------------------------------------------------------------------
STATUS
Framer::GetFrame(unsigned char* pucFrameBuffer_, const uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_)
{
    MetaDataStruct& stMetaData = dynamic_cast<MetaDataStruct&>(stMetaData_);

    // Trust Relationship: FindNextSyncByte leads to this function, so we can trust that the first byte is the start of a sync byte
    unsigned char ucDataByte = (*pclMyCircularDataBuffer)[uiMyByteCount];
    stMetaData.uiLength = uiMyByteCount;

    if (stMetaData.eFormat == HEADER_FORMAT::UNKNOWN)
    {
        uiMyCalculatedCrc32 = 0;
        switch (ucDataByte)
        {
        case OEM4_BINARY_SYNC1:
            if ((*pclMyCircularDataBuffer)[uiMyByteCount + 1] == OEM4_PROPRIETARY_BINARY_SYNC2)
            {
                stMetaData.eFormat = HEADER_FORMAT::PROPRIETARY_BINARY;
            }
            if ((*pclMyCircularDataBuffer)[uiMyByteCount + 2] == OEM4_BINARY_SYNC3)
            {
                if (stMetaData.eFormat != HEADER_FORMAT::PROPRIETARY_BINARY) { stMetaData.eFormat = HEADER_FORMAT::BINARY; }
                eMyFrameState = NovAtelFrameState::WAITING_FOR_BINARY_HEADER;
            }
            if ((*pclMyCircularDataBuffer)[uiMyByteCount + 2] == OEM4_SHORT_BINARY_SYNC3)
            {
                stMetaData.eFormat = HEADER_FORMAT::SHORT_BINARY;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_SHORT_BINARY_HEADER;
            }
            for (auto i = 0; i < 3; i++)
            {
                const unsigned char ucDataByte = (*pclMyCircularDataBuffer)[uiMyByteCount++];
                CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);
            }

            break;

        case OEM4_ASCII_SYNC:
            stMetaData.eFormat = HEADER_FORMAT::ASCII;
            eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY;
            uiMyByteCount++;
            break;

        case OEM4_SHORT_ASCII_SYNC:
            stMetaData.eFormat = HEADER_FORMAT::SHORT_ASCII;
            eMyFrameState = NovAtelFrameState::WAITING_FOR_ASCII_HEADER_AND_BODY;
            uiMyByteCount++;
            break;
        case OEM4_ABBREV_ASCII_SYNC:
            stMetaData.eFormat = HEADER_FORMAT::ABB_ASCII;
            eMyFrameState = NovAtelFrameState::WAITING_FOR_ABB_ASCII_HEADER;
            uiMyByteCount++;
            uiMyAbbrevAsciiHeaderPosition = uiMyByteCount;
            break;
        case '{':
            if (bMyFrameJson)
            {
                stMetaData.eFormat = HEADER_FORMAT::JSON;
                eMyFrameState = NovAtelFrameState::WAITING_FOR_JSON_OBJECT;
                uiMyJsonObjectOpenBraces++;
            }
            break;
        default: break;
        }
    }

    stMetaData.uiLength = uiMyByteCount;

    if (pucFrameBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    // Loop buffer to complete NovAtel message
    while (eMyFrameState != NovAtelFrameState::COMPLETE_MESSAGE)
    {
        stMetaData.bResponse = false;

        // Read data from circular buffer until end is reached
        if (pclMyCircularDataBuffer->GetLength() == uiMyByteCount)
        {
            // We have found a potential partial log, but we need more data to complete it
            if (eMyFrameState != NovAtelFrameState::WAITING_FOR_SYNC)
            {
                // If the data lands on the abbreviated header CRLF then it can be missed unless it's tested again when there is more data
                if (stMetaData.eFormat == HEADER_FORMAT::ABB_ASCII) { uiMyByteCount--; }
                return STATUS::INCOMPLETE;
            }

            stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
            stMetaData.uiLength = uiMyByteCount;

            if (uiMyByteCount == 0) { return STATUS::BUFFER_EMPTY; }

            uiMyFrameBufferOffset = uiMyByteCount;
            stMetaData.uiLength = uiMyByteCount;
            return STATUS::UNKNOWN;
        }

        ucDataByte = (*pclMyCircularDataBuffer)[uiMyByteCount++];
        stMetaData.uiLength = uiMyByteCount;

        // non-ASCII characters in an ASCII message indicates a corrupt log or unknown data. Either way, mark the data as unknown
        if ((stMetaData.eFormat == HEADER_FORMAT::ASCII || stMetaData.eFormat == HEADER_FORMAT::SHORT_ASCII ||
             stMetaData.eFormat == HEADER_FORMAT::ABB_ASCII || stMetaData.eFormat == HEADER_FORMAT::NMEA ||
             stMetaData.eFormat == HEADER_FORMAT::JSON) &&
            ucDataByte > 127)
        {
            stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;

            uiMyByteCount--;
            uiMyFrameBufferOffset = uiMyByteCount;
            stMetaData.uiLength = uiMyByteCount;
            return STATUS::UNKNOWN;
        }

        switch (eMyFrameState)
        {
        case NovAtelFrameState::WAITING_FOR_BINARY_HEADER: {
            CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte);

            if (uiMyByteCount == OEM4_BINARY_HEADER_LENGTH)
            {
                if (uiFrameBufferSize_ < OEM4_BINARY_HEADER_LENGTH)
                {
                    uiMyByteCount = 0;
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
                    stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData.uiLength = uiMyByteCount;
                    uiMyFrameBufferOffset = uiMyByteCount;
                    return STATUS::UNKNOWN;
                }

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetaData.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;
                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
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
                    stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData.uiLength = uiMyByteCount;
                    uiMyFrameBufferOffset = uiMyByteCount;
                    return STATUS::UNKNOWN;
                    break;
                }

                if ((bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedPayloadLength) ||
                    (!bMyPayloadOnly && uiFrameBufferSize_ < uiMyExpectedMessageLength))
                {
                    stMetaData.uiLength = bMyPayloadOnly ? uiMyExpectedPayloadLength : uiMyExpectedMessageLength;
                    uiMyByteCount = 0;
                    uiMyExpectedPayloadLength = 0;
                    uiMyExpectedMessageLength = 0;
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
                        stMetaData.uiLength = uiMyExpectedPayloadLength + OEM4_BINARY_CRC_LENGTH;
                        pclMyCircularDataBuffer->Discard(uiMyExpectedMessageLength - uiMyExpectedPayloadLength);
                    }

                    uiMyByteCount = 0;
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = stMetaData.eFormat == HEADER_FORMAT::BINARY ? OEM4_BINARY_SYNC_LENGTH : OEM4_SHORT_BINARY_SYNC_LENGTH;
                    stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData.uiLength = uiMyByteCount;
                    uiMyFrameBufferOffset = uiMyByteCount;
                    return STATUS::UNKNOWN;
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

                stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData.uiLength = uiMyByteCount;
                uiMyFrameBufferOffset = uiMyByteCount;
                return STATUS::UNKNOWN;
            }
            else { CalculateCharacterCrc32(uiMyCalculatedCrc32, ucDataByte); }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_HEADER:
            stMetaData.eFormat = HEADER_FORMAT::ABB_ASCII;
            if (IsCrlf(uiMyByteCount - 1))
            {
                if (IsAbbrevAsciiResponse())
                {
                    stMetaData.uiLength = uiMyByteCount + 1; // Add 1 to consume LF
                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    stMetaData.bResponse = true;

                    if (uiFrameBufferSize_ < stMetaData.uiLength) { return STATUS::BUFFER_FULL; }

                    // pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetaData.uiLength);
                    // pclMyCircularDataBuffer->Discard(stMetaData.uiLength);
                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                // End of buffer, can't look ahead but there should be more data
                else if (uiMyByteCount + 2 >= pclMyCircularDataBuffer->GetLength())
                {
                    uiMyByteCount--; // If the data lands on the header CRLF then it can be
                                     // missed unless it's tested again when there is more data
                    stMetaData.uiLength = uiMyByteCount;
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

                    stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData.uiLength = uiMyByteCount;
                    uiMyFrameBufferOffset = uiMyByteCount;
                    return STATUS::UNKNOWN;
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyAbbrevAsciiHeaderPosition = 0;
                uiMyExpectedPayloadLength = 0;

                stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData.uiLength = uiMyByteCount;
                uiMyFrameBufferOffset = uiMyByteCount;
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ABB_ASCII_BODY:
            // End of buffer (can't look ahead, assume incomplete message)
            if (uiMyByteCount + 3 >= pclMyCircularDataBuffer->GetLength())
            {
                uiMyByteCount--; // If the data lands on the header CRLF then it can be missed
                                 // unless it's tested again when there is more data
                stMetaData.uiLength = pclMyCircularDataBuffer->GetLength();
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

                        stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                        stMetaData.uiLength = uiMyByteCount;
                        uiMyFrameBufferOffset = uiMyByteCount;
                        return STATUS::UNKNOWN;
                    }
                }
            }

            // End of Abbrev
            if (IsCrlf(uiMyByteCount - 1))
            {
                uiMyByteCount++; // Add 1 to consume LF
                stMetaData.uiLength = uiMyByteCount;

                if (uiFrameBufferSize_ < stMetaData.uiLength)
                {
                    uiMyByteCount = 0;
                    uiMyAbbrevAsciiHeaderPosition = 0;
                    uiMyExpectedPayloadLength = 0;
                    return STATUS::BUFFER_FULL;
                }

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

                stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData.uiLength = uiMyByteCount;
                uiMyFrameBufferOffset = uiMyByteCount;
                return STATUS::UNKNOWN;
            }
            break;

        case NovAtelFrameState::WAITING_FOR_ASCII_CRC: {
            if (IsAsciiCrc(uiMyByteCount - 1))
            {
                uiMyByteCount--; // rewind back to delimiter before copying
                char acCrc[OEM4_ASCII_CRC_LENGTH + 1];
                for (int32_t i = 0; i < OEM4_ASCII_CRC_LENGTH; i++) { acCrc[i] = (*pclMyCircularDataBuffer)[uiMyByteCount++]; }
                uiMyByteCount += 2; // Add 2 for CRLF
                stMetaData.uiLength = uiMyByteCount;
                acCrc[OEM4_ASCII_CRC_LENGTH] = '\0';
                uiMyExpectedPayloadLength = 0;

                uint32_t uiMessageCrc;
                if (sscanf(acCrc, "%x", &uiMessageCrc) > 0 && uiMyCalculatedCrc32 == uiMessageCrc)
                {
                    uiMyByteCount = 0;

                    if (uiFrameBufferSize_ < stMetaData.uiLength) { return STATUS::BUFFER_FULL; }

                    eMyFrameState = NovAtelFrameState::COMPLETE_MESSAGE;
                }
                else
                {
                    uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                    stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                    stMetaData.uiLength = uiMyByteCount;
                    uiMyFrameBufferOffset = uiMyByteCount;
                    return STATUS::UNKNOWN;
                }
            }
            else if (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH)
            {
                uiMyByteCount = OEM4_ASCII_SYNC_LENGTH;
                uiMyExpectedPayloadLength = 0;
                stMetaData.eFormat = HEADER_FORMAT::UNKNOWN;
                stMetaData.uiLength = uiMyByteCount;
                uiMyFrameBufferOffset = uiMyByteCount;
                return STATUS::UNKNOWN;
            }
            break;
        }

        case NovAtelFrameState::WAITING_FOR_JSON_OBJECT:
            if (uiFrameBufferSize_ < uiMyByteCount)
            {
                uiMyByteCount = 0;
                uiMyExpectedPayloadLength = 0;
                return STATUS::BUFFER_FULL;
            }

            if (ucDataByte == '{') { uiMyJsonObjectOpenBraces++; }
            else if (ucDataByte == '}') { uiMyJsonObjectOpenBraces--; }

            if (uiMyJsonObjectOpenBraces == 0)
            {
                stMetaData.uiLength = uiMyByteCount;
                // pclMyCircularDataBuffer->Copy(pucFrameBuffer_, stMetaData.uiLength);
                // pclMyCircularDataBuffer->Discard(stMetaData.uiLength);
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
    return STATUS::SUCCESS;
}
