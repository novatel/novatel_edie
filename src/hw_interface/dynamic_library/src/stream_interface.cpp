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
//  DESCRIPTION:
//    Class to provide decoder functionality
//
////////////////////////////////////////////////////////////////////////////////
#include "stream_interface.hpp"

InputFileStream* ifs_init(const char* file)
{
   return new InputFileStream(file);
}

void ifs_read(InputFileStream* ifs, StreamReadStatus* srs, char* databuffer, int& iSize)
{
   StreamReadStatus status;
   ReadDataStructure data;
   data.cData = databuffer;
   data.uiDataSize = iSize;
   // read data from input stream
   status = ifs->ReadData(data);
   srs->bEOS = status.bEOS;
   srs->uiCurrentStreamRead = status.uiCurrentStreamRead;
   srs->uiPercentStreamRead = status.uiPercentStreamRead;
   srs->ullStreamLength = status.ullStreamLength;
   iSize = data.uiDataSize;
}

void is_del(InputStreamInterface* pSteam)
{
   if (pSteam)
      delete pSteam;
}
