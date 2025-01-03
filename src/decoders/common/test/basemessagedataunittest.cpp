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

#include "common/api/basemessagedata.hpp"
#include "string.h"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class BaseMessageDataTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:
};

TEST_F(BaseMessageDataTest, GetMessageLength)
{
   BaseMessageData *pclBmd = new BaseMessageData();
   pclBmd->setMessageFormat(MESSAGE_RINEX);
   pclBmd->setMessageLength(20);
   ASSERT_TRUE(pclBmd->getMessageLength()==20);
   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, GetMessageType)
{
   BaseMessageData *pclBmd = new BaseMessageData();
   pclBmd->setMessageFormat(MESSAGE_RINEX);
   ASSERT_TRUE(pclBmd->getMessageFormat()==MESSAGE_RINEX);
   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, GetMessageData)
{
   CHAR* chMessage = new char [strlen("This is sample rinex data.") + 1];
   strcpy(chMessage, "This is sample rinex data.");

   BaseMessageData *pclBmd = new BaseMessageData();
   pclBmd->setMessageFormat(MESSAGE_RINEX);
   pclBmd->setMessageData(chMessage);
   ASSERT_STREQ(pclBmd->getMessageData(), chMessage);
   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, CopyConstructor)
{
   CHAR* pcMessage = new CHAR[strlen("This is sample test data") + 1];
   strcpy(pcMessage, "This is sample test data");

   BaseMessageData *pclBmd = new BaseMessageData();
   pclBmd->setMessageID(20);
   pclBmd->setMessageFormat(MESSAGE_ASCII);
   pclBmd->setMessageLength((UINT)strlen("This is sample test data") + 1);
   pclBmd->setMessageData(pcMessage);

   BaseMessageData* pclCopyBmd1 = new BaseMessageData(*pclBmd);
   ASSERT_TRUE(pclCopyBmd1->getMessageID() == 20);
   delete pclCopyBmd1;
   pclCopyBmd1 = NULL;

   pclBmd->setMessageFormat(MESSAGE_UNKNOWN);
   pclCopyBmd1 = new BaseMessageData(*pclBmd);
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_UNKNOWN);
   delete pclCopyBmd1;
   pclCopyBmd1 = NULL;

   pclBmd->setMessageFormat(MESSAGE_BINARY);
   pclCopyBmd1 = new BaseMessageData(*pclBmd);
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_BINARY);
   delete pclCopyBmd1;
   pclCopyBmd1 = NULL;

   pclBmd->setMessageFormat(MESSAGE_ABB_ASCII);
   pclCopyBmd1 = new BaseMessageData(*pclBmd);
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_ABB_ASCII);
   delete pclCopyBmd1;
   pclCopyBmd1 = NULL;

   pclBmd->setMessageFormat(MESSAGE_RINEX);
   pclCopyBmd1 = new BaseMessageData(*pclBmd);
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_RINEX);
   delete pclCopyBmd1;
   pclCopyBmd1 = NULL;

   pclBmd->setMessageFormat(MESSAGE_NMEA2000);
   pclCopyBmd1 = new BaseMessageData(*pclBmd);
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_NMEA2000);
   delete pclCopyBmd1;
   pclCopyBmd1 = NULL;

   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, EqualsOperator)
{
   CHAR* pcMessage = new CHAR[strlen("This is sample test data") + 1];
   strcpy(pcMessage, "This is sample test data");
   BaseMessageData* pclBmd = new BaseMessageData();
   pclBmd->setMessageID(20);
   pclBmd->setMessageFormat(MESSAGE_ASCII);
   pclBmd->setMessageLength((UINT)strlen("This is sample test data") + 1);
   pclBmd->setMessageData(pcMessage);
   BaseMessageData* pclCopyBmd1 = new BaseMessageData();
   *pclCopyBmd1 = *pclBmd;
   ASSERT_TRUE(pclCopyBmd1->getMessageID() == 20);

   pclBmd->setMessageFormat(MESSAGE_UNKNOWN);
   *pclCopyBmd1 = *pclBmd;
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_UNKNOWN);

   pclBmd->setMessageFormat(MESSAGE_BINARY);
   *pclCopyBmd1 = *pclBmd;
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_BINARY);

   pclBmd->setMessageFormat(MESSAGE_ABB_ASCII);
   *pclCopyBmd1 = *pclBmd;
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_ABB_ASCII);

   pclBmd->setMessageFormat(MESSAGE_RINEX);
   *pclCopyBmd1 = *pclBmd;
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_RINEX);

   pclBmd->setMessageFormat(MESSAGE_NMEA2000);
   *pclCopyBmd1 = *pclBmd;
   ASSERT_TRUE(pclCopyBmd1->getMessageFormat() == MESSAGE_NMEA2000);

   delete pclCopyBmd1;
   pclCopyBmd1 = NULL;

   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, SetMessageAttributes)
{
   BaseMessageData* pclBmd = new BaseMessageData();
   pclBmd->setMessageDefCrc(45558);
   ASSERT_TRUE(pclBmd->getMessageDefCrc() == 45558);
   pclBmd->setMessagePort(0);
   ASSERT_TRUE(pclBmd->getMessagePort() == 0);
   pclBmd->setMessageType(TRUE);
   ASSERT_TRUE(pclBmd->getMessageType() == TRUE);
   pclBmd->setReceiverSWVersion(0);
   ASSERT_TRUE(pclBmd->getReceiverSWVersion() == 0);
   pclBmd->setReceiverStatus(0);
   ASSERT_TRUE(pclBmd->getReceiverStatus() == 0);
   pclBmd->setResponseID(1);
   ASSERT_TRUE(pclBmd->getResponseID() == 1);
   pclBmd->setResponseType(FALSE);
   ASSERT_TRUE(pclBmd->getResponseType() == FALSE);
   pclBmd->setMessageLength(0);
   ASSERT_TRUE(pclBmd->getMessageLength() == 0);
   pclBmd->setIdleTime(123.44);
   ASSERT_TRUE(pclBmd->getIdleTime() == 123.44);

   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, GetMessageSource)
{
   BaseMessageData *pclBmd = new BaseMessageData();
   pclBmd->setMessageSource(PRIMARY_ANTENNA);
   ASSERT_TRUE(pclBmd->getMessageSource()==PRIMARY_ANTENNA);
   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, GetNMEAMsgFieldCount)
{
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   BaseMessageData* pclBmd = new BaseMessageData();
   pclBmd->setMessageID(20);
   pclBmd->setMessageFormat(MESSAGE_ASCII);
   pclBmd->setMessageLength((UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1);
   pclBmd->setMessageData(pcMessage);
   ASSERT_TRUE(pclBmd->getNMEAMsgFieldCount()==16);
   delete pclBmd;
   pclBmd = NULL;

   // Check for NMEA msg format
   pclBmd = new BaseMessageData();
   pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   pclBmd->setMessageID(20);
   pclBmd->setMessageFormat(MESSAGE_BINARY); // This should make get nmea filed count is 0
   pclBmd->setMessageLength((UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1);
   pclBmd->setMessageData(pcMessage);
   ASSERT_TRUE(pclBmd->getNMEAMsgFieldCount()==0);   

   delete pclBmd;
   pclBmd = NULL;   

   // Check starting char is not '$'
   pclBmd = new BaseMessageData();
   pcMessage = new CHAR[strlen("#GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "#GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   pclBmd->setMessageID(20);
   pclBmd->setMessageFormat(MESSAGE_ASCII); // This should make get nmea filed count is 0
   pclBmd->setMessageLength((UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1);
   pclBmd->setMessageData(pcMessage);
   ASSERT_TRUE(pclBmd->getNMEAMsgFieldCount()==0);   

   delete pclBmd;
   pclBmd = NULL;   
}

TEST_F(BaseMessageDataTest, GetMessageTimeMilliSeconds)
{
   BaseMessageData *pclBmd = new BaseMessageData();
   pclBmd->setMessageFormat(MESSAGE_RINEX);
   pclBmd->setMessageTimeMilliSeconds(102230);
   ASSERT_TRUE(pclBmd->getMessageTimeMilliSeconds()==102230);
   delete pclBmd;
   pclBmd = NULL;
}

TEST_F(BaseMessageDataTest, GetObjSize)
{
   BaseMessageData *pclBmd = NULL;
   try{
	   pclBmd = new BaseMessageData();
	   pclBmd->setMessageFormat(MESSAGE_RINEX);
	   auto pFunction = [pclBmd] {pclBmd->GetObjSize(); };
	   //Assert::ExpectException<const char*>(pFunction, L"Exception not thrown");
	   ASSERT_TRUE(1 == 1); // TBD
	   delete pclBmd;
	   pclBmd = NULL;
   }
   catch(nExcept ne)
   {
	   ASSERT_EQ(ne.buffer, "Message Doesn't have derived method");
	   delete pclBmd;
	   pclBmd = NULL;
   }
}

TEST_F(BaseMessageDataTest, GetMessageSize)
{
   BaseMessageData *pclBmd = NULL;
   try{
	   pclBmd = new BaseMessageData();
	   pclBmd->setMessageFormat(MESSAGE_RINEX);
	   auto pFunction = [pclBmd] {pclBmd->GetMessageSize(); };
	   //Assert::ExpectException<const char*>(pFunction, L"Exception not thrown");
	   ASSERT_TRUE(1 == 1);// TBD
	   delete pclBmd;
	   pclBmd = NULL;
   }
   catch(nExcept ne)
   {
	   ASSERT_EQ(ne.buffer, "Message Doesn't have derived method");
	   delete pclBmd;
	   pclBmd = NULL;
   }
}

TEST_F(BaseMessageDataTest, SETGETMSGNAME)
{
   BaseMessageData *pclBmd = new BaseMessageData();
   pclBmd->setMessageName("BESTPOS");
   std::string strMsgName = pclBmd->getMessageName(); 
   ASSERT_STREQ(strMsgName.c_str(), "BESTPOS");
   delete pclBmd;
   pclBmd = NULL;   
}

TEST_F(BaseMessageDataTest, SetGetMsgTimeAndStatus)
{
   BaseMessageData *pclBmd = new BaseMessageData();
   
   pclBmd->setMessageTimeStatus(TIME_FINESTEERING);
   ASSERT_TRUE(pclBmd->getMessageTimeStatus()==TIME_FINESTEERING);  

   pclBmd->setMessageTimeStatus(TIME_UNKNOWN);
   ASSERT_TRUE(pclBmd->getMessageTimeStatus()==TIME_UNKNOWN);  

   pclBmd->setMessageTimeWeek(2020);
   ASSERT_TRUE(pclBmd->getMessageTimeWeek()==2020);  
   
   pclBmd->setMessageTimeWeek(0);
   ASSERT_TRUE(pclBmd->getMessageTimeWeek()==0);  

   delete pclBmd;
   pclBmd = NULL; 
}

TEST_F(BaseMessageDataTest, SetGetMsgLength)
{
   BaseMessageData *pclBmd = new BaseMessageData();

   pclBmd->setMessageFormat(MESSAGE_BINARY);
   pclBmd->setMessageLength(128);
   ASSERT_TRUE(pclBmd->getMessageLength()==128);  
   pclBmd->setMessageLength(-1);
   ASSERT_TRUE(pclBmd->getMessageLength()== -1);  

   pclBmd->setMessageFormat(MESSAGE_ABB_ASCII);
   pclBmd->setMessageLength(128);
   ASSERT_TRUE(pclBmd->getMessageLength()==128);  
   pclBmd->setMessageLength(-1);
   ASSERT_TRUE(pclBmd->getMessageLength()== -1);    
    
   pclBmd->setMessageFormat(MESSAGE_NMEA2000);
   pclBmd->setMessageLength(128);
   ASSERT_TRUE(pclBmd->getMessageLength()==128);  
   pclBmd->setMessageLength(-1);
   ASSERT_TRUE(pclBmd->getMessageLength()== -1);   

   pclBmd->setMessageFormat(MESSAGE_ASCII);
   pclBmd->setMessageLength(128);
   ASSERT_TRUE(pclBmd->getMessageLength()==128);  
   pclBmd->setMessageLength(-1);
   ASSERT_TRUE(pclBmd->getMessageLength()== -1);         

   pclBmd->setFlattenMessageLength(128);
   ASSERT_TRUE(pclBmd->getFlattenMessageLength()==128);  
   pclBmd->setFlattenMessageLength(-1);
   ASSERT_TRUE(pclBmd->getFlattenMessageLength()== -1);       

   delete pclBmd;
   pclBmd = NULL; 
}

TEST_F(BaseMessageDataTest, Constructor)
{
   CHAR* pcMessage = new CHAR[strlen("This is sample test data") + 1];
   strcpy(pcMessage, "This is sample test data");

   MessageHeader stMsgHeader;
   stMsgHeader.uiMessageID = 20;
   stMsgHeader.eMessageFormat = MESSAGE_ASCII;
   stMsgHeader.uiMessageLength = strlen("This is sample test data") + 1;
   BaseMessageData *pclBmd = new BaseMessageData(&stMsgHeader, pcMessage);
   ASSERT_TRUE(pclBmd->getMessageID()==20);     
   ASSERT_TRUE(pclBmd->getMessageFormat()== MESSAGE_ASCII);
   ASSERT_TRUE(pclBmd->getMessageLength()==strlen("This is sample test data") + 1);   

   delete pclBmd;
   pclBmd = NULL;   

   pcMessage = new CHAR[strlen("This is sample test data") + 1];
   strcpy(pcMessage, "This is sample test data");
   stMsgHeader.uiMessageID = 20;
   stMsgHeader.eMessageFormat = MESSAGE_UNKNOWN;
   pclBmd = new BaseMessageData(&stMsgHeader, pcMessage);
   ASSERT_TRUE(pclBmd->getMessageID()==20);     
   ASSERT_TRUE(pclBmd->getMessageFormat()== MESSAGE_UNKNOWN);
   ASSERT_TRUE(pclBmd->getMessageLength()==strlen("This is sample test data") + 1);   

   delete pclBmd;
   pclBmd = NULL; 

   pcMessage = new CHAR[strlen("This is sample test data") + 1];
   strcpy(pcMessage, "This is sample test data");
   stMsgHeader.uiMessageID = 20;
   stMsgHeader.eMessageFormat = MESSAGE_BINARY;
   pclBmd = new BaseMessageData(&stMsgHeader, pcMessage);
   ASSERT_TRUE(pclBmd->getMessageID()==20);     
   ASSERT_TRUE(pclBmd->getMessageFormat()== MESSAGE_BINARY);
   ASSERT_TRUE(pclBmd->getMessageLength()==strlen("This is sample test data") + 1);  

   delete pclBmd;
   pclBmd = NULL; 

   pcMessage = new CHAR[strlen("This is sample test data") + 1];
   strcpy(pcMessage, "This is sample test data");
   stMsgHeader.uiMessageID = 20;
   stMsgHeader.eMessageFormat = MESSAGE_ABB_ASCII;
   pclBmd = new BaseMessageData(&stMsgHeader, pcMessage);
   ASSERT_TRUE(pclBmd->getMessageID()==20);     
   ASSERT_TRUE(pclBmd->getMessageFormat()== MESSAGE_ABB_ASCII);
   ASSERT_TRUE(pclBmd->getMessageLength()==strlen("This is sample test data") + 1);  

   delete pclBmd;
   pclBmd = NULL; 
}

