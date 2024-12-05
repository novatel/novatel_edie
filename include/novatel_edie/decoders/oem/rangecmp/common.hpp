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
// ! \file common.hpp
// ===============================================================================

#ifndef RANGECMP_COMMON_HPP
#define RANGECMP_COMMON_HPP

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

#include "novatel_edie/decoders/common/common.hpp"

namespace novatel::edie::oem {

//-----------------------------------------------------------------------
// Generic Constexpr Functions
//-----------------------------------------------------------------------
// NOTE: Replace this with std::popcount when C++20 is available
template <typename T> constexpr uint32_t PopCount(T value) noexcept
{
    uint32_t count = 0;
    while (value)
    {
        count += value & 1;
        value >>= 1;
    }
    return count;
}

// NOTE: Replace this with std::countr_zero when C++20 is available
template <typename T> constexpr uint32_t Lsb(T value)
{
    if (value == 0) { return sizeof(T) * 8; } // Indicate no bits are set
    uint32_t index = 0;
    while ((value & 1) == 0)
    {
        value >>= 1;
        ++index;
    }
    return index;
}

template <typename T> constexpr uint32_t PopLsb(T& value)
{
    assert(value);
    const uint32_t index = Lsb(value);
    value &= value - 1;
    return index;
}

template <typename T, uint64_t Mask> constexpr T GetBitfield(uint64_t value)
{
    static_assert(std::is_integral<T>::value || std::is_enum_v<T>, "GetBitfield only returns integral or enum types.");
    static_assert(PopCount(Mask) <= sizeof(T) * 8, "Mask is too large for the return type");
    static_assert(((Mask >> Lsb(Mask)) & ((Mask >> Lsb(Mask)) + 1)) == 0, "Mask must have contiguous bits.");

    return static_cast<T>((value & Mask) >> Lsb(Mask));
}

template <uint32_t Mask> constexpr uint32_t EncodeBitfield(uint32_t value)
{
    if (PopCount(value << Lsb(Mask)) != PopCount(value)) { throw std::runtime_error("Lost bits after shift."); }

    return value << Lsb(Mask) & Mask;
}

template <auto Mask, typename T> constexpr void HandleSignExtension(T& value)
{
    static_assert(std::is_integral_v<T>, "Value must be an integral type.");
    static_assert(std::is_integral_v<decltype(Mask)>, "Mask must be an integral constant.");
    static_assert(sizeof(T) * 8 >= Lsb(Mask), "Mask must fit within the value type.");

    constexpr T signBit = static_cast<T>(1) << (Lsb(Mask) - 1);
    constexpr T extensionMask = static_cast<T>(Mask);

    if (value & signBit) { value |= extensionMask; }
}

template <typename T, uint32_t BitfieldBits> T ExtractBitfield(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_)
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "ExtractBitfield only returns integral or floating point types.");
    static_assert(BitfieldBits <= sizeof(T) * 8, "Return type too small for the requested bitfield.");

    if (BitfieldBits > uiBytesLeft_ * 8 - uiBitOffset_) { throw std::runtime_error("Not enough bytes remaining in the buffer."); }

    constexpr uint64_t mask = BitfieldBits < 64 ? (1ULL << BitfieldBits) - 1 : ~0ULL;
    uint64_t result;
    std::memcpy(&result, *ppucData_, std::min(uiBytesLeft_, 8U));
    result >>= uiBitOffset_;
    // A large bitfield with an offset can occupy 9 bytes
    if constexpr (BitfieldBits > 64 - 8)
    {
        const int32_t extraBits = uiBitOffset_ + BitfieldBits - 64;
        if (extraBits > 0)
        {
            uint64_t nextBytes = (*reinterpret_cast<uint64_t*>(*ppucData_ + 8)) & ((1ULL << extraBits) - 1);
            result |= nextBytes << (64 - uiBitOffset_);
        }
    }
    const uint32_t uiBitTotal = uiBitOffset_ + BitfieldBits;
    const uint32_t uiBytesConsumed = uiBitTotal / 8;

    *ppucData_ += uiBytesConsumed;
    uiBytesLeft_ -= uiBytesConsumed;
    uiBitOffset_ = uiBitTotal % 8;

    return static_cast<T>(mask & result);
}

