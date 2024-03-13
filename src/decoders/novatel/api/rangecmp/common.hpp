////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file common.hpp
//! \brief Header file containing the common structs, enums and defines
//! used across the NovAtel range decompressor.
//! \note Generally, any *_SIGNBIT_MASK or *_SIGNEXT_MASK macros are
//! sized for the minimum-sized type that the field can be contained in.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef RANGECMP_COMMON_HPP
#define RANGECMP_COMMON_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <stdint.h>
#include <string.h>

#include <array>

namespace novatel::edie::oem {

//-----------------------------------------------------------------------
// Generic Constants
//-----------------------------------------------------------------------
constexpr uint32_t SPEED_OF_LIGHT = 299792458;
constexpr uint32_t MAX_VALUE = 0x800000; //!< Also 8388608, defined in RANGECMP documentation for ADR.
constexpr uint32_t BITS_PER_BYTE = 8;
constexpr int32_t MAGIC_NEGATE = -1; //!< Some fields are magically negated in RANGE messages when
                                     //!< translating from a compressed version.

//! NOTE: See documentation on slot/PRN offsets for specific satellite systems:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm#Satellit
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
//! RANGE Channel Tracking Status data field bit masks and shifts.
//! NOTE: These masks, shifts and offsets will be used to construct or
//! deconstruct ChannelTrackingStatus words which appear in RANGE
//! messages.
//! "CTS" == "Channel Tracking Status"
//-----------------------------------------------------------------------
constexpr uint32_t CTS_TRACKING_STATE_MASK = 0x0000001F;
constexpr uint32_t CTS_TRACKING_STATE_SHIFT = 0;
constexpr uint32_t CTS_SV_CHANNEL_NUMBER_MASK = 0x000003E0;
constexpr uint32_t CTS_SV_CHANNEL_NUMBER_SHIFT = 5;
constexpr uint32_t CTS_PHASE_LOCK_MASK = 0x00000400;
constexpr uint32_t CTS_PHASE_LOCK_SHIFT = 10;
constexpr uint32_t CTS_PARITY_KNOWN_MASK = 0x00000800;
constexpr uint32_t CTS_PARITY_KNOWN_SHIFT = 11;
constexpr uint32_t CTS_CODE_LOCKED_MASK = 0x00001000;
constexpr uint32_t CTS_CODE_LOCKED_SHIFT = 12;
constexpr uint32_t CTS_CORRELATOR_MASK = 0x0000E000;
constexpr uint32_t CTS_CORRELATOR_SHIFT = 13;
constexpr uint32_t CTS_SATELLITE_SYSTEM_MASK = 0x00070000;
constexpr uint32_t CTS_SATELLITE_SYSTEM_SHIFT = 16;
constexpr uint32_t CTS_GROUPING_MASK = 0x00100000;
constexpr uint32_t CTS_GROUPING_SHIFT = 20;
constexpr uint32_t CTS_SIGNAL_TYPE_MASK = 0x03E00000;
constexpr uint32_t CTS_SIGNAL_TYPE_SHIFT = 21;
constexpr uint32_t CTS_PRIMARY_L1_CHANNEL_MASK = 0x08000000;
constexpr uint32_t CTS_PRIMARY_L1_CHANNEL_SHIFT = 27;
constexpr uint32_t CTS_CARRIER_PHASE_MASK = 0x10000000;
constexpr uint32_t CTS_CARRIER_PHASE_SHIFT = 28;
constexpr uint32_t CTS_DIGITAL_FILTERING_MASK = 0x20000000;
constexpr uint32_t CTS_DIGITAL_FILTERING_SHIFT = 29;
constexpr uint32_t CTS_PRN_LOCK_MASK = 0x40000000;
constexpr uint32_t CTS_PRN_LOCK_SHIFT = 30;
constexpr uint32_t CTS_CHANNEL_ASSIGNMENT_MASK = 0x80000000;
constexpr uint32_t CTS_CHANNEL_ASSIGNMENT_SHIFT = 31;

//-----------------------------------------------------------------------
//! RANGECMP data field masks, shifts and scale factors.
//! NOTE: RangeCmpDataStruct defines a number of fields that can be
//! masked out from larger data types.
//-----------------------------------------------------------------------
constexpr uint64_t RC_DOPPLER_FREQUENCY_MASK = 0x000000000FFFFFFF;
constexpr uint32_t RC_DOPPLER_FREQUENCY_SIGNBIT_MASK = 0x08000000;
constexpr uint32_t RC_DOPPLER_FREQUENCY_SIGNEXT_MASK = 0xF0000000;
constexpr float RC_DOPPLER_FREQUENCY_SCALE_FACTOR = 256.0f;
constexpr uint64_t RC_PSR_MEASUREMENT_MASK = 0xFFFFFFFFF0000000;
constexpr uint32_t RC_PSR_MEASUREMENT_SHIFT = 28;
constexpr double RC_PSR_MEASUREMENT_SCALE_FACTOR = 128.0;
constexpr double RC_ADR_SCALE_FACTOR = 256.0;
constexpr uint32_t RC_PSR_STDDEV_MASK = 0x0F;
constexpr uint32_t RC_ADR_STDDEV_MASK = 0xF0;
constexpr uint32_t RC_ADR_STDDEV_SHIFT = 4;
constexpr double RC_ADR_STDDEV_SCALE_FACTOR = 512.0;
constexpr uint32_t RC_ADR_STDDEV_SCALE_OFFSET = 1;
constexpr uint32_t RC_LOCK_TIME_MASK = 0x001FFFFF;
constexpr double RC_LOCK_TIME_SCALE_FACTOR = 32.0;
constexpr uint32_t RC_CNO_MASK = 0x03E00000;
constexpr uint32_t RC_CNO_SHIFT = 21;
constexpr uint32_t RC_CNO_SCALE_OFFSET = 20;
constexpr uint32_t RC_GLONASS_FREQUENCY_MASK = 0xFC000000;
constexpr uint32_t RC_GLONASS_FREQUENCY_SHIFT = 26;

//-----------------------------------------------------------------------
//! RANGECMP2 data field masks, shifts and scale factors.
//! NOTE: RANGECMP2 contains two kinds of blocks - Satellite (SAT) and
//! Signal (SIG) - each with oddly sized bitfields.  The bitfields can be
//! masked out of larger fields (that are multiples of bytes) which
//! contain the relevant information. By utilizing bitmasks and logical
//! shift operators, these oddly sized bitfields can be contained within
//! the smallest possible datatype. As per the comment in this header's
//! title block, any sign extension will be done according to the
//! smallest possible data type that the field can be contained in.
//-----------------------------------------------------------------------
constexpr uint64_t RC2_SAT_GLONASS_FREQUENCY_ID_MASK = 0x000000000000000F;
constexpr uint64_t RC2_SAT_SATELLITE_SYSTEM_ID_MASK = 0x00000000000001F0;
constexpr uint32_t RC2_SAT_SATELLITE_SYSTEM_ID_SHIFT = 4;
constexpr uint64_t RC2_SAT_SATELLITE_PSR_BASE_MASK = 0x0000007FFFFFFC00;
constexpr uint32_t RC2_SAT_SATELLITE_PSR_BASE_SHIFT = 10;
constexpr uint32_t RC2_SAT_SATELLITE_PSR_BASE_SIGNBIT_MASK = 0x10000000;
constexpr uint32_t RC2_SAT_SATELLITE_PSR_BASE_SIGNEXT_MASK = 0xE0000000;
constexpr uint64_t RC2_SAT_SATELLITE_DOPPLER_BASE_MASK = 0x0FFFFF8000000000;
constexpr uint32_t RC2_SAT_SATELLITE_DOPPLER_BASE_SHIFT = 39;
constexpr uint32_t RC2_SAT_SATELLITE_DOPPLER_BASE_SIGNBIT_MASK = 0x00100000;
constexpr uint32_t RC2_SAT_SATELLITE_DOPPLER_BASE_SIGNEXT_MASK = 0xFFE00000;
constexpr uint64_t RC2_SAT_NUM_SIGNAL_BLOCKS_BASE_MASK = 0xF000000000000000;
constexpr uint32_t RC2_SAT_NUM_SIGNAL_BLOCKS_BASE_SHIFT = 60;

// For combined field uiCombinedField1
constexpr uint32_t RC2_SIG_SIGNAL_TYPE_MASK = 0x0000001F;
constexpr uint32_t RC2_SIG_PHASE_LOCK_MASK = 0x00000020;
constexpr uint32_t RC2_SIG_PHASE_LOCK_SHIFT = 5;
constexpr uint32_t RC2_SIG_PARITY_KNOWN_MASK = 0x00000040;
constexpr uint32_t RC2_SIG_PARITY_KNOWN_SHIFT = 6;
constexpr uint32_t RC2_SIG_CODE_LOCK_MASK = 0x00000080;
constexpr uint32_t RC2_SIG_CODE_LOCK_SHIFT = 7;
constexpr uint32_t RC2_SIG_LOCKTIME_MASK = 0x01FFFF00;
constexpr uint32_t RC2_SIG_LOCKTIME_SHIFT = 8;
constexpr uint32_t RC2_SIG_CORRELATOR_TYPE_MASK = 0x1E000000;
constexpr uint32_t RC2_SIG_CORRELATOR_TYPE_SHIFT = 25;
constexpr uint32_t RC2_SIG_PRIMARY_SIGNAL_MASK = 0x20000000;
constexpr uint32_t RC2_SIG_PRIMARY_SIGNAL_SHIFT = 29;
constexpr uint32_t RC2_SIG_CARRIER_PHASE_MEAS_MASK = 0x40000000;
constexpr uint32_t RC2_SIG_CARRIER_PHASE_MEAS_SHIFT = 30;
// For combined field ulCombinedField2
constexpr uint64_t RC2_SIG_CNO_MASK = 0x000000000000001F;
constexpr uint32_t RC2_SIG_CNO_SCALE_OFFSET = 20;
constexpr uint64_t RC2_SIG_PSR_STDDEV_MASK = 0x00000000000001E0;
constexpr uint32_t RC2_SIG_PSR_STDDEV_SHIFT = 5;
constexpr uint64_t RC2_SIG_ADR_STDDEV_MASK = 0x0000000000001E00;
constexpr uint32_t RC2_SIG_ADR_STDDEV_SHIFT = 9;
constexpr uint64_t RC2_SIG_PSR_DIFF_MASK = 0x0000000007FFE000;
constexpr uint32_t RC2_SIG_PSR_DIFF_SHIFT = 13;
constexpr double RC2_SIG_PSR_DIFF_SCALE_FACTOR = 128.0;
constexpr uint64_t RC2_SIG_PHASERANGE_DIFF_MASK = 0x00007FFFF8000000;
constexpr uint32_t RC2_SIG_PHASERANGE_DIFF_SHIFT = 27;
constexpr double RC2_SIG_PHASERANGE_DIFF_SCALE_FACTOR = 2048.0;
constexpr uint64_t RC2_SIG_DOPPLER_DIFF_MASK = 0xFFFF800000000000;
constexpr uint32_t RC2_SIG_DOPPLER_DIFF_SHIFT = 47;
constexpr double RC2_SIG_DOPPLER_DIFF_SCALE_FACTOR = 256.0;
constexpr uint32_t RC2_SIG_DOPPLER_DIFF_SIGNBIT_MASK = 0x00010000;
constexpr uint32_t RC2_SIG_DOPPLER_DIFF_SIGNEXT_MASK = 0xFFFE0000;

//-----------------------------------------------------------------------
//! RANGECMP4 data field masks, shifts and scale factors.
//! NOTE: RANGECMP4 field sizes are defined by the number of bits.
//! Unlike RANGECMP and RANGECMP2, RANGECMP4 has no consistently sized
//! data.  The method of decoding will deviate from type-casting large
//! data blocks to extracting specific-sized bitfields from a byte array.
//! In order to achieve this, the size of each field (in bits, not bytes)
//! must be defined.
//! As per the comment in this header's title block, any sign extension
//! will be done according to the smallest possible data type that the
//! field can be contained in.
//-----------------------------------------------------------------------
// For mapping satellite systems defined in RANGECMP4 to the RANGE value representation.
constexpr uint32_t RC4_HEADER_BLOCK_SYSTEM_COUNT = 7;

// Valid limits for various Reference/Differential Primary/Secondary Blocks
constexpr int32_t RC4_SIG_RBLK_INVALID_PHASERANGE = -4194304;
constexpr int32_t RC4_PSIG_RBLK_INVALID_DOPPLER = -33554432;
constexpr int32_t RC4_SSIG_RBLK_INVALID_PSR = -524288;
constexpr int32_t RC4_SIG_DBLK_INVALID_PSR = -262144;
constexpr int32_t RC4_SIG_DBLK_INVALID_PHASERANGE = -32768;

// Bitfield sizes for the Satellite and Signal Block
constexpr uint32_t RC4_SATELLITE_SYSTEMS_BITS = 16;
constexpr uint32_t RC4_SATELLITES_BITS = 64;
constexpr uint32_t RC4_SIGNALS_BITS = 16;

// Bitfield sizes for the Measurement Block Header
constexpr uint32_t RC4_MBLK_HDR_DATAFORMAT_FLAG_BITS = 1;
constexpr uint32_t RC4_MBLK_HDR_REFERENCE_DATABLOCK_ID_BITS = 3;
constexpr uint32_t RC4_MBLK_HDR_GLONASS_FREQUENCY_NUMBER_BITS = 5;

// Bitfield sizes for the Reference & Differential Signal Measurement Blocks
constexpr uint32_t RC4_SIG_BLK_PARITY_FLAG_BITS = 1;
constexpr uint32_t RC4_SIG_BLK_HALF_CYCLE_BITS = 1;
constexpr uint32_t RC4_SIG_BLK_CNO_BITS = 11;
constexpr uint32_t RC4_SIG_BLK_LOCK_TIME_BITS = 4;
constexpr uint32_t RC4_SIG_BLK_PSR_STDDEV_BITS = 4;
constexpr uint32_t RC4_SIG_BLK_ADR_STDDEV_BITS = 4;
constexpr float RC4_SIG_BLK_CNO_SCALE_FACTOR = 0.05f;
constexpr double RC4_SIG_BLK_PSR_SCALE_FACTOR = 0.0005;
constexpr double RC4_SIG_BLK_PHASERANGE_SCALE_FACTOR = 0.0001;
constexpr double RC4_SIG_BLK_DOPPLER_SCALE_FACTOR = 0.0001;

// Bitfield sizes for the Differential Header
constexpr uint32_t RC4_SIG_DBLK_PSR_BITS = 19;
constexpr uint32_t RC4_SIG_DBLK_PSR_SIGNBIT_MASK = 0x00040000;
constexpr uint32_t RC4_SIG_DBLK_PSR_SIGNEXT_MASK = 0xFFF80000;
constexpr uint32_t RC4_SIG_DBLK_PHASERANGE_BITS = 16;
constexpr uint32_t RC4_SIG_DBLK_PHASERANGE_SIGNBIT_MASK = 0x00008000;
constexpr uint32_t RC4_SIG_DBLK_PHASERANGE_SIGNEXT_MASK = 0xFFFF0000;

// Bitfield sizes for the Primary and Secondary Reference Signal Measurement Blocks
constexpr uint64_t RC4_SSIG_RBLK_PSR_SIGNBIT_MASK = 0x0000000000080000;
constexpr uint64_t RC4_SSIG_RBLK_PSR_SIGNEXT_MASK = 0xFFFFFFFFFFF00000;

constexpr uint32_t RC4_RBLK_PHASERANGE_SIGNBIT_MASK = 0x00400000;
constexpr uint32_t RC4_RBLK_PHASERANGE_SIGNEXT_MASK = 0xFF800000;

//                                                                  PRIMARY        SECONDARY
constexpr std::array<int32_t, 2> RC4_DBLK_INVALID_DOPPLER = {-131072, -8192};
constexpr std::array<uint32_t, 2> RC4_DBLK_DOPPLER_BITS = {18, 14};
constexpr std::array<uint32_t, 2> RC4_DBLK_DOPPLER_SIGNBIT_MASK = {0x00020000, 0x00002000};
constexpr std::array<uint32_t, 2> RC4_DBLK_DOPPLER_SIGNEXT_MASK = {0xFFFC0000, 0xFFFFC000};
constexpr std::array<uint32_t, 2> RC4_RBLK_PSR_BITS = {37, 20};
constexpr std::array<uint32_t, 2> RC4_RBLK_PHASERANGE_BITS = {23, 23};
constexpr std::array<uint32_t, 2> RC4_RBLK_DOPPLER_BITS = {26, 14};
constexpr std::array<uint32_t, 2> RC4_RBLK_DOPPLER_SIGNBIT_MASK = {0x02000000, 0x00002000};
constexpr std::array<uint32_t, 2> RC4_RBLK_DOPPLER_SIGNEXT_MASK = {0xFC000000, 0xFFFFC000};
constexpr std::array<int64_t, 2> RC4_RBLK_INVALID_PSR = {137438953471, -524288};

//-----------------------------------------------------------------------
//! \enum SYSTEM
//! \brief Satellite Constellation System enumerations.  These can also
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
//! \struct RangeDataStruct
//! \brief Range data entry from a OEM4 binary RANGE message.
//-----------------------------------------------------------------------
struct RangeDataStruct
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

