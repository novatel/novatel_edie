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

/*! \file sourcefilter.hpp
 *  \brief 
 * 
 *  This filter will allow users to block messages from the primary or secondary antenna sources. 
 *  Dual antenna receiver cards will provide logs from both antenna feeds such as RANGE (RANGE and RANGE_1). 
 *  This filter provides the flexibility to select messages from the required antenna source.
 *  
 *  \sa MessageAntennaSourceEnum
 *  \sa MessageFormatEnum
 */ 

//----------------------------------------------------------------------
// Recursive Inclusion
//----------------------------------------------------------------------
#ifndef SOURCEFILTER_HPP
#define SOURCEFILTER_HPP

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "messagedatafilter.hpp"

/*! \class SourceFilter
 *  \brief Provides API's to Configure Filter that will allow users to block messages 
 *  from the primary or secondary antenna sources.
 * 
 *  Derived from MessageDataFilter.
*/
class SourceFilter : public MessageDataFilter
{
public:
   /*! A Constructor
    * \brief Intialize filter count to 0.
    *        Intialize vector with pair of message antenna source and format 
    */ 
   SourceFilter();

   /*! A virtual destructor
    * \brief Clears the vector with pair of message antenna source and format 
    */   
   virtual ~SourceFilter();

   /*! \fn BOOL Filter(BaseMessageData& pclBaseMessageData)
    *  \brief Gets message antenna source and format from BaseMessageData. 
    *  Increment the source filter by 1 if above pair found in vector.
    * 
    *  \param [in] pclBaseMessageData BaseMessageData object
    *  \return TRUE or FALSE
    */
   BOOL Filter(BaseMessageData& pclBaseMessgaeData);

   /*! \fn void ConfigureFilter(FilterConfig& stFilterConfig)
    *  \brief Copy the message antenna source and format key-value pair to vector.
    * 
    *  \param [in] stFilterConfig FilterConfig struct variable
    *  \sa FilterConfig
    */    
   void ConfigureFilter(FilterConfig& stFilterConfig);

   /*! \fn DOUBLE GetFilterCount()
    *  \return Number of antenna source filters
    */
   DOUBLE GetFilterCount();

   /*! \fn void Reset()
    *  \brief Set source filter count to 0.
    */ 
   void Reset();

   friend class SourceFilterTest;

private:
   DOUBLE dFilterCount; /**< Filter count(Number of antenna source filters) variable */

   /*! \fn void SetFilterCount()
    * \brief Increment filter count(Number of antenna source filters) by 1.
    */ 
   void SetFilterCount();
   /**< vector of antenna source enum and message format key value pair. */
   std::vector < std::pair <MessageAntennaSourceEnum, MessageFormatEnum > > pMySourceFormatPair;
};

#endif
