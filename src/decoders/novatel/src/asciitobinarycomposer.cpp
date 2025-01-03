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
//    Class to Start the ASCII to BINARY conversion
//
////////////////////////////////////////////////////////////////////////////////
#include "asciitobinarycomposer.hpp"

AsciiToBinaryComposer::AsciiToBinaryComposer() :
   MessageStatus(MsgConvertStatusEnum(-1)),
   pBaseData(NULL), eMyPort(CommunicationPortEnum(-1)),
   eMyFormat(ASCII), eMySeparatorStatus(SeparatorEnum(-1)),
   pstBinaryHeader(), pstBinaryShortHeader(),
   eMyMessageStyle(MessageStyleEnum(-1)), writeData(NULL),
   pucMyInputData(NULL), szMyLastParameter(NULL), sNextSeparator(NULL),
   pucMyBufferEnd(NULL), pucIOMessageBody(NULL), bMyFlipCRC(FALSE),
   bStatus(FALSE), bLength(FALSE),
   ulMyParameterNumber(0), ulMyFieldNumber(0),ulMyTotalClassArrayParameterNumber(0),
	ulMyCopiedClassArrayParameterNumber(0),
   ulSize(0), iMessageId(0), ulCopyParam(0), strEnumName(""),
   dbData(NULL)
{
   timeStatus.insert(std::pair<std::string, INT>("UNKNOWN", 20));
   timeStatus.insert(std::pair<std::string, INT>("ADJUSTING", 40));
   timeStatus.insert(std::pair<std::string, INT>("APPROXIMATE", 60));
   timeStatus.insert(std::pair<std::string, INT>("COARSEADJUSTING", 80));
   timeStatus.insert(std::pair<std::string, INT>("COARSE", 100));
   timeStatus.insert(std::pair<std::string, INT>("COARSESTEERING", 120));
   timeStatus.insert(std::pair<std::string, INT>("FREEWHEELING", 130));
   timeStatus.insert(std::pair<std::string, INT>("FINEADJUSTING", 140));
   timeStatus.insert(std::pair<std::string, INT>("FINE", 160));
   timeStatus.insert(std::pair<std::string, INT>("FINEBACKUPSTEERING", 170));
   timeStatus.insert(std::pair<std::string, INT>("FINESTEERING", 180));
   timeStatus.insert(std::pair<std::string, INT>("SATTIME", 200));
   timeStatus.insert(std::pair<std::string, INT>("EXTERN", 220));
   timeStatus.insert(std::pair<std::string, INT>("EXACT", 240));
   bMyFlipCRC = FALSE;
   ulMyParameterNumber = 0;
   ulSize = 0;
   dbData = loadDataFromJSON::getDatabaseObj();
}

AsciiToBinaryComposer::~AsciiToBinaryComposer()
{

}


