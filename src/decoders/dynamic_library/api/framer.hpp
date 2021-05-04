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

/*! \file   composer.hpp
 *  \brief  Class to start conversion from ASCII to BINARY and vice vera process
 */

#ifndef DYNAMIC_LIBRARY_FRAMER_HPP
#define DYNAMIC_LIBRARY_FRAMER_HPP

#include "decoders_export.h"
#include "src/decoders/novatel/api/framer.hpp"
#include "src/decoders/common/api/common.hpp"

#include <string.h>

 // Allocate 64k of data for the log frame
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

extern "C" {
   DECODERS_EXPORT INT get_frame_size();

   DECODERS_EXPORT Framer* framer_init(InputStreamInterface* ifs, FilterConfig* filter);
   DECODERS_EXPORT void framer_del(Framer* pcFramer);
   DECODERS_EXPORT void framer_read(Framer* pcFramer, StreamReadStatus* pcSrs, PythonBaseMessage* szFramedData);
}

PythonBaseMessage* BaseMessageDataToPythonBaseMessage(PythonBaseMessage* pPBM, BaseMessageData* pBMD)
{
   strcpy(pPBM->szMessageName, pBMD->getMessageName().c_str());
   pPBM->uiMessageID = pBMD->getMessageID();
   pPBM->ulPortAddressEnum = pBMD->getMessagePort();
   pPBM->uiMessageLength = pBMD->getMessageLength();
   pPBM->dIdleTime = pBMD->getIdleTime();
   pPBM->ulMessageWeek = pBMD->getMessageTimeWeek();
   pPBM->ulMessageTime = pBMD->getMessageTimeMilliSeconds();
   pPBM->eMessageFormat = pBMD->getMessageFormat();
   pPBM->eMessageTimeStatus = pBMD->getMessageTimeStatus();
   pPBM->ulReceiverStatus = pBMD->getReceiverStatus();
   pPBM->ulSWVersion = pBMD->getReceiverSWVersion();
   pPBM->bIsResponse = pBMD->getResponseID();
   memcpy(pPBM->cpFrameData, pBMD->getMessageData(), pBMD->getMessageLength());
   pPBM->cpFrameData[pBMD->getMessageLength()] = 0;

   return pPBM;
}

#endif