    constexpr RangeDataStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeStruct
//! \brief OEM4 binary RANGE message representation.
//-----------------------------------------------------------------------
struct RangeStruct
{
    uint32_t uiNumberOfObservations{0};
    RangeDataStruct astRangeData[RANGE_RECORD_MAX]{};

    constexpr RangeStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmpDataStruct
//! \brief Compressed range data entry from a OEM4 binary RANGECMP
//! message.
//-----------------------------------------------------------------------
struct RangeCmpDataStruct
{
    uint32_t uiChannelTrackingStatus{0};
    uint64_t ulDopplerFrequencyPSRField{0}; // This is a combination of two fields; Doppler Frequency and PSR.
    uint32_t uiADR{0};
    uint8_t ucStdDevPSRStdDevADR{0}; // This is a combination of two fields; PSR Std. and ADR std.
    uint8_t ucPRN{0};
    uint32_t uiLockTimeCNoGLOFreq{0}; // This is a combination of two fields; Lock time (21b), C/No
                                      // (5b), and GLONASS Frequency number (8b)
    uint16_t usReserved{0};

    constexpr RangeCmpDataStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmpStruct
//! \brief OEM4 binary RANGECMP message representation.
//-----------------------------------------------------------------------
struct RangeCmpStruct
{
    uint32_t uiNumberOfObservations{0};
    RangeCmpDataStruct astRangeData[RANGE_RECORD_MAX]{};

