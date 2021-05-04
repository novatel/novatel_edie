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
#include "decoder.hpp"
#include "encoder.hpp"

// -------------------------------------------------------------------------------------------------------
Decoder::Decoder(InputStreamInterface* pclInputStreamInterface) : Framer(pclInputStreamInterface)
{
   dbData = loadDataFromJSON::getDatabaseObj();
}

// -------------------------------------------------------------------------------------------------------
Decoder::Decoder(InputStreamInterface* pclInputStreamInterface,MessageDataFilter& rMessageDataFilter) : Framer(pclInputStreamInterface, rMessageDataFilter)
{
   dbData = loadDataFromJSON::getDatabaseObj();
}

// -------------------------------------------------------------------------------------------------------
void Decoder::GenerateBaseMessageData(BaseMessageData** pclBaseMessageData, MessageHeader* stMessageHeader, CHAR* pcData)
{
	std::string szUniqeKey;
	if (dbData != NULL)
	{
		
		*pclBaseMessageData = new BaseMessageData(stMessageHeader, pcData);
		if ((*pclBaseMessageData)->getMessageFormat() == MESSAGE_BINARY 
			|| (*pclBaseMessageData)->getMessageFormat() == MESSAGE_SHORT_HEADER_BINARY)
		{
			szUniqeKey = dbData->getMessageName((*pclBaseMessageData)->getMessageID());
			if (szUniqeKey == "")
				return;
			if ((*pclBaseMessageData)->getMessageLength() <= 32)
				return;
			(*pclBaseMessageData)->setMessageName(szUniqeKey.substr(0, szUniqeKey.find("_")));
			CHAR* ppLogBody = (*pclBaseMessageData)->getMessageData();
         if ((*pclBaseMessageData)->getMessageFormat() == MESSAGE_BINARY)
         {
            ppLogBody += OEM4_BINARY_HEADER_LENGTH;
         }
         else if ((*pclBaseMessageData)->getMessageFormat() == MESSAGE_SHORT_HEADER_BINARY)
         {
            ppLogBody += OEM4_BINARY_SHORT_HEADER_LENGTH;
         }
			pclMyLogUtils = new LogUtils(dbData, szUniqeKey);
			CHAR* pGlobalOutBuf;

			pGlobalOutBuf = new CHAR[20480];
			memset(pGlobalOutBuf, '\0', 20480);
			if (pGlobalOutBuf == NULL)
				throw "Can't allocate memory for read buffer";

			char* pOutBuf = NULL;
			pOutBuf = pGlobalOutBuf;
			if (GetBMDOutput() == FLATTEN || GetBMDOutput() == BOTH)
			{
				pclMyLogUtils->flatten_binary_log(&ppLogBody, &pOutBuf);
				CHAR* FlatenBuf = new CHAR[pclMyLogUtils->getFlattenbinarylength()];
				memset(FlatenBuf, '\0', pclMyLogUtils->getFlattenbinarylength());
				if (FlatenBuf == NULL)
					throw "Can't allocate memory for read buffer";

				memcpy(FlatenBuf, pGlobalOutBuf, pclMyLogUtils->getFlattenbinarylength());

				(*pclBaseMessageData)->setFlattenMessageData(FlatenBuf);
				(*pclBaseMessageData)->setFlattenMessageLength(pclMyLogUtils->getFlattenbinarylength());
			}
			if (GetBMDOutput() == JSON || GetBMDOutput() == BOTH)
			{
				std::string jsonmessage = pclMyLogUtils->getJSONString(&ppLogBody);
				(*pclBaseMessageData)->setMessagejsonstring(jsonmessage);
			}
			
			Format type = ASCII;
			Encoder   *pEncoder = new Encoder();
			pEncoder->WriteMessage(*pclBaseMessageData, type);
			(*pclBaseMessageData)->setMessageFormat(MESSAGE_BINARY);
			
			delete pGlobalOutBuf;
			pGlobalOutBuf = NULL;
			delete pEncoder;
			pEncoder = NULL;
			delete pclMyLogUtils;
			pclMyLogUtils = NULL;
		}
		else if ((*pclBaseMessageData)->getMessageFormat() == MESSAGE_ASCII
         || (*pclBaseMessageData)->getMessageFormat() == MESSAGE_SHORT_HEADER_ASCII)
		{
			UINT messageid = 0;
			Format type = BINARY;
			messageid = dbData->getMessageId((*pclBaseMessageData)->getMessageName());
			(*pclBaseMessageData)->setMessageID(messageid);
			szUniqeKey = dbData->getMessageName((*pclBaseMessageData)->getMessageID());
			if (szUniqeKey == "")
				return;
			if ((*pclBaseMessageData)->getMessageLength() <= 32)
				return;
			Encoder   *pEncoder = new Encoder();
			pEncoder->WriteMessage(*pclBaseMessageData , type);
			pclMyLogUtils = new LogUtils(dbData, szUniqeKey);
			CHAR* ppLogBody = (*pclBaseMessageData)->getMessageData();
         if ((*pclBaseMessageData)->getMessageFormat() == MESSAGE_BINARY)
         {
            ppLogBody += OEM4_BINARY_HEADER_LENGTH;
         }
         else if ((*pclBaseMessageData)->getMessageFormat() == MESSAGE_SHORT_HEADER_BINARY)
         {
            ppLogBody += OEM4_BINARY_SHORT_HEADER_LENGTH;
         }
			CHAR* pGlobalOutBuf;
			
			pGlobalOutBuf = new CHAR[20480];
			memset(pGlobalOutBuf, '\0', 20480);
			if (pGlobalOutBuf == NULL)
				throw "Can't allocate memory for read buffer";
			char* pOutBuf = NULL;
			pOutBuf = pGlobalOutBuf;
			if (GetBMDOutput() == FLATTEN || GetBMDOutput() == BOTH)
			{
				pclMyLogUtils->flatten_binary_log(&ppLogBody, &pOutBuf);
				CHAR* FlatenBuf = new CHAR[pclMyLogUtils->getFlattenbinarylength()];
				memset(FlatenBuf, '\0', pclMyLogUtils->getFlattenbinarylength());
				if (FlatenBuf == NULL)
					throw "Can't allocate memory for read buffer";
				memcpy(FlatenBuf, pGlobalOutBuf, pclMyLogUtils->getFlattenbinarylength());
				(*pclBaseMessageData)->setFlattenMessageData(FlatenBuf);
				(*pclBaseMessageData)->setFlattenMessageLength(pclMyLogUtils->getFlattenbinarylength());
			}
			if (GetBMDOutput() == JSON || GetBMDOutput() == BOTH)
			{
				std::string jsonmessage = pclMyLogUtils->getJSONString(&ppLogBody);
				(*pclBaseMessageData)->setMessagejsonstring(jsonmessage);
			}
			(*pclBaseMessageData)->setMessageFormat(MESSAGE_ASCII);
			
			delete pGlobalOutBuf;
			pGlobalOutBuf = NULL;
			delete pEncoder;
			pEncoder = NULL;
			delete pclMyLogUtils;
			pclMyLogUtils = NULL;

		}
		

	}
	
}