MsgConvertStatusEnum AsciiToBinaryComposer::Convert(BaseMessageData *pclBaseData, CHAR* writeData_,BOOL bIsRxConfig)
{
   //Conversion type is BINARY and got BINARY message 
   //Write the data to output file as it is available in Base message data.
   if ((pclBaseData->getMessageFormat() == MESSAGE_BINARY || pclBaseData->getMessageFormat() == MESSAGE_UNKNOWN) && (!bIsRxConfig))
   {
      return GOOD_MESSAGE;
   }
   pBaseData = pclBaseData;
   stringstream strMsgDefCrc;
   if (bIsRxConfig)
   {
      pucMyInputData = szMyLastParameter;
   }
   else
   {
      writeData_ = new CHAR[MAX_ASCII_BUFFER_SIZE];
      memset(writeData_, 0, MAX_ASCII_BUFFER_SIZE);
      writeData = writeData_;
	  pucMyInputData = new CHAR[pclBaseData->getMessageLength()+1];
	  memcpy(pucMyInputData, pclBaseData->getMessageData(), pclBaseData->getMessageLength()+1);
      //pclBaseData->setMessageFormat(MESSAGE_BINARY);
      pucIOMessageBody = (UCHAR*)writeData;
   }

   if (*pucMyInputData == '\n')
   {
      pclBaseData->setMessageData(writeData_);
      return BLANK_MESSAGE;
   }

   CHAR* sCurrentByte_ = pucMyInputData;

   INT iLength = static_cast<INT>(strlen(pucMyInputData));

   CHAR* sEnd = strchr(pucMyInputData, '\r');

   if (sEnd != NULL)
   {
      iLength = static_cast<INT>(sEnd - pucMyInputData + 1);
   }
   else
   {
      sEnd = sCurrentByte_ + iLength - 1;
   }


   pucMyBufferEnd = (UCHAR*)writeData;
   eMyPort = MSGCVT_COMPORT1;

   szMyLastParameter = NULL;

   while ((sCurrentByte_ <= pucMyInputData + iLength) &&
      ((*sCurrentByte_ == ' ') || (*sCurrentByte_ == '\0') || (*sCurrentByte_ == ',') ||
      (*sCurrentByte_ == ';') || (*sCurrentByte_ == '*') || (*sCurrentByte_ == '\r') || (*sCurrentByte_ == '\n')))
   {
      sCurrentByte_++;
   }
   if (sCurrentByte_ > pucMyInputData + iLength)
   {
      pclBaseData->setMessageData(writeData_);
      return BLANK_MESSAGE;
   }
   if (IsResponseMessage(&sCurrentByte_))
   {
      pclBaseData->setMessageData(writeData_);
      return RESPONSE_MESSAGE;
   }

   if (!bMyFlipCRC)
   {
      memset(pucIOMessageBody, 0, MESSAGE_SIZE_MAX);
   }

   if (sCurrentByte_ >= pucMyInputData + iLength)
   {
      pclBaseData->setMessageData(writeData_);
      return EMPTY_MESSAGE;
   }

   CHAR* sCrc;
   sCrc = strchr(sCurrentByte_, '\r');
   if (sCrc != NULL)
   {
      *sCrc = '\0';
   }

   if (*sCurrentByte_ == OEM4_ASCII_SYNC)
   {
      memset(&pstBinaryHeader, 0, sizeof(pstBinaryHeader));
      ++sCurrentByte_;
      eMyFormat = ASCII;
      eMyMessageStyle = OEM4_MESSAGE_STYLE;
      pstBinaryHeader.ucSync1 = OEM4_BINARY_SYNC1;
      pstBinaryHeader.ucSync2 = OEM4_BINARY_SYNC2;
      pstBinaryHeader.ucSync3 = OEM4_BINARY_SYNC3;
      pstBinaryHeader.ucHeaderLength = 28;
      pclBaseData->setMessageFormat(MESSAGE_BINARY);
   }
   else
   {
      memset(&pstBinaryShortHeader, 0, sizeof(pstBinaryShortHeader));
      ++sCurrentByte_;
      eMyFormat = ASCII;
      eMyMessageStyle = OEM4_COMPRESSED_STYLE;
      pstBinaryShortHeader.ucSync1 = OEM4_BINARY_SYNC1;
      pstBinaryShortHeader.ucSync2 = OEM4_BINARY_SYNC2;
      pstBinaryShortHeader.ucSync3 = OEM4_SHORT_BINARY_SYNC3;
      pclBaseData->setMessageFormat(MESSAGE_SHORT_HEADER_BINARY);
   }
   if (eMyFormat == ASCII)
   {
      // Find the CRC.  If there is no seperator between the message
      // body and CRC the message is invalid.
      // Backup 9 characters from the <CR> and confirm the *CRC exists
      if ((sCrc == NULL) || (*(sCrc - 9) != '*'))
      {
         pclBaseData->setMessageData(writeData_);
         return(INVALID_MESSAGE_FORMAT);
      }

      ULONG ulCrc = 0;
      if (!sscanf(sCrc - 8, "%lx", &ulCrc))
      {
         pclBaseData->setMessageData(writeData_);
         return (INVALID_CRC);
      }

      if (bMyFlipCRC)
      {
         ulCrc = ulCrc ^ 0xFFFFFFFF;
      }

      // Calculate the CRC
      CRC32 clCrc;
      if (clCrc.CalculateBlockCRC32(static_cast<UINT>(sCrc - sCurrentByte_ - 9), 0, (UCHAR*)(sCurrentByte_)) != ulCrc)
      {
         pclBaseData->setMessageData(writeData_);
         return (INVALID_CRC);
      }
   }

   MsgConvertStatusEnum eHeaderStatus = ExtractHeader(&sCurrentByte_, pucMyInputData + iLength, eMyPort);

   if (eHeaderStatus == GOOD_MESSAGE)
   {
      if (eMyMessageStyle == OEM4_COMPRESSED_STYLE)
      {
         memcpy(pucIOMessageBody, &pstBinaryShortHeader, sizeof(pstBinaryShortHeader));
         pucIOMessageBody += sizeof(pstBinaryShortHeader);
         iMessageId = pstBinaryShortHeader.usMessageId;
         strMsgDefCrc << dbData->getMsgDefCRC(iMessageId);
      }
      else
      {
         memcpy(pucIOMessageBody, &pstBinaryHeader, sizeof(pstBinaryHeader));
         pucIOMessageBody += sizeof(pstBinaryHeader);
         iMessageId = pstBinaryHeader.usMsgNumber;
         //Framing unique for each message with MsgName_MsgId_MsgDefCrc
         USHORT usMsgDefCrc = pstBinaryHeader.usMsgDefCRC;
         strMsgDefCrc << std::setfill('0') << setw(4) << std::hex << usMsgDefCrc;
      }

      if (bIsRxConfig)
      {
         strMsgDefCrc << dbData->getMsgDefCRC(pstBinaryHeader.usMsgNumber);
         bIsRxConfig = FALSE;
      }

      //std::string strMsgName = GetMessageNameByID(iMessageId) + "_" + std::to_string(iMessageId) + "_" + strMsgDefCrc.str().c_str();
	  std::string strMsgName = pclBaseData->getMessageName() + "_" + std::to_string(iMessageId);
      const vector<CMessageParams*> messgeParamList = dbData->getMessageParamObj(strMsgName);
      const vector<CTypes*> typesList = dbData->getTypesObj(strMsgName);
      const vector<CEnums*> enumsList = dbData->getEnumObj(strMsgName);
      const vector<CMessage*> messgeList = dbData->getMessageObj(strMsgName);

      ULONG ulMaxNumParameters = 0;
      ulMyParameterNumber = 0;
      ulMyFieldNumber = 0;

      if ((INT)messgeList.size() > 0)
         ulMaxNumParameters = messgeList.at(0)->getNoOfParamasById(iMessageId);
      MsgConvertStatusEnum eParameterStatus = GOOD_MESSAGE;

      while (ulMyParameterNumber < ulMaxNumParameters)
      {
         szMyLastParameter = sCurrentByte_;

         if (sCurrentByte_ == NULL)
            sCurrentByte_ = sEnd;
         eParameterStatus = ASCIIParameterToBinary(iMessageId, &sCurrentByte_, pucMyInputData + iLength, &pucIOMessageBody, messgeList, messgeParamList, typesList, enumsList);

         if (eParameterStatus == UNEXPECTED_END_OF_MESSAGE)
         {
            break;
         }
         else if (eParameterStatus != GOOD_MESSAGE)
         {
            pclBaseData->setMessageData(writeData_);
            return (INVALID_MESSAGE_FORMAT);
         }
      }

   }
   if (eMyMessageStyle == OEM4_COMPRESSED_STYLE)
   {
      USHORT usLength = static_cast<USHORT>(pucMyBufferEnd - (UCHAR*)writeData_);
      pstBinaryShortHeader.ucLength = usLength - 12/*pucMyBufferEnd - (UCHAR*)writeData_*/;
      USHORT usActualMsgLength = usLength - 12;
      memcpy(writeData_ + 3, &usActualMsgLength, 1);
      CRC32 clCrc;

      //Calculate the CRC over the entire header and data.
      UINT ulCrc = clCrc.CalculateBlockCRC32(usLength, 0, (UCHAR*)writeData_);

      UINT* uiCrc = (UINT*)pucIOMessageBody;
      *uiCrc = ulCrc;
      pucIOMessageBody += 4;
      pclBaseData->setMessageLength(usLength + 4);
      pclBaseData->setMessageData(writeData_);
   }
   else
   {
      ULONG iMyMessageLength = static_cast<ULONG>(pucIOMessageBody - (UCHAR*)writeData_);
      //pstBinaryHeader.usLength = pucIOMessageBody - (UCHAR*)writeData_ - 28;
      USHORT iActualMsgLength = 0;
      if (bMyFlipCRC)
      {
         iActualMsgLength = (USHORT)iMyMessageLength - 56;
         iMyMessageLength -= 28;
         writeData_ += 28;
      }
      else
      {
         iActualMsgLength = (USHORT)iMyMessageLength - 28;
      }

      pstBinaryHeader.usLength = iActualMsgLength;
      memcpy(writeData_ + LENGTH_OFFSET, &iActualMsgLength, 2);
      CRC32 clCrc;

      //Calculate the CRC over the entire header and data.
      UINT ulCrc = clCrc.CalculateBlockCRC32(iMyMessageLength, 0, (UCHAR*)writeData_);

      //Flip the internal CRC for RxConfig Msg
      ulCrc = bMyFlipCRC ? ulCrc ^ 0xFFFFFFFF : ulCrc;

      UINT* uiCrc = (UINT*)pucIOMessageBody;
      *uiCrc = ulCrc;
      pucIOMessageBody += 4;
      //memcpy(writeData_ + iMyMessageLength, &ulCrc, sizeof(ulCrc));
      //writeData_ += BINARY_CRC_SIZE;
      iMyMessageLength += BINARY_CRC_SIZE;

      iLength = pstBinaryHeader.usLength;
      pclBaseData->setMessageLength(iMyMessageLength);
      pclBaseData->setMessageData(writeData_);
   }
   return eHeaderStatus;
}

BOOL AsciiToBinaryComposer::IsResponseMessage(CHAR** psStart_)
{
   //First see if we're dealing with an abbreviated ASCII or response message.
   if (*(*psStart_) == '<')
   {
      return true;
   }
   CHAR* szTempStart = *psStart_;
   if (*szTempStart == '[')
   {
      while ((*szTempStart != ']') && (*szTempStart != '\n'))
      {
         szTempStart++;
      }

      if (*szTempStart == ']')
      {
         *psStart_ = szTempStart + 1;
      }
   }
   return FALSE;
}

