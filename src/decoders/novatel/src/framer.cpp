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
//    Class to provide framer functionality
//
////////////////////////////////////////////////////////////////////////////////
#include "framer.hpp"

// -------------------------------------------------------------------------------------------------------
Framer::Framer(InputStreamInterface* pclInputStreamInterface)
{
   bMyEnableUnknownData = TRUE;

   bMyNonBlockingMode = FALSE;

   eMyBMDOutputFormate = BMDOutputFormat::BOTH;

   pclMyInputStreamInterface = pclInputStreamInterface;

   if (pclMyInputStreamInterface == NULL)
      throw nExcept("Null Input Stream Interface Pointer");

   // Instantiate parser
   pclMyNovatelParser = std::unique_ptr<NovatelParser>(new NovatelParser(pclMyInputStreamInterface));
   if (pclMyNovatelParser == NULL)
      throw nExcept("Failed to instantiate parser");

   // Instantiate message counter
   pclMyMessageCounter = std::unique_ptr<MessageCounter>(new MessageCounter());
   if (pclMyMessageCounter == NULL)
      throw nExcept("Failed to instantiate message counter");

   NovatelParser* ptrNovatelParser = pclMyNovatelParser.get();
   pclMyInputStreamInterface->RegisterCallBack(ptrNovatelParser);

   // Instantiate UnknownData class
   pclMyUnknownDataHandler = std::unique_ptr<UnknownDataHandler>(new UnknownDataHandler());
   if (pclMyUnknownDataHandler == NULL)
      throw nExcept("Failed to instantiate UnknownDataHandler");

   pclMyMessageDataFilter = NULL;

}

// -------------------------------------------------------------------------------------------------------
Framer::Framer(InputStreamInterface* pclInputStreamInterface, MessageDataFilter& rMessageDataFilter)
{
   bMyEnableUnknownData = TRUE;

   bMyNonBlockingMode = FALSE;

   eMyBMDOutputFormate = BMDOutputFormat::BOTH;

   pclMyInputStreamInterface = pclInputStreamInterface;

   if (pclMyInputStreamInterface == NULL)
      throw nExcept("Null Input Stream Interface Pointer");

   // Instantiate parser
   pclMyNovatelParser = std::unique_ptr<NovatelParser>(new NovatelParser(pclMyInputStreamInterface));
   if (pclMyNovatelParser == NULL)
      throw nExcept("Failed to instantiate parser");

   // Instantiate message counter
   pclMyMessageCounter = std::unique_ptr<MessageCounter>(new MessageCounter());
   if (pclMyMessageCounter == NULL)
      throw nExcept("Failed to instantiate message counter");

   NovatelParser* ptrNovatelParser = pclMyNovatelParser.get();
   pclMyInputStreamInterface->RegisterCallBack(ptrNovatelParser);

   // Instantiate UnknownData class
   pclMyUnknownDataHandler = std::unique_ptr<UnknownDataHandler>(new UnknownDataHandler());
   if (pclMyUnknownDataHandler == NULL)
      throw nExcept("Failed to instantiate UnknownDataHandler");

   pclMyMessageDataFilter = &rMessageDataFilter;

}

// -------------------------------------------------------------------------------------------------------
Framer::Framer()
{
}

// -------------------------------------------------------------------------------------------------------
void Framer::EnableUnknownData(BOOL bEnable)
{
   bMyEnableUnknownData = bEnable;
}

// -------------------------------------------------------------------------------------------------------
void Framer::SetBMDOutput(BMDOutputFormat eMyBMDoutput)
{
	eMyBMDOutputFormate = eMyBMDoutput;
}

// -------------------------------------------------------------------------------------------------------
BMDOutputFormat Framer::GetBMDOutput(void)
{
	return eMyBMDOutputFormate;
}

// -------------------------------------------------------------------------------------------------------
void Framer::SkipCRCValidation(BOOL bEnable)
{
   pclMyNovatelParser->SetCrcCheckFlag(bEnable);
}

// -------------------------------------------------------------------------------------------------------
void Framer::EnableNonBlockingMode(BOOL bEnable)
{
   bMyNonBlockingMode = bEnable;
}

