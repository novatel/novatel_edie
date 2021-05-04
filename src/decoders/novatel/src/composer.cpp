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
//    Class to Start the Conversion process
//
////////////////////////////////////////////////////////////////////////////////
#include "composer.hpp"
#include "decoders/jsoninterface/api/messageparams.hpp"
#include "decoders/jsoninterface/api/types.hpp"
#include "decoders/jsoninterface/api/enums.hpp"

Composer::Composer() :
	m_status(MsgConvertStatusEnum(-1))
{

}

MsgConvertStatusEnum Composer::ComposeData(BaseMessageData *pclBaseData, Format conversionType, CHAR* writeData)
{
   if (conversionType == ASCII)
   {
      m_status = bBinaryToAscii.Convert(pclBaseData, writeData);
   }
   else if (conversionType == BINARY)
   {
      m_status = pAsciiToBinary.Convert(pclBaseData, writeData);
   }
   return m_status;
 }

Composer::~Composer()
{

}
