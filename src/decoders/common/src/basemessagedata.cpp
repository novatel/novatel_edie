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
//    Base Message Data Class used by EDIE Components
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "basemessagedata.hpp"

// code
// ---------------------------------------------------------------------------------------
BaseMessageData::BaseMessageData()
   : pcMyBinaryMessageData(NULL),
   pcMyAsciiMessageData(NULL),
   pcMyAbbAsciiMessageData(NULL),
   pcMyUnknownMessageData(NULL),
   pcMyRinexMessageData(NULL),
   pcMyNMEA2000MessageData(NULL),
   pcMyFlattenMessageData(NULL),
   sMyjson(""),
   sMyHeaderjson(""),
   uiMyBinaryMessageLength(0),
   uiMyFlattenBinaryMessageLength(0),
   uiMyAsciiMessageLength(0),
   uiMyAbbAsciiMessageLength(0),
   uiMyUnknownMessageLength(0),
   uiMyRinexMessageLength(0),
   uiMyNMEA2000MessageLength(0),
   uiMyMessageID(0),
   eMyMessageFormat(MESSAGE_UNKNOWN),
   eMyMessageTimeStatus(TIME_UNKNOWN),
   ulMyGnssWeek(0),
   ulMyMilliSeconds(0),
   eMyMessageSource(PRIMARY_ANTENNA),
   ulMyMessageDefCrc(0),
   sMyMessageName(""),
   ulMyReceiverStatus(0),
   ulMyReceiverSWVersion(0),
   dMyIdleTime(0),
   ucMyPortAddress(0),
   bIsMyMessageType(FALSE),
   bIsMyErrorResponse(FALSE),
   iMyResponseID(0)
{
}
// ---------------------------------------------------------------------------------------
BaseMessageData::BaseMessageData(MessageHeader* pstMessageHeader, CHAR* pcDecodedMessage)
   :sMyMessageName(pstMessageHeader->szMessageName),
   uiMyMessageID(pstMessageHeader->uiMessageID),
   eMyMessageFormat(pstMessageHeader->eMessageFormat),
   ulMyGnssWeek(pstMessageHeader->ulMessageWeek),
   ulMyMilliSeconds(pstMessageHeader->ulMessageTime),
   ulMyMessageDefCrc(pstMessageHeader->ulMessageDefCrc),
   ulMyReceiverStatus(pstMessageHeader->ulReceiverStatus),
   ulMyReceiverSWVersion(pstMessageHeader->ulReceiverSWVersion),
   dMyIdleTime(pstMessageHeader->dIdleTime),
   eMyMessageTimeStatus(pstMessageHeader->eMessageTimeStatus),
   eMyMessageSource(pstMessageHeader->eMessageSource),
   ucMyPortAddress(pstMessageHeader->ucPortAddress),
   bIsMyMessageType(pstMessageHeader->bMessageType),
   iMyResponseID(pstMessageHeader->iReponseID),
   bIsMyErrorResponse(pstMessageHeader->bErrorResponse),
   pcMyBinaryMessageData(NULL),
   pcMyAsciiMessageData(NULL),
   pcMyAbbAsciiMessageData(NULL),
   pcMyUnknownMessageData(NULL),
   pcMyRinexMessageData(NULL),
   pcMyNMEA2000MessageData(NULL),
   pcMyFlattenMessageData(NULL),
   sMyjson(""),
   sMyHeaderjson(""),
   uiMyBinaryMessageLength(0),
   uiMyFlattenBinaryMessageLength(0),
   uiMyAsciiMessageLength(0),
   uiMyAbbAsciiMessageLength(0),
   uiMyUnknownMessageLength(0),
   uiMyRinexMessageLength(0),
   uiMyNMEA2000MessageLength(0)
{

   if (pstMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_BINARY
         || pstMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY)
   {
      pcMyBinaryMessageData = pcDecodedMessage;
      uiMyBinaryMessageLength = pstMessageHeader->uiMessageLength;
   }
   else if (pstMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_ASCII
         || pstMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII)

   {
      pcMyAsciiMessageData = pcDecodedMessage;
      uiMyAsciiMessageLength = pstMessageHeader->uiMessageLength;
   }
   else if (pstMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_ABB_ASCII)
   {
      pcMyAbbAsciiMessageData = pcDecodedMessage;
      uiMyAbbAsciiMessageLength = pstMessageHeader->uiMessageLength;
   }
   else if (pstMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_RINEX)
   {
      pcMyRinexMessageData = pcDecodedMessage;
      uiMyRinexMessageLength = pstMessageHeader->uiMessageLength;
   }
   else if (pstMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
   {
      pcMyNMEA2000MessageData = pcDecodedMessage;
      uiMyNMEA2000MessageLength = pstMessageHeader->uiMessageLength;
   }
   else
   {
      pcMyUnknownMessageData = pcDecodedMessage;
      uiMyUnknownMessageLength = pstMessageHeader->uiMessageLength;
   }
}

// ---------------------------------------------------------
BaseMessageData::BaseMessageData(const BaseMessageData &cBaseMessageData)
   :sMyMessageName(cBaseMessageData.sMyMessageName),
   uiMyMessageID(cBaseMessageData.uiMyMessageID),
   eMyMessageFormat(cBaseMessageData.eMyMessageFormat),
   ulMyGnssWeek(cBaseMessageData.ulMyGnssWeek),
   ulMyMilliSeconds(cBaseMessageData.ulMyMilliSeconds),
   ulMyMessageDefCrc(cBaseMessageData.ulMyMessageDefCrc),
   ulMyReceiverStatus(cBaseMessageData.ulMyReceiverStatus),
   ulMyReceiverSWVersion(cBaseMessageData.ulMyReceiverSWVersion),
   dMyIdleTime(cBaseMessageData.dMyIdleTime),
   eMyMessageTimeStatus(cBaseMessageData.eMyMessageTimeStatus),
   eMyMessageSource(cBaseMessageData.eMyMessageSource),
   ucMyPortAddress(cBaseMessageData.ucMyPortAddress),
   bIsMyMessageType(cBaseMessageData.bIsMyMessageType),
   iMyResponseID(cBaseMessageData.iMyResponseID),
   bIsMyErrorResponse(cBaseMessageData.bIsMyErrorResponse),
   pcMyBinaryMessageData(NULL),
   pcMyAsciiMessageData(NULL),
   pcMyAbbAsciiMessageData(NULL),
   pcMyUnknownMessageData(NULL),
   pcMyRinexMessageData(NULL),
   pcMyNMEA2000MessageData(NULL),
   pcMyFlattenMessageData(NULL),
   sMyjson(cBaseMessageData.sMyjson),
   sMyHeaderjson(cBaseMessageData.sMyHeaderjson),
   uiMyBinaryMessageLength(cBaseMessageData.uiMyBinaryMessageLength),
   uiMyFlattenBinaryMessageLength(cBaseMessageData.uiMyFlattenBinaryMessageLength),
   uiMyAsciiMessageLength(cBaseMessageData.uiMyAsciiMessageLength),
   uiMyAbbAsciiMessageLength(cBaseMessageData.uiMyAbbAsciiMessageLength),
   uiMyUnknownMessageLength(cBaseMessageData.uiMyUnknownMessageLength),
   uiMyRinexMessageLength(cBaseMessageData.uiMyRinexMessageLength),
   uiMyNMEA2000MessageLength(cBaseMessageData.uiMyNMEA2000MessageLength)
{
   if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_BINARY
      || cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY)
   {
      pcMyBinaryMessageData = new CHAR[uiMyBinaryMessageLength];
      memcpy(pcMyBinaryMessageData, cBaseMessageData.pcMyBinaryMessageData, uiMyBinaryMessageLength);
	  pcMyFlattenMessageData = new CHAR[uiMyFlattenBinaryMessageLength];
	  memcpy(pcMyFlattenMessageData, cBaseMessageData.pcMyFlattenMessageData, uiMyFlattenBinaryMessageLength);

   }
   else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_ASCII
           || cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII)
   {
      pcMyAsciiMessageData = new CHAR[uiMyAsciiMessageLength];
      memcpy(pcMyAsciiMessageData, cBaseMessageData.pcMyAsciiMessageData, uiMyAsciiMessageLength);
   }
   else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_ABB_ASCII)
   {
      pcMyAbbAsciiMessageData = new CHAR[uiMyAbbAsciiMessageLength];
      memcpy(pcMyAbbAsciiMessageData, cBaseMessageData.pcMyAbbAsciiMessageData, uiMyAbbAsciiMessageLength);
   }
   else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_RINEX)
   {
      pcMyRinexMessageData = new CHAR[uiMyRinexMessageLength];
      memcpy(pcMyRinexMessageData, cBaseMessageData.pcMyRinexMessageData, uiMyRinexMessageLength);
   }
   else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
   {
      pcMyNMEA2000MessageData = new CHAR[uiMyNMEA2000MessageLength];
      memcpy(pcMyNMEA2000MessageData, cBaseMessageData.pcMyNMEA2000MessageData, uiMyNMEA2000MessageLength);
   }
   else
   {
      pcMyUnknownMessageData = new CHAR[uiMyUnknownMessageLength];
      memcpy(pcMyUnknownMessageData, cBaseMessageData.pcMyUnknownMessageData, uiMyUnknownMessageLength);
   }
}

