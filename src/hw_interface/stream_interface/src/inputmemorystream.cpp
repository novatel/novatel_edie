////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 NovAtel Inc.
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
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "inputmemorystream.hpp"

// ---------------------------------------------------------
InputMemoryStream::InputMemoryStream()
{
   pMyInMemoryStream = new MemoryStream();
}

// ---------------------------------------------------------
InputMemoryStream::InputMemoryStream(UINT uiBufferSize = 1024)
{
   pMyInMemoryStream = new MemoryStream(uiBufferSize);
}

// ---------------------------------------------------------
InputMemoryStream::InputMemoryStream(UCHAR* pucBuffer, UINT uiContentSize)
{
   pMyInMemoryStream = new MemoryStream(pucBuffer,uiContentSize);
}

// ---------------------------------------------------------
InputMemoryStream::~InputMemoryStream()
{
   delete pMyInMemoryStream;
}

// ---------------------------------------------------------
StreamReadStatus InputMemoryStream::ReadData(ReadDataStructure& pReadDataStructure )
{
   StreamReadStatus stMemoryReadStatus;
   stMemoryReadStatus = pMyInMemoryStream->Read(pReadDataStructure.cData , pReadDataStructure.uiDataSize );
   return stMemoryReadStatus;
}

// ---------------------------------------------------------
UINT InputMemoryStream::Write(UCHAR* pcData_,UINT uiBytes_ )
{
   UINT uiBytes;
   uiBytes = pMyInMemoryStream->Write(pcData_ , uiBytes_ );
   return uiBytes;
}

// ---------------------------------------------------------
BOOL InputMemoryStream::IsStreamAvailable(void)
{
   if(pMyInMemoryStream->Available())
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}
