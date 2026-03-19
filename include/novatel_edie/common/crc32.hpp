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

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace novatel::edie::crc {
template <uint32_t Poly> constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_);

namespace detail {
//============================================================================
//! \namespace novatel::edie::crc::detail
//! \brief Internal CRC implementation details.
//! \details Not part of the public API; may change without notice.
//============================================================================

// Build the slice-by-8 lookup tables for the given (reflected) generator polynomial.
// Called once per unique Poly; result cached in UI_CRC_TABLE<Poly> below.
template <uint32_t Poly> constexpr auto make_crc_table()
{
    std::array<std::array<uint32_t, 256>, 8> uiPreCalcCrcTables{};

    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t uiCrc = i;

        for (uint32_t j = 0; j < 8; ++j) { uiCrc = (uiCrc & 1) != 0U ? (uiCrc >> 1) ^ Poly : uiCrc >> 1; }

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
}

// Variable template: one table instance per polynomial, computed entirely at compile time.
template <uint32_t Poly> inline constexpr auto UI_CRC_TABLE = make_crc_table<Poly>();

//============================================================================
//! \brief Calculates the CRC-32 of a block of data using slice-by-8 algorithm.
//!
//! Assumes the buffer is properly aligned to sizeof(uint32_t).
//!
//! \param ucBuffer_ The buffer to calculate CRC for.
//! \param uiCount_ The number of bytes in the buffer.
//! \param uiInitialCrc_ The initial CRC value (default 0).
//! \return The calculated CRC-32 value.
//============================================================================
template <uint32_t Poly> inline uint32_t CalculateBlockCrc32Aligned(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    uint32_t crc = uiInitialCrc_;
    // Wrapper ensures this is aligned correctly
    const auto* words = reinterpret_cast<const uint32_t*>(ucBuffer_);
    const size_t chunks = uiCount_ / sizeof(uint64_t);
    size_t i = 0;

    for (; i < chunks * sizeof(uint64_t); i += 8)
    {
        uint32_t lo = words[i / sizeof(uint32_t)];
        uint32_t hi = words[(i / sizeof(uint32_t)) + 1];

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
        //   UI_CRC_TABLE<Poly>[7] for b0, UI_CRC_TABLE<Poly>[6] for b1, ..., UI_CRC_TABLE<Poly>[0] for b7.
        // See e.g. https://static.aminer.org/pdf/PDF/000/432/446/a_systematic_approach_to_building_high_performance_software_based_crc.pdf
        crc = UI_CRC_TABLE<Poly>[7][(lo) & 0xFF] ^ UI_CRC_TABLE<Poly>[6][(lo >> 8) & 0xFF] ^ UI_CRC_TABLE<Poly>[5][(lo >> 16) & 0xFF] ^
              UI_CRC_TABLE<Poly>[4][(lo >> 24)] ^ UI_CRC_TABLE<Poly>[3][(hi) & 0xFF] ^ UI_CRC_TABLE<Poly>[2][(hi >> 8) & 0xFF] ^
              UI_CRC_TABLE<Poly>[1][(hi >> 16) & 0xFF] ^ UI_CRC_TABLE<Poly>[0][(hi >> 24)];
    }

    // Process remaining bytes
    for (; i < uiCount_; i++) { CalculateCharacterCrc32<Poly>(crc, ucBuffer_[i]); }

    return crc;
}

//============================================================================
//! \brief Peel off misaligned head bytes to reach required alignment.
//!
//! \param ucBuffer_ The buffer pointer (updated to aligned position).
//! \param uiCount_ The byte count (updated to reflect remaining bytes).
//! \param crc The running CRC value (updated with head bytes).
//! \param alignmentBytes The required alignment (typically 4 or 8).
//============================================================================
template <uint32_t Poly> inline void PeelAlignmentHeadBytes(const unsigned char*& ucBuffer_, uint32_t& uiCount_, uint32_t& crc, size_t alignmentBytes)
{
    size_t alignment = reinterpret_cast<uintptr_t>(ucBuffer_) % alignmentBytes;

    if (alignment != 0)
    {
        size_t headBytes = (alignmentBytes - alignment) > uiCount_ ? uiCount_ : (alignmentBytes - alignment);

        for (size_t i = 0; i < headBytes; i++) { CalculateCharacterCrc32<Poly>(crc, ucBuffer_[i]); }

        ucBuffer_ += headBytes;
        uiCount_ -= static_cast<uint32_t>(headBytes);
    }
}
} // namespace detail

//============================================================================
//! \name Public API
//! Public CRC entry points. Components should use these wrappers (or
//! policy-specific wrappers) rather than calling detail symbols directly.
//! \{
//============================================================================

//============================================================================
//! \brief Calculates CRC-32 with automatic alignment handling for slice-by-8 algorithm.
//!
//! Ensures 4-byte alignment is maintained before delegating to CalculateBlockCrc32Aligned.
//!
//! \param ucBuffer_ The buffer to calculate CRC for.
//! \param uiCount_ The number of bytes in the buffer.
//! \param uiInitialCrc_ The initial CRC value (default 0).
//! \return The calculated CRC-32 value.
//============================================================================
template <uint32_t Poly = 0xEDB88320UL>
inline uint32_t CalculateBlockCrc32(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    const unsigned char* alignedBuffer = ucBuffer_;
    uint32_t alignedCount = uiCount_;
    novatel::edie::crc::detail::PeelAlignmentHeadBytes<Poly>(alignedBuffer, alignedCount, uiInitialCrc_, sizeof(uint32_t));
    return novatel::edie::crc::detail::CalculateBlockCrc32Aligned<Poly>(alignedBuffer, alignedCount, uiInitialCrc_);
}

//============================================================================
//! \brief Accumulate a single character into the given CRC-32 value.
//! \param uiCrc_ The running CRC value to update.
//! \param ucChar_ The character to accumulate.
//============================================================================
template <uint32_t Poly = 0xEDB88320UL> constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_)
{
    const uint32_t uiIndex = (uiCrc_ ^ ucChar_) & 0xff;
    uiCrc_ = ((uiCrc_ >> 8) & 0x00FFFFFFL) ^ novatel::edie::crc::detail::UI_CRC_TABLE<Poly>[0][uiIndex];
}

//============================================================================
//! \brief Calculates the CRC-32 for a string view using Sarwate byte-by-byte algorithm.
//!
//! \param buffer_ The string view to calculate CRC for.
//! \return The calculated CRC-32 value.
//============================================================================
template <uint32_t Poly = 0xEDB88320UL> constexpr uint32_t CalculateBlockCrc32(std::string_view buffer_)
{
    uint32_t uiCrc = 0;
    for (const char c : buffer_) { CalculateCharacterCrc32<Poly>(uiCrc, static_cast<unsigned char>(c)); }
    return uiCrc;
}
//! \}
} // namespace novatel::edie::crc
