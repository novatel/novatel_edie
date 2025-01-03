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
//    Class to Start the BINARY to ASCII conversion
//
////////////////////////////////////////////////////////////////////////////////
#include "binarytoasciicomposer.hpp"
#include <math.h>

BinaryToAsciiComposer::BinaryToAsciiComposer() :
   pstBinaryHeader(NULL), dbData(NULL), pstBinaryShortHeader(NULL),
   ulMaxNoOfParameters(0), msgStyle(MessageStyleEnum(-1)),
   ulMyParameterNumber(0), ulMyCopiedClassArrayParameterNumber(0),
   ulMyTotalClassArrayParameterNumber(0),pucMyInputData(NULL),
   bIsBinaryRxconfig(FALSE), pucMyIOStart(NULL),
   bMyFlipCRC(FALSE), cMySeparator(ASCII_FIELD_SEPERATOR),
   iMyMessageLength(0), ulSize(0), pBaseData(NULL),
   usBodyLength(0), ulMaxParameterNumberBin(0),usMsgNumber(0),
   sCurrentByte_(NULL),writeData(NULL),
   uiMsgId(0), ulCopyParam(0),
   MessageStatus(MsgConvertStatusEnum(-1))
{
   dbData = loadDataFromJSON::getDatabaseObj();
#ifdef _WIN32
#if (_MSC_VER < 1900)
   _set_output_format(_TWO_DIGIT_EXPONENT);
#endif // (_MSC_VER < 1900)
#endif	
   listOfShortHeaderLogs.push_back(813);
   listOfShortHeaderLogs.push_back(1362);
   listOfShortHeaderLogs.push_back(1305);
   listOfShortHeaderLogs.push_back(319);
   listOfShortHeaderLogs.push_back(320);
   listOfShortHeaderLogs.push_back(2052);
   listOfShortHeaderLogs.push_back(321);
   listOfShortHeaderLogs.push_back(508);
   listOfShortHeaderLogs.push_back(323);
   listOfShortHeaderLogs.push_back(324);
   listOfShortHeaderLogs.push_back(325);
   listOfShortHeaderLogs.push_back(1462);
   listOfShortHeaderLogs.push_back(622);
   listOfShortHeaderLogs.push_back(2264);
}

BinaryToAsciiComposer::~BinaryToAsciiComposer()
{

}

