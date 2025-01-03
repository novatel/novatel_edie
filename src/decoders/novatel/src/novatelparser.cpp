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
//    Class to parse the binary, ascii and abb ascii data
//
////////////////////////////////////////////////////////////////////////////////
#include "novatelparser.hpp"
// -------------------------------------------------------------------------------------------------------
NovatelParser::NovatelParser(InputStreamInterface* pclInputStreamInterface)
   :eMyParseStatus(MessageParseStatusEnum(-1))
{
   pclMyInputStreamInterface = pclInputStreamInterface;
   bIsInputStreamEOF = FALSE;
   uiMyByteCount = 0;
   bMySkipCrcCheck = FALSE;
   vcMyUnknownBytes.clear();

   //Prepare Time Status string to enum map
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("UNKNOWN",            TIME_UNKNOWN));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("APPROXIMATE",        TIME_APPROXIMATE));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("COARSEADJUSTING",    TIME_COARSEADJUSTING));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("COARSE",             TIME_COARSE));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("COARSESTEERING",     TIME_COARSESTEERING));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("FREEWHEELING",       TIME_FREEWHEELING));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("FINEADJUSTING",      TIME_FINEADJUSTING));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("FINE",               TIME_FINE));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("FINEBACKUPSTEERING", TIME_FINEBACKUPSTEERING));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("FINESTEERING",       TIME_FINESTEERING));
   mTimeStatusMap.insert(std::pair<std::string, MessageTimeStatusEnum>("SATTIME",            TIME_SATTIME));
}

