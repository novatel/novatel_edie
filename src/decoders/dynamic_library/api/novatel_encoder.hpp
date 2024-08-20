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
// ! \file novatel_encoder.hpp
// ===============================================================================

#ifndef DYNAMIC_LIBRARY_NOVATEL_ENCODER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_ENCODER_HPP

#include "decoders/common/api/json_reader.hpp"
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/encoder.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool NovatelEncoderSetLoggerLevel(novatel::edie::oem::Encoder* pclEncoder_, uint32_t iLogLevel_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::Encoder* NovatelEncoderInit(novatel::edie::JsonReader* pclJsonDb_);
    DECODERS_EXPORT void NovatelEncoderDelete(novatel::edie::oem::Encoder* pclEncoder_);

    // Config
    DECODERS_EXPORT void NovatelEncoderLoadJson(novatel::edie::oem::Encoder* pclEncoder_, novatel::edie::JsonReader* pclJsonDb_);

    // R/W
    DECODERS_EXPORT novatel::edie::STATUS NovatelEncoderEncode(novatel::edie::oem::Encoder* pclEncoder_, unsigned char* pucEncodeBuffer_,
                                                               uint32_t uiEncodeBufferSize_, novatel::edie::oem::IntermediateHeader* pstInterHeader_,
                                                               std::vector<novatel::edie::FieldContainer>* pstInterMessage_,
                                                               novatel::edie::MessageDataStruct* pstMessageData_,
                                                               novatel::edie::oem::MetaDataStruct* pstMetaData_,
                                                               novatel::edie::ENCODE_FORMAT eEncodeFormat_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_ENCODER_HPP
