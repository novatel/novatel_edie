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
//! \file common_jsonreader.hpp
//! \brief DLL-exposed JSON reader functionality.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef DYNAMIC_LIBRARY_COMMON_JSONREADER_HPP
#define DYNAMIC_LIBRARY_COMMON_JSONREADER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/jsonreader.hpp"
#include "decoders_export.h"

extern "C"
{
    DECODERS_EXPORT JsonReader* common_jsonreader_init();
    DECODERS_EXPORT bool common_jsonreader_load_file(JsonReader* pclJsonDb_, const char* pcJsonDBFilepath_);
    DECODERS_EXPORT bool common_jsonreader_parse_json(JsonReader* pclJsonDb_, const char* pcJsonData_);
    DECODERS_EXPORT bool common_jsonreader_delete(JsonReader* pclJsonDb_);
}

#endif // DYNAMIC_LIBRARY_COMMON_JSONREADER_HPP
