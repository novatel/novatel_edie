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

#include <charconv>

#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/decoders/common/framer_registration.hpp"
#include "novatel_edie/decoders/oem/framer_binary.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// Register the OEM binary framer with the framer factory
REGISTER_FRAMER(OEM_BINARY, oem::FramerBinary, MetaDataStruct)

// -------------------------------------------------------------------------------------------------------
FramerBinary::FramerBinary() : FramerBase("novatel_framer_binary") {}

// -------------------------------------------------------------------------------------------------------
FramerBinary::FramerBinary(std::shared_ptr<UCharFixedRingBuffer> ringBuffer) : FramerBase("novatel_framer_binary", ringBuffer)
{
    pclMyLogger->info("FramerBinary initialized");
}

// -------------------------------------------------------------------------------------------------------
STATUS
FramerBinary::GetFrame(unsigned char* pucFrameBuffer_, const uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_, bool bMetadataOnly_)
{
    if (pucFrameBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    // Dereferencing the pointer in the loop is expensive, so we cache the reference here
    const auto& clInternalFrameBuffer = *pclMyBuffer;
    if (clInternalFrameBuffer.empty()) { return STATUS::BUFFER_EMPTY; }

    // Helper to set metadata and handle unknown bytes
    auto handleUnknown = [&](uint32_t length) -> STATUS {
        stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
        stMetaData_.uiLength = length;
        if (!bMetadataOnly_) { HandleUnknownBytes(pucFrameBuffer_, length); }
        return STATUS::UNKNOWN;
    };

    auto uiMaxMessageLength = std::min(static_cast<size_t>(MAX_BINARY_MESSAGE_LENGTH), clInternalFrameBuffer.size());

    if (uiMyByteCount == 0)
    {
        size_t uiSyncIndex = 0;
        do
        {
            uiSyncIndex = clInternalFrameBuffer.search_char(OEM4_BINARY_SYNC1, uiMyByteCount, uiMaxMessageLength - uiMyByteCount);
            uiMyByteCount += static_cast<uint32_t>(uiSyncIndex + 1); // +1 moves past the first sync character for the next search
        } while (uiSyncIndex != UCharFixedRingBuffer::npos &&
                 ((uiSyncIndex + 1 < uiMaxMessageLength && clInternalFrameBuffer[uiSyncIndex + 1] != OEM4_BINARY_SYNC2) ||
                 (uiSyncIndex + 2 < uiMaxMessageLength && clInternalFrameBuffer[uiSyncIndex + 2] != OEM4_BINARY_SYNC3)));

        // Sync character not at start of buffer - handle unknown bytes before sync
        if (uiSyncIndex != 0) { return handleUnknown(static_cast<uint32_t>(uiSyncIndex == UCharFixedRingBuffer::npos ? uiMaxMessageLength : uiSyncIndex)); }
    }

    stMetaData_.eFormat = HEADER_FORMAT::BINARY;
    uint16_t uiMessageLength = 0;
    if (uiMaxMessageLength >= OEM4_BINARY_HEADER_LENGTH &&
        uiMaxMessageLength >= OEM4_BINARY_HEADER_LENGTH +
                              (uiMessageLength = clInternalFrameBuffer.read_value<uint16_t>(OEM4_BINARY_MESSAGE_LENGTH_INDEX)) +
                              OEM4_BINARY_CRC_LENGTH)
    {
        auto uiCombinedMessageLength = OEM4_BINARY_HEADER_LENGTH + uiMessageLength;
        auto uiCalcCrc = clInternalFrameBuffer.accumulate_range(0, uiCombinedMessageLength, static_cast<uint32_t>(0), GetNextCharacterCrc32);
        if (uiCalcCrc != clInternalFrameBuffer.read_value<uint32_t>(uiCombinedMessageLength))
        {
            return handleUnknown(OEM4_BINARY_SYNC_LENGTH);
        }

        // Valid frame found
        stMetaData_.uiLength = uiCombinedMessageLength + OEM4_BINARY_CRC_LENGTH;

        if (uiFrameBufferSize_ < stMetaData_.uiLength) { return STATUS::BUFFER_FULL; }

        if (!bMetadataOnly_)
        {
            pclMyBuffer->copy_out(pucFrameBuffer_, stMetaData_.uiLength);
            pclMyBuffer->erase_begin(stMetaData_.uiLength);
            InitAttributes();
        }

        return STATUS::SUCCESS;
    }

    if (uiMessageLength > MAX_BINARY_MESSAGE_LENGTH - OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_CRC_LENGTH)
    {
        // Invalid message length
        return handleUnknown(OEM4_BINARY_SYNC_LENGTH);
    }
    stMetaData_.uiLength = static_cast<uint32_t>(clInternalFrameBuffer.size());
    return STATUS::INCOMPLETE;
}
