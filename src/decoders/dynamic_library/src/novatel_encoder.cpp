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
// ! \file novatel_encoder.cpp
// ===============================================================================

#include "novatel_encoder.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool NovatelEncoderSetLoggerLevel(Encoder* pclEncoder_, uint32_t uiLogLevel_)
{
    return pclEncoder_ && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclEncoder_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

void NovatelEncoderShutdownLogger(Encoder* pclEncoder_)
{
    if (pclEncoder_) { pclEncoder_->ShutdownLogger(); } // TODO: ShutdownLogger is static, this function signature should be changed
}

Encoder* NovatelEncoderInit(JsonReader* pclJsonDb_) { return new Encoder(pclJsonDb_); }

void NovatelEncoderDelete(Encoder* pclEncoder_)
{
    if (pclEncoder_)
    {
        delete pclEncoder_;
        pclEncoder_ = nullptr;
    }
}

void NovatelEncoderLoadJson(Encoder* pclEncoder_, JsonReader* pclJsonDb_)
{
    if (pclEncoder_ && pclJsonDb_) { pclEncoder_->LoadJsonDb(pclJsonDb_); }
}

STATUS NovatelEncoderEncode(Encoder* pclEncoder_, unsigned char* pucEncodeBuffer_, uint32_t uiEncodeBufferSize_, IntermediateHeader* pstInterHeader_,
                            std::vector<FieldContainer>* pstInterMessage_, MessageDataStruct* pstMessageData_, MetaDataStruct* pstMetaData_,
                            ENCODE_FORMAT eEncodeFormat_)
{
    return pclEncoder_ && pucEncodeBuffer_ && pstInterHeader_ && pstInterMessage_ && pstMessageData_ && pstMetaData_
               ? pclEncoder_->Encode(&pucEncodeBuffer_, uiEncodeBufferSize_, *pstInterHeader_, *pstInterMessage_, *pstMessageData_, *pstMetaData_,
                                     eEncodeFormat_)
               : STATUS::NULL_PROVIDED;
}