//Function to start conversion from BINARY to ASCII
MsgConvertStatusEnum BinaryToAsciiComposer::Convert(BaseMessageData *pclBaseData, CHAR* writeData_)
{
   //Conversion type is ASCII and got ASCII message 
   //Write the data to output file as it is available in Base message data.
   if ((pclBaseData->getMessageFormat() == MESSAGE_ASCII || pclBaseData->getMessageFormat() == MESSAGE_UNKNOWN) && (!bIsBinaryRxconfig))
   {
      return GOOD_MESSAGE;
   }
   pBaseData = pclBaseData;
   stringstream strMsgDefDrc;
   std::string MsgName = "";
   usMsgNumber = 0;
   usBodyLength = 0;
   if (bIsBinaryRxconfig)
   {
      pucMyIOStart = pucMyInputData + 28;
      bIsBinaryRxconfig = FALSE;
      writeData = writeData_;
      sCurrentByte_ = writeData;
      MessageStatus = FormatHeader(pucMyInputData, &sCurrentByte_);
      pucMyInputData += 28;
      strMsgDefDrc << dbData->getMsgDefCRC(pstBinaryHeader->usMsgNumber);
	  MsgName = GetMessageNameByID(pstBinaryHeader->usMsgNumber) + "_" + std::to_string(pstBinaryHeader->usMsgNumber) + "_" + strMsgDefDrc.str().c_str();;
      usMsgNumber = pstBinaryHeader->usMsgNumber;
      usBodyLength = pstBinaryHeader->usLength;
   }
   else
   {
      writeData_ = new CHAR[MAX_ASCII_BUFFER_SIZE];
      memset(writeData_, 0, MAX_ASCII_BUFFER_SIZE);
      writeData = writeData_;
      pucMyInputData = (UCHAR*)pclBaseData->getMessageData();
      pclBaseData->setMessageFormat(MESSAGE_ASCII);
      sCurrentByte_ = writeData;
      MessageStatus = FormatHeader(pucMyInputData, &sCurrentByte_);
      //Framing unique for each message with MsgName_MsgId_MsgDefCrc	
      if (std::find(listOfShortHeaderLogs.begin(), listOfShortHeaderLogs.end(), pstBinaryHeader->usMsgNumber) != listOfShortHeaderLogs.end())
      {
         strMsgDefDrc << dbData->getMsgDefCRC(pstBinaryShortHeader->usMessageId);
         pucMyIOStart = pucMyInputData + 12;
         pucMyInputData += 12;
         MsgName = GetMessageNameByID(pstBinaryShortHeader->usMessageId) + "_" + std::to_string(pstBinaryShortHeader->usMessageId);// +"_" + strMsgDefDrc.str().c_str();
         usMsgNumber = pstBinaryShortHeader->usMessageId;
         usBodyLength = pstBinaryShortHeader->ucLength;
      }
      else
      {
         pucMyIOStart = pucMyInputData + 28;
         pucMyInputData += pstBinaryHeader->ucHeaderLength;
         USHORT usMsgDefCrc = pstBinaryHeader->usMsgDefCRC;
         if (strMsgDefDrc.str().empty())
            strMsgDefDrc << std::setfill('0') << setw(4) << std::hex << usMsgDefCrc;
		 MsgName = GetMessageNameByID(pstBinaryHeader->usMsgNumber) + "_" + std::to_string(pstBinaryHeader->usMsgNumber); //+ "_" + strMsgDefDrc.str().c_str();;
         usMsgNumber = pstBinaryHeader->usMsgNumber;
         usBodyLength = pstBinaryHeader->usLength;
      }
   }

   if (MessageStatus != GOOD_MESSAGE)
   {
      pclBaseData->setMessageData(writeData_);
      return MessageStatus;
   }

   const vector<CMessageParams*> messgeParamList = dbData->getMessageParamObj(MsgName);
   const vector<CTypes*> typesList = dbData->getTypesObj(MsgName);
   const vector<CEnums*> enumsList = dbData->getEnumObj(MsgName);
   const vector<CMessage*> messgeList = dbData->getMessageObj(MsgName);

   if ((INT)messgeList.size() > 0)
      ulMaxNoOfParameters = messgeList.at(0)->getNoOfParamasById(usMsgNumber);
   if (ulMaxNoOfParameters == 0)
   {
      pclBaseData->setMessageData(writeData_);
      return INVALID_MESSAGE_ID;
   }

   CHAR cMsgType = (CHAR)((usMsgNumber >> 15 >> 1) & 0xff);

   if (EncoderCommon::isResponse(cMsgType))
   {
      memcpy(sCurrentByte_, pucMyInputData, usBodyLength);
      sCurrentByte_ = sCurrentByte_ + usBodyLength + 1;
   }
   else
   {
      ulMyParameterNumber = 0;

      if (messgeList.empty() || messgeParamList.empty() || typesList.empty() || enumsList.empty())
      {
         pclBaseData->setMessageData(writeData_);
         return INVALID_MESSAGE_ID;
      }

      ulSize = static_cast<ULONG>(messgeParamList.size());

      while ((ulMyParameterNumber < ulMaxNoOfParameters) && (pucMyIOStart + usBodyLength > pucMyInputData))
      {
         if (ulMyParameterNumber >= ulSize || ulMyParameterNumber >= typesList.size() || ulMyParameterNumber >= enumsList.size())
         {
            pclBaseData->setMessageData(writeData_);
            return GOOD_MESSAGE;
         }

         MessageStatus = BinaryParameterToASCII(usMsgNumber, &sCurrentByte_, messgeList, messgeParamList, typesList, enumsList);

         if (MessageStatus != GOOD_MESSAGE)
         {
            pclBaseData->setMessageData(writeData_);
            return (INVALID_MESSAGE_FORMAT);
         }
      }
      --sCurrentByte_;
   }
   iMyMessageLength = static_cast<INT>(sCurrentByte_ - writeData_);
   *sCurrentByte_ = BODY_CRC_SEPARATOR;

   CRC32 clCrc;
   ULONG ulCRC = clCrc.CalculateBlockCRC32(iMyMessageLength - 1, 0, (UCHAR*)(writeData_ + 1));

   if (bMyFlipCRC)
      ulCRC = ulCRC ^ 0xFFFFFFFF;

   if (sprintf(++sCurrentByte_, "%08lx", ulCRC) != ASCII_CRC_SIZE)
   {
      pclBaseData->setMessageData(writeData_);
      return (INVALID_CRC);
   }

   sCurrentByte_ += ASCII_CRC_SIZE;
   iMyMessageLength += ASCII_CRC_SIZE + 1;

   if (!bMyFlipCRC)
   {
      *sCurrentByte_ = CR;
      *(sCurrentByte_ + 1) = LF;
      iMyMessageLength += 2;
   }
   pclBaseData->setMessageLength(iMyMessageLength);
   pclBaseData->setMessageData(writeData_);
   return MessageStatus;
}

