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
#include "filters/filtercontainer.hpp"

// code
// ---------------------------------------------------------
FilterContainer::FilterContainer()
{
   dFilterCount = 0;
   pMyMessageDataFilterContainer.clear();
}

// ---------------------------------------------------------
FilterContainer::~FilterContainer()
{
   pMyMessageDataFilterContainer.clear();
}

// ---------------------------------------------------------
BOOL FilterContainer::Filter(BaseMessageData& clBaseMessageData)
{
   for(auto it = pMyMessageDataFilterContainer.begin(); it!=pMyMessageDataFilterContainer.end(); ++it) 
   {
      if((*it)->Filter(clBaseMessageData) == FALSE)
      {         
         return FALSE;
      }
   }
   SetFilterCount();
   return TRUE;
}

// ---------------------------------------------------------
void FilterContainer::ConfigureFilter(FilterConfig& stFilterConfig)
{
   copy(stFilterConfig.pMessageDataFilterVec.begin(), stFilterConfig.pMessageDataFilterVec.end(), back_inserter(pMyMessageDataFilterContainer));
}

// ---------------------------------------------------------
DOUBLE FilterContainer::GetFilterCount()
{
   return dFilterCount;
}

// ---------------------------------------------------------
void FilterContainer::SetFilterCount()
{
   dFilterCount = dFilterCount + 1;
}

// ---------------------------------------------------------
void FilterContainer::Reset()
{
   dFilterCount = 0;

   for(auto it = pMyMessageDataFilterContainer.begin(); it!=pMyMessageDataFilterContainer.end(); ++it)
   {
      (*it)->Reset();
   }
}
