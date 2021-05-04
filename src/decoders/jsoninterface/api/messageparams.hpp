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

/*! \file messageparams.hpp
 *  \brief  Class to hold the Message Parameters information (ClassId,ElemenId,TypeId etc)
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef CMESSAGEPARAMS_H
#define CMESSAGEPARAMS_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/env.hpp"
#include <string>

/*! \class CMessageParams
 *  \brief Class with methods to Set/Get Message Parameters information (ClassId,ElemenId,TypeId etc)
 *  
 */
class CMessageParams
{
public:
   /*! A constructor
    * \brief Intializes Class ID, Element ID, Block value and child parameter number with 0
	* Intialize element name string with ""
	*/
   CMessageParams();

   /*! Default destructor */
   ~CMessageParams();

   /*! \fn void setClassId(UINT iClassId)
	* \brief store Class id value
	* \param [in] iClassId
	*/ 
   void setClassId(UINT iClassId);

   /*! \fn void setElementId(UINT iElementId)
	* \brief store Element id value
	* \param [in] iElementId
	*/ 
   void setElementId(UINT iElementId);

   /*! \fn void setBlockValue(UINT iBlockValue)
	* \brief store Block value
	* \param [in] iBlockValue
	*/ 
   void setBlockValue(UINT iBlockValue);

   /*! \fn void setChildParamValue(UINT iChildParamsValue)
	* \brief store Number of child parameters of a class
	* \param [in] iChildParamsValue
	*/ 
   void setChildParamValue(UINT iChildParamsValue);

   /*! \fn void setElementname(std::string szelementname)
	* \brief store Element name
	* \param [in] szelementname
	*/    
   void setElementname(std::string szelementname);

   /*! \fn UINT getClassID() const
	* \brief Gets Class ID value
	* \return Class ID value
	*/  
   UINT getClassID() const;

   /*! \fn UINT getElementId() const
	* \brief Gets Element ID value
	* \return Element ID value
	*/
   UINT getElementId() const;

   /*! \fn UINT getBlockValue() const
	* \brief Gets Block value
	* \return Block value
	*/
   UINT getBlockValue() const;

   /*! \fn USHORT getChildParamValue() const
	* \brief Gets Child number of parameters value
	* \return Number of parameters value
	*/
   USHORT getChildParamValue() const;

   /*! \fn std::string getElementname() const
	* \brief Get Element name 
	* \return Element name string
	*/
   std::string getElementname() const;

private:
   /*! Variable to store Class ID value */
   UINT m_iClassId;

   /*! Variable to store Element ID value */
   UINT m_iElementId;

   /*! Variable to store Block value */
   UINT m_iBlockValue;

   /*! Variable to store number of child parameters */
   USHORT m_iChildParamNo;

   /*! Variable to store Element name value */
   std::string m_elementname;
};

#endif // CMESSAGEPARAMS_H