    constexpr RangeCmpStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmp2SatelliteBlockStruct
//! \brief Compressed satellite block from a OEM4 binary RANGECMP2
//! message.
//-----------------------------------------------------------------------
struct RangeCmp2SatelliteBlockStruct
{
    uint8_t ucSVChanNumber{0};
    uint8_t ucSatelliteIdentifier{0};
    uint64_t ulCombinedField{0};

    constexpr RangeCmp2SatelliteBlockStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmp2SignalBlockStruct
//! \brief Compressed signal block from a OEM4 binary RANGECMP2 message.
//-----------------------------------------------------------------------
struct RangeCmp2SignalBlockStruct
{
    uint32_t uiCombinedField1{0};
    uint64_t ulCombinedField2{0};

    constexpr RangeCmp2SignalBlockStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmp2LocktimeInfoStruct
//! \brief Store persistent data for RANGECMP2 locktime extension.
struct RangeCmp2LocktimeInfoStruct
{
    double dLocktimeSaturatedMilliseconds{0.0}; // The time (milliseconds from OEM header) at which the locktime became saturated.
    bool bLocktimeSaturated{false};             // A flag to verify if dLocktimeSaturatedMilliseconds has been set.

    constexpr RangeCmp2LocktimeInfoStruct() = default;
};

namespace RangeCmp2 {
//--------------------------------------------------------------------
//! \enum RangeCmp2::SIGNAL_TYPE
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
} // namespace RangeCmp2

//-----------------------------------------------------------------------
//! \struct RangeCmp2Struct
//! \brief OEM4 binary RANGECMP2 message representation.
//-----------------------------------------------------------------------
struct RangeCmp2Struct
{
    uint32_t uiNumberOfRangeDataBytes{0};
    uint8_t aucRangeData[(RANGE_RECORD_MAX * (sizeof(RangeCmp2SatelliteBlockStruct) + sizeof(RangeCmp2SignalBlockStruct)))]{0};