// -------------------------------------------------------------------------------------------------------
StreamReadStatus NovatelParser::ParseData(CHAR** pcMessageBuffer, MessageHeader* stMessageHeader)
{
   UINT uiCalculatedCrc = 0;                       // CRC Claculated so far for the current binary message
   UINT uiBinaryMessageLength = 0;                 // Binary Message length                               // Holding current character in buffer
   OEM4BinaryHeader sOEM4BinaryHeader;             // Binary header structure
   OEM4BinaryShortHeader sOEM4BinaryShortHeader;   // Binart Short header structure

   // Store unknwon bytes till next valid sync or 10kb whichever is earlier
   BOOL bGotSync = FALSE;
   vcMyUnknownBytes.clear();

   // Set the default parsing status
   eMyParseStatus = WAITING_FOR_SYNC;

   // loop buffer till colpmete message
   while (eMyParseStatus != COMPLETE_MESSAGE)
   {
      // Read data from file or port when circular buffer reach end
      // or we didn't find complete frame in current data buffer
      if (( cMyCircularDataBuffer.GetLength() == 0)
         || ( cMyCircularDataBuffer.GetLength() == uiMyByteCount))
      {
		 if (pclMyInputStreamInterface->IsStreamAvailable())
		 {
			 bIsInputStreamEOF = FALSE;
		 }
         if (bIsInputStreamEOF != TRUE)
         {
            if (ReadInputStream() == 0)
            {
               uiMyByteCount = 0;
               //Return to application if we didn't get data from stream
               return eMyStreamReadStatus;
            }
         }
         else
         {
            eMyStreamReadStatus.bEOS = TRUE;
            // Send remaining data as unknown to application if we reach end of stream
            if (vcMyUnknownBytes.size() != 0)
            {
               *pcMessageBuffer = new CHAR[vcMyUnknownBytes.size()+1];
               memset(*pcMessageBuffer, '\0', vcMyUnknownBytes.size()+1);
               std::copy(vcMyUnknownBytes.begin(), vcMyUnknownBytes.end(), *pcMessageBuffer);

               stMessageHeader->uiMessageID = 0;
               stMessageHeader->szMessageName = "UNKNOWN";
               stMessageHeader->uiMessageLength = static_cast<UINT>(vcMyUnknownBytes.size());

               vcMyUnknownBytes.clear();
               uiMyByteCount = 0;
            }
            return eMyStreamReadStatus;
         }

      }

      // Get one character at a time
      UCHAR ucDataByte = cMyCircularDataBuffer[uiMyByteCount++];

      switch (eMyParseStatus)
      {
      case WAITING_FOR_SYNC:
         if (ucDataByte == OEM4_BINARY_SYNC1)
         {
            // Got first binary sync byte
            uiCalculatedCrc = 0;
            uiBinaryMessageLength = 0;
            uiCalculatedCrc = CalculateCharacterCrc(ucDataByte, uiCalculatedCrc);
            eMyParseStatus = WAITING_FOR_BINARY_SYNC2;
            bGotSync = TRUE;
         }
         else if (ucDataByte == OEM4_ASCII_SYNC)
         {
            // Got ASCII sync byte
            eMyParseStatus = WAITING_FOR_ASCII_BODY;
            bGotSync = TRUE;
         }
         else if (ucDataByte == OEM4_SHORT_ASCII_SYNC)
         {
            // Got Short ASCII sync byte
            eMyParseStatus = WAITING_FOR_SHORT_ASCII_BODY;
            bGotSync = TRUE;
         }
         else if (ucDataByte == NMEA_MESSAGE_SYNC)
         {
            // Got NMEA Message Sync byte
            eMyParseStatus = WAITING_FOR_NMEA_BODY;
            bGotSync = TRUE;
         }
         else if (ucDataByte == OEM4_ABBASCII_SYNC)
         {
            // Decode Abb Ascii Response only
            eMyParseStatus = WAITING_FOR_ABB_ASCII_BODY;
            bGotSync = TRUE;
         }
         else
         {
            // Unknown byte, discard and proceed
            HandleInvalidData();
            bGotSync = FALSE;
         }

         // Send unknown data to application on valid sync or 10kb of unknown data
         if (((bGotSync == TRUE) && (vcMyUnknownBytes.size() != 0) )||
            (vcMyUnknownBytes.size() == READ_BUFFER_SIZE))
         {
            *pcMessageBuffer = new CHAR[vcMyUnknownBytes.size()+1];
            memset(*pcMessageBuffer, '\0', vcMyUnknownBytes.size()+1);
            std::copy(vcMyUnknownBytes.begin(), vcMyUnknownBytes.end(), *pcMessageBuffer);

            stMessageHeader->uiMessageID = 0;
            stMessageHeader->szMessageName = "UNKNOWN";
            stMessageHeader->uiMessageLength = static_cast<UINT>(vcMyUnknownBytes.size());

            vcMyUnknownBytes.clear();
            uiMyByteCount = 0;
            return eMyStreamReadStatus;
         }
         break;

      case WAITING_FOR_BINARY_SYNC2:
         if (ucDataByte == OEM4_BINARY_SYNC2)
         {
            // Got second binary sync byte
            uiCalculatedCrc = CalculateCharacterCrc(ucDataByte, uiCalculatedCrc);
            eMyParseStatus = WAITING_FOR_BINARY_SYNC3;
         }
         else
         {
            // Expecting second binary sync byte and not received
            // Discard first byte and proceed
            HandleInvalidData();
            bGotSync = FALSE;
         }
         break;

      case WAITING_FOR_BINARY_SYNC3:
         if (ucDataByte == OEM4_BINARY_SYNC3)
         {
            // Got third binary sync byte
            uiCalculatedCrc = CalculateCharacterCrc(ucDataByte, uiCalculatedCrc);
            eMyParseStatus = WAITING_FOR_BINARY_HEADER;
         }
         else if (ucDataByte == OEM4_SHORT_BINARY_SYNC3)
         {
            // Got third binary sync byte
            uiCalculatedCrc = CalculateCharacterCrc(ucDataByte, uiCalculatedCrc);
            eMyParseStatus = WAITING_FOR_SHORT_BINARY_HEADER;
         }
         else
         {
            // Expecting third binary sync byte and not received
            // Discard first byte and proceed
            HandleInvalidData();
            bGotSync = FALSE;
         }
         break;

      case WAITING_FOR_BINARY_HEADER:
         uiCalculatedCrc = CalculateCharacterCrc(ucDataByte, uiCalculatedCrc);
         if (uiMyByteCount == OEM4_BINARY_HEADER_LENGTH)
         {
            // Got data of header length, try to parse binary header
            cMyCircularDataBuffer.Copy((UCHAR*)&sOEM4BinaryHeader, OEM4_BINARY_HEADER_LENGTH);
            uiBinaryMessageLength = OEM4_BINARY_HEADER_LENGTH + sOEM4BinaryHeader.usLength + OEM4_BINARY_CRC_LENGTH;
            eMyParseStatus = WAITING_FOR_BINARY_BODY_AND_CRC;
         }
         break;

      case WAITING_FOR_SHORT_BINARY_HEADER:
         uiCalculatedCrc = CalculateCharacterCrc(ucDataByte, uiCalculatedCrc);
         if (uiMyByteCount == OEM4_BINARY_SHORT_HEADER_LENGTH)
         {
            // Got data of header length, try to parse binary short header
            cMyCircularDataBuffer.Copy((UCHAR*)&sOEM4BinaryShortHeader, OEM4_BINARY_SHORT_HEADER_LENGTH);
            uiBinaryMessageLength = OEM4_BINARY_SHORT_HEADER_LENGTH + sOEM4BinaryShortHeader.ucLength + OEM4_BINARY_CRC_LENGTH;
            eMyParseStatus = WAITING_FOR_SHORT_BINARY_BODY_AND_CRC;
         }
         break;

      case WAITING_FOR_BINARY_BODY_AND_CRC:        //Intentional fall-through
      case WAITING_FOR_SHORT_BINARY_BODY_AND_CRC:
         uiCalculatedCrc = CalculateCharacterCrc(ucDataByte, uiCalculatedCrc);
         if (uiBinaryMessageLength == uiMyByteCount)
         {
            if (uiCalculatedCrc == 0)
            {
               // Got complete binary message with valid CRC.
               // This memory will be deleted in BaseMessageData destructor
               *pcMessageBuffer = new CHAR[uiBinaryMessageLength+1];
               memset(*pcMessageBuffer, '\0', uiBinaryMessageLength+1);

               if (*pcMessageBuffer == NULL)
                  throw "Error while creating memory for binary message";

               // Copy the message to message buffer
               cMyCircularDataBuffer.Copy((UCHAR*)*pcMessageBuffer,uiBinaryMessageLength);

               if (sOEM4BinaryHeader.ucMsgType & 0x80) // Reponse
               {
                  stMessageHeader->bMessageType = TRUE; // Reponse Message
                  stMessageHeader->iReponseID = ((OEM4BinaryReponseData*)*pcMessageBuffer)->iResponseID;
                  if(stMessageHeader->iReponseID != 1)
                  {
                     stMessageHeader->bErrorResponse = TRUE;
                  }
               }

               // Discard the processed data
               cMyCircularDataBuffer.Discard(uiBinaryMessageLength);

               // Make byte count 0
               uiMyByteCount = 0;

               // Set header information
               if (eMyParseStatus == WAITING_FOR_BINARY_BODY_AND_CRC)
               {
                  stMessageHeader->uiMessageID         = sOEM4BinaryHeader.usMsgNumber;
                  stMessageHeader->eMessageFormat      = MessageFormatEnum::MESSAGE_BINARY;
                  stMessageHeader->eMessageTimeStatus  = (MessageTimeStatusEnum) sOEM4BinaryHeader.ucTimeStatus;
                  stMessageHeader->uiMessageLength     = uiBinaryMessageLength;
                  stMessageHeader->ulMessageWeek       = sOEM4BinaryHeader.usWeekNo;
                  stMessageHeader->ulMessageTime       = sOEM4BinaryHeader.ulWeekMSec;
                  stMessageHeader->ulMessageDefCrc     = sOEM4BinaryHeader.usMsgDefCRC;
                  stMessageHeader->ulReceiverStatus    = sOEM4BinaryHeader.ulStatus;
                  stMessageHeader->ulReceiverSWVersion = sOEM4BinaryHeader.usReceiverSWVersion;
                  stMessageHeader->dIdleTime           = (DOUBLE) sOEM4BinaryHeader.ucIdleTime;
                  stMessageHeader->dIdleTime          *= 0.5;
                  stMessageHeader->ucPortAddress        = sOEM4BinaryHeader.ucPort;
				  stMessageHeader->ulSequenceNumber     = sOEM4BinaryHeader.usSequenceNumber;
                  // Antenna Source is the 0th bit in Message Type. If bit 0 is set, it's from Secondary Antenna
                  stMessageHeader->eMessageSource      = (MessageAntennaSourceEnum) (sOEM4BinaryHeader.ucMsgType & 0x01);
                  if (stMessageHeader->eMessageSource == MessageAntennaSourceEnum::SECONDARY_ANTENNA)
                  {
                     stMessageHeader->szMessageName.append("_1");
                  }
               }
               else if (eMyParseStatus == WAITING_FOR_SHORT_BINARY_BODY_AND_CRC)
               {
                  stMessageHeader->uiMessageID     = sOEM4BinaryShortHeader.usMessageId;
                  stMessageHeader->eMessageFormat  = MessageFormatEnum::MESSAGE_SHORT_HEADER_BINARY;
                  stMessageHeader->uiMessageLength = uiBinaryMessageLength;
                  stMessageHeader->ulMessageWeek   = sOEM4BinaryShortHeader.usWeekNo;
                  stMessageHeader->ulMessageTime   = sOEM4BinaryShortHeader.ulWeekMSec;
               }

               eMyParseStatus = COMPLETE_MESSAGE;
            }
            else
            {
               // Invalid CRC. Discard first byte and proceed
               HandleInvalidData();
               // Set format to indicate this as unknwon binary message
               stMessageHeader->eMessageFormat = MessageFormatEnum::MESSAGE_BINARY;
               bGotSync = FALSE;
            }
         }
         break;

      case WAITING_FOR_ASCII_BODY:        //Intentional fall-through
      case WAITING_FOR_SHORT_ASCII_BODY:
         if ((ucDataByte == '\n') || (uiMyByteCount >= MAX_ASCII_MESSAGE_LENGTH))
         {
            if (CalculateBlockCrc(uiMyByteCount))
            {
               // Got complete ASCII message with valid CRC
               // This memory will be deleted in BaseMessageData destructor
               *pcMessageBuffer = new CHAR[uiMyByteCount+1];
               memset(*pcMessageBuffer, '\0', uiMyByteCount+1);

               if (*pcMessageBuffer == NULL)
                  throw "Error while creating memory for ascii message";

               // Copy the message to message buffer
               cMyCircularDataBuffer.Copy((UCHAR*)*pcMessageBuffer, uiMyByteCount);

               // Extract Header String from Complete message
               if ((eMyParseStatus == WAITING_FOR_ASCII_BODY) &&
                  (ExtractAsciiHeader(*pcMessageBuffer, stMessageHeader) == TRUE))
               {
                  // Discard the processed data
                  cMyCircularDataBuffer.Discard(uiMyByteCount);
                  eMyParseStatus = COMPLETE_MESSAGE;
               }
               else if ((eMyParseStatus == WAITING_FOR_SHORT_ASCII_BODY) &&
                  (ExtractShortAsciiHeader(*pcMessageBuffer, stMessageHeader) == TRUE))
               {
                  // Discard the processed data
                  cMyCircularDataBuffer.Discard(uiMyByteCount);
                  eMyParseStatus = COMPLETE_MESSAGE;
               }
               else
               {
                  // Unknown message
                  HandleInvalidData();
                  delete *pcMessageBuffer;
                  *pcMessageBuffer = NULL;
                  bGotSync = FALSE;
               }

               // Make byte count 0
               uiMyByteCount = 0;
            }
            else
            {
               // Invalid CRC. Discard first byte and proceed
               HandleInvalidData();
               // Set format to indicate this as unknwon ascii message
               stMessageHeader->eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
               bGotSync = FALSE;
            }
         }
         break;

      case WAITING_FOR_NMEA_BODY:
         if ((ucDataByte == '\n') || (uiMyByteCount >= MAX_NMEA_MESSAGE_LENGTH))
         {
            if (ValidateNMEAChecksum(uiMyByteCount))
            {
               // Got complete NMEA message with valid CRC
               // This memory will be deleted in BaseMessageData destructor
               *pcMessageBuffer = new CHAR[uiMyByteCount+1];
               memset(*pcMessageBuffer, '\0', uiMyByteCount+1);

               if (*pcMessageBuffer == NULL)
                  throw "Error while creating memory for ascii message";

               // Copy the message to message buffer
               cMyCircularDataBuffer.Copy((UCHAR*)*pcMessageBuffer, uiMyByteCount);

               // Extract string from Complete message
               std::string szNMEAMessage(*pcMessageBuffer);
               UINT uiMessageStart = static_cast<UINT>(szNMEAMessage.find('$'));
               UINT uiMessageEnd = static_cast<UINT>(szNMEAMessage.find('*'));
               std::string szMessage = szNMEAMessage.substr(uiMessageStart+1, uiMessageEnd-uiMessageStart-1);

               // Extract values from message string
               std::vector<std::string> aszMessageParameters;
               std::stringstream szMessageStream(szMessage);
               while (szMessageStream.good())
               {
                  std::string szParameter;
                  std::getline(szMessageStream, szParameter, ',');
                  aszMessageParameters.push_back(szParameter);
               }

               // Fill Message Header
			   stMessageHeader->szMessageName = aszMessageParameters[0];
               stMessageHeader->eMessageFormat     = MessageFormatEnum::MESSAGE_ASCII;
               stMessageHeader->uiMessageLength    = uiMyByteCount;
               stMessageHeader->bIsNMEAMessage     = TRUE;

               // Discard the processed data
               cMyCircularDataBuffer.Discard(uiMyByteCount);
               eMyParseStatus = COMPLETE_MESSAGE;

               // Make byte count 0
               uiMyByteCount = 0;
            }
            else
            {
               // Invalid CRC. Discard first byte and proceed
               HandleInvalidData();
               // Set format to indicate this as unknwon ascii message
               stMessageHeader->eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
               bGotSync = FALSE;
            }
         }
         break;
      case WAITING_FOR_ABB_ASCII_BODY:
         if((ucDataByte == '\n') || (uiMyByteCount >= MAX_ABB_ASCII_RESPONSE_LENGTH))
         {
            *pcMessageBuffer = new CHAR[uiMyByteCount+1];
            memset(*pcMessageBuffer, '\0', uiMyByteCount+1);

            if (*pcMessageBuffer == NULL)
               throw "Error while creating memory for ascii message";

            // Copy the message to message buffer
            cMyCircularDataBuffer.Copy((UCHAR*)*pcMessageBuffer, uiMyByteCount);

            if(!strncmp(*pcMessageBuffer, "<OK", 3) ||
               !strncmp(*pcMessageBuffer, "<ERROR:", 7))
            {
               stMessageHeader->bMessageType = TRUE; // Reponse
               stMessageHeader->eMessageFormat = MessageFormatEnum::MESSAGE_ABB_ASCII;
               stMessageHeader->uiMessageLength = uiMyByteCount;
			   stMessageHeader->szMessageName = "UNKNOWN";
               // Discard the processed data
               cMyCircularDataBuffer.Discard(uiMyByteCount);
               eMyParseStatus = COMPLETE_MESSAGE;

               // Make byte count 0
               uiMyByteCount = 0;
            }
            else // Other than OK/ERROR
            {
			   stMessageHeader->bMessageType = TRUE; // Reponse
               stMessageHeader->eMessageFormat = MessageFormatEnum::MESSAGE_ABB_ASCII;
               // Invalid response. Discard first byte and proceed
               HandleInvalidData();
               bGotSync = FALSE;
            }
         }
         break;

      default:
         throw "Invalid parsing state in decoder";
         break;
      }
   }
   return eMyStreamReadStatus;
}

