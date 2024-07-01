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

#ifndef NOVATEL_EDIE_COMMON_CRC32_HPP
#define NOVATEL_EDIE_COMMON_CRC32_HPP

#include <array>
#include <cstdint>

constexpr auto UI_CRC_TABLE = [] {
    std::array<uint32_t, 256> uiPreCalcCrcTable{};

    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t uiCrc = i;

        for (uint32_t j = 0; j < 8; ++j) { uiCrc = (uiCrc & 1) ? (uiCrc >> 1) ^ 0xEDB88320L : uiCrc >> 1; }

        uiPreCalcCrcTable[i] = uiCrc;
    }

    return uiPreCalcCrcTable;
}();

// --------------------------------------------------------------------------
// Calculates the CRC-32 of a block of data one character for each call
// --------------------------------------------------------------------------
constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_)
{
    const uint32_t uiIndex = (uiCrc_ ^ ucChar_) & 0xff;
    uiCrc_ = ((uiCrc_ >> 8) & 0x00FFFFFFL) ^ UI_CRC_TABLE[uiIndex];
}

// --------------------------------------------------------------------------
// Calculates the CRC-32 of a block of data all at once
// --------------------------------------------------------------------------
constexpr uint32_t CalculateBlockCrc32(uint32_t uiCount_, uint32_t uiCrc_, const unsigned char* ucBuffer_)
{
    while (uiCount_-- != 0) { CalculateCharacterCrc32(uiCrc_, *ucBuffer_++); }
    return (uiCrc_);
}

// --------------------------------------------------------------------------
// Calculates the CRC-32 for a string
// --------------------------------------------------------------------------
constexpr uint32_t CalculateBlockCrc32(const char* ucBuffer_)
{
    uint32_t uiCrc = 0;
    while (*ucBuffer_ != '\0') { CalculateCharacterCrc32(uiCrc, static_cast<unsigned char>(*ucBuffer_++)); }
    return uiCrc;
}

#endif // NOVATEL_EDIE_COMMON_CRC32_HPP
