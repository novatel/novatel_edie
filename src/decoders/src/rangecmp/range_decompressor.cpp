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
// ! \file range_decompressor.cpp
// ===============================================================================

#include "novatel_edie/decoders/rangecmp/range_decompressor.hpp"

#include <stdexcept>

using namespace novatel::edie;
using namespace novatel::edie::oem;

//-----------------------------------------------------------------------
//! Table used to expand the scaled Pseudorange STDs.  This is defined
//! in the RANGECMP documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP.htm?Highlight=rangecmp#StdDevPSRValues
//-----------------------------------------------------------------------
constexpr float afTheRangeCmpPSRStdDevValues[] = {
    // 0       1       2       3       4       5       6       7
    0.050f, 0.075f, 0.113f, 0.169f, 0.253f, 0.380f, 0.570f, 0.854f,
    // 8       9       10      11      12       13       14       15
    1.281f, 2.375f, 4.750f, 9.500f, 19.000f, 38.000f, 76.000f, 152.000f};

//-----------------------------------------------------------------------
//! Table used to expand the scaled Pseudorange STDs.  This is defined
//! in the RANGECMP2 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP2.htm?Highlight=RANGECMP2#StdDevPSRScaling
//-----------------------------------------------------------------------
constexpr float afTheRangeCmp2PSRStdDevValues[] = {
    // 0       1       2       3       4       5       6       7
    0.020f, 0.030f, 0.045f, 0.066f, 0.099f, 0.148f, 0.220f, 0.329f,
    // 8       9       10      11      12      13      14      15 (>5.409)
    0.491f, 0.732f, 1.092f, 1.629f, 2.430f, 3.625f, 5.409f, 5.409f};

//-----------------------------------------------------------------------
//! Table used to expand the scaled Accumulated Doppler Range STDs.  This
//!  is defined in the RANGECMP2 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP2.htm?Highlight=RANGECMP2#StdDevADRScaling
//-----------------------------------------------------------------------
constexpr float afTheRangeCmp2ADRStdDevValues[] = {
    // 0         1         2         3         4         5         6         7
    0.00391f, 0.00521f, 0.00696f, 0.00929f, 0.01239f, 0.01654f, 0.02208f, 0.02947f,
    // 8         9         10        11        12        13        14        15 (>0.22230)
    0.03933f, 0.05249f, 0.07006f, 0.09350f, 0.12480f, 0.16656f, 0.22230f, 0.22230f};

//-----------------------------------------------------------------------
//! Two-dimensional map to look up L1/E1/B1 Scaling for RANGECMP2 signals
//! defined in the RANGECMP2 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP2.htm?Highlight=RANGECMP2#L1_E1_B1_Scaling
//-----------------------------------------------------------------------