//Function to send the data to appropriate internal functions
MsgConvertStatusEnum BinaryToAsciiComposer::BinaryParameterToASCII(const USHORT usMessageId_, CHAR** ppcCurrentByte_, const std::vector<CMessage*>& messgeList, const std::vector<CMessageParams*>& messgeParamList, const std::vector<CTypes*>& typesList, const std::vector<CEnums*>& enumsList)
{
   MessageStatus = GOOD_MESSAGE;
   ulMaxParameterNumberBin = messgeList.at(0)->getNoOfParamasById(usMessageId_);
   ElementTypeEnum eElementType = SIMPLE;
   BOOL bIndexOf = false;
   BOOL bDoingVarArray = false;
   std::string Typename = "";
   std::string Elementname = "";

   do
   {
      if (messgeParamList.empty() || typesList.empty() || enumsList.empty())
         return INVALID_MESSAGE_ID;

      if (ulMyParameterNumber >= ulSize || ulMyParameterNumber >= typesList.size() || ulMyParameterNumber >= enumsList.size())
         return GOOD_MESSAGE;

      const CMessageParams* msgParm = messgeParamList.at(ulMyParameterNumber);
      const CTypes* msgTypes = typesList.at(ulMyParameterNumber);
      const CEnums* msgEnums = enumsList.at(ulMyParameterNumber);

      eElementType = msgTypes->getStorageTypeEnum();

      CHAR *strConString = msgTypes->getConversionString();
	  if (msgTypes->getBaseTypeName() == "")
	  {
		  Typename = msgTypes->getTypeName();
	  }
	  else
	  {
		  Typename = msgTypes->getBaseTypeName();
	  }
      if ((strConString != NULL) && (!strcmp(strConString, "%R")))
      {
         bMyFlipCRC = TRUE;
         bIsBinaryRxconfig = TRUE;
         MessageStatus = Convert(pBaseData, *ppcCurrentByte_);
         ulMyParameterNumber = ulMaxNoOfParameters + 1;
         bMyFlipCRC = FALSE;
         **ppcCurrentByte_ = cMySeparator;
         ++*ppcCurrentByte_;
         return MessageStatus;
      }
      const ULONG& ulArrayLength = msgTypes->getArrayLength();
      ULONG ulArraySize = 1;
      ULONG ulElementLength = 0;
      ulElementLength = msgTypes->getLength();

      if (eElementType == CLASSARRAY || eElementType == VARARRAY)
      {
         ulArraySize = *(UINT*)(pucMyInputData);

         if (ulArraySize > ulArrayLength)
            return INVALID_MESSAGE_FORMAT;
         *ppcCurrentByte_ += sprintf(*ppcCurrentByte_, "%lu%c", ulArraySize, cMySeparator);
         pucMyInputData += 4;
         if (usMessageId_ == 1968 && Elementname.find("ausMySamples") != string::npos && eElementType == VARARRAY)
         {
            ulElementLength = 2;
         }
      }
      else if (eElementType != CLASS)
      {
         EncoderCommon::Memfunc_MemAlign(&pucMyInputData, ulElementLength);
      }
      if (eElementType == FIXEDARRAY)
         ulArraySize = ulArrayLength;

      ++ulMyParameterNumber;
      ULONG ulCopyofParameterNumber = ulMyParameterNumber;
      bool bStringType = false;
      if (strConString != NULL || eElementType == STRING)
      {
         if ((!strcmp(strConString, "%s")) || (eElementType == STRING))
         {
            ULONG ulMaxLength = ulArrayLength;
			ulMyCopiedClassArrayParameterNumber++;
            MessageStatus = StringParametertoASCII(ulMaxLength, ppcCurrentByte_, eElementType);
            bStringType = TRUE;

            if (MessageStatus != GOOD_MESSAGE)
               return INVALID_MESSAGE_FORMAT;
         }
         else if (strchr(strConString, 'Z') != NULL)
         {
            //Save the start of this string
            UCHAR* pucStringStart = pucMyInputData;
			ulMyCopiedClassArrayParameterNumber++;
            MessageStatus = HexParametertoASCII(ulArraySize, ppcCurrentByte_);
            bStringType = true;
            if (MessageStatus != GOOD_MESSAGE)
               return MessageStatus;

            //Figure out the length that the input has moved over and if not the
            //max length then move the input pointer over more
            ULONG ulStringLength = static_cast<ULONG>(pucMyInputData - pucStringStart);
            ULONG ulMaxLength = ulArrayLength;

            if (ulStringLength > ulMaxLength)
               pucMyInputData += ulMaxLength - ulStringLength;
         }
         else if (strchr(strConString, 'P') != NULL)
         {
            //Save the start of this string
            UCHAR* pucStringStart = pucMyInputData;
            MessageStatus = PassThroughtoASCII(ulArraySize, ppcCurrentByte_);
            bStringType = TRUE;
            if (MessageStatus != GOOD_MESSAGE)
               return MessageStatus;

            //Figure out the length that the input has moved over and if not the
            //max length then move the input pointer over more
            ULONG ulStringLength = static_cast<ULONG>(pucMyInputData - pucStringStart);
            ULONG ulMaxLength = ulArrayLength;

            if (ulStringLength > ulMaxLength)
               pucMyInputData += ulMaxLength - ulStringLength;
         }
      }
      if ((bIndexOf == TRUE) && ((eElementType == CLASSARRAY) || (eElementType == VARARRAY)))
      {
         bIndexOf = FALSE;
         ulMyParameterNumber++;
         continue; //only want the index... not the parameters underneath!
      }
      if (!bStringType)
      {
         // Extract all the items for this element
         for (ULONG ulArrayIndex = 1; ulArrayIndex <= ulArraySize; ++ulArrayIndex)
         {
            // For a class/class array, call ASCIIParameterToBinary (recursion)
            // We must reset the parameter number for each class in an array
            if ((eElementType == CLASS) || (eElementType == CLASSARRAY))
            {
               //we should increment the input pointer the size of a class each interation through
               //if we're dealing with a class array
               if (eElementType == CLASS)
               {
                  ulMyCopiedClassArrayParameterNumber++;
               }
				if (ulArrayIndex == 1 && eElementType == CLASSARRAY)
				{
					ulMyCopiedClassArrayParameterNumber = 0;
					ulMyTotalClassArrayParameterNumber = ulArraySize * msgParm->getChildParamValue();
				}
               ulMyParameterNumber = ulCopyofParameterNumber;

               MessageStatus = BinaryParameterToASCII(usMessageId_, ppcCurrentByte_, messgeList, messgeParamList, typesList, enumsList);

            }
            else if (eElementType == ENUM)
            {
			   ulMyCopiedClassArrayParameterNumber++;
               CHAR enumName[100];
               memset(enumName, 0, 100);
               sprintf(enumName, "%d", *(INT*)pucMyInputData);
               std::string str(enumName);
               UINT iEnumVal = 0;
               iEnumVal = std::stoi(str);
               std::string strEnum = msgEnums->getEnumNameByValue(iEnumVal);
               /*if (uiMsgId == 2129 && INSSEEDSTATUS_OEM6_Enabled && ulCurrentClassId == 2544 && ulTypeId == 816)
               {
                  strEnum = (iEnumVal == 1) ? "TRUE" : "FALSE";
               }*/
               m_strEnum = strEnum;
               if (strEnum.empty())
                  strEnum = "\"\"";

               *ppcCurrentByte_ += strlen(strcpy(*ppcCurrentByte_, strEnum.c_str()));
               **ppcCurrentByte_ = cMySeparator;
               ++*ppcCurrentByte_;
               pucMyInputData += 4;
              
            }
            // The only thing left is a simple or array of simples
            else
            {
				ulMyCopiedClassArrayParameterNumber++;
				MessageStatus = SimpleParameterToASCII(Typename, strConString, ppcCurrentByte_, ulElementLength);
            }
            if (MessageStatus != GOOD_MESSAGE)
               return MessageStatus;
         }
         if ((eElementType == CLASSARRAY) || (eElementType == VARARRAY) || bDoingVarArray)
         {
            if (bDoingVarArray)
               bDoingVarArray = FALSE;
         }
         if (eElementType == CLASSARRAY)
         {
            if (ulArraySize == 0)
            {
               ulMyParameterNumber = ulMaxNoOfParameters;
            }
         }
      }
      ulCopyParam = ulMyParameterNumber;
      if (ulSize == ulMyParameterNumber)
      {
         ulCopyParam = ulMyParameterNumber - 1;
      }
	  if (ulMyCopiedClassArrayParameterNumber == ulMyTotalClassArrayParameterNumber)
	  {
		  break;
	  }
   } while ((ulMyParameterNumber < ulMaxParameterNumberBin) &&
      (pucMyIOStart + usBodyLength > pucMyInputData));

   return MessageStatus;
}

