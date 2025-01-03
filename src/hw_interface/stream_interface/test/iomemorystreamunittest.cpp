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
//  DESCRIPTION: I/O Memory Stream Unit Test.
//
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "hw_interface/stream_interface/api/inputmemorystream.hpp"
#include "hw_interface/stream_interface/api/outputmemorystream.hpp"
#include "decoders/novatel/api/framer.hpp"
#include "string"
#include "hw_interface/stream_interface/api/inputfilestream.hpp"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class IOMemoryStreamTest : public ::testing::Test {
public:
   virtual void SetUp() {
}

   virtual void TearDown() {
   }

private:

protected:
};

TEST_F(IOMemoryStreamTest, Using_Stream)
{
   MessageHeader stMessageHeader;
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, pcMessage);
   clBaseMessageData.setMessageLength((UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));

   InputMemoryStream* pInputMemoryStream = NULL;
   OutputMemoryStream* pOutputMemoryStream = NULL;

   pOutputMemoryStream = new OutputMemoryStream(clBaseMessageData.getMessageLength() + 1);
   ASSERT_EQ((UINT)81 ,pOutputMemoryStream->GetMemoryStream()->GetCapacity());
   ASSERT_EQ(0 ,pOutputMemoryStream->GetMemoryStream()->Available());

   pOutputMemoryStream->WriteData(clBaseMessageData);
   ASSERT_EQ(80 ,pOutputMemoryStream->GetMemoryStream()->Available());

   pInputMemoryStream = new InputMemoryStream(pOutputMemoryStream->GetMemoryStream()->GetBuffer(),pOutputMemoryStream->GetMemoryStream()->Available());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = 80;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;

   pInputMemoryStream->ReadData(stReadDataStructure);
   ASSERT_EQ(0 ,pInputMemoryStream->GetMemoryStream()->Available());

   delete [] stReadDataStructure.cData;
   delete pOutputMemoryStream;
   delete pInputMemoryStream;
}

TEST_F(IOMemoryStreamTest, Using_Interface)
{
   MessageHeader stMessageHeader;
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, pcMessage);
   clBaseMessageData.setMessageLength((UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));

   InputStreamInterface* pInputMemoryStream = NULL;
   OutputStreamInterface* pOutputMemoryStream = NULL;

   pOutputMemoryStream = new OutputMemoryStream(clBaseMessageData.getMessageLength() + 1);
   ASSERT_EQ((UINT)81 ,pOutputMemoryStream->GetMemoryStream()->GetCapacity());
   ASSERT_EQ(0 ,pOutputMemoryStream->GetMemoryStream()->Available());

   pOutputMemoryStream->WriteData(clBaseMessageData);
   ASSERT_EQ(80 ,pOutputMemoryStream->GetMemoryStream()->Available());

   pInputMemoryStream = new InputMemoryStream((UCHAR*)clBaseMessageData.getMessageData(), clBaseMessageData.getMessageLength());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = 80;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;

   pInputMemoryStream->ReadData(stReadDataStructure);
   ASSERT_EQ(0 ,pInputMemoryStream->GetMemoryStream()->Available());

   delete [] stReadDataStructure.cData;
   delete pOutputMemoryStream;
   delete pInputMemoryStream;
}

TEST_F(IOMemoryStreamTest, Using_Interface1)
{
   MessageHeader stMessageHeader;
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, pcMessage);
   clBaseMessageData.setMessageLength((UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));

   InputStreamInterface* pInputMemoryStream = NULL;
   OutputStreamInterface* pOutputMemoryStream = NULL;

   pOutputMemoryStream = new OutputMemoryStream(clBaseMessageData.getMessageLength() + 1);
   ASSERT_EQ((UINT)81 ,pOutputMemoryStream->GetMemoryStream()->GetCapacity());
   ASSERT_EQ(0 ,pOutputMemoryStream->GetMemoryStream()->Available());

   pOutputMemoryStream->WriteData(clBaseMessageData);
   ASSERT_EQ(80 ,pOutputMemoryStream->GetMemoryStream()->Available());

   pInputMemoryStream = new InputMemoryStream(pOutputMemoryStream->GetMemoryStream()->GetBuffer(), pOutputMemoryStream->GetMemoryStream()->GetLength());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = 80;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;

   pInputMemoryStream->ReadData(stReadDataStructure);
   ASSERT_EQ(0 ,pInputMemoryStream->GetMemoryStream()->Available());

   delete [] stReadDataStructure.cData;
   delete pOutputMemoryStream;
   delete pInputMemoryStream;
}

