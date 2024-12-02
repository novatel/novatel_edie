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
    // TODO: Need to do some checking to ensure that the mask is valid (not too large) for the type of T
    return static_cast<T>((value & Mask) >> Lsb(Mask));
}

// TODO: Need to do some checking to ensure that the result is valid
template <uint32_t Mask> constexpr uint32_t EncodeBitfield(uint32_t value) { return value << Lsb(Mask) & Mask; }

template <auto Mask, typename T> constexpr void HandleSignExtension(T& value)
{
    static_assert(std::is_integral_v<T>, "Value must be an integral type.");
    static_assert(std::is_integral_v<decltype(Mask)>, "Mask must be an integral constant.");
    static_assert(sizeof(T) * 8 >= Lsb(Mask), "Mask must fit within the value type.");

    constexpr T signBit = static_cast<T>(1) << (Lsb(Mask) - 1);
    constexpr T extensionMask = static_cast<T>(Mask);

    if (value & signBit) { value |= extensionMask; }
}

template <typename T>
T ExtractBitfield(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_, const uint32_t uiBitsInBitfield_)
{
    static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "ExtractBitfield only returns integral or floating point types.");

    constexpr uint32_t typeBitSize = sizeof(T) * BITS_PER_BYTE;

    if (uiBitsInBitfield_ > typeBitSize) { return T{0}; } // return type is too small for the bitfield

    uint32_t uiBytesRequired = (uiBitsInBitfield_ + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    if (uiBytesRequired > uiBytesLeft_) { return T{0}; } // not enough bytes left in the buffer

    // Adjust remaining bytes by subtracting required bytes, accounting for any bit offset
    uiBytesLeft_ -= uiBytesRequired - ((uiBitsInBitfield_ % BITS_PER_BYTE + uiBitOffset_) < BITS_PER_BYTE);

    uint64_t ullBitfield = 0;
    uint32_t uiByteOffset = 0;
    unsigned char ucCurrentByte = **ppucData_;

    // Iterate over each bit, adding the bit to the return value if it is set.
    for (uint32_t uiBitsConsumed = 0; uiBitsConsumed < uiBitsInBitfield_; uiBitsConsumed++)
    {
        assert(uiBitOffset_ < 8);

        if ((ucCurrentByte & (1UL << uiBitOffset_)) != 0) { ullBitfield |= 1ULL << uiBitsConsumed; }

        // Rollover to the next byte when we reach the end of the current byte.
        if (++uiBitOffset_ == BITS_PER_BYTE)
        {
            uiBitOffset_ = 0;
            uiByteOffset++;
            if (uiBitsConsumed + 1 < uiBitsInBitfield_) { ucCurrentByte = *(*ppucData_ + uiByteOffset); }
        }
    }

    *ppucData_ += uiByteOffset;

    return static_cast<T>(ullBitfield);
}

//-----------------------------------------------------------------------
// Generic Constants
//-----------------------------------------------------------------------
constexpr uint32_t SPEED_OF_LIGHT = 299792458;
constexpr uint32_t MAX_VALUE = 0x800000; //!< Also 8388608, defined in RANGECMP documentation for ADR.
constexpr uint32_t BITS_PER_BYTE = 8;

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

//-----------------------------------------------------------------------
// Frequencies
//-----------------------------------------------------------------------
constexpr double FREQUENCY_HZ_GPS_L1 = 1575420000;
constexpr double FREQUENCY_HZ_GPS_L2 = 1227600000;
constexpr double FREQUENCY_HZ_GPS_L5 = 1176450000;

constexpr double FREQUENCY_HZ_GAL_E1 = FREQUENCY_HZ_GPS_L1;
constexpr double FREQUENCY_HZ_GAL_E5A = FREQUENCY_HZ_GPS_L5;
constexpr double FREQUENCY_HZ_GAL_E5B = 1207140000;
constexpr double FREQUENCY_HZ_GAL_E6 = 1278750000;
constexpr double FREQUENCY_HZ_GAL_ALTB = 1191795000;

constexpr double FREQUENCY_HZ_BDS_B1 = 1561098000;
constexpr double FREQUENCY_HZ_BDS_B1C = FREQUENCY_HZ_GPS_L1;
constexpr double FREQUENCY_HZ_BDS_B2 = FREQUENCY_HZ_GAL_E5B;
constexpr double FREQUENCY_HZ_BDS_B2A = FREQUENCY_HZ_GPS_L5;
constexpr double FREQUENCY_HZ_BDS_B2B = FREQUENCY_HZ_BDS_B2;
constexpr double FREQUENCY_HZ_BDS_B3 = 1268520000;

constexpr double GLONASS_L1_FREQUENCY_SCALE_HZ = 562500.0;
constexpr double GLONASS_L2_FREQUENCY_SCALE_HZ = 437500.0;
constexpr double GLONASS_L3_FREQUENCY_SCALE_HZ = GLONASS_L2_FREQUENCY_SCALE_HZ;

constexpr double FREQUENCY_HZ_GLO_L1 = 1602000000;
constexpr double FREQUENCY_HZ_GLO_L2 = 1246000000;
constexpr double FREQUENCY_HZ_GLO_L3 = 1202025000;

constexpr double FREQUENCY_HZ_QZSS_L1 = FREQUENCY_HZ_GPS_L1;
constexpr double FREQUENCY_HZ_QZSS_L2 = FREQUENCY_HZ_GPS_L2;
constexpr double FREQUENCY_HZ_QZSS_L5 = FREQUENCY_HZ_GPS_L5;
constexpr double FREQUENCY_HZ_QZSS_L6 = 1278750000;