MsgConvertStatusEnum AsciiToBinaryComposer::ExtractHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CommunicationPortEnum ePort_)
{
   std::string strCurrentByte;
   strCurrentByte.clear();
   std::string strTemp = "_1";
   std::string strSub;
   strSub.clear();

   CHAR* sNextSeparator = *psCurrentByte_;
   eMySeparatorStatus = clMySeparator.FindNext(eMyFormat, psCurrentByte_, &sNextSeparator, pcEnd_);

   if (eMySeparatorStatus == PAST_END_OF_BUFFER)
   {
      return (INVALID_MESSAGE_FORMAT);
   }

   strCurrentByte = *psCurrentByte_;
   INT size = static_cast<INT>(strCurrentByte.size() - 2);
   strSub = strCurrentByte.substr(size);

   if (strSub.compare(strTemp) == 0)
   {
      strCurrentByte.resize(size);
      strcpy(*psCurrentByte_, strCurrentByte.c_str());
      bStatus = TRUE;
   }
   else
   {
      bStatus = FALSE;
   }

   std::string strMsgName = *psCurrentByte_;
   MsgConvertStatusEnum eStatus;
   if (eMyMessageStyle == OEM4_COMPRESSED_STYLE)
   {
      eStatus = GetMessageId(strMsgName, (ULONG*)&pstBinaryShortHeader.usMessageId);
   }
   else
   {
      eStatus = GetMessageId(strMsgName, (ULONG*)&pstBinaryHeader.usMsgNumber);
   }

   if (bStatus)
      pstBinaryHeader.ucMsgType = 1;

   if (eStatus != GOOD_MESSAGE)
      return eStatus;

   if (eMySeparatorStatus != HEADER_TERMINATOR_FOUND)
   {
      *psCurrentByte_ = sNextSeparator;
   }

   if (eMyMessageStyle == OEM4_COMPRESSED_STYLE)
   {
      return ExtractCompressedHeader(psCurrentByte_, pcEnd_, sNextSeparator, ePort_);
   }
   else
   {
      return ExtractGeneralHeader(psCurrentByte_, pcEnd_, sNextSeparator, ePort_);
   }

}