BaseMessageData& BaseMessageData::operator=(const BaseMessageData& cBaseMessageData)
{
   if (&cBaseMessageData != this)
   {
      sMyMessageName = cBaseMessageData.sMyMessageName;
      uiMyMessageID = cBaseMessageData.uiMyMessageID;
      eMyMessageFormat = cBaseMessageData.eMyMessageFormat;
      ulMyGnssWeek = cBaseMessageData.ulMyGnssWeek;
      ulMyMilliSeconds = cBaseMessageData.ulMyMilliSeconds;
      ulMyMessageDefCrc = cBaseMessageData.ulMyMessageDefCrc;
      ulMyReceiverStatus = cBaseMessageData.ulMyReceiverStatus;
      ulMyReceiverSWVersion = cBaseMessageData.ulMyReceiverSWVersion;
      dMyIdleTime = cBaseMessageData.dMyIdleTime;
      eMyMessageTimeStatus = cBaseMessageData.eMyMessageTimeStatus;
      eMyMessageSource = cBaseMessageData.eMyMessageSource;
      ucMyPortAddress = cBaseMessageData.ucMyPortAddress;
      bIsMyMessageType = cBaseMessageData.bIsMyMessageType;
      bIsMyErrorResponse = cBaseMessageData.bIsMyErrorResponse;
      iMyResponseID = cBaseMessageData.iMyResponseID;
      pcMyBinaryMessageData = NULL;
      pcMyAsciiMessageData = NULL;
      pcMyAbbAsciiMessageData = NULL;
      pcMyUnknownMessageData = NULL;
      pcMyRinexMessageData = NULL;
      pcMyNMEA2000MessageData = NULL;
	  pcMyFlattenMessageData = NULL;
	  sMyjson = cBaseMessageData.sMyjson;
      sMyHeaderjson = cBaseMessageData.sMyHeaderjson;
      uiMyBinaryMessageLength = cBaseMessageData.uiMyBinaryMessageLength;
	  uiMyFlattenBinaryMessageLength = cBaseMessageData.uiMyFlattenBinaryMessageLength;
      uiMyAsciiMessageLength = cBaseMessageData.uiMyAsciiMessageLength;
      uiMyAbbAsciiMessageLength = cBaseMessageData.uiMyAbbAsciiMessageLength;
      uiMyUnknownMessageLength = cBaseMessageData.uiMyUnknownMessageLength;
      uiMyRinexMessageLength = cBaseMessageData.uiMyRinexMessageLength;
      uiMyNMEA2000MessageLength = cBaseMessageData.uiMyNMEA2000MessageLength;

      if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_BINARY
         || cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY)
      {
         pcMyBinaryMessageData = new CHAR[uiMyBinaryMessageLength];
         memcpy(pcMyBinaryMessageData, cBaseMessageData.pcMyBinaryMessageData, uiMyBinaryMessageLength);
		 pcMyFlattenMessageData = new CHAR[uiMyFlattenBinaryMessageLength];
		 memcpy(pcMyFlattenMessageData, cBaseMessageData.pcMyFlattenMessageData, uiMyFlattenBinaryMessageLength);
	  }
      else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_ASCII
         || cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII)
      {
         pcMyAsciiMessageData = new CHAR[uiMyAsciiMessageLength];
         memcpy(pcMyAsciiMessageData, cBaseMessageData.pcMyAsciiMessageData, uiMyAsciiMessageLength);
      }
      else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_ABB_ASCII)
      {
         pcMyAbbAsciiMessageData = new CHAR[uiMyAbbAsciiMessageLength];
         memcpy(pcMyAbbAsciiMessageData, cBaseMessageData.pcMyAbbAsciiMessageData, uiMyAbbAsciiMessageLength);
      }
      else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_RINEX)
      {
         pcMyRinexMessageData = new CHAR[uiMyRinexMessageLength];
         memcpy(pcMyRinexMessageData, cBaseMessageData.pcMyRinexMessageData, uiMyRinexMessageLength);
      }
      else if (cBaseMessageData.eMyMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
      {
         pcMyNMEA2000MessageData = new CHAR[uiMyNMEA2000MessageLength];
         memcpy(pcMyNMEA2000MessageData, cBaseMessageData.pcMyNMEA2000MessageData, uiMyNMEA2000MessageLength);
      }
      else
      {
         pcMyUnknownMessageData = new CHAR[uiMyUnknownMessageLength];
         memcpy(pcMyUnknownMessageData, cBaseMessageData.pcMyUnknownMessageData, uiMyUnknownMessageLength);
      }
   }
   return *this;
}