// Slower version of extract bitfield for when the bitfield size isnt known at compile time (included signals).
template <typename T> T ExtractBitfield(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_, const uint32_t uiBitsInBitfield_)
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "ExtractBitfield only returns integral or floating point types.");

    if (uiBitsInBitfield_ > sizeof(T) * 8) { throw std::runtime_error("Return type too small for the requested bitfield."); }
    if (uiBitsInBitfield_ > uiBytesLeft_ * 8 - uiBitOffset_) { throw std::runtime_error("Not enough bytes remaining in the buffer."); }

    uint64_t mask = uiBitsInBitfield_ < 64 ? (1ULL << uiBitsInBitfield_) - 1 : ~0ULL;
    uint64_t result;
    std::memcpy(&result, *ppucData_, std::min(uiBytesLeft_, 8U));
    result >>= uiBitOffset_;
    // A large bitfield with an offset can occupy 9 bytes
    const int32_t extraBits = uiBitOffset_ + uiBitsInBitfield_ - 64;
    if (extraBits > 0)
    {
        uint64_t nextBytes = (*reinterpret_cast<uint64_t*>(*ppucData_ + 8)) & ((1ULL << extraBits) - 1);
        result |= nextBytes << (64 - uiBitOffset_);
    }
    const uint32_t uiBitTotal = uiBitOffset_ + uiBitsInBitfield_;
    const uint32_t uiBytesConsumed = uiBitTotal / 8;

    *ppucData_ += uiBytesConsumed;
    uiBytesLeft_ -= uiBytesConsumed;
    uiBitOffset_ = uiBitTotal % 8;

    return static_cast<T>(mask & result);
}

//-----------------------------------------------------------------------
// Generic Constants
//-----------------------------------------------------------------------
constexpr uint32_t MAX_VALUE = 0x800000; //!< Also 8388608, defined in RANGECMP documentation for ADR.

//! NOTE: See documentation on slot/PRN offsets for specific satellite systems:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm
constexpr uint32_t SBAS_PRN_OFFSET_120 = 120; //!< PRN offset for SBAS (if the PRN is indicated as 1 to 39).
constexpr uint32_t SBAS_PRN_OFFSET_120_LOWER_LIMIT = 1;
constexpr uint32_t SBAS_PRN_OFFSET_120_UPPER_LIMIT = 39;
constexpr uint32_t SBAS_PRN_OFFSET_130 = 130; //!< PRN offset for SBAS (if the PRN is indicated as 54 to 62).
constexpr uint32_t SBAS_PRN_OFFSET_130_LOWER_LIMIT = 54;
constexpr uint32_t SBAS_PRN_OFFSET_130_UPPER_LIMIT = 62;
constexpr uint32_t QZSS_PRN_OFFSET = 193; //!< PRN offset for QZSS.

//! NOTE: See the documentation on GLONASS slot & frequency numbers for descriptions on how these
//! offsets work.
//! https://docs.novatel.com/OEM7/Content/Messages/GLONASS_Slot_and_Frequen.htm?Highlight=GLONASS%20Slot%20and%20Frequency%20Numbers
constexpr uint32_t GLONASS_SLOT_UNKNOWN_LOWER_LIMIT = 43; //!< Slot limit for unknown GLONASS satellites.
constexpr uint32_t GLONASS_SLOT_UNKNOWN_UPPER_LIMIT = 64; //!< Slot limit for unknown GLONASS satellites.
constexpr uint32_t GLONASS_SLOT_OFFSET = 38;              //!< Slot offset for GLONASS.
constexpr uint32_t GLONASS_FREQUENCY_NUMBER_OFFSET = 7;
constexpr uint32_t RANGE_RECORD_MAX = 325; //!< Maximum number of RANGES reported in a RANGE/RANGECMP message.

