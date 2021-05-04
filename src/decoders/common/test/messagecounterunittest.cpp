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
//  DESCRIPTION:
//    Unit test cases for message counter
//
////////////////////////////////////////////////////////////////////////////////
#include "common/api/messagecounter.hpp"

#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class MessageCounterTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:
};

TEST_F(MessageCounterTest, AddMessage)
{
   MessageCounter *pMessageCounter = new MessageCounter();
   MessageHeader stMessageHeader;
   stMessageHeader.uiMessageID = 42;
   stMessageHeader.uiMessageLength = 100;
   stMessageHeader.szMessageName = "BESTPOS";
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_BINARY;
   stMessageHeader.ulMessageWeek = 1989;
   pMessageCounter->AddNewMessage(&stMessageHeader);

   ULONG ulBinMsgCount = (ULONG)pMessageCounter->GetBinaryMessageStatistics().size();
   ASSERT_TRUE(ulBinMsgCount == 1);

   ULONG ulAscMsgCount = (ULONG)pMessageCounter->GetAsciiMessageStatistics().size();
   ASSERT_TRUE(ulAscMsgCount == 0);

   std::map<UINT, MessageInfo> mBinMessageStats = pMessageCounter->GetBinaryMessageStatistics();
   ASSERT_TRUE(mBinMessageStats[42].uiBinaryMessages == 1);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}

TEST_F(MessageCounterTest, UpdateMessage)
{
   MessageCounter *pMessageCounter = new MessageCounter();

   MessageHeader stMessageHeader1;
   stMessageHeader1.uiMessageID = 42;
   stMessageHeader1.uiMessageLength = 100;
   stMessageHeader1.szMessageName = "BESTPOS";
   stMessageHeader1.eMessageFormat = MessageFormatEnum::MESSAGE_BINARY;
   stMessageHeader1.ulMessageWeek = 1989;
   pMessageCounter->AddNewMessage(&stMessageHeader1);

   MessageHeader stMessageHeader2;
   stMessageHeader2.uiMessageID = 42;
   stMessageHeader2.uiMessageLength = 100;
   stMessageHeader2.szMessageName = "BESTPOS";
   stMessageHeader2.eMessageFormat = MessageFormatEnum::MESSAGE_BINARY;
   stMessageHeader2.ulMessageWeek = 1989;
   pMessageCounter->AddNewMessage(&stMessageHeader2);

   MessageHeader stMessageHeader3;
   stMessageHeader3.uiMessageID = 42;
   stMessageHeader3.uiMessageLength = 100;
   stMessageHeader3.szMessageName = "BESTPOS";
   stMessageHeader3.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   stMessageHeader3.ulMessageWeek = 1989;
   pMessageCounter->AddNewMessage(&stMessageHeader3);

   ULONG ulAscMsgCount = (ULONG)pMessageCounter->GetAsciiMessageStatistics().size();
   ASSERT_TRUE(ulAscMsgCount == 1);

   ULONG ulBinMsgCount = (ULONG)pMessageCounter->GetBinaryMessageStatistics().size();
   ASSERT_TRUE(ulBinMsgCount == 1);

   std::map<UINT, MessageInfo> mBinMessageStats = pMessageCounter->GetBinaryMessageStatistics();
   ASSERT_TRUE(mBinMessageStats[42].uiBinaryMessages == 2);

   std::map<std::string, MessageInfo> mAsciiMessageStats = pMessageCounter->GetAsciiMessageStatistics();
   ASSERT_TRUE(mAsciiMessageStats["BESTPOS"].uiAsciiMessages == 1);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}

TEST_F(MessageCounterTest, GetDecoderStatistics)
{
   MessageCounter *pMessageCounter = new MessageCounter();

   MessageHeader stMessageHeader3;
   stMessageHeader3.uiMessageID = 42;
   stMessageHeader3.uiMessageLength = 100;
   stMessageHeader3.szMessageName = "BESTPOS";
   stMessageHeader3.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   stMessageHeader3.ulMessageTime = 496230;
   stMessageHeader3.ulMessageWeek = 1364;
   stMessageHeader3.eMessageTimeStatus = TIME_FINE;
   pMessageCounter->AddNewMessage(&stMessageHeader3);

   DecoderStatistics decoderStats = pMessageCounter->GetDecoderStatistics();
   ASSERT_TRUE(decoderStats.ulTotalUniqueMessages == 1);
   ASSERT_TRUE(decoderStats.ulStartWeek == 1364);
   ASSERT_TRUE(decoderStats.ulStartTimeMSec == 496230);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}

TEST_F(MessageCounterTest, AddNewMessage)
{
   MessageCounter *pMessageCounter = new MessageCounter();
   MessageHeader stMessageHeader;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_UNKNOWN;
   pMessageCounter->AddNewMessage(&stMessageHeader);

   ULONG ulBinMsgCount = (ULONG)pMessageCounter->GetBinaryMessageStatistics().size();
   ASSERT_TRUE(ulBinMsgCount == 0);

   ULONG ulAscMsgCount = (ULONG)pMessageCounter->GetAsciiMessageStatistics().size();
   ASSERT_TRUE(ulAscMsgCount == 0);

   std::map<UINT, MessageInfo> mBinMessageStats = pMessageCounter->GetBinaryMessageStatistics();
   ASSERT_TRUE(mBinMessageStats.size() == 0);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}