// -------------------------------------------------------------------------------------------------------
void Framer::EnableTimeIssueFix(BOOL bEnable)
{
   pclMyMessageCounter->EnableTimeIssueFix(bEnable);
}

// -------------------------------------------------------------------------------------------------------
StreamReadStatus Framer::ReadMessage(BaseMessageData** pclBaseMessageData)
{
   StreamReadStatus stStreamReadStatus;

   // Make use of parser to read data from stream, identify and return a message
   do
   {
      MessageHeader stMessageHeader;
      CHAR* pcMessageBuffer;

      stStreamReadStatus = pclMyNovatelParser->ParseData(&pcMessageBuffer, &stMessageHeader);

	  pclMyInputStreamInterface->SetCurrentFileOffset(stMessageHeader.uiMessageLength);

      if ((stMessageHeader.uiMessageLength != 0) && (pcMessageBuffer != NULL))
      {
         // If UNKNOWN message, collect unknown data statistics
         if (stMessageHeader.uiMessageID == 0 && stMessageHeader.szMessageName == "UNKNOWN")
         {
            pclMyUnknownDataHandler->HandleUnknownData(pcMessageBuffer, &stMessageHeader, stStreamReadStatus.bEOS);
         }

         // Don't send UNKNOWN messages if not enabled
         if ((stMessageHeader.uiMessageID == 0) && (stMessageHeader.szMessageName == "UNKNOWN") && (bMyEnableUnknownData == FALSE))
         {
            delete [] pcMessageBuffer;    // delete message data created in parser
            continue;
         }

         // Generate BaseMessagedata
         GenerateBaseMessageData(pclBaseMessageData, &stMessageHeader, pcMessageBuffer);

         // Update message count before filtering for information
         pclMyMessageCounter->CountMessage(&stMessageHeader);

		 // Update json header string before filtering for information
		 stMessageHeader.uiMessageID = (*pclBaseMessageData)->getMessageID();
		 stMessageHeader.szMessageName = (*pclBaseMessageData)->getMessageName();
		 json jheader = json::object();
		 Header_json(jheader, &stMessageHeader);
		 (*pclBaseMessageData)->setHeaderjsonstring(jheader.dump());
		 jheader.clear();

         // When decoder instance is created without filter or filtering condition satisfied.
         if ((pclMyMessageDataFilter == NULL) || (pclMyMessageDataFilter->Filter(**pclBaseMessageData) == TRUE))
         {
            pclMyMessageCounter->AddNewMessage(&stMessageHeader);
         }
         else
         {
            delete *pclBaseMessageData;
            *pclBaseMessageData = NULL;
         }
      }
   // return to application on
   // 1. BaseMessageData is not empty
   // 2. Stream End
   // 3. 0 bytes read from stream
   // 4. Non-Blocking mode enabled
   }while ((*pclBaseMessageData == NULL) &&
           (stStreamReadStatus.bEOS == FALSE) &&
           (stStreamReadStatus.uiCurrentStreamRead != 0) &&
           (bMyNonBlockingMode == FALSE));

   return stStreamReadStatus;
}
void Framer:: Header_json(json& jHeader, MessageHeader* pstMessageHeader)
{
	jHeader = json{
				{"MessageName", pstMessageHeader->szMessageName},
				{"MessageId", pstMessageHeader->uiMessageID},
				{"MessageFormat", pstMessageHeader->eMessageFormat},
				{"MessageLength", pstMessageHeader->uiMessageLength},
				{"MessageTime", pstMessageHeader->ulMessageTime},
				{"MessageTimeStatus", pstMessageHeader->eMessageTimeStatus},
				{"MessageSource", pstMessageHeader->eMessageSource},
				{"port", pstMessageHeader->ucPortAddress},
				{"MessageWeek", pstMessageHeader->ulMessageWeek},
				{"SequenceNumber", pstMessageHeader->ulSequenceNumber},
				{"ReceiverStatus", pstMessageHeader->ulReceiverStatus},
				{"ReceiverSWVersion", pstMessageHeader->ulReceiverSWVersion},
				{"ReponseID", pstMessageHeader->iReponseID},
				{"IdleTime", pstMessageHeader->dIdleTime},
	};
}
// -------------------------------------------------------------------------------------------------------
void Framer::GenerateBaseMessageData(BaseMessageData** pclBaseMessageData, MessageHeader* stMessageHeader, CHAR* pcData)
{
   if(stMessageHeader->bMessageType == TRUE && (stMessageHeader->eMessageFormat != MESSAGE_BINARY))
   {
      std::string szResponse = pcData;
      INT iPos1 = 0;
      INT iPos2 = 0;

      if(stMessageHeader->eMessageFormat == MESSAGE_ABB_ASCII) // Abb Ascii Reponse
      {
         iPos1 = (INT)szResponse.find("<");
         iPos2 = (INT)szResponse.find("\r\n");
      }
      else if(stMessageHeader->eMessageFormat == MESSAGE_ASCII) // Ascii Reponse
      {
         iPos1 = (INT)szResponse.find(";");
         iPos2 = (INT)szResponse.find("*");
      }
      else
      {
         return;
      }
      szResponse = szResponse.substr(iPos1+1, iPos2-iPos1-1);
      if(szResponse != "OK")
         stMessageHeader->bErrorResponse = TRUE;

      memset(pcData, '\0',stMessageHeader->uiMessageLength);
      memcpy( pcData, const_cast<char *>(szResponse.c_str()), szResponse.size() );
      stMessageHeader->uiMessageLength = (UINT)szResponse.size();
   }

   // Application has to delete this memory
   *pclBaseMessageData = new BaseMessageData(stMessageHeader, pcData);
}

