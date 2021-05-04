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
#include "decoders/novatel/api/filters/filtercontainer.hpp"
#include "decoders/novatel/api/filters/messagefilter.hpp"
#include "decoders/novatel/api/filters/timefilter.hpp"
#include "string"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class FilterContainerTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:

protected:
};

// FilterContainer Test with only message filter
TEST_F(FilterContainerTest, Filter_with_messagefilter)
{
	MessageHeader stMessageHeader;
	char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
	strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
	stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
	BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
	clBaseMessageData.setMessageID(4);
	clBaseMessageData.setMessageFormat(MESSAGE_BINARY);
	clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

	FilterConfig stFilterConfig;

	stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY);
	stFilterConfig.AddIDFormatPair(6, MESSAGE_BINARY);

	stFilterConfig.IsNegativeFilter(TRUE);

	MessageDataFilter* pMyMessageFilterTestCommand = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
	pMyMessageFilterTestCommand->ConfigureFilter(stFilterConfig);

	stFilterConfig.AddFilterToContainer(pMyMessageFilterTestCommand);


	MessageDataFilter* pMyFilterContainerTestCommand = MessageDataFilter::CreateFilter(CONTAINER_FILTER);
	pMyFilterContainerTestCommand->ConfigureFilter(stFilterConfig);

	ASSERT_EQ(TRUE, pMyFilterContainerTestCommand->Filter(clBaseMessageData));
	ASSERT_EQ((double)1, pMyFilterContainerTestCommand->GetFilterCount());

	delete pMyMessageFilterTestCommand;
	delete pMyFilterContainerTestCommand;
}

// FilterContainer Test with only time filter
TEST_F(FilterContainerTest, Filter_with_timefilter)
{
	MessageHeader stMessageHeader;
	char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
	strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
	stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
	BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
	clBaseMessageData.setMessageTimeWeek(1364);
	clBaseMessageData.setMessageTimeMilliSeconds(496330);

	clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

	FilterConfig stFilterConfig;
	stFilterConfig.AddStartTimePair(496230, 1364);
	stFilterConfig.AddEndTimePair(497560, 1365);

	MessageDataFilter* pMyTimeFilterTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
	pMyTimeFilterTestCommand->ConfigureFilter(stFilterConfig);

	stFilterConfig.AddFilterToContainer(pMyTimeFilterTestCommand);

	MessageDataFilter* pMyFilterContainerTestCommand = MessageDataFilter::CreateFilter(CONTAINER_FILTER);
	pMyFilterContainerTestCommand->ConfigureFilter(stFilterConfig);

	ASSERT_EQ(TRUE, pMyFilterContainerTestCommand->Filter(clBaseMessageData));
	ASSERT_EQ((double)1, pMyFilterContainerTestCommand->GetFilterCount());

	delete pMyTimeFilterTestCommand;
	delete pMyFilterContainerTestCommand;
}

// FilterContainer Test
TEST_F(FilterContainerTest, Filter)
{
	MessageHeader stMessageHeader;
	char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
	strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
	stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
	BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
	clBaseMessageData.setMessageTimeWeek(1364);
	clBaseMessageData.setMessageTimeMilliSeconds(496330);
	clBaseMessageData.setMessageID(4);
	clBaseMessageData.setMessageFormat(MESSAGE_BINARY);
	clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

	FilterConfig stFilterConfig;
	stFilterConfig.AddStartTimePair(496230, 1364);
	stFilterConfig.AddEndTimePair(497560, 1365);

	stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY);
	stFilterConfig.AddIDFormatPair(6, MESSAGE_BINARY);

	stFilterConfig.IsNegativeFilter(TRUE);

	MessageDataFilter* pMyMessageFilterTestCommand = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
	pMyMessageFilterTestCommand->ConfigureFilter(stFilterConfig);

	MessageDataFilter* pMyTimeFilterTestCommand = MessageDataFilter::CreateFilter(TIME_FILTER);
	pMyTimeFilterTestCommand->ConfigureFilter(stFilterConfig);

	stFilterConfig.AddFilterToContainer(pMyMessageFilterTestCommand);
	stFilterConfig.AddFilterToContainer(pMyTimeFilterTestCommand);

	MessageDataFilter* pMyFilterContainerTestCommand = MessageDataFilter::CreateFilter(CONTAINER_FILTER);
	pMyFilterContainerTestCommand->ConfigureFilter(stFilterConfig);

	ASSERT_EQ(TRUE, pMyFilterContainerTestCommand->Filter(clBaseMessageData));
	ASSERT_EQ((double)1, pMyFilterContainerTestCommand->GetFilterCount());

	delete pMyMessageFilterTestCommand;
	delete pMyTimeFilterTestCommand;
	delete pMyFilterContainerTestCommand;
}

