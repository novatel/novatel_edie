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

#include "common/api/crc32.hpp"
#include "string.h"
#include "common/api/common.hpp"
#include "common/api/env.hpp"

#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class CRC32Test : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:
};

TEST_F(CRC32Test, CalculateBlockCRC32)
{
   UINT uiLength = strlen("#BESTPOSA,SPECIAL,0,72.5,FINESTEERING,2000,202512.000,02000020,b1f6,32768;SOL_COMPUTED,SINGLE,17.44306884140,78.37411522222,649.8119,-76.8000,WGS84,0.9206,1.0236,1.9887,\"\",0.000,0.000,34,34,34,34,00,06,39,33*42d4f5cc\r\n");
   CHAR* pcMessage = new CHAR[uiLength + 1];
   strcpy(pcMessage, "#BESTPOSA,SPECIAL,0,72.5,FINESTEERING,2000,202512.000,02000020,b1f6,32768;SOL_COMPUTED,SINGLE,17.44306884140,78.37411522222,649.8119,-76.8000,WGS84,0.9206,1.0236,1.9887,\"\",0.000,0.000,34,34,34,34,00,06,39,33*42d4f5cc\r\n");

   CRC32 clCRC32;
   UINT ulCRC = 0;
   ulCRC = clCRC32.CalculateBlockCRC32(uiLength+2, ulCRC, (UCHAR*)pcMessage);

   UINT uiCalculatedCRC = 0;
   UINT iTerminatorIndex = uiLength - (OEM4_ASCII_CRC_LENGTH + 3);

   if(iTerminatorIndex <= 0)
      return;

   for(INT i = 1; i < iTerminatorIndex; i++)
   {
      if(pcMessage[i] == '\0')
         break;
      uiCalculatedCRC = clCRC32.CalculateCharacterCRC32(uiCalculatedCRC, pcMessage[i]);
   }

   ASSERT_EQ(uiCalculatedCRC, 0x42d4f5cc);
}
