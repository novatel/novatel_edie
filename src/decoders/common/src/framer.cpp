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

#include "novatel_edie/decoders/common/framer.hpp"

using namespace novatel::edie;

[[nodiscard]] STATUS FramerBase::GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_, bool bMetadataOnly_)
{
    if (pucFrameBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    // Dereferencing the shared pointer is expensive, so we cache the reference here
    const auto& clInternalFrameBuffer = *pclMyBuffer;
    if (clInternalFrameBuffer.empty()) { return STATUS::BUFFER_EMPTY; }

    // Helper to set metadata and handle unknown bytes
    auto handleUnknown = [&](uint32_t length) -> STATUS {
        stMetaData_.eFormat = HEADER_FORMAT::UNKNOWN;
        stMetaData_.uiLength = length;
        if (!bMetadataOnly_) { HandleUnknownBytes(pucFrameBuffer_, length); }
        return STATUS::UNKNOWN;
    };

    // --- Stage 1: Find sync byte(s) ---
    if (uiMyByteCount == 0)
    {
        if (size_t syncIndex = FindSync()) { return handleUnknown(static_cast<uint32_t>(syncIndex)); }
    }

    // --- Stage 2: Find end of candidate frame ---
    FindFrameEndResult frameEnd = FindFrameEnd(uiMyByteCount);
    stMetaData_.eFormat = frameEnd.eFormat;
    if (uiFrameBufferSize_ < static_cast<uint32_t>(frameEnd.uiIndex))
    {
        stMetaData_.uiLength = static_cast<uint32_t>(frameEnd.uiIndex);
        return STATUS::BUFFER_FULL;
    }

    // --- Stage 3: Validate candidate frame ---
    uiMyByteCount = static_cast<uint32_t>(frameEnd.uiIndex);
    stMetaData_.uiLength = uiMyByteCount;
    switch (frameEnd.eStatus)
    {
    case FindFrameEndResult::Status::RESPONSE: stMetaData_.bResponse = true; [[fallthrough]];
    case FindFrameEndResult::Status::COMPLETE:
        if (size_t bytesToDiscard = Validate(frameEnd.uiIndex)) { return handleUnknown(static_cast<uint32_t>(bytesToDiscard)); }
        break;
    case FindFrameEndResult::Status::INCOMPLETE:
        stMetaData_.uiLength = static_cast<uint32_t>(clInternalFrameBuffer.size());
        return STATUS::INCOMPLETE;
    case FindFrameEndResult::Status::INVALID: [[fallthrough]];
    default: return handleUnknown(static_cast<uint32_t>(frameEnd.uiIndex));
    }

    if (!bMetadataOnly_)
    {
        pclMyBuffer->copy_out(pucFrameBuffer_, stMetaData_.uiLength);
        pclMyBuffer->erase_begin(stMetaData_.uiLength);
        InitAttributes();
        ResetState();
    }

    return STATUS::SUCCESS;
}
