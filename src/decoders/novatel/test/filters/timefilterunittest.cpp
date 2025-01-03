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
#include "decoders/novatel/api/filters/timefilter.hpp"
#include "string"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class TimeFilterTest : public ::testing::Test {
public:
   virtual void SetUp() {
   }

   virtual void TearDown() {
   }

   size_t GetStartPairSize() { return pMyTestCommand->ulMyStartPair.size(); }
   size_t GetEndPairSize()   { return pMyTestCommand->ulMyEndPair.size(); }
   ULONG  GetStartPairFirstValue(INT iIndex) { return pMyTestCommand->ulMyStartPair[iIndex].first; }
   ULONG  GetStartPairSecondValue(INT iIndex) { return pMyTestCommand->ulMyStartPair[iIndex].second; }
   ULONG  GetEndPairFirstValue(INT iIndex) { return pMyTestCommand->ulMyEndPair[iIndex].first; }
   ULONG  GetEndPairSecondValue(INT iIndex) { return pMyTestCommand->ulMyEndPair[iIndex].second; }

private:
protected:
   TimeFilter* pMyTestCommand = NULL;
};

// ConfigureFilter Test
TEST_F(TimeFilterTest, ConfigureFilter)
{
   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(496230000, 1364);
   stFilterConfig.AddEndTimePair(497560000, 1365);

   pMyTestCommand = new TimeFilter();
   pMyTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(GetStartPairSize(), stFilterConfig.ulStartTimePair.size());
   ASSERT_EQ(GetEndPairSize(), stFilterConfig.ulEndTimePair.size());

   ASSERT_EQ(GetStartPairFirstValue(0), stFilterConfig.ulStartTimePair[0].first);
   ASSERT_EQ(GetStartPairSecondValue(0), stFilterConfig.ulStartTimePair[0].second);

   ASSERT_EQ(GetEndPairFirstValue(0), stFilterConfig.ulEndTimePair[0].first);
   ASSERT_EQ(GetEndPairSecondValue(0), stFilterConfig.ulEndTimePair[0].second);

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}


// Filter Test
TEST_F(TimeFilterTest, Filter)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(1364);
   clBaseMessageData.setMessageTimeMilliSeconds(496330000);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(496230000, 1364);
   stFilterConfig.AddEndTimePair(497560000, 1365);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Filter Test Start Time and End Time with Week change in end Time
TEST_F(TimeFilterTest, Filter_EndWeek)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(2035);
   clBaseMessageData.setMessageTimeMilliSeconds(10);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(9, 2035);
   stFilterConfig.AddEndTimePair(7, 2036);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Filter Test In between Start Time and End Time with Week change in end Time
TEST_F(TimeFilterTest, Filter_InBwStartEndTime)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(2036);
   clBaseMessageData.setMessageTimeMilliSeconds(3);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(9, 2035);
   stFilterConfig.AddEndTimePair(7, 2036);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Filter Test Smaller Then Start Time
TEST_F(TimeFilterTest, Filter_SmallerStartTime)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(2035);
   clBaseMessageData.setMessageTimeMilliSeconds(7);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(9, 2035);
   stFilterConfig.AddEndTimePair(7, 2036);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)0, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Filter Test Time is more then End Time
TEST_F(TimeFilterTest, Filter_GreaterEndWeek)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(2036);
   clBaseMessageData.setMessageTimeMilliSeconds(10);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(9, 2035);
   stFilterConfig.AddEndTimePair(7, 2036);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)0, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Filter  Start Time
TEST_F(TimeFilterTest, Filter_StartTime)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(2035);
   clBaseMessageData.setMessageTimeMilliSeconds(9);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(9, 2035);
   stFilterConfig.AddEndTimePair(7, 2036);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)0, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

// Filter End Time
TEST_F(TimeFilterTest, Filter_EndTime)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(2036);
   clBaseMessageData.setMessageTimeMilliSeconds(7);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(9, 2035);
   stFilterConfig.AddEndTimePair(7, 2036);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)0, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

TEST_F(TimeFilterTest, FilterWithoutStartWeek)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(1364);
   clBaseMessageData.setMessageTimeMilliSeconds(496330000);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(496230000);
   stFilterConfig.AddEndTimePair(497560000, 1365);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

TEST_F(TimeFilterTest, FilterWithoutEndWeek)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(1364);
   clBaseMessageData.setMessageTimeMilliSeconds(496330000);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(496230000, 1364);
   stFilterConfig.AddEndTimePair(497560000);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}

TEST_F(TimeFilterTest, FilterWithoutWeek)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeWeek(1364);
   clBaseMessageData.setMessageTimeMilliSeconds(496330000);
   clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

   FilterConfig stFilterConfig;
   stFilterConfig.AddStartTimePair(496230000);
   stFilterConfig.AddEndTimePair(497560000);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.IsNegativeTimeFilter(FALSE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)2, pMyLocalTestCommand->GetFilterCount());

   delete pMyLocalTestCommand;
}