TEST_F(IOMemoryStreamTest, ReadFromFile)
{
   ReadDataStructure stReadDataStructure;
   InputFileStream* pMyTestCommand = NULL;
   std::string filename = std::string(DATADIR) + "decoder_bestpos.asc";

   const char* filepath = filename.c_str();
   pMyTestCommand = new InputFileStream(filepath);

   InputStreamInterface* pInputMemoryStream = NULL;

   Framer stFramer(pMyTestCommand);
   stFramer.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   do
   {
      BaseMessageData *pBMD = NULL;
      stStreamReadStatus = stFramer.ReadMessage(&pBMD);
      if (pBMD == NULL)
      {
         continue;
      }
      else
      {
         pInputMemoryStream = new InputMemoryStream((UCHAR*)pBMD->getMessageData(), pBMD->getMessageLength());
         ASSERT_EQ(217 ,pInputMemoryStream->GetMemoryStream()->Available());

         //ReadDataStructure stReadDataStructure;
         stReadDataStructure.uiDataSize = 217;
         stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
         stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;

         pInputMemoryStream->ReadData(stReadDataStructure);
         ASSERT_EQ(0 ,pInputMemoryStream->GetMemoryStream()->Available());

         delete pBMD;
         pBMD = NULL;
         delete [] stReadDataStructure.cData;
      }
   } while(stStreamReadStatus.bEOS != TRUE);

   delete pInputMemoryStream;
   delete pMyTestCommand;
}

TEST_F(IOMemoryStreamTest, Constructor1)
{
   InputMemoryStream* pInputMemoryStream = new InputMemoryStream();
   
   UINT uiBytes_ = pInputMemoryStream->Write((UCHAR*)"Hello World!", strlen("Hello World!"));
   ASSERT_TRUE(uiBytes_ == strlen("Hello World!"));
   ASSERT_TRUE(pInputMemoryStream->IsStreamAvailable() == TRUE);
   ASSERT_EQ(uiBytes_ , pInputMemoryStream->GetMemoryStream()->Available());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = uiBytes_;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
   pInputMemoryStream->ReadData(stReadDataStructure);

   ASSERT_EQ(0 ,pInputMemoryStream->GetMemoryStream()->Available());
   ASSERT_TRUE(pInputMemoryStream->IsStreamAvailable() == FALSE);
   ASSERT_STREQ("Hello World!", stReadDataStructure.cData);

   delete [] stReadDataStructure.cData;   
   delete pInputMemoryStream;
}

TEST_F(IOMemoryStreamTest, Constructor2)
{
   InputMemoryStream* pInputMemoryStream = new InputMemoryStream(1023);
   
   UINT uiBytes_ = pInputMemoryStream->Write((UCHAR*)"Hello World!", strlen("Hello World!"));
   ASSERT_TRUE(uiBytes_ == strlen("Hello World!"));
   ASSERT_TRUE(pInputMemoryStream->IsStreamAvailable() == TRUE);
   ASSERT_EQ(uiBytes_ , pInputMemoryStream->GetMemoryStream()->Available());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = uiBytes_;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
   pInputMemoryStream->ReadData(stReadDataStructure);

   ASSERT_EQ(0 ,pInputMemoryStream->GetMemoryStream()->Available());
   ASSERT_TRUE(pInputMemoryStream->IsStreamAvailable() == FALSE);
   ASSERT_STREQ("Hello World!", stReadDataStructure.cData);

   delete [] stReadDataStructure.cData;   
   delete pInputMemoryStream;
}

TEST_F(IOMemoryStreamTest, OutputMemoryStreamTest)
{
   OutputMemoryStream* pOutputMemoryStream = new OutputMemoryStream((UCHAR*)"Hello World!", strlen("Hello World!"));
   ASSERT_EQ(strlen("Hello World!"),pOutputMemoryStream->GetMemoryStream()->Available());
   ASSERT_EQ(strlen("Hello World!") + 512/* Extra Addon internally */ ,pOutputMemoryStream->GetMemoryStream()->GetCapacity());

   delete pOutputMemoryStream;
   pOutputMemoryStream = NULL;

   MessageHeader stMessageHeader;
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   stMessageHeader.eMessageFormat = MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, pcMessage);
   clBaseMessageData.setMessageLength((UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));

   pOutputMemoryStream = new OutputMemoryStream();
   pOutputMemoryStream->WriteData(clBaseMessageData);
   ASSERT_EQ(80 ,pOutputMemoryStream->GetMemoryStream()->Available());  

   InputMemoryStream* pInputMemoryStream = new InputMemoryStream(pOutputMemoryStream->GetMemoryStream()->GetBuffer(), pOutputMemoryStream->GetMemoryStream()->GetLength());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = 80;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;

   pInputMemoryStream->ReadData(stReadDataStructure);
   ASSERT_EQ(0 ,pInputMemoryStream->GetMemoryStream()->Available());

   delete [] stReadDataStructure.cData;
   delete pOutputMemoryStream;
   delete pInputMemoryStream;
}
