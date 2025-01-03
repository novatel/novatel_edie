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

// Includes
#include "hw_interface/stream_interface/api/outputfilestream.hpp"
#include "string"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class OutputFileStreamTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:

protected:
};

// Constructor1
TEST_F(OutputFileStreamTest, Constructor1)
{
   OutputFileStream* pMyTestCommand = NULL;
   std::string filename = std::string(DATADIR) + "outputfilestream_file1.asc";       
   
   const char* filepath = filename.c_str();
   pMyTestCommand = new OutputFileStream(filepath);
   ASSERT_TRUE(pMyTestCommand->pOutFileStream != NULL);
   delete pMyTestCommand;
}

// Constructor2
TEST_F(OutputFileStreamTest, Constructor2)
{
   OutputFileStream* pMyTestCommand = NULL;
   MessageDataFilter* pMyTestCommand1 = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
   std::string filename = std::string(DATADIR) + "outputfilestream_file2.asc";       
   
   const char* filepath = filename.c_str();
   pMyTestCommand = new OutputFileStream(filepath, *pMyTestCommand1);
   ASSERT_TRUE(pMyTestCommand->pOutFileStream != NULL);
   delete pMyTestCommand;
   delete pMyTestCommand1;
}

// WriteData Test
TEST_F(OutputFileStreamTest, WriteData)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char[strlen("This is outputfilestream_file3.") + 1];
   strcpy(chDecodedMessage, "This is outputfilestream_file3.");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageLength((UINT)strlen("This is outputfilestream_file3."));
   OutputFileStream* pMyTestCommand = NULL;
   std::string filename = std::string(DATADIR) + "outputfilestream_file3.asc";       
   
   const char* filepath = filename.c_str();
   pMyTestCommand = new OutputFileStream(filepath);
   UINT uiLenght = pMyTestCommand->WriteData(clBaseMessageData);
   ASSERT_EQ(31, (INT)uiLenght);

   FilterConfig stFilterConfig;
   stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY);
   stFilterConfig.AddIDFormatPair(6, MESSAGE_ASCII);
   stFilterConfig.IsNegativeFilter(FALSE);
   MessageDataFilter* pMyTestCommand2 = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
   pMyTestCommand2->ConfigureFilter(stFilterConfig);

   OutputFileStream* pMyTestCommand1 = NULL;
   filename = std::string(DATADIR) + "outputfilestream_file4.asc";          
   filepath = filename.c_str();
   pMyTestCommand1 = new OutputFileStream(filepath, *pMyTestCommand2);
   chDecodedMessage = new char[strlen("This is outputfilestream_file4.") + 1];
   strcpy(chDecodedMessage, "This is outputfilestream_file4.");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData1(&stMessageHeader, chDecodedMessage);
   clBaseMessageData1.setMessageLength((UINT)strlen("This is outputfilestream_file4."));
   clBaseMessageData1.setMessageTimeStatus(MessageTimeStatusEnum::TIME_FINE);
   clBaseMessageData1.setMessageID(6);
   clBaseMessageData1.setMessageFormat(MESSAGE_ASCII);
   uiLenght = pMyTestCommand1->WriteData(clBaseMessageData1);
   ASSERT_EQ(31, (INT)uiLenght);

   delete pMyTestCommand;
}
