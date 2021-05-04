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
//    Class to extract the ASCII data 
//
////////////////////////////////////////////////////////////////////////////////
#include "separator.hpp"
#include <string.h>

Separator::Separator()
{
	acMyASCIISeparatorSet[0] = ASCII_FIELD_SEPERATOR;
	acMyASCIISeparatorSet[1] = ASCII_HEADER_TERMINATOR;
	acMyASCIISeparatorSet[2] = BODY_CRC_SEPARATOR;
	acMyASCIISeparatorSet[3] = QUOTE;
	acMyASCIISeparatorSet[4] = CR_HEX;
	acMyASCIISeparatorSet[5] = '\0';
	sMyASCIISeparators = &acMyASCIISeparatorSet[0];

	strSeparatorSet = sMyASCIISeparators;
}

Separator::~Separator()
{

}

SeparatorEnum Separator::FindNext(const Format eFormat_, CHAR** psFieldStart_, CHAR** psFieldEnd_, const CHAR* pcBufferEnd_)
{
	const CHAR* sSeparatorSet = "";
	if (eFormat_ == ASCII)
		sSeparatorSet = strSeparatorSet.c_str();

	// Skip any whitespace at the start of the field and 
	// return an error if all we have is whitespace
	while (isspace(**psFieldStart_))
		++*psFieldStart_;

	if (*psFieldStart_ > pcBufferEnd_)
		return (PAST_END_OF_BUFFER);

	// Find the next separator.  If NULL is returned, we went past the end of the buffer.
	// If the last character in buffer is the string terminator, use it as the next separator
	*psFieldEnd_ = strpbrk(*psFieldStart_, sSeparatorSet);
	if (*psFieldEnd_ == NULL)
	{
		if (*pcBufferEnd_ == '\0')
			*psFieldEnd_ = (CHAR*)pcBufferEnd_;
		else
			return (PAST_END_OF_BUFFER);
	}

	// Did we find a quote character?
	// If so, we are at the start of a string. 
	// Point to the start of the string, then find the next quote, 
	// then find the next separator
	if (**psFieldEnd_ == QUOTE)
	{
		*psFieldEnd_ = strpbrk(*psFieldStart_ + 1, &QUOTE);
		if (*psFieldEnd_ == NULL)
			return (PAST_END_OF_BUFFER);

		// Find the next separator and return an error if one is not found.
		*psFieldEnd_ = strpbrk((*psFieldEnd_) + 1, sSeparatorSet);
		if (*psFieldEnd_ == NULL)
			return (PAST_END_OF_BUFFER);
	}

	// Remove any extra whitespace at the end of the field
	CHAR* sCopyofFieldEnd = *psFieldEnd_ - 1;
	while (isspace(*sCopyofFieldEnd))
		*sCopyofFieldEnd-- = '\0';

	// Which Seperator did we find?    
	switch (**psFieldEnd_)
	{
	case ASCII_HEADER_TERMINATOR:
	{
		// change header terminator to string terminator
		**psFieldEnd_ = '\0';
		++*psFieldEnd_;
		return (HEADER_TERMINATOR_FOUND);

	}

	default:
	{
		// change separator to string terminator
		**psFieldEnd_ = '\0';
		++*psFieldEnd_;
		return (SEPARATOR_FOUND);

	}
	}

	return INVALID_SEPARATOR;
}