// -------------------------------------------------------------------------------------------------------
void NovatelParser::HandleInvalidData(void)
{
   eMyParseStatus = WAITING_FOR_SYNC;
   vcMyUnknownBytes.push_back(cMyCircularDataBuffer.GetByte(0));
   cMyCircularDataBuffer.Discard(1);
   uiMyByteCount = 0;
}

// -------------------------------------------------------------------------------------------------------
BOOL NovatelParser::ExtractAsciiHeader(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader)
{
   // Extract Header String from Complete message
   std::string szAsciiMessage(pcMessageBuffer);
   UINT uiHeaderStart = static_cast<UINT>(szAsciiMessage.find('#'));
   UINT uiHeaderEnd = static_cast<UINT>(szAsciiMessage.find(';'));
   std::string szHeader = szAsciiMessage.substr(uiHeaderStart+1, uiHeaderEnd-uiHeaderStart-1);

   // Extract header values from header string
   std::vector<std::string> aszHeaderParameters;
   std::stringstream szHeaderStream(szHeader);
   while (szHeaderStream.good())
   {
      std::string szParameter;
      std::getline(szHeaderStream, szParameter, ',');
      aszHeaderParameters.push_back(szParameter);
   }

   // Check whether we received all header parameters (excluding sync byte) or not
   if (aszHeaderParameters.size() != ASCIIHeaderEnum::NUMBER_OF_ASCII_HEADER_ELEMENTS)
      return FALSE;

   // Set header information
   INT iMessageID = 0;
   std::string szMessageName = aszHeaderParameters[ASCIIHeaderEnum::ASCII_MESSAGE_NAME];
   if(szMessageName.at(szMessageName.size() -1) == 'R') //It is ASCII response
   {
      stMessageHeader->bMessageType = TRUE;
   }

   if (iMessageID != 0 || szMessageName != "")
   {
      stMessageHeader->uiMessageID         = iMessageID;
	   if (szMessageName.find('_') != -1)
	   {
		   szMessageName = szMessageName.substr(0, (szMessageName.find('_')));
	   }
	   else
	   {
		   szMessageName = szMessageName.substr(0, (szMessageName.size() - 1));
	   }
	   stMessageHeader->szMessageName = szMessageName;
      stMessageHeader->eMessageFormat      = MessageFormatEnum::MESSAGE_ASCII;
      stMessageHeader->eMessageTimeStatus  = mTimeStatusMap[aszHeaderParameters[ASCIIHeaderEnum::ASCII_TIME_STATUS]];
      stMessageHeader->uiMessageLength     = uiMyByteCount;
      stMessageHeader->ulMessageWeek       = stoul(aszHeaderParameters[ASCIIHeaderEnum::ASCII_WEEK]);
      stMessageHeader->ulReceiverSWVersion = stoul(aszHeaderParameters[ASCIIHeaderEnum::ASCII_RECEIVER_SW_VERSION]);
      stMessageHeader->dIdleTime           = stod(aszHeaderParameters[ASCIIHeaderEnum::ASCII_IDLETIME]);
      // Open Source Decoder Changes - Need to replace this with the methods from JSON interface
      //stMessageHeader->eMessagePort        = (PortAddressEnum)GetEnumValue("PortAddressEnum",aszHeaderParameters[ASCIIHeaderEnum::ASCII_PORT]);
      StringToHexULong(aszHeaderParameters[ASCIIHeaderEnum::ASCII_RESERVED].c_str(),&(stMessageHeader->ulMessageDefCrc));
      StringToHexULong(aszHeaderParameters[ASCIIHeaderEnum::ASCII_RECEIVER_STATUS].c_str(),&(stMessageHeader->ulReceiverStatus));
	  StringToULong(aszHeaderParameters[ASCIIHeaderEnum::ASCII_SEQUENCE].c_str(),&(stMessageHeader->ulSequenceNumber));
      // Check for message antenna source. Message name will contain _1 (ex: RANGE_1) for secondary antenna
      if (szMessageName.find("_1") != std::string::npos)
      {
         stMessageHeader->eMessageSource  = MessageAntennaSourceEnum::SECONDARY_ANTENNA;
         stMessageHeader->szMessageName.append("_1");
      }
      else
      {
         stMessageHeader->eMessageSource  = MessageAntennaSourceEnum::PRIMARY_ANTENNA;
      }

      // ASCII message will give time in seconds in the form of 123456.123
      // here we extract integer and fraction values and then convert time to milliseconds
      // to be consistent with binary where we get time in milliseconds
      int uiMessageTimeInteger = 0, uiMessageTimeFraction = 0;
      sscanf(aszHeaderParameters[ASCIIHeaderEnum::ASCII_SECONDS].c_str(), "%d.%d", &uiMessageTimeInteger, &uiMessageTimeFraction);
      stMessageHeader->ulMessageTime = (ULONG)(uiMessageTimeInteger * SECONDS_TO_MILLISECONDS) + uiMessageTimeFraction;

      eMyParseStatus = COMPLETE_MESSAGE;

      return TRUE;
   }
   return FALSE;

}