//-----------------------------------------------------------------------
// Wavelengths
//-----------------------------------------------------------------------
constexpr double WAVELENGTH_GPS_L1 = SPEED_OF_LIGHT / FREQUENCY_HZ_GPS_L1;
constexpr double WAVELENGTH_GPS_L2 = SPEED_OF_LIGHT / FREQUENCY_HZ_GPS_L2;
constexpr double WAVELENGTH_GPS_L5 = SPEED_OF_LIGHT / FREQUENCY_HZ_GPS_L5;

constexpr double WAVELENGTH_GAL_E5AQ = SPEED_OF_LIGHT / FREQUENCY_HZ_GAL_E5A;
constexpr double WAVELENGTH_GAL_E5BQ = SPEED_OF_LIGHT / FREQUENCY_HZ_GAL_E5B;
constexpr double WAVELENGTH_GAL_ALTBQ = SPEED_OF_LIGHT / FREQUENCY_HZ_GAL_ALTB;
constexpr double WAVELENGTH_GAL_E1 = SPEED_OF_LIGHT / FREQUENCY_HZ_GAL_E1;
constexpr double WAVELENGTH_GAL_E6 = SPEED_OF_LIGHT / FREQUENCY_HZ_GAL_E6;

constexpr double WAVELENGTH_BDS_B1 = SPEED_OF_LIGHT / FREQUENCY_HZ_BDS_B1;
constexpr double WAVELENGTH_BDS_B1C = SPEED_OF_LIGHT / FREQUENCY_HZ_BDS_B1C;
constexpr double WAVELENGTH_BDS_B2 = SPEED_OF_LIGHT / FREQUENCY_HZ_BDS_B2;
constexpr double WAVELENGTH_BDS_B2A = SPEED_OF_LIGHT / FREQUENCY_HZ_BDS_B2A;
constexpr double WAVELENGTH_BDS_B2B = SPEED_OF_LIGHT / FREQUENCY_HZ_BDS_B2B;
constexpr double WAVELENGTH_BDS_B3 = SPEED_OF_LIGHT / FREQUENCY_HZ_BDS_B3;

constexpr double WAVELENGTH_GLO_L3 = SPEED_OF_LIGHT / FREQUENCY_HZ_GLO_L3;

constexpr double WAVELENGTH_QZSS_L1 = SPEED_OF_LIGHT / FREQUENCY_HZ_QZSS_L1;
constexpr double WAVELENGTH_QZSS_L2 = SPEED_OF_LIGHT / FREQUENCY_HZ_QZSS_L2;
constexpr double WAVELENGTH_QZSS_L5 = SPEED_OF_LIGHT / FREQUENCY_HZ_QZSS_L5;
constexpr double WAVELENGTH_QZSS_L6 = SPEED_OF_LIGHT / FREQUENCY_HZ_QZSS_L6;

constexpr double WAVELENGTH_NAVIC_L5 = SPEED_OF_LIGHT / FREQUENCY_HZ_GPS_L5;

//-----------------------------------------------------------------------
//! RANGE Channel Tracking Status data field bit masks.
//! NOTE: These masks and offsets will be used to construct or
//! deconstruct ChannelTrackingStatus words which appear in RANGE
//! messages.
//! "CTS" == "Channel Tracking Status"
//-----------------------------------------------------------------------
constexpr uint32_t CTS_TRACKING_STATE_MASK = 0x0000001F;
constexpr uint32_t CTS_SV_CHANNEL_NUMBER_MASK = 0x000003E0;
constexpr uint32_t CTS_PHASE_LOCK_MASK = 0x00000400;
constexpr uint32_t CTS_PARITY_KNOWN_MASK = 0x00000800;
constexpr uint32_t CTS_CODE_LOCKED_MASK = 0x00001000;
constexpr uint32_t CTS_CORRELATOR_MASK = 0x0000E000;
constexpr uint32_t CTS_SATELLITE_SYSTEM_MASK = 0x00070000;
constexpr uint32_t CTS_GROUPING_MASK = 0x00100000;
constexpr uint32_t CTS_SIGNAL_TYPE_MASK = 0x03E00000;
constexpr uint32_t CTS_PRIMARY_L1_CHANNEL_MASK = 0x08000000;
constexpr uint32_t CTS_CARRIER_PHASE_MASK = 0x10000000;
constexpr uint32_t CTS_DIGITAL_FILTERING_MASK = 0x20000000;
constexpr uint32_t CTS_PRN_LOCK_MASK = 0x40000000;
constexpr uint32_t CTS_CHANNEL_ASSIGNMENT_MASK = 0x80000000;

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
    double dLockTimeSaturatedMilliseconds{0.0}; // The time (milliseconds from OEM header) at which the locktime became saturated.
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
//! \struct RangeCmp4MeasurementBlockHeader
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
//! \brief Store persistent data for RANGECMP4 locktime extrapolation.
//-----------------------------------------------------------------------
struct LockTimeInfo
{
    double dMilliseconds{0.0};                           // The current running locktime for this observation.
    double dLastBitfieldChangeMilliseconds{0.0};         // The last time (milliseconds from OEM header) locktime was updated.
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

//-----------------------------------------------------------------------
//! \struct ChannelTrackingStatus
//! \brief Channel Tracking Status word fields decoded. Fields are from
//! https://docs.novatel.com/OEM7/Content/Logs/RANGE.htm#TrackingState.
//! Not every RANGECMP* message contains a raw form of the channel
//! tracking status word. It is the case that it must be constructed
//! from known data during the decompression process. However, it is
//! also highly probably that there will be fields in the channel
//! tracking status word that cannot be inferred based on the data in the
//! RANGECMP* log, and certain defaults must be applied.
//-----------------------------------------------------------------------
struct ChannelTrackingStatus
{
    //-----------------------------------------------------------------------
    //! \enum TRACKING_STATE
    //-----------------------------------------------------------------------
    enum class TRACKING_STATE
    {
        IDLE = 0,
        SKY_SEARCH = 1,
        WIDEBAND_PULLIN = 2,
        NARROWBAND_PULLIN = 3,
        PHASE_LOCK_LOOP = 4,
        CHANNEL_STEERING = 6,
        FREQUENCY_LOCK_LOOP = 7,
        CHANNEL_ALIGNMENT = 9,
        CODE_SEARCH = 10,
        AIDED_PHASE_LOCK_LOOP = 11,
        SIDE_PEAK_DETECTION = 23,
        FFT_SKY_SERACH = 24
    };

