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

/*! \file   novatel.hpp
 *  \brief  Class to start conversion from ASCII to BINARY and vice vera process
 */

#ifndef DYNAMIC_LIBRARY_FRAMER_HPP
#define DYNAMIC_LIBRARY_FRAMER_HPP

#include "decoders_export.h"
#include "src/decoders/novatel/api/decoder.hpp"
#include "src/decoders/common/api/common.hpp"

#include <string.h>

 // Allocate 20k of data for the log frame
#define FRAME_DATA_SIZE 65536

struct PythonBaseMessage
{
   CHAR szMessageName[100];
   uint32_t uiMessageID;
   uint32_t ulPortAddressEnum;
   uint32_t uiMessageLength;
   double_t dIdleTime;
   uint32_t ulMessageWeek;
   uint32_t ulMessageTime;
   MessageFormatEnum eMessageFormat;
   MessageTimeStatusEnum eMessageTimeStatus;
   uint32_t ulReceiverStatus;
   uint32_t ulSWVersion;
   uint32_t bIsResponse;
   uint8_t cpFrameData[FRAME_DATA_SIZE];
};

struct MessageHeader2
{
   void copyBMD(BaseMessageData* bmd)
   {
      strcpy(cMessageName, bmd->getMessageName().c_str());
      uiMessageID = bmd->getMessageID();
      ulPortAddressEnum = bmd->getMessagePort();
      uiMessageLength = bmd->getMessageLength();
      dIdleTime = bmd->getIdleTime();
      ulMessageWeek = bmd->getMessageTimeWeek();
      ulMessageTime = bmd->getMessageTimeMilliSeconds();
      eMessageFormat = bmd->getMessageFormat();
      eMessageTimeStatus = bmd->getMessageTimeStatus();
      ulReceiverStatus = bmd->getReceiverStatus();
      ulSWVersion = bmd->getReceiverSWVersion();
      bIsResponse = bmd->getMessageType();
   }

   CHAR cMessageName[100];
   UINT uiMessageID;
   ULONG ulPortAddressEnum;
   UINT uiMessageLength;
   DOUBLE dIdleTime;
   ULONG ulMessageWeek;
   ULONG ulMessageTime;
   MessageFormatEnum eMessageFormat;
   MessageTimeStatusEnum eMessageTimeStatus;
   ULONG ulReceiverStatus;
   ULONG ulSWVersion;
   ULONG bIsResponse;
};

extern "C" {
   DECODERS_EXPORT Decoder* decoder_init(char* json_database, InputFileStream* ifs, FilterConfig* filter = NULL);
   DECODERS_EXPORT BOOL decoder_read(Decoder* advDecoder, StreamReadStatus* srs, MessageHeader2* msgHdr, CHAR* pacMsgData, CHAR* pacJsonData);
   DECODERS_EXPORT void decoder_del(Decoder* advDecoder);
   DECODERS_EXPORT void decoder_copy_body(CHAR* pDestBody, CHAR* pSrcBody);
   DECODERS_EXPORT void decoder_copy_header(MessageHeader2* pDestHeader, MessageHeader2* pSrcHeader);

}

//PythonBaseMessage* BaseMessageDataToPythonBaseMessage(PythonBaseMessage* pPBM, BaseMessageData* pBMD)
//{
//   strcpy_s(pPBM->szMessageName, 100, pBMD->getMessageName().c_str());
//   pPBM->uiMessageID = pBMD->getMessageID();
//   pPBM->ulPortAddressEnum = pBMD->getMessagePort();
//   pPBM->uiMessageLength = pBMD->getMessageLength();
//   pPBM->dIdleTime = pBMD->getIdleTime();
//   pPBM->ulMessageWeek = pBMD->getMessageTimeWeek();
//   pPBM->ulMessageTime = pBMD->getMessageTimeMilliSeconds();
//   pPBM->eMessageFormat = pBMD->getMessageFormat();
//   pPBM->eMessageTimeStatus = pBMD->getMessageTimeStatus();
//   pPBM->ulReceiverStatus = pBMD->getReceiverStatus();
//   pPBM->ulSWVersion = pBMD->getReceiverSWVersion();
//   pPBM->bIsResponse = pBMD->getResponseID();
//   memcpy_s(pPBM->cpFrameData, FRAME_DATA_SIZE, pBMD->getMessageData(), pBMD->getMessageLength());
//   pPBM->cpFrameData[pBMD->getMessageLength()] = 0;
//
//   return pPBM;
//}

#endif