// ---------------------------------------------------------
void BaseMessageData::setMessageName(const std::string& iMessageName)
{
   sMyMessageName = iMessageName;
}

// ---------------------------------------------------------
std::string BaseMessageData::getMessageName(void) const
{
   return sMyMessageName;
}

void BaseMessageData::setMessageID(UINT uiMessageID)
{
   uiMyMessageID = uiMessageID;
}

// ---------------------------------------------------------
UINT BaseMessageData::getMessageID(void) const
{
   return uiMyMessageID;
}

//----------------------------------------------------------
void BaseMessageData::setMessageFormat(MessageFormatEnum eMessageFormat)
{
   eMyMessageFormat = eMessageFormat;
}

//----------------------------------------------------------
MessageFormatEnum BaseMessageData::getMessageFormat(void) const
{
   return eMyMessageFormat;
}

//-----------------------------------------------------------
void BaseMessageData::setMessageTimeStatus(MessageTimeStatusEnum eMessageTimeStatus)
{
   eMyMessageTimeStatus = eMessageTimeStatus;
}

//-----------------------------------------------------------
MessageTimeStatusEnum BaseMessageData::getMessageTimeStatus(void) const
{
   return eMyMessageTimeStatus;
}

//-----------------------------------------------------------
void BaseMessageData::setMessageTimeWeek(ULONG ulGnssWeek)
{
   ulMyGnssWeek = ulGnssWeek;
}

