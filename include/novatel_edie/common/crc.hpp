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
// ! \file crc.hpp
// ===============================================================================

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

// Build the slice-by-8 lookup tables for the given generator polynomial.
// Rev=true expects a reflected polynomial and computes LSB-first tables.
// Rev=false expects a normal polynomial and computes MSB-first tables.
// Called once per unique (type, poly, rev) combination; result cached in UI_CRC_TABLE<CrcT, Poly, Rev> below.
template <typename CrcT, CrcT Poly, bool Rev> constexpr auto make_crc_table()
{
    static_assert(std::is_integral_v<CrcT> && std::is_unsigned_v<CrcT>, "CrcT must be an unsigned integral type");

    std::array<std::array<CrcT, 256>, 8> uiPreCalcCrcTables{};
    constexpr size_t firstBitShift = Rev ? 0 : (sizeof(CrcT) * 8 - 1);
    constexpr size_t firstByteShift = Rev ? 0 : (sizeof(CrcT) * 8 - 8);

    for (size_t i = 0; i < 256; i++)
    {
        auto uiCrc = static_cast<CrcT>(i << firstByteShift);

        for (uint8_t j = 0; j < 8; j++)
        {
            if constexpr (Rev)
            {
                uiCrc =
                    (uiCrc & static_cast<CrcT>(1)) != static_cast<CrcT>(0) ? static_cast<CrcT>((uiCrc >> 1) ^ Poly) : static_cast<CrcT>(uiCrc >> 1);
            }
            else
            {
                uiCrc = (uiCrc & (static_cast<CrcT>(1) << firstBitShift)) != static_cast<CrcT>(0) ? static_cast<CrcT>((uiCrc << 1) ^ Poly)
                                                                                                  : static_cast<CrcT>(uiCrc << 1);
            }
        }

        uiPreCalcCrcTables[0][i] = uiCrc;
    }

    for (uint8_t t = 1; t < 8; t++)
    {
        for (size_t i = 0; i < 256; i++)
        {
            const auto uiCrc = uiPreCalcCrcTables[t - 1][i];
            if constexpr (Rev)
            {
                // Following conditional avoids compiler warning about shifting a uint8_t >> 8
                if constexpr (sizeof(CrcT) == 1) { uiPreCalcCrcTables[t][i] = uiPreCalcCrcTables[0][static_cast<uint8_t>(uiCrc)]; }
                else
                {
                    uiPreCalcCrcTables[t][i] =
                        static_cast<CrcT>((uiCrc >> 8) ^ uiPreCalcCrcTables[0][static_cast<uint8_t>(uiCrc & static_cast<CrcT>(0xFF))]);
                }
            }
            else
            {
                uiPreCalcCrcTables[t][i] = static_cast<CrcT>(
                    (uiCrc << 8) ^ uiPreCalcCrcTables[0][static_cast<uint8_t>((uiCrc >> firstByteShift) & static_cast<CrcT>(0xFF))]);
            }
        }
    }

    return uiPreCalcCrcTables;
}

// Variable template: one table instance per CRC type/polynomial, computed entirely at compile time.
template <typename CrcT, CrcT Poly, bool Rev> inline constexpr auto UI_CRC_TABLE = make_crc_table<CrcT, Poly, Rev>();

// --------------------------------------------------------------------------
// Accumulate a single character into the given CRC value
// --------------------------------------------------------------------------
template <typename CrcT, CrcT Poly, bool Rev> constexpr void CalculateCharacterCrc(CrcT& uiCrc_, unsigned char ucChar_)
{
    constexpr size_t firstByteShift = sizeof(CrcT) * 8 - 8;

    if constexpr (Rev)
    {
        const CrcT uiIndex = static_cast<CrcT>((uiCrc_ ^ static_cast<CrcT>(ucChar_)) & static_cast<CrcT>(0xFF));

        // Following conditional avoids compiler warning about shifting a uint8_t >> 8
        if constexpr (sizeof(CrcT) == 1) { uiCrc_ = UI_CRC_TABLE<CrcT, Poly, Rev>[0][uiIndex]; }
        else { uiCrc_ = static_cast<CrcT>((uiCrc_ >> 8) ^ UI_CRC_TABLE<CrcT, Poly, Rev>[0][uiIndex]); }
    }
    else
    {
        const CrcT uiIndex = static_cast<CrcT>(((uiCrc_ >> firstByteShift) ^ static_cast<CrcT>(ucChar_)) & static_cast<CrcT>(0xFF));
        uiCrc_ = static_cast<CrcT>((uiCrc_ << 8) ^ UI_CRC_TABLE<CrcT, Poly, Rev>[0][uiIndex]);
    }
}