MsgConvertStatusEnum AsciiToBinaryComposer::ExtractCompressedHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CHAR* sNextSeparator_, CommunicationPortEnum ePort_)
{
   CHAR cDummy;

   INT iLastField = (eMySeparatorStatus == HEADER_TERMINATOR_FOUND) ? 1 : HEADER_NUM_ASCII_FIELDS;

   for (INT iFieldNumber = 2; iFieldNumber <= HEADER_NUM_COMP_ASCII_FIELDS; ++iFieldNumber)
   {
      if (eMySeparatorStatus != HEADER_TERMINATOR_FOUND)
      {
         // Don't continue to parse the buffer if we have reached the end of the header
         eMySeparatorStatus = clMySeparator.FindNext(eMyFormat, psCurrentByte_, &sNextSeparator_, pcEnd_);
         if (eMySeparatorStatus == HEADER_TERMINATOR_FOUND)
         {
            iLastField = iFieldNumber;
         }

         else if (eMySeparatorStatus == PAST_END_OF_BUFFER)
         {
            return(INVALID_MESSAGE_FORMAT);
         }
      }

      if (iFieldNumber == HEADER_COMP_WEEK_NUMBER_FIELD)
      {
         ULONG ulWeekNumber;
         if (iLastField >= HEADER_COMP_WEEK_NUMBER_FIELD)
         {
            if (sscanf(*psCurrentByte_, "%lu%c", &ulWeekNumber, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);
            pstBinaryShortHeader.usWeekNo = (USHORT)ulWeekNumber;
         }
         else
         {
            pstBinaryShortHeader.usWeekNo = 0;
         }
         
      }
      // If this field is not missing, extract it
      else if (iFieldNumber == HEADER_COMP_MILLISECONDS_FIELD)
      {
         if (iLastField >= HEADER_COMP_MILLISECONDS_FIELD)
         {
            DOUBLE dMilliseconds;
            if (sscanf(*psCurrentByte_, "%lf%c", &dMilliseconds, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);

            pstBinaryShortHeader.ulWeekMSec = (ULONG)(dMilliseconds * SEC_TO_MSEC);
         }
         else
         {
            pstBinaryShortHeader.ulWeekMSec = 0;
         }
         
      }

      if (eMySeparatorStatus != HEADER_TERMINATOR_FOUND)
         *psCurrentByte_ = sNextSeparator_;
   }
   if (eMySeparatorStatus != HEADER_TERMINATOR_FOUND)
   {
      sNextSeparator_ = strchr(*psCurrentByte_, ASCII_HEADER_TERMINATOR);
      if (sNextSeparator_ == NULL)
         return (INVALID_MESSAGE_FORMAT);
      else
         ++sNextSeparator_;
   }
   *psCurrentByte_ = sNextSeparator_;

   if (iLastField < HEADER_COMP_MILLISECONDS_FIELD)
      return (GOOD_MESSAGE_NO_TIME);
   else
      return (GOOD_MESSAGE);
}

MsgConvertStatusEnum AsciiToBinaryComposer::ExtractGeneralHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CHAR* sNextSeparator_, CommunicationPortEnum ePort_)
{
   CHAR cDummy;

   INT iLastField = (eMySeparatorStatus == HEADER_TERMINATOR_FOUND) ? 1 : HEADER_NUM_ASCII_FIELDS;

   for (INT iFieldNumber = 2; iFieldNumber <= HEADER_NUM_ASCII_FIELDS; ++iFieldNumber)
   {
      if (eMySeparatorStatus != HEADER_TERMINATOR_FOUND)
      {
         eMySeparatorStatus = clMySeparator.FindNext(eMyFormat, psCurrentByte_, &sNextSeparator_, pcEnd_);

         if (eMySeparatorStatus == HEADER_TERMINATOR_FOUND)
         {
            iLastField = iFieldNumber;
         }

         else if (eMySeparatorStatus == PAST_END_OF_BUFFER)
         {
            return (INVALID_MESSAGE_FORMAT);
         }
      }
      
      if (iFieldNumber == HEADER_PORT_ADDRESS_FIELD)
      {
         std::string strPort = *psCurrentByte_;
         if (iLastField >= HEADER_PORT_ADDRESS_FIELD)
         {
            if (strPort.compare("COM") == 0)
            {
               if ((*(*psCurrentByte_ + 3)) == '1')
                  pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT1, 0);
               else if ((*(*psCurrentByte_ + 3)) == '2')
                  pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT2, 0);
               else if ((*(*psCurrentByte_ + 3)) == '3')
                  pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT3, 0);
               else
                  pstBinaryHeader.ucPort = setComPort(ePort_, 0);
            }
            else if (strPort.compare("USB") == 0)
            {
               if ((*(*psCurrentByte_ + 3)) == '1')
                  pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT_USB1, 0);
               else if ((*(*psCurrentByte_ + 3)) == '2')
                  pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT_USB2, 0);
               else if ((*(*psCurrentByte_ + 3)) == '3')
                  pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT_USB3, 0);
               else
                  pstBinaryHeader.ucPort = setComPort(ePort_, 0);
            }
            else if (strPort.compare("XCOM1") == 0)
            {
               pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORTX1, 0);
            }
            else if (strPort.compare("XCOM2") == 0)
            {
               pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORTX2, 0);
            }
            else if (strPort.compare("XCOM3") == 0)
            {
               pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORTX3, 0);
            }
            else if (strPort.compare("AUX") == 0)
            {
               pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT_AUX, 0);
            }
            else if (strPort.compare("NO_PORT") == 0)
            {
               pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT1, 0);
            }
            else if (strPort.compare("UNKNOWN") == 0 || strPort.compare("SPECIAL") == 0)
            {
               pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT_UNKNOWN, 0);
            }
            else
            {
               pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT1, 0);
            }
         }
         else
         {
            pstBinaryHeader.ucPort = setComPort(MSGCVT_COMPORT1, 0);
         }
        
      }
      else if (iFieldNumber == HEADER_TIME_STATUS_FIELD)
      {
         ULONG ulEnumValue = 0;
         if (iLastField >= HEADER_TIME_STATUS_FIELD)
         {
            const INT iMaxBufferLength = 50;
            CHAR szTempBuffer[iMaxBufferLength];
            INT iCurrentPointer = 0;
            //Go until we run out of room or we hit a separator
            while ((iCurrentPointer < iMaxBufferLength - 1) && (*(*psCurrentByte_) != '\0'))
            {
               szTempBuffer[iCurrentPointer] = *(*psCurrentByte_);
               (*psCurrentByte_)++;
               iCurrentPointer++;
            }
            szTempBuffer[iCurrentPointer] = '\0';

            //skip over the separator now
            (*psCurrentByte_)++;

            //Get the enum value that goes with this string
            ulEnumValue = timeStatus.find(szTempBuffer)->second;
         }
         pstBinaryHeader.ucTimeStatus = (UCHAR)ulEnumValue;
      }

      else if (iFieldNumber == HEADER_WEEK_NUMBER_FIELD)
      {
         if (iLastField >= HEADER_WEEK_NUMBER_FIELD)
         {
            if (sscanf(*psCurrentByte_, "%hu%c", &pstBinaryHeader.usWeekNo, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);
         }
         else
         {
            pstBinaryHeader.usWeekNo = 0;
         }
      }
      else if (iFieldNumber == HEADER_MILLISECONDS_FIELD)
      {
         if (iLastField >= HEADER_MILLISECONDS_FIELD)
         {
            DOUBLE dMilliseconds;
            if (sscanf(*psCurrentByte_, "%lf%c", &dMilliseconds, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);

            dMilliseconds *= (DOUBLE)SEC_TO_MSEC;
            pstBinaryHeader.ulWeekMSec = (ULONG)dMilliseconds;
         }
         else
         {
            pstBinaryHeader.ulWeekMSec = 0;
         }
      }
      else if (iFieldNumber == HEADER_SEQ_NUM)
      {
         if (iLastField >= HEADER_SEQ_NUM)
         {
            unsigned int uTempNum = 0;
            if (sscanf(*psCurrentByte_, "%ud%c", &uTempNum, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);

            pstBinaryHeader.usSequenceNumber = (USHORT)uTempNum;
         }
         else
         {
            pstBinaryHeader.usSequenceNumber = 0;
         }
      }
      else if (iFieldNumber == HEADER_IDLE_TIME)
      {
         if (iLastField >= HEADER_IDLE_TIME)
         {
            FLOAT fTempValue = 0;
            if (sscanf(*psCurrentByte_, "%f%c", &fTempValue, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);

            pstBinaryHeader.ucIdleTime = (CHAR)(fTempValue * 2.0);
         }
         else
         {
            pstBinaryHeader.ucIdleTime = 0;
         }
      }

      else if (iFieldNumber == HEADER_RECEIVER_STATUS_FIELD)
      {
         if (iLastField >= HEADER_RECEIVER_STATUS_FIELD)
         {
            if (sscanf(*psCurrentByte_, "%lx%c", &pstBinaryHeader.ulStatus, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);
         }
         else
         {
            pstBinaryHeader.ulStatus = 0;
         }
        
      }

      else if (iFieldNumber == HEADER_MESSAGE_DEF_CRC_FIELD)
      {
         if (iLastField >= HEADER_MESSAGE_DEF_CRC_FIELD)
         {
            if (sscanf(*psCurrentByte_, "%hx%c", &pstBinaryHeader.usMsgDefCRC, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);
         }
         else
         {
            pstBinaryHeader.usMsgDefCRC = 0;
         }
      }

      else if (iFieldNumber == HEADER_RECEIVER_SW_VERSION_FIELD)
      {
         if (iLastField >= HEADER_RECEIVER_SW_VERSION_FIELD)
         {
            if (sscanf(*psCurrentByte_, "%hu%c", &pstBinaryHeader.usReceiverSWVersion, &cDummy) != 1)
               return (INVALID_MESSAGE_FORMAT);
         }
         else
         {
            pstBinaryHeader.usReceiverSWVersion = 0;
         }
      }
      if (eMySeparatorStatus != HEADER_TERMINATOR_FOUND)
         *psCurrentByte_ = sNextSeparator_;
   }
   if (eMySeparatorStatus != HEADER_TERMINATOR_FOUND)
   {
      sNextSeparator_ = strchr(*psCurrentByte_, ASCII_HEADER_TERMINATOR);
      if (sNextSeparator_ == NULL)
         return (INVALID_MESSAGE_FORMAT);
      else
         ++sNextSeparator_;
   }
   *psCurrentByte_ = sNextSeparator_;

   if (iLastField < HEADER_MILLISECONDS_FIELD)
      return (GOOD_MESSAGE_NO_TIME);
   else
      return (GOOD_MESSAGE);
}

MsgConvertStatusEnum AsciiToBinaryComposer::GetMessageId(std::string sMessageName_, ULONG* pulMessageId_)
{
   std::string strTempMsgName = sMessageName_;
   *pulMessageId_ = GetMessageIDByName(sMessageName_);

   if (*pulMessageId_ == 0)
   {
      INT iLength = static_cast<INT>(sMessageName_.size());

      // If not, ensure that the last character is an 'A' or 'B'
      //char* sLastCharacter = sMessageName_ + iLength - 1;
      std::string sLastCharacter = sMessageName_.substr(iLength - 1);

      if (sLastCharacter == "A")
      {
         eMyFormat = ASCII;
         strTempMsgName.resize(iLength - 1);
      }
      else if (sLastCharacter == "B")
      {
         eMyFormat = BINARY;
         strTempMsgName.resize(iLength - 1);
      }
      else
      {
         return(INVALID_MESSAGE_ID);
      }

      //if only a single charcter then it's invalid too
      if (iLength <= 1)
      {
         return(INVALID_MESSAGE_ID);
      }
      *pulMessageId_ = GetMessageIDByName(strTempMsgName);
   }
   return GOOD_MESSAGE;
}

MsgConvertStatusEnum AsciiToBinaryComposer::ASCIIParameterToBinary(const ULONG ulMessageId_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR** ppucIOMessageBody_, const std::vector<CMessage*>& messgeList, const std::vector<CMessageParams*>& messgeParamList, const std::vector<CTypes*>& typesList, const std::vector<CEnums*>& enumsList)
{
   MsgConvertStatusEnum eMessageStatus = GOOD_MESSAGE;
   ULONG ulMaxParameterNumber = messgeList.at(0)->getNoOfParamasById(ulMessageId_);

   ElementTypeEnum eElementType = SIMPLE;

   ULONG ulLength = 0;
   ULONG ulMaxArraySize = 0;
   ULONG ulBlockValue;
   ULONG ulBlockValue1 = 0;
   std::string Typename = "";

   do
   {
      if (messgeParamList.empty() || typesList.empty() || enumsList.empty())
         return INVALID_MESSAGE_ID;

      if (ulMyParameterNumber >= messgeParamList.size() || ulMyParameterNumber >= typesList.size() || ulMyParameterNumber >= enumsList.size())
      {
         return GOOD_MESSAGE;
      }

      ulSize = static_cast<ULONG>(messgeParamList.size());

      const CMessageParams* msgParm = messgeParamList.at(ulMyParameterNumber);
      const CTypes* typeParam = typesList.at(ulMyParameterNumber);
      const CEnums* enumParam = enumsList.at(ulMyParameterNumber);

      while ((*psCurrentByte_ < pcEnd_) && ((*(*psCurrentByte_) == ' ') || (*(*psCurrentByte_) == '\0') || (*(*psCurrentByte_) == ',') || (*(*psCurrentByte_) == '\r')))
      {
         (*psCurrentByte_)++;
      }

      ulLength = typeParam->getLength();
      eElementType = typeParam->getStorageTypeEnum();
      ulBlockValue = msgParm->getBlockValue();
	  if (typeParam->getBaseTypeName() == "")
	  {
		  Typename = typeParam->getTypeName();
	  }
	  else
	  {
		  Typename = typeParam->getBaseTypeName();
	  }

      if ((eElementType != CLASSARRAY) && (eElementType != CLASS))
         ulMyFieldNumber++;

      szMyLastParameter = *psCurrentByte_;
      ULONG ulArraySize = 1;
      if ((eElementType == CLASSARRAY) || (eElementType == FIXEDARRAY) || (eElementType == VARARRAY))
      {
         if (*psCurrentByte_ >= pcEnd_)
         {
            szMyLastParameter = NULL;
            ulMyParameterNumber++;
            return UNEXPECTED_END_OF_MESSAGE;
         }
         ulMaxArraySize = typeParam->getArrayLength();
         eMessageStatus = ExtractArraySize(eElementType, psCurrentByte_, pcEnd_, &ulArraySize, ulMaxArraySize);
         if (eMessageStatus != GOOD_MESSAGE)
         {
            return(eMessageStatus);
         }
         if (eElementType != FIXEDARRAY)
         {
            EncoderCommon::Memfunc_MemAlign(ppucIOMessageBody_, sizeof(INT));
            INT *pulTemp = (INT*)*ppucIOMessageBody_;
            *pulTemp = ulArraySize;
            pucMyBufferEnd += sizeof(ulArraySize);

            ulLength = typeParam->getLength();
            if (ulLength == 1 && eElementType == VARARRAY || ulLength == 2 && eElementType == VARARRAY)
            {
               *ppucIOMessageBody_ += sizeof(INT);
            }
            else
            {
               *ppucIOMessageBody_ += ulLength;
            }
         }
      }
      else if (eElementType == STRING)
      {
		  ulMyCopiedClassArrayParameterNumber++;
         if (*psCurrentByte_ >= pcEnd_)
         {
            ulMyParameterNumber++;
            szMyLastParameter = NULL;
            return UNEXPECTED_END_OF_MESSAGE;
         }

         ulMaxArraySize = typeParam->getArrayLength();
         eMessageStatus = ExtractStringLength(ulMaxArraySize, *psCurrentByte_, pcEnd_, &ulArraySize);
         if (eMessageStatus != GOOD_MESSAGE)
         {
            return eMessageStatus;
         }
      }
      ULONG ulElementLength = 0;
      const CHAR* sConvertString = "";
      CHAR szStrippedConvert[5];
      szStrippedConvert[0] = '\0';

      ElementTypeEnum eCopyofElementType = eElementType;
      if ((eElementType != CLASSARRAY) && (eElementType != CLASS) && (eElementType != STRING))
      {
         INT iStrippedIndex = 0;
         sConvertString = typeParam->getConversionString();
         while (*sConvertString != 0)
         {
            if ((*sConvertString == '%') || ((*sConvertString >= 'a') && (*sConvertString <= 'z')) ||
               ((*sConvertString >= 'A') && (*sConvertString <= 'Z')))
            {
               szStrippedConvert[iStrippedIndex] = *sConvertString;
               iStrippedIndex++;
            }

            sConvertString++;
         }
         szStrippedConvert[iStrippedIndex] = '\0';

         ulElementLength = typeParam->getLength();
         
      }
      else
      {
         ulElementLength = typeParam->getLength();
         sConvertString = "";
      }
      ++ulMyParameterNumber;
      ULONG ulCopyofParameterNumber = ulMyParameterNumber;
      if ((!strcmp(szStrippedConvert, "%s")) || (eCopyofElementType == STRING))
      {
         bool bEndNullNeeded = true;
         if ((eCopyofElementType == VARARRAY) || (eCopyofElementType == STRING))
         {
            if (ulArraySize > ulMaxArraySize)
            {
               return (INVALID_MESSAGE_FORMAT);
            }
            else if (ulArraySize == ulMaxArraySize)
            {
               bEndNullNeeded = false;
            }
         }
         else
         {
            ULONG ulTempLength = 0;
            eMessageStatus = ExtractStringLength(ulMaxArraySize, *psCurrentByte_, pcEnd_, &ulTempLength);
            if (eMessageStatus != GOOD_MESSAGE)
            {
               ulMyParameterNumber++;
               return(eMessageStatus);
            }

            if (ulTempLength < ulArraySize)
            {
               //Set the memory to 0's to make sure we're padded properly
               memset(*ppucIOMessageBody_, 0, ulArraySize);
               ulArraySize = ulTempLength;
            }
         }
         ULONG ulTempSize = ulMaxArraySize;
         UCHAR* pucTempEnd = *ppucIOMessageBody_;
         eMessageStatus = StringParametertoBinary(ulArraySize, psCurrentByte_, pcEnd_, *ppucIOMessageBody_, bEndNullNeeded);
		 ulMyCopiedClassArrayParameterNumber++;
         INT  iReminder = 0;
         iReminder = ulArraySize % 4;

         if (eMessageStatus != GOOD_MESSAGE)
         {
            return eMessageStatus;
         }
         if (eCopyofElementType == FIXEDARRAY)
         {
            //Fixed array of chars...then we need to move the buffer end to the max size
            if (pucMyBufferEnd < (pucTempEnd + ulTempSize + 1))
               pucMyBufferEnd = pucTempEnd + ulTempSize + 1;
            *ppucIOMessageBody_ += ulMaxArraySize;
         }
         else
         {
            ulArraySize += 4 - iReminder;
            *ppucIOMessageBody_ += ulArraySize;
         }
      }
      else if (strchr(szStrippedConvert, 'Z') != NULL)
      {
         //Check to see if we're past the end!
         if (*psCurrentByte_ >= pcEnd_)
         {
            szMyLastParameter = NULL;
            return UNEXPECTED_END_OF_MESSAGE;
         }

         eMessageStatus = HexParametertoBinary(ulArraySize, psCurrentByte_, pcEnd_, *ppucIOMessageBody_);
		 ulMyCopiedClassArrayParameterNumber++;
         if (eMessageStatus != GOOD_MESSAGE)
            return(eMessageStatus);
         *ppucIOMessageBody_ += ulArraySize;
      }
      else if (strchr(szStrippedConvert, 'R') != NULL)
      {
         CHAR* pcRxCRCPtr = strchr(*psCurrentByte_, '*');
         if (pcRxCRCPtr == NULL)
            return UNEXPECTED_END_OF_MESSAGE;

         CHAR* pcMsgCRCPtr = strchr(pcRxCRCPtr + 1, '*');
         if (pcMsgCRCPtr == NULL)
            return UNEXPECTED_END_OF_MESSAGE;

         *pcMsgCRCPtr = '\r';
         bMyFlipCRC = TRUE;
         MsgConvertStatusEnum eStatus = Convert(pBaseData, writeData,/*messgeList,messgeParamList,typesList,enumsList,*/TRUE);
         bMyFlipCRC = FALSE;
         *psCurrentByte_ = pcMsgCRCPtr + 1;
         *pcMsgCRCPtr = '*';
         ulMyParameterNumber = ulMaxParameterNumber + 1;
         return eStatus;
      }
      else if (strchr(szStrippedConvert, 'P') != NULL) //Is it a pass through type?
      {
         MsgConvertStatusEnum eStatus = ASCIItoPassthrough(ulArraySize, psCurrentByte_, pcEnd_, *ppucIOMessageBody_);
         //Move the output pointer along by the max size
         *ppucIOMessageBody_ += ulArraySize;
         pucMyBufferEnd = *ppucIOMessageBody_;
         return eStatus;
      }
      else
      {
         for (ULONG ulArrayIndex = 1; ulArrayIndex <= ulArraySize; ++ulArrayIndex)
         {
            while ((*psCurrentByte_ < pcEnd_) && ((*(*psCurrentByte_) == ' ') || (*(*psCurrentByte_) == '\0') || (*(*psCurrentByte_) == ',') || (*(*psCurrentByte_) == '\r')))
            {
               (*psCurrentByte_)++;
            }
            if ((eElementType != CLASS) && (*psCurrentByte_ >= pcEnd_))
            {
               szMyLastParameter = NULL;
               return UNEXPECTED_END_OF_MESSAGE;
            }
            szMyLastParameter = *psCurrentByte_;
            
            if (eElementType == CLASS)
            {
               ulMyParameterNumber = ulCopyofParameterNumber;
			   
			   ulMyCopiedClassArrayParameterNumber++;
			   
               eMessageStatus = ASCIIParameterToBinary(ulMessageId_, psCurrentByte_, pcEnd_, ppucIOMessageBody_, messgeList, messgeParamList, typesList, enumsList);
               
            }
            else if (eElementType == CLASSARRAY)
            {
               ulMyParameterNumber = ulCopyofParameterNumber;
			   if (ulArrayIndex == 1)
			   {
				   ulMyCopiedClassArrayParameterNumber = 0;
				   ulMyTotalClassArrayParameterNumber = ulArraySize * msgParm->getChildParamValue();
			   }  
               eMessageStatus = ASCIIParameterToBinary(ulMessageId_, psCurrentByte_, pcEnd_, ppucIOMessageBody_, messgeList, messgeParamList, typesList, enumsList);
               if (eMessageStatus == GOOD_MESSAGE)
               {
                  if (ulMyParameterNumber < ulMaxParameterNumber)
                  {
                     ULONG ulTempParam = ulMyParameterNumber;
                     if (ulTempParam >= messgeParamList.size())
                        return eMessageStatus;
                  }
                  else
                  {
                  }
               }
               else
               {
               }
               
            }
            else if (eElementType == ENUM)
            {
               if (*psCurrentByte_ >= pcEnd_)
               {
                  szMyLastParameter = NULL;
                  return UNEXPECTED_END_OF_MESSAGE;
               }
               eMessageStatus = EnumParameterToBinary(psCurrentByte_, pcEnd_);
               INT ulEnumValue = enumParam->getEnumValueByName(strEnumName);
               EncoderCommon::Memfunc_MemAlign(ppucIOMessageBody_, sizeof(INT));
               INT* pulTemp = (INT*)*ppucIOMessageBody_;
               *pulTemp = ulEnumValue;
               *psCurrentByte_ = sNextSeparator;
               if (pucMyBufferEnd < (*ppucIOMessageBody_ + ulElementLength))
                  pucMyBufferEnd = (*ppucIOMessageBody_ + ulElementLength);

               *ppucIOMessageBody_ += ulLength;
			   ulMyCopiedClassArrayParameterNumber++;
              
            }
            else
            {
				ulMyCopiedClassArrayParameterNumber++;
				
               eMessageStatus = SimpleParameterToBinary(ulElementLength, &(szStrippedConvert[0]), psCurrentByte_, pcEnd_, ppucIOMessageBody_, Typename);
               if (!bLength)
                  *ppucIOMessageBody_ += ulLength;
              
            }
         }
         if (eMessageStatus != GOOD_MESSAGE)
            return eMessageStatus;
         
         if (eCopyofElementType == CLASSARRAY)
         {
            if (ulArraySize == 0)
            {
               ulMyParameterNumber = ulMaxParameterNumber;
            }
         }
      }
      ulCopyParam = ulMyParameterNumber;
      if (ulSize == ulMyParameterNumber)
      {
         ulCopyParam = ulMyParameterNumber - 1;
      }
      if (ulMyParameterNumber < ulMaxParameterNumber)
      {
         const CMessageParams* msgParamNext = messgeParamList.at(ulCopyParam);
         ulBlockValue1 = msgParamNext->getBlockValue();
		 
      }
	  if (ulMyCopiedClassArrayParameterNumber == ulMyTotalClassArrayParameterNumber)
	  {
		  break;
	  }

   } while ((ulMyParameterNumber < ulMaxParameterNumber) &&
      (*psCurrentByte_ < pcEnd_) && (*psCurrentByte_ != NULL));
   return eMessageStatus;
}

MsgConvertStatusEnum AsciiToBinaryComposer::ExtractArraySize(ElementTypeEnum eElementType_, CHAR** psCurrentByte_, const CHAR* pcEnd_, ULONG* pulArraySize, ULONG ulMaxArraySize_)
{
   ULONG ulMaxArraySize = ulMaxArraySize_;
   if (eElementType_ == FIXEDARRAY)
   {
      *pulArraySize = ulMaxArraySize;
   }

   else
   {
      CHAR*  sNextSeparator = *psCurrentByte_;

     
      if (clMySeparator.FindNext(eMyFormat, psCurrentByte_, &sNextSeparator, pcEnd_) == PAST_END_OF_BUFFER)
      {
         if (*psCurrentByte_ >= pcEnd_) //no data left
         {
            szMyLastParameter = NULL;
            return (UNEXPECTED_END_OF_MESSAGE);
         }
         sNextSeparator = (CHAR*)pcEnd_; //last field so set things to exit
        
      }
      
      CHAR cDummy;
      if (sscanf(*psCurrentByte_, "%lu%c", pulArraySize, &cDummy) != 1)
         return (INVALID_MESSAGE_FORMAT);

      if (*pulArraySize > ulMaxArraySize)
         return (INVALID_MESSAGE_FORMAT);

      *psCurrentByte_ = sNextSeparator;
   }
   return(GOOD_MESSAGE);
}

MsgConvertStatusEnum AsciiToBinaryComposer::ExtractStringLength(ULONG ulMaxSize_, CHAR* pcStart_, const CHAR* pcEnd_, ULONG* ulArraySize_)
{
   while ((pcStart_ <= pcEnd_) && (((*pcStart_) == ' ') || ((*pcStart_) == ',')))
   {
      pcStart_++;
   }
   CHAR* sSearchPointer = pcStart_;
   ULONG ulLength = 0;

   if (*pcStart_ == '"')
   {
      //We want to look for the next quote. If we run out space or find a CR/LF then it's an error
      BOOL bQuoteFound = FALSE;
      pcStart_++;

      while ((!bQuoteFound) && (pcStart_ <= pcEnd_))
      {
        
         if (*pcStart_ == '"')
         {
            bQuoteFound = true;
         }
         else if ((*pcStart_ == CR) || (*pcStart_ == LF))
         {
            return INVALID_MESSAGE_FORMAT;
         }
         else
         {
            ulLength++;
            pcStart_++;
         }
         
      }

      if (!bQuoteFound)
      {
         szMyLastParameter = NULL;
         return UNEXPECTED_END_OF_MESSAGE;
      }
   }
   else
   {
      BOOL bSeparatorFound = false;

      //if we're already on a separator move past it
      while ((pcStart_ <= pcEnd_) && ((*pcStart_ == CR) || (*pcStart_ == LF) || (*pcStart_ == ',') || (*pcStart_ == ' ') || (*pcStart_ == '\0')))
      {
         pcStart_++;
      }

      while ((pcStart_ <= pcEnd_) && (!bSeparatorFound))
      {
         
         if (*pcStart_ == '"')
         {
            return INVALID_MESSAGE_FORMAT;
         }
         else if ((*pcStart_ == CR) || (*pcStart_ == LF) || (*pcStart_ == ',') || (*pcStart_ == '*') || (*pcStart_ == '\0'))
         {
            bSeparatorFound = true;
           
         }
         else
         {
            ulLength++;
            pcStart_++;
           
         }
        
      }

      if (!bSeparatorFound)
      {
         szMyLastParameter = NULL;
         return UNEXPECTED_END_OF_MESSAGE;
      }
   }
   //If you length is 0 or it excedes the maximum size then we have a problem.
   //Similarly the length + where we started reading should not extend past the end
   if ((ulLength > ulMaxSize_) || (sSearchPointer + ulLength > pcEnd_))
   {
      return INVALID_MESSAGE_FORMAT;
   }
   *ulArraySize_ = ulLength;

   return GOOD_MESSAGE;
}

MsgConvertStatusEnum AsciiToBinaryComposer::StringParametertoBinary(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_, BOOL bNullEnding_)
{
   while ((*psCurrentByte_ <= pcEnd_) && ((*(*psCurrentByte_) == ' ') || (*(*psCurrentByte_) == ',')))
   {
      (*psCurrentByte_)++;
   }

   BOOL bQuoteFound = FALSE;
   if ((*psCurrentByte_) + ulArraySize_ > pcEnd_)
   {
      szMyLastParameter = NULL;
      return (UNEXPECTED_END_OF_MESSAGE);
   }

   //std::string expectedString;

   if (*(*psCurrentByte_) == '"')
   {
      bQuoteFound = true;
      (*psCurrentByte_)++;
      //expectedString = *(psCurrentByte_);
   }

   if (bQuoteFound && (*(*psCurrentByte_ + ulArraySize_)) != '"')
   {
      ulArraySize_--;
   }
   memcpy(pucIOMessageBody_, *psCurrentByte_, ulArraySize_);

   if (bNullEnding_)
   {
      pucIOMessageBody_[ulArraySize_] = 0;
   }

   (*psCurrentByte_) += ulArraySize_;

   if (bQuoteFound)
   {
      if (*(*psCurrentByte_) != '"')
      {
         return (INVALID_MESSAGE_FORMAT);
      }
   }
   else
   {
      CHAR cTemp = *(*psCurrentByte_);
      if ((cTemp != ' ') && (cTemp != ',') && (cTemp != '*') && (cTemp != '\0') && (cTemp != '\r'))
      {
         return (INVALID_MESSAGE_FORMAT);
      }
   }

   if (bQuoteFound)
   {
      (*psCurrentByte_) += 2;
   }

   if (pucMyBufferEnd < (pucIOMessageBody_ + ulArraySize_ + 1))
      pucMyBufferEnd = pucIOMessageBody_ + ulArraySize_ + 1;

   return (GOOD_MESSAGE);
}

MsgConvertStatusEnum AsciiToBinaryComposer::HexParametertoBinary(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_)
{
   CHAR* sNextSeparator = *psCurrentByte_;
   MsgConvertStatusEnum eMessageStatus = GOOD_MESSAGE;
     SeparatorEnum sEnum = clMySeparator.FindNext(eMyFormat, psCurrentByte_, &sNextSeparator, pcEnd_);
   if (sEnum == PAST_END_OF_BUFFER)
   {
      if (*psCurrentByte_ >= pcEnd_) //no data left
      {
         szMyLastParameter = NULL;
         return (UNEXPECTED_END_OF_MESSAGE);
      }
      sNextSeparator = (char*)pcEnd_; //last field so set things to exit
     
   }
   else if (sEnum == HEADER_TERMINATOR_FOUND)
   {
      return INVALID_MESSAGE_FORMAT;
   }
   unsigned int uiResult = 0;
   if (strlen(*psCurrentByte_) != (ulArraySize_ * 2))
      return (INVALID_MESSAGE_FORMAT);
   for (ULONG ulArrayIndex = 1; ulArrayIndex <= ulArraySize_; ++ulArrayIndex)
   {
      if (!isxdigit(**psCurrentByte_) || !isxdigit(*(*psCurrentByte_ + 1)))
         return(INVALID_MESSAGE_FORMAT);

      sscanf(*psCurrentByte_, "%02x", &uiResult);
      pucIOMessageBody_[ulArrayIndex - 1] = (UCHAR)uiResult;
      *psCurrentByte_ += 2;
   }

   *psCurrentByte_ = sNextSeparator;
   if (pucMyBufferEnd < (pucIOMessageBody_ + (ulArraySize_ * 2)))
      pucMyBufferEnd = pucIOMessageBody_ + (ulArraySize_ * 2);

   return (eMessageStatus);
}

MsgConvertStatusEnum AsciiToBinaryComposer::ASCIItoPassthrough(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_)
{
   for (ULONG ulArrayIndex = 1; ulArrayIndex <= ulArraySize_; ++ulArrayIndex)
   {
      CHAR* pcData = *psCurrentByte_;
      CHAR cStoreData;

      if (pcData > pcEnd_)
         return UNEXPECTED_END_OF_MESSAGE;

      //Check to see if this is a '\'. If so then we're dealing with a \xHH hex value
      if (*pcData == '\\')
      {
         if (*(pcData + 1) != '\\')
         {
            pcData += 2;
            // Read off the next 2 characters as a hex value
            unsigned int uiResult = 0;
            int iNumBytes = sscanf(pcData, "%02x", &uiResult);

            if (iNumBytes != 1)
               return INVALID_MESSAGE_FORMAT;

            cStoreData = (char)(uiResult & 0xFF);
            pcData += 2; //skip past HH)
         }
         else
         {
            cStoreData = '\\';
            pcData += 2;  // skip over the double slashes
         }
      }
      else
      {
         cStoreData = *pcData;
         pcData++;
      }

      *psCurrentByte_ = pcData;
      (*pucIOMessageBody_) = (UCHAR)cStoreData;
      pucIOMessageBody_++;
   }

   return (GOOD_MESSAGE);
}

MsgConvertStatusEnum AsciiToBinaryComposer::EnumParameterToBinary(CHAR** psCurrentByte_, const CHAR* pcEnd_)
{
   sNextSeparator = *psCurrentByte_;
   strEnumName.clear();
   
   SeparatorEnum sEnum = clMySeparator.FindNext(eMyFormat, psCurrentByte_, &sNextSeparator, pcEnd_);
   if (sEnum == PAST_END_OF_BUFFER)
   {
      //if this enum is not optional then return end of message
      if ((*psCurrentByte_ != NULL) && ((*(*psCurrentByte_) == ' ') || (*(*psCurrentByte_) == ',') || (*(*psCurrentByte_) == '\0')))
      {
         szMyLastParameter = NULL;
         return (UNEXPECTED_END_OF_MESSAGE);
      }
      else
      {
         sNextSeparator = (CHAR*)pcEnd_;
      }
   }
   else if (sEnum == HEADER_TERMINATOR_FOUND)
   {
      return (INVALID_MESSAGE_FORMAT);
   }
   else
   {

   }
   CHAR szBuffer[MAX_ENUM_VALUE_LENGTH];
   if (strlen(*psCurrentByte_) >= MAX_ENUM_VALUE_LENGTH)
      return INVALID_MESSAGE_FORMAT;

   sprintf(szBuffer, "%s", *psCurrentByte_);
   strEnumName = szBuffer;
   return (GOOD_MESSAGE);
}

MsgConvertStatusEnum AsciiToBinaryComposer::SimpleParameterToBinary(ULONG ulElementLength_, const CHAR* sConvertString_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR** ppucIOMessageBody_, std::string szTypeName)
{
	CHAR* sNextSeparator = *psCurrentByte_;
	bLength = FALSE;

	SeparatorEnum sEnum = clMySeparator.FindNext(eMyFormat, psCurrentByte_, &sNextSeparator, pcEnd_);
	if (sEnum == PAST_END_OF_BUFFER)
	{
		if ((*psCurrentByte_ != NULL) && ((*(*psCurrentByte_) == ' ') || (*(*psCurrentByte_) == ',') || (*(*psCurrentByte_) == '\0')))
		{
			szMyLastParameter = NULL;
			return (UNEXPECTED_END_OF_MESSAGE);
		}
		else
		{
			sNextSeparator = (CHAR*)pcEnd_;
		}
		
	}
	else if (sEnum == HEADER_TERMINATOR_FOUND)
	{
		return (INVALID_MESSAGE_FORMAT);
	}

	EncoderCommon::Memfunc_MemAlign(ppucIOMessageBody_, ulElementLength_);
	if (!strcmp(sConvertString_, "%m"))
	{
		ULONG* pulTemp = (ULONG*)*ppucIOMessageBody_;
		if (GetMessageId((*psCurrentByte_), pulTemp) != GOOD_MESSAGE)
			return(INVALID_MESSAGE_FORMAT);
	}
	else if (!strcmp(sConvertString_, "%M"))
	{
		if (GetMessageId((*psCurrentByte_), (ULONG*)(*ppucIOMessageBody_)) == INVALID_MESSAGE_ID)
			return(INVALID_MESSAGE_FORMAT);
	}
	else if (!strcmp(sConvertString_, "%T")) //IS it a GPSTime msec field?
	{
		double dSecTime = 0;
		char cDummy = '\0';
		if (sscanf(*psCurrentByte_, "%lf%c", &dSecTime, &cDummy) != 1)
			return (INVALID_MESSAGE_FORMAT);

		//Put it in the IO struct as an ULONG msecs
		ULONG ulMsecTime = (ULONG)(dSecTime * SEC_TO_MSEC);
		INT* pulTemp = (INT*)*ppucIOMessageBody_;
		*pulTemp = ulMsecTime;
	}
	else if (!strcmp(sConvertString_, "%B")) //IS it a char field that we want to read in as an int?
	{
		INT dValueRead = 0;
		CHAR cDummy = '\0';
		if (sscanf(*psCurrentByte_, "%d%c", &dValueRead, &cDummy) != 1)
			return (INVALID_MESSAGE_FORMAT);

		//Put it in the IO struct as a char
		*(*ppucIOMessageBody_) = (CHAR)dValueRead;
	}
	else if (!strcmp(sConvertString_, "%UB")) //IS it a char field that we want to read in as an unsigned int?
	{
		UINT dValueRead = 0;
		CHAR cDummy = '\0';
		if (sscanf(*psCurrentByte_, "%u%c", &dValueRead, &cDummy) != 1)
			return (INVALID_MESSAGE_FORMAT);

		//Put it in the IO struct as an unsigned char
		*(*ppucIOMessageBody_) = (UCHAR)dValueRead;
	}
	else if (!strcmp(sConvertString_, "%XB")) //IS it a char field that we want to read in hex byte?
	{
		unsigned int dValueRead = 0;
		CHAR cDummy = '\0';
		if (sscanf(*psCurrentByte_, "%02x%c", &dValueRead, &cDummy) != 1)
			return (INVALID_MESSAGE_FORMAT);

		*(*ppucIOMessageBody_) = (CHAR)dValueRead;
	}
	else if (!strcmp(sConvertString_, "%k")) //IS it a Super Floating Point
	{
		//Read the ascii in as a float
		FLOAT fValueRead = 0.0;
		CHAR cDummy = '\0';
		sscanf(*psCurrentByte_, "%f%c", &fValueRead, &cDummy); // Changed for reading NaN values for BeiDou Doppler out of range values.
		//Put it in the IO struct as a float
		FLOAT* pfTemp = (FLOAT*)*ppucIOMessageBody_;
		*pfTemp = fValueRead;
	}
	else if (!strcmp(sConvertString_, "%f"))
	{
		CHAR cDummy = '\0';
      if (szTypeName == "DOUBLE")
		{
			DOUBLE dFloatVal = 0.0;
			if (sscanf(*psCurrentByte_, "%lf%c", &dFloatVal, &cDummy) != 1)
				return (INVALID_MESSAGE_FORMAT);
			DOUBLE* pdTemp = (DOUBLE*)*ppucIOMessageBody_;
			*pdTemp = dFloatVal;
		}
		else
		{
			FLOAT dFloatVal = 0.0;
			if (sscanf(*psCurrentByte_, "%f%c", &dFloatVal, &cDummy) != 1)
				return (INVALID_MESSAGE_FORMAT);
			FLOAT* pdTemp = (FLOAT*)*ppucIOMessageBody_;
			*pdTemp = dFloatVal;
		}
	}
	else if (!strcmp(sConvertString_, "%lk")) //IS it a Super Floating Point
	{
		DOUBLE dValueRead = 0.0;
		CHAR cDummy = '\0';
		if (sscanf(*psCurrentByte_, "%lf%c", &dValueRead, &cDummy) != 1)
			return (INVALID_MESSAGE_FORMAT);
		//Put it in the IO struct as a float
		DOUBLE* pdTemp = (DOUBLE*)*ppucIOMessageBody_;
		*pdTemp = dValueRead;
	}
	else if ( szTypeName == "BOOL") //IS it a BOOL?
	{
		BOOL bValueRead = FALSE;
		if (!strcmp(*psCurrentByte_, "TRUE"))
		{
			bValueRead = TRUE;
		}
		else if (strcmp(*psCurrentByte_, "FALSE"))
		{
			//okay it's not true or false. Something is wrong here
			return INVALID_MESSAGE_FORMAT;
		}

		BOOL* pbTemp = (BOOL*)*ppucIOMessageBody_;
		*pbTemp = bValueRead;
	}
	/*else if (ulClassId == 1568 && ulElementId == 25 && iMessageId == 1309)
	{
		CHAR acBuffer[10];
		CHAR* sModifiedConvertString = strcpy(&acBuffer[0], "%u");
		CHAR cDummy;
		sModifiedConvertString = strcat(sModifiedConvertString, "%c");
		if (sscanf(*psCurrentByte_, sModifiedConvertString, *ppucIOMessageBody_, &cDummy) != 1)
			return (INVALID_MESSAGE_FORMAT);
	}*/
	else
	{
		CHAR acBuffer[10];
		CHAR* sModifiedConvertString = strcpy(&acBuffer[0], sConvertString_);
		CHAR cDummy;
		sModifiedConvertString = strcat(sModifiedConvertString, "%c");
		if (szTypeName == "SATELLITEID" && strEnumName == "GLONASS")
		{
			bLength = true;
			std::string str = *psCurrentByte_;
			if ((str.compare("-") == 0) || (str.compare("+") == 0))
			{
				INT iIndex = 0;
				std::string strSub;
				std::string strSub1;
				iIndex = static_cast<INT>(str.find("-"));
				if (iIndex == -1)
				{
					iIndex = static_cast<INT>(str.find("+"));
				}
				if (iIndex == -1)
				{
					strSub = str;
					strSub1 = "0";
				}
				else
				{
					strSub = str.substr(0, iIndex);
					strSub1 = str.substr(iIndex, str.size());
				}
				if (sscanf(strSub.c_str(), sModifiedConvertString, *ppucIOMessageBody_, &cDummy) != 1)
					return (INVALID_MESSAGE_FORMAT);
				*ppucIOMessageBody_ += 2;
				if (sscanf(strSub1.c_str(), sModifiedConvertString, *ppucIOMessageBody_, &cDummy) != 1)
					return (INVALID_MESSAGE_FORMAT);
				*ppucIOMessageBody_ += 2;
			}
			else
			{
				if (sscanf(*psCurrentByte_, sModifiedConvertString, *ppucIOMessageBody_, &cDummy) != 1)
					return (INVALID_MESSAGE_FORMAT);
				*ppucIOMessageBody_ += 4;
			}
		}
		else
		{
			if (sscanf(*psCurrentByte_, sModifiedConvertString, *ppucIOMessageBody_, &cDummy) != 1)
				return (INVALID_MESSAGE_FORMAT);
		}
	}

	*psCurrentByte_ = sNextSeparator;
	if (pucMyBufferEnd < (*ppucIOMessageBody_ + ulElementLength_))
		pucMyBufferEnd = (*ppucIOMessageBody_ + ulElementLength_);

	return (GOOD_MESSAGE);
}


UCHAR AsciiToBinaryComposer::setComPort(CommunicationPortEnum ePort_, UINT uiVirtualAddress_)
{
   if (ePort_ > 0x7)
      return (UCHAR)((MSGCVT_COMPORT_UNKNOWN << PORT_POSITION) | uiVirtualAddress_);
   else
      return (UCHAR)((ePort_ << PORT_POSITION) | uiVirtualAddress_);
}
