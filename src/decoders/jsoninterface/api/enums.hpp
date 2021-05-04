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

/*! \file enums.hpp
 *  \brief Class to Hold the enum name and enum value map.
 *
 */
//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef CENUMS_H
#define CENUMS_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <string>
#include <map>
#include <list>
#include "decoders/common/api/env.hpp"

/*! \class CEnums
 *  \brief Class with methods to SET/GET Enumaration values in a Messaage
 *
 */
class CEnums
{
public:
   /*! Default constructor */
   CEnums();
   /*! Default destructor */
   ~CEnums();
   /*! \fn void setEnumNameAndValue(UINT iEnumValue, std::string strEnumName)
    * \brief Insert Enum value and name into a map.
    *
    * \param [in] iEnumValue Enumaration value as integer
    * \param [in] strEnumName Enumaration name as string
    *
    * \details If Enum value key is already exist, then will update corresponding Enum name value in map
    *  If Enum name key is already exist, then will update corresponding Enum value in map
    *  If given Enum name or value is new, will insert in to map as new key value pair
    */
   void setEnumNameAndValue(UINT iEnumValue, std::string strEnumName);

   /*! void setEnumValues(UINT iTypeId, UINT iEnumValue)
    * \brief Set Enum values using type id as key in a map
    * \sa enumValuesByTypeId
    *
    * \param [in] iTypeId  Enum type value
    * \param [in] iEnumValue Enumaration value
    *
    * \details Insert type id and key and enum value as a value into a map
    */
   void setEnumValues(UINT iTypeId, UINT iEnumValue);

   /*! void setEnumNames(UINT iTypeId, std::string strName)
    * \brief Set Enum names using type id as key in a map
    * \sa enumNamesByTypeId
    *
    * \param [in] iTypeId  Enum type value
    * \param [in] strName  Enumaration string
    *
    * \details Insert type id and key and enum string name as a value into a map
    */
   void setEnumNames(UINT iTypeId, std::string strName);

   /*! \fn UINT getEnumValueByName(std::string strEnumName) const
    * \brief Gets Enumaration value with given Enumaration name from map
    *
    * \param [in] strEnumName Enum string variable
    *
    * \return Enumaration value
    */
   UINT getEnumValueByName(std::string strEnumName) const;

   /*! \fn std::string getEnumNameByValue(UINT iEnumValue) const
    * \brief Gets Enumaration name with given Enumaration value from map
    *
    * \param [in] iEnumValue Enum value variable
    *
    * \return Enumaration name as string
    */
   std::string getEnumNameByValue(UINT iEnumValue) const;

private:
   /*! \var std::map<UINT, std::string> enumName
    * \brief map of enum value-name as key-value pair
    */
   std::map<UINT, std::string> enumName;
   /*! \var std::map<std::string, UINT> enumValue
    * \brief map of enum name-value as key-value pair
    */
   std::map<std::string, UINT> enumValue;
   /*! \var std::map<UINT, std::string> enumNamesByTypeId
    * \brief map of enum typeid-name as key-value pair
    */
   std::map<UINT, std::string> enumNamesByTypeId;
   /*! \var std::map<UINT, UINT> enumValuesByTypeId
    * \brief map of enum typeid-value as key-value pair
    */
   std::map<UINT, UINT> enumValuesByTypeId;
};

#endif // CENUMS_H