//-----------------------------------------------------------------------
// OEM4 Range Message IDs
//-----------------------------------------------------------------------
constexpr uint32_t RANGE_MSG_ID = 43;
constexpr uint32_t RANGECMP_MSG_ID = 140;
constexpr uint32_t RANGECMP2_MSG_ID = 1273;
constexpr uint32_t RANGECMP3_MSG_ID = 1734;
constexpr uint32_t RANGECMP4_MSG_ID = 2050;
constexpr uint32_t RANGECMP5_MSG_ID = 2537;

namespace rangecmp {

//-----------------------------------------------------------------------
//! RANGECMP data field masks and scale factors.
//! NOTE: RangeCmpData defines a number of fields that can be
//! masked out from larger data types.
//-----------------------------------------------------------------------
constexpr uint64_t DOPPLER_FREQUENCY_MASK = 0x000000000FFFFFFF;
constexpr uint32_t DOPPLER_FREQUENCY_SIGNEXT_MASK = 0xF0000000;
constexpr float DOPPLER_FREQUENCY_SCALE_FACTOR = 256.0F;
constexpr uint64_t PSR_MEASUREMENT_MASK = 0xFFFFFFFFF0000000;
constexpr double PSR_MEASUREMENT_SCALE_FACTOR = 128.0;
constexpr double ADR_SCALE_FACTOR = 256.0;
constexpr uint32_t PSR_STDDEV_MASK = 0x0F;
constexpr uint32_t ADR_STDDEV_MASK = 0xF0;
constexpr double ADR_STDDEV_SCALE_FACTOR = 512.0;
constexpr uint32_t ADR_STDDEV_SCALE_OFFSET = 1;
constexpr uint32_t LOCK_TIME_MASK = 0x001FFFFF;
constexpr double LOCK_TIME_SCALE_FACTOR = 32.0;
constexpr uint32_t CNO_MASK = 0x03E00000;
constexpr uint32_t CNO_SCALE_OFFSET = 20;
constexpr uint32_t GLONASS_FREQUENCY_MASK = 0xFC000000;

} // namespace rangecmp

namespace rangecmp2 {

//-----------------------------------------------------------------------
//! RANGECMP2 data field masks and scale factors.
//! NOTE: RANGECMP2 contains two kinds of blocks - Satellite (SAT) and
//! Signal (SIG) - each with oddly sized bitfields. The bitfields can be
//! masked out of larger fields (that are multiples of bytes) which
//! contain the relevant information. By utilizing bitmasks and logical
//! shift operators, these oddly sized bitfields can be contained within
//! the smallest possible datatype. As per the comment in this header's
//! title block, any sign extension will be done according to the
//! smallest possible data type that the field can be contained in.
//-----------------------------------------------------------------------
constexpr uint64_t SAT_GLONASS_FREQUENCY_ID_MASK = 0x000000000000000F;
constexpr uint64_t SAT_SATELLITE_SYSTEM_ID_MASK = 0x00000000000001F0;
constexpr uint64_t SAT_SATELLITE_PSR_BASE_MASK = 0x0000007FFFFFFC00;
constexpr uint32_t SAT_SATELLITE_PSR_BASE_SIGNEXT_MASK = 0xE0000000;
constexpr uint64_t SAT_SATELLITE_DOPPLER_BASE_MASK = 0x0FFFFF8000000000;
constexpr uint32_t SAT_SATELLITE_DOPPLER_BASE_SIGNEXT_MASK = 0xFFE00000;
constexpr uint64_t SAT_NUM_SIGNAL_BLOCKS_BASE_MASK = 0xF000000000000000;

// For combined field uiCombinedField1
constexpr uint32_t SIG_SIGNAL_TYPE_MASK = 0x0000001F;
constexpr uint32_t SIG_PHASE_LOCK_MASK = 0x00000020;
constexpr uint32_t SIG_PARITY_KNOWN_MASK = 0x00000040;
constexpr uint32_t SIG_CODE_LOCK_MASK = 0x00000080;
constexpr uint32_t SIG_LOCKTIME_MASK = 0x01FFFF00;
constexpr uint32_t SIG_CORRELATOR_TYPE_MASK = 0x1E000000;
constexpr uint32_t SIG_PRIMARY_SIGNAL_MASK = 0x20000000;
constexpr uint32_t SIG_CARRIER_PHASE_MEAS_MASK = 0x40000000;
// For combined field ulCombinedField2
constexpr uint64_t SIG_CNO_MASK = 0x000000000000001F;
constexpr uint32_t SIG_CNO_SCALE_OFFSET = 20;
constexpr uint64_t SIG_PSR_STDDEV_MASK = 0x00000000000001E0;
constexpr uint64_t SIG_ADR_STDDEV_MASK = 0x0000000000001E00;
constexpr uint64_t SIG_PSR_DIFF_MASK = 0x0000000007FFE000;
constexpr double SIG_PSR_DIFF_SCALE_FACTOR = 128.0;
constexpr uint64_t SIG_PHASERANGE_DIFF_MASK = 0x00007FFFF8000000;
constexpr double SIG_PHASERANGE_DIFF_SCALE_FACTOR = 2048.0;
constexpr uint64_t SIG_DOPPLER_DIFF_MASK = 0xFFFF800000000000;
constexpr double SIG_DOPPLER_DIFF_SCALE_FACTOR = 256.0;
constexpr uint32_t SIG_DOPPLER_DIFF_SIGNEXT_MASK = 0xFFFE0000;

} // namespace rangecmp2