MsgConvertStatusEnum BinaryToAsciiComposer::StringParametertoASCII(ULONG ulMaxLength_, CHAR** psCurrentByte_, ElementTypeEnum eConvertType)
{
   ULONG uLength = (ULONG)strlen((CHAR*)pucMyInputData);

   if (uLength > ulMaxLength_)
   {
      uLength = ulMaxLength_;
   }

   BOOL quoted = TRUE;

   if (uiMsgId == 579)
      quoted = FALSE;

   if (quoted) {
      **psCurrentByte_ = QUOTE;
      (*psCurrentByte_)++;
   }
   BOOL converted = FALSE;
   std::string expectedString("");
   if (converted) {
      uLength = static_cast<ULONG>(expectedString.length());
      strncpy(*psCurrentByte_, expectedString.c_str(), uLength);
   }
   else
   {
      strncpy(*psCurrentByte_, (CHAR*)pucMyInputData, uLength);
   }
   *psCurrentByte_ += uLength;
   if (quoted) {
      **psCurrentByte_ = QUOTE;
      (*psCurrentByte_)++;
   }
   **psCurrentByte_ = cMySeparator;
   ++*psCurrentByte_;
   INT  iReminder = 0;
   iReminder = uLength % 4;

   if (eConvertType == FIXEDARRAY)
      pucMyInputData += ulMaxLength_;
   else
   {
      uLength += 4 - iReminder;
      pucMyInputData += uLength;
   }
   return GOOD_MESSAGE;
}