TEST_F(MessageCounterTest, AddNewMessage_WithTimeEnableFix)
{
   MessageCounter *pMessageCounter = new MessageCounter();
   MessageHeader stMessageHeader;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   stMessageHeader.uiMessageID = 8;
   pMessageCounter->EnableTimeIssueFix(TRUE);
   pMessageCounter->AddNewMessage(&stMessageHeader);

   ULONG ulAscMsgCount = (ULONG)pMessageCounter->GetAsciiMessageStatistics().size();
   ASSERT_TRUE(ulAscMsgCount == 0);

   std::map<std::string, MessageInfo> mAsciiMessageStats = pMessageCounter->GetAsciiMessageStatistics();
   ASSERT_TRUE(mAsciiMessageStats.size() == 0);

   stMessageHeader.uiMessageID = 1347;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   stMessageHeader.ulMessageWeek = 0;
   pMessageCounter->AddNewMessage(&stMessageHeader);

   ulAscMsgCount = (ULONG)pMessageCounter->GetAsciiMessageStatistics().size();
   ASSERT_TRUE(ulAscMsgCount == 0);

   mAsciiMessageStats = pMessageCounter->GetAsciiMessageStatistics();
   ASSERT_TRUE(mAsciiMessageStats.size() == 0);

   pMessageCounter->EnableTimeIssueFix(FALSE);
   stMessageHeader.uiMessageID = 1347;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   stMessageHeader.ulMessageWeek = 0;
   pMessageCounter->AddNewMessage(&stMessageHeader);

   ulAscMsgCount = (ULONG)pMessageCounter->GetAsciiMessageStatistics().size();
   ASSERT_TRUE(ulAscMsgCount == 0);   

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}

TEST_F(MessageCounterTest, CountAsciiMessage)
{
   MessageCounter *pMessageCounter = new MessageCounter();
   MessageHeader stMessageHeader;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;
   stMessageHeader.uiMessageID = 20;
   stMessageHeader.szMessageName = "BESTPOS";
   pMessageCounter->Reset();

   pMessageCounter->CountMessage(&stMessageHeader);
   std::map<std::string, MessageInfo> mAsciiMessageStats = pMessageCounter->GetAsciiMessageStatisticsWithoutFilter();
   ASSERT_TRUE(mAsciiMessageStats.size() == 1);
   ASSERT_TRUE(mAsciiMessageStats["BESTPOS"].uiAsciiMessages == 1);

   pMessageCounter->CountMessage(&stMessageHeader);
   mAsciiMessageStats = pMessageCounter->GetAsciiMessageStatisticsWithoutFilter();
   ASSERT_TRUE(mAsciiMessageStats.size() == 1);
   ASSERT_TRUE(mAsciiMessageStats["BESTPOS"].uiAsciiMessages == 2);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}


TEST_F(MessageCounterTest, CountBinaryMessage)
{
   MessageCounter *pMessageCounter = new MessageCounter();
   MessageHeader stMessageHeader;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_BINARY;
   stMessageHeader.uiMessageID = 20;
   stMessageHeader.szMessageName = "BESTPOS";
   pMessageCounter->Reset();

   pMessageCounter->CountMessage(&stMessageHeader);
   std::map<UINT, MessageInfo> mBinMessageStats = pMessageCounter->GetBinaryMessageStatisticsWithoutFilter();
   ASSERT_TRUE(mBinMessageStats.size() == 1);
   ASSERT_TRUE(mBinMessageStats[20].uiBinaryMessages == 1);

   pMessageCounter->CountMessage(&stMessageHeader);
   mBinMessageStats = pMessageCounter->GetBinaryMessageStatisticsWithoutFilter();
   ASSERT_TRUE(mBinMessageStats.size() == 1);
   ASSERT_TRUE(mBinMessageStats[20].uiBinaryMessages == 2);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}

TEST_F(MessageCounterTest, UpdateMsg_Ascii)
{
   MessageCounter *pMessageCounter = new MessageCounter();
   MessageHeader stMessageHeader;
   stMessageHeader.uiMessageID = 20;
   stMessageHeader.szMessageName = "BESTPOS"; 
   stMessageHeader.ulMessageWeek = 2021;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_ASCII;

   pMessageCounter->AddNewMessage(&stMessageHeader);
   pMessageCounter->AddNewMessage(&stMessageHeader);
   std::map<std::string, MessageInfo> mAsciiMessageStats = pMessageCounter->GetAsciiMessageStatistics();
   ASSERT_TRUE(mAsciiMessageStats["BESTPOS"].uiAsciiMessages == 2);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}

TEST_F(MessageCounterTest, UpdateMsg_NMEA2000)
{
   MessageCounter *pMessageCounter = new MessageCounter();
   MessageHeader stMessageHeader;
   stMessageHeader.uiMessageID = 20;
   stMessageHeader.szMessageName = "BESTPOS"; 
   stMessageHeader.ulMessageWeek = 2021;
   stMessageHeader.eMessageFormat = MessageFormatEnum::MESSAGE_NMEA2000;

   pMessageCounter->AddNewMessage(&stMessageHeader);
   pMessageCounter->AddNewMessage(&stMessageHeader);
   std::map<UINT, MessageInfo> mBinMessageStats = pMessageCounter->GetBinaryMessageStatistics();
   ASSERT_TRUE(mBinMessageStats[20].uiBinaryMessages == 2);

   if( pMessageCounter != NULL )
   {
      delete pMessageCounter;
      pMessageCounter = NULL;
   }
}
