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
#include "decoders/novatel/api/filters/decimationfilter.hpp"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class DecimationFilterTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  DOUBLE GetSamplePeriod() { return pMyTestCommand->dMySamplePeriod; }
  size_t GetSamplePeriodMapSize() { return pMyTestCommand->mMySamplePeriodMap.size(); }

private:

protected:
   DecimationFilter* pMyTestCommand = NULL;
};

TEST_F(DecimationFilterTest, IsEqual)
{
   pMyTestCommand = new DecimationFilter();

   ASSERT_EQ(TRUE, IsEqual(3.5, 3.5));
   ASSERT_EQ(TRUE, IsEqual(3.002, 3.002));

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

TEST_F(DecimationFilterTest, ConfigureFilter)
{
   FilterConfig stFilterConfig;
   pMyTestCommand = new DecimationFilter();

   stFilterConfig.AddSamplePeriod(2);
   pMyTestCommand->ConfigureFilter(stFilterConfig);
   ASSERT_EQ(GetSamplePeriod(), stFilterConfig.dSamplePeriod);

   stFilterConfig.AddSamplePeriod(2.0);
   pMyTestCommand->ConfigureFilter(stFilterConfig);
   ASSERT_EQ(2.0, stFilterConfig.dSamplePeriod);

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

TEST_F(DecimationFilterTest, Filter)
{
   FilterConfig stFilterConfig;
   stFilterConfig.AddSamplePeriod(2.0);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(DECIMATION_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);

   clBaseMessageData.setMessageID(5);
   clBaseMessageData.setMessageTimeMilliSeconds(543900000);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(10);
   clBaseMessageData.setMessageTimeMilliSeconds(543922000);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(15);
   clBaseMessageData.setMessageTimeMilliSeconds(543924000);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)3, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543926000);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)4, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543926000);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)4, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543928000);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)5, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543930000);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)6, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543931000);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)6, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543932000);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)7, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.AddSamplePeriod(.2);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);
   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543932500);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)7, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.AddSamplePeriod(0);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);
   clBaseMessageData.setMessageID(20);
   clBaseMessageData.setMessageTimeMilliSeconds(543932500);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)7, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageTimeStatus(TIME_UNKNOWN);
   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));

   delete pMyLocalTestCommand;
}

TEST_F(DecimationFilterTest, Filter_Message)
{
   FilterConfig stFilterConfig;
   stFilterConfig.AddSamplePeriod(2.0);
   stFilterConfig.AddMessageName("BESTPOS");
   stFilterConfig.AddMessageName("VERSION");

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(DECIMATION_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);

   clBaseMessageData.setMessageName("BESTPOS");
   clBaseMessageData.setMessageID(1);
   clBaseMessageData.setMessageTimeMilliSeconds(543900000);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageTimeMilliSeconds(544000000);
   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageName("LOGLIST");
   clBaseMessageData.setMessageID(2);
   clBaseMessageData.setMessageTimeMilliSeconds(544000000);
   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   clBaseMessageData.setMessageName("VERSION");
   clBaseMessageData.setMessageID(3);
   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)3, pMyLocalTestCommand->GetFilterCount());

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)3, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

TEST_F(DecimationFilterTest, Reset)
{
   pMyTestCommand = new DecimationFilter();

   pMyTestCommand->Reset();
   ASSERT_EQ(0.0, pMyTestCommand->GetFilterCount());
   ASSERT_EQ((size_t)0, GetSamplePeriodMapSize());

   delete pMyTestCommand;
}
