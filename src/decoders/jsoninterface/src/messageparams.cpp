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
//    Class to hold the Message Parameters (ClassId,ElemenId,TypeId etc) information
//
////////////////////////////////////////////////////////////////////////////////
#include "messageparams.hpp"

CMessageParams::CMessageParams()
   :m_iClassId(0),
   m_iElementId(0),
   m_iBlockValue(0),
   m_iChildParamNo(0),
   m_elementname("")
{

}

CMessageParams::~CMessageParams()
{

}

void CMessageParams::setClassId(UINT iClassId)
{
   m_iClassId = iClassId;
}

void CMessageParams::setElementId(UINT iElementId)
{
   m_iElementId = iElementId;
}

void CMessageParams::setBlockValue(UINT iBlockValue)
{
   m_iBlockValue = iBlockValue;
}

void CMessageParams::setChildParamValue(UINT iBlockValue)
{
	m_iChildParamNo = (USHORT)iBlockValue;
}

void CMessageParams::setElementname(std::string szelementname)
{
	m_elementname = szelementname;
}
UINT CMessageParams::getClassID() const
{
   return m_iClassId;
}

UINT CMessageParams::getElementId() const
{
   return m_iElementId;
}

UINT CMessageParams::getBlockValue() const
{
   return m_iBlockValue;
}

USHORT CMessageParams::getChildParamValue() const
{
	return m_iChildParamNo;
}

std::string CMessageParams::getElementname() const
{
	return m_elementname;
}
