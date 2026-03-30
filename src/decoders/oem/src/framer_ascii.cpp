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
// ! \file framer_ascii.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/framer_ascii.hpp"

#include <charconv>

#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/decoders/common/framer_registration.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// Register the OEM ASCII framer with the framer factory
REGISTER_FRAMER(OEM_ASCII, oem::FramerAscii, MetaDataStruct)

// -------------------------------------------------------------------------------------------------------
FramerAscii::FramerAscii() : FramerBase("novatel_framer_ascii") {}

// -------------------------------------------------------------------------------------------------------
FramerAscii::FramerAscii(std::shared_ptr<UCharFixedBuffer> buffer) : FramerBase("novatel_framer_ascii", buffer)
{
    pclMyLogger->info("FramerAscii initialized");
}

[[nodiscard]] size_t FramerAscii::FindSync() const
{
    return pclMyBuffer->search_char(OEM4_ASCII_SYNC, 0, MAX_ASCII_MESSAGE_LENGTH);
}

[[nodiscard]] FramerBase::FindFrameEndResult FramerAscii::FindFrameEnd(size_t start) const
{
    using ReturnStatus = FramerBase::FindFrameEndResult::Status;
    const auto& clFrameBuffer = *pclMyBuffer;
    auto uiCrcDelimIndex = clFrameBuffer.search_char(OEM4_ASCII_CRC_DELIMITER, start, MAX_ASCII_MESSAGE_LENGTH);
    while (uiCrcDelimIndex != UCharFixedBuffer::npos && clFrameBuffer.size() - uiCrcDelimIndex >= OEM4_ASCII_CRC_LENGTH + 3 && !IsAsciiCrc(static_cast<uint32_t>(uiCrcDelimIndex + 1)))
    {
        uiCrcDelimIndex = clFrameBuffer.search_char(OEM4_ASCII_CRC_DELIMITER, uiCrcDelimIndex + 1, MAX_ASCII_MESSAGE_LENGTH);
    }

    if (uiCrcDelimIndex == UCharFixedBuffer::npos)
    {
        return clFrameBuffer.size() >= MAX_ASCII_MESSAGE_LENGTH ? FramerBase::FindFrameEndResult{ReturnStatus::INVALID, HEADER_FORMAT::UNKNOWN, OEM4_ASCII_SYNC_LENGTH} :
                    FramerBase::FindFrameEndResult{ReturnStatus::INCOMPLETE, HEADER_FORMAT::ASCII, clFrameBuffer.size()};
    }
    return clFrameBuffer.size() - uiCrcDelimIndex >= OEM4_ASCII_CRC_LENGTH + 3 ? FramerBase::FindFrameEndResult{ReturnStatus::COMPLETE, HEADER_FORMAT::ASCII, uiCrcDelimIndex + OEM4_ASCII_CRC_LENGTH + 3} :
           FramerBase::FindFrameEndResult{ReturnStatus::INCOMPLETE, HEADER_FORMAT::ASCII, uiCrcDelimIndex};
}

[[nodiscard]] size_t FramerAscii::Validate(size_t frameEnd) const
{
    const auto& clFrameBuffer = *pclMyBuffer;
    char acCrc[OEM4_ASCII_CRC_LENGTH + 1];
    std::memcpy(acCrc, clFrameBuffer.data() + frameEnd - OEM4_ASCII_CRC_LENGTH - 2, OEM4_ASCII_CRC_LENGTH);

    uint32_t uiMessageCrc = 0;
    auto result = std::from_chars(acCrc, acCrc + OEM4_ASCII_CRC_LENGTH, uiMessageCrc, 16);
    auto uiCalcCrc = CalculateBlockCrc32(clFrameBuffer.data() + OEM4_ASCII_SYNC_LENGTH, static_cast<uint32_t>(frameEnd - OEM4_ASCII_SYNC_LENGTH - OEM4_ASCII_CRC_LENGTH - 3));
    return result.ec == std::errc() && uiCalcCrc == uiMessageCrc ? 0 : OEM4_ASCII_SYNC_LENGTH;
}
