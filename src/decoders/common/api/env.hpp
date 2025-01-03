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

/*! \file env.hpp
 *  \brief header file contianing macro's, typedef's, bool types...etc... 
 *  \author Gopi R
 *  \date FEB 2021
 * 
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef ENV_HPP
#define ENV_HPP

/*! user defined macro for a TRUE is '1'*/        
#define TRUE 1

/*! user defined macro for a FALSE is '0'*/
#define FALSE       0

/*! user defined macro for a YES is '1'*/
#define YES         1

/*! user defined macro for a NO is '0'*/
#define NO          0

/*! user defined macro for a ON is '1'*/
#define ON          1

/*! user defined macro for a OFF is '0'*/
#define OFF         0

// NULLs        
#ifndef  NULL
/*! user defined macro for a NULL is '0' if not defined by user*/
#define  NULL  0    
#endif
#ifndef  NUL
/*! user defined macro for a NUL is '0' if not defined by user*/
#define  NUL  0 
#endif

/*! \typedef signed int INT
    \brief user defined data type for a signed int
*/

typedef  signed int     INT;
/*! \typedef unsigned int UINT
    \brief user defined data type for a unsigned int
*/

typedef  unsigned int   UINT;
/*! \typedef char CHAR
    \brief user defined data type for a character
*/

typedef  char           CHAR;
/*! \typedef unsinged char UCHAR
    \brief user defined data type for a unsigned character
*/

typedef  unsigned char  UCHAR;
/*! \typedef signed short SHORT
    \brief user defined data type for a signed short
*/

typedef  signed short   SHORT;
/*! \typedef unsinged short USHORT
    \brief user defined data type for a unsigned short
*/
typedef  unsigned short USHORT;

/*! Provide correct size for ULONG data type as per Novatel,
    for libgen classes across Linux and windows */
#ifdef _LINUX_
/*! \typedef singed int LONG
    \brief user defined data type for a signed int data type in Linux
*/
	typedef  signed int   LONG;
/*! \typedef usinged int ULONG
    \brief user defined data type for a unsigned int data type in Linux
*/
	typedef  unsigned int ULONG;
#else
/*! \typedef singed long LONG
    \brief user defined data type for a signed long data type in windows
*/
	typedef  signed long   LONG;

/*! \typedef unsinged long ULONG
    \brief user defined data type for a unsigned long data type in windows
*/
	typedef  unsigned long ULONG;
#endif

/*! \typedef unsinged long long ULONGLONG
    \brief user defined data type for a unsigned long long
*/
typedef  unsigned long long ULONGLONG;

/*! \typedef singed long long LONGLONG
    \brief user defined data type for a signed long long
*/
typedef  signed long long LONGLONG;

/*! \typedef float FLOAT
    \brief user defined data type for a float
*/
typedef  float          FLOAT;

/*! \typedef double DOUBLE
    \brief user defined data type for a double
*/
typedef  double         DOUBLE;

/*! \typedef long double LDOUBLE
    \brief user defined data type for a long double
*/
typedef  long double    LDOUBLE;

/*! \typedef int BOOL
    \brief user defined data type for a boolean int
*/
typedef  int            BOOL;

/*! \typedef unsigned char BOOLCHAR
    \brief user defined data type for a boolean char
*/
typedef  unsigned char  BOOLCHAR;

#endif