static std::map<SYSTEM, std::map<RangeCmp2::SIGNAL_TYPE, const double>> mmTheRangeCmp2SignalScalingMapping = {
    {SYSTEM::GPS,
     {{RangeCmp2::SIGNAL_TYPE::GPS_L1C, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::GPS_L1CA, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::GPS_L2Y, (154.0 / 120.0)},
      {RangeCmp2::SIGNAL_TYPE::GPS_L2CM, (154.0 / 120.0)},
      {RangeCmp2::SIGNAL_TYPE::GPS_L5Q, (154.0 / 115.0)}}},
    {SYSTEM::GLONASS,
     {{RangeCmp2::SIGNAL_TYPE::GLONASS_L1CA, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::GLONASS_L2CA, (9.0 / 7.0)},
      {RangeCmp2::SIGNAL_TYPE::GLONASS_L2P, (9.0 / 7.0)},
      {RangeCmp2::SIGNAL_TYPE::GLONASS_L3Q, (313.0 / 235.0)}}},
    {SYSTEM::SBAS, {{RangeCmp2::SIGNAL_TYPE::SBAS_L1CA, (1.0)}, {RangeCmp2::SIGNAL_TYPE::SBAS_L5I, (154.0 / 115.0)}}},
    {SYSTEM::GALILEO,
     {{RangeCmp2::SIGNAL_TYPE::GALILEO_E1C, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::GALILEO_E5AQ, (154.0 / 115.0)},
      {RangeCmp2::SIGNAL_TYPE::GALILEO_E5BQ, (154.0 / 118.0)},
      {RangeCmp2::SIGNAL_TYPE::GALILEO_ALTBOCQ, (154.0 / 116.5)},
      {RangeCmp2::SIGNAL_TYPE::GALILEO_E6C, (154.0 / 125.0)},
      {RangeCmp2::SIGNAL_TYPE::GALILEO_E6B, (154.0 / 125.0)}}},
    {SYSTEM::BEIDOU,
     {{RangeCmp2::SIGNAL_TYPE::BEIDOU_B1D1I, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B1D2I, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B1CP, (1526.0 / 1540.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B2D1I, (1526.0 / 1180.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B2D2I, (1526.0 / 1180.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B2AP, (1526.0 / 1150.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B2B_I, (1526.0 / 1180.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B3D1I, (1526.0 / 1240.0)},
      {RangeCmp2::SIGNAL_TYPE::BEIDOU_B3D2I, (1526.0 / 1240.0)}}},
    {SYSTEM::QZSS,
     {{RangeCmp2::SIGNAL_TYPE::QZSS_L1C, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::QZSS_L1CA, (1.0)},
      {RangeCmp2::SIGNAL_TYPE::QZSS_L2CM, (154.0 / 120.0)},
      {RangeCmp2::SIGNAL_TYPE::QZSS_L5Q, (154.0 / 115.0)},
      {RangeCmp2::SIGNAL_TYPE::QZSS_L6P, (154.0 / 125.0)}}},
    {SYSTEM::LBAND, {{RangeCmp2::SIGNAL_TYPE::LBAND, (1.0)}}},
    {SYSTEM::NAVIC, {{RangeCmp2::SIGNAL_TYPE::NAVIC_L5SPS, (1.0)}}}};

//-----------------------------------------------------------------------
//! A list of bitmasks to iterate easily through a RANGECMP4 message, the
//! masks are defined in the RANGECMP4 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=RANGECMP#Header
//-----------------------------------------------------------------------
constexpr SYSTEM aeTheRangeCmp4SatelliteSystems[RC4_HEADER_BLOCK_SYSTEM_COUNT]{
    SYSTEM::GPS,     // bit 0
    SYSTEM::GLONASS, // bit 1
    SYSTEM::SBAS,    // bit 2
    SYSTEM::GALILEO, // bit 5
    SYSTEM::BEIDOU,  // bit 6
    SYSTEM::QZSS,    // bit 7
    SYSTEM::NAVIC    // bit 9
};

//-----------------------------------------------------------------------
//! Map of lists to look up and iterate signal enumerations in RANGECMP4
//! satellite and signal blocks defined in the RANGECMP4 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=RANGECMP#Signal
//-----------------------------------------------------------------------
static std::map<SYSTEM, std::vector<RangeCmp4::SIGNAL_TYPE>> mvTheRangeCmp4SystemSignalMasks = {
    {SYSTEM::GPS,
     {RangeCmp4::SIGNAL_TYPE::GPS_L1CA, RangeCmp4::SIGNAL_TYPE::GPS_L2Y, RangeCmp4::SIGNAL_TYPE::GPS_L2C, RangeCmp4::SIGNAL_TYPE::GPS_L2P,
      RangeCmp4::SIGNAL_TYPE::GPS_L5Q, RangeCmp4::SIGNAL_TYPE::GPS_L1C}},
    {SYSTEM::GLONASS,
     {

         RangeCmp4::SIGNAL_TYPE::GLONASS_L1CA, RangeCmp4::SIGNAL_TYPE::GLONASS_L2CA, RangeCmp4::SIGNAL_TYPE::GLONASS_L2P,
         RangeCmp4::SIGNAL_TYPE::GLONASS_L3}},
    {SYSTEM::SBAS, {RangeCmp4::SIGNAL_TYPE::SBAS_L1CA, RangeCmp4::SIGNAL_TYPE::SBAS_L5I}},
    {SYSTEM::GALILEO,
     {RangeCmp4::SIGNAL_TYPE::GALILEO_E1, RangeCmp4::SIGNAL_TYPE::GALILEO_E5A, RangeCmp4::SIGNAL_TYPE::GALILEO_E5B,
      RangeCmp4::SIGNAL_TYPE::GALILEO_ALTBOC, RangeCmp4::SIGNAL_TYPE::GALILEO_E6C, RangeCmp4::SIGNAL_TYPE::GALILEO_E6B}},
    {SYSTEM::BEIDOU,
     {RangeCmp4::SIGNAL_TYPE::BEIDOU_B1I, RangeCmp4::SIGNAL_TYPE::BEIDOU_B1GEO, RangeCmp4::SIGNAL_TYPE::BEIDOU_B2I,
      RangeCmp4::SIGNAL_TYPE::BEIDOU_B2GEO, RangeCmp4::SIGNAL_TYPE::BEIDOU_B3I, RangeCmp4::SIGNAL_TYPE::BEIDOU_B3GEO,
      RangeCmp4::SIGNAL_TYPE::BEIDOU_B1CP, RangeCmp4::SIGNAL_TYPE::BEIDOU_B2AP, RangeCmp4::SIGNAL_TYPE::BEIDOU_B2BI}},
    {SYSTEM::QZSS,
     {RangeCmp4::SIGNAL_TYPE::QZSS_L1CA, RangeCmp4::SIGNAL_TYPE::QZSS_L2C, RangeCmp4::SIGNAL_TYPE::QZSS_L5Q, RangeCmp4::SIGNAL_TYPE::QZSS_L1C,
      RangeCmp4::SIGNAL_TYPE::QZSS_L6D, RangeCmp4::SIGNAL_TYPE::QZSS_L6P}},
    {SYSTEM::NAVIC, {RangeCmp4::SIGNAL_TYPE::NAVIC_L5SPS}}};

//-----------------------------------------------------------------------
//! List of pre-defined floats used as translations for RANGECMP4 PSR
//! standard deviation values defined in the RANGECMP4 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#Pseudora
//-----------------------------------------------------------------------
constexpr float afTheRangeCmp4PSRStdDevValues[] = {
    // 0         1         2         3         4         5         6         7
    0.020f, 0.030f, 0.045f, 0.066f, 0.099f, 0.148f, 0.220f, 0.329f,
    // 8         9         10        11        12        13        14        15 (>5.410)
    0.491f, 0.732f, 1.092f, 1.629f, 2.430f, 3.625f, 5.409f, 5.409f};

//-----------------------------------------------------------------------
//! List of pre-defined floats used as translations for RANGECMP4 ADR
//! standard deviation values defined in the RANGECMP4 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#ADR
//! Note: LSD have been removed to reduce rouning errors in the range log.
//!       ADR STD is only 3 decimal places and could round up causing values
//!       to be greater than values in the table.
//-----------------------------------------------------------------------
constexpr float afTheRangeCmp4ADRStdDevValues[] = {
    // 0         1         2         3         4         5         6         7
    0.003f, 0.005f, 0.007f, 0.009f, 0.012f, 0.016f, 0.022f, 0.029f,
    // 8         9         10        11        12        13        14        15 (>0.2223)
    0.039f, 0.052f, 0.070f, 0.093f, 0.124f, 0.166f, 0.222f, 0.222f};

//-----------------------------------------------------------------------
//! List of pre-defined floats used as translations for RANGECMP4 lock
//! time values defined in the RANGECMP4 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#Lock
//! NOTE: These values are the lower bound of the range representations.
//! For more information on decompressing locktime bitfields, see the
//! comment block above RangeDecompressor::DetermineRangeCmp4ObservationLocktime().
//-----------------------------------------------------------------------
constexpr float afTheRangeCmp4LockTimeValues[] = {
    // 0         1         2         3         4         5         6         7
    0.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f, 512.0f, 1024.0f,
    // 8         9         10        11        12        13        14         15
    2048.0f, 4096.0f, 8192.0f, 16384.0f, 32768.0f, 65536.0f, 131072.0f, 262144.0f};

//------------------------------------------------------------------------------
RangeDecompressor::RangeDecompressor(JsonReader* pclJsonDB_) : clMyHeaderDecoder(pclJsonDB_), clMyMessageDecoder(pclJsonDB_), clMyEncoder(pclJsonDB_)
{
    pclMyLogger = Logger::RegisterLogger("range_decompressor");
    pclMyLogger->debug("RangeDecompressor initializing...");

    if (pclJsonDB_ != nullptr) { LoadJsonDb(pclJsonDB_); }

    clMyRangeCmpFilter.IncludeMessageId(RANGECMP_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
    clMyRangeCmpFilter.IncludeMessageId(RANGECMP_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
    clMyRangeCmpFilter.IncludeMessageId(RANGECMP2_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
    clMyRangeCmpFilter.IncludeMessageId(RANGECMP2_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
    clMyRangeCmpFilter.IncludeMessageId(RANGECMP3_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
    clMyRangeCmpFilter.IncludeMessageId(RANGECMP3_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
    clMyRangeCmpFilter.IncludeMessageId(RANGECMP4_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
    clMyRangeCmpFilter.IncludeMessageId(RANGECMP4_MSG_ID, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);

    pclMyLogger->debug("RangeDecompressor initialized");
}

//------------------------------------------------------------------------------
void RangeDecompressor::LoadJsonDb(JsonReader* pclJsonDB_)
{
    pclMyMsgDB = pclJsonDB_;
    clMyHeaderDecoder.LoadJsonDb(pclJsonDB_);
    clMyMessageDecoder.LoadJsonDb(pclJsonDB_);
    clMyEncoder.LoadJsonDb(pclJsonDB_);
}

// -------------------------------------------------------------------------------------------------------
void RangeDecompressor::SetLoggerLevel(spdlog::level::level_enum eLevel_) { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void RangeDecompressor::ShutdownLogger() { Logger::Shutdown(); }

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> RangeDecompressor::GetLogger() { return pclMyLogger; }

//------------------------------------------------------------------------------
//! This function acts as a lookup for a signal wavelength based on the provided
//! ChannelTrackingStatus.  Uses the Satellite System and Signal fields, and in
//! the case of GLONASS, it will use the provided GLONASS frequency.
//------------------------------------------------------------------------------
double RangeDecompressor::GetSignalWavelength(const ChannelTrackingStatusStruct& stChannelTrackingStatus_, int16_t sGLONASSFrequency_)
{
    switch (stChannelTrackingStatus_.eSatelliteSystem)
    {
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::GPS:
        return stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GPS_L1CA   ? WAVELENGTH_GPS_L1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GPS_L1CP ? WAVELENGTH_GPS_L1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GPS_L2P  ? WAVELENGTH_GPS_L2
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GPS_L2Y  ? WAVELENGTH_GPS_L2
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GPS_L2CM ? WAVELENGTH_GPS_L2
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GPS_L5Q  ? WAVELENGTH_GPS_L5
                                                                                                            : 0.0;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::GLONASS:
        return stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GLONASS_L1CA
                   ? SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L1 + (sGLONASSFrequency_ * GLONASS_L1_FREQUENCY_SCALE_HZ))
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GLONASS_L2CA
                   ? SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L2 + (sGLONASSFrequency_ * GLONASS_L2_FREQUENCY_SCALE_HZ))
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GLONASS_L2P
                   ? SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L2 + (sGLONASSFrequency_ * GLONASS_L2_FREQUENCY_SCALE_HZ))
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GLONASS_L3Q ? WAVELENGTH_GLO_L3
                                                                                                               : 0.0;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::SBAS:
        return stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::SBAS_L1CA  ? WAVELENGTH_GPS_L1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::SBAS_L5I ? WAVELENGTH_GPS_L5
                                                                                                            : 0.0;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::GALILEO:
        return stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GALILEO_E1C         ? WAVELENGTH_GAL_E1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GALILEO_E6B       ? WAVELENGTH_GAL_E6
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GALILEO_E6C       ? WAVELENGTH_GAL_E6
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GALILEO_E5AQ      ? WAVELENGTH_GAL_E5AQ
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GALILEO_E5BQ      ? WAVELENGTH_GAL_E5BQ
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::GALILEO_E5ALTBOCQ ? WAVELENGTH_GAL_ALTBQ
                                                                                                                     : 0.0;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::BEIDOU:
        return stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B1ID1   ? WAVELENGTH_BDS_B1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B1ID2 ? WAVELENGTH_BDS_B1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B2ID1 ? WAVELENGTH_BDS_B2
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B2ID2 ? WAVELENGTH_BDS_B2
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B3ID1 ? WAVELENGTH_BDS_B3
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B3ID2 ? WAVELENGTH_BDS_B3
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B1CP  ? WAVELENGTH_BDS_B1C
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B2AP  ? WAVELENGTH_BDS_B2A
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::BEIDOU_B2BI  ? WAVELENGTH_BDS_B2B
                                                                                                                : 0.0;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::QZSS:
        return stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::QZSS_L1CA   ? WAVELENGTH_QZSS_L1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::QZSS_L1CP ? WAVELENGTH_QZSS_L1
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::QZSS_L2CM ? WAVELENGTH_QZSS_L2
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::QZSS_L5Q  ? WAVELENGTH_QZSS_L5
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::QZSS_L6P  ? WAVELENGTH_QZSS_L6
               : stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::QZSS_L6D  ? WAVELENGTH_QZSS_L6
                                                                                                             : 0.0;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::NAVIC:
        return stChannelTrackingStatus_.eSignalType == ChannelTrackingStatusStruct::SIGNAL_TYPE::NAVIC_L5SPS ? WAVELENGTH_NAVIC_L5 : 0.0;
    default: return 0.0;
    }
}

//------------------------------------------------------------------------------
//! Bitfield helper function.  This will collect the number of bits specified
//! from the provided buffer pointer.  It will keep an internal track of the
//! current bit offset within a given byte.
//------------------------------------------------------------------------------
uint64_t RangeDecompressor::GetBitfieldFromBuffer(uint8_t** ppucDataBuffer_, uint32_t uiBitsInBitfield_)
{
    // If the user is asking for too many bits, don't do anything.
    if (uiBitsInBitfield_ > (sizeof(uint64_t) * BITS_PER_BYTE))
    {
        pclMyLogger->critical("Too many bits requested! Requested {}, max {}.", uiBitsInBitfield_, (sizeof(uint64_t) * BITS_PER_BYTE));
        return 0;
    }

    // If the user is asking for more bits than are available in the buffer, don't do anything.
    uint32_t uiRemainderBits = uiBitsInBitfield_ % BITS_PER_BYTE;
    uint32_t uiBytesRequired = uiBitsInBitfield_ / BITS_PER_BYTE + (uiRemainderBits ? 1 : 0);
    if (uiBytesRequired > uiMyBytesRemaining)
    {
        pclMyLogger->critical("Not enough bytes in this buffer. Required {}, have {}.", uiBytesRequired, uiMyBytesRemaining);
        return 0;
    }

    // If the requested number of bits does not run into the next byte, let the remaining bits count as a remaining byte.
    uiMyBytesRemaining -= uiBytesRequired - (((uiRemainderBits + uiMyBitOffset) < BITS_PER_BYTE) ? 1 : 0);

    uint64_t ulBitfield = 0;
    uint32_t uiByteOffset = 0;
    uint8_t ucCurrentByte = **ppucDataBuffer_;

    // Iterate over each bit, adding the bit to the 64-bit return value if it is set.
    for (uint32_t uiBitsConsumed = 0; uiBitsConsumed < uiBitsInBitfield_; uiBitsConsumed++)
    {
        if (ucCurrentByte & (1UL << uiMyBitOffset)) { ulBitfield |= (1ULL << uiBitsConsumed); }

        // Rollover to the next byte when we reach the end of the current byte.
        uiMyBitOffset++;
        if (uiMyBitOffset == BITS_PER_BYTE)
        {
            uiMyBitOffset = 0;
            uiByteOffset++;
            if (uiBitsConsumed + 1 < uiBitsInBitfield_) { ucCurrentByte = *(*ppucDataBuffer_ + uiByteOffset); }
        }
    }

    // Advance the data buffer we received by the amount of bytes that we moved.
    *ppucDataBuffer_ += uiByteOffset;
    return ulBitfield;
}

//------------------------------------------------------------------------------
//! The locktime can only report up to 131071ms (0x1FFFF).  If this value is
//! reached, the locktime must continue to increment. Once the saturated value
//! has been reached, store the header time at which the locktime was found to
//! be saturated  and any difference between the header and stored time in the
//! future can be added to the saturated locktime value to obtain the true
//! locktime value.
//! NOTE: that this is only true in the case that locktime is not saturated in
//! the first observation for this system, signal, PRN. If the locktime is
//! saturated in the first observation, the locktime is relative to the first
//! observation, and may not be a true representation of the time the
//! observation has actually been locked.
//------------------------------------------------------------------------------
float RangeDecompressor::DetermineRangeCmp2ObservationLockTime(const MetaDataStruct& stMetaData_, uint32_t uiLockTimeBits_,
                                                               ChannelTrackingStatusStruct::SATELLITE_SYSTEM eSystem_,
                                                               ChannelTrackingStatusStruct::SIGNAL_TYPE eSignal_, uint16_t usPRN_)
{
    float fLocktimeMilliseconds = static_cast<float>(uiLockTimeBits_);

    RangeCmp2LockTimeInfoStruct& stLocktimeInfo =
        ammmMyRangeCmp2LockTimes[static_cast<uint32_t>(stMetaData_.eMeasurementSource)][eSystem_][eSignal_][static_cast<uint32_t>(usPRN_)];
    if (uiLockTimeBits_ == (RC2_SIG_LOCKTIME_MASK >> RC2_SIG_LOCKTIME_SHIFT))
    {
        // If the locktime was already saturated, use the stored time to add the missing offset.
        if (stLocktimeInfo.bLockTimeSaturated)
        {
            fLocktimeMilliseconds += static_cast<float>(stMetaData_.dMilliseconds - stLocktimeInfo.dLockTimeSaturatedMilliseconds);
        }
        else // If the locktime is not already saturated, store this information if this
             // observation is seen again.
        {
            stLocktimeInfo.dLockTimeSaturatedMilliseconds = stMetaData_.dMilliseconds;
            stLocktimeInfo.bLockTimeSaturated = true;
        }
    }
    else if (stLocktimeInfo.bLockTimeSaturated) // If the locktime marked as saturated, but is not reported
                                                // as such from the RANGECMP2 message, clear the flag.
    {
        stLocktimeInfo.bLockTimeSaturated = false;
    }

    return fLocktimeMilliseconds / SEC_TO_MILLI_SEC;
}

//------------------------------------------------------------------------------
//! RANGECMP4 locktime information is categorized into certain ranges for a
//! given observation.  Decompressing this information without knowledge of the
//! precise time at which lock was acquired can result in initially misledaing
//! locktimes.  There are a number of cases which may occur when decompressing
//! the locktime bitfields from a RANGECMP4 observation:
//! NOTE: See afTheRangeCmp4LockTimeValues for bitfield:locktime translations.
//!   1. The locktime bitfield is b0000:
//!      - This is the simplest case.  The locktime will increase with the time
//!        reported by the message header.
//!   2. The locktime bitfield is b1111:
//!      - There is no way to determine when the locktime reached that range,
//!        so as for case 1, the locktime will increase with the time reported
//!        by the message header.
//!   3. The locktime bitfield is any value from b0001 to b1110:
//!      - This case is the most complex.  Because there is no guarantee that
//!        the current locktime reported has just transitioned to the lower
//!        boundary of the bitfield representation, it must be stated that the
//!        locktime is relative, and not absolute.  Only when a change in the
//!        bitfield is detected can it be guaranteed the the locktime is
//!        absolute.  At this point, the locktime can jump or slip to adjust
//!        to the newly found bitfield.  After the jump or slip occurs, the
//!        locktime must not change again, as the lower bitfield values will
//!        produce the highest degree of accuracy.
//!      - For example, if the locktime bitfield is b0111, the observation has
//!        been locked for at least 1024ms.  However it is possible the message
//!        was produced when the observation was locked for 1800ms.  This means
//!        a 776ms discrepancy exsists between what the RangeDecompressor tracks
//!        and what is reported in the RANGECMP messages.  When this discrepancy
//!        is detected, the locktime can jump to match what the RANGECMP message
//!        reports.  Thus, the distinction between "relative" and "absolute"
//!        must be made to allow the decompressor to accurately reflect
//!        observation locktimes.  See the example below:
//!
//! Locktime (t)
//!   ^
//!   |                                                 . < Continues 1:1
//!   |                                               .
//!   |                                             .
//!   |                                           /
//!   |                                         /
//!   |                                       /
//!   |                                     /
//!   |                                   /   < At this point a bitfield change
//!   |                                 *|      was found.  The transition from
//!   |                               *  |      "relative" to "absolute" time
//!   |                             *    |      results in a locktime jump as
//!   |                           *      |      seen here.
//!   |                         *       /
//!   |                       *       /
//!   |                     *       /
//!   |                   *       /
//!   | Absolute        *       /   < Relative locktime inferred by the
//!   | locktime >    *       /       RangeDecompressor.  These values
//!   |             *       /         will be output to RANGE message
//!   |           *       /           locktime fields.
//!   +-------------------------------------------------------------------->
//!                   Header time (t)
//------------------------------------------------------------------------------
float RangeDecompressor::DetermineRangeCmp4ObservationLockTime(const MetaDataStruct& stMetaData_, uint8_t ucLockTimeBits_,
                                                               ChannelTrackingStatusStruct::SATELLITE_SYSTEM eSystem_,
                                                               ChannelTrackingStatusStruct::SIGNAL_TYPE eSignal_, uint32_t uiPRN_)
{
    // Store the locktime if it is different then the once we have currently.
    RangeCmp4LocktimeInfoStruct& stLocktimeInfo =
        ammmMyRangeCmp4LockTimes[static_cast<uint32_t>(stMetaData_.eMeasurementSource)][eSystem_][eSignal_][uiPRN_];

    // Is the locktime relative and has a bitfield change been found?
    if ((!stLocktimeInfo.bLocktimeAbsolute) && (ucLockTimeBits_ != stLocktimeInfo.ucLocktimeBits))
    {
        // If the locktime bits are 0, we can immediately set this as absolute locktime.
        // Or, if the locktime bits stored for this observation are non-default (0xFF),
        // this is a transition point from relative to absolute locktime.
        if (ucLockTimeBits_ == 0 || (stLocktimeInfo.ucLocktimeBits != 0xFF && ucLockTimeBits_ > stLocktimeInfo.ucLocktimeBits))
        {
            stLocktimeInfo.bLocktimeAbsolute = true;

            // Record any locktime jump or slip for records.
            double dLocktimeChangeMilliseconds = afTheRangeCmp4LockTimeValues[ucLockTimeBits_] - stLocktimeInfo.dLocktimeMilliseconds;
            if (fabs(dLocktimeChangeMilliseconds) > std::numeric_limits<double>::epsilon())
            {
                pclMyLogger->warn("Detected a locktime jump of {}ms at time {}w, {}ms. SYSTEM: {}, SIGNAL: {}, "
                                  "PRN: {}.",
                                  std::abs(dLocktimeChangeMilliseconds), stMetaData_.usWeek, stMetaData_.dMilliseconds,
                                  static_cast<int32_t>(eSystem_), static_cast<int32_t>(eSignal_), uiPRN_);
            }
        }

        // Record the last bit change and the bitfield that was changed to.
        stLocktimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
        stLocktimeInfo.ucLocktimeBits = ucLockTimeBits_;
    } // If the locktime is absolute and the locktime bits have decreased, there was likely an
      // outage.
    else if (ucLockTimeBits_ < stLocktimeInfo.ucLocktimeBits)
    {
        // In the event of an outage of any kind, reset the locktime to relative.
        stLocktimeInfo.bLocktimeAbsolute = false;
        stLocktimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
        stLocktimeInfo.ucLocktimeBits = ucLockTimeBits_;

        // Record the slip
        pclMyLogger->warn("Detected a locktime slip (perhaps caused by an outage) of {}ms at time {}w, {}ms. "
                          "SYSTEM: {}, SIGNAL: {}, PRN: {}.",
                          (stLocktimeInfo.dLocktimeMilliseconds - afTheRangeCmp4LockTimeValues[stLocktimeInfo.ucLocktimeBits]), stMetaData_.usWeek,
                          stMetaData_.dMilliseconds, static_cast<int32_t>(eSystem_), static_cast<int32_t>(eSignal_), uiPRN_);
    }
    else
    {
        // If the locktime is absolute, and the bits have not changed after a change would be
        // expected, reset the time since last bitfield change as there may have been an outage.
        if ((stMetaData_.dMilliseconds - stLocktimeInfo.dLastBitfieldChangeMilliseconds) > (2 * afTheRangeCmp4LockTimeValues[ucLockTimeBits_]))
        {
            pclMyLogger->warn("Expected a bit change much sooner at time {}w, {}ms. SYSTEM: {}, SIGNAL: {}, PRN: "
                              "{}.",
                              stMetaData_.usWeek, stMetaData_.dMilliseconds, static_cast<int32_t>(eSystem_), static_cast<int32_t>(eSignal_), uiPRN_);
            stLocktimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
        }
    }
    stLocktimeInfo.dLocktimeMilliseconds =
        (stMetaData_.dMilliseconds - stLocktimeInfo.dLastBitfieldChangeMilliseconds) + afTheRangeCmp4LockTimeValues[stLocktimeInfo.ucLocktimeBits];
    return static_cast<float>(stLocktimeInfo.dLocktimeMilliseconds) / SEC_TO_MILLI_SEC;
}

//------------------------------------------------------------------------------
//! Decompresses a RANGECMP4 reference measurement block.  Populates the
//! provided reference block struct.
//------------------------------------------------------------------------------
template <bool bSecondary>
void RangeDecompressor::DecompressReferenceBlock(uint8_t** ppucDataPointer_, RangeCmp4MeasurementSignalBlockStruct& stReferenceBlock_,
                                                 MEASUREMENT_SOURCE eMeasurementSource_)
{
    // These fields are the same size regardless of the reference block being primary or secondary.
    stReferenceBlock_.bParityKnown = static_cast<bool>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_PARITY_FLAG_BITS));
    stReferenceBlock_.bHalfCycleAdded = static_cast<bool>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_HALF_CYCLE_BITS));
    stReferenceBlock_.fCNo = static_cast<float>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_CNO_BITS)) * RC4_SIG_BLK_CNO_SCALE_FACTOR;
    stReferenceBlock_.ucLockTimeBitfield = static_cast<uint8_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_LOCK_TIME_BITS));
    stReferenceBlock_.ucPSRBitfield = static_cast<uint8_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_PSR_STDDEV_BITS));
    stReferenceBlock_.ucADRBitfield = static_cast<uint8_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_ADR_STDDEV_BITS));

    auto llPSRBitfield = static_cast<int64_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_RBLK_PSR_BITS[bSecondary]));
    if (bSecondary && llPSRBitfield & RC4_SSIG_RBLK_PSR_SIGNBIT_MASK) { llPSRBitfield |= RC4_SSIG_RBLK_PSR_SIGNEXT_MASK; }
    auto iPhaseRangeBitfield = static_cast<int32_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_RBLK_PHASERANGE_BITS[bSecondary]));
    if (iPhaseRangeBitfield & RC4_RBLK_PHASERANGE_SIGNBIT_MASK) { iPhaseRangeBitfield |= RC4_RBLK_PHASERANGE_SIGNEXT_MASK; }
    auto iDopplerBitfield = static_cast<int32_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_RBLK_DOPPLER_BITS[bSecondary]));
    if (iDopplerBitfield & RC4_RBLK_DOPPLER_SIGNBIT_MASK[bSecondary]) { iDopplerBitfield |= RC4_RBLK_DOPPLER_SIGNEXT_MASK[bSecondary]; }

    stReferenceBlock_.bValidPSR = llPSRBitfield != RC4_RBLK_INVALID_PSR[bSecondary];
    stReferenceBlock_.dPSR = static_cast<double>(llPSRBitfield) * RC4_SIG_BLK_PSR_SCALE_FACTOR +
                             (bSecondary ? astMyLastPrimaryReferenceBlocks[static_cast<uint32_t>(eMeasurementSource_)].dPSR : 0);
    stReferenceBlock_.bValidPhaseRange = iPhaseRangeBitfield != RC4_SIG_RBLK_INVALID_PHASERANGE;
    stReferenceBlock_.dPhaseRange = static_cast<double>(iPhaseRangeBitfield) * RC4_SIG_BLK_PHASERANGE_SCALE_FACTOR + stReferenceBlock_.dPSR;
    stReferenceBlock_.bValidDoppler = iDopplerBitfield != RC4_PSIG_RBLK_INVALID_DOPPLER;
    stReferenceBlock_.dDoppler = static_cast<double>(iDopplerBitfield) * RC4_SIG_BLK_DOPPLER_SCALE_FACTOR +
                                 (bSecondary ? astMyLastPrimaryReferenceBlocks[static_cast<uint32_t>(eMeasurementSource_)].dDoppler : 0);

    if constexpr (!bSecondary)
    { // For subsequent blocks, we're going to need to store this primary
      // block.
        memcpy(&astMyLastPrimaryReferenceBlocks[static_cast<uint32_t>(eMeasurementSource_)], &stReferenceBlock_,
               sizeof(RangeCmp4MeasurementSignalBlockStruct));
    }
}

