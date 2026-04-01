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
// ! \file crc32_unit_test.cpp
// ===============================================================================

#include <gtest/gtest.h>

#include "novatel_edie/common/crc32.hpp"

using namespace novatel::edie::crc;

// -------------------------------------------------------------------------------------------------------
// CRC32 Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST(CRC32Test, CalculateCharacterCRC32_BESTPOS)
{
    std::string sMessage("#BESTPOSA,SPECIAL,0,72.5,FINESTEERING,2000,202512.000,02000020,b1f6,32768;SOL_COMPUTED,"
                         "SINGLE,17.44306884140,78.37411522222,649.8119,-76.8000,WGS84,0.9206,1.0236,1.9887,\"\",0."
                         "000,0.000,34,34,34,34,00,06,39,33*42d4f5cc\r\n");

    const uint16_t OEM4_ASCII_CRC_LENGTH = 8;

    uint32_t uiCalculatedCRC = 0;
    uint64_t uiTerminatorIndex = sMessage.length() - (OEM4_ASCII_CRC_LENGTH + 3);

    if (uiTerminatorIndex == 0) { return; }

    for (uint64_t i = 1ULL; i < uiTerminatorIndex; i++)
    {
        if (sMessage[i] == '\0') { break; }
        CalculateCharacterCrc32(uiCalculatedCRC, sMessage[i]);
    }

    ASSERT_EQ(uiCalculatedCRC, 0x42d4f5ccUL);
}

TEST(CRC32Test, CalculateBlockCRC32_BESTPOS)
{
    std::string sMessage("#BESTPOSA,SPECIAL,0,72.5,FINESTEERING,2000,202512.000,02000020,b1f6,32768;SOL_COMPUTED,"
                         "SINGLE,17.44306884140,78.37411522222,649.8119,-76.8000,WGS84,0.9206,1.0236,1.9887,\"\",0."
                         "000,0.000,34,34,34,34,00,06,39,33*42d4f5cc\r\n");

    const uint16_t OEM4_ASCII_CRC_LENGTH = 8;

    uint64_t uiTerminatorIndex = sMessage.length() - (OEM4_ASCII_CRC_LENGTH + 3);

    if (uiTerminatorIndex == 0) { return; }

    auto uiCalculatedCRC = CalculateBlockCrc32(reinterpret_cast<unsigned char*>(sMessage.data() + 1), uiTerminatorIndex - 1);

    ASSERT_EQ(uiCalculatedCRC, 0x42d4f5ccUL);
}

// CRC32 parameters due to Koopman, "32-Bit Cyclic Redundancy Codes for Internet Applications"
// see: https://users.ece.cmu.edu/~koopman/networks/dsn02/dsn02_koopman.pdf
TEST(CRC32Test, CalculateBlockCRC32_Koopman)
{
    constexpr unsigned char log[] = {0x33, 0x22, 0x55, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x3D, 0x34, 0x5A, 0xA6};
    auto uiCalculatedCRC = CalculateBlockCrc32<0xC8DF352FUL>(log, sizeof(log), 0xFFFFFFFFUL);
    ASSERT_EQ(uiCalculatedCRC, 0x904CDDBFUL);
}
