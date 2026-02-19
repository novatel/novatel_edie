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

#include <array>
#include <cstdint>
#include <string_view>

constexpr auto UI_CRC_TABLE = [] {
    std::array<std::array<uint32_t, 256>, 8> uiPreCalcCrcTables{};

    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t uiCrc = i;

        for (uint32_t j = 0; j < 8; ++j) { uiCrc = (uiCrc & 1) != 0U ? (uiCrc >> 1) ^ 0xEDB88320L : uiCrc >> 1; }

        uiPreCalcCrcTables[0][i] = uiCrc;
    }

    for (uint8_t t = 1; t < 8; t++)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t uiCrc = uiPreCalcCrcTables[t - 1][i];
            uiPreCalcCrcTables[t][i] = (uiCrc >> 8) ^ uiPreCalcCrcTables[0][uiCrc & 0xFF];
        }
    }

    return uiPreCalcCrcTables;
}();

// --------------------------------------------------------------------------
// Accumulate a single character into the given CRC-32 value
// --------------------------------------------------------------------------
constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_)
{
    const uint32_t uiIndex = (uiCrc_ ^ ucChar_) & 0xff;
    uiCrc_ = ((uiCrc_ >> 8) & 0x00FFFFFFL) ^ UI_CRC_TABLE[0][uiIndex];
}

// --------------------------------------------------------------------------
// Calculates the CRC-32 of a block of data all at once
// --------------------------------------------------------------------------
constexpr uint32_t CalculateBlockCrc32(const unsigned char* ucBuffer_, uint32_t uiCount_)
{
    uint32_t crc = 0;

    size_t i = 0;

    const size_t chunks = uiCount_ / 8;
    for (size_t k = 0; k < chunks; k++)
    {
        // Read the next 8 bytes from the input buffer (Note: cannot in general assume
        // that ucBuffer_ is 4-byte aligned, so we cannot safely cast it to a uint32_t*)
        uint32_t lo = (static_cast<uint32_t>(ucBuffer_[i + 0])) | (static_cast<uint32_t>(ucBuffer_[i + 1]) << 8) |
                      (static_cast<uint32_t>(ucBuffer_[i + 2]) << 16) | (static_cast<uint32_t>(ucBuffer_[i + 3]) << 24);
        uint32_t hi = (static_cast<uint32_t>(ucBuffer_[i + 4])) | (static_cast<uint32_t>(ucBuffer_[i + 5]) << 8) |
                      (static_cast<uint32_t>(ucBuffer_[i + 6]) << 16) | (static_cast<uint32_t>(ucBuffer_[i + 7]) << 24);

        lo ^= crc;

        // Slice-by-8 algorithm
        //
        // CRC32 is linear over GF(2): CRC32(A ^ B) = CRC32(A) ^ CRC32(B).
        //
        // After XORing the running CRC into the first 4 bytes of input (lo ^= crc),
        // the next 8 bytes b0..b7 can be interpreted as the 64-bit value:
        //
        //   (b0 << 56) ^ (b1 << 48) ^ ... ^ (b6 << 8) ^ b7
        //
        // By linearity, the CRC of this value can be decomposed as:
        //
        //   CRC32(b0 << 56) ^ CRC32(b1 << 48) ^ ... ^ CRC32(b7)
        //
        // Each term depends only on one byte and its bit position, so it can be
        // precomputed in a 256-entry lookup table per position:
        //   UI_CRC_TABLE[7] for b0, UI_CRC_TABLE[6] for b1, ..., UI_CRC_TABLE[0] for b7.
        // See e.g. https://static.aminer.org/pdf/PDF/000/432/446/a_systematic_approach_to_building_high_performance_software_based_crc.pdf
        crc = UI_CRC_TABLE[7][(lo) & 0xFF] ^ UI_CRC_TABLE[6][(lo >> 8) & 0xFF] ^ UI_CRC_TABLE[5][(lo >> 16) & 0xFF] ^ UI_CRC_TABLE[4][(lo >> 24)] ^
              UI_CRC_TABLE[3][(hi) & 0xFF] ^ UI_CRC_TABLE[2][(hi >> 8) & 0xFF] ^ UI_CRC_TABLE[1][(hi >> 16) & 0xFF] ^ UI_CRC_TABLE[0][(hi >> 24)];

        i += 8;
    }

    // Process remaining bytes
    for (; i < uiCount_; i++) { CalculateCharacterCrc32(crc, ucBuffer_[i]); }

    return crc;
}

// --------------------------------------------------------------------------
// Calculates the CRC-32 for a string
// --------------------------------------------------------------------------
constexpr uint32_t CalculateBlockCrc32(std::string_view buffer_)
{
    uint32_t uiCrc = 0;
    for (const char c : buffer_) { CalculateCharacterCrc32(uiCrc, static_cast<unsigned char>(c)); }
    return uiCrc;
}

#endif // CRC32_HPP