namespace rangecmp4 {

//-----------------------------------------------------------------------
//! RANGECMP4 data field masks, shifts and scale factors.
//! NOTE: RANGECMP4 field sizes are defined by the number of bits.
//! Unlike RANGECMP and RANGECMP2, RANGECMP4 has no consistently sized
//! data. The method of decoding will deviate from type-casting large
//! data blocks to extracting specific-sized bitfields from a byte array.
//! In order to achieve this, the size of each field (in bits, not bytes)
//! must be defined.
//! As per the comment in this header's title block, any sign extension
//! will be done according to the smallest possible data type that the
//! field can be contained in.
//-----------------------------------------------------------------------
// For mapping satellite systems defined in RANGECMP4 to the RANGE value representation.
constexpr uint32_t HEADER_BLOCK_SYSTEM_COUNT = 7;

// Valid limits for various Reference/Differential Primary/Secondary Blocks
constexpr int32_t SIG_RBLK_INVALID_PHASERANGE = -4194304;
constexpr int32_t PSIG_RBLK_INVALID_DOPPLER = -33554432;
constexpr int32_t SSIG_RBLK_INVALID_PSR = -524288;
constexpr int32_t SIG_DBLK_INVALID_PSR = -262144;
constexpr int32_t SIG_DBLK_INVALID_PHASERANGE = -32768;

// Bitfield sizes for the Satellite and Signal Block
constexpr uint32_t SATELLITE_SYSTEMS_BITS = 16;
constexpr uint32_t SATELLITES_BITS = 64;
constexpr uint32_t SIGNALS_BITS = 16;

// Bitfield sizes for the Measurement Block Header
constexpr uint32_t MBLK_HDR_DATAFORMAT_FLAG_BITS = 1;
constexpr uint32_t MBLK_HDR_REFERENCE_DATABLOCK_ID_BITS = 3;
constexpr uint32_t MBLK_HDR_GLONASS_FREQUENCY_NUMBER_BITS = 5;

// Bitfield sizes for the Reference & Differential Signal Measurement Blocks
constexpr uint32_t SIG_BLK_PARITY_FLAG_BITS = 1;
constexpr uint32_t SIG_BLK_HALF_CYCLE_BITS = 1;
constexpr uint32_t SIG_BLK_CNO_BITS = 11;
constexpr uint32_t SIG_BLK_LOCK_TIME_BITS = 4;
constexpr uint32_t SIG_BLK_PSR_STDDEV_BITS = 4;
constexpr uint32_t SIG_BLK_ADR_STDDEV_BITS = 4;
constexpr float SIG_BLK_CNO_SCALE_FACTOR = 0.05F;
constexpr double SIG_BLK_PSR_SCALE_FACTOR = 0.0005;
constexpr double SIG_BLK_PHASERANGE_SCALE_FACTOR = 0.0001;
constexpr double SIG_BLK_DOPPLER_SCALE_FACTOR = 0.0001;

// Bitfield sizes for the Differential Header
constexpr uint32_t SIG_DBLK_PSR_BITS = 19;
constexpr uint32_t SIG_DBLK_PSR_SIGNEXT_MASK = 0xFFF80000;
constexpr uint32_t SIG_DBLK_PHASERANGE_BITS = 16;
constexpr uint32_t SIG_DBLK_PHASERANGE_SIGNEXT_MASK = 0xFFFF0000;

// Bitfield sizes for the Primary and Secondary Reference Signal Measurement Blocks
constexpr uint64_t SSIG_RBLK_PSR_SIGNEXT_MASK = 0xFFFFFFFFFFF00000;
constexpr uint32_t RBLK_PHASERANGE_SIGNEXT_MASK = 0xFF800000;

constexpr std::array<int32_t, 2> DBLK_INVALID_DOPPLER = {-131072, -8192};
constexpr std::array<uint32_t, 2> DBLK_DOPPLER_BITS = {18, 14};
constexpr std::array<uint32_t, 2> DBLK_DOPPLER_SIGNEXT_MASK = {0xFFFC0000, 0xFFFFC000};
constexpr std::array<uint32_t, 2> RBLK_PSR_BITS = {37, 20};
constexpr uint32_t RBLK_PHASERANGE_BITS = 23;
constexpr std::array<uint32_t, 2> RBLK_DOPPLER_BITS = {26, 14};
constexpr std::array<uint32_t, 2> RBLK_DOPPLER_SIGNEXT_MASK = {0xFC000000, 0xFFFFC000};
constexpr std::array<int64_t, 2> RBLK_INVALID_PSR = {137438953471, -524288};

} // namespace rangecmp4

