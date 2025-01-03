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

/*! \file sattimefilter.hpp
 *  \brief
 *  
 *  This filter will allow user to select the messages for which the time status 
 *  is set to TIME_SATTIME(MessageTimeStatusEnum).
 *  
 *  \sa MessageTimeStatusEnum
 */ 

//----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef SATTIMEFILTER_HPP
#define SATTIMEFILTER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "messagedatafilter.hpp"

/*! \class SatTimeFilter
 *  \brief Provides API's to Configure Filter with log GPS time status value.
 * 
 *  Derived from MessageDataFilter.
*/
class SatTimeFilter : public MessageDataFilter
{
public:
   /*! A Consructor
    *
    *  \brief Intiaize filter count to 0.
    *         Diable filter.
    */ 
   SatTimeFilter();

   /*! A virtual destructor
    * \brief Diable filter.
    */ 
   virtual ~SatTimeFilter();

   /*! \fn BOOL Filter(BaseMessageData& pclBaseMessageData)
    *  \brief Gets message time status from BaseMessageData. 
    *  It the time status equals to SATTIME, then count the filter.
    * 
    *  \param [in] pclBaseMessageData BaseMessageData object
    *  \return TRUE or FALSE
    */ 
   BOOL Filter(BaseMessageData& pclBaseMessageData);

   /*! \fn void ConfigureFilter(FilterConfig& stFilterConfig)
    *  \brief Sets the boolean variable from given filter configuration
    * 
    *  \param [in] stFilterConfig FilterConfig struct variable
    *  \sa FilterConfig 
    */ 
   void ConfigureFilter(FilterConfig& stFilterConfig);

   /*! \fn DOUBLE GetFilterCount()
    *  \return Number of SATTIME filters
    */
   DOUBLE GetFilterCount();

   /*! \fn void Reset()
    *  \brief Set filter count(Number of SATTIME filters) to 0.
    */   
   void Reset();

   friend class SatTimeFilterTest;

private:
   DOUBLE dFilterCount;   /**< Filter count(Number of SATTIME filters) variable */

   /*! \fn void SetFilterCount()
    * \brief Increment filter count(Number of SATTIME filters) by 1.
    */ 
   void SetFilterCount();

   /**< Boolean variable to Enable/Disable SATTIME filter. */
   BOOL bMySatTimeFilter;
};

#endif
