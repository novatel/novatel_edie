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

// Anonymous namespace to hide internal functions and variables
namespace
{
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

        i += 8;
    }

    // Process remaining bytes
    for (; i < uiCount_; i++) { CalculateCharacterCrc32<Poly>(crc, ucBuffer_[i]); }

    return crc;
}
} // namespace

//============================================================================
//! \brief Accumulate a single character into the given CRC-32 value.
//! \param uiCrc_ The running CRC value to update.
//! \param ucChar_ The character to accumulate.
//============================================================================
template <uint32_t Poly> constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_)
{
    const uint32_t uiIndex = (uiCrc_ ^ ucChar_) & 0xff;
    uiCrc_ = ((uiCrc_ >> 8) & 0x00FFFFFFL) ^ UI_CRC_TABLE<Poly>[0][uiIndex];
}

//============================================================================
//! \brief An implementation of the Chorba CRC32 algorithm from https://arxiv.org/abs/2412.16398.
//!
//! This algorithm is more efficient than slice-by-8 for larger inputs (e.g. RANGE logs), while
//! remaining at least as fast for smaller inputs. Assumes the buffer is properly aligned to sizeof(uint64_t).
//!
//! \param ucBuffer_ The buffer to calculate CRC for.
//! \param uiCount_ The number of bytes in the buffer.
//! \param uiInitialCrc_ The initial CRC value (default 0).
//! \return The calculated CRC-32 value.
//============================================================================
inline uint32_t CalculateBlockCrc32FastAligned(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    // Wrapper ensures this is aligned correctly
    const uint64_t* words = reinterpret_cast<const uint64_t*>(ucBuffer_);
    uint64_t final[6] = {0};
    uint64_t next1 = uiInitialCrc_, next2 = 0, next3 = 0, next4 = 0, next5 = 0;
    uint64_t in1, a1, a2, a3, a4;

    uint32_t i = 0;

    // A zero polynomial Z(x) satisfies Z(x) mod G(x) = 0, where G(x) is the generator.
    // The core idea of the Chorba algorithm is that adding any multiple of such a
    // zero polynomial does not change the CRC:
    // M(x) mod G(x) = (M(x) + k(x) * Z(x)) mod G(x).
    // For each 64-bit word w from the input, the algorithm conceptually adds
    // w * Z(x) (at the word's position) to the message, where
    //
    //        Z(x) = x^300 + x^211 + x^183 + x^145 + 1.
    //
    // (More precisely, we add w * x^k * Z(x), where k corresponds to the
    // bit position of w in the stream.)
    //
    // This is a good choice of Z(x) because it has small degree (we only need to look
    // ceil(300 / 64) = 5 words ahead) and few terms (so we can implement the multiplication
    // with few shifts/XORs).
    //
    // The algorithm maintains a 5-word window of pending modifications. "next1..next5"
    // are the terms to be XORed into the next five input words as the loop advances,
    // and "a1..a4" are the newly generated downstream terms from the current "in1".
    // This mapping may help visualize where each term lands:
    //
    // words:     w0       w1       w2       w3       w4       w5       w6       ...
    // vars:      in1               a1       a2       a3       a4
    //            next1    next2    next3    next4    next5

    for (; (i + 40 + 8) < uiCount_; i += 8)
    {
        in1 = words[i / sizeof(uint64_t)] ^ next1;

        a1 = (in1 << 17) /* represents x^145 (2*64 + 17 = 145) */ ^ (in1 << 55) /* represents x^183 (2*64 + 55 = 183) */;
        a2 = (in1 >> 47) /* overflow from x^145 */ ^ (in1 >> 9) /* overflow from x^183 */ ^ (in1 << 19) /* represents x^211 (3*64 + 19 = 211) */;
        a3 = (in1 >> 45) /* overflow from x^211 */ ^ (in1 << 44) /* represents x^300 (4*64 + 44 = 300) */;
        a4 = (in1 >> 20) /* overflow from x^300 */;

        next1 = next2;
        next2 = next3 ^ a1;
        next3 = next4 ^ a2;
        next4 = next5 ^ a3;
        next5 = a4;
    }

    std::memcpy(final, words + (i / sizeof(uint64_t)), uiCount_ - i);
    final[0] ^= next1;
    final[1] ^= next2;
    final[2] ^= next3;
    final[3] ^= next4;
    final[4] ^= next5;

    return CalculateBlockCrc32Aligned(reinterpret_cast<const unsigned char*>(final), uiCount_ - i, 0);
}

//============================================================================
//! \brief Peel off misaligned head bytes to reach required alignment.
//!
//! \param ucBuffer_ The buffer pointer (updated to aligned position).
//! \param uiCount_ The byte count (updated to reflect remaining bytes).
//! \param crc The running CRC value (updated with head bytes).
//! \param alignmentBytes The required alignment (typically 4 or 8).
//============================================================================
inline void PeelAlignmentHeadBytes(const unsigned char*& ucBuffer_, uint32_t& uiCount_, uint32_t& crc, size_t alignmentBytes)
{
    size_t alignment = reinterpret_cast<uintptr_t>(ucBuffer_) % alignmentBytes;

    if (alignment != 0)
    {
        size_t headBytes = (alignmentBytes - alignment) > uiCount_ ? uiCount_ : (alignmentBytes - alignment);

        for (size_t i = 0; i < headBytes; i++) { CalculateCharacterCrc32(crc, ucBuffer_[i]); }

        ucBuffer_ += headBytes;
        uiCount_ -= static_cast<uint32_t>(headBytes);
    }
}

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
inline uint32_t CalculateBlockCrc32(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    const unsigned char* alignedBuffer = ucBuffer_;
    uint32_t alignedCount = uiCount_;
    PeelAlignmentHeadBytes(alignedBuffer, alignedCount, uiInitialCrc_, sizeof(uint32_t));
    return CalculateBlockCrc32Aligned(alignedBuffer, alignedCount, uiInitialCrc_);
}

//============================================================================
//! \brief Calculates CRC-32 with automatic alignment handling for Chorba algorithm.
//!
//! Ensures 8-byte alignment is maintained before delegating to CalculateBlockCrc32FastAligned.
//! Falls back to slice-by-8 for inputs no larger than 48 bytes.
//!
//! \param ucBuffer_ The buffer to calculate CRC for.
//! \param uiCount_ The number of bytes in the buffer.
//! \param uiInitialCrc_ The initial CRC value (default 0).
//! \return The calculated CRC-32 value.
//============================================================================
inline uint32_t CalculateBlockCrc32Fast(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    const unsigned char* alignedBuffer = ucBuffer_;
    uint32_t alignedCount = uiCount_;
    PeelAlignmentHeadBytes(alignedBuffer, alignedCount, uiInitialCrc_, sizeof(uint64_t));
    return (alignedCount > 48) ? CalculateBlockCrc32FastAligned(alignedBuffer, alignedCount, uiInitialCrc_)
                               : CalculateBlockCrc32Aligned(alignedBuffer, alignedCount, uiInitialCrc_);
}

//============================================================================
//! \brief Calculates the CRC-32 for a string view using Sarwate byte-by-byte algorithm.
//!
//! \param buffer_ The string view to calculate CRC for.
//! \return The calculated CRC-32 value.
//============================================================================
constexpr uint32_t CalculateBlockCrc32(std::string_view buffer_)
{
    uint32_t uiCrc = 0;
    for (const char c : buffer_) { CalculateCharacterCrc32<Poly>(uiCrc, static_cast<unsigned char>(c)); }
    return uiCrc;
}
