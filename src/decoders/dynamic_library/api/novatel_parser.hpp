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
// ! \file novatel_parser.hpp
// ===============================================================================

#ifndef DYNAMIC_LIBRARY_NOVATEL_PARSER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_PARSER_HPP

#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/parser.hpp"
#include "decoders_export.h"

extern "C"
{
    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::Parser* NovatelParserInit(JsonReader* pclJsonDb_);
    DECODERS_EXPORT void NovatelParserDelete(novatel::edie::oem::Parser* pclParser_);

    // Config
    DECODERS_EXPORT void NovatelParserLoadJsonDb(novatel::edie::oem::Parser* pclParser_, JsonReader* pclJsonDb_);

    DECODERS_EXPORT void NovatelParserSetIgnoreAbbrevAsciiResponses(novatel::edie::oem::Parser* pclParser_, bool bIgnoreAbbrevAsciiResponsesCmp_);
    DECODERS_EXPORT bool NovatelParserGetIgnoreAbbrevAsciiResponses(novatel::edie::oem::Parser* pclParser_);
    DECODERS_EXPORT void NovatelParserSetDecompressRangeCmp(novatel::edie::oem::Parser* pclParser_, bool bDecompressRangeCmp_);
    DECODERS_EXPORT bool NovatelParserGetDecompressRangeCmp(novatel::edie::oem::Parser* pclParser_);
    DECODERS_EXPORT void NovatelParserSetReturnUnknownBytes(novatel::edie::oem::Parser* pclParser_, bool bReturnUnknownBytes_);
    DECODERS_EXPORT bool NovatelParserGetReturnUnknownBytes(novatel::edie::oem::Parser* pclParser_);
    DECODERS_EXPORT void NovatelParserSetEncodeFormat(novatel::edie::oem::Parser* pclParser_, novatel::edie::ENCODE_FORMAT eEncodeFormat_);
    DECODERS_EXPORT novatel::edie::ENCODE_FORMAT NovatelParserGetEncodeFormat(novatel::edie::oem::Parser* pclParser_);

    DECODERS_EXPORT novatel::edie::oem::Filter* NovatelParserGetFilter(novatel::edie::oem::Parser* pclParser_);
    DECODERS_EXPORT void NovatelParserSetFilter(novatel::edie::oem::Parser* pclParser_, novatel::edie::oem::Filter* pclFilter_);

    DECODERS_EXPORT unsigned char* NovatelParserGetBuffer(novatel::edie::oem::Parser* pclParser_);

    // R/W
    DECODERS_EXPORT uint32_t NovatelParserWrite(novatel::edie::oem::Parser* pclParser_, unsigned char* pucBytes_, uint32_t uiByteCount_);
    DECODERS_EXPORT novatel::edie::STATUS NovatelParserRead(novatel::edie::oem::Parser* pclParser_, novatel::edie::MessageDataStruct* pstMessageData_,
                                                            novatel::edie::oem::MetaDataStruct* pstMetaData_);

    DECODERS_EXPORT uint32_t NovatelParserFlush(novatel::edie::oem::Parser* pclParser_, unsigned char* pucBuffer_, uint32_t uiBufferSize_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_PARSER_HPP
