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
//! \file novatel_parser.cpp
//! \brief DLL-exposed OEM parser functionality.
//! \remark See novatel::edie::oem::Parser for API details.
///////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "novatel_message_decoder.hpp"

using namespace novatel::edie;

bool novatel_message_decoder_set_logger_level(MessageDecoder* pclMessageDecoder_, uint32_t uiLogLevel_)
{
    return pclMessageDecoder_ && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclMessageDecoder_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

void novatel_message_decoder_shutdown_logger(MessageDecoder* pclMessageDecoder_)
{
    if (pclMessageDecoder_) { pclMessageDecoder_->ShutdownLogger(); }
}

MessageDecoder* novatel_message_decoder_init(JsonReader* pclJsonDb_) { return new MessageDecoder(pclJsonDb_); }

void novatel_message_decoder_delete(MessageDecoder* pclMessageDecoder_)
{
    if (pclMessageDecoder_)
    {
        delete pclMessageDecoder_;
        pclMessageDecoder_ = nullptr;
    }
}

void novatel_message_decoder_load_json(MessageDecoder* pclMessageDecoder_, JsonReader* pclJsonDb_)
{
    if (pclMessageDecoder_ && pclJsonDb_) { pclMessageDecoder_->LoadJsonDb(pclJsonDb_); }
}

STATUS novatel_message_decoder_decode(MessageDecoder* pclMessageDecoder_, unsigned char* pucLogBuf_, IntermediateMessage* pstInterMessage_,
                                      oem::MetaDataStruct* pstMetaData_)
{
    return pclMessageDecoder_ && pstInterMessage_ && pstMetaData_ ? pclMessageDecoder_->Decode(pucLogBuf_, *pstInterMessage_, *pstMetaData_)
                                                                  : STATUS::NULL_PROVIDED;
}

IntermediateMessage* novatel_intermediate_message_init() { return new IntermediateMessage(); }

void novatel_intermediate_message_delete(IntermediateMessage* pstInterMessage_)
{
    if (pstInterMessage_)
    {
        delete pstInterMessage_;
        pstInterMessage_ = nullptr;
    }
}