    constexpr RangeCmp2Struct() = default;
};

#pragma pack(pop)
//! NOTE: None of the RANGECMP4 structures are packed because the method
//! of decompression is not a cast-based decode operation.  The bitfields
//! are far too complex and oddly-sized such that each field must be
//! extracted from the binary data buffer one at a time.  A bitfield
//! helper function should be used to determine and populate the
//! following structures.  These are for containment purposes only.

//-----------------------------------------------------------------------
//! \struct RangeCmp4MeasurementBlockHeaderStruct
//! \brief Measurement Block Header structure to contain the values
//! within the compressed bitfields for OEM4 RANGECMP4 messages.
//-----------------------------------------------------------------------
struct RangeCmp4MeasurementBlockHeaderStruct
{
    bool bIsDifferentialData{false};
    uint8_t ucReferenceDataBlockID{0};
    int8_t cGLONASSFrequencyNumber{0};

    constexpr RangeCmp4MeasurementBlockHeaderStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmp4MeasurementSignalBlockStruct
//! \brief Measurement Signal Block structure to contain the
//! values within the compressed bitfields for OEM4 RANGECMP4 messages.
//-----------------------------------------------------------------------
struct RangeCmp4MeasurementSignalBlockStruct
{
    bool bParityKnown{false};
    bool bHalfCycleAdded{false};
    float fCNo{0.0f};
    uint8_t ucLockTimeBitfield{0};
    uint8_t ucPSRBitfield{0};
    uint8_t ucADRBitfield{0};
    double dPSR{0.0};
    bool bValidPSR{false};
    double dPhaseRange{0.0};
    bool bValidPhaseRange{false};
    double dDoppler{0.0};
    bool bValidDoppler{false};

