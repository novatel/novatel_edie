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
#include "framer.hpp"

INT get_frame_size()
{
   return FRAME_DATA_SIZE;
}

Framer* framer_init(InputStreamInterface* ifs, FilterConfig* filter)
{
   if (filter != NULL)
   {
      MessageDataFilter* pMessageFilter = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
      pMessageFilter->ConfigureFilter(*filter);
      return new Framer(ifs, *pMessageFilter);
   }
   else
   {
      return new Framer(ifs);
   }
}

void framer_del(Framer* pcFramer)
{
   if (pcFramer)
      delete pcFramer;
}

void framer_read(Framer* pcFramer, StreamReadStatus* pcSrs, PythonBaseMessage* szFramedData)
{
   BaseMessageData* data;
   *pcSrs = pcFramer->ReadMessage(&data);
   if ((pcSrs->bEOS) || pcSrs->uiCurrentStreamRead == 0)
      return;

   BaseMessageDataToPythonBaseMessage(szFramedData, data);
   delete data;
}
