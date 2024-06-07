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
// ! \file novatel_header_decoder.hpp
// ===============================================================================

#ifndef DYNAMIC_LIBRARY_NOVATEL_HEADER_DECODER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_HEADER_DECODER_HPP

#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/header_decoder.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool NovatelHeaderDecoderSetLoggerLevel(novatel::edie::oem::HeaderDecoder* pclHeaderDecoder_, uint32_t iLogLevel_);
    DECODERS_EXPORT void NovatelHeaderDecoderShutdownLogger(novatel::edie::oem::HeaderDecoder* pclHeaderDecoder_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::HeaderDecoder* NovatelHeaderDecoderInit(novatel::edie::JsonReader* pclJsonDb_);
    DECODERS_EXPORT void NovatelHeaderDecoderDelete(novatel::edie::oem::HeaderDecoder* pclHeaderDecoder_);

    // Config
    DECODERS_EXPORT void NovatelHeaderDecoderLoadJson(novatel::edie::oem::HeaderDecoder* pclHeaderDecoder_, novatel::edie::JsonReader* pclJsonDb_);

    // R/W
    DECODERS_EXPORT novatel::edie::STATUS NovatelHeaderDecoderDecode(novatel::edie::oem::HeaderDecoder* pclHeaderDecoder_, unsigned char* pucLogBuf_,
                                                                     novatel::edie::oem::IntermediateHeader* pstInterHeader_,
                                                                     novatel::edie::oem::MetaDataStruct* pstMetaData_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_HEADER_DECODER_HPP