    constexpr RangeCmp4MeasurementSignalBlockStruct() = default;
};

//-----------------------------------------------------------------------
//! \struct RangeCmp4LocktimeInfoStruct
//! \brief Store persistent data for RANGECMP4 locktime extrapolation.
//-----------------------------------------------------------------------
struct RangeCmp4LocktimeInfoStruct
{
    double dLocktimeMilliseconds{0.0};           // The current running locktime for this observation.
    double dLastBitfieldChangeMilliseconds{0.0}; // The last time (milliseconds from OEM header) locktime was updated.
    uint8_t ucLocktimeBits{UINT8_MAX};           // The last recorded bit pattern.
    bool bLocktimeAbsolute{false};               // Is the lock time absolute or relative?

    RangeCmp4LocktimeInfoStruct() = default;
};

//-----------------------------------------------------------------------
//! \namespace RangeCmp4
//! \brief Containment for RANGECMP4-specific field values found in other
//! RANGE data representations.
//-----------------------------------------------------------------------
namespace RangeCmp4 {
//--------------------------------------------------------------------
//! \enum RangeCmp4::SIGNAL_TYPE
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
    QZSS_L6D = 10,
    QZSS_L6P = 11,
    // NAVIC
    NAVIC_L5SPS = 1,

    UNKNOWN = -1
};
} // namespace RangeCmp4

//-----------------------------------------------------------------------
//! \struct ChannelTrackingStatusStruct
//! \brief Channel Tracking Status word fields decoded.  Fields are from
//! https://docs.novatel.com/OEM7/Content/Logs/RANGE.htm#TrackingState.
//! Not every RANGECMP* message contains a raw form of the channel
//! tracking status word.  It is the case that it must be constructed
//! from known data during the decompression process.  However, it is
//! also highly probably that there will be fields in the channel
//! tracking status word that cannot be inferred based on the data in the
//! RANGECMP* log, and certain defaults must be applied.
//-----------------------------------------------------------------------
struct ChannelTrackingStatusStruct
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
        PULSE_APERATURE = 4,
        NARROW_PULSE_APERATURE = 5,
        MULTIPATH_ESTIMATION_AND_CORRECTION = 6
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
    ChannelTrackingStatusStruct() = default;

    //! Constructor from a channel tracking status word.
    ChannelTrackingStatusStruct(uint32_t uiChannelTrackingStatus_)
    {
        eTrackingState = static_cast<TRACKING_STATE>((uiChannelTrackingStatus_ & CTS_TRACKING_STATE_MASK) >> CTS_TRACKING_STATE_SHIFT);
        uiSVChannelNumber = static_cast<uint32_t>((uiChannelTrackingStatus_ & CTS_SV_CHANNEL_NUMBER_MASK) >> CTS_SV_CHANNEL_NUMBER_SHIFT);
        eCorrelatorType = static_cast<CORRELATOR_TYPE>((uiChannelTrackingStatus_ & CTS_CORRELATOR_MASK) >> CTS_CORRELATOR_SHIFT);
        eSatelliteSystem = static_cast<SATELLITE_SYSTEM>((uiChannelTrackingStatus_ & CTS_SATELLITE_SYSTEM_MASK) >> CTS_SATELLITE_SYSTEM_SHIFT);
        eSignalType = static_cast<SIGNAL_TYPE>((uiChannelTrackingStatus_ & CTS_SIGNAL_TYPE_MASK) >> CTS_SIGNAL_TYPE_SHIFT);

        bPhaseLocked = static_cast<bool>((uiChannelTrackingStatus_ & CTS_PHASE_LOCK_MASK) >> CTS_PHASE_LOCK_SHIFT);
        bParityKnown = static_cast<bool>((uiChannelTrackingStatus_ & CTS_PARITY_KNOWN_MASK) >> CTS_PARITY_KNOWN_SHIFT);
        bCodeLocked = static_cast<bool>((uiChannelTrackingStatus_ & CTS_CODE_LOCKED_MASK) >> CTS_CODE_LOCKED_SHIFT);
        bGrouped = static_cast<bool>((uiChannelTrackingStatus_ & CTS_GROUPING_MASK) >> CTS_GROUPING_SHIFT);
        bPrimaryL1Channel = static_cast<bool>((uiChannelTrackingStatus_ & CTS_PRIMARY_L1_CHANNEL_MASK) >> CTS_PRIMARY_L1_CHANNEL_SHIFT);
        bHalfCycleAdded = static_cast<bool>((uiChannelTrackingStatus_ & CTS_CARRIER_PHASE_MASK) >> CTS_CARRIER_PHASE_SHIFT);
        bDigitalFilteringOnSignal = static_cast<bool>((uiChannelTrackingStatus_ & CTS_DIGITAL_FILTERING_MASK) >> CTS_DIGITAL_FILTERING_SHIFT);
        bPRNLocked = static_cast<bool>((uiChannelTrackingStatus_ & CTS_PRN_LOCK_MASK) >> CTS_PRN_LOCK_SHIFT);
        bChannelAssignmentForced = static_cast<bool>((uiChannelTrackingStatus_ & CTS_CHANNEL_ASSIGNMENT_MASK) >> CTS_CHANNEL_ASSIGNMENT_SHIFT);
    }

