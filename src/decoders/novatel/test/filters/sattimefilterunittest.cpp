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
#include "decoders/novatel/api/filters/sattimefilter.hpp"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class SatTimeFilterTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  BOOL IsSatTimeFilter() { return pMyTestCommand->bMySatTimeFilter; }
private:
protected:
   SatTimeFilter* pMyTestCommand = NULL;
};

// ConfigureFilter Test
TEST_F(SatTimeFilterTest, Filter_with_messagefilter)
{
   FilterConfig stFilterConfig;
   stFilterConfig.EnableSatTimeFilter(FALSE);

   pMyTestCommand = new SatTimeFilter();
   pMyTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, IsSatTimeFilter());

   delete pMyTestCommand;
   pMyTestCommand = nullptr;
}

// Filter Test
TEST_F(SatTimeFilterTest, Filter)
{
   MessageHeader stMessageHeader;
   char* chDecodedMessage = new char [strlen("Only to create BaseMessageData instance") + 1];
   strcpy(chDecodedMessage, "Only to create BaseMessageData instance");
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   BaseMessageData clBaseMessageData(&stMessageHeader, chDecodedMessage);
   clBaseMessageData.setMessageTimeStatus(TIME_SATTIME);

   FilterConfig stFilterConfig;
   stFilterConfig.EnableSatTimeFilter(FALSE);

   MessageDataFilter* pMyLocalTestCommand = MessageDataFilter::CreateFilter(SATTIME_FILTER);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(FALSE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)0, pMyLocalTestCommand->GetFilterCount());

   stFilterConfig.EnableSatTimeFilter(TRUE);
   pMyLocalTestCommand->ConfigureFilter(stFilterConfig);

   ASSERT_EQ(TRUE, pMyLocalTestCommand->Filter(clBaseMessageData));
   ASSERT_EQ((double)1, pMyLocalTestCommand->GetFilterCount());

	delete pMyLocalTestCommand;
}

// Reset
TEST_F(SatTimeFilterTest, Reset)
{
   pMyTestCommand = new SatTimeFilter();
   pMyTestCommand->Reset();

   ASSERT_EQ(0.0, pMyTestCommand->GetFilterCount());
   delete pMyTestCommand;
}
