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

#include "novatel_edie/common/crc.hpp"

namespace {
//============================================================================
//! \brief An implementation of the Chorba CRC32 algorithm from https://arxiv.org/abs/2412.16398.
//!     Uses the reflected generator polynomial 0xEDB88320 and treats the input as LSB-first.
//!
//! This algorithm is more efficient than slice-by-8 for larger inputs (e.g. RANGE logs), while
//! remaining at least as fast for smaller inputs.
//!
//! \param ucBuffer_ The buffer to calculate CRC for.
//! \param uiCount_ The number of bytes in the buffer.
//! \param uiInitialCrc_ The initial CRC value (default 0).
//! \return The calculated CRC-32 value.
//============================================================================
inline uint32_t CalculateBlockCrc32Chorba(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
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
        std::memcpy(&in1, ucBuffer_ + i, sizeof(in1));
        in1 ^= next1;

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

    std::memcpy(final, ucBuffer_ + i, uiCount_ - i);
    final[0] ^= next1;
    final[1] ^= next2;
    final[2] ^= next3;
    final[3] ^= next4;
    final[4] ^= next5;

    return novatel::edie::CalculateBlockCrc<uint32_t, 0xEDB88320UL, true>(reinterpret_cast<const unsigned char*>(final), uiCount_ - i, 0);
}
} // namespace

namespace novatel::edie::oem {
constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_)
{
    CalculateCharacterCrc<uint32_t, 0xEDB88320UL, true>(uiCrc_, ucChar_);
}

constexpr uint32_t CalculateBlockCrc32(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    return uiCount_ > 48 ? CalculateBlockCrc32Chorba(ucBuffer_, uiCount_, uiInitialCrc_)
                         : CalculateBlockCrc<uint32_t, 0xEDB88320UL, true>(ucBuffer_, uiCount_, uiInitialCrc_);
}

constexpr uint32_t CalculateBlockCrc32(std::string_view buffer_) { return CalculateBlockCrc<uint32_t, 0xEDB88320UL, true>(buffer_); }
} // namespace novatel::edie::oem
