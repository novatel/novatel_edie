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
//    Class to Calculate the CRC24 of a message
//
////////////////////////////////////////////////////////////////////////////////

#include "crc24.hpp"

static ULONG ulCRCTable[256] =   /* reverse 24 bit CRC table */
{
   0x000000, 0x25E2CC, 0x4BC598, 0x6E2754, 0x978B30, 0xB269FC, 0xDC4EA8, 0xF9AC64,
   0x9172A3, 0xB4906F, 0xDAB73B, 0xFF55F7, 0x06F993, 0x231B5F, 0x4D3C0B, 0x68DEC7,
   0x9C8185, 0xB96349, 0xD7441D, 0xF2A6D1, 0x0B0AB5, 0x2EE879, 0x40CF2D, 0x652DE1,
   0x0DF326, 0x2811EA, 0x4636BE, 0x63D472, 0x9A7816, 0xBF9ADA, 0xD1BD8E, 0xF45F42,
   0x8767C9, 0xA28505, 0xCCA251, 0xE9409D, 0x10ECF9, 0x350E35, 0x5B2961, 0x7ECBAD,
   0x16156A, 0x33F7A6, 0x5DD0F2, 0x78323E, 0x819E5A, 0xA47C96, 0xCA5BC2, 0xEFB90E,
   0x1BE64C, 0x3E0480, 0x5023D4, 0x75C118, 0x8C6D7C, 0xA98FB0, 0xC7A8E4, 0xE24A28,
   0x8A94EF, 0xAF7623, 0xC15177, 0xE4B3BB, 0x1D1FDF, 0x38FD13, 0x56DA47, 0x73388B,
   0xB0AB51, 0x95499D, 0xFB6EC9, 0xDE8C05, 0x272061, 0x02C2AD, 0x6CE5F9, 0x490735,
   0x21D9F2, 0x043B3E, 0x6A1C6A, 0x4FFEA6, 0xB652C2, 0x93B00E, 0xFD975A, 0xD87596,
   0x2C2AD4, 0x09C818, 0x67EF4C, 0x420D80, 0xBBA1E4, 0x9E4328, 0xF0647C, 0xD586B0,
   0xBD5877, 0x98BABB, 0xF69DEF, 0xD37F23, 0x2AD347, 0x0F318B, 0x6116DF, 0x44F413,
   0x37CC98, 0x122E54, 0x7C0900, 0x59EBCC, 0xA047A8, 0x85A564, 0xEB8230, 0xCE60FC,
   0xA6BE3B, 0x835CF7, 0xED7BA3, 0xC8996F, 0x31350B, 0x14D7C7, 0x7AF093, 0x5F125F,
   0xAB4D1D, 0x8EAFD1, 0xE08885, 0xC56A49, 0x3CC62D, 0x1924E1, 0x7703B5, 0x52E179,
   0x3A3FBE, 0x1FDD72, 0x71FA26, 0x5418EA, 0xADB48E, 0x885642, 0xE67116, 0xC393DA,
   0xDF3261, 0xFAD0AD, 0x94F7F9, 0xB11535, 0x48B951, 0x6D5B9D, 0x037CC9, 0x269E05,
   0x4E40C2, 0x6BA20E, 0x05855A, 0x206796, 0xD9CBF2, 0xFC293E, 0x920E6A, 0xB7ECA6,
   0x43B3E4, 0x665128, 0x08767C, 0x2D94B0, 0xD438D4, 0xF1DA18, 0x9FFD4C, 0xBA1F80,
   0xD2C147, 0xF7238B, 0x9904DF, 0xBCE613, 0x454A77, 0x60A8BB, 0x0E8FEF, 0x2B6D23,
   0x5855A8, 0x7DB764, 0x139030, 0x3672FC, 0xCFDE98, 0xEA3C54, 0x841B00, 0xA1F9CC,
   0xC9270B, 0xECC5C7, 0x82E293, 0xA7005F, 0x5EAC3B, 0x7B4EF7, 0x1569A3, 0x308B6F,
   0xC4D42D, 0xE136E1, 0x8F11B5, 0xAAF379, 0x535F1D, 0x76BDD1, 0x189A85, 0x3D7849,
   0x55A68E, 0x704442, 0x1E6316, 0x3B81DA, 0xC22DBE, 0xE7CF72, 0x89E826, 0xAC0AEA,
   0x6F9930, 0x4A7BFC, 0x245CA8, 0x01BE64, 0xF81200, 0xDDF0CC, 0xB3D798, 0x963554,
   0xFEEB93, 0xDB095F, 0xB52E0B, 0x90CCC7, 0x6960A3, 0x4C826F, 0x22A53B, 0x0747F7,
   0xF318B5, 0xD6FA79, 0xB8DD2D, 0x9D3FE1, 0x649385, 0x417149, 0x2F561D, 0x0AB4D1,
   0x626A16, 0x4788DA, 0x29AF8E, 0x0C4D42, 0xF5E126, 0xD003EA, 0xBE24BE, 0x9BC672,
   0xE8FEF9, 0xCD1C35, 0xA33B61, 0x86D9AD, 0x7F75C9, 0x5A9705, 0x34B051, 0x11529D,
   0x798C5A, 0x5C6E96, 0x3249C2, 0x17AB0E, 0xEE076A, 0xCBE5A6, 0xA5C2F2, 0x80203E,
   0x747F7C, 0x519DB0, 0x3FBAE4, 0x1A5828, 0xE3F44C, 0xC61680, 0xA831D4, 0x8DD318,
   0xE50DDF, 0xC0EF13, 0xAEC847, 0x8B2A8B, 0x7286EF, 0x576423, 0x394377, 0x1CA1BB
};


// --------------------------------------------------------------------------
// Class constructor
// --------------------------------------------------------------------------
CRC24::CRC24()
{
}

// --------------------------------------------------------------------------
// Class destructor.
// --------------------------------------------------------------------------
CRC24::~CRC24()
{
}

// --------------------------------------------------------------------------
// Calculates the CRC-24 of a block of data
// -------------------------------------------------------------------------- 
ULONG CRC24::Calculate(ULONG ulCount, ULONG *ucBuffer, ULONG ulCRC)
{
   while (ulCount--)
   {                               
      UCHAR index = static_cast<UCHAR>(ulCRC ^ *ucBuffer++);        /* index into 256 byte CRC table */
      ulCRC = (ulCRC >> 8) ^ ulCRCTable[index];
   }

   // return the CRC with the top 8 bits set to 0
   return(ulCRC & 0x00ffffff);
}


ULONG  CRC24::CalculateWAAS(UCHAR* pucMsg,           /* string to check */
                            UINT  uiNumBits,          /* number of bits in string to check */
                            ULONG ulCRC24)           /* which version */
{
   UINT   i, j;
   ULONG  ulCRC;

   for (ulCRC = 0, i = 0; i <= uiNumBits / 8; i++)
   {
      ULONG  ulData = ((ULONG)pucMsg[i]) << 16;

      for (j = (i == uiNumBits / 8) ? uiNumBits % 8 : 8; j > 0; j--)
      {
         if ((ulData ^ ulCRC) & 0x00800000)
            ulCRC = (ulCRC << 1) ^ ulCRC24;
         else
            ulCRC <<= 1;

         ulData <<= 1;
      }
   }

   ulCRC &= 0x00FFFFFF; /* extract only last 24 bits */
   return ulCRC;
}

