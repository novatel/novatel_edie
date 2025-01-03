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
//  DESCRIPTION: Multi Output File Stream Unit Test.
//
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "hw_interface/stream_interface/api/multioutputfilestream.hpp"
#include "string"

#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class MultiOutputFileStreamTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  typedef std::map<std::string, FileStream* > FstreamMap;
  MultiOutputFileStream::FstreamMap::iterator itFstreamMapIterator;

  BOOL IsFileSplit() { return pMyTestCommand->bMyFileSplit; }
  FileStream* GetLocalFileStream() { return pMyTestCommand->pLocalFileStream; }
  MessageDataFilter* GetMessageDataFilter() { return pMyTestCommand->pMyMessageDataFilter; }
  FileSplitMethodEnum GetFileSplitMethodEnum() { return pMyTestCommand->eMyFileSplitMethodEnum; }
  std::string GetBaseFileName() { return pMyTestCommand->stMyBaseName; }
  std::string GetExtensionName() { return pMyTestCommand->stMyExtentionName; }
  UINT GetFileSplitSize() { return (UINT)pMyTestCommand->ullMyFileSplitSize; }
  ULONGLONG GetFileSize() { return pMyTestCommand->ullMyFileSize; }
  UINT GetFileCount() { return pMyTestCommand->uiMyFileCount; }
  DOUBLE GetTimeSplitSize() { return pMyTestCommand->dMyTimeSplitSize; }
  DOUBLE GetTimeInSeconds() { return pMyTestCommand->dMyTimeInSeconds; }
  DOUBLE GetStartTimeInSecs() { return pMyTestCommand->dMyStartTimeInSeconds; }
  ULONG GetWeek() { return pMyTestCommand->ulMyWeek; }
  ULONG GetStartWeek() { return pMyTestCommand->ulMyStartWeek; }
  FstreamMap GetMap() { return pMyTestCommand->GetFileMap(); }private:

protected:
	MultiOutputFileStream* pMyTestCommand = NULL;
   //typedef std::map<std::string, FileStream* > FstreamMap;
   //FstreamMap mMyFstreamMap;

};

// Constructor
TEST_F(MultiOutputFileStreamTest, Constructor)
{
	MessageDataFilter* pMessageDataFilter = NULL;
	pMyTestCommand = new MultiOutputFileStream(*pMessageDataFilter);
	ASSERT_EQ(0, IsFileSplit());
	ASSERT_TRUE(GetLocalFileStream() == NULL);
	ASSERT_TRUE(GetMessageDataFilter() == NULL);
	ASSERT_EQ(3, (INT)GetFileSplitMethodEnum());
	ASSERT_STREQ("DefaultBase", GetBaseFileName().c_str());
	ASSERT_STREQ("DefaultExt", GetExtensionName().c_str());
	ASSERT_EQ(0, GetFileSplitSize());
	ASSERT_EQ(0, GetFileSize());
	ASSERT_EQ(0, GetFileCount());
	ASSERT_EQ(0.0, GetTimeSplitSize());
	ASSERT_EQ(0.0, GetTimeInSeconds());
	ASSERT_EQ(0.0, GetStartTimeInSecs());
	delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, ConfigureSplitByLog)
{
   pMyTestCommand = new MultiOutputFileStream();

   pMyTestCommand->ConfigureSplitByLog(TRUE);
   ASSERT_TRUE(IsFileSplit() == TRUE);

   pMyTestCommand->ConfigureSplitByLog(FALSE);
   ASSERT_TRUE(IsFileSplit() == FALSE);

   delete pMyTestCommand;
}

// Test the ConfigureBaseFileName method.
TEST_F(MultiOutputFileStreamTest, ConfigureBaseFileName)
{
   pMyTestCommand = new MultiOutputFileStream();

   pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file13");
   ASSERT_STREQ("multiOutputfilestream_file13", GetBaseFileName().c_str());
   ASSERT_STREQ("DefaultExt", GetExtensionName().c_str());

   pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file13.txt");
   ASSERT_STREQ("multiOutputfilestream_file13", GetBaseFileName().c_str());
   ASSERT_STREQ("txt", GetExtensionName().c_str());

   delete pMyTestCommand;
}

