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
//    Class to count the decoder message statistics
//
////////////////////////////////////////////////////////////////////////////////
#include "messagecounter.hpp"
#include <string.h>
// -------------------------------------------------------------------------------------------------------
MessageCounter::MessageCounter()
{
   bMyEnableTimeIssueFix = FALSE;
}

// -------------------------------------------------------------------------------------------------------
void MessageCounter::EnableTimeIssueFix(BOOL bEnable)
{
   bMyEnableTimeIssueFix = bEnable;
}

// -------------------------------------------------------------------------------------------------------
void MessageCounter::AddNewMessage(MessageHeader* stMessageHeader)
{
   // Ignore UNKNOWN Format Messages
   if (stMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_UNKNOWN)
      return;

   // If time fix is enabled, don't consider IONUTC and QZSSIONUTC messages to update start and end times and return from here.
   if ((bMyEnableTimeIssueFix == TRUE) && (stMessageHeader->uiMessageID == 8 || stMessageHeader->uiMessageID == 1347))
   {
      return;
   }

   // Ignore messages with week 0 to update start and end times
   if (stMessageHeader->ulMessageWeek == 0)
	   return;      

   // Check if message already exists in map
   if (UpdateMessage(stMessageHeader) == FALSE)
   {
      // It's a new message. Add to map
      stMyDecoderStatistics.ulTotalUniqueMessages++;

      if (stMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_BINARY)
      {
         mBinaryMessageCountMap[stMessageHeader->uiMessageID].uiBinaryMessages = 1;
         mBinaryMessageCountMap[stMessageHeader->uiMessageID].uiMessageID = stMessageHeader->uiMessageID;
      }
      else if (stMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_ASCII)
      {
         mAsciiMessageCountMap[stMessageHeader->szMessageName].uiAsciiMessages = 1;
         mAsciiMessageCountMap[stMessageHeader->szMessageName].uiMessageID = stMessageHeader->uiMessageID;
      }
      else if (stMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
      {
         mBinaryMessageCountMap[stMessageHeader->uiMessageID].uiBinaryMessages = 1;
         mBinaryMessageCountMap[stMessageHeader->uiMessageID].uiMessageID = stMessageHeader->uiMessageID;
      }
   }

   // Don't record UNKNOWN, COARSEADJUSTING and SATTIME
   if ((stMessageHeader->eMessageTimeStatus != MessageTimeStatusEnum::TIME_UNKNOWN) &&
       (stMessageHeader->eMessageTimeStatus != MessageTimeStatusEnum::TIME_SATTIME) &&
       (stMessageHeader->eMessageTimeStatus != MessageTimeStatusEnum::TIME_COARSEADJUSTING))
   {
      // Set the start week if it's not set
      // Set the new start time if current time is less than existing start time
      if ((stMyDecoderStatistics.ulStartWeek == 0) ||
          (stMyDecoderStatistics.ulStartWeek > stMessageHeader->ulMessageWeek) ||
          (stMyDecoderStatistics.ulStartWeek == stMessageHeader->ulMessageWeek && stMyDecoderStatistics.ulStartTimeMSec > stMessageHeader->ulMessageTime))
      {
         stMyDecoderStatistics.ulStartWeek = stMessageHeader->ulMessageWeek;
         stMyDecoderStatistics.ulStartTimeMSec = stMessageHeader->ulMessageTime;
      }
      // Set the end week if it's not set
      // Set the new end week if current time is greater than existing end time
      if ((stMyDecoderStatistics.ulEndWeek == 0) ||
          (stMyDecoderStatistics.ulEndWeek < stMessageHeader->ulMessageWeek) ||
          (stMyDecoderStatistics.ulEndWeek == stMessageHeader->ulMessageWeek && stMyDecoderStatistics.ulEndTimeMSec < stMessageHeader->ulMessageTime))
      {
         stMyDecoderStatistics.ulEndWeek = stMessageHeader->ulMessageWeek;
         stMyDecoderStatistics.ulEndTimeMSec = stMessageHeader->ulMessageTime;
      }
   }
}

// -------------------------------------------------------------------------------------------------------
BOOL MessageCounter::UpdateMessage(MessageHeader* stMessageHeader)
{
   if (stMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_ASCII)
   {
      if (mAsciiMessageCountMap.find(stMessageHeader->szMessageName) == mAsciiMessageCountMap.end())
      {
         // Didn't find message in map
         return FALSE;
      }
      else
      {
         // Found message in map. Update it's count
         mAsciiMessageCountMap[stMessageHeader->szMessageName].uiAsciiMessages++;
         return TRUE;
      }
   }
   else if (stMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_BINARY
      || stMessageHeader->eMessageFormat == MessageFormatEnum::MESSAGE_NMEA2000)
   {
      if (mBinaryMessageCountMap.find(stMessageHeader->uiMessageID) == mBinaryMessageCountMap.end())
      {
         // Didn't find message in map
         return FALSE;
      }
      else
      {
         // Found message in map. Update it's count
         mBinaryMessageCountMap[stMessageHeader->uiMessageID].uiBinaryMessages++;
         return TRUE;
      }
   }
   return FALSE;
}

// -------------------------------------------------------------------------------------------------------
void MessageCounter::CountMessage(MessageHeader* stMessageHeader)
{
   MessageFormatEnum eFormat = stMessageHeader->eMessageFormat;
   if (eFormat == MessageFormatEnum::MESSAGE_BINARY)
   {
      if (mTotalBinaryMessagesMap.find(stMessageHeader->uiMessageID) == mTotalBinaryMessagesMap.end())
      {
         mTotalBinaryMessagesMap[stMessageHeader->uiMessageID].uiBinaryMessages = 1;
      }
      else
      {
         mTotalBinaryMessagesMap[stMessageHeader->uiMessageID].uiBinaryMessages++;
      }
   }
   else if (eFormat == MessageFormatEnum::MESSAGE_ASCII)
   {
      if (mTotalAsciiMessagesMap.find(stMessageHeader->szMessageName) == mTotalAsciiMessagesMap.end())
      {
         mTotalAsciiMessagesMap[stMessageHeader->szMessageName].uiAsciiMessages = 1;
      }
      else
      {
         mTotalAsciiMessagesMap[stMessageHeader->szMessageName].uiAsciiMessages++;
      }
   }
   else
   {

   }
}

// -------------------------------------------------------------------------------------------------------
std::map<std::string, MessageInfo> MessageCounter::GetAsciiMessageStatistics()
{
   return mAsciiMessageCountMap;
}

std::map<UINT, MessageInfo> MessageCounter::GetBinaryMessageStatistics()
{
   return mBinaryMessageCountMap;
}

// -------------------------------------------------------------------------------------------------------
std::map<std::string, MessageInfo> MessageCounter::GetAsciiMessageStatisticsWithoutFilter() const
{
   return mTotalAsciiMessagesMap;
}

// -------------------------------------------------------------------------------------------------------
std::map<UINT, MessageInfo> MessageCounter::GetBinaryMessageStatisticsWithoutFilter() const
{
   return mTotalBinaryMessagesMap;
}

// -------------------------------------------------------------------------------------------------------
DecoderStatistics MessageCounter::GetDecoderStatistics() const
{
   return stMyDecoderStatistics;
}

// -------------------------------------------------------------------------------------------------------
void MessageCounter::Reset()
{
   memset(&stMyDecoderStatistics, 0x00, sizeof(stMyDecoderStatistics));

   mAsciiMessageCountMap.clear();
   mBinaryMessageCountMap.clear();

   mTotalAsciiMessagesMap.clear();
   mTotalBinaryMessagesMap.clear();
}


// -------------------------------------------------------------------------------------------------------
MessageCounter::~MessageCounter()
{

}