// -------------------------------------------------------------------------------------------------------
BOOL NovatelParser::ExtractShortAsciiHeader(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader)
{
   // Extract Header String from Complete message
   std::string szAsciiMessage(pcMessageBuffer);
   UINT uiHeaderStart = static_cast<UINT>(szAsciiMessage.find('%'));
   UINT uiHeaderEnd = static_cast<UINT>(szAsciiMessage.find(';'));
   std::string szHeader = szAsciiMessage.substr(uiHeaderStart+1, uiHeaderEnd-uiHeaderStart-1);

   // Extract header values from header string
   std::vector<std::string> aszHeaderParameters;
   std::stringstream szHeaderStream(szHeader);
   while (szHeaderStream.good())
   {
      std::string szParameter;
      std::getline(szHeaderStream, szParameter, ',');
      aszHeaderParameters.push_back(szParameter);
   }

   // Check whether we received all header parameters (excluding sync byte) or not
   if (aszHeaderParameters.size() != ShortASCIIHeaderEnum::NUMBER_OF_SHORT_ASCII_HEADER_ELEMENTS)
      return FALSE;

   // Set header information
   std::string szMessageName = aszHeaderParameters[ShortASCIIHeaderEnum::SHORT_ASCII_MESSAGE_NAME];
   if(szMessageName.at(szMessageName.size() -1) == 'R') //It is ASCII response
   {
      szMessageName = szMessageName.substr (0,(szMessageName.size()-1));
      stMessageHeader->bMessageType = TRUE;
   }
   if (szMessageName.find('_') != -1)
   {
      szMessageName = szMessageName.substr(0, (szMessageName.find('_')));
   }
   else
   {
      szMessageName = szMessageName.substr(0, (szMessageName.size() - 1));
   }
   INT iMessageID =0;
   if (iMessageID != 0 || szMessageName != "")
   {
      stMessageHeader->szMessageName = szMessageName;
	  stMessageHeader->eMessageFormat = MessageFormatEnum::MESSAGE_SHORT_HEADER_ASCII;
      stMessageHeader->uiMessageLength = uiMyByteCount;
      stMessageHeader->ulMessageWeek = stoul(aszHeaderParameters[ShortASCIIHeaderEnum::SHORT_ASCII_WEEK]);

      // ASCII message will give time in seconds in the form of 123456.123
      // here we extract integer and fraction values and then convert time to milliseconds
      // to be consistent with binary where we get time in milliseconds
      int uiMessageTimeInteger = 0, uiMessageTimeFraction = 0;
      sscanf(aszHeaderParameters[ShortASCIIHeaderEnum::SHORT_ASCII_SECONDS].c_str(), "%d.%d", &uiMessageTimeInteger, &uiMessageTimeFraction);
      stMessageHeader->ulMessageTime = (ULONG)(uiMessageTimeInteger * SECONDS_TO_MILLISECONDS) + uiMessageTimeFraction;

      eMyParseStatus = COMPLETE_MESSAGE;

      return TRUE;
   }

   return FALSE;
}

