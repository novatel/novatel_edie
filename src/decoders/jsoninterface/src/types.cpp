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
//
//  DESCRIPTION:
//    Class to hold the type information
//
////////////////////////////////////////////////////////////////////////////////
#include "types.hpp"
#include <string.h>

CTypes::CTypes()
   :m_uiTypeId(0),
   m_uiLength(0),
   m_uiBaseType(0),
   m_uiArrayLength(0),
   m_strConversion(new char[10]),
   m_strStorageType(""),
   m_strTypeName(""),
   m_strBaseTypeName("")
{
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(CLASS, "CLASS"));
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(SIMPLE, "SIMPLE"));
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(FIXEDARRAY, "FIXEDARRAY"));
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(VARARRAY, "VARARRAY"));
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(ENUM, "ENUM"));
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(CLASSARRAY, "CLASSARRAY"));
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(STRING, "STRING"));
   m_typeMap.insert(std::pair<ElementTypeEnum, std::string>(INDEXOF, "INDEXOF"));
   m_eType = TYPE_UNDEFINED;
   memset(m_strConversion, 0, 10);
}

CTypes::CTypes(const CTypes& copy)
   :m_uiTypeId(copy.m_uiTypeId),
   m_uiLength(copy.m_uiLength),
   m_uiBaseType(copy.m_uiBaseType),
   m_uiArrayLength(copy.m_uiArrayLength),
   m_strStorageType(copy.m_strStorageType),
   m_strTypeName(copy.m_strStorageType),
   m_strBaseTypeName(copy.m_strStorageType),
   m_eType(copy.m_eType),
   m_strConversion(new char[*copy.m_strConversion])
{

}
CTypes& CTypes::operator=(const CTypes& copy)
{
   if(this == &copy)
      return *this;

   m_uiTypeId = copy.m_uiTypeId;
   m_uiLength = copy.m_uiLength;
   m_uiBaseType = copy.m_uiBaseType;
   m_uiArrayLength = copy.m_uiArrayLength;
   m_strStorageType = copy.m_strStorageType;
   m_strTypeName = copy.m_strStorageType;
   m_strBaseTypeName = copy.m_strStorageType;
   m_eType = copy.m_eType;
   m_strConversion = new char[*copy.m_strConversion];
   return *this;
}
CTypes::~CTypes()
{
   if(m_strConversion != NULL)
   {
      delete []m_strConversion;
      m_strConversion = NULL;
   }
}

void CTypes::setTypeId(const UINT iTypeId)
{
   m_uiTypeId = iTypeId;
}

void CTypes::setBaseType(const UINT iTypeId, const UINT iBaseType)
{
   m_mapBaseType.insert(std::pair<UINT, UINT>(iTypeId, iBaseType));
   m_uiBaseType = iBaseType;
}

void CTypes::setLength(const UINT iLength)
{
   m_uiLength = iLength;
}

void CTypes::setConversionString(char *strConversion)
{
   memcpy(m_strConversion, strConversion, 10);
}

void CTypes::setStorageType(const std::string& strStorageType)
{
   m_strStorageType = strStorageType;
   if (m_typeMap.size() > 0)
   {
      for (std::map<ElementTypeEnum, std::string>::iterator m_Iterator = m_typeMap.begin(); m_Iterator != m_typeMap.end(); ++m_Iterator)
      {
         if (m_Iterator->second == strStorageType)
            m_eType = m_Iterator->first;
      }
   }
}

void CTypes::setTypeName(const std::string& strTypeName)
{
	m_strTypeName = strTypeName;
}

void CTypes::setBaseTypeName(const std::string& strBaseTypeName)
{
	m_strBaseTypeName = strBaseTypeName;
}

void CTypes::setArrayLength(const UINT iArrayLength)
{
   m_uiArrayLength = iArrayLength;
}

UINT CTypes::getTypeId() const
{
   return m_uiTypeId;
}

UINT CTypes::getLength() const
{
   return m_uiLength;
}

UINT CTypes::getBaseType() const
{
   return m_uiBaseType;
}

UINT CTypes::getBaseTypeId(UINT uiTypeId) const
{
   if (m_mapBaseType.count(uiTypeId) > 0)
   {
      return m_mapBaseType.find(uiTypeId)->second;
   }

   return 0;
}

UINT CTypes::getArrayLength() const
{
   return m_uiArrayLength;
}
char *CTypes::getConversionString() const
{
   return m_strConversion;
}

const std::string &CTypes::getStorageType() const
{
   return m_strStorageType;
}

const std::string &CTypes::getTypeName() const
{
	return m_strTypeName;
}

const std::string &CTypes::getBaseTypeName() const
{
	return m_strBaseTypeName;
}

ElementTypeEnum CTypes::getStorageTypeEnum() const
{
   return m_eType;
}