namespace rangecmp5 {

//-----------------------------------------------------------------------
//! RANGECMP5 data field masks, shifts and scale factors.
//-----------------------------------------------------------------------
// Bitfield sizes for the Satellite and Signal Block
constexpr uint32_t SATELLITE_SYSTEMS_BITS = 16;
constexpr uint32_t SATELLITES_BITS = 64;
constexpr uint32_t SIGNALS_BITS = 16;

// Valid limits for various Reference Primary/Secondary Blocks
constexpr int32_t SIG_RBLK_INVALID_PHASERANGE = -4194304;
constexpr int32_t PSIG_RBLK_INVALID_DOPPLER = -33554432;
constexpr int32_t SSIG_RBLK_INVALID_PSR = -524288;

// Bitfield sizes for the Reference Signal Measurement Blocks
constexpr uint32_t SIG_BLK_PARITY_FLAG_BITS = 1;
constexpr uint32_t SIG_BLK_HALF_CYCLE_BITS = 1;
constexpr uint32_t SIG_BLK_CNO_BITS = 11;
constexpr uint32_t SIG_BLK_LOCK_TIME_BITS = 4;
constexpr uint32_t SIG_BLK_PSR_STDDEV_BITS = 5;
constexpr uint32_t SIG_BLK_ADR_STDDEV_BITS = 5;
constexpr float SIG_BLK_CNO_SCALE_FACTOR = 0.05F;
constexpr double SIG_BLK_PSR_SCALE_FACTOR = 0.0005;
constexpr double SIG_BLK_PHASERANGE_SCALE_FACTOR = 0.0001;
constexpr double SIG_BLK_DOPPLER_SCALE_FACTOR = 0.0001;

// Bitfield sizes for the Primary and Secondary Reference Signal Measurement Blocks
constexpr uint64_t SSIG_RBLK_PSR_SIGNEXT_MASK = 0xFFFFFFFFFFF00000;
constexpr uint32_t RBLK_PHASERANGE_SIGNEXT_MASK = 0xFF800000;

constexpr std::array<uint32_t, 2> RBLK_PSR_BITS = {37, 20};
constexpr uint32_t RBLK_PHASERANGE_BITS = 23;
constexpr std::array<uint32_t, 2> RBLK_DOPPLER_BITS = {28, 16};
constexpr std::array<uint32_t, 2> RBLK_DOPPLER_SIGNEXT_MASK = {0xFC000000, 0xFFFFC000};
constexpr std::array<int64_t, 2> RBLK_INVALID_PSR = {137438953471, -524288};

} // namespace rangecmp5

