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
// ! \file novatel_parser.cpp
// ===============================================================================

#include "novatel_parser.hpp"

#include <limits>

using namespace novatel::edie;
using namespace novatel::edie::oem;

Parser* NovatelParserInit(JsonReader* pclJsonDb_) { return new Parser(pclJsonDb_); }

void NovatelParserDelete(Parser* pclParser_)
{
    if (pclParser_ != nullptr)
    {
        delete pclParser_;
        pclParser_ = nullptr;
    }
}

void NovatelParserLoadJsonDb(Parser* pclParser_, JsonReader* pclJsonDb_)
{
    if ((pclParser_ != nullptr) && (pclJsonDb_ != nullptr)) { pclParser_->LoadJsonDb(pclJsonDb_); }
}

void NovatelParserSetIgnoreAbbrevAsciiResponses(Parser* pclParser_, bool bIgnoreAbbrevAsciiResponsesCmp_)
{
    if (pclParser_ != nullptr) { pclParser_->SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbrevAsciiResponsesCmp_); }
}

bool NovatelParserGetIgnoreAbbrevAsciiResponses(Parser* pclParser_)
{
    return pclParser_ != nullptr ? pclParser_->GetIgnoreAbbreviatedAsciiResponses() : false;
}

void NovatelParserSetDecompressRangeCmp(Parser* pclParser_, bool bDecompressRangeCmp_)
{
    if (pclParser_ != nullptr) { pclParser_->SetDecompressRangeCmp(bDecompressRangeCmp_); }
}

bool NovatelParserGetDecompressRangeCmp(Parser* pclParser_) { return pclParser_ != nullptr ? pclParser_->GetDecompressRangeCmp() : false; }

void NovatelParserSetReturnUnknownBytes(Parser* pclParser_, bool bReturnUnknownBytes_)
{
    if (pclParser_ != nullptr) { pclParser_->SetReturnUnknownBytes(bReturnUnknownBytes_); }
}

bool NovatelParserGetReturnUnknownBytes(Parser* pclParser_) { return pclParser_ != nullptr ? pclParser_->GetReturnUnknownBytes() : false; }

void NovatelParserSetEncodeFormat(Parser* pclParser_, ENCODE_FORMAT eEncodeFormat_)
{
    if (pclParser_ != nullptr) { pclParser_->SetEncodeFormat(eEncodeFormat_); }
}

ENCODE_FORMAT NovatelParserGetEncodeFormat(Parser* pclParser_)
{
    return pclParser_ != nullptr ? pclParser_->GetEncodeFormat() : ENCODE_FORMAT::UNSPECIFIED;
}

Filter* NovatelParserGetFilter(Parser* pclParser_) { return pclParser_ != nullptr ? pclParser_->GetFilter() : nullptr; }

void NovatelParserSetFilter(Parser* pclParser_, Filter* pclFilter_)
{
    if ((pclParser_ != nullptr) && (pclFilter_ != nullptr)) { pclParser_->SetFilter(pclFilter_); }
}

unsigned char* NovatelParserGetBuffer(Parser* pclParser_) { return pclParser_ != nullptr ? pclParser_->GetInternalBuffer() : nullptr; }

uint32_t NovatelParserWrite(Parser* pclParser_, unsigned char* pucBytes_, uint32_t uiByteCount_)
{
    return (pclParser_ != nullptr) && (pucBytes_ != nullptr) ? pclParser_->Write(pucBytes_, uiByteCount_) : std::numeric_limits<uint32_t>::max();
}

STATUS NovatelParserRead(Parser* pclParser_, MessageDataStruct* pstMessageData_, MetaDataStruct* pstMetaData_)
{
    return (pclParser_ != nullptr) && (pstMessageData_ != nullptr) && (pstMetaData_ != nullptr) ? pclParser_->Read(*pstMessageData_, *pstMetaData_)
                                                                                                : STATUS::NULL_PROVIDED;
}

uint32_t NovatelParserFlush(Parser* pclParser_, unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    return (pclParser_ != nullptr) && (pucBuffer_ != nullptr) ? pclParser_->Flush(pucBuffer_, uiBufferSize_) : std::numeric_limits<uint32_t>::max();
}