    //! Constructor from the available data from a RANGECMP2 SAT/SIG block pair.
    //! NOTE: Some defaults exist here.
    ChannelTrackingStatusStruct(const RangeCmp2SatelliteBlockStruct& stRangeCmp2SatBlock_, const RangeCmp2SignalBlockStruct& stRangeCmp2SigBlock_)
    {
        bGrouped = (static_cast<uint64_t>(stRangeCmp2SatBlock_.ulCombinedField & RC2_SAT_NUM_SIGNAL_BLOCKS_BASE_MASK) >>
                    RC2_SAT_NUM_SIGNAL_BLOCKS_BASE_SHIFT) > 1;
        bPhaseLocked = static_cast<bool>((stRangeCmp2SigBlock_.uiCombinedField1 & RC2_SIG_PHASE_LOCK_MASK) >> RC2_SIG_PHASE_LOCK_SHIFT);
        bParityKnown = static_cast<bool>((stRangeCmp2SigBlock_.uiCombinedField1 & RC2_SIG_PARITY_KNOWN_MASK) >> RC2_SIG_PARITY_KNOWN_SHIFT);
        bCodeLocked = static_cast<bool>((stRangeCmp2SigBlock_.uiCombinedField1 & RC2_SIG_CODE_LOCK_MASK) >> RC2_SIG_CODE_LOCK_SHIFT);
        bPrimaryL1Channel = static_cast<bool>((stRangeCmp2SigBlock_.uiCombinedField1 & RC2_SIG_PRIMARY_SIGNAL_MASK) >> RC2_SIG_PRIMARY_SIGNAL_SHIFT);
        bHalfCycleAdded =
            static_cast<bool>((stRangeCmp2SigBlock_.uiCombinedField1 & RC2_SIG_CARRIER_PHASE_MEAS_MASK) >> RC2_SIG_CARRIER_PHASE_MEAS_SHIFT);
        bDigitalFilteringOnSignal = false;
        bChannelAssignmentForced = false;
        bPRNLocked = false;
        uiSVChannelNumber = static_cast<uint32_t>(stRangeCmp2SatBlock_.ucSVChanNumber);
        eTrackingState = bPrimaryL1Channel ? ChannelTrackingStatusStruct::TRACKING_STATE::PHASE_LOCK_LOOP
                                           : ChannelTrackingStatusStruct::TRACKING_STATE::AIDED_PHASE_LOCK_LOOP;
        eCorrelatorType =
            static_cast<CORRELATOR_TYPE>((stRangeCmp2SigBlock_.uiCombinedField1 & RC2_SIG_CORRELATOR_TYPE_MASK) >> RC2_SIG_CORRELATOR_TYPE_SHIFT);
        eSatelliteSystem = SystemToSatelliteSystem(
            static_cast<SYSTEM>((stRangeCmp2SatBlock_.ulCombinedField & RC2_SAT_SATELLITE_SYSTEM_ID_MASK) >> RC2_SAT_SATELLITE_SYSTEM_ID_SHIFT));
        eSignalType = RangeCmp2SignalTypeToSignalType(
            eSatelliteSystem, static_cast<RangeCmp2::SIGNAL_TYPE>(stRangeCmp2SigBlock_.uiCombinedField1 & RC2_SIG_SIGNAL_TYPE_MASK));
    }

    //! Constructor from the available data from a RANGECMP4 Primary Block and Measurement Block
    //! pair.
    ChannelTrackingStatusStruct(SYSTEM eSystem_, RangeCmp4::SIGNAL_TYPE eSignalType_,
                                const RangeCmp4MeasurementSignalBlockStruct& stMeasurementBlock_)
    {
        // Defaults that cannot be determined:
        eCorrelatorType = ChannelTrackingStatusStruct::CORRELATOR_TYPE::NONE;
        uiSVChannelNumber = 0;
        bPRNLocked = false;
        bChannelAssignmentForced = false;
        bDigitalFilteringOnSignal = false;
        bGrouped = false; // Note that bGrouped can be changed once the number of signals for this
                          // PRN have been determined.

        eSatelliteSystem = SystemToSatelliteSystem(eSystem_);
        eSignalType = RangeCmp4SignalTypeToSignalType(eSatelliteSystem, eSignalType_);
        bParityKnown = stMeasurementBlock_.bParityKnown;
        bHalfCycleAdded = stMeasurementBlock_.bHalfCycleAdded;
        bCodeLocked = stMeasurementBlock_.bValidPSR;
        bPhaseLocked = stMeasurementBlock_.bValidPhaseRange;

        if (eSignalType_ == RangeCmp4::SIGNAL_TYPE::GPS_L1CA || eSignalType_ == RangeCmp4::SIGNAL_TYPE::GLONASS_L1CA ||
            eSignalType_ == RangeCmp4::SIGNAL_TYPE::SBAS_L1CA || eSignalType_ == RangeCmp4::SIGNAL_TYPE::GALILEO_E1 ||
            eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B1I || eSignalType_ == RangeCmp4::SIGNAL_TYPE::QZSS_L1CA ||
            (eSatelliteSystem == SATELLITE_SYSTEM::BEIDOU && eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B1GEO))
        {
            bPrimaryL1Channel = true;
            eTrackingState = ChannelTrackingStatusStruct::TRACKING_STATE::PHASE_LOCK_LOOP;
        }
        else
        {
            bPrimaryL1Channel = false;
            eTrackingState = ChannelTrackingStatusStruct::TRACKING_STATE::AIDED_PHASE_LOCK_LOOP;
        }
    }

