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
#include "novatel.hpp"


Decoder* decoder_init(char* json_database, InputFileStream* ifs, FilterConfig* filter)
{
   Decoder* decoder = NULL;

   loadDataFromJSON::initializeDatabaseObj();
   JSONFileReader* jsonReader = NULL;
   jsonReader = loadDataFromJSON::getDatabaseObj(json_database);

   if (filter != NULL)
   {
      MessageDataFilter* pMessageFilter = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
      pMessageFilter->ConfigureFilter(*filter);
      decoder = new Decoder(ifs, *pMessageFilter);
   }
   else
   {
      decoder = new Decoder(ifs);
   }
   ////Unknown data or messages will not be considered
   decoder->EnableUnknownData(FALSE);
   decoder->SetBMDOutput(BMDOutputFormat::BOTH);

   return decoder;
}


BOOL decoder_read(Decoder* advDecoder, StreamReadStatus* srs, MessageHeader2* msgHdr, CHAR* pacMsgData, CHAR* pacJsonData)
{
   BOOL bReturnCode = FALSE;
   BaseMessageData* bmd = NULL;

   try
   {
      // TODO: Enable unknown data when BaseMessageData has a CopyMessage function. Right now if decoding fails/has unknown bytes BMD will throw an exception as it
      // doesn't have a CopyMessage function

      pacMsgData[0] = '\0';

      // Read the message via advanced decoder
      *srs = advDecoder->ReadMessage(&bmd);
      if (bmd != NULL)
      {
         msgHdr->copyBMD(bmd);
         std::string sLog;
         if (bmd->getMessageName() == "" || bmd->getMessageID() == (UINT)-1)
            sLog = "{\"header\": " + bmd->getjsonHeaderData() + "}";
         else
            sLog = "{\"header\": " + bmd->getjsonHeaderData() + ", \"body\": " + bmd->getjsonMessageData() + "}";
         strcpy(pacJsonData, sLog.c_str());
         memcpy(pacMsgData, bmd->getMessageData(), bmd->getMessageLength());
         delete bmd;
         bmd = NULL;
      }
      else
      {
         bReturnCode = FALSE;
      }
   }

   // Keep this print for the time being. If an exception occurs during decoding then the user can see it happening.
   // StreamReadStatus.uiCurrentStreamRead doesn't work correctly so the user won't be able to see where the exception occurred.
   //
   // Depending on where the exception was thrown the header could contain the previously successfully decoded logs values.
   catch (const char* exception)
   {
      bReturnCode = FALSE;
   }

   // Base message data must be deleted
   if (bmd)
   {
      delete bmd;
      bmd = NULL;
   }

   return bReturnCode;
}

void decoder_del(Decoder* advDecoder)
{
   delete advDecoder;
}

void decoder_copy_header(MessageHeader2* pDestHeader, MessageHeader2* pSrcHeader)
{
   memcpy(pDestHeader, pSrcHeader, sizeof(MessageHeader2));
}

void decoder_copy_body(CHAR* pDestBody, CHAR* pSrcBody)
{
   memcpy(pDestBody, pSrcBody, FRAME_DATA_SIZE);
}