// --------------------------------------------------------------------------
// Calculates the CRC of a block of data all at once
// --------------------------------------------------------------------------
template <typename CrcT, CrcT Poly, bool Rev>
constexpr CrcT CalculateBlockCrc(const unsigned char* ucBuffer_, uint32_t uiCount_, CrcT uiInitialCrc_ = static_cast<CrcT>(0))
{
    static_assert(sizeof(CrcT) == 1 || sizeof(CrcT) == 2 || sizeof(CrcT) == 4 || sizeof(CrcT) == 8, "CrcT size must be 1, 2, 4, or 8 bytes");

    CrcT crc = uiInitialCrc_;
    size_t i = 0;

    // Slice-by-8 algorithm
    //
    // CRC is linear over GF(2): CRC(A ^ B) = CRC(A) ^ CRC(B).
    //
    // After XORing the running CRC into the input data stream (word ^= crc),
    // the next 8 bytes b0..b7 can be interpreted as the 64-bit value:
    //
    //   (b0 << 56) ^ (b1 << 48) ^ ... ^ (b6 << 8) ^ b7
    //
    // By linearity, the CRC of this value can be decomposed as:
    //
    //   CRC(b0 << 56) ^ CRC(b1 << 48) ^ ... ^ CRC(b7)
    //
    // Each term depends only on one byte and its bit position, so it can be
    // precomputed in a 256-entry lookup table per position:
    //   UI_CRC_TABLE[7] for b0, UI_CRC_TABLE[6] for b1, ..., UI_CRC_TABLE[0] for b7.
    // See e.g. https://static.aminer.org/pdf/PDF/000/432/446/a_systematic_approach_to_building_high_performance_software_based_crc.pdf
    const size_t chunks = uiCount_ / 8;
    for (size_t k = 0; k < chunks; k++)
    {
        uint8_t b0 = ucBuffer_[i + 0];
        uint8_t b1 = ucBuffer_[i + 1];
        uint8_t b2 = ucBuffer_[i + 2];
        uint8_t b3 = ucBuffer_[i + 3];
        uint8_t b4 = ucBuffer_[i + 4];
        uint8_t b5 = ucBuffer_[i + 5];
        uint8_t b6 = ucBuffer_[i + 6];
        uint8_t b7 = ucBuffer_[i + 7];

        if constexpr (Rev)
        {
            // LSB-first: inject running CRC bytes from right-to-left
            b0 ^= static_cast<uint8_t>(crc & static_cast<CrcT>(0xFF));
            if constexpr (sizeof(CrcT) > 1) { b1 ^= static_cast<uint8_t>((crc >> 8) & static_cast<CrcT>(0xFF)); }
            if constexpr (sizeof(CrcT) > 2)
            {
                b2 ^= static_cast<uint8_t>((crc >> 16) & static_cast<CrcT>(0xFF));
                b3 ^= static_cast<uint8_t>((crc >> 24) & static_cast<CrcT>(0xFF));
            }
            if constexpr (sizeof(CrcT) > 4)
            {
                b4 ^= static_cast<uint8_t>((crc >> 32) & static_cast<CrcT>(0xFF));
                b5 ^= static_cast<uint8_t>((crc >> 40) & static_cast<CrcT>(0xFF));
                b6 ^= static_cast<uint8_t>((crc >> 48) & static_cast<CrcT>(0xFF));
                b7 ^= static_cast<uint8_t>((crc >> 56) & static_cast<CrcT>(0xFF));
            }
        }
        else
        {
            // MSB-first: inject running CRC bytes from left-to-right
            if constexpr (sizeof(CrcT) == 1) { b0 ^= static_cast<uint8_t>(crc); }
            else if constexpr (sizeof(CrcT) == 2)
            {
                b0 ^= static_cast<uint8_t>((crc >> 8) & static_cast<CrcT>(0xFF));
                b1 ^= static_cast<uint8_t>(crc & static_cast<CrcT>(0xFF));
            }
            else if constexpr (sizeof(CrcT) == 4)
            {
                b0 ^= static_cast<uint8_t>((crc >> 24) & static_cast<CrcT>(0xFF));
                b1 ^= static_cast<uint8_t>((crc >> 16) & static_cast<CrcT>(0xFF));
                b2 ^= static_cast<uint8_t>((crc >> 8) & static_cast<CrcT>(0xFF));
                b3 ^= static_cast<uint8_t>(crc & static_cast<CrcT>(0xFF));
            }
            else
            {
                b0 ^= static_cast<uint8_t>((crc >> 56) & static_cast<CrcT>(0xFF));
                b1 ^= static_cast<uint8_t>((crc >> 48) & static_cast<CrcT>(0xFF));
                b2 ^= static_cast<uint8_t>((crc >> 40) & static_cast<CrcT>(0xFF));
                b3 ^= static_cast<uint8_t>((crc >> 32) & static_cast<CrcT>(0xFF));
                b4 ^= static_cast<uint8_t>((crc >> 24) & static_cast<CrcT>(0xFF));
                b5 ^= static_cast<uint8_t>((crc >> 16) & static_cast<CrcT>(0xFF));
                b6 ^= static_cast<uint8_t>((crc >> 8) & static_cast<CrcT>(0xFF));
                b7 ^= static_cast<uint8_t>(crc & static_cast<CrcT>(0xFF));
            }
        }

        crc = UI_CRC_TABLE<CrcT, Poly, Rev>[7][b0] ^ UI_CRC_TABLE<CrcT, Poly, Rev>[6][b1] ^ UI_CRC_TABLE<CrcT, Poly, Rev>[5][b2] ^
              UI_CRC_TABLE<CrcT, Poly, Rev>[4][b3] ^ UI_CRC_TABLE<CrcT, Poly, Rev>[3][b4] ^ UI_CRC_TABLE<CrcT, Poly, Rev>[2][b5] ^
              UI_CRC_TABLE<CrcT, Poly, Rev>[1][b6] ^ UI_CRC_TABLE<CrcT, Poly, Rev>[0][b7];

        i += 8;
    }

    // Process remaining bytes
    for (; i < uiCount_; i++) { CalculateCharacterCrc<CrcT, Poly, Rev>(crc, ucBuffer_[i]); }

    return crc;
}