// Test the SelectLogFile method.
TEST_F(MultiOutputFileStreamTest, SelectLogFile)
{
  typedef std::map<std::string, FileStream* > FstreamMap;
  FstreamMap myMap;

  pMyTestCommand = new MultiOutputFileStream();
  pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file14.asc");

  BaseMessageData clBaseMessageData;
  clBaseMessageData.setMessageName("bestpos");

  pMyTestCommand->SelectLogFile(clBaseMessageData);
  myMap = GetMap();
  itFstreamMapIterator = myMap.begin();

  std::string strFileName = GetBaseFileName() +  "_" + "bestpos" + "." + GetExtensionName();
  ASSERT_STREQ(strFileName.c_str(), (itFstreamMapIterator->first).c_str());

  ASSERT_STREQ(strFileName.c_str(), itFstreamMapIterator->second->GetFileName());

  pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file15");
  pMyTestCommand->SetExtensionName("DefaultExt");
  pMyTestCommand->SelectLogFile(clBaseMessageData);

  itFstreamMapIterator++;
  strFileName = GetBaseFileName() +  "_" + "bestpos";

  // Need to improve further here
  //printf("myMap size: %d\n", myMap.size());
  //printf("first: %s\n", (itFstreamMapIterator->first).c_str());
  //ASSERT_STREQ(strFileName.c_str(), (itFstreamMapIterator->first).c_str());
  delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, WriteData)
{
   pMyTestCommand = new MultiOutputFileStream();
   pMyTestCommand->ConfigureSplitByLog(TRUE);
   pMyTestCommand->ConfigureBaseFileName("Log.txt");

   BaseMessageData* clBMD = new BaseMessageData();
   clBMD->setMessageName("BESTPOS");
   clBMD->setMessageData("HELLO");
   clBMD->setMessageLength(5);

   INT iLen = pMyTestCommand->WriteData(*clBMD);
   ASSERT_TRUE(iLen == 5);

   std::ifstream ifile("Log_BESTPOS.txt");
   if (ifile) {
     // The file exists, and is open for input
   }
   else
      ASSERT_TRUE(4 == 5); // Simple fails

   ifile.close();

   pMyTestCommand->ClearFileStreamMap();
}

TEST_F(MultiOutputFileStreamTest, ConfigureSplitBySize)
{
   pMyTestCommand = new MultiOutputFileStream();
   pMyTestCommand->ConfigureSplitBySize(1);
   ASSERT_TRUE((BOOL)1 == IsFileSplit());
   ASSERT_TRUE(1 == (INT)GetFileSplitSize());

   pMyTestCommand->ConfigureBaseFileName("Log.txt");

   BaseMessageData* clBMD = new BaseMessageData();
   clBMD->setMessageName("BESTPOS");
   clBMD->setMessageData("HELLO");
   clBMD->setMessageLength(5);

   INT iLen = pMyTestCommand->WriteData(*clBMD);
   ASSERT_TRUE(iLen == 5);

   std::ifstream ifile("Log_Part0.txt");
   if (ifile) {
     // The file exists, and is open for input
   }
   else
      ASSERT_TRUE(4 == 5); // Simple fails

   ifile.close();

   pMyTestCommand->ClearFileStreamMap();

   try
   {
     pMyTestCommand->ConfigureSplitBySize(MIN_FILE_SPLIT_SIZE - 1); // 0
     ASSERT_TRUE(4 == 1); // Should not execute
   }
   catch(nExcept e)
   {
     ASSERT_STREQ("File Split by Size not valid", e.buffer);
   }

  delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, ConfigureSplitByTime)
{
   pMyTestCommand  = new MultiOutputFileStream();
   pMyTestCommand->ConfigureSplitByTime(0.01);
   pMyTestCommand->ConfigureBaseFileName("Log.txt");

   BaseMessageData* clBMD = new BaseMessageData();
   clBMD->setMessageName("BESTPOS");
   clBMD->setMessageData("HELLO");
   clBMD->setMessageLength(5);

   INT iLen = pMyTestCommand->WriteData(*clBMD);
   ASSERT_TRUE(iLen == 5);

   std::ifstream ifile("Log_Part0.txt");
   if (ifile) {
     // The file exists, and is open for input
   }
   else
      ASSERT_TRUE(4 == 5); // Simpley fails

   ifile.close();

   pMyTestCommand->ClearFileStreamMap();

   try
   {
     pMyTestCommand->ConfigureSplitByTime(0.0); // 0
     ASSERT_TRUE(4 == 1); // Should not execute
   }
   catch(nExcept e)
   {
     ASSERT_STREQ("File Split by time not valid", e.buffer);
   }
   delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, SelectTimeFile)
{
   BaseMessageData* clBMD = new BaseMessageData();
   clBMD->setMessageName("BESTPOS");
   clBMD->setMessageData("HELLO");
   clBMD->setMessageLength(5);

   pMyTestCommand  = new MultiOutputFileStream();
   pMyTestCommand->ConfigureSplitByTime(0.01);
   pMyTestCommand->SelectFileStream("Log.txt");
   pMyTestCommand->SelectTimeFile(*clBMD);

   ASSERT_TRUE(GetTimeInSeconds() == 0.0);
   ASSERT_TRUE(GetStartTimeInSecs() == 0.0);
   ASSERT_TRUE(GetWeek() == 0);
   ASSERT_TRUE(GetStartWeek() == 0);

   clBMD->setMessageTimeStatus(MessageTimeStatusEnum::TIME_SATTIME);
   pMyTestCommand->SelectTimeFile(*clBMD);

   ASSERT_TRUE(GetTimeInSeconds() == 0.0);
   ASSERT_TRUE(GetStartTimeInSecs() == 0.0);
   ASSERT_TRUE(GetWeek() == 0);
   ASSERT_TRUE(GetStartWeek() == 0);

   delete pMyTestCommand;
}