//-----------------------------------------------------------------------
//! \enum SYSTEM
//! \brief Satellite Constellation System enumerations. These can also
//! be used to shift a sytem mask for decoding.
//! \note These are different than the enumerations for the Satellite
//! System field of the Channel Tracking Status word.
//-----------------------------------------------------------------------
enum class SYSTEM
{
    GPS = 0,
    GLONASS = 1,
    SBAS = 2,
    GALILEO = 5,
    BEIDOU = 6,
    QZSS = 7,
    LBAND = 8,
    NAVIC = 9,
    UNKNOWN = -1
};

#pragma pack(push, 1)

//-----------------------------------------------------------------------
//! \struct RangeData
//! \brief Range data entry from a OEM4 binary RANGE message.
//-----------------------------------------------------------------------
struct RangeData
{
    uint16_t usPRN{0};
    int16_t sGLONASSFrequency{0};
    double dPSR{0};
    float fPSRStdDev{0};
    double dADR{0};
    float fADRStdDev{0};
    float fDopplerFrequency{0};
    float fCNo{0};
    float fLockTime{0};
    uint32_t uiChannelTrackingStatus{0};

    RangeData() = default;
};

//-----------------------------------------------------------------------
//! \struct Range
//! \brief OEM4 binary RANGE message representation.
//-----------------------------------------------------------------------
struct Range
{
    uint32_t uiNumberOfObservations{0};
    RangeData astRangeData[RANGE_RECORD_MAX]{};

    Range() = default;
};

namespace rangecmp {

//-----------------------------------------------------------------------
//! \struct RangeCmpData
//! \brief Compressed range data from an OEM4 binary RANGECMP message.
//-----------------------------------------------------------------------
struct RangeCmpData
{
    uint32_t uiChannelTrackingStatus{0};
    uint64_t ulDopplerFrequencyPSRField{0}; // This is a combination of two fields; Doppler Frequency and PSR.
    uint32_t uiADR{0};
    uint8_t ucStdDevPSRStdDevADR{0}; // This is a combination of two fields; PSR Std. and ADR std.
    uint8_t ucPRN{0};
    uint32_t uiLockTimeCNoGLOFreq{0}; // This is a combination of two fields; Lock time (21b), C/No
                                      // (5b), and GLONASS Frequency number (8b)
    uint16_t usReserved{0};

    RangeCmpData() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmp
//! \brief OEM4 binary RANGECMP message representation.
//-----------------------------------------------------------------------
struct RangeCmp
{
    uint32_t uiNumberOfObservations{0};
    RangeCmpData astRangeData[RANGE_RECORD_MAX]{};

    RangeCmp() = default;
};

} // namespace rangecmp

namespace rangecmp2 {

//-----------------------------------------------------------------------
//! \struct SatelliteBlock
//! \brief Compressed satellite block from an OEM4 binary RANGECMP2 message.
//-----------------------------------------------------------------------
struct SatelliteBlock
{
    uint8_t ucSVChanNumber{0};
    uint8_t ucSatelliteIdentifier{0};
    uint64_t ulCombinedField{0};

    SatelliteBlock() = default;
};

//-----------------------------------------------------------------------
//! \struct SignalBlock
//! \brief Compressed signal block from an OEM4 binary RANGECMP2 message.
//-----------------------------------------------------------------------
struct SignalBlock
{
    uint32_t uiCombinedField1{0};
    uint64_t ullCombinedField2{0};

    SignalBlock() = default;
};

//-----------------------------------------------------------------------
//! \struct LockTimeInfo
//! \brief Store persistent data for RANGECMP2 lock time extension.
//-----------------------------------------------------------------------
struct LockTimeInfo
{
    double dLockTimeSaturatedMilliseconds{0.0}; // The time (milliseconds from OEM header) at which the lock time became saturated.
    bool bLockTimeSaturated{false};             // A flag to verify if dLockTimeSaturatedMilliseconds has been set.