    //! Convert a RANGECMP2 signal type to the channel tracking status enumeration.
    SIGNAL_TYPE
    RangeCmp2SignalTypeToSignalType(SATELLITE_SYSTEM eSystem_, RangeCmp2::SIGNAL_TYPE eSignalType_)
    {
        switch (eSystem_)
        {
        case SATELLITE_SYSTEM::GPS:
            return eSignalType_ == RangeCmp2::SIGNAL_TYPE::GPS_L1CA   ? SIGNAL_TYPE::GPS_L1CA
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GPS_L2Y  ? SIGNAL_TYPE::GPS_L2Y
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GPS_L2CM ? SIGNAL_TYPE::GPS_L2CM
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GPS_L2P  ? SIGNAL_TYPE::GPS_L2P
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GPS_L5Q  ? SIGNAL_TYPE::GPS_L5Q
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GPS_L1C  ? SIGNAL_TYPE::GPS_L1CP
                                                                      : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::GLONASS:
            return eSignalType_ == RangeCmp2::SIGNAL_TYPE::GLONASS_L1CA   ? SIGNAL_TYPE::GLONASS_L1CA
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GLONASS_L2CA ? SIGNAL_TYPE::GLONASS_L2CA
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GLONASS_L2P  ? SIGNAL_TYPE::GLONASS_L2P
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GLONASS_L3Q  ? SIGNAL_TYPE::GLONASS_L3Q
                                                                          : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::BEIDOU:
            return eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B1D1I   ? SIGNAL_TYPE::BEIDOU_B1ID1
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B1D2I ? SIGNAL_TYPE::BEIDOU_B1ID2
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B2D1I ? SIGNAL_TYPE::BEIDOU_B2ID1
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B2D2I ? SIGNAL_TYPE::BEIDOU_B2ID2
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B3D1I ? SIGNAL_TYPE::BEIDOU_B3ID1
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B3D2I ? SIGNAL_TYPE::BEIDOU_B3ID2
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B1CP  ? SIGNAL_TYPE::BEIDOU_B1CP
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B2AP  ? SIGNAL_TYPE::BEIDOU_B2AP
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::BEIDOU_B2B_I ? SIGNAL_TYPE::BEIDOU_B2BI
                                                                          : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::GALILEO:
            return eSignalType_ == RangeCmp2::SIGNAL_TYPE::GALILEO_E1C       ? SIGNAL_TYPE::GALILEO_E1C
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GALILEO_E5AQ    ? SIGNAL_TYPE::GALILEO_E5AQ
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GALILEO_E5BQ    ? SIGNAL_TYPE::GALILEO_E5BQ
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GALILEO_ALTBOCQ ? SIGNAL_TYPE::GALILEO_E5ALTBOCQ
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GALILEO_E6C     ? SIGNAL_TYPE::GALILEO_E6C
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::GALILEO_E6B     ? SIGNAL_TYPE::GALILEO_E6B
                                                                             : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::QZSS:
            return eSignalType_ == RangeCmp2::SIGNAL_TYPE::QZSS_L1CA   ? SIGNAL_TYPE::QZSS_L1CA
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::QZSS_L2CM ? SIGNAL_TYPE::QZSS_L2CM
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::QZSS_L5Q  ? SIGNAL_TYPE::QZSS_L5Q
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::QZSS_L1C  ? SIGNAL_TYPE::QZSS_L1CP
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::QZSS_L6P  ? SIGNAL_TYPE::QZSS_L6P
                                                                       : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::SBAS:
            return eSignalType_ == RangeCmp2::SIGNAL_TYPE::SBAS_L1CA  ? SIGNAL_TYPE::SBAS_L1CA
                   : eSignalType_ == RangeCmp2::SIGNAL_TYPE::SBAS_L5I ? SIGNAL_TYPE::SBAS_L5I
                                                                      : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::NAVIC: return eSignalType_ == RangeCmp2::SIGNAL_TYPE::NAVIC_L5SPS ? SIGNAL_TYPE::NAVIC_L5SPS : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::OTHER: return eSignalType_ == RangeCmp2::SIGNAL_TYPE::LBAND ? SIGNAL_TYPE::LBAND : SIGNAL_TYPE::UNKNOWN;
        default: return SIGNAL_TYPE::UNKNOWN;
        }
    }

