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
#include "outputmemorystream.hpp"

OutputMemoryStream::OutputMemoryStream()
{
   pMyOutMemoryStream = new MemoryStream();
}
// ---------------------------------------------------------
OutputMemoryStream::OutputMemoryStream(UINT uiBufferSize = 1024)
{
   pMyOutMemoryStream = new MemoryStream(uiBufferSize);
}

// ---------------------------------------------------------
OutputMemoryStream::OutputMemoryStream(UCHAR* pucBuffer, UINT uiContentSize)
{
   pMyOutMemoryStream = new MemoryStream(pucBuffer, uiContentSize);
}

// ---------------------------------------------------------
OutputMemoryStream::~OutputMemoryStream()
{
   delete pMyOutMemoryStream;
}

// ---------------------------------------------------------
// for now it is BaseMessageData, but it will be replaced with BaseTransportData when available
UINT OutputMemoryStream::WriteData(BaseMessageData& pBaseMessageData)
{
   UINT uiReturn = 0;
   uiReturn = pMyOutMemoryStream->Write((UCHAR*)pBaseMessageData.getMessageData() , pBaseMessageData.getMessageLength());
   return uiReturn;
}