// -------------------------------------------------------------------------------------------------------
UINT NovatelParser::CalculateCharacterCrc(UCHAR uCharacter, UINT uiCrcCalculated)
{
   // If CRC Check Skip enabled, return 0
   if (bMySkipCrcCheck == TRUE)
   {
      return 0;
   }

   // Calculates the CRC-32 of a block of data one character for each call
   CRC32 clCrc;
   return clCrc.CalculateCharacterCRC32(uiCrcCalculated, uCharacter);
}

// -------------------------------------------------------------------------------------------------------
BOOL NovatelParser::CalculateBlockCrc(UINT uiNumberOfBytes)
{
   // If CRC Check Skip enabled, return TRUE
   if (bMySkipCrcCheck == TRUE)
   {
      return TRUE;
   }

   // Calculates the CRC-32 of a block of data
   UINT uiCalculatedCRC = 0;
   UINT uiMessageCRC = 0;
   INT   iTerminatorIndex = 0;
   iTerminatorIndex = uiNumberOfBytes - (OEM4_ASCII_CRC_LENGTH + 3);

   if(iTerminatorIndex <= 0)
      return FALSE;

   for(INT i = 1; i < iTerminatorIndex; i++)
   {
      if(cMyCircularDataBuffer[i] == '\0')
         break;
      uiCalculatedCRC = CalculateCharacterCrc(cMyCircularDataBuffer[i],uiCalculatedCRC);
   }

   // get the crc from the message
   char szCrc[OEM4_ASCII_CRC_LENGTH+1];
   for(UINT i = 0; i < OEM4_ASCII_CRC_LENGTH; i++)
   {
      if(cMyCircularDataBuffer[iTerminatorIndex+i+1] == '\0')
      {
         break;
      }
      szCrc[i] = cMyCircularDataBuffer[iTerminatorIndex+i+1];
   }
   szCrc[OEM4_ASCII_CRC_LENGTH] = '\0';
   sscanf( szCrc, "%x", &uiMessageCRC );
   return (uiCalculatedCRC == uiMessageCRC);
}

