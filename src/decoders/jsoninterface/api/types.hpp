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
 *  \brief  Class to hold the Message type information 
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef CTYPES_H
#define CTYPES_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <string>
#include <map>
#include "decoders/common/api/env.hpp"


/*! An enumaraion
* Enumaration values for element type in a message identification
*/ 
enum ElementTypeEnum
{
   TYPE_UNDEFINED,  /*!< Unidentified element*/
   SIMPLE,          /*!< Simple type element*/
   FIXEDARRAY,      /*!< Fixed length rray type element*/
   VARARRAY,        /*!< Variable length array type element*/
   ENUM,            /*!< Enumaration type*/
   CLASS,           /*!< Element is Class type*/
   CLASSARRAY,      /*!< Element is array of Class type*/
   STRING,          /*!< Element is string type*/
   INDEXOF          /*!< Element is indexed type*/
};


/*! \class CTypes
 *  \brief Class with methods to Set/Get Message type information 
 *  Like Element type id, length, base type, length of array etc... 
 *  Intialize a map with ElementTypeEnum and corresponding string representation as key-value
 *  
 */
class CTypes
{
public:
   /*! A constructor
    * \brief Constructor intializes Element type id, length, base type, length of array etc... 
    * Intialize a map with ElementTypeEnum and corresponding string representation as key-value
    *
	*/ 
   explicit CTypes();

   /*! Copy Constructor of CTypes class. */  
   CTypes(const CTypes& copy);

   /*! Assignment operator for CTypes class. */
   CTypes& operator=(const CTypes& copy);

   /*! Default destructor */
	~CTypes();

   /*! \fn void setTypeId(const UINT iTypeId)
    * \brief Store Type ID value
	* \param [in] iTypeId
	*/
   void setTypeId(const UINT iTypeId);

   /*! \fn void setLength(const UINT iLength)
    * \brief Store length
	* \param [in] iLength
	*/
   void setLength(const UINT iLength);

   /*! \fn void setBaseType(const UINT iTypeId, const UINT iBaseType)
    * \brief Store type id and base type as key-value pair
	* \param [in] iTypeId Type id value of an element
	* \param [in] iBaseType Base type value of element
	*/
   void setBaseType(const UINT iTypeId, const UINT iBaseType);

   /*! \fn void setArrayLength(const UINT iArrayLength)
    * \brief Store length of array type element
	* \param [in] iArrayLength
	*/
   void setArrayLength(const UINT iArrayLength);

   /*! \fn void setConversionString(char* strConversion)
    * \brief Store Conversion string
	* \param [in] strConversion
	*/
   void setConversionString(char* strConversion);

   /*! \fn void setStorageType(const std::string& strStorageType)
    * \brief Store storage type string
	* \param [in] strStorageType
	*/
   void setStorageType(const std::string& strStorageType);

   /*! \fn void setTypeName(const std::string& strTypeName)
    * \brief Store type name
	* \param [in] strTypeName
	*/
   void setTypeName(const std::string& strTypeName);

   /*! \fn void setBaseTypeName(const std::string& strBaseTypeName)
    * \brief Store base type name
	* \param [in] strBaseTypeName
	*/
   void setBaseTypeName(const std::string& strBaseTypeName);
   
   /*! \fn UINT getTypeId() const
    * \brief Method to get type id value
	* \return type id value
	*/
   UINT getTypeId() const;

   /*! \fn UINT getLength() const
    * \brief Method to get length of element
	* \return element length
	*/
   UINT getLength() const;

   /*! \fn UINT getBaseType() const
    * \brief Method to get base type of element
	* \return Base type of element
	*/   
   UINT getBaseType() const;

   /*! \fn UINT getBaseTypeId(UINT uiTypeId) const
    * \brief Method to get base type id of element from type id value
	* \return Base type id of element
	*/ 
   UINT getBaseTypeId(UINT uiTypeId) const;

   /*! \fn UINT getArrayLength() const
    * \brief Method to get length of array type element
	* \return Array type element length
	*/
   UINT getArrayLength() const;

   /*! \fn char* getConversionString() const
    * \brief Method to get Conversion string
	* \return Conversion string
	*/
   char* getConversionString() const;

   /*! \fn const std::string& getStorageType() const
    * \brief Method to get storage type 
	* \return storage type string
	*/
   const std::string& getStorageType() const;

   /*! \fn const std::string& getTypeName() const
    * \brief Method to get type name
	* \return type name string
	*/
   const std::string& getTypeName() const;

   /*! \fn const std::string& getBaseTypeName() const
    * \brief Method to get base type name
	* \return Base type name string
	*/
   const std::string& getBaseTypeName() const;

   /*! \fn ElementTypeEnum getStorageTypeEnum() const;
    * \brief Method to get storage type enum name
	* \return Storage type Enum
	* \sa ElementTypeEnum
	*/
   ElementTypeEnum getStorageTypeEnum() const;

private:
   /*! Type id value */
   UINT m_uiTypeId;
   
   /*! Length of element */
   UINT m_uiLength;
   
   /*! base type of element */
   UINT m_uiBaseType;
   
   /*! Array length of element */
   UINT m_uiArrayLength;
   
   /*! Conversion string value */
   char *m_strConversion;

   /*! Storage type value */
   std::string m_strStorageType;

   /*! Storage type name value */
   std::string m_strTypeName;
   
   /*! Base type of element */
   std::string m_strBaseTypeName;

   /*! Type enumaration value */
   ElementTypeEnum m_eType;

   /*! MAp with ElementTypeEnum and its string value as key-value pair */
   std::map<ElementTypeEnum, std::string> m_typeMap;
   
   /*! Map with Base type and ID as key value-pair */
   std::map<UINT, UINT> m_mapBaseType;
};

#endif // CTYPES_H