//------------------------------------------------------------------------------
//! Decompresses a RANGECMP4 differential measurement block.  Populates the
//! provided reference block struct, but must be given the appropriate
//! reference block from the same RANGECMP4 message.
//------------------------------------------------------------------------------
template <bool bIsSecondary>
void RangeDecompressor::DecompressDifferentialBlock(uint8_t** ppucDataPointer_, RangeCmp4MeasurementSignalBlockStruct& stDifferentialBlock_,
                                                    const RangeCmp4MeasurementSignalBlockStruct& stReferenceBlock_, double dSecondOffset_)
{
    // These fields are the same size regardless of the reference block being primary or secondary.
    stDifferentialBlock_.bParityKnown = static_cast<bool>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_PARITY_FLAG_BITS));
    stDifferentialBlock_.bHalfCycleAdded = static_cast<bool>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_HALF_CYCLE_BITS));
    stDifferentialBlock_.fCNo = static_cast<float>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_CNO_BITS)) * RC4_SIG_BLK_CNO_SCALE_FACTOR;
    stDifferentialBlock_.ucLockTimeBitfield = static_cast<uint8_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_LOCK_TIME_BITS));
    stDifferentialBlock_.ucPSRBitfield = static_cast<uint8_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_PSR_STDDEV_BITS));
    stDifferentialBlock_.ucADRBitfield = static_cast<uint8_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_BLK_ADR_STDDEV_BITS));

    auto iPSRBitfield = static_cast<int32_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_DBLK_PSR_BITS));
    if (iPSRBitfield & RC4_SIG_DBLK_PSR_SIGNBIT_MASK) { iPSRBitfield |= RC4_SIG_DBLK_PSR_SIGNEXT_MASK; }
    auto iPhaseRangeBitfield = static_cast<int32_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_SIG_DBLK_PHASERANGE_BITS));
    if (iPhaseRangeBitfield & RC4_SIG_DBLK_PHASERANGE_SIGNBIT_MASK) { iPhaseRangeBitfield |= RC4_SIG_DBLK_PHASERANGE_SIGNEXT_MASK; }
    auto iDopplerBitfield = static_cast<int32_t>(GetBitfieldFromBuffer(ppucDataPointer_, RC4_DBLK_DOPPLER_BITS[bIsSecondary]));
    if (iDopplerBitfield & RC4_DBLK_DOPPLER_SIGNBIT_MASK[bIsSecondary]) { iDopplerBitfield |= RC4_DBLK_DOPPLER_SIGNEXT_MASK[bIsSecondary]; }

    stDifferentialBlock_.bValidPSR = iPSRBitfield != RC4_SIG_DBLK_INVALID_PSR;
    stDifferentialBlock_.dPSR =
        static_cast<double>(iPSRBitfield) * RC4_SIG_BLK_PSR_SCALE_FACTOR + stReferenceBlock_.dPSR + (stReferenceBlock_.dDoppler * dSecondOffset_);
    stDifferentialBlock_.bValidPhaseRange = iPhaseRangeBitfield != RC4_SIG_DBLK_INVALID_PHASERANGE;
    stDifferentialBlock_.dPhaseRange = static_cast<double>(iPhaseRangeBitfield) * RC4_SIG_BLK_PHASERANGE_SCALE_FACTOR +
                                       stReferenceBlock_.dPhaseRange + (stReferenceBlock_.dDoppler * dSecondOffset_);
    stDifferentialBlock_.bValidDoppler = iDopplerBitfield != RC4_DBLK_INVALID_DOPPLER[bIsSecondary];
    stDifferentialBlock_.dDoppler = static_cast<double>(iDopplerBitfield) * RC4_SIG_BLK_DOPPLER_SCALE_FACTOR + stReferenceBlock_.dDoppler;
}