    //-----------------------------------------------------------------------
    //! \enum CORRELATOR_TYPE
    //-----------------------------------------------------------------------
    enum class CORRELATOR_TYPE
    {
        NONE = 0,
        STANDARD = 1,
        NARROW = 2,
        RESERVED = 3,
        PULSE_APERTURE = 4,
        NARROW_PULSE_APERTURE = 5,
        MULTI_PATH_ESTIMATION_AND_CORRECTION = 6
    };

    //-----------------------------------------------------------------------
    //! \enum SATELLITE_SYSTEM
    //-----------------------------------------------------------------------
    enum class SATELLITE_SYSTEM
    {
        GPS = 0,
        GLONASS = 1,
        SBAS = 2,
        GALILEO = 3,
        BEIDOU = 4,
        QZSS = 5,
        NAVIC = 6,
        OTHER = 7
    };

    //-----------------------------------------------------------------------
    //! \enum SIGNAL_TYPE
    //-----------------------------------------------------------------------
    enum class SIGNAL_TYPE
    {
        // GPS
        GPS_L1CA = 0,
        GPS_L2P = 5,
        GPS_L2Y = 9,
        GPS_L5Q = 14,
        GPS_L1CP = 16,
        GPS_L2CM = 17,
        // GLONASS
        GLONASS_L1CA = 0,
        GLONASS_L2CA = 1,
        GLONASS_L2P = 5,
        GLONASS_L3Q = 6,
        // BEIDOU
        BEIDOU_B1ID1 = 0,
        BEIDOU_B2ID1 = 1,
        BEIDOU_B3ID1 = 2,
        BEIDOU_B1ID2 = 4,
        BEIDOU_B2ID2 = 5,
        BEIDOU_B3ID2 = 6,
        BEIDOU_B1CP = 7,
        BEIDOU_B2AP = 9,
        BEIDOU_B2BI = 11,
        // GALILEO
        GALILEO_E1C = 2,
        GALILEO_E6B = 6,
        GALILEO_E6C = 7,
        GALILEO_E5AQ = 12,
        GALILEO_E5BQ = 17,
        GALILEO_E5ALTBOCQ = 20,
        // QZSS
        QZSS_L1CA = 0,
        QZSS_L5Q = 14,
        QZSS_L1CP = 16,
        QZSS_L2CM = 17,
        QZSS_L6P = 27,
        QZSS_L6D = 28,
        // SBAS
        SBAS_L1CA = 0,
        SBAS_L5I = 6,
        // NAVIC
        NAVIC_L5SPS = 0,
        // LBAND
        LBAND = 19,
        // UNKNOWN
        UNKNOWN = 0
    };

    static uint64_t MakeKey(SATELLITE_SYSTEM system, uint32_t satellite, SIGNAL_TYPE signal, MEASUREMENT_SOURCE source)
    {
        // TODO: scale PRNs down based on system?
        assert(static_cast<uint64_t>(system) < 16 && satellite < 256 && static_cast<uint64_t>(signal) < 32 && static_cast<uint64_t>(source) < 2);
        return (static_cast<uint64_t>(system) << 14) | (satellite << 6) | static_cast<uint64_t>(signal) << 1 | static_cast<uint64_t>(source);
    }

    TRACKING_STATE eTrackingState{TRACKING_STATE::IDLE};
    uint32_t uiSVChannelNumber{0U};
    bool bPhaseLocked{false};
    bool bParityKnown{false};
    bool bCodeLocked{false};
    CORRELATOR_TYPE eCorrelatorType{CORRELATOR_TYPE::NONE};
    SATELLITE_SYSTEM eSatelliteSystem{SATELLITE_SYSTEM::GPS};
    bool bGrouped{false};
    SIGNAL_TYPE eSignalType{SIGNAL_TYPE::UNKNOWN};
    bool bPrimaryL1Channel{false};
    bool bHalfCycleAdded{false};
    bool bDigitalFilteringOnSignal{false};
    bool bPRNLocked{false};
    bool bChannelAssignmentForced{false};

    //! Default constructor.
    ChannelTrackingStatus() = default;

