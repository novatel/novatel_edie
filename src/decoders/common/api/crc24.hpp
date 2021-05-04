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

#ifndef CRC24_H
#define CRC24_H

/*! \file crc24.hpp
 *  \brief Class to Calculate the CRC24 of a message
 *  \author Gopi R
 *  \date FEB 2021
 * 
 * Additional details can follow in subsequent paragraphs.
 */ 

//-----------------------------------------------------------------------
// Includes                                                               
//-----------------------------------------------------------------------
#include "env.hpp"

//-----------------------------------------------------------------------
// Globals and Locals                                                     
//-----------------------------------------------------------------------
//!* RTCA CRC size
const int RTCA_CRC_SIZE = 3; 
//! RTCMV3 CRC size
const int RTCMV3_CRC_SIZE = 3; 

/*! \brief Class to Calculate the CRC24 of a message
 *
 * This allows the user to ensure the data received (or transmitted) 
 * is valid with a high level of certainty.
 * 
 * The functions below may be implemented to generate the CRC of a block of data.
 */ 
class CRC24
{
public:

   /*! \brief default crc24 class constructor.*/
   CRC24();

   /*! \brief default crc24 class destructor.*/
   virtual ~CRC24();

   /*! \brief Calculates the CRC-24 of a block of data
    *
    *  \pre None
    *  \post None
    * 
    *  \param[in] ulCount Size of buffer (bytes) used for calculating CRC24.  
    *  \param[in] ucBuffer Start of Buffer which is used for calculating CRC24.  
    *  \param[in] ulCRC Seed(reference variable )which can store CRC for given buffer.
    * 
    *  \return CRC of given amount of buffer
    */ 
   ULONG Calculate(ULONG ulCount, ULONG *ucBuffer, ULONG ulCRC = 0);  

   /*! \brief Calculates and returns CRC-24 of a string for WAAS Logs
    *
    *  \pre None
    *  \post None
    * 
    *  \param[in] pucMsg String (bytes) used for calculating CRC24.  
    *  \param[in] uiNumBits Number of bits in string, used for calculating CRC24.  
    *  \param[in] ulCRC extract only last 24 bits of the calculated CRC.
    * 
    *  \return CRC-24 of a string 
    */ 
   ULONG CalculateWAAS(UCHAR* pucMsg, UINT uiNumBits, ULONG ulCRC);               
};

#endif // CRC24_H
