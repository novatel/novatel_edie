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

#include "novatel_edie/common/crc32.hpp"
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
    auto syncIndex = clFrameBuffer.search_char(OEM4_ABBREV_ASCII_SYNC, 0, MAX_ASCII_MESSAGE_LENGTH);
    while (syncIndex != UCharFixedBuffer::npos && clFrameBuffer.size() > syncIndex + 1 && clFrameBuffer[syncIndex + 1] == OEM4_ABBREV_ASCII_SEPARATOR)
    {
        syncIndex = clFrameBuffer.search_char(OEM4_ABBREV_ASCII_SYNC, syncIndex + 1, MAX_ASCII_MESSAGE_LENGTH);
    }
    return syncIndex;
}

[[nodiscard]] FramerBase::FindFrameEndResult FramerAbbAscii::FindFrameEnd(size_t start) const
{
    using ReturnStatus = FramerBase::FindFrameEndResult::Status;
    auto crlf = std::array<unsigned char, 2>{'\r', '\n'};
    const auto& clFrameBuffer = *pclMyBuffer;
    auto uiCrlfIndex = clFrameBuffer.search_chars(crlf, start, MAX_ASCII_MESSAGE_LENGTH);
    if (uiCrlfIndex != UCharFixedBuffer::npos && IsAbbrevAsciiResponse())
    {
        return FramerBase::FindFrameEndResult{ReturnStatus::RESPONSE, HEADER_FORMAT::ABB_ASCII, uiCrlfIndex + 2};
    }

    // Maybe need to keep persistent count of CRLFs... or something like that.
    // following line rejects if a message's last line is split across writes

    // Next line must be the start of the message body (so it must be sync followed by separator)
    if (uiCrlfIndex != UCharFixedBuffer::npos && 
        (clFrameBuffer.size() > uiCrlfIndex + 2 && clFrameBuffer[uiCrlfIndex + 2] != OEM4_ABBREV_ASCII_SYNC ||
        clFrameBuffer.size() > uiCrlfIndex + 3 && clFrameBuffer[uiCrlfIndex + 3] != OEM4_ABBREV_ASCII_SEPARATOR))
    {
        return FramerBase::FindFrameEndResult{ReturnStatus::INVALID, HEADER_FORMAT::UNKNOWN, OEM4_ASCII_SYNC_LENGTH};
    }

    // Search for the end of the message body
    while (uiCrlfIndex != UCharFixedBuffer::npos && clFrameBuffer.size() > uiCrlfIndex + 3 && clFrameBuffer[uiCrlfIndex + 2] == OEM4_ABBREV_ASCII_SYNC && clFrameBuffer[uiCrlfIndex + 3] == OEM4_ABBREV_ASCII_SEPARATOR)
    {
        uiCrlfIndex = clFrameBuffer.search_chars(crlf, uiCrlfIndex + 1, MAX_ASCII_MESSAGE_LENGTH);
    }
    if (uiCrlfIndex == UCharFixedBuffer::npos)
    {
        return clFrameBuffer.size() >= MAX_ASCII_MESSAGE_LENGTH ? FramerBase::FindFrameEndResult{ReturnStatus::INVALID, HEADER_FORMAT::UNKNOWN, OEM4_ASCII_SYNC_LENGTH} :
                FramerBase::FindFrameEndResult{ReturnStatus::INCOMPLETE, HEADER_FORMAT::ABB_ASCII, clFrameBuffer.size()};
    }
    if (clFrameBuffer.size() <= uiCrlfIndex + 3)
    {
        return FramerBase::FindFrameEndResult{ReturnStatus::INCOMPLETE, HEADER_FORMAT::ABB_ASCII, uiCrlfIndex};
    }

    return FramerBase::FindFrameEndResult{ReturnStatus::COMPLETE, HEADER_FORMAT::ABB_ASCII, uiCrlfIndex + 2};
}
