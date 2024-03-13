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
//! \file novatel_header_decoder.cpp
//! \brief DLL-exposed OEM header decoder functionality.
//! \remark See novatel::edie::oem::HeaderDecoder for API details.
///////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "novatel_header_decoder.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool novatel_header_decoder_set_logger_level(HeaderDecoder* pclHeaderDecoder_, uint32_t uiLogLevel_)
{
    return pclHeaderDecoder_ && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclHeaderDecoder_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

void novatel_header_decoder_shutdown_logger(HeaderDecoder* pclHeaderDecoder_)
{
    if (pclHeaderDecoder_) { pclHeaderDecoder_->ShutdownLogger(); }
}

HeaderDecoder* novatel_header_decoder_init(JsonReader* pclJsonDb_) { return new HeaderDecoder(pclJsonDb_); }

void novatel_header_decoder_delete(HeaderDecoder* pclHeaderDecoder_)
{
    if (pclHeaderDecoder_)
    {
        delete pclHeaderDecoder_;
        pclHeaderDecoder_ = nullptr;
    }
}

void novatel_header_decoder_load_json(HeaderDecoder* pclHeaderDecoder_, JsonReader* pclJsonDb_)
{
    if (pclHeaderDecoder_ && pclJsonDb_) { pclHeaderDecoder_->LoadJsonDb(pclJsonDb_); }
}

STATUS novatel_header_decoder_decode(HeaderDecoder* pclHeaderDecoder_, unsigned char* pucLogBuf_, IntermediateHeader* pstInterHeader_,
                                     MetaDataStruct* pstMetaData_)
{
    return pclHeaderDecoder_ && pucLogBuf_ && pstInterHeader_ && pstMetaData_ ? pclHeaderDecoder_->Decode(pucLogBuf_, *pstInterHeader_, *pstMetaData_)
                                                                              : STATUS::NULL_PROVIDED;
}
