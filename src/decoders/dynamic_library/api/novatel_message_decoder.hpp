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

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/message_decoder.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool novatel_message_decoder_set_logger_level(novatel::edie::oem::MessageDecoder* pclMessageDecoder_, uint32_t iLogLevel_);
    DECODERS_EXPORT void novatel_message_decoder_shutdown_logger(novatel::edie::oem::MessageDecoder* pclMessageDecoder_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::MessageDecoder* novatel_message_decoder_init(JsonReader* pclJsonDb_);
    DECODERS_EXPORT void novatel_message_decoder_delete(novatel::edie::oem::MessageDecoder* pclMessageDecoder_);

    // Config
    DECODERS_EXPORT void novatel_message_decoder_load_json(novatel::edie::oem::MessageDecoder* pclMessageDecoder_, JsonReader* pclJsonDb_);

    // R/W
    DECODERS_EXPORT novatel::edie::STATUS novatel_message_decoder_decode(novatel::edie::oem::MessageDecoder* pclMessageDecoder_,
                                                                         unsigned char* pucLogBuf_,
                                                                         novatel::edie::IntermediateMessage* pstInterMessage_,
                                                                         novatel::edie::oem::MetaDataStruct* pstMetaData_);

    // Intermediate Log handling.
    DECODERS_EXPORT novatel::edie::IntermediateMessage* novatel_intermediate_message_init();
    DECODERS_EXPORT void novatel_intermediate_message_delete(novatel::edie::IntermediateMessage* pstInterMessage_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_MESSAGE_DECODER_HPP