//----------------------------------------------------------
ULONG BaseMessageData::getMessageTimeWeek(void) const
{
   return ulMyGnssWeek;
}

//----------------------------------------------------------
void BaseMessageData::setMessageTimeMilliSeconds(ULONG ulMilliSeconds)
{
   ulMyMilliSeconds = ulMilliSeconds;
}

//----------------------------------------------------------
ULONG BaseMessageData::getMessageTimeMilliSeconds(void) const
{
   return ulMyMilliSeconds;
}

//------------------------------------------------------------------------------
void BaseMessageData::setMessageSource(MessageAntennaSourceEnum eMessageSource)
{
   eMyMessageSource = eMessageSource;
}

//------------------------------------------------------------------------------
void BaseMessageData::setMessageType(BOOL bMessageType)
{
   bIsMyMessageType = bMessageType;
}

//------------------------------------------------------------------------------
BOOL BaseMessageData::getMessageType(void) const
{
   return bIsMyMessageType;
}

//------------------------------------------------------------------------------
void BaseMessageData::setResponseID(INT iResponseID)
{
   iMyResponseID = iResponseID;
}

//------------------------------------------------------------------------------
INT BaseMessageData::getResponseID(void) const
{
   return iMyResponseID;
}

//------------------------------------------------------------------------------
void BaseMessageData::setResponseType(BOOL bReponse)
{
   bIsMyErrorResponse = bReponse;
}