// -------------------------------------------------------------------------------------------------------
std::map<std::string, MessageInfo> Framer::GetAsciiMessageStatistics() const
{
   return pclMyMessageCounter->GetAsciiMessageStatistics();
}

// -------------------------------------------------------------------------------------------------------
std::map<UINT, MessageInfo> Framer::GetBinaryMessageStatistics() const
{
   return pclMyMessageCounter->GetBinaryMessageStatistics();
}

// -------------------------------------------------------------------------------------------------------
std::map<std::string, MessageInfo> Framer::GetAsciiMessageStatisticsWithoutFilter() const
{
   return pclMyMessageCounter->GetAsciiMessageStatisticsWithoutFilter();
}

// -------------------------------------------------------------------------------------------------------
std::map<UINT, MessageInfo> Framer::GetBinaryMessageStatisticsWithoutFilter() const
{
   return pclMyMessageCounter->GetBinaryMessageStatisticsWithoutFilter();
}

// -------------------------------------------------------------------------------------------------------
DecoderStatistics Framer::GetDecoderStatistics() const
{
   return pclMyMessageCounter->GetDecoderStatistics();
}

// -------------------------------------------------------------------------------------------------------
UnknownDataStatistics Framer::GetUnknownDataStatistics() const
{
   return pclMyUnknownDataHandler->GetUnknownDataStatistics();
}

// -------------------------------------------------------------------------------------------------------
void Framer::Reset(std::streamoff offset, std::ios_base::seekdir dir)
{
   if(pclMyNovatelParser != NULL)
   {
      pclMyNovatelParser->Reset();
   }

   if(pclMyMessageCounter != NULL)
   {
      pclMyMessageCounter->Reset();
   }

   if(pclMyMessageDataFilter != NULL)
   {
      pclMyMessageDataFilter->Reset();
   }

   if(pclMyUnknownDataHandler != NULL)
   {
      pclMyUnknownDataHandler->Reset();
   }

   if(pclMyInputStreamInterface != NULL)
   {
      pclMyInputStreamInterface->Reset(offset, dir);
   }
}
// -------------------------------------------------------------------------------------------------------
ULONGLONG Framer::GetCurrentFileOffset(void) const
{
   return pclMyInputStreamInterface->GetCurrentFileOffset();
}

// -------------------------------------------------------------------------------------------------------
Framer::~Framer()
{
   pclMyMessageDataFilter = NULL;
   pclMyInputStreamInterface = NULL;
}