MsgConvertStatusEnum BinaryToAsciiComposer::HexParametertoASCII(ULONG ulArraySize_, CHAR** psCurrentByte_)
{
   for (ULONG ulArrayIndex = 1; ulArrayIndex <= ulArraySize_; ++ulArrayIndex)
   {
      int iLength = sprintf(*psCurrentByte_, "%02x", (UINT)*pucMyInputData);
      *psCurrentByte_ += iLength;
      pucMyInputData++;
   }

   **psCurrentByte_ = cMySeparator;
   ++*psCurrentByte_;
   return (GOOD_MESSAGE);
}

MsgConvertStatusEnum BinaryToAsciiComposer::PassThroughtoASCII(ULONG ulArraySize_, CHAR** psCurrentByte_)
{
   for (ULONG ulArrayIndex = 1; ulArrayIndex <= ulArraySize_; ++ulArrayIndex)
   {
      UCHAR ucData = *pucMyInputData;

      //Check to see if this is an ASCII character
      INT iLength = 0;
      if (ucData == '\\')
      {
         iLength = sprintf(*psCurrentByte_, "\\\\");
      }
      else if (((ucData > (UCHAR)32) && (ucData < (UCHAR)127)) || (ucData == ' '))
      {
         //just print out the character
         iLength = sprintf(*psCurrentByte_, "%c", ucData);
      }
      else
      {
         //print it out as a hex character with a () surrounding
         iLength = sprintf(*psCurrentByte_, "\\x%02x", ucData);
      }
      *psCurrentByte_ += iLength;
      pucMyInputData++;
   }
   **psCurrentByte_ = cMySeparator;
   ++*psCurrentByte_;

   return GOOD_MESSAGE;
}

//Function to frame the GLONASS slot id as it has a different signature in ASCII
void BinaryToAsciiComposer::frameGlonassSatIdandSlotFreq(CHAR **psCurrentByte_)
{
   USHORT iValue = *((USHORT*)(pucMyInputData));
   SHORT sValue1 = *((SHORT*)(pucMyInputData + 2));
   *psCurrentByte_ += sprintf(*psCurrentByte_, "%d", iValue);
   std::string strValue = "+";
   if (sValue1 > 0)
   {
      strValue.append(std::to_string(sValue1));
      *psCurrentByte_ += sprintf(*psCurrentByte_, "%s", strValue.c_str());
   }
   else if (sValue1 != 0)
   {
      *psCurrentByte_ += sprintf(*psCurrentByte_, "%d", sValue1);
   }
}