//------------------------------------------------------------------------------
BOOL BaseMessageData::getResponseType(void) const
{
   return bIsMyErrorResponse;
}

//---------------------------------------------------------------------
MessageAntennaSourceEnum BaseMessageData::getMessageSource(void) const
{
   return eMyMessageSource;
}

// ---------------------------------------------------------
void BaseMessageData::setMessageLength(UINT uiMessageLength)
{
   if (eMyMessageFormat == MessageFormatEnum::MESSAGE_BINARY
      || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY)
      uiMyBinaryMessageLength = uiMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ASCII
      || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII)
      uiMyAsciiMessageLength = uiMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ABB_ASCII)
      uiMyAbbAsciiMessageLength = uiMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_RINEX)
      uiMyRinexMessageLength = uiMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
      uiMyNMEA2000MessageLength = uiMessageLength;
   else
      uiMyUnknownMessageLength = uiMessageLength;
}

//----------------------------------------------------------
void BaseMessageData::setFlattenMessageLength(UINT uiMessageLength)
{
	uiMyFlattenBinaryMessageLength = uiMessageLength;
}
// ---------------------------------------------------------
UINT BaseMessageData::getMessageLength(void) const
{
   if (eMyMessageFormat == MessageFormatEnum::MESSAGE_BINARY
      || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY)
      return uiMyBinaryMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ASCII
      || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII)
      return uiMyAsciiMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ABB_ASCII)
      return uiMyAbbAsciiMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_RINEX)
      return uiMyRinexMessageLength;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
      return uiMyNMEA2000MessageLength;
   else
      return uiMyUnknownMessageLength;
}
// ---------------------------------------------------------
UINT BaseMessageData::getFlattenMessageLength(void) const
{
	return uiMyFlattenBinaryMessageLength;
}
// ---------------------------------------------------------
void BaseMessageData::setMessageDefCrc(ULONG ulMessageDefCrc)
{
   ulMyMessageDefCrc = ulMessageDefCrc;
}

// ---------------------------------------------------------
ULONG BaseMessageData::getMessageDefCrc(void) const
{
   return ulMyMessageDefCrc;
}

// ---------------------------------------------------------
void BaseMessageData::setIdleTime(DOUBLE dIdleTime)
{
   dMyIdleTime = dIdleTime;
}

// ---------------------------------------------------------
DOUBLE BaseMessageData::getIdleTime(void) const
{
   return dMyIdleTime;
}

// ---------------------------------------------------------
void BaseMessageData::setReceiverStatus(ULONG ulReceiverStatus)
{
   ulMyReceiverStatus = ulReceiverStatus;
}

// ---------------------------------------------------------
ULONG BaseMessageData::getReceiverStatus(void) const
{
   return ulMyReceiverStatus;
}

// ---------------------------------------------------------
void BaseMessageData::setReceiverSWVersion(ULONG ulReceiverSWVersion)
{
   ulMyReceiverSWVersion = ulReceiverSWVersion;
}

// ---------------------------------------------------------
ULONG BaseMessageData::getReceiverSWVersion(void) const
{
   return ulMyReceiverSWVersion;
}

// ---------------------------------------------------------
void BaseMessageData::setMessagePort(UCHAR ucPort)
{
   ucMyPortAddress = ucPort;
}

// ---------------------------------------------------------
UCHAR BaseMessageData::getMessagePort(void) const
{
   return ucMyPortAddress;
}