    //! Constructor from a channel tracking status word.
    ChannelTrackingStatus(uint32_t uiChannelTrackingStatus_)
    {
        eTrackingState = GetBitfield<TRACKING_STATE, CTS_TRACKING_STATE_MASK>(uiChannelTrackingStatus_);
        uiSVChannelNumber = GetBitfield<uint32_t, CTS_SV_CHANNEL_NUMBER_MASK>(uiChannelTrackingStatus_);
        eCorrelatorType = GetBitfield<CORRELATOR_TYPE, CTS_CORRELATOR_MASK>(uiChannelTrackingStatus_);
        eSatelliteSystem = GetBitfield<SATELLITE_SYSTEM, CTS_SATELLITE_SYSTEM_MASK>(uiChannelTrackingStatus_);
        eSignalType = GetBitfield<SIGNAL_TYPE, CTS_SIGNAL_TYPE_MASK>(uiChannelTrackingStatus_);
        bPhaseLocked = GetBitfield<bool, CTS_PHASE_LOCK_MASK>(uiChannelTrackingStatus_);
        bParityKnown = GetBitfield<bool, CTS_PARITY_KNOWN_MASK>(uiChannelTrackingStatus_);
        bCodeLocked = GetBitfield<bool, CTS_CODE_LOCKED_MASK>(uiChannelTrackingStatus_);
        bGrouped = GetBitfield<bool, CTS_GROUPING_MASK>(uiChannelTrackingStatus_);
        bPrimaryL1Channel = GetBitfield<bool, CTS_PRIMARY_L1_CHANNEL_MASK>(uiChannelTrackingStatus_);
        bHalfCycleAdded = GetBitfield<bool, CTS_CARRIER_PHASE_MASK>(uiChannelTrackingStatus_);
        bDigitalFilteringOnSignal = GetBitfield<bool, CTS_DIGITAL_FILTERING_MASK>(uiChannelTrackingStatus_);
        bPRNLocked = GetBitfield<bool, CTS_PRN_LOCK_MASK>(uiChannelTrackingStatus_);
        bChannelAssignmentForced = GetBitfield<bool, CTS_CHANNEL_ASSIGNMENT_MASK>(uiChannelTrackingStatus_);
    }

    //! Constructor from the available data from a RANGECMP2 SAT/SIG block pair.
    //! NOTE: Some defaults exist here.
    ChannelTrackingStatus(const rangecmp2::SatelliteBlock& stRangeCmp2SatBlock_, const rangecmp2::SignalBlock& stRangeCmp2SigBlock_)
    {
        using namespace rangecmp2;

        bGrouped = GetBitfield<uint64_t, SAT_NUM_SIGNAL_BLOCKS_BASE_MASK>(stRangeCmp2SatBlock_.ulCombinedField) > 1;
        bPhaseLocked = GetBitfield<bool, SIG_PHASE_LOCK_MASK>(stRangeCmp2SigBlock_.uiCombinedField1);
        bParityKnown = GetBitfield<bool, SIG_PARITY_KNOWN_MASK>(stRangeCmp2SigBlock_.uiCombinedField1);
        bCodeLocked = GetBitfield<bool, SIG_CODE_LOCK_MASK>(stRangeCmp2SigBlock_.uiCombinedField1);
        bPrimaryL1Channel = GetBitfield<bool, SIG_PRIMARY_SIGNAL_MASK>(stRangeCmp2SigBlock_.uiCombinedField1);
        bHalfCycleAdded = GetBitfield<bool, SIG_CARRIER_PHASE_MEAS_MASK>(stRangeCmp2SigBlock_.uiCombinedField1);
        bDigitalFilteringOnSignal = false;
        bChannelAssignmentForced = false;
        bPRNLocked = false;
        uiSVChannelNumber = static_cast<uint32_t>(stRangeCmp2SatBlock_.ucSVChanNumber);
        eTrackingState = bPrimaryL1Channel ? TRACKING_STATE::PHASE_LOCK_LOOP : TRACKING_STATE::AIDED_PHASE_LOCK_LOOP;
        eCorrelatorType = GetBitfield<CORRELATOR_TYPE, SIG_CORRELATOR_TYPE_MASK>(stRangeCmp2SigBlock_.uiCombinedField1);
        eSatelliteSystem = SystemToSatelliteSystem(GetBitfield<SYSTEM, SAT_SATELLITE_SYSTEM_ID_MASK>(stRangeCmp2SatBlock_.ulCombinedField));
        eSignalType = RangeCmp2SignalTypeToSignalType(
            eSatelliteSystem, GetBitfield<rangecmp2::SIGNAL_TYPE, SIG_SIGNAL_TYPE_MASK>(stRangeCmp2SigBlock_.uiCombinedField1));
    }

