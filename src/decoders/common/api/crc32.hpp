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

#ifndef CRC32_H
#define CRC32_H

/*! \file crc24.hpp
 *  \brief Class to Calculate the CRC32 of a message
 *  \author Gopi R
 *  \date FEB 2021
 * 
 * The ASCII and Binary OEM7 family and SMART2 message formats all contain a 32-bit CRC
 * for data verification. This allows the user to ensure the data received (or transmitted) 
 * is valid with a high level of certainty.
 * 
 * The functions below may be implemented to generate the CRC of a block of data.
 */ 

//-----------------------------------------------------------------------
// Includes                                                               
//-----------------------------------------------------------------------
#include "env.hpp"

/*! \brief Class to Calculate the CRC32 of a message
 *
 *  
 */ 
class CRC32
{
public:

   /*! \brief default crc32 class constructor.
    *
    *  Class constructor which creates CRC 32 lookup table.
    */ 
   CRC32();
   
   /*!  \brief default crc32 class constructor. */
   ~CRC32();

   /*! \brief Calculates the CRC-32 of a block of data all at once
    *
    *  
    *  \pre None
    *  \post None
    * 
    *  \param[in] ulCount Number of bytes in buffer  
    *  \param[in] ulCRC Seed Variable to hold CRC-32 value
    *  \param[in] ucBuffer Buffer used for calculating CRC-32
    * 
    *  \return CRC-32 of a string 
    */
   UINT CalculateBlockCRC32(UINT ulCount, UINT ulCRC, UCHAR *ucBuffer);

   /*! \brief Calculates the CRC-32 of a block of data one character for each call 
    *
    *  \pre None
    *  \post None
    * 
    *  \param[in] ulCRC CRC-32 Seed value  
    *  \param[in] ucChar Character used for calculating CRC-32
    * 
    *  \return CRC-32 of a block of data
    */ 
   UINT CalculateCharacterCRC32(UINT ulCRC, UCHAR ucChar);
};

#endif // CRC32_H
