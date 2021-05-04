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
#include "decoders/novatel/api/filters/sourcefilter.hpp"
#include "string"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class SourceFilterTest : public ::testing::Test {
public:
   virtual void SetUp() {
   }

   virtual void TearDown() {
   }

   size_t GetSourceFormatPairSize() { return pMyTestCommand->pMySourceFormatPair.size(); }
private:
protected:
   SourceFilter* pMyTestCommand = NULL;
};

// ConfigureFilter Test
TEST_F(SourceFilterTest, ConfigureFilter)
{
   FilterConfig stFilterConfig;
   stFilterConfig.AddSourceFormatPair(SECONDARY_ANTENNA, MESSAGE_BINARY);

   pMyTestCommand = new SourceFilter();
   pMyTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(GetSourceFormatPairSize(), stFilterConfig.pSourceFormatPair.size());

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

// Filter Test
TEST_F(SourceFilterTest, SecondaryAntenna)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageID(5);
   clBaseMessageData.setMessageSource(MessageAntennaSourceEnum::SECONDARY_ANTENNA);
   clBaseMessageData.setMessageFormat(MessageFormatEnum::MESSAGE_ASCII);

   FilterConfig stFilterConfig;
   stFilterConfig.AddSourceFormatPair(MessageAntennaSourceEnum::SECONDARY_ANTENNA, MessageFormatEnum::MESSAGE_ASCII);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(SOURCE_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageFormat(MESSAGE_BINARY);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Filter Test
TEST_F(SourceFilterTest, PrimaryAntenna)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageID(5);
   clBaseMessageData.setMessageSource(MessageAntennaSourceEnum::PRIMARY_ANTENNA);
   clBaseMessageData.setMessageFormat(MESSAGE_BINARY);

   FilterConfig stFilterConfig;
   stFilterConfig.AddSourceFormatPair(PRIMARY_ANTENNA, MESSAGE_BINARY);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(SOURCE_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageSource(SECONDARY_ANTENNA);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Reset
TEST_F(SourceFilterTest, Reset)
{
   pMyTestCommand = new SourceFilter();
   pMyTestCommand->Reset();

   ASSERT_EQ(0.0, pMyTestCommand->GetFilterCount());
   delete pMyTestCommand;
}
