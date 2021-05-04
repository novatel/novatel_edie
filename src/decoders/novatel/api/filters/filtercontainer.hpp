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

/*! \file filtercontainer.hpp
 *  \brief 
 * 
 *  This filter is a collection of filters. A user can configure individual filters 
 *  separately and add those configured filters to filter container. Internally a message 
 *  will be considered filtered when all the filters will satisfy the configuration.
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef FILTERCONTAINER_HPP
#define FILTERCONTAINER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "messagedatafilter.hpp"

/*! \class FilterContainer
 *  \brief Class which has a collection of filters. User has to configure indiviual filters
 *  and add to the vector which will be used when filtering. 
 * 
 *  Derived from MessageDataFilter.
 */
class FilterContainer : public MessageDataFilter
{
public:
   /*! A Constructor
    *  \brief Clears the filter count and vector of MessageDataFilter objects
    *  \sa MessageDataFilter
    */
   FilterContainer();

   /*! A virtual destructor
    *  \brief Clears the vector of MessageDataFilter objects
    *  \sa MessageDataFilter
    */
   virtual ~FilterContainer();

   /*! \fn BOOL Filter(BaseMessageData& pclBaseMessageData)
    *  \brief Intialize the filter with information from BaseMessageData and increment count by 1.
    *  \return FALSE, If intialization fails else TRUE.
    */ 
   BOOL Filter(BaseMessageData& pclBaseMessageData);

   /*! \fn BOOL ConfigureFilter(FilterConfig& stFilterConfig)
    *  \brief Configure filter with the information from BaseMessageData and increment count by 1.
    *  \return FALSE, If filter configuration fails else TRUE.
    *  \sa FilterConfig
    *  \remark Sets vector of MessageDataFilter objects from given filter configuration
    */    
   void ConfigureFilter(FilterConfig& stFilterConfig);

   /*! \fn DOUBLE GetFilterCount()
    *  
    *  \return Filter count(Number of FilterContainer objects).
    */    
   DOUBLE GetFilterCount();

   /*! \fn void Reset()
    *  \brief Reset the filter count and vector of MessageDataFilter objects
    */   
   void Reset();

   friend class FilterContainerUnitTest;

private:
   DOUBLE dFilterCount;   /**< Filter count variable(Number of FilterContainer objects) */
   
   /*! \fn void SetFilterCount()
    *  \brief Increment Filter count(Number of FilterContainer objects) by 1. 
    */ 
   void SetFilterCount(); 
   
   /**< vector of MessageDataFilter objects. */
   std::vector < MessageDataFilter* > pMyMessageDataFilterContainer;

	/*! Private Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class. 
	 */ 
   FilterContainer(const FilterContainer& clTemp);

	/*! Private assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */ 
   const FilterContainer& operator= (const FilterContainer& clTemp);
};

#endif
