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
// ! \file framer_abb_ascii.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/framer_abb_ascii.hpp"

#include <charconv>

#include "novatel_edie/common/crc.hpp"
#include "novatel_edie/decoders/common/framer_registration.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// Register the OEM abb ASCII framer with the framer factory
REGISTER_FRAMER(OEM_ABB_ASCII, oem::FramerAbbAscii, MetaDataStruct)

// -------------------------------------------------------------------------------------------------------
FramerAbbAscii::FramerAbbAscii() : FramerBase("novatel_framer_abb_ascii") {}

// -------------------------------------------------------------------------------------------------------
FramerAbbAscii::FramerAbbAscii(std::shared_ptr<UCharFixedBuffer> buffer) : FramerBase("novatel_framer_abb_ascii", buffer)
{
    pclMyLogger->info("FramerAbbAscii initialized");
}

[[nodiscard]] size_t FramerAbbAscii::FindSync() const
{
    const auto& clFrameBuffer = *pclMyBuffer;
    auto syncIndex = clFrameBuffer.search_char(OEM4_ABBREV_ASCII_SYNC, 0, MAX_LOOKAHEAD_BYTES);
    while (syncIndex != UCharFixedBuffer::npos && MAX_LOOKAHEAD_BYTES > syncIndex + 1 && clFrameBuffer.size() > syncIndex + 1 &&
           clFrameBuffer[syncIndex + 1] == OEM4_ABBREV_ASCII_SEPARATOR)
    {
        syncIndex = clFrameBuffer.search_char(OEM4_ABBREV_ASCII_SYNC, syncIndex + 1, MAX_LOOKAHEAD_BYTES - (syncIndex + 1));
    }
    return syncIndex == UCharFixedBuffer::npos ? std::min(clFrameBuffer.size(), MAX_LOOKAHEAD_BYTES) : syncIndex;
}

[[nodiscard]] FramerBase::FindFrameEndResult FramerAbbAscii::FindFrameEnd(size_t start) const
{
    using Result = FramerBase::FindFrameEndResult;
    using ReturnStatus = Result::Status;
    const auto crlf = std::array<unsigned char, 2>{'\r', '\n'};
    const auto& clFrameBuffer = *pclMyBuffer;

    auto midLineIncomplete = [&clFrameBuffer]() -> Result {
        return clFrameBuffer.size() >= MAX_ASCII_MESSAGE_LENGTH ? Result{ReturnStatus::INVALID, HEADER_FORMAT::UNKNOWN, OEM4_ASCII_SYNC_LENGTH}
                                                                : Result{ReturnStatus::INCOMPLETE, HEADER_FORMAT::ABB_ASCII, clFrameBuffer.size()};
    };

    // Step 1: Parse the first line (header/response line)
    const size_t headerCrlfIndex = clFrameBuffer.search_chars(crlf, 0, MAX_ASCII_MESSAGE_LENGTH);
    if (headerCrlfIndex == UCharFixedBuffer::npos) { return midLineIncomplete(); }

    if (IsAbbrevAsciiResponse()) { return Result{ReturnStatus::RESPONSE, HEADER_FORMAT::ABB_ASCII, headerCrlfIndex + 2}; }

    // If we reach this point, then the message is not a response
    // Step 2: Ensure the body is nonempty
    const size_t bodyStart = headerCrlfIndex + 2;
    if (clFrameBuffer.size() <= bodyStart + 1) { return Result{ReturnStatus::INCOMPLETE, HEADER_FORMAT::ABB_ASCII, headerCrlfIndex}; }
    if (clFrameBuffer.size() > bodyStart && !IsBodyLine(bodyStart))
    {
        return Result{ReturnStatus::INVALID, HEADER_FORMAT::UNKNOWN, OEM4_ASCII_SYNC_LENGTH};
    }

    // Step 3: Parse body lines
    size_t uiCrlfIndex = clFrameBuffer.search_chars(crlf, std::max(start, bodyStart), MAX_ASCII_MESSAGE_LENGTH);
    while (uiCrlfIndex != UCharFixedBuffer::npos && clFrameBuffer.size() > uiCrlfIndex + 3 && IsBodyLine(uiCrlfIndex + 2))
    {
        uiCrlfIndex = clFrameBuffer.search_chars(crlf, uiCrlfIndex + 1, MAX_ASCII_MESSAGE_LENGTH);
    }

    if (uiCrlfIndex == UCharFixedBuffer::npos) { return midLineIncomplete(); }

    if (clFrameBuffer.size() <= uiCrlfIndex + 3) { return Result{ReturnStatus::INCOMPLETE, HEADER_FORMAT::ABB_ASCII, uiCrlfIndex}; }

    return Result{ReturnStatus::COMPLETE, HEADER_FORMAT::ABB_ASCII, uiCrlfIndex + 2};
}