//------------------------------------------------------------------------------
//! Populates a provided RangeData structure from the RANGECMP4 blocks
//! provided.
//------------------------------------------------------------------------------
void RangeDecompressor::PopulateNextRangeData(RangeDataStruct& stRangeData_, const RangeCmp4MeasurementSignalBlockStruct& stBlock_,
                                              const MetaDataStruct& stMetaData_, const ChannelTrackingStatusStruct& stChannelTrackingStatus_,
                                              uint32_t uiPRN_, char cGLONASSFrequencyNumber_)
{
    double dSignalWavelength =
        GetSignalWavelength(stChannelTrackingStatus_, static_cast<int16_t>(cGLONASSFrequencyNumber_ - GLONASS_FREQUENCY_NUMBER_OFFSET));

    //! Some logic for PRN offsets based on the constellation.  See documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm#Measurem
    switch (stChannelTrackingStatus_.eSatelliteSystem)
    {
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::GLONASS:
        // If ternary returns true, documentation suggests we should save this PRN as
        // GLONASS_SLOT_UNKNOWN_UPPER_LIMIT - cGLONASSFrequencyNumber_. However, this
        // would output the PRN as an actual valid Slot ID, which is not true. We will
        // set this to 0 here because 0 is considered an unknown/invalid GLONASS Slot ID.
        stRangeData_.usPRN = (GLONASS_SLOT_UNKNOWN_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= GLONASS_SLOT_UNKNOWN_UPPER_LIMIT)
                                 ? 0
                                 : static_cast<uint16_t>(uiPRN_) + GLONASS_SLOT_OFFSET - 1;
        break;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::SBAS:
        stRangeData_.usPRN = (SBAS_PRN_OFFSET_120_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_120_UPPER_LIMIT)
                                 ? static_cast<uint16_t>(uiPRN_ + SBAS_PRN_OFFSET_120 - 1)
                             : (SBAS_PRN_OFFSET_130_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_130_UPPER_LIMIT)
                                 ? static_cast<uint16_t>(uiPRN_ + SBAS_PRN_OFFSET_130 - 1)
                                 : 0;
        break;
    case ChannelTrackingStatusStruct::SATELLITE_SYSTEM::QZSS: stRangeData_.usPRN = static_cast<uint16_t>(uiPRN_ + QZSS_PRN_OFFSET - 1); break;
    default: stRangeData_.usPRN = static_cast<uint16_t>(uiPRN_); break;
    }

    if (stRangeData_.usPRN == 0) { throw std::runtime_error("PopulateNextRangeData(): PRN outside of limits"); }

    // any fields flagged as invalid are set to NaN and appear in the log as such
    stRangeData_.sGLONASSFrequency = static_cast<int16_t>(cGLONASSFrequencyNumber_);
    stRangeData_.dPSR = stBlock_.bValidPSR ? stBlock_.dPSR : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fPSRStdDev = afTheRangeCmp4PSRStdDevValues[stBlock_.ucPSRBitfield];
    stRangeData_.dADR =
        stBlock_.bValidPhaseRange ? MAGIC_NEGATE * (stBlock_.dPhaseRange / dSignalWavelength) : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fADRStdDev = afTheRangeCmp4ADRStdDevValues[stBlock_.ucADRBitfield];
    stRangeData_.fDopplerFrequency =
        stBlock_.bValidDoppler ? static_cast<float>(MAGIC_NEGATE * (stBlock_.dDoppler / dSignalWavelength)) : std::numeric_limits<float>::quiet_NaN();
    stRangeData_.fCNo = stBlock_.fCNo;
    stRangeData_.fLockTime = DetermineRangeCmp4ObservationLockTime(
        stMetaData_, stBlock_.ucLockTimeBitfield, stChannelTrackingStatus_.eSatelliteSystem, stChannelTrackingStatus_.eSignalType, uiPRN_);
    stRangeData_.uiChannelTrackingStatus = stChannelTrackingStatus_.GetAsWord();
}

