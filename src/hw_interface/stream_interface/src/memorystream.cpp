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
//
//  DESCRIPTION: Basic Memory Stream Functions.
//    
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "memorystream.hpp"
using namespace std;

// ---------------------------------------------------------
MemoryStream::MemoryStream():CircularBuffer()
{

}

// ---------------------------------------------------------
MemoryStream::MemoryStream(UINT uiBufferSize = 1024):CircularBuffer()
{
   SetCapacity(uiBufferSize);
}

// ------------------------------------------------------------------------------------
MemoryStream::MemoryStream(UCHAR* pucBuffer, UINT uiContentSize):CircularBuffer()
{
   UINT uiBytesAppended = 0;
   uiBytesAppended = Append(pucBuffer, uiContentSize);
   
   if(uiBytesAppended != 0)
   {
	  stMemoryReadStatus.ullStreamLength += uiBytesAppended;
     stMemoryReadStatus.bEOS = FALSE;
   }
}

// ---------------------------------------------------------
MemoryStream::~MemoryStream()
{

}

// ---------------------------------------------------------
// Returns the actual content size, this is not the memory allocated size
// The size increases when we write and it decreases when read.
INT MemoryStream::Available()
{
   return (INT)GetLength();
}


// ---------------------------------------------------------
//Clear the data in the buffer
void MemoryStream::Flush()
{
   Clear();
}

// ---------------------------------------------------------
// Read the desired size from the buffer, if available 
StreamReadStatus MemoryStream::Read(CHAR* pucBuffer, UINT uisize)
{
   UINT length = 0;
   UINT bytes_Read = Copy((UCHAR *)pucBuffer,uisize);
   Discard(bytes_Read);
   length = GetLength();
   stMemoryReadStatus.uiCurrentStreamRead = bytes_Read;
   if(length == 0)
   {
      stMemoryReadStatus.uiPercentStreamRead = CalculatePercentage((UINT)stMemoryReadStatus.ullStreamLength - 0);
      stMemoryReadStatus.bEOS = TRUE;
   }
   else
   {
      stMemoryReadStatus.bEOS = FALSE;
      stMemoryReadStatus.uiPercentStreamRead = CalculatePercentage((UINT)stMemoryReadStatus.ullStreamLength - length);
   }
   return stMemoryReadStatus;
}

// ---------------------------------------------------------
// Read the one byte from the buffer
UINT MemoryStream::Read(void)
{
   UINT length = 0;
   UINT byte = GetByte(0);
   Discard(1);
   length = GetLength();
   stMemoryReadStatus.uiCurrentStreamRead = 1;
   if(length == 0)
   {
      stMemoryReadStatus.uiPercentStreamRead = CalculatePercentage((UINT)stMemoryReadStatus.ullStreamLength - 0);
      stMemoryReadStatus.bEOS = TRUE;
   }
   else
   {
      stMemoryReadStatus.bEOS = FALSE;
      stMemoryReadStatus.uiPercentStreamRead = CalculatePercentage((UINT)stMemoryReadStatus.ullStreamLength - length);
   }
   return byte;
}

// ---------------------------------------------------------
//Write Data to buffer of dezired size
UINT MemoryStream::Write(UCHAR* pucBuffer, UINT uisize)
{
   UINT uiBytesAppended = 0;
   uiBytesAppended = Append(pucBuffer, uisize);
   
   if(uiBytesAppended != 0)
   {
	  stMemoryReadStatus.ullStreamLength += uiBytesAppended;
     stMemoryReadStatus.bEOS = FALSE;
   }
   return uiBytesAppended;
}

// ---------------------------------------------------------
//Write one byte or character to buffer
UINT MemoryStream::Write(UCHAR pucBuffer)
{
   UINT uiBytesAppended = 0;
   uiBytesAppended = Append(&pucBuffer, 1);
   
   if(uiBytesAppended != 0)
   {
	  stMemoryReadStatus.ullStreamLength += uiBytesAppended;
     stMemoryReadStatus.bEOS = FALSE;
   }
   return uiBytesAppended;
}

// ---------------------------------------------------------
// Calculates the percentage of current Memory read.
UINT MemoryStream::CalculatePercentage(UINT uiCurrentMemoryRead)
{
   UINT uiResult = ((uiCurrentMemoryRead*100)/(UINT)stMemoryReadStatus.ullStreamLength);
   return uiResult;
}
