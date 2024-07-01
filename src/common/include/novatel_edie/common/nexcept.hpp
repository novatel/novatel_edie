// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file nexcept.h
// ===============================================================================

#ifndef NOVATEL_EDIE_COMMON_NEXCEPT_H
#define NOVATEL_EDIE_COMMON_NEXCEPT_H

#include <cstdarg>
#include <cstdio>
#include <cstring>

//============================================================================
//! \class NExcept
//! \brief Used for raising exceptions with valid error message in the library.
//============================================================================
class NExcept
{
  public:
    char buffer[256]{'\0'}; //!< Character buffer of 256 bytes to hold exception message

    //----------------------------------------------------------------------------
    //! \brief A constructor for the nExcept class.
    //----------------------------------------------------------------------------
    NExcept() = default;

    //----------------------------------------------------------------------------
    //! \brief Exception Class constructor.
    //
    //! \param[in] clNew_ Another nExcept Class object.
    //----------------------------------------------------------------------------
    NExcept(const NExcept& clNew_) { strncpy(buffer, clNew_.buffer, 256); }

    //----------------------------------------------------------------------------
    //! \brief Exception class with variable number of arguments which included.
    //
    //! \param[in] szFormat_ String formatting.
    //! \param[in] ...
    //----------------------------------------------------------------------------
    NExcept(const char* szFormat_, ...)
    {
        va_list argPtr;
        va_start(argPtr, szFormat_);
        vsprintf(buffer, szFormat_, argPtr);
        perror(buffer);
        va_end(argPtr);
    }

    //----------------------------------------------------------------------------
    //! \brief Assignment operator for nExcept.
    //----------------------------------------------------------------------------
    NExcept& operator=(const NExcept& clNew_)
    {
        if (this != &clNew_) { strncpy(buffer, clNew_.buffer, 256); }
        return *this;
    }

    //----------------------------------------------------------------------------
    //! \brief Method to print exception with variable arguments which included.
    //
    //! \param[in] szFormat_ String formatting.
    //! \param[in] ...
    //----------------------------------------------------------------------------
    void Printf(const char* szFormat_, ...)
    {
        va_list argPtr;
        va_start(argPtr, szFormat_);
        vsprintf(buffer, szFormat_, argPtr);
        perror(buffer);
        va_end(argPtr);
    }

    //----------------------------------------------------------------------------
    //! \brief Destructor for the nExcept class.
    //----------------------------------------------------------------------------
    ~NExcept() = default;
};

#endif // NOVATEL_EDIE_COMMON_NEXCEPT_H
