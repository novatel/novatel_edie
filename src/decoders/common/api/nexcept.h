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

/*! \file nexcept.h
 *  \brief Used for rasing exceptions with valid error message.
 *  \author akhan
 *  \date   FEB 2021
 *
 *  This class will be used for rasing/catching exceptions in the library
 *  with valid error message and in which class exception was raised.
 */

#ifndef NEXCEPT_H
#define NEXCEPT_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/*! \brief Used for rasing exceptions with valid error message in the library.
 *
 *  Having buffer of 256 bytes to hold the error message.
 *  Will store exception message including in which class it was raised.
 *  Parent class for generating exceptions in entire library.
 */
class nExcept
{
public:
   /*! \var buffer
    *  \brief Character buffer of 256 bytes to hold exception message
    */
	char buffer[256];

   /*! \brief default exception class constructor with no arguments.
    *
    */
   ///////////////////////////////////////////////////////////////////////////////
   nExcept() {}

   /*! \brief Exception Class constructor
    *
    *  \param[in] nNew Another nExcept Class object.
    *
    *  \remarks Will copy exception data from nNew class object
    */
   nExcept (const nExcept &nNew)
   {
		strncpy(buffer, nNew.buffer, 256);
   }

   /*! \brief Exception class with variable number of arguments which included
    *
    *  Exception message in which class it was raised and line number ...etc
    */
   nExcept(const char* szFormat, ...)
   {
      va_list arg_ptr;

      va_start (arg_ptr, szFormat);
      vsprintf (buffer,szFormat, arg_ptr);
      perror (buffer);
      va_end (arg_ptr);
   }

   /*! \brief Copy Exception class constructor.
    *
    */
   nExcept &operator= (const nExcept &nNew)
   {
		strncpy(buffer, nNew.buffer, 256);

      return *this;
   }

   /*! \brief Method to print exception with vatiable arguments which included
    *
    *  Exception message in which class it was raised and line number ...etc
    */
   void Printf( const char* szFormat, ...)
   {
      va_list arg_ptr;

      va_start (arg_ptr, szFormat);

      vsprintf (buffer,szFormat, arg_ptr);
      perror (buffer);
      va_end (arg_ptr);
   }

   /*! \brief default exception class destructor.
    *
    */
   ~nExcept() {}
};

#endif

