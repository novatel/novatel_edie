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

/*! \file decimationfilter.hpp
 *  \brief Provide a way for an application to reduce the sampling rate of messages.
 * 
 *  Decimation filters provide a way for an application to reduce the sampling rate of messages.
 *  If the receiver is providing messages every 1 second and the application wants the message 
 *  only every 5 seconds, then application can configure this filter with a decimation of 5 seconds.
 *  
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef DECIMATIONFILTER_HPP
#define DECIMATIONFILTER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "messagedatafilter.hpp"

/*! \class DecimationFilter
 *  \brief Provides API's to Configure Filter with decimation value.
 * 
 *  Derived from MessageDataFilter.
*/
class DecimationFilter : public MessageDataFilter
{
public:
   /*! A Constructor
    *
    *  \brief Resets the filter count and decimation value and associated maps for message name and decimation values.
    */ 
   DecimationFilter();

   /*! A Destructor
    *
    *  \brief Resets maps for message name and decimation values.
    */
   virtual ~DecimationFilter();

   /*! \fn BOOL Filter(BaseMessageData& pclBaseMessageData)
    *  \brief Gets message id/name and time from BaseMessageData. If the current time is multiple of sample period(decimation value)
    *  which will set by user. And sets map with message name and time as key-value pair,
    *  And sets map with message id and time as key-value pair,  Increment filter count.
    *  If message is already updated in map, then return FALSE.
    * 
    *  \param [in] pclBaseMessageData BaseMessageData object
    *  \return TRUE or FALSE
    *  \remark By default, messages with unknown and sattime statuses will always be passed.
    */ 
   BOOL Filter(BaseMessageData& pclBaseMessageData);

   /*! \fn void ConfigureFilter(FilterConfig& stFilterConfig)
    *
    *  \sa FilterConfig
    */ 
   void ConfigureFilter(FilterConfig& stFilterConfig);

   /*! \fn DOUBLE GetFilterCount()
    *  
    *  \return Filter count(Number of decimation filters).
    */    
   DOUBLE GetFilterCount();

   /*! \fn void Reset()
    *  \brief Reset the filter count to 0. And Clears the map with message id/time as key-value pair.
    */
   void Reset();

   friend class DecimationFilterTest;

private:
   DOUBLE dFilterCount;      /**< Filter Count variable(Number of decimation filters) */

   /*! \fn void SetFilterCount()
    * \brief Increment filter count(Number of decimation filters) variable by 1.
    */ 
   void SetFilterCount();

   DOUBLE dMySamplePeriod; /**< Decimation value variable */
   std::map <UINT, DOUBLE> mMySamplePeriodMap; /**< map of message id and time as key-value pair */
   std::vector < std::string > szMyMessageName; /**< vector of message names */
};

#endif
