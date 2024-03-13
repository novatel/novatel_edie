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
//! \file novatel_fileparser.cpp
//! \brief DLL-exposed OEM file parser functionality.
//! \remark See novatel::edie::oem::FileParser for API details.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "novatel_fileparser.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

FileParser* novatel_fileparser_init(JsonReader* pclJsonDb_) { return new FileParser(pclJsonDb_); }

void novatel_fileparser_delete(FileParser* pclFileParser_)
{
    if (pclFileParser_)
    {
        delete pclFileParser_;
        pclFileParser_ = nullptr;
    }
}

void novatel_fileparser_load_json_db(FileParser* pclFileParser_, JsonReader* pclJsonDb_)
{
    if (pclFileParser_ && pclJsonDb_) { pclFileParser_->LoadJsonDb(pclJsonDb_); }
}

void novatel_fileparser_set_ignore_abbrev_ascii_responses(FileParser* pclFileParser_, bool bIgnoreAbbrevASCIIResponsesCmp_)
{
    if (pclFileParser_) { pclFileParser_->SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbrevASCIIResponsesCmp_); }
}

bool novatel_fileparser_get_ignore_abbrev_ascii_responses(FileParser* pclFileParser_)
{
    return pclFileParser_ ? pclFileParser_->GetIgnoreAbbreviatedAsciiResponses() : false;
}

void novatel_fileparser_set_decompress_rangecmp(FileParser* pclFileParser_, bool bDecompressRangeCmp_)
{
    if (pclFileParser_) { pclFileParser_->SetDecompressRangeCmp(bDecompressRangeCmp_); }
}

bool novatel_fileparser_get_decompress_rangecmp(FileParser* pclFileParser_)
{
    return pclFileParser_ ? pclFileParser_->GetDecompressRangeCmp() : false;
}

void novatel_fileparser_set_return_unknownbytes(FileParser* pclFileParser_, bool bReturnUnknownBytes_)
{
    if (pclFileParser_) { pclFileParser_->SetReturnUnknownBytes(bReturnUnknownBytes_); }
}

bool novatel_fileparser_get_return_unknownbytes(FileParser* pclFileParser_)
{
    return pclFileParser_ ? pclFileParser_->GetReturnUnknownBytes() : false;
}

void novatel_fileparser_set_encodeformat(FileParser* pclFileParser_, ENCODEFORMAT eEncodeFormat_)
{
    if (pclFileParser_) { return pclFileParser_->SetEncodeFormat(eEncodeFormat_); }
}

ENCODEFORMAT novatel_fileparser_get_encodeformat(FileParser* pclFileParser_)
{
    return pclFileParser_ ? pclFileParser_->GetEncodeFormat() : ENCODEFORMAT::UNSPECIFIED;
}

unsigned char* novatel_fileparser_get_buffer(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->GetInternalBuffer() : nullptr; }

Filter* novatel_fileparser_get_filter(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->GetFilter() : nullptr; }

void novatel_fileparser_set_filter(FileParser* pclFileParser_, Filter* pclFilter_)
{
    if (pclFileParser_ && pclFilter_) { pclFileParser_->SetFilter(pclFilter_); }
}

bool novatel_fileparser_set_stream(FileParser* pclFileParser_, InputFileStream* pclIFS_)
{
    return pclFileParser_ && pclIFS_ ? pclFileParser_->SetStream(pclIFS_) : false;
}

uint32_t novatel_fileparser_get_percent_read(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->GetPercentRead() : UINT_MAX; }

STATUS novatel_fileparser_read(FileParser* pclFileParser_, MessageDataStruct* pstMessageData_, MetaDataStruct* pstMetaData_)
{
    return pclFileParser_ && pstMessageData_ && pstMetaData_ ? pclFileParser_->Read(*pstMessageData_, *pstMetaData_) : STATUS::NULL_PROVIDED;
}

bool novatel_fileparser_reset(FileParser* pclFileParser_) { return pclFileParser_ ? pclFileParser_->Reset() : false; }

uint32_t novatel_fileparser_flush(FileParser* pclFileParser_, unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    return pclFileParser_ && pucBuffer_ ? pclFileParser_->Flush(pucBuffer_, uiBufferSize_) : UINT_MAX;
}
