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
//  This file consists of methods to convert string to various data type values
//
////////////////////////////////////////////////////////////////////////////////

#include "stringtotypes.hpp"
#include <string.h>
#include "nexcept.h"


//------------------------------------------------------------
void StringToUChar(const CHAR* source, UCHAR* destination)
{

	UINT uiValueRead = 0;
	CHAR cDummy ='\0';
	if(sscanf(source,"%u%c",&uiValueRead,&cDummy) != 1)
		throw nExcept("Error in converting string to unsigned char");
	*destination = (UCHAR)uiValueRead;

}

//------------------------------------------------------------
void StringToChar(const CHAR* source, CHAR* destination)
{

	INT iValueRead = 0;
	CHAR cDummy ='\0';
	if(sscanf(source,"%d%c",&iValueRead,&cDummy) != 1)
		throw nExcept("Error in converting string to char");
	*destination = (CHAR)iValueRead;

}

//------------------------------------------------------------
void StringToHexChar(const CHAR* source, UCHAR* destination)
{

	UINT uiValueRead = 0;
	CHAR cDummy ='\0';
	if(sscanf(source,"%02x%c",&uiValueRead,&cDummy) != 1)
		throw nExcept("Can't convert string to hex char");
	*destination = (UCHAR)uiValueRead;

}

//------------------------------------------------------------
void StringToDouble(const CHAR* source, DOUBLE* destination)
{
	try
	{
		*destination = std::stod(source,nullptr);
	}
	catch(...)
	{
		throw nExcept("Error in Converting String to Double");
	}

}

//------------------------------------------------------------
void StringToFloat(const CHAR* source, FLOAT* destination)
{
	try
	{
		*destination = std::stof(source,nullptr);
	}
	catch(...)
	{
		throw nExcept("Error in Converting String to Float");
	}

}

//------------------------------------------------------------
void StringToULong(const CHAR* source, ULONG* destination)
{
	try
	{
		*destination = std::stoul(source, nullptr);
	}
	catch (...)
	{
		throw nExcept("Error in Converting String to Unsigned Long");
	}
}

//-----------------------------------------------------------------
void StringToHexULong(const CHAR* source, ULONG* destination)
{
	ULONG ulValueRead = 0;

#ifndef _LINUX_
	if(sscanf(source,"%lx",&ulValueRead) != 1)
		throw nExcept("Error in Converting hex string to unsigned long");
#else
	if(sscanf(source,"%x",&ulValueRead) != 1)
		throw nExcept("Error in Converting hex string to unsigned long");
#endif

	*destination = ulValueRead;
}

//------------------------------------------------------------
void StringToULongLong(const CHAR* source, ULONGLONG* destination)
{

	try
	{
		*destination = std::stoull(source,nullptr);
	}
	catch(...)
	{
		throw nExcept("Error in Converting String to Unsigned Long Long");
	}

}

//------------------------------------------------------------
void StringToLongLong(const CHAR* source, LONGLONG* destination)
{
	try
	{
		*destination = std::stoll(source,nullptr);
	}
	catch(...)
	{
		throw nExcept("Error in Converting String to Long Long");
	}

}

//------------------------------------------------------------
void StringToLong(const CHAR* source, LONG* destination)
{
	try
	{
		*destination = std::stol(source,nullptr);
	}
	catch(...)
	{
		throw nExcept("Error in Converting String to Long");
	}

}

//------------------------------------------------------------
void StringToInt(const CHAR* source, INT* destination)
{
	try
	{
		*destination = std::stoi(source,nullptr);
	}
	catch(...)
	{
		throw nExcept("Error in Converting String to Int");
	}

}

//------------------------------------------------------------
void StringToUInt(const CHAR* source, UINT* destination)
{

	UINT uiValueRead = 0;
	if(sscanf(source,"%u",&uiValueRead) != 1)
		throw nExcept("Error in converting string to unsigned int");
	*destination = uiValueRead;

}

//------------------------------------------------------------
void StringToShort(const CHAR* source, SHORT* destination)
{

	SHORT sValueRead = 0;
	if(sscanf(source,"%hi",&sValueRead) != 1)
		throw nExcept("Error in converting string to short");
	*destination = sValueRead;

}

//------------------------------------------------------------
void StringToUShort(const CHAR* source, USHORT* destination)
{

	USHORT usValueRead = 0;
	if(sscanf(source,"%hu",&usValueRead) != 1)
		throw nExcept("Error in converting string to unsigned short");
	*destination = usValueRead;

}

//------------------------------------------------------------
void StringToString(const CHAR* source, UCHAR* destination)
{
	std::string Temp(source, strlen(source));
	Temp.erase(std::remove(Temp.begin(), Temp.end(), '"'), Temp.end());
	memcpy(destination,Temp.c_str(),strlen(Temp.c_str()));
}

//------------------------------------------------------------
void StringToString(const CHAR* source, CHAR* destination)
{
	std::string Temp(source, strlen(source));
	Temp.erase(std::remove(Temp.begin(), Temp.end(), '"'), Temp.end());
	memcpy(destination,Temp.c_str(),strlen(Temp.c_str()));
}

//-------------------------------------------------------------
void StringToBool(const CHAR* source, BOOL* destination)
{
	std::string strAsciiString(source);

	if (strAsciiString.compare("TRUE") == 0)
	{
		*destination = 1;
	}
	else
	{
		*destination = 0;
	}

}

//--------------------------------------------------------------
void StringToSatelliteID(const CHAR* source, SATELLITEID* destination)
{
	std::string strAsciiString(source);
	if (strAsciiString.find('+') != std::string::npos)
	{
		sscanf(source, "%hu+%hi", &(destination->usPrnOrSlot), &(destination->sFrequencyChannel));
	}
	else if (strAsciiString.find('-') != std::string::npos)
	{
		sscanf(source, "%hu-%hi", &(destination->usPrnOrSlot), &(destination->sFrequencyChannel));
		destination->sFrequencyChannel *= -1;  // make the frequency channel negative
	}
	else
	{
		sscanf(source, "%hu", &(destination->usPrnOrSlot));
	}
}

//--------------------------------------------------------------------
void StringToXCharArray(const CHAR* source, UCHAR* destination)
{
	INT iTempValue = 0, iDestinationIndex = 0, iIndex = 0;
	const CHAR *pcSource = source;
	while (*pcSource)
	{
		if (*pcSource >= '0' && *pcSource <= '9')
		{
			iTempValue = (iTempValue * 16) + (*pcSource - '0');
		}
		else if ((*pcSource >= 'a' && *pcSource <= 'f'))
		{
			iTempValue = (iTempValue * 16) + ((*pcSource - 'a') + 10);
		}
		else if (*pcSource >= 'A' && *pcSource <= 'F')
		{
			iTempValue = (iTempValue * 16) + ((*pcSource - 'A') + 10);
		}
		if (++iIndex == 2)
		{
			destination[iDestinationIndex++] = (UCHAR)iTempValue;
			iIndex = iTempValue = 0;
		}
		pcSource++;
	}
}