//------------------------------------------------------------------------------
//! Convert a RANGECMP message into RANGE message.
//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmpToRange(const RangeCmpStruct& stRangeCmpMessage_, RangeStruct& stRangeMessage_)
{
    uint32_t uiChannelTrackingStatus = 0;
    stRangeMessage_.uiNumberOfObservations = stRangeCmpMessage_.uiNumberOfObservations;

    // Decompress each field from the RangeCmpData and put it into the respective RangeData.
    for (uint32_t uiRangeDataIndex = 0; uiRangeDataIndex < stRangeMessage_.uiNumberOfObservations; uiRangeDataIndex++)
    {
        RangeDataStruct& stRangeData = stRangeMessage_.astRangeData[uiRangeDataIndex];
        const RangeCmpDataStruct& stRangeCmpData = stRangeCmpMessage_.astRangeData[uiRangeDataIndex];

        // Grab the channel tracking status word and put it into our structure to use it later
        uiChannelTrackingStatus = stRangeCmpData.uiChannelTrackingStatus;
        ChannelTrackingStatusStruct stChannelTrackingStatus(uiChannelTrackingStatus);

        stRangeData.uiChannelTrackingStatus = uiChannelTrackingStatus;
        stRangeData.usPRN = static_cast<uint16_t>(stRangeCmpData.ucPRN);

        // Extend the sign
        auto iTempDoppler = static_cast<int32_t>(stRangeCmpData.ulDopplerFrequencyPSRField & RC_DOPPLER_FREQUENCY_MASK);
        if (iTempDoppler & RC_DOPPLER_FREQUENCY_SIGNBIT_MASK) { iTempDoppler |= RC_DOPPLER_FREQUENCY_SIGNEXT_MASK; }
        stRangeData.fDopplerFrequency = static_cast<float>(iTempDoppler / RC_DOPPLER_FREQUENCY_SCALE_FACTOR);

        stRangeData.dPSR = static_cast<double>(((stRangeCmpData.ulDopplerFrequencyPSRField & RC_PSR_MEASUREMENT_MASK) >> RC_PSR_MEASUREMENT_SHIFT) /
                                               RC_PSR_MEASUREMENT_SCALE_FACTOR);
        stRangeData.fPSRStdDev = afTheRangeCmpPSRStdDevValues[static_cast<uint8_t>(stRangeCmpData.ucStdDevPSRStdDevADR & RC_PSR_STDDEV_MASK)];
        stRangeData.fADRStdDev =
            static_cast<float>((RC_ADR_STDDEV_SCALE_OFFSET + ((stRangeCmpData.ucStdDevPSRStdDevADR & RC_ADR_STDDEV_MASK) >> RC_ADR_STDDEV_SHIFT)) /
                               RC_ADR_STDDEV_SCALE_FACTOR);
        stRangeData.fLockTime = static_cast<float>((stRangeCmpData.uiLockTimeCNoGLOFreq & RC_LOCK_TIME_MASK) / RC_LOCK_TIME_SCALE_FACTOR);
        stRangeData.fCNo = static_cast<float>(RC_CNO_SCALE_OFFSET + ((stRangeCmpData.uiLockTimeCNoGLOFreq & RC_CNO_MASK) >> RC_CNO_SHIFT));
        stRangeData.sGLONASSFrequency =
            static_cast<int16_t>((stRangeCmpData.uiLockTimeCNoGLOFreq & RC_GLONASS_FREQUENCY_MASK) >> RC_GLONASS_FREQUENCY_SHIFT);

        double dWavelength = GetSignalWavelength(stChannelTrackingStatus, stRangeData.sGLONASSFrequency);

        stRangeData.dADR = static_cast<double>(stRangeCmpData.uiADR) / RC_ADR_SCALE_FACTOR;
        double dADRRolls = ((stRangeData.dPSR / dWavelength) + stRangeData.dADR) / MAX_VALUE;

        // Round to the nearest integer.
        dADRRolls += (dADRRolls <= 0) ? -0.5 : 0.5;

        stRangeData.dADR = stRangeData.dADR - (MAX_VALUE * static_cast<uint64_t>(dADRRolls));
    }
}