// -------------------------------------------------------------------------------------------------------
BOOL NovatelParser::ValidateNMEAChecksum(UINT uiNumberOfBytes)
{
   INT iCalculatedCRC = 0;
   INT iTerminatorIndex = 0;
   INT iByteIndex;
   iTerminatorIndex = uiNumberOfBytes - (NMEA_MESSAGE_CRC_LENGTH + 3);

   if(iTerminatorIndex <= 0)
      return FALSE;

   for(iByteIndex = 1; iByteIndex < iTerminatorIndex; iByteIndex++)
   {
      iCalculatedCRC ^= cMyCircularDataBuffer[iByteIndex];
   }

   // After claculating checksum, next characters has to be "*".
   if(cMyCircularDataBuffer[iByteIndex] == '*')
   {
      INT iUpperChecksumDigit = HexToInt(cMyCircularDataBuffer[iByteIndex+1]);
      INT iLowerChecksumDigit = HexToInt(cMyCircularDataBuffer[iByteIndex+2]);
      INT iMessageCRC = iUpperChecksumDigit << 4 | iLowerChecksumDigit;

      if(iMessageCRC == iCalculatedCRC)
      {
         return TRUE;
      }
   }

   return FALSE;
}

// -------------------------------------------------------------------------------------------------------
INT NovatelParser::HexToInt(CHAR cValue)
{
   if (cValue >= '0' && cValue <= '9')
      return cValue - '0';

   if (cValue >= 'A' && cValue <= 'F')
      return cValue - 'A' + 10;

   if (cValue >= 'a' && cValue <= 'f')
      return cValue - 'a' + 10;

   return -1;
}

