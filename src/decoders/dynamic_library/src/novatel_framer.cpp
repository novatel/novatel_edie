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
//! \file novatel_framer.cpp
//! \brief DLL-exposed OEM framer functionality.
//! \remark See novatel::edie::oem::Framer for API details.
///////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "novatel_framer.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool novatel_framer_set_logger_level(Framer* pclFramer_, uint32_t uiLogLevel_)
{
    return pclFramer_ && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclFramer_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

void novatel_framer_shutdown_logger(Framer* pclFramer_)
{
    if (pclFramer_) { pclFramer_->ShutdownLogger(); }
}

Framer* novatel_framer_init() { return new Framer(); }

void novatel_framer_delete(Framer* pclFramer_)
{
    if (pclFramer_ != nullptr)
    {
        delete pclFramer_;
        pclFramer_ = nullptr;
    }
}

void novatel_framer_frame_json(Framer* pclFramer_, bool bFrameJson_)
{
    if (pclFramer_) { pclFramer_->SetFrameJson(bFrameJson_); }
}

void novatel_framer_payload_only(Framer* pclFramer_, bool bPayloadOnly_)
{
    if (pclFramer_) { pclFramer_->SetPayloadOnly(bPayloadOnly_); }
}

void novatel_framer_report_unknown_bytes(Framer* pclFramer_, bool bReportUnknownBytes_)
{
    if (pclFramer_) { pclFramer_->SetReportUnknownBytes(bReportUnknownBytes_); }
}

uint32_t novatel_framer_get_available_bytes(Framer* pclFramer_) { return pclFramer_ ? pclFramer_->GetBytesAvailableInBuffer() : UINT_MAX; }

uint32_t novatel_framer_write(Framer* pclFramer_, unsigned char* pucBytes_, uint32_t uiByteCount_)
{
    return pclFramer_ ? pclFramer_->Write(pucBytes_, uiByteCount_) : UINT_MAX;
}

STATUS novatel_framer_read(Framer* pclFramer_, unsigned char* pucBuffer_, uint32_t uiBufferSize_, MetaDataStruct* pstMetaData_)
{
    return pclFramer_ && pstMetaData_ ? pclFramer_->GetFrame(pucBuffer_, uiBufferSize_, *pstMetaData_) : STATUS::NULL_PROVIDED;
}

uint32_t novatel_framer_flush(Framer* pclFramer_, unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    return pclFramer_ && pucBuffer_ ? pclFramer_->Flush(pucBuffer_, uiBufferSize_) : UINT_MAX;
}
