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

/*! \file timefilter.hpp
 *  \brief 
 *  
 *  Time filter will work based on the start and end time pairs configured in the filter configuration structure.
 *  Each time pair has GPS Time in milliseconds and GPS Week where GPS Week is optional parameter. This filter allows 
 *  messages which have a time stamp within the range of start and end time.  All remaining messages will be discarded.
 * 
 */

//----------------------------------------------------------------------
// Recursive Inclusion
//----------------------------------------------------------------------
#ifndef TIMEFILTER_HPP
#define TIMEFILTER_HPP

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "messagedatafilter.hpp"

/*! \class TimeFilter
 *  \brief Provides API's to configure filter with message start and end time pairs.
 * 
 *  Derived from MessageDataFilter.
 */
class TimeFilter : public MessageDataFilter
{
public:
   /*! A Constructor
    *  \brief Initialize filter count(Number of time filters) to 0.
	*         Disable Negative time filtering.
	*         Clears vector of start time pair and end time pair.
	*         Here pair means((GPS time in milliseconds and GPS week)
	*/ 
   TimeFilter();

   /*! A virtual destructor
    *  \brief Clears vector of start time pair and end time pair.
	*         Here pair means((GPS time in milliseconds and GPS week)
	*/ 
   virtual ~TimeFilter();

   /*! \fn BOOL Filter(BaseMessageData& pclBaseMessageData)
    *  \brief Gets the message Week and time in milli seconds and compare with configured pairs to filter. 
	*  \param [in] pclBaseMessageData BaseMessageData object
	*  \return TRUE or FALSE.
	*  \remark  By default, messages with unknown and sattime statuses will always be passed.
	*  If negative filter is enabled, the messages whitch are not in between start and end pair will be filtered.         
    */	
   BOOL Filter(BaseMessageData& pclBaseMessageData);

   /*! \fn void ConfigureFilter(FilterConfig& stFilterConfig)
    *  \brief Sets Start and End time pairs from given configuration.
	*         Enable/Disable negative time filtering.
	*  \param [in] stFilterConfig FilterConfig structure variable
	*/ 
   void ConfigureFilter(FilterConfig& stFilterConfig);

   /*! \fn DOUBLE GetFilterCount()
    *  \return Number of time filters.
    */
   DOUBLE GetFilterCount();

   /*! \fn void Reset()
    *  \brief Set time filter count to 0.
    */ 
   void Reset();

   friend class TimeFilterTest;
private:
   DOUBLE dFilterCount; /**< Filter count(Number of time filters) variable */

   /*! \fn void SetFilterCount()
    * \brief Increment filter count(Number of time filters) by 1.
    */    
   void SetFilterCount();

   /**< Vector of Start time pair with message GNSS Week and time in milliseconds as key-value pair. */
   std::vector < std::pair <ULONG, ULONG > > ulMyStartPair;
   /**< Vector of End time pair with message GNSS Week and time in milliseconds as key-value pair. */
   std::vector < std::pair <ULONG, ULONG > > ulMyEndPair;
   /**< Enable or Disable negative time filtering. */
   BOOL bMyIsNegativeTimeFilter;
};

#endif
