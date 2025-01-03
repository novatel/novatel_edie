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

/*! \file messagedatafilter.hpp
 *  \brief 
 *  Details...
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef MESSAGEDATAFILTER_HPP
#define MESSAGEDATAFILTER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"
#include "decoders/common/api/basemessagedata.hpp"

/*! \class MessageDataFilter
 *   \brief 
 * 
 *  More detailed description... 
*/
class MessageDataFilter
{
public:
    /*! A default Constructor */
   MessageDataFilter();
    /*! A default virtual destructor */
   virtual ~MessageDataFilter();

   /** A pure virtual member.
    *
    * \sa Filter()
    * \param [in] pclBaseMessageData BaseMessageData object
    * \return Returns TRUE or FALSE based on filter request.
    *  The implementation is filter specific.
    */  
   virtual BOOL Filter(BaseMessageData& pclBaseMessageData) = 0;

   /** A pure virtual member.
    *
    * \sa ConfigureFilter()
    * \param [in] stFilerConfig FilterConfig structure
    *  \sa FilterConfig
    * \remark Configs the individual filter. The implementation is filter specific.
    */  
   virtual void ConfigureFilter(FilterConfig& stFilerConfig) = 0;

   /** A pure virtual member.
    *
    * \sa GetFilterCount()
    * \return Number of messages filtered.
    */ 
   virtual DOUBLE GetFilterCount() = 0;

   /** A pure virtual member.
    *
    * \sa CreateFilter()
    * \param [in] eFilterType Which type of filter to be created.
    * \return Returns the object instances of the specific filters.
    */  
   static MessageDataFilter* CreateFilter(FILTERTYPE eFilterType);

   /** A pure virtual member.
    *
    * \sa Reset()
    * \remark Clears the filter count. And clears the all members of individual filter.
    * The implementation is filter specific.
    */    
   virtual void Reset() = 0;
private:
};

#endif