    LockTimeInfo() = default;
};

//--------------------------------------------------------------------
//! \enum SIGNAL_TYPE
//! \brief RANGECMP2 message Signal Type value bitfield
//! representations as defined by RANGECMP2 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP2.htm?Highlight=Range#SignalType_RANGECMP2
//--------------------------------------------------------------------
enum class SIGNAL_TYPE
{
    // GPS
    GPS_L1CA = 1,
    GPS_L2Y = 4,
    GPS_L2CM = 5,
    GPS_L2P = 6,
    GPS_L5Q = 7,
    GPS_L1C = 15,
    // GLONASS
    GLONASS_L1CA = 1,
    GLONASS_L2CA = 3,
    GLONASS_L2P = 4,
    GLONASS_L3Q = 6,
    // SBAS
    SBAS_L1CA = 1,
    SBAS_L5I = 2,
    // GALILEO
    GALILEO_E1C = 1,
    GALILEO_E5AQ = 2,
    GALILEO_E5BQ = 3,
    GALILEO_ALTBOCQ = 4,
    GALILEO_E6C = 5,
    GALILEO_E6B = 12,
    // QZSS
    QZSS_L1CA = 1,
    QZSS_L2CM = 3,
    QZSS_L5Q = 4,
    QZSS_L1C = 8,
    QZSS_L6P = 11,
    // LBAND
    LBAND = 1,
    // BEIDOU
    BEIDOU_B1D1I = 1,
    BEIDOU_B1D2I = 2,
    BEIDOU_B2D1I = 3,
    BEIDOU_B2D2I = 4,
    BEIDOU_B3D1I = 13,
    BEIDOU_B3D2I = 16,
    BEIDOU_B1CP = 19,
    BEIDOU_B2AP = 20,
    BEIDOU_B2B_I = 21,
    // NAVIC
    NAVIC_L5SPS = 1
};

//-----------------------------------------------------------------------
//! \struct RangeCmp
//! \brief OEM4 binary RANGECMP2 message representation.
//-----------------------------------------------------------------------
struct RangeCmp
{
    uint32_t uiNumberOfRangeDataBytes{0};
    uint8_t aucRangeData[RANGE_RECORD_MAX * (sizeof(SatelliteBlock) + sizeof(SignalBlock))]{};

    RangeCmp() = default;
};

} // namespace rangecmp2

#pragma pack(pop)

namespace rangecmp4 {

//--------------------------------------------------------------------
//! \enum SIGNAL_TYPE
//! \brief RANGECMP4 message Signal Type value bitfield
//! representations as defined by RANGECMP4 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#Signal
//--------------------------------------------------------------------
enum class SIGNAL_TYPE
{
    // GPS
    GPS_L1CA = 1,
    GPS_L2Y = 4,
    GPS_L2C = 5,
    GPS_L2P = 6,
    GPS_L5Q = 7,
    GPS_L1C = 15,
    // GLONASS
    GLONASS_L1CA = 1,
    GLONASS_L2CA = 3,
    GLONASS_L2P = 4,
    GLONASS_L3 = 6,
    // SBAS
    SBAS_L1CA = 1,
    SBAS_L5I = 2,
    // GALILEO
    GALILEO_E1 = 1,
    GALILEO_E5A = 2,
    GALILEO_E5B = 3,
    GALILEO_ALTBOC = 4,
    GALILEO_E6C = 5,
    GALILEO_E6B = 12,
    // BEIDOU
    BEIDOU_B1I = 1,
    BEIDOU_B1GEO = 2,
    BEIDOU_B2I = 3,
    BEIDOU_B2GEO = 4,
    BEIDOU_B3I = 5,
    BEIDOU_B3GEO = 6,
    BEIDOU_B1CP = 7,
    BEIDOU_B2AP = 9,
    BEIDOU_B2BI = 11,
    // QZSS
    QZSS_L1CA = 1,
    QZSS_L2C = 3,
    QZSS_L5Q = 4,
    QZSS_L1C = 8,
    QZSS_L6P = 11,
    // NAVIC
    NAVIC_L5SPS = 1,

