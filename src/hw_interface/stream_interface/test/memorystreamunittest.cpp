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
//  DESCRIPTION: Memory Stream Unit Test.
//    
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "hw_interface/stream_interface/api/memorystream.hpp"
#include "string"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class MemoryStreamTest : public ::testing::Test {
public:
   virtual void SetUp() {
}

   virtual void TearDown() {
   }
   UINT GetPercentStreamRead() { return pMyTestCommand->stMemoryReadStatus.uiPercentStreamRead; }
   UINT GetCurrentStreamRead() { return pMyTestCommand->stMemoryReadStatus.uiCurrentStreamRead; }
   ULONGLONG GetStreamLength() { return pMyTestCommand->stMemoryReadStatus.ullStreamLength; }
private:

protected:
   MemoryStream* pMyTestCommand = NULL;
};

// Constructor 
TEST_F(MemoryStreamTest, Constructor1)
{
   pMyTestCommand = NULL;
      
   pMyTestCommand = new MemoryStream(10240);
   ASSERT_EQ((UINT)10240 ,pMyTestCommand->GetCapacity());
   ASSERT_EQ(0 ,pMyTestCommand->Available());

   pMyTestCommand->Write('T');
   ASSERT_EQ(1 ,pMyTestCommand->Available());
   ASSERT_EQ((UINT)'T' ,pMyTestCommand->Read());
   ASSERT_EQ(0 ,pMyTestCommand->Available());

   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   pMyTestCommand->Write((UCHAR*)pcMessage, (UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));
   ASSERT_EQ(80 ,pMyTestCommand->Available());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = 80;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ(0 ,pMyTestCommand->Available());

   delete [] pcMessage;
   delete [] stReadDataStructure.cData;
   delete pMyTestCommand;
}

// Constructor2
TEST_F(MemoryStreamTest, Constructor2)
{
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   pMyTestCommand = new MemoryStream((UCHAR*)pcMessage, (UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));
   ASSERT_EQ(80 ,pMyTestCommand->Available());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = 20;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ((UINT)25 ,GetPercentStreamRead());
   ASSERT_EQ(60 ,pMyTestCommand->Available());

   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ((UINT)50 ,GetPercentStreamRead());
   ASSERT_EQ(40 ,pMyTestCommand->Available());

   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ((UINT)75 ,GetPercentStreamRead());
   ASSERT_EQ(20 ,pMyTestCommand->Available());

   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ((UINT)100 ,GetPercentStreamRead());
   ASSERT_EQ(0 ,pMyTestCommand->Available());

   delete [] pcMessage;
   delete [] stReadDataStructure.cData;
   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

TEST_F(MemoryStreamTest, Append)
{
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   pMyTestCommand = new MemoryStream((UCHAR*)pcMessage, (UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));
   ASSERT_EQ(80 ,pMyTestCommand->Available());

   ReadDataStructure stReadDataStructure; 
   stReadDataStructure.uiDataSize = 80;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ((UINT)100 ,GetPercentStreamRead());
   ASSERT_EQ(0 ,pMyTestCommand->Available());
 
   //APPEND
   pMyTestCommand->Append((UCHAR*)"Hello World!", (UINT)strlen("Hello World!"));
   ASSERT_EQ((INT)strlen("Hello World!") ,pMyTestCommand->Available());
   ASSERT_EQ(12 ,pMyTestCommand->Available());

   // ONE more Append
   pMyTestCommand->Append((UCHAR*)"Hello World!", (UINT)strlen("Hello World!"));
   ASSERT_EQ((INT)strlen("Hello World!")*2 ,pMyTestCommand->Available());
   ASSERT_EQ(24 ,pMyTestCommand->Available());

   stReadDataStructure.uiDataSize = 20;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ(4 ,pMyTestCommand->Available());

   // ONE more Append
   pMyTestCommand->Append((UCHAR*)"Hello World!", (UINT)strlen("Hello World!"));
   ASSERT_EQ((INT)strlen("Hello World!")+4 ,pMyTestCommand->Available());
   ASSERT_EQ(16 ,pMyTestCommand->Available());

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

TEST_F(MemoryStreamTest, Flush) // To clear the data in buffer
{
   pMyTestCommand = NULL;
		
   CHAR* pcMessage = new CHAR[strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29") + 1];
   strcpy(pcMessage, "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29");
   pMyTestCommand = new MemoryStream((UCHAR*)pcMessage, (UINT)strlen("$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29"));
   ASSERT_EQ(80 ,pMyTestCommand->Available());

   //FLUSH
   pMyTestCommand->Flush();
   ASSERT_EQ(0 ,pMyTestCommand->Available());

   //APPEND & FLUSH
   pMyTestCommand->Append((UCHAR*)"Hello World!", (UINT)strlen("Hello World!"));
   ASSERT_EQ(12 ,pMyTestCommand->Available());
   pMyTestCommand->Flush();
   ASSERT_EQ(0 ,pMyTestCommand->Available());
}

TEST_F(MemoryStreamTest, DummyConstructor) // Dummy constructor
{
   pMyTestCommand = NULL;		
   pMyTestCommand = new MemoryStream();
   ASSERT_EQ(0 ,pMyTestCommand->Available());

   pMyTestCommand->Append((UCHAR*)"",10);
   ASSERT_EQ(10 ,pMyTestCommand->Available());

   pMyTestCommand->Write((UCHAR*)"NovAtel TEST",11);
   ASSERT_EQ(21 ,pMyTestCommand->Available());

   pMyTestCommand->Write('\0');
   ASSERT_EQ(22 ,pMyTestCommand->Available());
}

TEST_F(MemoryStreamTest, Write)
{
   pMyTestCommand = new MemoryStream();
   ASSERT_EQ(0 ,pMyTestCommand->Available());

   pMyTestCommand->Write((UCHAR*)"Hello World!", 12);
   ASSERT_EQ(12 ,pMyTestCommand->Available());

   ReadDataStructure stReadDataStructure;
   stReadDataStructure.uiDataSize = 15;
   stReadDataStructure.cData = new CHAR[stReadDataStructure.uiDataSize + 1];
   stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
   pMyTestCommand->Read(stReadDataStructure.cData, stReadDataStructure.uiDataSize);
   ASSERT_EQ((UINT)12, GetCurrentStreamRead());
   ASSERT_EQ((ULONGLONG)12, GetStreamLength());
   ASSERT_EQ((UINT)100, GetPercentStreamRead());
   ASSERT_EQ(0, pMyTestCommand->Available());

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}