// -------------------------------------------------------------------------------------------------------
UINT NovatelParser::ReadInputStream(void)
{
   // Read data from input stream
   if (pclMyInputStreamInterface != NULL)
   {
      ReadDataStructure stReadStream;
      stReadStream.cData = new CHAR[READ_BUFFER_SIZE];
      memset(stReadStream.cData, '\0', READ_BUFFER_SIZE);

      if (stReadStream.cData == NULL)
         throw "Can't allocate memory for read buffer";

      stReadStream.uiDataSize = READ_BUFFER_SIZE;

      // read data from input stream
      eMyStreamReadStatus = pclMyInputStreamInterface->ReadData(stReadStream);

      // append the data read from file to praser buffer
      // if Input file size is less then READ_BUFFER_SIZE, then process only those bytes.
      // append, only non callback data in case of port.
      if ((eMyStreamReadStatus.uiCurrentStreamRead > 0) && (pclMyInputStreamInterface->IsCallBackEnable() != TRUE))
      {
         cMyCircularDataBuffer.Append((UCHAR*)stReadStream.cData, eMyStreamReadStatus.uiCurrentStreamRead);
      }

      // Even though we reached end of file, wait till we process the current buffer
      // to report that end of file status
      if (eMyStreamReadStatus.bEOS == TRUE)
      {
         bIsInputStreamEOF = TRUE;
         eMyStreamReadStatus.bEOS = FALSE;
      }

      delete[] stReadStream.cData;
      stReadStream.cData = NULL;

      // return the byte count read
      return eMyStreamReadStatus.uiCurrentStreamRead;
   }

   return 0;
}

// -------------------------------------------------------------------------------------------------------
void NovatelParser::SetCrcCheckFlag(BOOL bEnable)
{
   bMySkipCrcCheck = bEnable;
}

// -------------------------------------------------------------------------------------------------------
CircularBuffer* NovatelParser::getCircularBuffer(void)
{
   return &cMyCircularDataBuffer;
}

// -------------------------------------------------------------------------------------------------------
StreamReadStatus* NovatelParser::getStreamReadStatus(void)
{
   return &eMyStreamReadStatus;
}

// -------------------------------------------------------------------------------------------------------
void NovatelParser::Reset()
{
   bIsInputStreamEOF = FALSE;
   uiMyByteCount = 0;
   cMyCircularDataBuffer.Clear();
}

// -------------------------------------------------------------------------------------------------------
NovatelParser::~NovatelParser()
{

}