    UNKNOWN = -1
};

//! NOTE: None of the RANGECMP4 structures are packed because the method
//! of decompression is not a cast-based decode operation. The bitfields
//! are far too complex and oddly-sized such that each field must be
//! extracted from the binary data buffer one at a time. A bitfield
//! helper function should be used to determine and populate the
//! following structures. These are for containment purposes only.

//-----------------------------------------------------------------------
//! \struct MeasurementBlockHeader
//! \brief Measurement Block Header structure to contain the values
//! within the compressed bitfields for OEM4 RANGECMP4 messages.
//-----------------------------------------------------------------------
struct MeasurementBlockHeader
{
    bool bIsDifferentialData{false};
    uint8_t ucReferenceDataBlockID{0};
    int8_t cGLONASSFrequencyNumber{0};

    MeasurementBlockHeader() = default;
};

//-----------------------------------------------------------------------
//! \struct MeasurementSignalBlock
//! \brief Measurement Signal Block structure to contain the
//! values within the compressed bitfields for OEM4 RANGECMP4 messages.
//-----------------------------------------------------------------------
struct MeasurementSignalBlock
{
    bool bParityKnown{false};
    bool bHalfCycleAdded{false};
    float fCNo{0.0F};
    uint8_t ucLockTimeBitfield{0};
    uint8_t ucPSRBitfield{0};
    uint8_t ucADRBitfield{0};
    double dPSR{0.0};
    bool bValidPSR{false};
    double dPhaseRange{0.0};
    bool bValidPhaseRange{false};
    double dDoppler{0.0};
    bool bValidDoppler{false};

    MeasurementSignalBlock() = default;
};

//-----------------------------------------------------------------------
//! \struct LockTimeInfo
//! \brief Store persistent data for RANGECMP4 lock time extrapolation.
//-----------------------------------------------------------------------
struct LockTimeInfo
{
    double dMilliseconds{0.0};                           // The current running lock time for this observation.
    double dLastBitfieldChangeMilliseconds{0.0};         // The last time (milliseconds from OEM header) lock time was updated.
    uint8_t ucBits{std::numeric_limits<uint8_t>::max()}; // The last recorded bit pattern.
    bool bAbsolute{false};                               // Is the lock time absolute or relative?

    LockTimeInfo() = default;
};

constexpr uint64_t MakeKey(SYSTEM system, uint32_t satellite, SIGNAL_TYPE signal, MEASUREMENT_SOURCE source)
{
    assert(static_cast<uint64_t>(system) < 16 && satellite < 64 && static_cast<uint64_t>(signal) < 16 && static_cast<uint64_t>(source) < 2);
    return (static_cast<uint64_t>(system) << 11) | (satellite << 5) | static_cast<uint64_t>(signal) << 1 | static_cast<uint64_t>(source);
}

} // namespace rangecmp4

namespace rangecmp5 {

//-----------------------------------------------------------------------
//! \struct MeasurementBlockHeader
//! \brief Measurement Block Header structure to contain the values
//! within the compressed bitfields for OEM4 RANGECMP5 messages.
//-----------------------------------------------------------------------
struct MeasurementBlockHeader
{
    bool bDataFormatFlag{false};
    uint8_t ucReserved{0};
    int8_t cGLONASSFrequencyNumber{0};

    MeasurementBlockHeader() = default;
};

//-----------------------------------------------------------------------
//! \struct MeasurementSignalBlock
//! \brief Measurement Signal Block structure to contain the
//! values within the compressed bitfields for OEM4 RANGECMP5 messages.
//-----------------------------------------------------------------------
struct MeasurementSignalBlock
{
    bool bParityKnown{false};
    bool bHalfCycleAdded{false};
    float fCNo{0.0F};
    uint8_t ucLockTimeBitfield{0};
    uint8_t ucPseudorangeStdDev{0};
    uint8_t ucPhaserangeStdDev{0};
    double dPseudorange{0.0};
    bool bValidPseudorange{false};
    double dPhaserange{0.0};
    bool bValidPhaserange{false};
    double dDoppler{0.0};
    bool bValidDoppler{false};

    MeasurementSignalBlock() = default;
};

} // namespace rangecmp5

} // namespace novatel::edie::oem

#endif // RANGECMP_COMMON_HPP
