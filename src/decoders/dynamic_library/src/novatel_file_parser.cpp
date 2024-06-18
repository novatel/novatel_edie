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
// ! \file novatel_file_parser.cpp
// ===============================================================================

#include <limits>

#include "novatel_file_parser.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

FileParser* NovatelFileParserInit(JsonReader* pclJsonDb_) { return new FileParser(pclJsonDb_); }

void NovatelFileParserDelete(FileParser* pclFileParser_)
{
    if (pclFileParser_)
    {
        delete pclFileParser_;
        pclFileParser_ = nullptr;
    }
}

void NovatelFileParserLoadJsonDb(FileParser* pclFileParser_, JsonReader* pclJsonDb_)
{
    if (pclFileParser_ && pclJsonDb_) { pclFileParser_->LoadJsonDb(pclJsonDb_); }
}

void NovatelFileParserSetIgnoreAbbrevAsciiResponses(FileParser* pclFileParser_, bool bIgnoreAbbrevAsciiResponsesCmp_)
{
    if (pclFileParser_) { pclFileParser_->SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbrevAsciiResponsesCmp_); }
}

bool NovatelFileParserGetIgnoreAbbrevAsciiResponses(FileParser* pclFileParser_)
{
    return pclFileParser_ ? pclFileParser_->GetIgnoreAbbreviatedAsciiResponses() : false;
}

void NovatelFileParserSetDecompressRangeCmp(FileParser* pclFileParser_, bool bDecompressRangeCmp_)
{
    if (pclFileParser_) { pclFileParser_->SetDecompressRangeCmp(bDecompressRangeCmp_); }
}

bool NovatelFileParserGetDecompressRangeCmp(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->GetDecompressRangeCmp() : false; }

void NovatelFileParserSetReturnUnknownBytes(FileParser* pclFileParser_, bool bReturnUnknownBytes_)
{
    if (pclFileParser_) { pclFileParser_->SetReturnUnknownBytes(bReturnUnknownBytes_); }
}

bool NovatelFileParserGetReturnUnknownBytes(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->GetReturnUnknownBytes() : false; }

void NovatelFileParserSetEncodeFormat(FileParser* pclFileParser_, ENCODE_FORMAT eEncodeFormat_)
{
    if (pclFileParser_) { return pclFileParser_->SetEncodeFormat(eEncodeFormat_); }
}

ENCODE_FORMAT NovatelFileParserGetEncodeFormat(FileParser* pclFileParser_)
{
    return pclFileParser_ ? pclFileParser_->GetEncodeFormat() : ENCODE_FORMAT::UNSPECIFIED;
}

unsigned char* NovatelFileParserGetBuffer(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->GetInternalBuffer() : nullptr; }

Filter* NovatelFileParserGetFilter(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->GetFilter() : nullptr; }

void NovatelFileParserSetFilter(FileParser* pclFileParser_, Filter* pclFilter_)
{
    if (pclFileParser_ && pclFilter_) { pclFileParser_->SetFilter(pclFilter_); }
}

bool NovatelFileParserSetStream(FileParser* pclFileParser_, InputFileStream* pclIfs_)
{
    return pclFileParser_ && pclIfs_ ? pclFileParser_->SetStream(pclIfs_) : false;
}

uint32_t NovatelFileParserGetPercentRead(FileParser* pclFileParser_)
{
    return pclFileParser_ ? pclFileParser_->GetPercentRead() : std::numeric_limits<uint32_t>::max();
}

STATUS NovatelFileParserRead(FileParser* pclFileParser_, MessageDataStruct* pstMessageData_, MetaDataStruct* pstMetaData_)
{
    return pclFileParser_ && pstMessageData_ && pstMetaData_ ? pclFileParser_->Read(*pstMessageData_, *pstMetaData_) : STATUS::NULL_PROVIDED;
}

bool NovatelFileParserReset(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->Reset() : false; }

uint32_t NovatelFileParserFlush(FileParser* pclFileParser_, unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    return pclFileParser_ && pucBuffer_ ? pclFileParser_->Flush(pucBuffer_, uiBufferSize_) : std::numeric_limits<uint32_t>::max();
}
