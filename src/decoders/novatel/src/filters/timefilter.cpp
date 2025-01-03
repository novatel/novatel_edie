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
#include "filters/timefilter.hpp"

// code
// ---------------------------------------------------------
TimeFilter::TimeFilter()
{
   dFilterCount = 0;
   bMyIsNegativeTimeFilter = FALSE;
   ulMyStartPair.clear();
   ulMyEndPair.clear();
}

// ---------------------------------------------------------
TimeFilter::~TimeFilter()
{
   ulMyStartPair.clear();
   ulMyEndPair.clear();
}

// ---------------------------------------------------------
BOOL TimeFilter::Filter(BaseMessageData& clBaseMessageData)
{
   ULONG ulGnssWeek = clBaseMessageData.getMessageTimeWeek();
   ULONG ulMilliSeconds = clBaseMessageData.getMessageTimeMilliSeconds();


   // By default, messages with unknown and sattime statuses will always be passed.
   if ( (clBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_UNKNOWN) || 
      (clBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_SATTIME) )
      return TRUE;

   if(ulMyStartPair[0].second == 0)
   {
      ulMyStartPair[0].second = ulGnssWeek;
   }
   if(ulMyEndPair[0].second == 0)
   {
      ulMyEndPair[0].second = ulMyStartPair[0].second;
   }

   // Case of size 0 will generate exception, because we will be trying to access vector without initialization.
   if ((ulMyStartPair.size() != 0) && (ulMyEndPair.size() != 0))
   {
      // If end pair is less then start pair
      if( (ulMyStartPair[0].first > ulMyEndPair[0].first) && (ulMyStartPair[0].second == ulMyEndPair[0].second) )
      {
         return FALSE;
      }
      // If Configured pair and start pair is same or less then
      // If Configured pair and end pair is same or greater then
      else if( ((ulMilliSeconds <= ulMyStartPair[0].first)  && (ulGnssWeek <= ulMyStartPair[0].second)) 
         || ((ulMilliSeconds >= ulMyEndPair[0].first)  && (ulGnssWeek >= ulMyEndPair[0].second)) )
      {
         if (bMyIsNegativeTimeFilter == FALSE)
         {
            return FALSE;
         }
         else
         {
            SetFilterCount();
            return TRUE;
         }
      }
      // If in b/w
      else if( ((ulMilliSeconds > ulMyStartPair[0].first) && (ulGnssWeek >= ulMyStartPair[0].second)) 
         || ((ulMilliSeconds < ulMyEndPair[0].first) &&(ulGnssWeek <= ulMyEndPair[0].second))  )
      {
         if(ulMilliSeconds > ulMyEndPair[0].first && ulGnssWeek >= ulMyEndPair[0].second)
         {
            if (bMyIsNegativeTimeFilter == FALSE)
            {
               return FALSE;
            }
            else
            {
               SetFilterCount();
               return TRUE;
            }
         }
         else
         {
            if (bMyIsNegativeTimeFilter == FALSE)
            {
               SetFilterCount();
               return TRUE;
            }
            else
            {
               return FALSE;
            }
         }
      }
      // If in b/w
      else if( ((ulMilliSeconds < ulMyStartPair[0].first) && (ulGnssWeek >= ulMyStartPair[0].second)) 
         && ((ulMilliSeconds < ulMyEndPair[0].first) &&(ulGnssWeek == ulMyEndPair[0].second))  )
      {
         if (bMyIsNegativeTimeFilter == FALSE)
         {
            SetFilterCount();
            return TRUE;
         }
         else
         {
            return FALSE;
         }
      }
      else
      {
         return FALSE;
      }
   }
   else
   {
      throw nExcept("Time pair is not valid");
   }

}

// ---------------------------------------------------------
void TimeFilter::ConfigureFilter(FilterConfig& stFilterConfig)
{
   copy(stFilterConfig.ulStartTimePair.begin(), stFilterConfig.ulStartTimePair.end(), back_inserter(ulMyStartPair));
   copy(stFilterConfig.ulEndTimePair.begin(), stFilterConfig.ulEndTimePair.end(), back_inserter(ulMyEndPair));
   bMyIsNegativeTimeFilter = stFilterConfig.bIsNegTimeFilter;
}

// ---------------------------------------------------------
DOUBLE TimeFilter::GetFilterCount()
{
   return dFilterCount;
}

// ---------------------------------------------------------
void TimeFilter::SetFilterCount()
{
   dFilterCount = dFilterCount + 1;
}

// ---------------------------------------------------------
void TimeFilter::Reset()
{
   dFilterCount = 0;
}