//Reset
TEST_F(FilterContainerTest, Reset)
{
   FilterConfig stFilterConfig;
   stFilterConfig.AddIDFormatPair(5, MESSAGE_BINARY);
   stFilterConfig.AddIDFormatPair(6, MESSAGE_BINARY);
   MessageDataFilter* pMyMessageFilterTestCommand = MessageDataFilter::CreateFilter(MESSAGE_FILTER);
   pMyMessageFilterTestCommand->ConfigureFilter(stFilterConfig);
   stFilterConfig.AddFilterToContainer(pMyMessageFilterTestCommand);
   MessageDataFilter* pMyFilterContainerTestCommand = MessageDataFilter::CreateFilter(CONTAINER_FILTER);
   pMyFilterContainerTestCommand->ConfigureFilter(stFilterConfig);		
   pMyFilterContainerTestCommand->Reset();
   ASSERT_EQ(0.0, pMyFilterContainerTestCommand->GetFilterCount());
   delete pMyFilterContainerTestCommand;
}

// FilterContainer Test with only source filter
TEST_F(FilterContainerTest, Filter_with_source)
{
	MessageHeader stMessageHeader;
	char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
	strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
	stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
	BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
	clBaseMessageData.setMessageSource(MessageAntennaSourceEnum::SECONDARY_ANTENNA);
	clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

	FilterConfig stFilterConfig;
	stFilterConfig.AddSourceFormatPair(MessageAntennaSourceEnum::SECONDARY_ANTENNA, MessageFormatEnum::MESSAGE_ASCII);

	MessageDataFilter* pMyTimeFilterTestCommand = MessageDataFilter::CreateFilter(SOURCE_FILTER);
	pMyTimeFilterTestCommand->ConfigureFilter(stFilterConfig);

	stFilterConfig.AddFilterToContainer(pMyTimeFilterTestCommand);

	MessageDataFilter* pMyFilterContainerTestCommand = MessageDataFilter::CreateFilter(CONTAINER_FILTER);
	pMyFilterContainerTestCommand->ConfigureFilter(stFilterConfig);

	ASSERT_EQ(TRUE, pMyFilterContainerTestCommand->Filter(clBaseMessageData));
	ASSERT_EQ((double)1, pMyFilterContainerTestCommand->GetFilterCount());

	delete pMyTimeFilterTestCommand;
	delete pMyFilterContainerTestCommand;
}

// FilterContainer Test with only sattime filter
TEST_F(FilterContainerTest, Filter_with_sattime)
{
	MessageHeader stMessageHeader;
	char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
	strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
	stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
	BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
	clBaseMessageData.setMessageTimeStatus(TIME_SATTIME);

	FilterConfig stFilterConfig;
    stFilterConfig.EnableSatTimeFilter(FALSE);

	MessageDataFilter* pMyTimeFilterTestCommand = MessageDataFilter::CreateFilter(SATTIME_FILTER);
	pMyTimeFilterTestCommand->ConfigureFilter(stFilterConfig);

	stFilterConfig.AddFilterToContainer(pMyTimeFilterTestCommand);

	MessageDataFilter* pMyFilterContainerTestCommand = MessageDataFilter::CreateFilter(CONTAINER_FILTER);
	pMyFilterContainerTestCommand->ConfigureFilter(stFilterConfig);

	ASSERT_EQ(FALSE, pMyFilterContainerTestCommand->Filter(clBaseMessageData));
	ASSERT_EQ((double)0, pMyFilterContainerTestCommand->GetFilterCount());

	delete pMyTimeFilterTestCommand;
	delete pMyFilterContainerTestCommand;
}

// FilterContainer Test with source and decimation
TEST_F(FilterContainerTest, Filter_with_source_decimation_filter)
{
	MessageHeader stMessageHeader;
	char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
	strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
	stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
	BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
	clBaseMessageData.setMessageSource(MessageAntennaSourceEnum::SECONDARY_ANTENNA);
	clBaseMessageData.setMessageFormat(MessageFormatEnum::MESSAGE_ASCII);
    clBaseMessageData.setMessageTimeMilliSeconds(543900000);
    clBaseMessageData.setMessageTimeStatus(TIME_FINEADJUSTING);

	FilterConfig stFilterConfig;
	stFilterConfig.AddSourceFormatPair(MessageAntennaSourceEnum::SECONDARY_ANTENNA, MessageFormatEnum::MESSAGE_ASCII);

	stFilterConfig.AddSamplePeriod(2.0);

	MessageDataFilter* pMyMessageFilterTestCommand = MessageDataFilter::CreateFilter(SOURCE_FILTER);
	pMyMessageFilterTestCommand->ConfigureFilter(stFilterConfig);

	MessageDataFilter* pMyTimeFilterTestCommand = MessageDataFilter::CreateFilter(DECIMATION_FILTER);
	pMyTimeFilterTestCommand->ConfigureFilter(stFilterConfig);

	stFilterConfig.AddFilterToContainer(pMyMessageFilterTestCommand);
	stFilterConfig.AddFilterToContainer(pMyTimeFilterTestCommand);

	MessageDataFilter* pMyFilterContainerTestCommand = MessageDataFilter::CreateFilter(CONTAINER_FILTER);
	pMyFilterContainerTestCommand->ConfigureFilter(stFilterConfig);

	ASSERT_EQ(TRUE, pMyFilterContainerTestCommand->Filter(clBaseMessageData));
	ASSERT_EQ((double)1, pMyFilterContainerTestCommand->GetFilterCount());

	delete pMyMessageFilterTestCommand;
	delete pMyTimeFilterTestCommand;
	delete pMyFilterContainerTestCommand;
}
