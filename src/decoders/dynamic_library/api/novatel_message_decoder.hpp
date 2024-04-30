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
// ! \file novatel_message_decoder.hpp
// ===============================================================================

#ifndef DYNAMIC_LIBRARY_NOVATEL_MESSAGE_DECODER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_MESSAGE_DECODER_HPP

#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/message_decoder.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool NovatelMessageDecoderSetLoggerLevel(novatel::edie::oem::MessageDecoder* pclMessageDecoder_, uint32_t iLogLevel_);
    DECODERS_EXPORT void NovatelMessageDecoderShutdownLogger(novatel::edie::oem::MessageDecoder* pclMessageDecoder_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::MessageDecoder* NovatelMessageDecoderInit(JsonReader* pclJsonDb_);
    DECODERS_EXPORT void NovatelMessageDecoderDelete(novatel::edie::oem::MessageDecoder* pclMessageDecoder_);

    // Config
    DECODERS_EXPORT void NovatelMessageDecoderLoadJson(novatel::edie::oem::MessageDecoder* pclMessageDecoder_, JsonReader* pclJsonDb_);

    // R/W
    DECODERS_EXPORT novatel::edie::STATUS NovatelMessageDecoderDecode(novatel::edie::oem::MessageDecoder* pclMessageDecoder_,
                                                                      unsigned char* pucLogBuf_, novatel::edie::IntermediateMessage* pstInterMessage_,
                                                                      novatel::edie::oem::MetaDataStruct* pstMetaData_);

    // Intermediate Log handling.
    DECODERS_EXPORT novatel::edie::IntermediateMessage* NovatelIntermediateMessageInit();
    DECODERS_EXPORT void NovatelIntermediateMessageDelete(novatel::edie::IntermediateMessage* pstInterMessage_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_MESSAGE_DECODER_HPP
