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
#include "filters/decimationfilter.hpp"

BOOL IsEqual(DOUBLE dFirst, DOUBLE dSecond)
{
   DOUBLE epsilon = 0.001;
   return std::abs(dFirst - dSecond) < epsilon;
}

// code
// ---------------------------------------------------------
DecimationFilter::DecimationFilter()
{
   dFilterCount = 0.0;
   dMySamplePeriod = 0.0;
   mMySamplePeriodMap.clear();
   szMyMessageName.clear();
}

// ---------------------------------------------------------
DecimationFilter::~DecimationFilter()
{
   mMySamplePeriodMap.clear();
   szMyMessageName.clear();
}

// ---------------------------------------------------------
BOOL DecimationFilter::Filter(BaseMessageData& clBaseMessageData)
{
   UINT uiMessageID = clBaseMessageData.getMessageID();
   std::string szMsgName = clBaseMessageData.getMessageName();
   ULONG ulMilliSeconds = clBaseMessageData.getMessageTimeMilliSeconds();
   DOUBLE dSeconds = ulMilliSeconds/1000.0;
   DOUBLE dFmodSeconds = ulMilliSeconds;
   DOUBLE dFmodSamplePeriod = dMySamplePeriod * 1000.0;

   // By default, messages with unknown and sattime statuses will always be passed.
   if ( (clBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_UNKNOWN) || 
      (clBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_SATTIME) )
      return TRUE;

   if (dFmodSamplePeriod == 0.0)
      return TRUE;

   // The message appears first time
   if( mMySamplePeriodMap.find(uiMessageID) == mMySamplePeriodMap.end() ) // not found
   {
      // Check if the current time is multiple of sample period
      if (fmod(dFmodSeconds, dFmodSamplePeriod) == 0.0)
      {
         if (szMyMessageName.size() != 0)
         {
            if (std::find (szMyMessageName.begin(), szMyMessageName.end(), szMsgName) != szMyMessageName.end())
            {
               // Store first time
               mMySamplePeriodMap.emplace(std::pair <UINT, DOUBLE> (uiMessageID, dSeconds));
               SetFilterCount();
               return TRUE;
            }
            else
            {
               return FALSE;
            }
         }
         // Store first time
         mMySamplePeriodMap.emplace(std::pair <UINT, DOUBLE> (uiMessageID, dSeconds));
         SetFilterCount();
         return TRUE;
      }
   }
   // Message already appeared previously.
   else
   {
      // Check if the current time is same as the stored time.Allow such messages
      if (IsEqual(dSeconds, mMySamplePeriodMap.find(uiMessageID)->second))
      {
         return TRUE;
      }

      // Check if the current time is multiple of sample period 
      if (fmod(dFmodSeconds, dFmodSamplePeriod) == 0.0)
      {
         // Update the stored
         mMySamplePeriodMap.find(uiMessageID)->second = dSeconds;
         SetFilterCount();
         return TRUE;
      }
   }
   return FALSE;
}

// ---------------------------------------------------------
void DecimationFilter::ConfigureFilter(FilterConfig& stFilterConfig)
{
   dMySamplePeriod = stFilterConfig.dSamplePeriod;
   copy(stFilterConfig.szMessageName.begin(), stFilterConfig.szMessageName.end(), back_inserter(szMyMessageName));
}

// ---------------------------------------------------------
DOUBLE DecimationFilter::GetFilterCount()
{
   return dFilterCount;
}

// ---------------------------------------------------------
void DecimationFilter::SetFilterCount()
{
   dFilterCount = dFilterCount + 1;
}

// ---------------------------------------------------------
void DecimationFilter::Reset()
{
   dFilterCount = 0.0;
   mMySamplePeriodMap.clear();
}