//------------------------------------------------------------------------------
//! Convert a RANGECMP2 message into RANGE message.
//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmp2ToRange(const RangeCmp2Struct& stRangeCmp2Message_, RangeStruct& stRangeMessage_, const MetaDataStruct& stMetaData_)
{
    stRangeMessage_.uiNumberOfObservations = 0;
    uint32_t uiRangeDataBytesDecompressed = 0;
    while (uiRangeDataBytesDecompressed < stRangeCmp2Message_.uiNumberOfRangeDataBytes)
    {
        const auto& stRangeCmp2SatBlock =
            reinterpret_cast<const RangeCmp2SatelliteBlockStruct&>(stRangeCmp2Message_.aucRangeData[uiRangeDataBytesDecompressed]);

        // Decompress the Satellite Block
        const auto eSatelliteSystem =
            static_cast<SYSTEM>((stRangeCmp2SatBlock.ulCombinedField & RC2_SAT_SATELLITE_SYSTEM_ID_MASK) >> RC2_SAT_SATELLITE_SYSTEM_ID_SHIFT);
        const auto ucNumberOfSignalBlocks = static_cast<uint8_t>(
            static_cast<uint64_t>(stRangeCmp2SatBlock.ulCombinedField & RC2_SAT_NUM_SIGNAL_BLOCKS_BASE_MASK) >> RC2_SAT_NUM_SIGNAL_BLOCKS_BASE_SHIFT);

        // Extend the sign
        auto iPSRBase = static_cast<int32_t>(static_cast<uint64_t>(stRangeCmp2SatBlock.ulCombinedField & RC2_SAT_SATELLITE_PSR_BASE_MASK) >>
                                             RC2_SAT_SATELLITE_PSR_BASE_SHIFT);
        if (iPSRBase & RC2_SAT_SATELLITE_PSR_BASE_SIGNBIT_MASK) { iPSRBase |= RC2_SAT_SATELLITE_PSR_BASE_SIGNEXT_MASK; }

        // Extend the sign
        auto iDopplerBase = static_cast<int32_t>(static_cast<uint64_t>(stRangeCmp2SatBlock.ulCombinedField & RC2_SAT_SATELLITE_DOPPLER_BASE_MASK) >>
                                                 RC2_SAT_SATELLITE_DOPPLER_BASE_SHIFT);
        if (iDopplerBase & RC2_SAT_SATELLITE_DOPPLER_BASE_SIGNBIT_MASK) { iDopplerBase |= RC2_SAT_SATELLITE_DOPPLER_BASE_SIGNEXT_MASK; }

        uiRangeDataBytesDecompressed += sizeof(RangeCmp2SatelliteBlockStruct);

        // Decompress the Signal Blocks associated with the Satellite Block
        for (uint8_t ucSignalBlockIndex = 0; ucSignalBlockIndex < ucNumberOfSignalBlocks; ucSignalBlockIndex++)
        {
            const auto& stRangeCmp2SigBlock =
                reinterpret_cast<const RangeCmp2SignalBlockStruct&>(stRangeCmp2Message_.aucRangeData[uiRangeDataBytesDecompressed]);

            const auto eSignalType = static_cast<RangeCmp2::SIGNAL_TYPE>(stRangeCmp2SigBlock.uiCombinedField1 & RC2_SIG_SIGNAL_TYPE_MASK);
            const auto ucPSRBitfield =
                static_cast<uint8_t>((stRangeCmp2SigBlock.ulCombinedField2 & RC2_SIG_PSR_STDDEV_MASK) >> RC2_SIG_PSR_STDDEV_SHIFT);
            const auto ucADRBitfield =
                static_cast<uint8_t>((stRangeCmp2SigBlock.ulCombinedField2 & RC2_SIG_ADR_STDDEV_MASK) >> RC2_SIG_ADR_STDDEV_SHIFT);
            const auto uiLocktimeBits =
                static_cast<uint32_t>((stRangeCmp2SigBlock.uiCombinedField1 & RC2_SIG_LOCKTIME_MASK) >> RC2_SIG_LOCKTIME_SHIFT);
            const auto usPRN = static_cast<uint16_t>(stRangeCmp2SatBlock.ucSatelliteIdentifier +
                                                     (eSatelliteSystem == SYSTEM::GLONASS ? (GLONASS_SLOT_OFFSET - 1) : 0));

            // Extend the sign
            auto iDopplerBitfield = static_cast<int32_t>(static_cast<uint64_t>(stRangeCmp2SigBlock.ulCombinedField2 & RC2_SIG_DOPPLER_DIFF_MASK) >>
                                                         RC2_SIG_DOPPLER_DIFF_SHIFT);
            if (iDopplerBitfield & RC2_SIG_DOPPLER_DIFF_SIGNBIT_MASK) { iDopplerBitfield |= RC2_SIG_DOPPLER_DIFF_SIGNEXT_MASK; }

            const auto fPSRDiff = static_cast<float>((stRangeCmp2SigBlock.ulCombinedField2 & RC2_SIG_PSR_DIFF_MASK) >> RC2_SIG_PSR_DIFF_SHIFT);
            const auto fPhaseRangeDiff = static_cast<float>(
                static_cast<uint64_t>(stRangeCmp2SigBlock.ulCombinedField2 & RC2_SIG_PHASERANGE_DIFF_MASK) >> RC2_SIG_PHASERANGE_DIFF_SHIFT);
            const auto fScaledDopplerDiff = static_cast<float>(static_cast<float>(iDopplerBitfield) / RC2_SIG_DOPPLER_DIFF_SCALE_FACTOR);

            // Construct the ChannelTrackingStatus word
            ChannelTrackingStatusStruct stChannelTrackingStatus(stRangeCmp2SatBlock, stRangeCmp2SigBlock);

            // Construct the decompressed range data
            RangeDataStruct& stRangeData = stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++];
            stRangeData.usPRN = usPRN;
            stRangeData.sGLONASSFrequency = static_cast<int16_t>(stRangeCmp2SatBlock.ulCombinedField & RC2_SAT_GLONASS_FREQUENCY_ID_MASK);
            stRangeData.dPSR = static_cast<double>(iPSRBase + static_cast<double>(fPSRDiff / RC2_SIG_PSR_DIFF_SCALE_FACTOR));
            stRangeData.fPSRStdDev = afTheRangeCmp2PSRStdDevValues[ucPSRBitfield];
            stRangeData.dADR = MAGIC_NEGATE *
                               (static_cast<double>(iPSRBase) + static_cast<double>(fPhaseRangeDiff / RC2_SIG_PHASERANGE_DIFF_SCALE_FACTOR)) /
                               (GetSignalWavelength(stChannelTrackingStatus, (stRangeData.sGLONASSFrequency - GLONASS_FREQUENCY_NUMBER_OFFSET)));
            stRangeData.fADRStdDev = afTheRangeCmp2ADRStdDevValues[ucADRBitfield];
            stRangeData.fDopplerFrequency = static_cast<float>(iDopplerBase + fScaledDopplerDiff) /
                                            static_cast<float>(mmTheRangeCmp2SignalScalingMapping[eSatelliteSystem][eSignalType]);
            stRangeData.fCNo = RC2_SIG_CNO_SCALE_OFFSET + static_cast<float>(stRangeCmp2SigBlock.ulCombinedField2 & RC2_SIG_CNO_MASK);
            stRangeData.fLockTime = DetermineRangeCmp2ObservationLockTime(stMetaData_, uiLocktimeBits, stChannelTrackingStatus.eSatelliteSystem,
                                                                          stChannelTrackingStatus.eSignalType, usPRN);
            stRangeData.uiChannelTrackingStatus = stChannelTrackingStatus.GetAsWord();

            uiRangeDataBytesDecompressed += sizeof(RangeCmp2SignalBlockStruct);
        }
    }
}

