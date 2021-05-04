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

/*! \file stringtotypes.hpp
 *  \brief This file consists of methods to convert string to various data type values 
 *  \author akhan
 *  \date   FEB 2021 
 * 
 */ 

#ifndef STRINGTOTYPES_HPP 
#define STRINGTOTYPES_HPP 

//-----------------------------------------------------------------------
// Includes                                                               
//-----------------------------------------------------------------------
#include "env.hpp"
#include "common.hpp"
// Open Source Decoder Changes - Need to replace this with the methods from JSON interface
//#include "messagesinfo.hpp"
#include <sstream>

/*! \fn void StringToUChar(const CHAR* source,UCHAR* destination)
    \brief Convert string to Unsigned char.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of unsigned char value to be converted from string.
*/
void StringToUChar(const CHAR* source,UCHAR* destination);

/*! \fn void StringToChar(const CHAR* source, CHAR* destination);
    \brief Convert string to char vaue.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of char value to be conveted from string.
*/
void StringToChar(const CHAR* source, CHAR* destination);

/*! \fn void StringToHexChar(const CHAR* source, UCHAR* destination);
    \brief Convert string to Hex char value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of Hex char value to be converted from string.
*/
void StringToHexChar(const CHAR* source, UCHAR* destination);

/*! \fn void StringToDouble(const CHAR* source, DOUBLE* destination)
    \brief Convert string to double value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of double value to be converted from string.
*/
void StringToDouble(const CHAR* source, DOUBLE* destination);

/*! \fn void StringToFloat(const CHAR* source, FLOAT* destination)
    \brief Convert string to float value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of float value to be converted from string.
*/
void StringToFloat(const CHAR* source, FLOAT* destination);

/*! \fn void StringToULong(const CHAR* source, ULONG* destination)
    \brief Convert string to unsigned long value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of unsigned value to be converted from string.
*/
void StringToULong(const CHAR* source, ULONG* destination);

/*! \fn void StringToHexULong(const CHAR* source, ULONG* destination)
    \brief Convert string to unsigned long Hex value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of unsigned long hex value to be converted from string.
*/
void StringToHexULong(const CHAR* source, ULONG* destination);

/*! \fn void StringToULongLong(const CHAR* source, ULONGLONG* destination)
    \brief Convert string to unsigned long long value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of unsigned long long value to be converted from string.
*/
void StringToULongLong(const CHAR* source, ULONGLONG* destination);

/*! \fn void StringToLongLong(const CHAR* source, LONGLONG* destination)
    \brief Convert string to signed long long value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of signed long long value to be converted from string.
*/
void StringToLongLong(const CHAR* source, LONGLONG* destination);

/*! \fn void StringToLong(const CHAR* source, LONG* destination)
    \brief Convert string to long value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of long value to be converted from string.
*/
void StringToLong(const CHAR* source, LONG* destination);

/*! \fn void StringToInt(const CHAR* source, INT* destination)
    \brief Convert string to int value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of int value to be converted from string.
*/
void StringToInt(const CHAR* source, INT* destination);

/*! \fn void StringToUInt(const CHAR* source, UINT* destination)
    \brief Convert string to unsigned int value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of unsigned int value to be converted from string.
*/
void StringToUInt(const CHAR* source, UINT* destination);

/*! \fn void StringToShort(const CHAR* source, SHORT* destination)
    \brief Convert string to short value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of short value to be converted from string.
*/
void StringToShort(const CHAR* source, SHORT* destination);

/*! \fn void StringToUShort(const CHAR* source, USHORT* destination)
    \brief Convert string to unsigned short value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of unsigned short value to be converted from string.
*/
void StringToUShort(const CHAR* source, USHORT* destination);

/*! \fn void StringToString(const CHAR* source, UCHAR* destination)
    \brief Convert string to unsigned char string.
    \param [in] source char string reference as source to be convert.
    \param [in] destination unsigned string to be converted from char string.
*/
void StringToString(const CHAR* source, UCHAR* destination);

/*! \fn void StringToString(const CHAR* source, CHAR* destination)
    \brief Convert string to signed char string.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of signed string to be converted from char string.
*/
void StringToString(const CHAR* source, CHAR* destination);

/*! \fn void StringToBool(const CHAR* source, BOOL* destination);
    \brief Convert string to boolean value.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of boolean value to be converted from string.
*/
void StringToBool(const CHAR* source, BOOL* destination);

/*! \fn void StringToSatelliteID(const CHAR* source, SATELLITEID* destination);
    \brief Convert string to SATELLITE ID structure.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of SATELLITE structure to be converted from string.
*/
void StringToSatelliteID(const CHAR* source, SATELLITEID* destination);

/*! \fn void StringToXCharArray(const CHAR* source, UCHAR* destination);
    \brief Convert string to hex char byte array.
    \param [in] source char string reference as source to be convert.
    \param [in] destination reference of unsigned char hex byes array to be converted from string.
*/
void StringToXCharArray(const CHAR* source, UCHAR* destination);
#endif