// --------------------------------------------------------------------------
// Calculates the CRC for a string
// --------------------------------------------------------------------------
template <typename CrcT, CrcT Poly, bool Rev> constexpr CrcT CalculateBlockCrc(std::string_view buffer_, CrcT uiInitialCrc_ = static_cast<CrcT>(0))
{
    CrcT uiCrc = uiInitialCrc_;
    for (const char c : buffer_) { CalculateCharacterCrc<CrcT, Poly, Rev>(uiCrc, static_cast<unsigned char>(c)); }
    return uiCrc;
}

// --------------------------------------------------------------------------
// Backward-compatible CRC-32 API
// --------------------------------------------------------------------------
template <uint32_t Poly = 0xEDB88320UL, bool Rev = true> constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_)
{
    CalculateCharacterCrc<uint32_t, Poly, Rev>(uiCrc_, ucChar_);
}

template <uint32_t Poly = 0xEDB88320UL, bool Rev = true>
constexpr uint32_t CalculateBlockCrc32(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    return CalculateBlockCrc<uint32_t, Poly, Rev>(ucBuffer_, uiCount_, uiInitialCrc_);
}

template <uint32_t Poly = 0xEDB88320UL, bool Rev = true> constexpr uint32_t CalculateBlockCrc32(std::string_view buffer_)
{
    return CalculateBlockCrc<uint32_t, Poly, Rev>(buffer_);
}