//------------------------------------------------------------------------------
//! Decompress a buffer containing a RANGECMP4 message and translate it into
//! a RANGE message.
//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmp4ToRange(uint8_t* pucCompressedData_, RangeStruct& stRangeMessage_, const MetaDataStruct& stMetaData_)
{
    uint8_t* pucTempDataPointer = pucCompressedData_;

    MEASUREMENT_SOURCE eMeasurementSource = stMetaData_.eMeasurementSource;
    double dSecondOffset = static_cast<double>(static_cast<uint32_t>(stMetaData_.dMilliseconds) % SEC_TO_MILLI_SEC) / SEC_TO_MILLI_SEC;
    // Clear any dead reference blocks on the whole second.  We should be storing new ones.
    if (dSecondOffset == 0.0) { ammmMyReferenceBlocks[static_cast<uint32_t>(eMeasurementSource)].clear(); }

    auto eCurrentSatelliteSystem = SYSTEM::UNKNOWN;
    std::vector<RangeCmp4::SIGNAL_TYPE> vSignals;  // All available signals
    std::vector<uint32_t> vPRNs;                   // All available PRNs
    std::map<uint32_t, uint64_t> mIncludedSignals; // IncludedSignal bitmasks for each PRN.

    uint16_t usSatelliteSystems = 0;
    uint64_t ulSatellites = 0;
    uint16_t usSignals = 0;

    uint64_t ulIncludedSignals = 0;
    uint32_t uiSignalBitMaskShift = 0;
    uint32_t uiSignalsForPRN = 0;

    bool bPrimaryBlock = false;
    ChannelTrackingStatusStruct stChannelTrackingStatus;
    RangeCmp4MeasurementBlockHeaderStruct stMeasurementBlockHeader;
    RangeCmp4MeasurementSignalBlockStruct stMeasurementBlock;

    // Reset bit offset and 0 the observation count.
    uiMyBitOffset = 0;
    stRangeMessage_.uiNumberOfObservations = 0;

    // Pull out the first few fields.
    // We have to set uiMyBytesRemaining by typecasting because GetBitfieldFromBuffer() relies on
    // it.
    uiMyBytesRemaining = *reinterpret_cast<uint32_t*>(pucTempDataPointer);
    pucTempDataPointer += sizeof(uint32_t);
    usSatelliteSystems = static_cast<uint16_t>(GetBitfieldFromBuffer(&pucTempDataPointer, RC4_SATELLITE_SYSTEMS_BITS));

    // For each satellite system, we will decode a series of measurement block headers, each with
    // their own subsequent reference signal measurement blocks.
    for (uint8_t ucSystemIndex = 0; ucSystemIndex < RC4_HEADER_BLOCK_SYSTEM_COUNT; ucSystemIndex++)
    {
        eCurrentSatelliteSystem = aeTheRangeCmp4SatelliteSystems[ucSystemIndex];
        vSignals.clear();
        vPRNs.clear();

        // Does this message have any data for this satellite system?
        if (usSatelliteSystems & (1UL << static_cast<uint16_t>(eCurrentSatelliteSystem)))
        {
            ulSatellites = GetBitfieldFromBuffer(&pucTempDataPointer, RC4_SATELLITES_BITS);
            usSignals = static_cast<uint16_t>(GetBitfieldFromBuffer(&pucTempDataPointer, RC4_SIGNALS_BITS));

            // Collect the signals tracked in this satellite system.
            for (RangeCmp4::SIGNAL_TYPE eCurrentSignalType : mvTheRangeCmp4SystemSignalMasks[eCurrentSatelliteSystem])
            {
                if (usSignals & (1UL << static_cast<uint16_t>(eCurrentSignalType))) { vSignals.push_back(eCurrentSignalType); }
            }

            // Collect the satellite PRNs tracked in this satellite system.
            for (uint8_t ucBitPosition = 0; ucBitPosition < RC4_SATELLITES_BITS; ucBitPosition++)
            {
                // Note that ucBitPosition contains the PRN value at this point.
                if (ulSatellites & (1ULL << ucBitPosition))
                {
                    vPRNs.push_back(ucBitPosition + 1); // Bit position is PRN-1, so +1 here
                }
            }

            // Iterate through the PRNs once to collect the signals tracked by each.  We need this
            // info before we can start decompressing.
            mIncludedSignals.clear();
            for (const auto& uiPRN : vPRNs)
            {
                // Get the m*n bit matrix that describes the included signals in this RANGECMP4
                // message.
                mIncludedSignals[uiPRN] = GetBitfieldFromBuffer(&pucTempDataPointer, static_cast<uint32_t>(vSignals.size()));
            }

            // Check each PRN against the signals tracked in this satellite system to see if the
            // signal is included.
            for (const auto& uiPRN : vPRNs)
            {
                // Begin decoding Reference Measurement Block Header.
                stMeasurementBlockHeader.bIsDifferentialData =
                    static_cast<bool>(GetBitfieldFromBuffer(&pucTempDataPointer, RC4_MBLK_HDR_DATAFORMAT_FLAG_BITS));
                stMeasurementBlockHeader.ucReferenceDataBlockID =
                    static_cast<uint8_t>(GetBitfieldFromBuffer(&pucTempDataPointer, RC4_MBLK_HDR_REFERENCE_DATABLOCK_ID_BITS));
                stMeasurementBlockHeader.cGLONASSFrequencyNumber = 0;

                // This field is only present for GLONASS and reference blocks.
                if (eCurrentSatelliteSystem == SYSTEM::GLONASS && !stMeasurementBlockHeader.bIsDifferentialData)
                {
                    stMeasurementBlockHeader.cGLONASSFrequencyNumber =
                        static_cast<uint8_t>(GetBitfieldFromBuffer(&pucTempDataPointer, RC4_MBLK_HDR_GLONASS_FREQUENCY_NUMBER_BITS));
                }

                uiSignalsForPRN = 0;
                uiSignalBitMaskShift = 0;
                bPrimaryBlock = true; // Reset to true for the first reference block from each PRN.
                for (RangeCmp4::SIGNAL_TYPE eCurrentSignalType : vSignals)
                {
                    // Get the included signals bitmask for this PRN.
                    try
                    {
                        ulIncludedSignals = mIncludedSignals.at(uiPRN);
                    }
                    catch (...)
                    {
                        pclMyLogger->critical("No included signal bitmask for the PRN {}.", uiPRN);
                        return;
                    }

                    // Is the signal included?
                    if (ulIncludedSignals & (1ULL << uiSignalBitMaskShift++))
                    {
                        if (!stMeasurementBlockHeader.bIsDifferentialData) // This is a reference block.
                        {
                            if (bPrimaryBlock)
                            {
                                DecompressReferenceBlock<false>(&pucTempDataPointer, stMeasurementBlock, eMeasurementSource);
                                bPrimaryBlock = false;
                            }
                            else { DecompressReferenceBlock<true>(&pucTempDataPointer, stMeasurementBlock, eMeasurementSource); }

                            stChannelTrackingStatus = ChannelTrackingStatusStruct(eCurrentSatelliteSystem, eCurrentSignalType, stMeasurementBlock);
                            PopulateNextRangeData((stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++]), stMeasurementBlock,
                                                  stMetaData_, stChannelTrackingStatus, uiPRN, stMeasurementBlockHeader.cGLONASSFrequencyNumber);

                            // Always store reference blocks.
                            ammmMyReferenceBlocks[static_cast<uint32_t>(eMeasurementSource)][eCurrentSatelliteSystem][eCurrentSignalType][uiPRN] =
                                std::pair(stMeasurementBlockHeader, stMeasurementBlock);
                        }
                        else // This is a differential block.
                        {
                            RangeCmp4MeasurementBlockHeaderStruct* pstReferenceBlockHeader = nullptr;
                            RangeCmp4MeasurementSignalBlockStruct* pstReferenceBlock = nullptr;
                            try
                            {
                                pstReferenceBlockHeader = &ammmMyReferenceBlocks[static_cast<uint32_t>(eMeasurementSource)]
                                                               .at(eCurrentSatelliteSystem)
                                                               .at(eCurrentSignalType)
                                                               .at(uiPRN)
                                                               .first;
                                pstReferenceBlock = &ammmMyReferenceBlocks[static_cast<uint32_t>(eMeasurementSource)]
                                                         .at(eCurrentSatelliteSystem)
                                                         .at(eCurrentSignalType)
                                                         .at(uiPRN)
                                                         .second;
                            }
                            catch (...)
                            {
                                pclMyLogger->warn("No reference data exists for SATELLITE_SYSTEM {}, SIGNAL_TYPE "
                                                  "{}, PRN {}, ID {}",
                                                  static_cast<int32_t>(eCurrentSatelliteSystem), static_cast<int32_t>(eCurrentSignalType), uiPRN,
                                                  stMeasurementBlockHeader.ucReferenceDataBlockID);
                            }

                            // Do nothing if we can't find reference data.
                            if (pstReferenceBlockHeader != nullptr && pstReferenceBlock != nullptr)
                            {
                                if (stMeasurementBlockHeader.ucReferenceDataBlockID == pstReferenceBlockHeader->ucReferenceDataBlockID)
                                {
                                    if (bPrimaryBlock)
                                    {
                                        DecompressDifferentialBlock<false>(&pucTempDataPointer, stMeasurementBlock, *pstReferenceBlock,
                                                                           dSecondOffset);
                                        bPrimaryBlock = false;
                                    }
                                    else
                                    {
                                        DecompressDifferentialBlock<true>(&pucTempDataPointer, stMeasurementBlock, *pstReferenceBlock, dSecondOffset);
                                    }

                                    stChannelTrackingStatus =
                                        ChannelTrackingStatusStruct(eCurrentSatelliteSystem, eCurrentSignalType, stMeasurementBlock);
                                    PopulateNextRangeData(stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++], stMeasurementBlock,
                                                          stMetaData_, stChannelTrackingStatus, uiPRN,
                                                          pstReferenceBlockHeader->cGLONASSFrequencyNumber);
                                }
                                else
                                {
                                    pclMyLogger->warn("Invalid reference data: Diff ID {} != Ref ID {}",
                                                      static_cast<uint32_t>(stMeasurementBlockHeader.ucReferenceDataBlockID),
                                                      static_cast<uint32_t>(pstReferenceBlockHeader->ucReferenceDataBlockID));
                                }
                            }
                        }

                        // Keep track of how many signals came from this PRN so we can go back and
                        // alter the message fields we cannot know until some time in the future.
                        uiSignalsForPRN++;
                    }
                }

                // Go back and update the grouping bit in the status word if we've counted more than
                // one signal for this PRN.  We can't know this ahead of time without blindly
                // jumping forward into the compressed message.
                if (uiSignalsForPRN > 1 && uiSignalsForPRN <= stRangeMessage_.uiNumberOfObservations)
                {
                    for (uint32_t uiIndex = uiSignalsForPRN; uiIndex > 0; uiIndex--)
                    {
                        stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations - uiIndex].uiChannelTrackingStatus |= CTS_GROUPING_MASK;
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
//! This method will decompress the range message provided in pucRangeMessageBuffer_
//! and overwrite the contents with the decompressed message.
//------------------------------------------------------------------------------
STATUS
RangeDecompressor::Decompress(unsigned char* pucRangeMessageBuffer_, uint32_t uiRangeMessageBufferSize_, MetaDataStruct& stMetaData_,
                              ENCODE_FORMAT eFormat_)
{
    // Check for buffer validity
    if (!pucRangeMessageBuffer_) { return STATUS::NULL_PROVIDED; }

    if (!pclMyMsgDB) { return STATUS::NO_DATABASE; }

    MessageDataStruct stMessageData;
    IntermediateHeader stHeader;
    std::vector<FieldContainer> stMessage;
    auto eStatus = STATUS::UNKNOWN;

    unsigned char* pucTempMessagePointer = pucRangeMessageBuffer_;
    eStatus = clMyHeaderDecoder.Decode(pucTempMessagePointer, stHeader, stMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    if (!clMyRangeCmpFilter.DoFiltering(stMetaData_)) { return STATUS::UNSUPPORTED; }

    HEADER_FORMAT eInitialFormat = stMetaData_.eFormat;
    pucTempMessagePointer += stMetaData_.uiHeaderLength;
    // If the message is already in binary format, we don't need to do anything.
    // If the message is not in binary format, we need to ensure that it is encoded to binary so
    // that it can be decompressed.
    if (eInitialFormat != HEADER_FORMAT::BINARY)
    {
        eStatus = clMyMessageDecoder.Decode(pucTempMessagePointer, stMessage, stMetaData_);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }

        eStatus = clMyEncoder.Encode(&pucRangeMessageBuffer_, uiRangeMessageBufferSize_, stHeader, stMessage, stMessageData, stMetaData_,
                                     ENCODE_FORMAT::FLATTENED_BINARY);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }

        pucTempMessagePointer = stMessageData.pucMessageBody;
    }

    // Convert the RANGECMPx message to a RANGE message
    try
    {
        RangeStruct stRange;
        switch (stMetaData_.usMessageId)
        {
        case RANGECMP_MSG_ID: RangeCmpToRange(*reinterpret_cast<RangeCmpStruct*>(pucTempMessagePointer), stRange); break;
        case RANGECMP2_MSG_ID: RangeCmp2ToRange(*reinterpret_cast<RangeCmp2Struct*>(pucTempMessagePointer), stRange, stMetaData_); break;
        case RANGECMP3_MSG_ID: [[fallthrough]];
        case RANGECMP4_MSG_ID: RangeCmp4ToRange(pucTempMessagePointer, stRange, stMetaData_); break;
        default: return STATUS::UNSUPPORTED;
        }

        // Set the binary message length in the metadata for decoding purposes.
        stMetaData_.uiBinaryMsgLength = sizeof(stRange.uiNumberOfObservations) + (stRange.uiNumberOfObservations * sizeof(RangeDataStruct));
        memcpy(pucTempMessagePointer, &stRange, stMetaData_.uiBinaryMsgLength);
    }
    catch (...)
    {
        return STATUS::DECOMPRESSION_FAILURE;
    }

    // Adjust metadata/header data
    stHeader.usMessageId = RANGE_MSG_ID;
    stMetaData_.usMessageId = RANGE_MSG_ID;
    stMetaData_.uiMessageCrc = 0; // Use the first message definition
    memcpy(stMetaData_.acMessageName, "RANGE", 6);

    // The message should be returned in its original format
    stMetaData_.eFormat = HEADER_FORMAT::BINARY;
    stMessage.clear();
    eStatus = clMyMessageDecoder.Decode(pucTempMessagePointer, stMessage, stMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    stMetaData_.eFormat = eInitialFormat;
    // Re-encode to the original format if a format was not specified.
    if (eFormat_ == ENCODE_FORMAT::UNSPECIFIED)
    {
        eFormat_ =
            (eInitialFormat == HEADER_FORMAT::BINARY || eInitialFormat == HEADER_FORMAT::SHORT_BINARY ||
             eInitialFormat == HEADER_FORMAT::PROPRIETARY_BINARY)
                ? ENCODE_FORMAT::BINARY
            : (eInitialFormat == HEADER_FORMAT::ASCII || eInitialFormat == HEADER_FORMAT::SHORT_ASCII || eInitialFormat == HEADER_FORMAT::ABB_ASCII)
                ? ENCODE_FORMAT::ASCII
            : (eInitialFormat == HEADER_FORMAT::JSON) ? ENCODE_FORMAT::JSON
                                                      : ENCODE_FORMAT::ASCII; // Default to ASCII
    }

    // Re-encode the data back into the range message buffer.
    eStatus = clMyEncoder.Encode(&pucRangeMessageBuffer_, uiRangeMessageBufferSize_, stHeader, stMessage, stMessageData, stMetaData_, eFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Final adjustments to MetaData
    stMetaData_.uiLength = stMessageData.uiMessageLength;
    stMetaData_.uiHeaderLength = stMessageData.uiMessageHeaderLength;

    return STATUS::SUCCESS;
}
