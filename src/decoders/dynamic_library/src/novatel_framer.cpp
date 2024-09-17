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
// ! \file novatel_framer.cpp
// ===============================================================================

#include "novatel_framer.hpp"

#include <limits>

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool NovatelFramerSetLoggerLevel(Framer* pclFramer_, uint32_t uiLogLevel_)
{
    return (pclFramer_ != nullptr) && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclFramer_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

Framer* NovatelFramerInit() { return new Framer(); }

void NovatelFramerDelete(Framer* pclFramer_)
{
    if (pclFramer_ != nullptr)
    {
        delete pclFramer_;
        pclFramer_ = nullptr;
    }
}

void NovatelFramerFrameJson(Framer* pclFramer_, bool bFrameJson_)
{
    if (pclFramer_ != nullptr) { pclFramer_->SetFrameJson(bFrameJson_); }
}

void NovatelFramerPayloadOnly(Framer* pclFramer_, bool bPayloadOnly_)
{
    if (pclFramer_ != nullptr) { pclFramer_->SetPayloadOnly(bPayloadOnly_); }
}

void NovatelFramerReportUnknownBytes(Framer* pclFramer_, bool bReportUnknownBytes_)
{
    if (pclFramer_ != nullptr) { pclFramer_->SetReportUnknownBytes(bReportUnknownBytes_); }
}

uint32_t NovatelFramerGetAvailableBytes(Framer* pclFramer_)
{
    return pclFramer_ != nullptr ? pclFramer_->GetBytesAvailableInBuffer() : std::numeric_limits<uint32_t>::max();
}

uint32_t NovatelFramerWrite(Framer* pclFramer_, unsigned char* pucBytes_, uint32_t uiByteCount_)
{
    return pclFramer_ != nullptr ? pclFramer_->Write(pucBytes_, uiByteCount_) : std::numeric_limits<uint32_t>::max();
}

STATUS NovatelFramerRead(Framer* pclFramer_, unsigned char* pucBuffer_, uint32_t uiBufferSize_, MetaDataStruct* pstMetaData_)
{
    return (pclFramer_ != nullptr) && (pstMetaData_ != nullptr) ? pclFramer_->GetFrame(pucBuffer_, uiBufferSize_, *pstMetaData_)
                                                                : STATUS::NULL_PROVIDED;
}

uint32_t NovatelFramerFlush(Framer* pclFramer_, unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    return (pclFramer_ != nullptr) && (pucBuffer_ != nullptr) ? pclFramer_->Flush(pucBuffer_, uiBufferSize_) : std::numeric_limits<uint32_t>::max();
}
