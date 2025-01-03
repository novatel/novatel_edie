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
//    Class to Hold the enum name and enum value
//
////////////////////////////////////////////////////////////////////////////////
#include "enums.hpp"

CEnums::CEnums()
{

}

CEnums::~CEnums()
{

}

void CEnums::setEnumNameAndValue(UINT iEnumValue, std::string strEnumName)
{
	if (enumName.size() > 0 && enumName.count(iEnumValue))
	{
		enumName.find(iEnumValue)->second = strEnumName;
	}
	else
	{
		enumName.insert(std::pair<UINT, std::string>(iEnumValue, strEnumName));
	}	
	if (enumValue.size() > 0 && enumValue.count(strEnumName))
	{
		enumValue.find(strEnumName)->second = iEnumValue;
	}
	else
	{
		enumValue.insert(std::pair<std::string, UINT>(strEnumName, iEnumValue));
	}
	
}

void CEnums::setEnumValues(UINT iTypeId, UINT iEnumValue)
{
	enumValuesByTypeId.insert(std::pair<UINT, UINT>(iTypeId, iEnumValue));
}

void CEnums::setEnumNames(UINT iTypeId, std::string strName)
{
	enumNamesByTypeId.insert(std::pair<UINT, std::string>(iTypeId, strName));
}

UINT CEnums::getEnumValueByName(std::string strEnumName) const
{
	if (enumValue.size() > 0 && enumValue.count(strEnumName))
	{
		return enumValue.find(strEnumName)->second;
	}
	return 0;
}

std::string CEnums::getEnumNameByValue(UINT iEnumValue) const
{
	if (enumName.size() > 0 && enumName.count(iEnumValue))
	{
		return enumName.find(iEnumValue)->second;
	}
	return "";	
}