//Function to Convert simple binary data to ASCII
MsgConvertStatusEnum BinaryToAsciiComposer::SimpleParameterToASCII(std::string ulTypeName_, const CHAR*sConvertString_, CHAR** psCurrentByte_, ULONG uLength)
{
	INT iBeforePoint = 0;        //default values
	INT iAfterPoint = 0;         //default values

	CHAR szStrippedConvert[5]; //string shouldn't be bigger then 4 chars long + /0
	INT iStrippedIndex = 0;
	szStrippedConvert[0] = '\0'; //set to null terminated to start
	BOOL bIsBeforePoint = true;
	CHAR *sConvertString = (CHAR*)sConvertString_;
	INT iBytesRead = 0;

	while (*sConvertString)
	{
		if ((*sConvertString >= '0') && (*sConvertString <= '9'))
		{
			if (bIsBeforePoint) //before point
			{
				sscanf(sConvertString, "%d.", &iBeforePoint);
				if (iBeforePoint > 9)
					iBytesRead = 2;
				else
					iBytesRead = 1;
				sConvertString = sConvertString + iBytesRead; // Skip the numerals
			}
			else
			{
				sscanf(sConvertString, "%d", &iAfterPoint);
				if (iAfterPoint > 9)
					iBytesRead = 2;
				else
					iBytesRead = 1;
				sConvertString = sConvertString + iBytesRead; // Skip the numerals
			}
		}

		if ((*sConvertString == '%') ||
			((*sConvertString >= 'a') && (*sConvertString <= 'z')) ||
			((*sConvertString >= 'A') && (*sConvertString <= 'Z')))
		{
			szStrippedConvert[iStrippedIndex] = *sConvertString;
			iStrippedIndex++;
			sConvertString++;
		}

		if (*sConvertString == '.')
		{
			bIsBeforePoint = FALSE; // Found the decimal
			sConvertString++;
		}
	}
	szStrippedConvert[iStrippedIndex] = '\0';

	if (!strcmp(szStrippedConvert, "%m"))
	{
		ULONG ulMessageId_ = *((UINT*)pucMyInputData);
		std::string MsgName = GetMessageNameByID(ulMessageId_ & 0xFFFF);

		switch (EncoderCommon::GetMsgFormat(ulMessageId_))
		{
		case MASCII:
			if (pstBinaryHeader->ucMsgType == '1')
			{
				MsgName.append("A_1");
			}
			else
			{
				MsgName.append("A");
			}
			break;
		case MBINARY:
			MsgName.append("B");
			break;
		default:
			break;
		}
		*psCurrentByte_ += sprintf(*psCurrentByte_, "%s", MsgName.c_str());
	}
	else if (!strcmp(szStrippedConvert, "%T")) //IS it a GPSTime msec field?
	{
		//Get the value as a ULONG msec from the IO struct
		ULONG ulMsecTime = *((UINT*)pucMyInputData);

		//Write it out as an ASCII double sec into the output
		DOUBLE dSecTime = ((DOUBLE)ulMsecTime) / ((DOUBLE)1000);
		*psCurrentByte_ += sprintf(*psCurrentByte_, "%.3lf", dSecTime);
	}
	else if (!strcmp(szStrippedConvert, "%X")) //IS it a Hex that we want to outpit as Ulong ?
	{
		//Get the value as a ULONG msec from the IO struct
		ULONG ulValue = *((UINT*)pucMyInputData);

		if (uiMsgId == 1877 || uiMsgId == 2106 || uiMsgId == 2141) //handled for RADARSTATUS log.
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_, sConvertString_, ulValue);
		}
		else
		{
			//Write it out as an ASCII long into the output
			*psCurrentByte_ += sprintf(*psCurrentByte_, "%lu", ulValue);
		}
	}
	else if (!strcmp(szStrippedConvert, "%B")) //IS it a char field that we want to output as an int?
	{
		//Get the value as a char from the IO struct
		INT iIntValue = (INT)*((CHAR*)pucMyInputData);

		//Write it out as an ASCII int into the output
		*psCurrentByte_ += sprintf(*psCurrentByte_, "%d", iIntValue);
	}
	else if (!strcmp(szStrippedConvert, "%uc")) //IS it a char field that we want to output as an unsigned int?
	{
		//Get the value as a char from the IO struct
		UINT uiIntValue = (UINT)*((UCHAR*)pucMyInputData);

		//Write it out as an ASCII int into the output
		*psCurrentByte_ += sprintf(*psCurrentByte_, "%ud", uiIntValue);
	}
	else if (!strcmp(szStrippedConvert, "%hd")) //IS it unsigned int?
	{
		//Get the value as a char from the IO struct
		SHORT uiIntValue = (SHORT)*(pucMyInputData);

		//Write it out as an ASCII int into the output
		*psCurrentByte_ += sprintf(*psCurrentByte_, "%d", uiIntValue);
	}
	else if (!strcmp(szStrippedConvert, "%id")) //IS it unsigned int?
	{
		if (ulTypeName_ == "SATELLITEID" && m_strEnum == "GLONASS")
		{
			frameGlonassSatIdandSlotFreq(psCurrentByte_);
		}
		else
		{
			UINT uiIntValue = (UINT)*(pucMyInputData);

			//Write it out as an ASCII int into the output
			*psCurrentByte_ += sprintf(*psCurrentByte_, "%u", uiIntValue);
		}
	}
	else if (!strcmp(szStrippedConvert, "%UB")) //IS it a char field that we want to output as an unsigned int?
	{
		//Get the value as a char from the IO struct
		UINT uiIntValue = (UINT)*((UCHAR*)pucMyInputData);

		//Write it out as an ASCII int into the output
		*psCurrentByte_ += sprintf(*psCurrentByte_, "%u", uiIntValue);
	}
	else if (!strcmp(szStrippedConvert, "%u")) //IS it a char field that we want to output as an unsigned int?
	{
      if (ulTypeName_ == "UCHAR")
      {
          //Get the value as a char from the IO struct
         UINT uiIntValue = (UINT)*((UCHAR*)pucMyInputData);
         //Write it out as an ASCII int into the output
         *psCurrentByte_ += sprintf(*psCurrentByte_, "%u", uiIntValue);
         
      }
      else
      {
         UINT uiIntValue = *((UINT*)pucMyInputData);
         //Write it out as an ASCII int into the output
         *psCurrentByte_ += sprintf(*psCurrentByte_, "%u", uiIntValue);
      }

		

	}
	else if (!strcmp(szStrippedConvert, "%XB")) //IS it a char field that we want to output as a hex byte?
	{
		//Get the value as a char from the IO struct
		INT iIntValue = (INT)*((CHAR*)pucMyInputData);
		iIntValue &= 0xFF;

		//Write it out as an ASCII int into the output
		*psCurrentByte_ += sprintf(*psCurrentByte_, "%02x", iIntValue);
	}
	else if (!strcmp(szStrippedConvert, "%k")) //IS it a SUPER Float
	{
		//Get the value as a FLOAT from the IO struct
		FLOAT fFloatVal = *((FLOAT*)pucMyInputData);
		CHAR  acNewConvertString[7];

		if (fFloatVal == 0.0)
			sprintf(acNewConvertString, "%%0.%df", iAfterPoint);
		else if ((iAfterPoint == 0) && (iBeforePoint == 0))
			sprintf(acNewConvertString, "%%0.1f");
		else if (fabs(fFloatVal) > pow(10.0, iBeforePoint))
			sprintf(acNewConvertString, "%%0.%de", iBeforePoint + iAfterPoint - 1);
		else if (fabs(fFloatVal) < pow(10.0, -iBeforePoint))
			sprintf(acNewConvertString, "%%0.%de", iAfterPoint);
		else
			sprintf(acNewConvertString, "%%0.%df", iAfterPoint);

		*psCurrentByte_ += sprintf(*psCurrentByte_, (char*)acNewConvertString, fFloatVal);
	}
	else if (!strcmp(szStrippedConvert, "%lk")) //IS it a SUPER LONG Float
	{
		//Get the value as a FLOAT from the IO struct
		DOUBLE dFloatVal = *((DOUBLE*)pucMyInputData);
		CHAR  acNewConvertString[7];

		if (dFloatVal == 0.0)
			sprintf(acNewConvertString, "%%0.%df", iAfterPoint);
		else if ((iAfterPoint == 0) && (iBeforePoint == 0))
			sprintf(acNewConvertString, "%%0.1f");
		else if (fabs(dFloatVal) > pow(10.0, iBeforePoint))
			sprintf(acNewConvertString, "%%0.%de", iBeforePoint + iAfterPoint - 1); \
		else if (fabs(dFloatVal) < pow(10.0, -iBeforePoint))
			sprintf(acNewConvertString, "%%0.%de", iAfterPoint);
		else
			sprintf(acNewConvertString, "%%0.%df", iAfterPoint);
		*psCurrentByte_ += sprintf(*psCurrentByte_, (CHAR*)acNewConvertString, dFloatVal);
	}
	else if (!strcmp(szStrippedConvert, "%f"))
	{
		if (ulTypeName_ == "DOUBLE")
		{
			DOUBLE dFloatVal = *((DOUBLE*)pucMyInputData);
			CHAR  acNewConvertString[7];
			sprintf(acNewConvertString, "%%0.%df", iAfterPoint);
			*psCurrentByte_ += sprintf(*psCurrentByte_, (CHAR*)acNewConvertString, dFloatVal);
		}
		else
		{
			FLOAT dFloatVal = *((FLOAT*)pucMyInputData);
			CHAR  acNewConvertString[7];
			sprintf(acNewConvertString, "%%0.%df", iAfterPoint);
			*psCurrentByte_ += sprintf(*psCurrentByte_, (CHAR*)acNewConvertString, dFloatVal);
		}
	}
	else  // Otherwise it is a simple type.
	{
		
		if (ulTypeName_ == "BOOL") //BOOL 
		{
			if (*((BOOL*)pucMyInputData))
			{
				*psCurrentByte_ += sprintf(*psCurrentByte_, "TRUE");
			}
			else
			{
				*psCurrentByte_ += sprintf(*psCurrentByte_, "FALSE");
			}
			
		}
		else if (ulTypeName_ == "ULONG")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((UINT*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "LONG")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((UINT*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "FLOAT")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((FLOAT*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "DOUBLE")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((DOUBLE*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "INT")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((INT*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "UCHAR")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((UCHAR*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "CHAR")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((CHAR*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "UINT")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((UINT*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "USHORT")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((USHORT*)pucMyInputData));
			
		}
		else if (ulTypeName_ == "SHORT")
		{
			*psCurrentByte_ += sprintf(*psCurrentByte_,
				(CHAR*)sConvertString_,
				*((SHORT*)pucMyInputData));
			
		}
		else
		{
			return INVALID_MESSAGE_FORMAT;
		}
		
	}
	**psCurrentByte_ = cMySeparator;
	++*psCurrentByte_;

	pucMyInputData += uLength;
	return GOOD_MESSAGE;
}


//Function to Convert the BINARY Header to ASCII header
MsgConvertStatusEnum BinaryToAsciiComposer::FormatHeader(UCHAR *pucIOMessage, CHAR **pAsciiData)
{
   pstBinaryHeader = (OEM4BinaryHeader*)(pucIOMessage);

   if (pstBinaryHeader->ucSync1 == OEM4_BINARY_SYNC1
      && pstBinaryHeader->ucSync2 == OEM4_BINARY_SYNC2
      && pstBinaryHeader->ucSync3 == OEM4_SHORT_BINARY_SYNC3)
   {
      pstBinaryShortHeader = (OEM4BinaryShortHeader*)(pucIOMessage);
      msgStyle = OEM4_COMPRESSED_STYLE;
      uiMsgId = pstBinaryShortHeader->usMessageId;
      usMsgNumber = pstBinaryShortHeader->usMessageId;
   }
   else
   {
      uiMsgId = pstBinaryHeader->usMsgNumber;
      msgStyle = OEM4_MESSAGE_STYLE;
      usMsgNumber = pstBinaryHeader->usMsgNumber;
   }

   cMySeparator = ASCII_FIELD_SEPERATOR;
   if (msgStyle == OEM4_MESSAGE_STYLE)
   {
      **pAsciiData = OEM4_ASCII_SYNC;
   }
   else if (msgStyle == OEM4_COMPRESSED_STYLE)
   {
      **pAsciiData = OEM4_SHORT_ASCII_SYNC;
   }
   else
   {

   }
   ++*pAsciiData;

   strMsgName = GetMessageNameByID(uiMsgId);
   strMsgName.append("A");
   UINT uiTypeMeasurementSource = pstBinaryHeader->ucMsgType & 0x1F;
   if (msgStyle == OEM4_MESSAGE_STYLE && uiTypeMeasurementSource > 0)
   {
      strMsgName.append("_");
      strMsgName.append(std::to_string(uiTypeMeasurementSource));
   }

   *pAsciiData += sprintf(*pAsciiData, "%s%c", strMsgName.c_str(), cMySeparator);

   if (msgStyle == OEM4_MESSAGE_STYLE)
   {
      CHAR* sCommPortName = (CHAR*)"";
      if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT1)
      {
         sCommPortName = (CHAR*)"COM1";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT2)
      {
         sCommPortName = (CHAR*)"COM2";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT3)
      {
         sCommPortName = (CHAR*)"COM3";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT_AUX)
      {
         sCommPortName = (CHAR*)"AUX";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORTX1)
      {
         sCommPortName = (CHAR*)"XCOM1";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORTX2)
      {
         sCommPortName = (CHAR*)"XCOM2";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORTX3)
      {
         sCommPortName = (CHAR*)"XCOM3";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT_UNKNOWN)
      {
         sCommPortName = (CHAR*)"SPECIAL";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT_USB1)
      {
         sCommPortName = (CHAR*)"USB1";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT_USB2)
      {
         sCommPortName = (CHAR*)"USB2";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT_USB3)
      {
         sCommPortName = (CHAR*)"USB3";
      }
      else if (((CommunicationPortEnum)EncoderCommon::getPort(pstBinaryHeader->ucPort)) == MSGCVT_COMPORT_FILE)
      {
         sCommPortName = (CHAR*)"FILE";
      }
      else
      {
         return (INVALID_MESSAGE_FORMAT);
      }
      

      if (EncoderCommon::getVirtualPortAddress(pstBinaryHeader->ucPort) > MAX_VIRTUAL_ADDRESS)
         return INVALID_MESSAGE_FORMAT;

      if (EncoderCommon::getVirtualPortAddress(pstBinaryHeader->ucPort) == 0)
      {
         *pAsciiData += sprintf(*pAsciiData, "%s%c", sCommPortName, cMySeparator);
      }
      else
      {
         *pAsciiData += sprintf(*pAsciiData, "%s_%u%c", sCommPortName, EncoderCommon::getVirtualPortAddress(pstBinaryHeader->ucPort), cMySeparator);
      }

      *pAsciiData += sprintf(*pAsciiData, "%d%c", pstBinaryHeader->usSequenceNumber, cMySeparator);

      DOUBLE fIdleTime = ((UCHAR)pstBinaryHeader->ucIdleTime) * 0.500;
      *pAsciiData += sprintf(*pAsciiData, "%.1f%c", fIdleTime, cMySeparator);

      UINT uiTimeStatus = (MsgConvertTimeStatusEnum)pstBinaryHeader->ucTimeStatus;
      *pAsciiData += sprintf(*pAsciiData, "%s%c", EncoderCommon::getTimeStatus(uiTimeStatus).c_str(), cMySeparator);

      *pAsciiData += sprintf(*pAsciiData, "%hu%c", (USHORT)pstBinaryHeader->usWeekNo, cMySeparator);

      *pAsciiData += sprintf(*pAsciiData, "%.3f%c", (DOUBLE)pstBinaryHeader->ulWeekMSec / 1000, cMySeparator);

      *pAsciiData += sprintf(*pAsciiData, "%08lx%c", pstBinaryHeader->ulStatus, cMySeparator);

      *pAsciiData += sprintf(*pAsciiData, "%04x%c", (USHORT)pstBinaryHeader->usMsgDefCRC, cMySeparator);

      *pAsciiData += sprintf(*pAsciiData, "%d", (USHORT)pstBinaryHeader->usReceiverSWVersion);

      *pAsciiData += sprintf(*pAsciiData, "%c", ASCII_HEADER_TERMINATOR);
   }

   if (msgStyle == OEM4_COMPRESSED_STYLE)
   {
      *pAsciiData += sprintf(*pAsciiData, "%hu%c", (USHORT)pstBinaryShortHeader->usWeekNo, cMySeparator);

      *pAsciiData += sprintf(*pAsciiData, "%.3f", (DOUBLE)pstBinaryShortHeader->ulWeekMSec / 1000);

      *pAsciiData += sprintf(*pAsciiData, "%c", ASCII_HEADER_TERMINATOR);
   }

   return GOOD_MESSAGE;
}
