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
constexpr size_t CHORBA_WINDOW = 48; // 6 words * 8 bytes/word

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
    uint64_t acc[5] = {0};
    acc[0] = uiInitialCrc_;
    uint64_t f[4] = {0};
    uint64_t cur = 0;

    uint32_t i = 0;

    // A zero polynomial Z(x) satisfies Z(x) mod G(x) = 0, where G(x) is the generator.
    // The core idea of the Chorba algorithm is that adding any multiple of such a
    // zero polynomial does not change the CRC:
    // M(x) mod G(x) = (M(x) + k(x) * Z(x)) mod G(x).
    // For each 64-bit word m from the input, the algorithm conceptually adds
    // m * Z(x) (at the word's position) to the message, where
    //
    //        Z(x) = x^300 + x^211 + x^183 + x^145 + 1.
    //
    // (More precisely, we add m * x^k * Z(x), where k corresponds to the
    // bit position of m in the stream.)
    //
    // This is a good choice of Z(x) because it has small degree (we only need to look
    // ceil(300 / 64) = 5 words ahead) and few terms (so we can implement the multiplication
    // with few shifts/XORs).
    //
    // The algorithm maintains a 5-word window of pending modifications. "acc[0]..acc[4]"
    // are the terms to be XORed into the next five input words as the loop advances,
    // and "f[0]..f[3]" are the newly generated downstream terms from the current "cur".
    // This diagram may help visualize where each term lands:
    //
    // input:     m0       m1       m2       m3       m4       m5       m6       ...
    // vars:      cur               f[0]     f[1]     f[2]     f[3]
    //            acc[0]   acc[1]   acc[2]   acc[3]   acc[4]

    for (; i + CHORBA_WINDOW < uiCount_; i += 8)
    {
        std::memcpy(&cur, ucBuffer_ + i, sizeof(cur));
        cur ^= acc[0];

        f[0] = (cur << 17) /* represents x^145 (2*64 + 17 = 145) */ ^ (cur << 55) /* represents x^183 (2*64 + 55 = 183) */;
        f[1] = (cur >> 47) /* overflow from x^145 */ ^ (cur >> 9) /* overflow from x^183 */ ^ (cur << 19) /* represents x^211 (3*64 + 19 = 211) */;
        f[2] = (cur >> 45) /* overflow from x^211 */ ^ (cur << 44) /* represents x^300 (4*64 + 44 = 300) */;
        f[3] = (cur >> 20) /* overflow from x^300 */;

        acc[0] = acc[1];
        acc[1] = acc[2] ^ f[0];
        acc[2] = acc[3] ^ f[1];
        acc[3] = acc[4] ^ f[2];
        acc[4] = f[3];
    }

    uint64_t leftover[6] = {0};
    std::memcpy(leftover, ucBuffer_ + i, uiCount_ - i);
    for (size_t j = 0; j < 5; j++) { leftover[j] ^= acc[j]; }

    return novatel::edie::CalculateBlockCrc<uint32_t, 0xEDB88320UL, true>(reinterpret_cast<const unsigned char*>(leftover), uiCount_ - i, 0);
}
} // namespace

namespace novatel::edie::oem {
constexpr void CalculateCharacterCrc32(uint32_t& uiCrc_, unsigned char ucChar_)
{
    CalculateCharacterCrc<uint32_t, 0xEDB88320UL, true>(uiCrc_, ucChar_);
}

constexpr uint32_t CalculateBlockCrc32(const unsigned char* ucBuffer_, uint32_t uiCount_, uint32_t uiInitialCrc_ = 0)
{
    return uiCount_ > CHORBA_WINDOW ? CalculateBlockCrc32Chorba(ucBuffer_, uiCount_, uiInitialCrc_)
                                    : CalculateBlockCrc<uint32_t, 0xEDB88320UL, true>(ucBuffer_, uiCount_, uiInitialCrc_);
}

constexpr uint32_t CalculateBlockCrc32(std::string_view buffer_) { return CalculateBlockCrc<uint32_t, 0xEDB88320UL, true>(buffer_); }
} // namespace novatel::edie::oem
