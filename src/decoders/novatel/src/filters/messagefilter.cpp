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
#include "filters/messagefilter.hpp"
#include <algorithm>

// code
// ---------------------------------------------------------
MessageFilter::MessageFilter()
{
   dFilterCount = 0;
   bIsNegativeFilter = FALSE;
   pMyIDFormatPair.clear();
}

// ---------------------------------------------------------
MessageFilter::~MessageFilter()
{
   pMyIDFormatPair.clear();
}

// ---------------------------------------------------------
BOOL MessageFilter::Filter(BaseMessageData& clBaseMessageData)
{
   UINT uiMessageID = clBaseMessageData.getMessageID();
   MessageFormatEnum eMessageFormat = clBaseMessageData.getMessageFormat();
   MessageAntennaSourceEnum eMessageAntennaSource = clBaseMessageData.getMessageSource();
   std::map < UINT, MessageAntennaSourceEnum >::iterator ite = pMyIDSourcePair.end();

   for (auto itr = pMyIDSourcePair.find(uiMessageID); itr != pMyIDSourcePair.end(); itr++)
   {
      if (itr->first == uiMessageID && (itr->second == eMessageAntennaSource || itr->second == BOTH_ANTENNA))
      {
         ite = itr;
         break;
      }
   }
   if (std::find(pMyIDFormatPair.begin(), pMyIDFormatPair.end(), std::make_pair(uiMessageID, eMessageFormat)) != pMyIDFormatPair.end() 
      && ite != pMyIDSourcePair.end())
   {
      // If Pair is found in the configured vector and it is a negative filter.
      // return false so that the message can be blocked at decoder level.
      if (bIsNegativeFilter == TRUE)
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
      // If Pair is not found in the configured vector and it is a negative filter.
      // return true so that the message can be passed at decoder level.
      if (bIsNegativeFilter == TRUE)
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

// ---------------------------------------------------------
void MessageFilter::ConfigureFilter(FilterConfig& stFilterConfig)
{
   copy(stFilterConfig.pIDFormatPair.begin(), stFilterConfig.pIDFormatPair.end(), back_inserter(pMyIDFormatPair));
   copy(stFilterConfig.pIDSourcePair.begin(), stFilterConfig.pIDSourcePair.end(), inserter(pMyIDSourcePair, pMyIDSourcePair.begin()));
   bIsNegativeFilter = stFilterConfig.bIsNegFilter;
}

// ---------------------------------------------------------
DOUBLE MessageFilter::GetFilterCount()
{
   return dFilterCount;
}

// ---------------------------------------------------------
void MessageFilter::SetFilterCount()
{
   dFilterCount = dFilterCount + 1;
}

// ---------------------------------------------------------
void MessageFilter::Reset()
{
   dFilterCount = 0;
}
