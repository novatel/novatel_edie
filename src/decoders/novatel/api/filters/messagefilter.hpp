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

/*! \file messagefilter.hpp
 *  \brief Message filters work based on Message Id and Message format.
 *
 * Applications create a message filter by writing the message numbers and their format 
 * that need to be allowed or blocked into the filter configuration structure.
 * 
 * Message filters have two modes. One is normal mode where defined messages are blocked.
 * The Second one is reverse mode(negative filter) where all messages except the defined
 * messages are blocked.
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef MESSAGEFILTER_HPP
#define MESSAGEFILTER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "messagedatafilter.hpp"

/*! \class MessageFilter
 *  \brief Class which has API's tp configure filrer with message name and format.
 * 
 *  Derived from MessageDataFilter.
 */
class MessageFilter : public MessageDataFilter
{
public:
   /*! A Constructor
    *
    * \brief Intialize Filter count to 0.
    *        Set reverse mode(negative filter) filtering to FALSE 
    *        Clears vector of message id and format pair.
    */
   MessageFilter();

   /*! A virtual destructor
    *
    * \brief Clears vector of message id and format pair.
    */   
   virtual ~MessageFilter();

   /*! \fn BOOL Filter(BaseMessageData& pclBaseMessageData)
    *  \brief Set message id and format pair from BaseMessageData and increment filter count by 1.
    *  \param [in] pclBaseMessageData BaseMessageData object
    *  \return TRUE if Intialization of filtering with the BaseMessageData success else FALSE.
    */ 
   BOOL Filter(BaseMessageData& pclBaseMessageData);

   /*! \fn void ConfigureFilter(FilterConfig& stFilterConfig)
    *  \brief Sets vactor with the pair of message id and format & message id and source.
    *          Enable or Disable negative filtering.
    *  \param [in] stFilterConfig FilterConfig structure
    *  \sa FilterConfig
    */ 
   void ConfigureFilter(FilterConfig& stFilterConfig);

   /*! \fn DOUBLE GetFilterCount()
    *  
    *  \return Filter count(Number of message filters).
    */       
   DOUBLE GetFilterCount();

   /*! \fn void Reset()
    *  \brief Reset the filter count(Number of message filters) to 0. 
    *   And Clears the vector with message id/format as key-value pair.
    */
   void Reset();

   friend class MessageFilterTest;

private:
   DOUBLE dFilterCount;  /**< Filter Count variable(Number of message filters) */
   /*! \fn void SetFilterCount()
    * \brief Increment filter count(Number of message filters) variable by 1.
    */   
   void SetFilterCount();
   /**< Vector of message id and format as key-value pairs. */
   std::vector < std::pair <UINT, MessageFormatEnum > > pMyIDFormatPair;
   /**< multimap of message id and source as key-value pairs. */
   std::multimap < UINT, MessageAntennaSourceEnum > pMyIDSourcePair;
   /**< Boolean variable to Enable/Disable negative(reverse) filtering. */
   BOOL bIsNegativeFilter;
};

#endif