    //! Convert a RANGECMP4 signal type to the channel tracking status enumeration.
    SIGNAL_TYPE
    RangeCmp4SignalTypeToSignalType(SATELLITE_SYSTEM eSystem_, RangeCmp4::SIGNAL_TYPE eSignalType_)
    {
        switch (eSystem_)
        {
        case SATELLITE_SYSTEM::GPS:
            return eSignalType_ == RangeCmp4::SIGNAL_TYPE::GPS_L1CA  ? SIGNAL_TYPE::GPS_L1CA
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GPS_L2Y ? SIGNAL_TYPE::GPS_L2Y
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GPS_L2C ? SIGNAL_TYPE::GPS_L2CM
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GPS_L2P ? SIGNAL_TYPE::GPS_L2P
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GPS_L5Q ? SIGNAL_TYPE::GPS_L5Q
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GPS_L1C ? SIGNAL_TYPE::GPS_L1CP
                                                                     : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::GLONASS:
            return eSignalType_ == RangeCmp4::SIGNAL_TYPE::GLONASS_L1CA   ? SIGNAL_TYPE::GLONASS_L1CA
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GLONASS_L2CA ? SIGNAL_TYPE::GLONASS_L2CA
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GLONASS_L2P  ? SIGNAL_TYPE::GLONASS_L2P
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GLONASS_L3   ? SIGNAL_TYPE::GLONASS_L3Q
                                                                          : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::BEIDOU:
            return eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B1I     ? SIGNAL_TYPE::BEIDOU_B1ID1
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B1GEO ? SIGNAL_TYPE::BEIDOU_B1ID2
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B2I   ? SIGNAL_TYPE::BEIDOU_B2ID1
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B2GEO ? SIGNAL_TYPE::BEIDOU_B2ID2
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B3I   ? SIGNAL_TYPE::BEIDOU_B3ID1
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B3GEO ? SIGNAL_TYPE::BEIDOU_B3ID2
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B1CP  ? SIGNAL_TYPE::BEIDOU_B1CP
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B2AP  ? SIGNAL_TYPE::BEIDOU_B2AP
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::BEIDOU_B2BI  ? SIGNAL_TYPE::BEIDOU_B2BI
                                                                          : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::GALILEO:
            return eSignalType_ == RangeCmp4::SIGNAL_TYPE::GALILEO_E1       ? SIGNAL_TYPE::GALILEO_E1C
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GALILEO_E5A    ? SIGNAL_TYPE::GALILEO_E5AQ
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GALILEO_E5B    ? SIGNAL_TYPE::GALILEO_E5BQ
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GALILEO_ALTBOC ? SIGNAL_TYPE::GALILEO_E5ALTBOCQ
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GALILEO_E6C    ? SIGNAL_TYPE::GALILEO_E6C
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::GALILEO_E6B    ? SIGNAL_TYPE::GALILEO_E6B
                                                                            : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::QZSS:
            return eSignalType_ == RangeCmp4::SIGNAL_TYPE::QZSS_L1CA  ? SIGNAL_TYPE::QZSS_L1CA
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::QZSS_L2C ? SIGNAL_TYPE::QZSS_L2CM
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::QZSS_L5Q ? SIGNAL_TYPE::QZSS_L5Q
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::QZSS_L1C ? SIGNAL_TYPE::QZSS_L1CP
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::QZSS_L6D ? SIGNAL_TYPE::QZSS_L6D
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::QZSS_L6P ? SIGNAL_TYPE::QZSS_L6P
                                                                      : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::SBAS:
            return eSignalType_ == RangeCmp4::SIGNAL_TYPE::SBAS_L1CA  ? SIGNAL_TYPE::SBAS_L1CA
                   : eSignalType_ == RangeCmp4::SIGNAL_TYPE::SBAS_L5I ? SIGNAL_TYPE::SBAS_L5I
                                                                      : SIGNAL_TYPE::UNKNOWN;
        case SATELLITE_SYSTEM::NAVIC: return eSignalType_ == RangeCmp4::SIGNAL_TYPE::NAVIC_L5SPS ? SIGNAL_TYPE::NAVIC_L5SPS : SIGNAL_TYPE::UNKNOWN;
        default: return SIGNAL_TYPE::UNKNOWN;
        }
    }

    //! Convert a SYSTEM enumeration to a channel tracking status SATELLITE_SYSTEM.
    SATELLITE_SYSTEM
    SystemToSatelliteSystem(SYSTEM eSystem_)
    {
        return eSystem_ == SYSTEM::GPS       ? SATELLITE_SYSTEM::GPS
               : eSystem_ == SYSTEM::GLONASS ? SATELLITE_SYSTEM::GLONASS
               : eSystem_ == SYSTEM::SBAS    ? SATELLITE_SYSTEM::SBAS
               : eSystem_ == SYSTEM::GALILEO ? SATELLITE_SYSTEM::GALILEO
               : eSystem_ == SYSTEM::BEIDOU  ? SATELLITE_SYSTEM::BEIDOU
               : eSystem_ == SYSTEM::QZSS    ? SATELLITE_SYSTEM::QZSS
               : eSystem_ == SYSTEM::NAVIC   ? SATELLITE_SYSTEM::NAVIC
                                             : SATELLITE_SYSTEM::OTHER;
    }

    //! Combine the channel tracking status fields into a single 4-byte value according to
    //! documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGE.htm?Highlight=RANGE#Table_ChannelTrackingStatus
    uint32_t GetAsWord() const
    {
        uint32_t uiWord = (static_cast<uint32_t>(eTrackingState) & CTS_TRACKING_STATE_MASK) |
                          ((static_cast<uint32_t>(eCorrelatorType) << CTS_CORRELATOR_SHIFT) & CTS_CORRELATOR_MASK) |
                          ((static_cast<uint32_t>(eSatelliteSystem) << CTS_SATELLITE_SYSTEM_SHIFT) & CTS_SATELLITE_SYSTEM_MASK) |
                          ((static_cast<uint32_t>(eSignalType) << CTS_SIGNAL_TYPE_SHIFT) & CTS_SIGNAL_TYPE_MASK) |
                          ((static_cast<uint32_t>(uiSVChannelNumber) << CTS_SV_CHANNEL_NUMBER_SHIFT) & CTS_SV_CHANNEL_NUMBER_MASK);

        if (bPhaseLocked) uiWord |= CTS_PHASE_LOCK_MASK;
        if (bParityKnown) uiWord |= CTS_PARITY_KNOWN_MASK;
        if (bCodeLocked) uiWord |= CTS_CODE_LOCKED_MASK;
        if (bGrouped) uiWord |= CTS_GROUPING_MASK;
        if (bPrimaryL1Channel) uiWord |= CTS_PRIMARY_L1_CHANNEL_MASK;
        if (bHalfCycleAdded) uiWord |= CTS_CARRIER_PHASE_MASK;
        if (bDigitalFilteringOnSignal) uiWord |= CTS_DIGITAL_FILTERING_MASK;
        if (bPRNLocked) uiWord |= CTS_PRN_LOCK_MASK;
        if (bChannelAssignmentForced) uiWord |= CTS_CHANNEL_ASSIGNMENT_MASK;

        return uiWord;
    }
};

} // namespace novatel::edie::oem

#endif // RANGECMP_COMMON_HPP