    //! Constructor from the available data from a RANGECMP4 Primary Block and Measurement Block pair.
    ChannelTrackingStatus(SYSTEM eSystem_, rangecmp4::SIGNAL_TYPE eSignalType_, const rangecmp4::MeasurementSignalBlock& stMeasurementBlock_)
    {
        // Defaults that cannot be determined:
        eCorrelatorType = CORRELATOR_TYPE::NONE;
        uiSVChannelNumber = 0;
        bPRNLocked = false;
        bChannelAssignmentForced = false;
        bDigitalFilteringOnSignal = false;
        bGrouped = false; // Note that bGrouped can be changed once the number of signals for this PRN have been determined.

        eSatelliteSystem = SystemToSatelliteSystem(eSystem_);
        eSignalType = RangeCmp4SignalTypeToSignalType(eSatelliteSystem, eSignalType_);
        bParityKnown = stMeasurementBlock_.bParityKnown;
        bHalfCycleAdded = stMeasurementBlock_.bHalfCycleAdded;
        bCodeLocked = stMeasurementBlock_.bValidPSR;
        bPhaseLocked = stMeasurementBlock_.bValidPhaseRange;

        if (eSignalType_ == rangecmp4::SIGNAL_TYPE::GPS_L1CA || eSignalType_ == rangecmp4::SIGNAL_TYPE::GLONASS_L1CA ||
            eSignalType_ == rangecmp4::SIGNAL_TYPE::SBAS_L1CA || eSignalType_ == rangecmp4::SIGNAL_TYPE::GALILEO_E1 ||
            eSignalType_ == rangecmp4::SIGNAL_TYPE::BEIDOU_B1I || eSignalType_ == rangecmp4::SIGNAL_TYPE::QZSS_L1CA ||
            (eSatelliteSystem == SATELLITE_SYSTEM::BEIDOU && eSignalType_ == rangecmp4::SIGNAL_TYPE::BEIDOU_B1GEO))
        {
            bPrimaryL1Channel = true;
            eTrackingState = TRACKING_STATE::PHASE_LOCK_LOOP;
        }
        else
        {
            bPrimaryL1Channel = false;
            eTrackingState = TRACKING_STATE::AIDED_PHASE_LOCK_LOOP;
        }
    }

    //! Constructor from the available data from a RANGECMP5 Primary Block and Measurement Block pair.
    ChannelTrackingStatus(SYSTEM eSystem_, rangecmp4::SIGNAL_TYPE eSignalType_, const rangecmp5::MeasurementSignalBlock& stMeasurementBlock_)
    {
        // Defaults that cannot be determined:
        eCorrelatorType = CORRELATOR_TYPE::NONE;
        uiSVChannelNumber = 0;
        bPRNLocked = false;
        bChannelAssignmentForced = false;
        bDigitalFilteringOnSignal = false;
        bGrouped = false; // Note that bGrouped can be changed once the number of signals for this PRN have been determined.

        eSatelliteSystem = SystemToSatelliteSystem(eSystem_);
        eSignalType = RangeCmp4SignalTypeToSignalType(eSatelliteSystem, eSignalType_);
        bParityKnown = stMeasurementBlock_.bParityKnown;
        bHalfCycleAdded = stMeasurementBlock_.bHalfCycleAdded;
        bCodeLocked = stMeasurementBlock_.bValidPseudorange;
        bPhaseLocked = stMeasurementBlock_.bValidPhaserange;

        if (eSignalType_ == rangecmp4::SIGNAL_TYPE::GPS_L1CA || eSignalType_ == rangecmp4::SIGNAL_TYPE::GLONASS_L1CA ||
            eSignalType_ == rangecmp4::SIGNAL_TYPE::SBAS_L1CA || eSignalType_ == rangecmp4::SIGNAL_TYPE::GALILEO_E1 ||
            eSignalType_ == rangecmp4::SIGNAL_TYPE::BEIDOU_B1I || eSignalType_ == rangecmp4::SIGNAL_TYPE::QZSS_L1CA ||
            (eSatelliteSystem == SATELLITE_SYSTEM::BEIDOU && eSignalType_ == rangecmp4::SIGNAL_TYPE::BEIDOU_B1GEO))
        {
            bPrimaryL1Channel = true;
            eTrackingState = TRACKING_STATE::PHASE_LOCK_LOOP;
        }
        else
        {
            bPrimaryL1Channel = false;
            eTrackingState = TRACKING_STATE::AIDED_PHASE_LOCK_LOOP;
        }
    }

