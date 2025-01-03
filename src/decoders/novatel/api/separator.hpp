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

/*! \file separator.hpp
 *  \brief To extact the ASCII field separators (, or ;.
 *  Ascii field separator in message ','
 *  Ascii header end ';'
 *  Ascii bosy and CRC separator '*'
 *  Ascii string terminator '\0'
 *  Quote '"'
 *  Carriage return '0x0d'
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef SEPARATOR_H
#define SEPARATOR_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"

//const char VT = 0x9;                   // Vertical Tab
//const char HT = 0xB;                   // Horizontal Tab

/*! \def CR_HEX
 * \brief A macro that returns the hex value of Carriage Return(CR)
 *
 */
#define CR_HEX  (0x0D)

/*! \def LF_HEX
 * \brief A macro that returns the hex value of Line Feed(LF)
 *
 */
#define LF_HEX  (0x0A)

/*! \class Separator
 *  \brief Return Ascii filed separators in a message.
 *
 */
class Separator
{
public:
	/*! A constructor
	 *
	 */
	Separator();
	/*! A destructor
	 * \brief Set Ascii separator set array
	 */
	~Separator();
	/*! \fn SeparatorEnum FindNext(const Format eFormat_, CHAR** psFieldStart_, CHAR** psFieldEnd_, const CHAR* pcBufferEnd_)
	 * \brief It will return ascii field separator
	 * \param [in] eFormat_ Message Format(Currently Supports ASCII)
	 * \param [in] psFieldStart_ Start of the field address
	 * \param [in] psFieldEnd_ of the field address
	 * \param [in] pcBufferEnd_ Buffer end point
	 * \return Ascii field separator Enumaration
	 * \sa SeparatorEnum
	 */
	SeparatorEnum FindNext(const Format eFormat_, CHAR** psFieldStart_, CHAR** psFieldEnd_, const CHAR* pcBufferEnd_);
private:
	/*! Private Copy Constructor
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class.
	 */
	Separator(const Separator& Source_);

	/*! Private assignment operator
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution,
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */
	const Separator& operator= (const Separator& Source_);

	/*! Array of Ascii field separator */
	CHAR acMyASCIISeparatorSet[6];
	/*! String of Ascii field separators */
	CHAR* sMyASCIISeparators;
	/*! string of Ascii filed set */
	std::string strSeparatorSet;
};
#endif //SEPARATOR_H
