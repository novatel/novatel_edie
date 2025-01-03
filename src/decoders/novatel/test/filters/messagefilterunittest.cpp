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
#include "decoders/novatel/api/filters/messagefilter.hpp"
#include "string"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class MessageFilterTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  size_t GetIDFormatPairVectorSize() { return pMyTestCommand->pMyIDFormatPair.size(); }
  MessageAntennaSourceEnum GetMessageFormatEnum(INT iIndex) { return pMyTestCommand->pMyIDSourcePair.find(iIndex)->second; }
private:
protected:
   MessageFilter* pMyTestCommand = NULL;
protected:
};

// ConfigureFilter Test
TEST_F(MessageFilterTest, ConfigureFilter)
{
   FilterConfig stFilterConfig;
   stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY);
   stFilterConfig.AddIDFormatPair(6, MESSAGE_ASCII);

   pMyTestCommand = new MessageFilter();
   pMyTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(GetIDFormatPairVectorSize(), stFilterConfig.pIDFormatPair.size());

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

TEST_F(MessageFilterTest, ConfigureIDSourceFilter)
{
   FilterConfig stFilterConfig;
   stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY, PRIMARY_ANTENNA);
   stFilterConfig.AddIDFormatPair(6, MESSAGE_ASCII);

   pMyTestCommand = new MessageFilter();
   pMyTestCommand->ConfigureFilter(stFilterConfig);

   BOOL bPassed = (GetMessageFormatEnum(6) == BOTH_ANTENNA);
   ASSERT_EQ(TRUE, bPassed);
   bPassed = (GetMessageFormatEnum(5) == PRIMARY_ANTENNA);
   ASSERT_EQ(TRUE, bPassed);

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

// Positive Filter Test
TEST_F(MessageFilterTest, PositiveFilter)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageID(5);
	clBaseMessageData.setMessageFormat(MESSAGE_BINARY); 

   FilterConfig stFilterConfig;
   stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY);
   stFilterConfig.AddIDFormatPair(6, MESSAGE_ASCII);
   stFilterConfig.AddIDFormatPair(7, MESSAGE_ASCII, SECONDARY_ANTENNA);
   stFilterConfig.AddIDFormatPair(7, MESSAGE_ASCII, PRIMARY_ANTENNA);
   stFilterConfig.IsNegativeFilter(FALSE);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageFormat(MESSAGE_ASCII);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(7);
   clBaseMessageData.setMessageSource(SECONDARY_ANTENNA);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageSource(PRIMARY_ANTENNA);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)3, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Negative Filter Test
TEST_F(MessageFilterTest, NegativeFilter)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageID(5);
   clBaseMessageData.setMessageFormat(MESSAGE_BINARY);

   FilterConfig stFilterConfig;
   stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY);
   stFilterConfig.AddIDFormatPair(6, MESSAGE_BINARY);
   stFilterConfig.AddIDFormatPair(7, MESSAGE_BINARY, SECONDARY_ANTENNA);
   stFilterConfig.IsNegativeFilter(TRUE);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)0, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(4);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageSource(SECONDARY_ANTENNA);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());
      
   delete pMyLocalTestCommand;
}