    //! Convert a RANGECMP2 signal type to the channel tracking status enumeration.
    static SIGNAL_TYPE RangeCmp2SignalTypeToSignalType(SATELLITE_SYSTEM eSystem_, rangecmp2::SIGNAL_TYPE eSignalType_)
    {
        switch (eSystem_)
        {
        case SATELLITE_SYSTEM::GPS:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::GPS_L1CA: return SIGNAL_TYPE::GPS_L1CA;
            case rangecmp2::SIGNAL_TYPE::GPS_L2Y: return SIGNAL_TYPE::GPS_L2Y;
            case rangecmp2::SIGNAL_TYPE::GPS_L2CM: return SIGNAL_TYPE::GPS_L2CM;
            case rangecmp2::SIGNAL_TYPE::GPS_L2P: return SIGNAL_TYPE::GPS_L2P;
            case rangecmp2::SIGNAL_TYPE::GPS_L5Q: return SIGNAL_TYPE::GPS_L5Q;
            case rangecmp2::SIGNAL_TYPE::GPS_L1C: return SIGNAL_TYPE::GPS_L1CP;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::GLONASS:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::GLONASS_L1CA: return SIGNAL_TYPE::GLONASS_L1CA;
            case rangecmp2::SIGNAL_TYPE::GLONASS_L2CA: return SIGNAL_TYPE::GLONASS_L2CA;
            case rangecmp2::SIGNAL_TYPE::GLONASS_L2P: return SIGNAL_TYPE::GLONASS_L2P;
            case rangecmp2::SIGNAL_TYPE::GLONASS_L3Q: return SIGNAL_TYPE::GLONASS_L3Q;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::BEIDOU:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B1D1I: return SIGNAL_TYPE::BEIDOU_B1ID1;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B1D2I: return SIGNAL_TYPE::BEIDOU_B1ID2;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B2D1I: return SIGNAL_TYPE::BEIDOU_B2ID1;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B2D2I: return SIGNAL_TYPE::BEIDOU_B2ID2;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B3D1I: return SIGNAL_TYPE::BEIDOU_B3ID1;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B3D2I: return SIGNAL_TYPE::BEIDOU_B3ID2;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B1CP: return SIGNAL_TYPE::BEIDOU_B1CP;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B2AP: return SIGNAL_TYPE::BEIDOU_B2AP;
            case rangecmp2::SIGNAL_TYPE::BEIDOU_B2B_I: return SIGNAL_TYPE::BEIDOU_B2BI;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::GALILEO:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::GALILEO_E1C: return SIGNAL_TYPE::GALILEO_E1C;
            case rangecmp2::SIGNAL_TYPE::GALILEO_E5AQ: return SIGNAL_TYPE::GALILEO_E5AQ;
            case rangecmp2::SIGNAL_TYPE::GALILEO_E5BQ: return SIGNAL_TYPE::GALILEO_E5BQ;
            case rangecmp2::SIGNAL_TYPE::GALILEO_ALTBOCQ: return SIGNAL_TYPE::GALILEO_E5ALTBOCQ;
            case rangecmp2::SIGNAL_TYPE::GALILEO_E6C: return SIGNAL_TYPE::GALILEO_E6C;
            case rangecmp2::SIGNAL_TYPE::GALILEO_E6B: return SIGNAL_TYPE::GALILEO_E6B;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::QZSS:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::QZSS_L1CA: return SIGNAL_TYPE::QZSS_L1CA;
            case rangecmp2::SIGNAL_TYPE::QZSS_L2CM: return SIGNAL_TYPE::QZSS_L2CM;
            case rangecmp2::SIGNAL_TYPE::QZSS_L5Q: return SIGNAL_TYPE::QZSS_L5Q;
            case rangecmp2::SIGNAL_TYPE::QZSS_L1C: return SIGNAL_TYPE::QZSS_L1CP;
            case rangecmp2::SIGNAL_TYPE::QZSS_L6P: return SIGNAL_TYPE::QZSS_L6P;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::SBAS:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::SBAS_L1CA: return SIGNAL_TYPE::SBAS_L1CA;
            case rangecmp2::SIGNAL_TYPE::SBAS_L5I: return SIGNAL_TYPE::SBAS_L5I;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::NAVIC:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::NAVIC_L5SPS: return SIGNAL_TYPE::NAVIC_L5SPS;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::OTHER:
            switch (eSignalType_)
            {
            case rangecmp2::SIGNAL_TYPE::LBAND: return SIGNAL_TYPE::LBAND;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        default: return SIGNAL_TYPE::UNKNOWN;
        }
    }

    //! Convert a RANGECMP4 signal type to the channel tracking status enumeration.
    static SIGNAL_TYPE RangeCmp4SignalTypeToSignalType(SATELLITE_SYSTEM eSystem_, rangecmp4::SIGNAL_TYPE eSignalType_)
    {
        switch (eSystem_)
        {
        case SATELLITE_SYSTEM::GPS:
            switch (eSignalType_)
            {
            case rangecmp4::SIGNAL_TYPE::GPS_L1CA: return SIGNAL_TYPE::GPS_L1CA;
            case rangecmp4::SIGNAL_TYPE::GPS_L2Y: return SIGNAL_TYPE::GPS_L2Y;
            case rangecmp4::SIGNAL_TYPE::GPS_L2C: return SIGNAL_TYPE::GPS_L2CM;
            case rangecmp4::SIGNAL_TYPE::GPS_L2P: return SIGNAL_TYPE::GPS_L2P;
            case rangecmp4::SIGNAL_TYPE::GPS_L5Q: return SIGNAL_TYPE::GPS_L5Q;
            case rangecmp4::SIGNAL_TYPE::GPS_L1C: return SIGNAL_TYPE::GPS_L1CP;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::GLONASS:
            switch (eSignalType_)
            {
            case rangecmp4::SIGNAL_TYPE::GLONASS_L1CA: return SIGNAL_TYPE::GLONASS_L1CA;
            case rangecmp4::SIGNAL_TYPE::GLONASS_L2CA: return SIGNAL_TYPE::GLONASS_L2CA;
            case rangecmp4::SIGNAL_TYPE::GLONASS_L2P: return SIGNAL_TYPE::GLONASS_L2P;
            case rangecmp4::SIGNAL_TYPE::GLONASS_L3: return SIGNAL_TYPE::GLONASS_L3Q;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::BEIDOU:
            switch (eSignalType_)
            {
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B1I: return SIGNAL_TYPE::BEIDOU_B1ID1;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B1GEO: return SIGNAL_TYPE::BEIDOU_B1ID2;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B2I: return SIGNAL_TYPE::BEIDOU_B2ID1;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B2GEO: return SIGNAL_TYPE::BEIDOU_B2ID2;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B3I: return SIGNAL_TYPE::BEIDOU_B3ID1;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B3GEO: return SIGNAL_TYPE::BEIDOU_B3ID2;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B1CP: return SIGNAL_TYPE::BEIDOU_B1CP;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B2AP: return SIGNAL_TYPE::BEIDOU_B2AP;
            case rangecmp4::SIGNAL_TYPE::BEIDOU_B2BI: return SIGNAL_TYPE::BEIDOU_B2BI;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::GALILEO:
            switch (eSignalType_)
            {
            case rangecmp4::SIGNAL_TYPE::GALILEO_E1: return SIGNAL_TYPE::GALILEO_E1C;
            case rangecmp4::SIGNAL_TYPE::GALILEO_E5A: return SIGNAL_TYPE::GALILEO_E5AQ;
            case rangecmp4::SIGNAL_TYPE::GALILEO_E5B: return SIGNAL_TYPE::GALILEO_E5BQ;
            case rangecmp4::SIGNAL_TYPE::GALILEO_ALTBOC: return SIGNAL_TYPE::GALILEO_E5ALTBOCQ;
            case rangecmp4::SIGNAL_TYPE::GALILEO_E6C: return SIGNAL_TYPE::GALILEO_E6C;
            case rangecmp4::SIGNAL_TYPE::GALILEO_E6B: return SIGNAL_TYPE::GALILEO_E6B;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::QZSS:
            switch (eSignalType_)
            {
            case rangecmp4::SIGNAL_TYPE::QZSS_L1CA: return SIGNAL_TYPE::QZSS_L1CA;
            case rangecmp4::SIGNAL_TYPE::QZSS_L2C: return SIGNAL_TYPE::QZSS_L2CM;
            case rangecmp4::SIGNAL_TYPE::QZSS_L5Q: return SIGNAL_TYPE::QZSS_L5Q;
            case rangecmp4::SIGNAL_TYPE::QZSS_L1C: return SIGNAL_TYPE::QZSS_L1CP;
            case rangecmp4::SIGNAL_TYPE::QZSS_L6P: return SIGNAL_TYPE::QZSS_L6P;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::SBAS:
            switch (eSignalType_)
            {
            case rangecmp4::SIGNAL_TYPE::SBAS_L1CA: return SIGNAL_TYPE::SBAS_L1CA;
            case rangecmp4::SIGNAL_TYPE::SBAS_L5I: return SIGNAL_TYPE::SBAS_L5I;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        case SATELLITE_SYSTEM::NAVIC:
            switch (eSignalType_)
            {
            case rangecmp4::SIGNAL_TYPE::NAVIC_L5SPS: return SIGNAL_TYPE::NAVIC_L5SPS;
            default: return SIGNAL_TYPE::UNKNOWN;
            }
        default: return SIGNAL_TYPE::UNKNOWN;
        }
    }

    //! Convert a SYSTEM enumeration to a channel tracking status SATELLITE_SYSTEM.
    static SATELLITE_SYSTEM SystemToSatelliteSystem(SYSTEM eSystem_)
    {
        switch (eSystem_)
        {
        case SYSTEM::GPS: return SATELLITE_SYSTEM::GPS;
        case SYSTEM::GLONASS: return SATELLITE_SYSTEM::GLONASS;
        case SYSTEM::SBAS: return SATELLITE_SYSTEM::SBAS;
        case SYSTEM::GALILEO: return SATELLITE_SYSTEM::GALILEO;
        case SYSTEM::BEIDOU: return SATELLITE_SYSTEM::BEIDOU;
        case SYSTEM::QZSS: return SATELLITE_SYSTEM::QZSS;
        case SYSTEM::NAVIC: return SATELLITE_SYSTEM::NAVIC;
        default: return SATELLITE_SYSTEM::OTHER;
        }
    }

     //------------------------------------------------------------------------------
    //! This function acts as a lookup for a signal wavelength.
    //! Uses the Satellite System and Signal fields, and in
    //! the case of GLONASS, it will use the provided GLONASS frequency.
    //------------------------------------------------------------------------------
    double GetSignalWavelength(const int16_t sGLONASSFrequency_) const
    {
        //// TODO: Size these arrays correctly
        //constexpr auto glonassL1LookupTable = [] {
        //    std::array<double, 64> arr{};
        //    for (int32_t i = 0; i < arr.size(); i++)
        //    {
        //        arr[i] = SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L1 + (i - GLONASS_FREQUENCY_OFFSET) * GLONASS_L1_FREQUENCY_SCALE_HZ);
        //    }
        //    return arr;
        //}();
        //
        //constexpr auto glonassL2LookupTable = [] {
        //    std::array<double, 64> arr{};
        //    for (int32_t i = 0; i < arr.size(); i++)
        //    {
        //        arr[i] = SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L2 + (i - GLONASS_FREQUENCY_OFFSET) * GLONASS_L2_FREQUENCY_SCALE_HZ);
        //    }
        //    return arr;
        //}();

        switch (eSatelliteSystem)
        {
        case SATELLITE_SYSTEM::GPS:
            switch (eSignalType)
            {
            case SIGNAL_TYPE::GPS_L1CA: return WAVELENGTH_GPS_L1;
            case SIGNAL_TYPE::GPS_L1CP: return WAVELENGTH_GPS_L1;
            case SIGNAL_TYPE::GPS_L2P: return WAVELENGTH_GPS_L2;
            case SIGNAL_TYPE::GPS_L2Y: return WAVELENGTH_GPS_L2;
            case SIGNAL_TYPE::GPS_L2CM: return WAVELENGTH_GPS_L2;
            case SIGNAL_TYPE::GPS_L5Q: return WAVELENGTH_GPS_L5;
            default: return 0.0;
            }
        case SATELLITE_SYSTEM::GLONASS:
            switch (eSignalType)
            {
            case SIGNAL_TYPE::GLONASS_L1CA: return SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L1 + sGLONASSFrequency_ * GLONASS_L1_FREQUENCY_SCALE_HZ);
            case SIGNAL_TYPE::GLONASS_L2CA: [[fallthrough]];
            case SIGNAL_TYPE::GLONASS_L2P: return SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L2 + sGLONASSFrequency_ * GLONASS_L2_FREQUENCY_SCALE_HZ);
            case SIGNAL_TYPE::GLONASS_L3Q: return WAVELENGTH_GLO_L3;
            default: return 0.0;
            }
        case SATELLITE_SYSTEM::SBAS:
            switch (eSignalType)
            {
            case SIGNAL_TYPE::SBAS_L1CA: return WAVELENGTH_GPS_L1;
            case SIGNAL_TYPE::SBAS_L5I: return WAVELENGTH_GPS_L5;
            default: return 0.0;
            }
        case SATELLITE_SYSTEM::GALILEO:
            switch (eSignalType)
            {
            case SIGNAL_TYPE::GALILEO_E1C: return WAVELENGTH_GAL_E1;
            case SIGNAL_TYPE::GALILEO_E6B: return WAVELENGTH_GAL_E6;
            case SIGNAL_TYPE::GALILEO_E6C: return WAVELENGTH_GAL_E6;
            case SIGNAL_TYPE::GALILEO_E5AQ: return WAVELENGTH_GAL_E5AQ;
            case SIGNAL_TYPE::GALILEO_E5BQ: return WAVELENGTH_GAL_E5BQ;
            case SIGNAL_TYPE::GALILEO_E5ALTBOCQ: return WAVELENGTH_GAL_ALTBQ;
            default: return 0.0;
            }
        case SATELLITE_SYSTEM::BEIDOU:
            switch (eSignalType)
            {
            case SIGNAL_TYPE::BEIDOU_B1ID1: return WAVELENGTH_BDS_B1;
            case SIGNAL_TYPE::BEIDOU_B1ID2: return WAVELENGTH_BDS_B1;
            case SIGNAL_TYPE::BEIDOU_B2ID1: return WAVELENGTH_BDS_B2;
            case SIGNAL_TYPE::BEIDOU_B2ID2: return WAVELENGTH_BDS_B2;
            case SIGNAL_TYPE::BEIDOU_B3ID1: return WAVELENGTH_BDS_B3;
            case SIGNAL_TYPE::BEIDOU_B3ID2: return WAVELENGTH_BDS_B3;
            case SIGNAL_TYPE::BEIDOU_B1CP: return WAVELENGTH_BDS_B1C;
            case SIGNAL_TYPE::BEIDOU_B2AP: return WAVELENGTH_BDS_B2A;
            case SIGNAL_TYPE::BEIDOU_B2BI: return WAVELENGTH_BDS_B2B;
            default: return 0.0;
            }
        case SATELLITE_SYSTEM::QZSS:
            switch (eSignalType)
            {
            case SIGNAL_TYPE::QZSS_L1CA: return WAVELENGTH_QZSS_L1;
            case SIGNAL_TYPE::QZSS_L1CP: return WAVELENGTH_QZSS_L1;
            case SIGNAL_TYPE::QZSS_L2CM: return WAVELENGTH_QZSS_L2;
            case SIGNAL_TYPE::QZSS_L5Q: return WAVELENGTH_QZSS_L5;
            case SIGNAL_TYPE::QZSS_L6P: return WAVELENGTH_QZSS_L6;
            case SIGNAL_TYPE::QZSS_L6D: return WAVELENGTH_QZSS_L6;
            default: return 0.0;
            }
        case SATELLITE_SYSTEM::NAVIC:
            switch (eSignalType)
            {
            case SIGNAL_TYPE::NAVIC_L5SPS: return WAVELENGTH_NAVIC_L5;
            default: return 0.0;
            }
        default: return 0.0;
        }
    }

    //! Combine the channel tracking status fields into a single 4-byte value according to documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGE.htm?Highlight=RANGE#Table_ChannelTrackingStatus
    [[nodiscard]] uint32_t GetAsWord() const
    {
        uint32_t uiWord = EncodeBitfield<CTS_TRACKING_STATE_MASK>(static_cast<uint32_t>(eTrackingState)) |
                          EncodeBitfield<CTS_CORRELATOR_MASK>(static_cast<uint32_t>(eCorrelatorType)) |
                          EncodeBitfield<CTS_SATELLITE_SYSTEM_MASK>(static_cast<uint32_t>(eSatelliteSystem)) |
                          EncodeBitfield<CTS_SIGNAL_TYPE_MASK>(static_cast<uint32_t>(eSignalType)) |
                          EncodeBitfield<CTS_SV_CHANNEL_NUMBER_MASK>(uiSVChannelNumber);

        if (bPhaseLocked) { uiWord |= CTS_PHASE_LOCK_MASK; }
        if (bParityKnown) { uiWord |= CTS_PARITY_KNOWN_MASK; }
        if (bCodeLocked) { uiWord |= CTS_CODE_LOCKED_MASK; }
        if (bGrouped) { uiWord |= CTS_GROUPING_MASK; }
        if (bPrimaryL1Channel) { uiWord |= CTS_PRIMARY_L1_CHANNEL_MASK; }
        if (bHalfCycleAdded) { uiWord |= CTS_CARRIER_PHASE_MASK; }
        if (bDigitalFilteringOnSignal) { uiWord |= CTS_DIGITAL_FILTERING_MASK; }
        if (bPRNLocked) { uiWord |= CTS_PRN_LOCK_MASK; }
        if (bChannelAssignmentForced) { uiWord |= CTS_CHANNEL_ASSIGNMENT_MASK; }

        return uiWord;
    }
};

} // namespace novatel::edie::oem

#endif // RANGECMP_COMMON_HPP
