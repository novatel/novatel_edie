////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
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
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file nexcept.h
//! \brief Used for rasing exceptions with valid error message.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef NEXCEPT_H
#define NEXCEPT_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//============================================================================
//! \class JsonReaderFailure
//! \brief Used for rasing exceptions with valid error message in the library.
//============================================================================
class nExcept
{
  public:
    char buffer[256]{'\0'}; //!< Character buffer of 256 bytes to hold exception message

    //----------------------------------------------------------------------------
    //! \brief A constructor for the nExcept class.
    //----------------------------------------------------------------------------
    nExcept() = default;

    //----------------------------------------------------------------------------
    //! \brief Exception Class constructor.
    //
    //! \param[in] clNew Another nExcept Class object.
    //----------------------------------------------------------------------------
    nExcept(const nExcept& clNew) { strncpy(buffer, clNew.buffer, 256); }

    //----------------------------------------------------------------------------
    //! \brief Exception class with variable number of arguments which included.
    //
    //! \param[in] szFormat String formatting.
    //----------------------------------------------------------------------------
    nExcept(const char* szFormat, ...)
    {
        va_list arg_ptr;

        va_start(arg_ptr, szFormat);
        vsprintf(buffer, szFormat, arg_ptr);
        perror(buffer);
        va_end(arg_ptr);
    }

    //----------------------------------------------------------------------------
    //! \brief Assignment operator for nExcept.
    //----------------------------------------------------------------------------
    nExcept& operator=(const nExcept& clNew)
    {
        strncpy(buffer, clNew.buffer, 256);

        return *this;
    }

    //----------------------------------------------------------------------------
    //! \brief Method to print exception with vatiable arguments which included.
    //
    //! \param[in] szFormat String formatting.
    //----------------------------------------------------------------------------
    void Printf(const char* szFormat, ...)
    {
        va_list arg_ptr;

        va_start(arg_ptr, szFormat);
        vsprintf(buffer, szFormat, arg_ptr);
        perror(buffer);
        va_end(arg_ptr);
    }

    //----------------------------------------------------------------------------
    //! \brief Destructor for the nExcept class.
    //----------------------------------------------------------------------------
    ~nExcept() = default;
};

#endif