// ---------------------------------------------------------
void BaseMessageData::setMessageData(CHAR *pcDecodedMessage)
{
   if (eMyMessageFormat == MessageFormatEnum::MESSAGE_BINARY
         || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY)
      pcMyBinaryMessageData = pcDecodedMessage;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ASCII
      || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII)
      pcMyAsciiMessageData = pcDecodedMessage;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ABB_ASCII)
      pcMyAbbAsciiMessageData = pcDecodedMessage;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_RINEX)
      pcMyRinexMessageData = pcDecodedMessage;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
      pcMyNMEA2000MessageData = pcDecodedMessage;
   else
      pcMyUnknownMessageData = pcDecodedMessage;
}
void BaseMessageData::setFlattenMessageData(CHAR *pcDecodedMessage)
{
	pcMyFlattenMessageData = pcDecodedMessage;
}
void BaseMessageData::setMessagejsonstring(std::string sjasonMessage)
{
	sMyjson = sjasonMessage;
}
void BaseMessageData::setHeaderjsonstring(std::string sjsonheader)
{
	sMyHeaderjson = sjsonheader;
}
// ---------------------------------------------------------
CHAR* BaseMessageData::getMessageData(void)
{
   if (eMyMessageFormat == MessageFormatEnum::MESSAGE_BINARY
       || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY)
      return pcMyBinaryMessageData;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ASCII
      || eMyMessageFormat == MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII)
      return pcMyAsciiMessageData;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_ABB_ASCII)
      return pcMyAbbAsciiMessageData;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_RINEX)
      return pcMyRinexMessageData;
   else if (eMyMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
      return pcMyNMEA2000MessageData;
   else
      return pcMyUnknownMessageData;
}
// ---------------------------------------------------------
CHAR* BaseMessageData::getFlattenMessageData(void)
{

	return pcMyFlattenMessageData;
}
// ---------------------------------------------------------
std::string BaseMessageData::getjsonMessageData(void)
{

	return sMyjson;
}
// ---------------------------------------------------------
std::string BaseMessageData::getjsonHeaderData(void)
{
	return sMyHeaderjson;
}
// ---------------------------------------------------------
INT BaseMessageData::getNMEAMsgFieldCount()
{
   // NMEA will be in ASCII only
   if (eMyMessageFormat != MESSAGE_ASCII)
   {
      return 0;
   }

   // Check for NMEA or not?
   std::string szMessage(getMessageData(), uiMyAsciiMessageLength);
   if (szMessage[0] != '$')
   {
      return 0;
   }

   // Extract values from message string
   std::vector<std::string> aszMessageParameters;
   std::stringstream szMessageStream(szMessage);
   while (szMessageStream.good())
   {
      std::string szParameter;
      std::getline(szMessageStream, szParameter, ',');
      aszMessageParameters.push_back(szParameter);
   }
   return (INT)aszMessageParameters.size();
}
// ---------------------------------------------------------
/*void BaseMessageData::CopyMessage(CHAR* pcMsgBuf)
{
	throw nExcept("Message Doesn't have derived method");
}*/
UINT BaseMessageData::GetMessageSize(void)
{
	throw nExcept("Message Doesn't have derived method");
}
UINT BaseMessageData::GetObjSize(void)
{
	throw nExcept("Message Doesn't have derived method");
}
// ---------------------------------------------------------
BaseMessageData::~BaseMessageData()
{
   if (pcMyBinaryMessageData != NULL)
   {
      delete[] pcMyBinaryMessageData;
      pcMyBinaryMessageData = NULL;
   }

   if (pcMyAsciiMessageData != NULL)
   {
      delete[] pcMyAsciiMessageData;
      pcMyAsciiMessageData = NULL;
   }

   if (pcMyAbbAsciiMessageData != NULL)
   {
      delete[] pcMyAbbAsciiMessageData;
      pcMyAbbAsciiMessageData = NULL;
   }

   if (pcMyUnknownMessageData != NULL)
   {
      delete[] pcMyUnknownMessageData;
      pcMyUnknownMessageData = NULL;
   }

   if (pcMyRinexMessageData != NULL)
   {
      delete[] pcMyRinexMessageData;
      pcMyRinexMessageData = NULL;
   }

   if (pcMyNMEA2000MessageData != NULL)
   {
      delete[] pcMyNMEA2000MessageData;
      pcMyNMEA2000MessageData = NULL;
   }

   if (pcMyFlattenMessageData != NULL)
   {
      delete[] pcMyFlattenMessageData;
      pcMyFlattenMessageData = NULL;
   }
}
// ----------------------------------------------------------
