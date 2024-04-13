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
// ! \file crc32.hpp
// ===============================================================================

#ifndef CRC32_HPP
#define CRC32_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <array>
#include <cstdint>

constexpr auto uiCRCTable = [] {
    std::array<uint32_t, 256> uiPreCalcCRCTable{};

    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t crc = i;

        for (uint32_t j = 0; j < 8; ++j) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320L : crc >> 1;

        uiPreCalcCRCTable[i] = crc;
    }

    return uiPreCalcCRCTable;
}();

// --------------------------------------------------------------------------
// Calculates the CRC-32 of a block of data one character for each call
// --------------------------------------------------------------------------
constexpr void CalculateCharacterCRC32(uint32_t& uiCRC, unsigned char ucChar)
{
    const uint32_t uiIndex = (uiCRC ^ ucChar) & 0xff;
    uiCRC = ((uiCRC >> 8) & 0x00FFFFFFL) ^ (uiCRCTable[uiIndex]);
}

// --------------------------------------------------------------------------
// Calculates the CRC-32 of a block of data all at once
// --------------------------------------------------------------------------
constexpr uint32_t CalculateBlockCRC32(uint32_t uiCount, uint32_t uiCRC, const unsigned char* ucBuffer)
{
    while (uiCount-- != 0) { CalculateCharacterCRC32(uiCRC, *ucBuffer++); }
    return (uiCRC);
}

// --------------------------------------------------------------------------
// Calculates the CRC-32 for a string
// --------------------------------------------------------------------------
constexpr uint32_t CalculateBlockCRC32(const char* ucBuffer)
{
    uint32_t uiCRC = 0;
    while (*ucBuffer != '\0') { CalculateCharacterCRC32(uiCRC, static_cast<unsigned char>(*ucBuffer++)); }
    return (uiCRC);
}

#endif // CRC32_HPP
