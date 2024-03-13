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
#include "novatel_parser.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

Parser* novatel_parser_init(JsonReader* pclJsonDb_) { return new Parser(pclJsonDb_); }

void novatel_parser_delete(Parser* pclParser_)
{
    if (pclParser_)
    {
        delete pclParser_;
        pclParser_ = nullptr;
    }
}

void novatel_parser_load_json_db(Parser* pclParser_, JsonReader* pclJsonDb_)
{
    if (pclParser_ && pclJsonDb_) { pclParser_->LoadJsonDb(pclJsonDb_); }
}

void novatel_parser_set_ignore_abbrev_ascii_responses(novatel::edie::oem::Parser* pclParser_, bool bIgnoreAbbrevASCIIResponsesCmp_)
{
    if (pclParser_) { pclParser_->SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbrevASCIIResponsesCmp_); }
}

bool novatel_parser_get_ignore_abbrev_ascii_responses(novatel::edie::oem::Parser* pclParser_)
{
    return pclParser_ ? pclParser_->GetIgnoreAbbreviatedAsciiResponses() : false;
}

void novatel_parser_set_decompress_rangecmp(Parser* pclParser_, bool bDecompressRangeCmp_)
{
    if (pclParser_) { pclParser_->SetDecompressRangeCmp(bDecompressRangeCmp_); }
}

bool novatel_parser_get_decompress_rangecmp(Parser* pclParser_) { return pclParser_ ? pclParser_->GetDecompressRangeCmp() : false; }

void novatel_parser_set_return_unknownbytes(Parser* pclParser_, bool bReturnUnknownBytes_)
{
    if (pclParser_) { pclParser_->SetReturnUnknownBytes(bReturnUnknownBytes_); }
}

bool novatel_parser_get_return_unknownbytes(Parser* pclParser_) { return pclParser_ ? pclParser_->GetReturnUnknownBytes() : false; }

void novatel_parser_set_encodeformat(Parser* pclParser_, ENCODEFORMAT eEncodeFormat_)
{
    if (pclParser_) { pclParser_->SetEncodeFormat(eEncodeFormat_); }
}

ENCODEFORMAT novatel_parser_get_encodeformat(Parser* pclParser_) { return pclParser_ ? pclParser_->GetEncodeFormat() : ENCODEFORMAT::UNSPECIFIED; }

Filter* novatel_parser_get_filter(Parser* pclParser_) { return pclParser_ ? pclParser_->GetFilter() : nullptr; }

void novatel_parser_set_filter(Parser* pclParser_, Filter* pclFilter_)
{
    if (pclParser_ && pclFilter_) { pclParser_->SetFilter(pclFilter_); }
}

unsigned char* novatel_parser_get_buffer(Parser* pclParser_) { return pclParser_ ? pclParser_->GetInternalBuffer() : nullptr; }

uint32_t novatel_parser_write(Parser* pclParser_, unsigned char* pucBytes_, uint32_t uiByteCount_)
{
    return pclParser_ && pucBytes_ ? pclParser_->Write(pucBytes_, uiByteCount_) : UINT_MAX;
}

STATUS novatel_parser_read(Parser* pclParser_, MessageDataStruct* pstMessageData_, MetaDataStruct* pstMetaData_)
{
    return pclParser_ && pstMessageData_ && pstMetaData_ ? pclParser_->Read(*pstMessageData_, *pstMetaData_) : STATUS::NULL_PROVIDED;
}

uint32_t novatel_parser_flush(Parser* pclParser_, unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
    return pclParser_ && pucBuffer_ ? pclParser_->Flush(pucBuffer_, uiBufferSize_) : UINT_MAX;
}
