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
//! \file crc32unittest.cpp
//! \brief Unit test cases for circular buffer implemenatation.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <gtest/gtest.h>

#include "decoders/common/api/common.hpp"
#include "decoders/common/api/crc32.hpp"
#include "decoders/novatel/api/common.hpp"

class CRC32Test : public testing::Test
{
  public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

// -------------------------------------------------------------------------------------------------------
// CRC32 Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(CRC32Test, CalculateBlockCRC32)
{
    std::string sMessage("#BESTPOSA,SPECIAL,0,72.5,FINESTEERING,2000,202512.000,02000020,b1f6,32768;SOL_COMPUTED,"
                         "SINGLE,17.44306884140,78.37411522222,649.8119,-76.8000,WGS84,0.9206,1.0236,1.9887,\"\",0."
                         "000,0.000,34,34,34,34,00,06,39,33*42d4f5cc\r\n");

    uint32_t uiCalculatedCRC = 0;
    uint64_t uiTerminatorIndex = sMessage.length() - (novatel::edie::oem::OEM4_ASCII_CRC_LENGTH + 3);

    if (uiTerminatorIndex == 0) return;

    for (uint64_t i = 1ULL; i < uiTerminatorIndex; i++)
    {
        if (sMessage[i] == '\0') break;
        uiCalculatedCRC = CalculateCharacterCRC32(uiCalculatedCRC, sMessage[i]);
    }

    ASSERT_EQ(uiCalculatedCRC, 0x42d4f5ccUL);
}
