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

#include "novatel_edie/decoders/oem/rangecmp/range_decompressor.hpp"

#include <stdexcept>

using namespace novatel::edie;
using namespace novatel::edie::oem;

//------------------------------------------------------------------------------
RangeDecompressor::RangeDecompressor(JsonReader* pclJsonDB_) : clMyHeaderDecoder(pclJsonDB_), clMyMessageDecoder(pclJsonDB_), clMyEncoder(pclJsonDB_)
{
    pclMyLogger = Logger::RegisterLogger("range_decompressor");
    pclMyLogger->debug("RangeDecompressor initializing...");

    if (pclJsonDB_ != nullptr) { LoadJsonDb(pclJsonDB_); }

    for (const auto id : {RANGECMP_MSG_ID, RANGECMP2_MSG_ID, RANGECMP3_MSG_ID, RANGECMP4_MSG_ID, RANGECMP5_MSG_ID})
    {
        clMyRangeCmpFilter.IncludeMessageId(id, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
        clMyRangeCmpFilter.IncludeMessageId(id, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
    }

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

//-----------------------------------------------------------------------
//! Lookup function for L1/E1/B1 Scaling for RANGECMP2 signals
//! defined in the RANGECMP2 documentation:
//! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP2.htm?Highlight=RANGECMP2#L1_E1_B1_Scaling
//-----------------------------------------------------------------------
double RangeDecompressor::RangeCmp2SignalScaling(SYSTEM system, rangecmp2::SIGNAL_TYPE signal)
{
    using namespace rangecmp2;

    switch (system)
    {
    case SYSTEM::GPS:
        switch (signal)
        {
        case SIGNAL_TYPE::GPS_L1C: return 1.0;
        case SIGNAL_TYPE::GPS_L1CA: return 1.0;
        case SIGNAL_TYPE::GPS_L2Y: return 154.0 / 120.0;
        case SIGNAL_TYPE::GPS_L2CM: return 154.0 / 120.0;
        case SIGNAL_TYPE::GPS_L5Q: return 154.0 / 115.0;
        default: return 0.0;
        }
    case SYSTEM::GLONASS:
        switch (signal)
        {
        case SIGNAL_TYPE::GLONASS_L1CA: return 1.0;
        case SIGNAL_TYPE::GLONASS_L2CA: return 9.0 / 7.0;
        case SIGNAL_TYPE::GLONASS_L2P: return 9.0 / 7.0;
        case SIGNAL_TYPE::GLONASS_L3Q: return 313.0 / 235.0;
        default: return 0.0;
        }
    case SYSTEM::SBAS:
        switch (signal)
        {
        case SIGNAL_TYPE::SBAS_L1CA: return 1.0;
        case SIGNAL_TYPE::SBAS_L5I: return 154.0 / 115.0;
        default: return 0.0;
        }
    case SYSTEM::GALILEO:
        switch (signal)
        {
        case SIGNAL_TYPE::GALILEO_E1C: return 1.0;
        case SIGNAL_TYPE::GALILEO_E5AQ: return 154.0 / 115.0;
        case SIGNAL_TYPE::GALILEO_E5BQ: return 154.0 / 118.0;
        case SIGNAL_TYPE::GALILEO_ALTBOCQ: return 154.0 / 116.5;
        case SIGNAL_TYPE::GALILEO_E6C: return 154.0 / 125.0;
        case SIGNAL_TYPE::GALILEO_E6B: return 154.0 / 125.0;
        default: return 0.0;
        }
    case SYSTEM::BEIDOU:
        switch (signal)
        {
        case SIGNAL_TYPE::BEIDOU_B1D1I: return 1.0;
        case SIGNAL_TYPE::BEIDOU_B1D2I: return 1.0;
        case SIGNAL_TYPE::BEIDOU_B1CP: return 1526.0 / 1540.0;
        case SIGNAL_TYPE::BEIDOU_B2D1I: return 1526.0 / 1180.0;
        case SIGNAL_TYPE::BEIDOU_B2D2I: return 1526.0 / 1180.0;
        case SIGNAL_TYPE::BEIDOU_B2AP: return 1526.0 / 1150.0;
        case SIGNAL_TYPE::BEIDOU_B2B_I: return 1526.0 / 1180.0;
        case SIGNAL_TYPE::BEIDOU_B3D1I: return 1526.0 / 1240.0;
        case SIGNAL_TYPE::BEIDOU_B3D2I: return 1526.0 / 1240.0;
        default: return 0.0;
        }
    case SYSTEM::QZSS:
        switch (signal)
        {
        case SIGNAL_TYPE::QZSS_L1C: return 1.0;
        case SIGNAL_TYPE::QZSS_L1CA: return 1.0;
        case SIGNAL_TYPE::QZSS_L2CM: return 154.0 / 120.0;
        case SIGNAL_TYPE::QZSS_L5Q: return 154.0 / 115.0;
        case SIGNAL_TYPE::QZSS_L6P: return 154.0 / 125.0;
        default: return 0.0;
        }
    case SYSTEM::LBAND:
        switch (signal)
        {
        case SIGNAL_TYPE::LBAND: return 1.0;
        default: return 0.0;
        }
    case SYSTEM::NAVIC:
        switch (signal)
        {
        case SIGNAL_TYPE::NAVIC_L5SPS: return 1.0;
        default: return 0.0;
        }
    default: return 0.0;
    }
}

//------------------------------------------------------------------------------
//! This function acts as a lookup for a signal wavelength based on the provided
//! ChannelTrackingStatus. Uses the Satellite System and Signal fields, and in
//! the case of GLONASS, it will use the provided GLONASS frequency.
//------------------------------------------------------------------------------
double RangeDecompressor::GetSignalWavelength(const ChannelTrackingStatus& stChannelStatus_, int16_t sGLONASSFrequency_)
{
    using Signal = ChannelTrackingStatus::SIGNAL_TYPE;
    using System = ChannelTrackingStatus::SATELLITE_SYSTEM;

    switch (stChannelStatus_.eSatelliteSystem)
    {
    case System::GPS:
        switch (stChannelStatus_.eSignalType)
        {
        case Signal::GPS_L1CA: return WAVELENGTH_GPS_L1;
        case Signal::GPS_L1CP: return WAVELENGTH_GPS_L1;
        case Signal::GPS_L2P: return WAVELENGTH_GPS_L2;
        case Signal::GPS_L2Y: return WAVELENGTH_GPS_L2;
        case Signal::GPS_L2CM: return WAVELENGTH_GPS_L2;
        case Signal::GPS_L5Q: return WAVELENGTH_GPS_L5;
        default: return 0.0;
        }
    case System::GLONASS:
        switch (stChannelStatus_.eSignalType)
        {
        case Signal::GLONASS_L1CA: return SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L1 + sGLONASSFrequency_ * GLONASS_L1_FREQUENCY_SCALE_HZ);
        case Signal::GLONASS_L2CA: [[fallthrough]];
        case Signal::GLONASS_L2P: return SPEED_OF_LIGHT / (FREQUENCY_HZ_GLO_L2 + sGLONASSFrequency_ * GLONASS_L2_FREQUENCY_SCALE_HZ);
        case Signal::GLONASS_L3Q: return WAVELENGTH_GLO_L3;
        default: return 0.0;
        }
    case System::SBAS:
        switch (stChannelStatus_.eSignalType)
        {
        case Signal::SBAS_L1CA: return WAVELENGTH_GPS_L1;
        case Signal::SBAS_L5I: return WAVELENGTH_GPS_L5;
        default: return 0.0;
        }
    case System::GALILEO:
        switch (stChannelStatus_.eSignalType)
        {
        case Signal::GALILEO_E1C: return WAVELENGTH_GAL_E1;
        case Signal::GALILEO_E6B: return WAVELENGTH_GAL_E6;
        case Signal::GALILEO_E6C: return WAVELENGTH_GAL_E6;
        case Signal::GALILEO_E5AQ: return WAVELENGTH_GAL_E5AQ;
        case Signal::GALILEO_E5BQ: return WAVELENGTH_GAL_E5BQ;
        case Signal::GALILEO_E5ALTBOCQ: return WAVELENGTH_GAL_ALTBQ;
        default: return 0.0;
        }
    case System::BEIDOU:
        switch (stChannelStatus_.eSignalType)
        {
        case Signal::BEIDOU_B1ID1: return WAVELENGTH_BDS_B1;
        case Signal::BEIDOU_B1ID2: return WAVELENGTH_BDS_B1;
        case Signal::BEIDOU_B2ID1: return WAVELENGTH_BDS_B2;
        case Signal::BEIDOU_B2ID2: return WAVELENGTH_BDS_B2;
        case Signal::BEIDOU_B3ID1: return WAVELENGTH_BDS_B3;
        case Signal::BEIDOU_B3ID2: return WAVELENGTH_BDS_B3;
        case Signal::BEIDOU_B1CP: return WAVELENGTH_BDS_B1C;
        case Signal::BEIDOU_B2AP: return WAVELENGTH_BDS_B2A;
        case Signal::BEIDOU_B2BI: return WAVELENGTH_BDS_B2B;
        default: return 0.0;
        }
    case System::QZSS:
        switch (stChannelStatus_.eSignalType)
        {
        case Signal::QZSS_L1CA: return WAVELENGTH_QZSS_L1;
        case Signal::QZSS_L1CP: return WAVELENGTH_QZSS_L1;
        case Signal::QZSS_L2CM: return WAVELENGTH_QZSS_L2;
        case Signal::QZSS_L5Q: return WAVELENGTH_QZSS_L5;
        case Signal::QZSS_L6P: return WAVELENGTH_QZSS_L6;
        case Signal::QZSS_L6D: return WAVELENGTH_QZSS_L6;
        default: return 0.0;
        }
    case System::NAVIC:
        switch (stChannelStatus_.eSignalType)
        {
        case Signal::NAVIC_L5SPS: return WAVELENGTH_NAVIC_L5;
        default: return 0.0;
        }
    default: return 0.0;
    }
}

//------------------------------------------------------------------------------
//! The locktime can only report up to 131071ms (0x1FFFF). If this value is
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
double RangeDecompressor::GetRangeCmp2LockTime(const MetaDataStruct& stMetaData_, uint32_t uiLockTimeBits_,
                                               ChannelTrackingStatus::SATELLITE_SYSTEM eSystem_, ChannelTrackingStatus::SIGNAL_TYPE eSignal_,
                                               uint16_t usPRN_)
{
    using namespace rangecmp2;

    double fLockTimeMilliseconds = uiLockTimeBits_;

    rangecmp2::LockTimeInfo& stLockTimeInfo =
        mMyRangeCmp2LockTimes[ChannelTrackingStatus::MakeKey(eSystem_, usPRN_, eSignal_, stMetaData_.eMeasurementSource)];
    if (uiLockTimeBits_ == SIG_LOCKTIME_MASK >> Lsb(SIG_LOCKTIME_MASK))
    {
        // If the locktime was already saturated, use the stored time to add the missing offset.
        if (stLockTimeInfo.bLockTimeSaturated) { fLockTimeMilliseconds += stMetaData_.dMilliseconds - stLockTimeInfo.dLockTimeSaturatedMilliseconds; }
        // If the locktime is not already saturated, store this information if this observation is seen again.
        else
        {
            stLockTimeInfo.dLockTimeSaturatedMilliseconds = stMetaData_.dMilliseconds;
            stLockTimeInfo.bLockTimeSaturated = true;
        }
    }
    // If the locktime marked as saturated, but is not reported as such from the RANGECMP2 message, clear the flag.
    else if (stLockTimeInfo.bLockTimeSaturated) { stLockTimeInfo.bLockTimeSaturated = false; }

    return fLockTimeMilliseconds / SEC_TO_MILLI_SEC;
}

//------------------------------------------------------------------------------
//! RANGECMP4 locktime information is categorized into certain ranges for a
//! given observation. Decompressing this information without knowledge of the
//! precise time at which lock was acquired can result in initially misledaing
//! locktimes. There are a number of cases which may occur when decompressing
//! the locktime bitfields from a RANGECMP4 observation:
//! NOTE: See lockTime for ullBitfield:locktime translations.
//!   1. The locktime ullBitfield is b0000:
//!      - This is the simplest case. The locktime will increase with the time
//!        reported by the message header.
//!   2. The locktime ullBitfield is b1111:
//!      - There is no way to determine when the locktime reached that range,
//!        so as for case 1, the locktime will increase with the time reported
//!        by the message header.
//!   3. The locktime ullBitfield is any value from b0001 to b1110:
//!      - This case is the most complex. Because there is no guarantee that
//!        the current locktime reported has just transitioned to the lower
//!        boundary of the ullBitfield representation, it must be stated that the
//!        locktime is relative, and not absolute. Only when a change in the
//!        ullBitfield is detected can it be guaranteed the the locktime is
//!        absolute. At this point, the locktime can jump or slip to adjust
//!        to the newly found ullBitfield. After the jump or slip occurs, the
//!        locktime must not change again, as the lower ullBitfield values will
//!        produce the highest degree of accuracy.
//!      - For example, if the locktime ullBitfield is b0111, the observation has
//!        been locked for at least 1024ms. However it is possible the message
//!        was produced when the observation was locked for 1800ms. This means
//!        a 776ms discrepancy exists between what the RangeDecompressor tracks
//!        and what is reported in the RANGECMP messages. When this discrepancy
//!        is detected, the locktime can jump to match what the RANGECMP message
//!        reports. Thus, the distinction between "relative" and "absolute"
//!        must be made to allow the decompressor to accurately reflect
//!        observation locktimes. See the example below:
//!
//! Lock Time (t)
//!   ^
//!   |                                                 . < Continues 1:1
//!   |                                               .
//!   |                                             .
//!   |                                           /
//!   |                                         /
//!   |                                       /
//!   |                                     /
//!   |                                   /   < At this point a ullBitfield change
//!   |                                 *|      was found. The transition from
//!   |                               *  |      "relative" to "absolute" time
//!   |                             *    |      results in a locktime jump as
//!   |                           *      |      seen here.
//!   |                         *       /
//!   |                       *       /
//!   |                     *       /
//!   |                   *       /
//!   | Absolute        *       /   < Relative locktime inferred by the
//!   | locktime >    *       /       RangeDecompressor. These values
//!   |             *       /         will be output to RANGE message
//!   |           *       /           locktime fields.
//!   +-------------------------------------------------------------------->
//!                   Header time (t)
//------------------------------------------------------------------------------
double RangeDecompressor::GetRangeCmp4LockTime(const MetaDataStruct& stMetaData_, uint8_t ucLockTimeBits_,
                                               ChannelTrackingStatus::SATELLITE_SYSTEM eSystem_, ChannelTrackingStatus::SIGNAL_TYPE eSignal_,
                                               uint32_t uiPRN_)
{
    //-----------------------------------------------------------------------
    //! List of pre-defined doubles used as translations for RANGECMP4 lock
    //! time values defined in the RANGECMP4 documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#Lock
    //! NOTE: These values are the lower bound of the range representations.
    //-----------------------------------------------------------------------
    constexpr std::array<double, 16> lockTime = {0.0,    16.0,   32.0,   64.0,    128.0,   256.0,   512.0,    1024.0,
                                                 2048.0, 4096.0, 8192.0, 16384.0, 32768.0, 65536.0, 131072.0, 262144.0};

    using namespace rangecmp4;

    // Store the locktime if it is different than the once we have currently.
    LockTimeInfo& stLockTimeInfo = mMyRangeCmp4LockTimes[ChannelTrackingStatus::MakeKey(eSystem_, uiPRN_, eSignal_, stMetaData_.eMeasurementSource)];

    // Is the locktime relative and has a ullBitfield change been found?
    if (!stLockTimeInfo.bAbsolute && ucLockTimeBits_ != stLockTimeInfo.ucBits)
    {
        // Set locktime as absolute if bits are 0 or transitioning from relative to absolute.
        if (ucLockTimeBits_ == 0 || (stLockTimeInfo.ucBits != 0xFF && ucLockTimeBits_ > stLockTimeInfo.ucBits))
        {
            stLockTimeInfo.bAbsolute = true;

            double dLockTimeDeltaMs = std::abs(lockTime[ucLockTimeBits_] - stLockTimeInfo.dMilliseconds);
            if (dLockTimeDeltaMs > std::numeric_limits<double>::epsilon())
            {
                pclMyLogger->warn("Detected a locktime jump of {}ms at time {}w, {}ms. SYSTEM: {}, SIGNAL: {}, PRN: {}.", dLockTimeDeltaMs,
                                  stMetaData_.usWeek, stMetaData_.dMilliseconds, static_cast<int32_t>(eSystem_), static_cast<int32_t>(eSignal_),
                                  uiPRN_);
            }
        }

        // Record the last bit change and the ullBitfield that was changed to.
        stLockTimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
        stLockTimeInfo.ucBits = ucLockTimeBits_;
    }
    // If the locktime is absolute and the locktime bits have decreased, there was likely an outage.
    else if (ucLockTimeBits_ < stLockTimeInfo.ucBits)
    {
        // In the event of an outage of any kind, reset the locktime to relative.
        stLockTimeInfo.bAbsolute = false;
        stLockTimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
        stLockTimeInfo.ucBits = ucLockTimeBits_;

        pclMyLogger->warn("Detected a locktime slip (perhaps caused by an outage) of {}ms at time {}w, {}ms. SYSTEM: {}, SIGNAL: {}, PRN: {}.",
                          stLockTimeInfo.dMilliseconds - lockTime[stLockTimeInfo.ucBits], stMetaData_.usWeek, stMetaData_.dMilliseconds,
                          static_cast<int32_t>(eSystem_), static_cast<int32_t>(eSignal_), uiPRN_);
    }
    else
    {
        // If the locktime is absolute and the ullBitfield hasn't changed within the expected time, reset the last change time
        if (stMetaData_.dMilliseconds - stLockTimeInfo.dLastBitfieldChangeMilliseconds > 2 * lockTime[ucLockTimeBits_])
        {
            stLockTimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
            pclMyLogger->warn("Expected a bit change much sooner at time {}w, {}ms. SYSTEM: {}, SIGNAL: {}, PRN: {}.", stMetaData_.usWeek,
                              stMetaData_.dMilliseconds, static_cast<int32_t>(eSystem_), static_cast<int32_t>(eSignal_), uiPRN_);
        }
    }
    stLockTimeInfo.dMilliseconds = stMetaData_.dMilliseconds - stLockTimeInfo.dLastBitfieldChangeMilliseconds + lockTime[stLockTimeInfo.ucBits];
    return stLockTimeInfo.dMilliseconds / SEC_TO_MILLI_SEC;
}

//------------------------------------------------------------------------------
//! Decompresses a RANGECMP4 reference measurement block. Populates the
//! provided reference block struct.
//------------------------------------------------------------------------------
template <bool bSecondary>
void RangeDecompressor::DecompressReferenceBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                                 rangecmp4::MeasurementSignalBlock& stRefBlock_, double primaryPseudorange, double primaryDoppler)
{
    using namespace rangecmp4;

    stRefBlock_.bParityKnown = ExtractBitfield<bool>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_PARITY_FLAG_BITS);
    stRefBlock_.bHalfCycleAdded = ExtractBitfield<bool>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_HALF_CYCLE_BITS);
    stRefBlock_.fCNo = ExtractBitfield<float>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_CNO_BITS) * SIG_BLK_CNO_SCALE_FACTOR;
    stRefBlock_.ucLockTimeBitfield = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_LOCK_TIME_BITS);
    stRefBlock_.ucPSRBitfield = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_PSR_STDDEV_BITS);
    stRefBlock_.ucADRBitfield = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_ADR_STDDEV_BITS);

    auto llPSRBitfield = ExtractBitfield<int64_t>(ppucData_, uiBytesLeft_, uiBitOffset_, RBLK_PSR_BITS[bSecondary]);
    if constexpr (bSecondary) { HandleSignExtension<SSIG_RBLK_PSR_SIGNEXT_MASK>(llPSRBitfield); }
    auto iPhaseRangeBitfield = ExtractBitfield<int32_t>(ppucData_, uiBytesLeft_, uiBitOffset_, RBLK_PHASERANGE_BITS);
    HandleSignExtension<RBLK_PHASERANGE_SIGNEXT_MASK>(iPhaseRangeBitfield);
    auto iDopplerBitfield = ExtractBitfield<int32_t>(ppucData_, uiBytesLeft_, uiBitOffset_, RBLK_DOPPLER_BITS[bSecondary]);
    HandleSignExtension<RBLK_DOPPLER_SIGNEXT_MASK[bSecondary]>(iDopplerBitfield);

    stRefBlock_.bValidPSR = llPSRBitfield != RBLK_INVALID_PSR[bSecondary];
    stRefBlock_.dPSR = llPSRBitfield * SIG_BLK_PSR_SCALE_FACTOR + (bSecondary ? primaryPseudorange : 0);
    stRefBlock_.bValidPhaseRange = iPhaseRangeBitfield != SIG_RBLK_INVALID_PHASERANGE;
    stRefBlock_.dPhaseRange = iPhaseRangeBitfield * SIG_BLK_PHASERANGE_SCALE_FACTOR + stRefBlock_.dPSR;
    stRefBlock_.bValidDoppler = iDopplerBitfield != PSIG_RBLK_INVALID_DOPPLER;
    stRefBlock_.dDoppler = iDopplerBitfield * SIG_BLK_DOPPLER_SCALE_FACTOR + (bSecondary ? primaryDoppler : 0);
}

//------------------------------------------------------------------------------
//! Decompresses a RANGECMP4 differential measurement block. Populates the
//! provided reference block struct, but must be given the appropriate
//! reference block from the same RANGECMP4 message.
//------------------------------------------------------------------------------
template <bool bSecondary>
void RangeDecompressor::DecompressDifferentialBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                                    rangecmp4::MeasurementSignalBlock& stDiffBlock_,
                                                    const rangecmp4::MeasurementSignalBlock& stRefBlock_, double dSecondOffset_)
{
    using namespace rangecmp4;

    stDiffBlock_.bParityKnown = ExtractBitfield<bool>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_PARITY_FLAG_BITS);
    stDiffBlock_.bHalfCycleAdded = ExtractBitfield<bool>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_HALF_CYCLE_BITS);
    stDiffBlock_.fCNo = ExtractBitfield<float>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_CNO_BITS) * SIG_BLK_CNO_SCALE_FACTOR;
    stDiffBlock_.ucLockTimeBitfield = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_LOCK_TIME_BITS);
    stDiffBlock_.ucPSRBitfield = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_PSR_STDDEV_BITS);
    stDiffBlock_.ucADRBitfield = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_ADR_STDDEV_BITS);

    auto iPSR = ExtractBitfield<int32_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_DBLK_PSR_BITS);
    HandleSignExtension<SIG_DBLK_PSR_SIGNEXT_MASK>(iPSR);
    auto iPhaseRange = ExtractBitfield<int32_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_DBLK_PHASERANGE_BITS);
    HandleSignExtension<SIG_DBLK_PHASERANGE_SIGNEXT_MASK>(iPhaseRange);
    auto iDoppler = ExtractBitfield<int32_t>(ppucData_, uiBytesLeft_, uiBitOffset_, DBLK_DOPPLER_BITS[bSecondary]);
    HandleSignExtension<DBLK_DOPPLER_SIGNEXT_MASK[bSecondary]>(iDoppler);

    stDiffBlock_.bValidPSR = iPSR != SIG_DBLK_INVALID_PSR;
    stDiffBlock_.dPSR = iPSR * SIG_BLK_PSR_SCALE_FACTOR + stRefBlock_.dPSR + (stRefBlock_.dDoppler * dSecondOffset_);
    stDiffBlock_.bValidPhaseRange = iPhaseRange != SIG_DBLK_INVALID_PHASERANGE;
    stDiffBlock_.dPhaseRange = iPhaseRange * SIG_BLK_PHASERANGE_SCALE_FACTOR + stRefBlock_.dPhaseRange + stRefBlock_.dDoppler * dSecondOffset_;
    stDiffBlock_.bValidDoppler = iDoppler != DBLK_INVALID_DOPPLER[bSecondary];
    stDiffBlock_.dDoppler = iDoppler * SIG_BLK_DOPPLER_SCALE_FACTOR + stRefBlock_.dDoppler;
}

//------------------------------------------------------------------------------
//! Populates a provided RangeData structure from the RANGECMP4 blocks provided.
//------------------------------------------------------------------------------
void RangeDecompressor::PopulateNextRangeData(RangeData& stRangeData_, const rangecmp4::MeasurementSignalBlock& stBlock_,
                                              const MetaDataStruct& stMetaData_, const ChannelTrackingStatus& stChannelStatus_, uint32_t uiPRN_,
                                              char cGLONASSFrequencyNumber_)
{
    //-----------------------------------------------------------------------
    //! List of pre-defined doubles used as translations for RANGECMP4 PSR
    //! standard deviation values defined in the RANGECMP4 documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#Pseudora
    //-----------------------------------------------------------------------
    constexpr std::array<double, 16> stdDevPsrScaling = {0.020, 0.030, 0.045, 0.066, 0.099, 0.148, 0.220, 0.329,
                                                         0.491, 0.732, 1.092, 1.629, 2.430, 3.625, 5.409, 5.409};

    //-----------------------------------------------------------------------
    //! List of pre-defined doubles used as translations for RANGECMP4 ADR
    //! standard deviation values defined in the RANGECMP4 documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#ADR
    //! Note: LSD have been removed to reduce rouning errors in the range log.
    //!       ADR STD is only 3 decimal places and could round up causing values
    //!       to be greater than values in the table.
    //-----------------------------------------------------------------------
    constexpr std::array<double, 16> stdDevAdrScaling = {0.003, 0.005, 0.007, 0.009, 0.012, 0.016, 0.022, 0.029,
                                                         0.039, 0.052, 0.070, 0.093, 0.124, 0.166, 0.222, 0.222};

    double dSignalWavelength = GetSignalWavelength(stChannelStatus_, cGLONASSFrequencyNumber_ - GLONASS_FREQUENCY_NUMBER_OFFSET);

    //! Some logic for PRN offsets based on the constellation. See documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm#Measurem
    switch (stChannelStatus_.eSatelliteSystem)
    {
    case ChannelTrackingStatus::SATELLITE_SYSTEM::GLONASS:
        // If ternary returns true, documentation suggests we should save this PRN as
        // GLONASS_SLOT_UNKNOWN_UPPER_LIMIT - cGLONASSFrequencyNumber_. However, this
        // would output the PRN as an actual valid Slot ID, which is not true. We will
        // set this to 0 here because 0 is considered an unknown/invalid GLONASS Slot ID.
        stRangeData_.usPRN =
            (GLONASS_SLOT_UNKNOWN_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= GLONASS_SLOT_UNKNOWN_UPPER_LIMIT) ? 0 : uiPRN_ + GLONASS_SLOT_OFFSET - 1;
        break;
    case ChannelTrackingStatus::SATELLITE_SYSTEM::SBAS:
        stRangeData_.usPRN =
            (SBAS_PRN_OFFSET_120_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_120_UPPER_LIMIT)   ? uiPRN_ + SBAS_PRN_OFFSET_120 - 1
            : (SBAS_PRN_OFFSET_130_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_130_UPPER_LIMIT) ? uiPRN_ + SBAS_PRN_OFFSET_130 - 1
                                                                                                       : 0;
        break;
    case ChannelTrackingStatus::SATELLITE_SYSTEM::QZSS: stRangeData_.usPRN = uiPRN_ + QZSS_PRN_OFFSET - 1; break;
    default: stRangeData_.usPRN = uiPRN_; break;
    }

    if (stChannelStatus_.eSatelliteSystem != ChannelTrackingStatus::SATELLITE_SYSTEM::GLONASS && stRangeData_.usPRN == 0)
    {
        throw std::runtime_error("PopulateNextRangeData(): PRN outside of limits");
    }

    // any fields flagged as invalid are set to NaN and appear in the log as such
    stRangeData_.sGLONASSFrequency = static_cast<unsigned char>(cGLONASSFrequencyNumber_);
    stRangeData_.dPSR = stBlock_.bValidPSR ? stBlock_.dPSR : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fPSRStdDev = stdDevPsrScaling[stBlock_.ucPSRBitfield];
    stRangeData_.dADR = stBlock_.bValidPhaseRange ? -stBlock_.dPhaseRange / dSignalWavelength : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fADRStdDev = stdDevAdrScaling[stBlock_.ucADRBitfield];
    stRangeData_.fDopplerFrequency = stBlock_.bValidDoppler ? -stBlock_.dDoppler / dSignalWavelength : std::numeric_limits<float>::quiet_NaN();
    stRangeData_.fCNo = stBlock_.fCNo;
    stRangeData_.fLockTime =
        GetRangeCmp4LockTime(stMetaData_, stBlock_.ucLockTimeBitfield, stChannelStatus_.eSatelliteSystem, stChannelStatus_.eSignalType, uiPRN_);
    stRangeData_.uiChannelTrackingStatus = stChannelStatus_.GetAsWord();
}

//------------------------------------------------------------------------------
//! Populates a provided RangeData structure from the RANGECMP5 blocks provided.
//------------------------------------------------------------------------------
void RangeDecompressor::PopulateNextRangeData(RangeData& stRangeData_, const rangecmp5::MeasurementSignalBlock& stBlock_,
                                              const MetaDataStruct& stMetaData_, const ChannelTrackingStatus& stChannelStatus_, uint32_t uiPRN_,
                                              char cGLONASSFrequencyNumber_)
{
    //-----------------------------------------------------------------------
    //! List of pre-defined doubles used as translations for RANGECMP4 PSR
    //! standard deviation values defined in the RANGECMP4 documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#Pseudora
    //-----------------------------------------------------------------------
    constexpr std::array<double, 32> stdDevPsrScaling = {0.00391, 0.00458, 0.00536, 0.00628, 0.00735, 0.00861, 0.01001, 0.1182,
                                                         0.01385, 0.01621, 0.1900,  0.02223, 0.02607, 0.03054, 0.03577, 0.04190,
                                                         0.04908, 0.05749, 0.06734, 0.07889, 0.09240, 0.10824, 0.12679, 0.14851,
                                                         0.17396, 0.20378, 0.23870, 0.27961, 0.32753, 0.38366, 0.44940, 0.44940};

    //-----------------------------------------------------------------------
    //! List of pre-defined doubles used as translations for RANGECMP5 ADR
    //! standard deviation values defined in the RANGECMP4 documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm?Highlight=Range#ADR
    //! Note: LSD have been removed to reduce rouning errors in the range log.
    //!       ADR STD is only 3 decimal places and could round up causing values
    //!       to be greater than values in the table.
    //-----------------------------------------------------------------------
    constexpr std::array<double, 32> stdDevAdrScaling = {0.020,  0.030,  0.045,  0.066,  0.099,  0.148,   0.220,   0.329,   0.491,   0.732,  1.092,
                                                         1.629,  2.430,  3.625,  5.409,  6.876,  8.741,   11.111,  14.125,  17.957,  22.828, 29.020,
                                                         36.891, 46.898, 59.619, 75.791, 96.349, 122.484, 155.707, 197.943, 251.634, 251.634};

    double dSignalWavelength = GetSignalWavelength(stChannelStatus_, cGLONASSFrequencyNumber_ - GLONASS_FREQUENCY_NUMBER_OFFSET);

    //! Some logic for PRN offsets based on the constellation. See documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm#Measurem
    switch (stChannelStatus_.eSatelliteSystem)
    {
    case ChannelTrackingStatus::SATELLITE_SYSTEM::GLONASS:
        // If ternary returns true, documentation suggests we should save this PRN as
        // GLONASS_SLOT_UNKNOWN_UPPER_LIMIT - cGLONASSFrequencyNumber_. However, this
        // would output the PRN as an actual valid Slot ID, which is not true. We will
        // set this to 0 here because 0 is considered an unknown/invalid GLONASS Slot ID.
        stRangeData_.usPRN =
            (GLONASS_SLOT_UNKNOWN_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= GLONASS_SLOT_UNKNOWN_UPPER_LIMIT) ? 0 : uiPRN_ + GLONASS_SLOT_OFFSET - 1;
        break;
    case ChannelTrackingStatus::SATELLITE_SYSTEM::SBAS:
        stRangeData_.usPRN =
            (SBAS_PRN_OFFSET_120_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_120_UPPER_LIMIT)   ? uiPRN_ + SBAS_PRN_OFFSET_120 - 1
            : (SBAS_PRN_OFFSET_130_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_130_UPPER_LIMIT) ? uiPRN_ + SBAS_PRN_OFFSET_130 - 1
                                                                                                       : 0;
        break;
    case ChannelTrackingStatus::SATELLITE_SYSTEM::QZSS: stRangeData_.usPRN = uiPRN_ + QZSS_PRN_OFFSET - 1; break;
    default: stRangeData_.usPRN = uiPRN_; break;
    }

    if (stChannelStatus_.eSatelliteSystem != ChannelTrackingStatus::SATELLITE_SYSTEM::GLONASS && stRangeData_.usPRN == 0)
    {
        throw std::runtime_error("PopulateNextRangeData(): PRN outside of limits");
    }

    // any fields flagged as invalid are set to NaN and appear in the log as such
    stRangeData_.sGLONASSFrequency = static_cast<unsigned char>(cGLONASSFrequencyNumber_);
    stRangeData_.dPSR = stBlock_.bValidPseudorange ? stBlock_.dPseudorange : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fPSRStdDev = stdDevPsrScaling[stBlock_.ucPseudorangeStdDev];
    stRangeData_.dADR = stBlock_.bValidPhaserange ? -stBlock_.dPhaserange / dSignalWavelength : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fADRStdDev = stdDevAdrScaling[stBlock_.ucPhaserangeStdDev];
    stRangeData_.fDopplerFrequency = stBlock_.bValidDoppler ? -stBlock_.dDoppler / dSignalWavelength : std::numeric_limits<float>::quiet_NaN();
    stRangeData_.fCNo = stBlock_.fCNo;
    stRangeData_.fLockTime =
        GetRangeCmp4LockTime(stMetaData_, stBlock_.ucLockTimeBitfield, stChannelStatus_.eSatelliteSystem, stChannelStatus_.eSignalType, uiPRN_);
    stRangeData_.uiChannelTrackingStatus = stChannelStatus_.GetAsWord();
}

//------------------------------------------------------------------------------
//! Convert a RANGECMP message into RANGE message.
//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmpToRange(const rangecmp::RangeCmp& stRangeCmpMessage_, Range& stRangeMessage_)
{
    using namespace rangecmp;

    //-----------------------------------------------------------------------
    //! Table used to expand the scaled Pseudorange STDs. This is defined
    //! in the RANGECMP documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP.htm?Highlight=rangecmp#StdDevPSRValues
    //-----------------------------------------------------------------------
    constexpr std::array<double, 16> stdDevPsrScaling = {0.050, 0.075, 0.113, 0.169, 0.253,  0.380,  0.570,  0.854,
                                                         1.281, 2.375, 4.750, 9.500, 19.000, 38.000, 76.000, 152.000};

    stRangeMessage_.uiNumberOfObservations = stRangeCmpMessage_.uiNumberOfObservations;

    // Decompress each field from the RangeCmpData and put it into the respective RangeData.
    for (uint32_t uiRangeDataIndex = 0; uiRangeDataIndex < stRangeMessage_.uiNumberOfObservations; uiRangeDataIndex++)
    {
        RangeData& stRangeData = stRangeMessage_.astRangeData[uiRangeDataIndex];
        const RangeCmpData& stRangeCmpData = stRangeCmpMessage_.astRangeData[uiRangeDataIndex];

        // Grab the channel tracking status word and put it into our structure to use it later
        ChannelTrackingStatus stChannelTrackingStatus(stRangeCmpData.uiChannelTrackingStatus);

        stRangeData.uiChannelTrackingStatus = stRangeCmpData.uiChannelTrackingStatus;
        stRangeData.usPRN = stRangeCmpData.ucPRN;

        auto iDoppler = GetBitfield<int32_t, DOPPLER_FREQUENCY_MASK>(stRangeCmpData.ulDopplerFrequencyPSRField);
        HandleSignExtension<DOPPLER_FREQUENCY_SIGNEXT_MASK>(iDoppler);
        stRangeData.fDopplerFrequency = iDoppler / DOPPLER_FREQUENCY_SCALE_FACTOR;

        stRangeData.dPSR = GetBitfield<uint64_t, PSR_MEASUREMENT_MASK>(stRangeCmpData.ulDopplerFrequencyPSRField) / PSR_MEASUREMENT_SCALE_FACTOR;
        stRangeData.fPSRStdDev = stdDevPsrScaling[stRangeCmpData.ucStdDevPSRStdDevADR & PSR_STDDEV_MASK];
        stRangeData.fADRStdDev =
            (GetBitfield<uint32_t, ADR_STDDEV_MASK>(stRangeCmpData.ucStdDevPSRStdDevADR) + ADR_STDDEV_SCALE_OFFSET) / ADR_STDDEV_SCALE_FACTOR;
        stRangeData.fLockTime = GetBitfield<uint32_t, LOCK_TIME_MASK>(stRangeCmpData.uiLockTimeCNoGLOFreq) / LOCK_TIME_SCALE_FACTOR;
        stRangeData.fCNo = GetBitfield<uint32_t, CNO_MASK>(stRangeCmpData.uiLockTimeCNoGLOFreq) + CNO_SCALE_OFFSET;
        stRangeData.sGLONASSFrequency = GetBitfield<int16_t, GLONASS_FREQUENCY_MASK>(stRangeCmpData.uiLockTimeCNoGLOFreq);

        double dWavelength = GetSignalWavelength(stChannelTrackingStatus, stRangeData.sGLONASSFrequency);
        stRangeData.dADR = stRangeCmpData.uiADR / ADR_SCALE_FACTOR;
        double dADRRolls = ((stRangeData.dPSR / dWavelength) + stRangeData.dADR) / MAX_VALUE;
        stRangeData.dADR -= MAX_VALUE * static_cast<uint64_t>(std::round(dADRRolls));
    }
}

//------------------------------------------------------------------------------
// Convert a RANGECMP2 message into RANGE message.
//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmp2ToRange(const rangecmp2::RangeCmp& stRangeCmpMessage_, Range& stRangeMessage_, const MetaDataStruct& stMetaData_)
{
    using namespace rangecmp2;

    //-----------------------------------------------------------------------
    //! Table used to expand the scaled Pseudorange STDs. This is defined
    //! in the RANGECMP2 documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP2.htm?Highlight=RANGECMP2#StdDevPSRScaling
    //-----------------------------------------------------------------------
    constexpr std::array<double, 16> stdDevPsrScaling = {0.020, 0.030, 0.045, 0.066, 0.099, 0.148, 0.220, 0.329,
                                                         0.491, 0.732, 1.092, 1.629, 2.430, 3.625, 5.409, 5.409};

    //-----------------------------------------------------------------------
    //! Table used to expand the scaled Accumulated Doppler Range STDs. This
    //!  is defined in the RANGECMP2 documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP2.htm?Highlight=RANGECMP2#StdDevADRScaling
    //-----------------------------------------------------------------------
    constexpr std::array<double, 16> stdDevAdrScaling = {0.00391, 0.00521, 0.00696, 0.00929, 0.01239, 0.01654, 0.02208, 0.02947,
                                                         0.03933, 0.05249, 0.07006, 0.09350, 0.12480, 0.16656, 0.22230, 0.22230};

    stRangeMessage_.uiNumberOfObservations = 0;
    uint32_t uiRangeDataBytesDecompressed = 0;
    while (uiRangeDataBytesDecompressed < stRangeCmpMessage_.uiNumberOfRangeDataBytes)
    {
        const auto& stSatBlock = reinterpret_cast<const SatelliteBlock&>(stRangeCmpMessage_.aucRangeData[uiRangeDataBytesDecompressed]);

        const auto eSystem = GetBitfield<SYSTEM, SAT_SATELLITE_SYSTEM_ID_MASK>(stSatBlock.ulCombinedField);
        const auto ucSignalBlockCount = GetBitfield<uint8_t, SAT_NUM_SIGNAL_BLOCKS_BASE_MASK>(stSatBlock.ulCombinedField);

        auto iPSRBase = GetBitfield<int32_t, SAT_SATELLITE_PSR_BASE_MASK>(stSatBlock.ulCombinedField);
        HandleSignExtension<SAT_SATELLITE_PSR_BASE_SIGNEXT_MASK>(iPSRBase);
        auto iDopplerBase = GetBitfield<int32_t, SAT_SATELLITE_DOPPLER_BASE_MASK>(stSatBlock.ulCombinedField);
        HandleSignExtension<SAT_SATELLITE_DOPPLER_BASE_SIGNEXT_MASK>(iDopplerBase);

        uiRangeDataBytesDecompressed += sizeof(SatelliteBlock);

        // Decompress the Signal Blocks associated with the Satellite Block
        for (uint8_t ucSignalBlockIndex = 0; ucSignalBlockIndex < ucSignalBlockCount; ucSignalBlockIndex++)
        {
            // Decompress the signal block
            const auto& stSigBlock = reinterpret_cast<const SignalBlock&>(stRangeCmpMessage_.aucRangeData[uiRangeDataBytesDecompressed]);

            const auto eSignalType = GetBitfield<SIGNAL_TYPE, SIG_SIGNAL_TYPE_MASK>(stSigBlock.uiCombinedField1);
            const auto ucPSRBitfield = GetBitfield<uint8_t, SIG_PSR_STDDEV_MASK>(stSigBlock.ullCombinedField2);
            const auto ucADRBitfield = GetBitfield<uint8_t, SIG_ADR_STDDEV_MASK>(stSigBlock.ullCombinedField2);
            const auto uiLockTimeBits = GetBitfield<uint32_t, SIG_LOCKTIME_MASK>(stSigBlock.uiCombinedField1);
            const auto usPRN = stSatBlock.ucSatelliteIdentifier + (eSystem == SYSTEM::GLONASS ? GLONASS_SLOT_OFFSET - 1 : 0);

            auto iDopplerBitfield = GetBitfield<int32_t, SIG_DOPPLER_DIFF_MASK>(stSigBlock.ullCombinedField2);
            HandleSignExtension<SIG_DOPPLER_DIFF_SIGNEXT_MASK>(iDopplerBitfield);

            ChannelTrackingStatus stChannelTrackingStatus(stSatBlock, stSigBlock);

            // Construct the decompressed range data
            RangeData& stRangeData = stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++];
            stRangeData.usPRN = usPRN;
            stRangeData.sGLONASSFrequency = GetBitfield<int16_t, SAT_GLONASS_FREQUENCY_ID_MASK>(stSatBlock.ulCombinedField);
            stRangeData.dPSR = iPSRBase + (GetBitfield<uint64_t, SIG_PSR_DIFF_MASK>(stSigBlock.ullCombinedField2) / SIG_PSR_DIFF_SCALE_FACTOR);
            stRangeData.fPSRStdDev = stdDevPsrScaling[ucPSRBitfield];
            stRangeData.dADR =
                -((iPSRBase + (GetBitfield<uint64_t, SIG_PHASERANGE_DIFF_MASK>(stSigBlock.ullCombinedField2) / SIG_PHASERANGE_DIFF_SCALE_FACTOR)) /
                  GetSignalWavelength(stChannelTrackingStatus, stRangeData.sGLONASSFrequency - GLONASS_FREQUENCY_NUMBER_OFFSET));
            stRangeData.fADRStdDev = stdDevAdrScaling[ucADRBitfield];
            stRangeData.fDopplerFrequency =
                (iDopplerBase + (iDopplerBitfield / SIG_DOPPLER_DIFF_SCALE_FACTOR)) / RangeCmp2SignalScaling(eSystem, eSignalType);
            stRangeData.fCNo = SIG_CNO_SCALE_OFFSET + GetBitfield<uint64_t, SIG_CNO_MASK>(stSigBlock.ullCombinedField2);
            stRangeData.fLockTime = GetRangeCmp2LockTime(stMetaData_, uiLockTimeBits, stChannelTrackingStatus.eSatelliteSystem,
                                                         stChannelTrackingStatus.eSignalType, usPRN);
            stRangeData.uiChannelTrackingStatus = stChannelTrackingStatus.GetAsWord();

            uiRangeDataBytesDecompressed += sizeof(SignalBlock);
        }
    }
}

//------------------------------------------------------------------------------
// Convert a RANGECMP4 message into RANGE message.
//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmp4ToRange(unsigned char* pucData_, Range& stRangeMessage_, const MetaDataStruct& stMetaData_)
{
    using namespace rangecmp4;

    const MEASUREMENT_SOURCE eSource = stMetaData_.eMeasurementSource;
    double dSecondOffset = static_cast<double>(static_cast<uint32_t>(stMetaData_.dMilliseconds) % SEC_TO_MILLI_SEC) / SEC_TO_MILLI_SEC;
    // Clear any dead reference blocks on the whole second. We should be storing new ones.
    if (std::abs(dSecondOffset) < std::numeric_limits<double>::epsilon())
    {
        for (auto& it : mMyReferenceBlocks)
        {
            if (static_cast<MEASUREMENT_SOURCE>(it.first & 1) == eSource) { it.second = {}; }
        }
    }

    stRangeMessage_.uiNumberOfObservations = 0;
    uint32_t uiBitOffset = 0;
    uint32_t uiBytesLeft = *reinterpret_cast<uint32_t*>(pucData_);
    pucData_ += sizeof(uint32_t);

    auto systems = ExtractBitfield<uint16_t>(&pucData_, uiBytesLeft, uiBitOffset, SATELLITE_SYSTEMS_BITS);

    while (systems)
    {
        auto system = static_cast<SYSTEM>(PopLsb(systems));

        // WARNING: We use arrays instead of vectors for PRNs and Signals to avoid using dynamic memory allocation.
        // This means that we have to be careful using the size of the array and iterators.
        auto satellitesTemp = ExtractBitfield<uint64_t>(&pucData_, uiBytesLeft, uiBitOffset, SATELLITES_BITS);
        std::array<uint32_t, SATELLITES_BITS> aPrns;
        uint32_t uiPrnCount = 0;
        while (satellitesTemp) { aPrns[uiPrnCount++] = PopLsb(satellitesTemp) + 1; } // Bit position is PRN - 1, so + 1 here

        auto signalsTemp = ExtractBitfield<uint64_t>(&pucData_, uiBytesLeft, uiBitOffset, SIGNALS_BITS);
        std::array<SIGNAL_TYPE, SIGNALS_BITS> aSignals;
        uint32_t uiSignalCount = 0;
        while (signalsTemp) { aSignals[uiSignalCount++] = static_cast<SIGNAL_TYPE>(PopLsb(signalsTemp)); }

        std::array<uint64_t, SATELLITES_BITS> includedSignals;
        // Iterate through the PRNs once to collect the signals tracked by each. We need this info before we can start decompressing.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Get the m*n bit matrix that describes the included signals in this RANGECMP4 message.
            includedSignals[aPrns[uiPrnIndex]] = ExtractBitfield<uint64_t>(&pucData_, uiBytesLeft, uiBitOffset, uiSignalCount);
        }

        // Check each PRN against the signals tracked in this satellite system to see if the signal is included.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Begin decoding Reference Measurement Block Header.
            MeasurementBlockHeader stMbHeader;
            stMbHeader.bIsDifferentialData = ExtractBitfield<bool>(&pucData_, uiBytesLeft, uiBitOffset, MBLK_HDR_DATAFORMAT_FLAG_BITS);
            stMbHeader.ucReferenceDataBlockID = ExtractBitfield<uint8_t>(&pucData_, uiBytesLeft, uiBitOffset, MBLK_HDR_REFERENCE_DATABLOCK_ID_BITS);
            stMbHeader.cGLONASSFrequencyNumber = 0;

            // This field is only present for GLONASS and reference blocks.
            if (system == SYSTEM::GLONASS && !stMbHeader.bIsDifferentialData)
            {
                stMbHeader.cGLONASSFrequencyNumber =
                    ExtractBitfield<uint8_t>(&pucData_, uiBytesLeft, uiBitOffset, MBLK_HDR_GLONASS_FREQUENCY_NUMBER_BITS);
            }

            const uint32_t& prn = aPrns[uiPrnIndex];
            uint64_t& included = includedSignals[prn];
            const uint32_t uiIncludedSignalCount = PopCount(included);
            bool bPrimaryBlock = true;
            double primaryPseudorange{};
            double primaryDoppler{};

            while (included)
            {
                const SIGNAL_TYPE signal = aSignals[PopLsb(included)];
                const uint64_t key = MakeKey(system, prn, signal, eSource);
                MeasurementSignalBlock stBlock;

                if (stMbHeader.bIsDifferentialData) // This is a differential block.
                {
                    try
                    {
                        const std::pair<MeasurementBlockHeader, MeasurementSignalBlock>& stRb = mMyReferenceBlocks.at(key);

                        if (stMbHeader.ucReferenceDataBlockID == stRb.first.ucReferenceDataBlockID)
                        {
                            if (bPrimaryBlock)
                            {
                                DecompressDifferentialBlock<false>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, stRb.second, dSecondOffset);
                                bPrimaryBlock = false;
                            }
                            else { DecompressDifferentialBlock<true>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, stRb.second, dSecondOffset); }

                            ChannelTrackingStatus stChannelTrackingStatus(system, signal, stBlock);
                            PopulateNextRangeData(stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++], stBlock, stMetaData_,
                                                  stChannelTrackingStatus, prn, stRb.first.cGLONASSFrequencyNumber);
                        }
                        else
                        {
                            pclMyLogger->warn("Invalid reference data: Diff ID {} != Ref ID {}", stMbHeader.ucReferenceDataBlockID,
                                              stRb.first.ucReferenceDataBlockID);
                        }
                    }
                    catch (...)
                    {
                        pclMyLogger->warn("No reference data exists for SATELLITE_SYSTEM {}, SIGNAL_TYPE {}, PRN {}, ID {}",
                                          static_cast<int32_t>(system), static_cast<int32_t>(signal), prn, stMbHeader.ucReferenceDataBlockID);
                    }
                }
                else // This is a reference block.
                {
                    if (bPrimaryBlock)
                    {
                        DecompressReferenceBlock<false>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler);
                        primaryPseudorange = stBlock.dPSR;
                        primaryDoppler = stBlock.dDoppler;
                        bPrimaryBlock = false;
                    }
                    else { DecompressReferenceBlock<true>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler); }

                    ChannelTrackingStatus stChannelTrackingStatus(system, signal, stBlock);
                    PopulateNextRangeData(stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++], stBlock, stMetaData_,
                                          stChannelTrackingStatus, prn, stMbHeader.cGLONASSFrequencyNumber);

                    // Always store reference blocks.
                    mMyReferenceBlocks[key] = std::pair(stMbHeader, stBlock);
                }
            }

            // Update the grouping bit in the status word if multiple signals for this PRN are counted.
            if (uiIncludedSignalCount > 1 && uiIncludedSignalCount <= stRangeMessage_.uiNumberOfObservations)
            {
                for (uint32_t uiIndex = uiIncludedSignalCount; uiIndex > 0; uiIndex--)
                {
                    stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations - uiIndex].uiChannelTrackingStatus |= CTS_GROUPING_MASK;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
//! Decompresses a RANGECMP5 differential measurement block. Populates the
//! provided reference block struct, but must be given the appropriate
//! reference block from the same RANGECMP4 message.
//------------------------------------------------------------------------------
template <bool bSecondary>
void RangeDecompressor::DecompressBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                        rangecmp5::MeasurementSignalBlock& stBlock_, double primaryPseudorange, double primaryDoppler)
{
    using namespace rangecmp5;

    stBlock_.bParityKnown = ExtractBitfield<bool>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_PARITY_FLAG_BITS);
    stBlock_.bHalfCycleAdded = ExtractBitfield<bool>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_HALF_CYCLE_BITS);
    stBlock_.fCNo = ExtractBitfield<float>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_CNO_BITS) * SIG_BLK_CNO_SCALE_FACTOR;
    stBlock_.ucLockTimeBitfield = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_LOCK_TIME_BITS);
    stBlock_.ucPseudorangeStdDev = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_PSR_STDDEV_BITS);
    stBlock_.ucPhaserangeStdDev = ExtractBitfield<uint8_t>(ppucData_, uiBytesLeft_, uiBitOffset_, SIG_BLK_ADR_STDDEV_BITS);

    auto llPSRBitfield = ExtractBitfield<int64_t>(ppucData_, uiBytesLeft_, uiBitOffset_, RBLK_PSR_BITS[bSecondary]);
    if constexpr (bSecondary) { HandleSignExtension<SSIG_RBLK_PSR_SIGNEXT_MASK>(llPSRBitfield); }
    auto iPhaseRangeBitfield = ExtractBitfield<int32_t>(ppucData_, uiBytesLeft_, uiBitOffset_, RBLK_PHASERANGE_BITS);
    HandleSignExtension<RBLK_PHASERANGE_SIGNEXT_MASK>(iPhaseRangeBitfield);
    auto iDopplerBitfield = ExtractBitfield<int32_t>(ppucData_, uiBytesLeft_, uiBitOffset_, RBLK_DOPPLER_BITS[bSecondary]);
    HandleSignExtension<RBLK_DOPPLER_SIGNEXT_MASK[bSecondary]>(iDopplerBitfield);

    stBlock_.bValidPseudorange = llPSRBitfield != RBLK_INVALID_PSR[bSecondary];
    stBlock_.dPseudorange = llPSRBitfield * SIG_BLK_PSR_SCALE_FACTOR + (bSecondary ? primaryPseudorange : 0);
    stBlock_.bValidPhaserange = iPhaseRangeBitfield != SIG_RBLK_INVALID_PHASERANGE;
    stBlock_.dPhaserange = iPhaseRangeBitfield * SIG_BLK_PHASERANGE_SCALE_FACTOR + stBlock_.bValidPseudorange;
    stBlock_.bValidDoppler = iDopplerBitfield != PSIG_RBLK_INVALID_DOPPLER;
    stBlock_.dDoppler = iDopplerBitfield * SIG_BLK_DOPPLER_SCALE_FACTOR + (bSecondary ? primaryDoppler : 0);
}

//------------------------------------------------------------------------------
// Convert a RANGECMP5 message into RANGE message.
//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmp5ToRange(unsigned char* pucData_, Range& stRangeMessage_, [[maybe_unused]] const MetaDataStruct& stMetaData_)
{
    using namespace rangecmp5;

    stRangeMessage_.uiNumberOfObservations = 0;
    uint32_t uiBitOffset = 0;
    uint32_t uiBytesLeft = *reinterpret_cast<uint32_t*>(pucData_);
    pucData_ += sizeof(uint32_t);

    auto systems = ExtractBitfield<uint16_t>(&pucData_, uiBytesLeft, uiBitOffset, SATELLITE_SYSTEMS_BITS);

    while (systems)
    {
        auto system = static_cast<SYSTEM>(PopLsb(systems));

        // WARNING: We use arrays instead of vectors for PRNs and Signals to avoid using dynamic memory allocation.
        // This means that we have to be careful using the size of the array and iterators.
        auto satellitesTemp = ExtractBitfield<uint64_t>(&pucData_, uiBytesLeft, uiBitOffset, SATELLITES_BITS);
        std::array<uint32_t, SATELLITES_BITS> aPrns;
        uint32_t uiPrnCount = 0;
        while (satellitesTemp) { aPrns[uiPrnCount++] = PopLsb(satellitesTemp); }

        auto signalsTemp = ExtractBitfield<uint64_t>(&pucData_, uiBytesLeft, uiBitOffset, SIGNALS_BITS);
        std::array<rangecmp4::SIGNAL_TYPE, SIGNALS_BITS> aSignals;
        uint32_t uiSignalCount = 0;
        while (signalsTemp) { aSignals[uiSignalCount++] = static_cast<rangecmp4::SIGNAL_TYPE>(PopLsb(signalsTemp)); }

        std::array<uint64_t, SATELLITES_BITS> includedSignals;
        // Iterate through the PRNs once to collect the signals tracked by each. We need this info before we can start decompressing.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Get the m*n bit matrix that describes the included signals in this RANGECMP5 message.
            includedSignals[aPrns[uiPrnIndex]] = ExtractBitfield<uint64_t>(&pucData_, uiBytesLeft, uiBitOffset, uiSignalCount);
        }

        // Check each PRN against the signals tracked in this satellite system to see if the signal is included.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Begin decoding Reference Measurement Block Header.
            MeasurementBlockHeader stMbHeader;
            stMbHeader.bDataFormatFlag = ExtractBitfield<bool>(&pucData_, uiBytesLeft, uiBitOffset, 1);
            stMbHeader.ucReserved = ExtractBitfield<uint8_t>(&pucData_, uiBytesLeft, uiBitOffset, 3);
            stMbHeader.cGLONASSFrequencyNumber = 0;

            // This field is only present for GLONASS and reference blocks.
            if (system == SYSTEM::GLONASS) { stMbHeader.cGLONASSFrequencyNumber = ExtractBitfield<uint8_t>(&pucData_, uiBytesLeft, uiBitOffset, 5); }

            const uint32_t& prn = aPrns[uiPrnIndex];
            uint64_t& included = includedSignals[prn];
            const uint32_t uiIncludedSignalCount = PopCount(included);
            bool bPrimaryBlock = true;
            double primaryPseudorange{};
            double primaryDoppler{};

            while (included)
            {
                const rangecmp4::SIGNAL_TYPE signal = aSignals[PopLsb(included)];
                MeasurementSignalBlock stBlock;

                if (bPrimaryBlock)
                {
                    DecompressBlock<false>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler);
                    primaryPseudorange = stBlock.dPseudorange;
                    primaryDoppler = stBlock.dDoppler;
                    bPrimaryBlock = false;
                }
                else { DecompressBlock<true>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler); }

                ChannelTrackingStatus stChannelTrackingStatus(system, signal, stBlock);
                PopulateNextRangeData(stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++], stBlock, stMetaData_,
                                      stChannelTrackingStatus, prn, stMbHeader.cGLONASSFrequencyNumber);
            }

            // Update the grouping bit in the status word if multiple signals for this PRN are counted.
            if (uiIncludedSignalCount > 1 && uiIncludedSignalCount <= stRangeMessage_.uiNumberOfObservations)
            {
                for (uint32_t uiIndex = uiIncludedSignalCount; uiIndex > 0; uiIndex--)
                {
                    stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations - uiIndex].uiChannelTrackingStatus |= CTS_GROUPING_MASK;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
//! This method will decompress the range message provided in pucBuffer_
//! and overwrite the contents with the decompressed message.
//------------------------------------------------------------------------------
STATUS RangeDecompressor::Decompress(unsigned char* pucBuffer_, uint32_t uiBufferSize_, MetaDataStruct& stMetaData_, ENCODE_FORMAT eFormat_)
{
    if (pucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDB == nullptr) { return STATUS::NO_DATABASE; }

    MessageDataStruct stMessageData;
    IntermediateHeader stHeader;
    std::vector<FieldContainer> stMessage;
    auto eStatus = STATUS::UNKNOWN;

    unsigned char* pucTempMessagePointer = pucBuffer_;
    eStatus = clMyHeaderDecoder.Decode(pucTempMessagePointer, stHeader, stMetaData_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    if (!clMyRangeCmpFilter.DoFiltering(stMetaData_)) { return STATUS::UNSUPPORTED; }

    HEADER_FORMAT eFormat = stMetaData_.eFormat;
    pucTempMessagePointer += stMetaData_.uiHeaderLength;
    // If the message is not in binary format, we need to ensure that it is encoded to binary so that it can be decompressed.
    if (eFormat != HEADER_FORMAT::BINARY)
    {
        eStatus = clMyMessageDecoder.Decode(pucTempMessagePointer, stMessage, stMetaData_);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }

        eStatus = clMyEncoder.Encode(&pucBuffer_, uiBufferSize_, stHeader, stMessage, stMessageData, stMetaData_, ENCODE_FORMAT::FLATTENED_BINARY);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }

        pucTempMessagePointer = stMessageData.pucMessageBody;
    }

    // Convert the RANGECMPx message to a RANGE message
    try
    {
        Range stRange;
        switch (stMetaData_.usMessageId)
        {
        case RANGECMP_MSG_ID: RangeCmpToRange(*reinterpret_cast<rangecmp::RangeCmp*>(pucTempMessagePointer), stRange); break;
        case RANGECMP2_MSG_ID: RangeCmp2ToRange(*reinterpret_cast<rangecmp2::RangeCmp*>(pucTempMessagePointer), stRange, stMetaData_); break;
        case RANGECMP3_MSG_ID: [[fallthrough]];
        case RANGECMP4_MSG_ID: RangeCmp4ToRange(pucTempMessagePointer, stRange, stMetaData_); break;
        case RANGECMP5_MSG_ID: RangeCmp5ToRange(pucTempMessagePointer, stRange, stMetaData_); break;
        default: return STATUS::UNSUPPORTED;
        }

        // Set the binary message length in the metadata for decoding purposes.
        stMetaData_.uiBinaryMsgLength = sizeof(stRange.uiNumberOfObservations) + stRange.uiNumberOfObservations * sizeof(RangeData);
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

    stMetaData_.eFormat = eFormat;
    // Re-encode to the original format if a format was not specified.
    if (eFormat_ == ENCODE_FORMAT::UNSPECIFIED)
    {
        eFormat_ = eFormat == HEADER_FORMAT::BINARY || eFormat == HEADER_FORMAT::SHORT_BINARY || eFormat == HEADER_FORMAT::PROPRIETARY_BINARY
                       ? ENCODE_FORMAT::BINARY
                   : eFormat == HEADER_FORMAT::ASCII || eFormat == HEADER_FORMAT::SHORT_ASCII || eFormat == HEADER_FORMAT::ABB_ASCII
                       ? ENCODE_FORMAT::ASCII
                   : eFormat == HEADER_FORMAT::JSON ? ENCODE_FORMAT::JSON
                                                    : ENCODE_FORMAT::ASCII; // Default to ASCII
    }

    // Re-encode the data back into the range message buffer.
    eStatus = clMyEncoder.Encode(&pucBuffer_, uiBufferSize_, stHeader, stMessage, stMessageData, stMetaData_, eFormat_);
    if (eStatus != STATUS::SUCCESS) { return eStatus; }

    // Final adjustments to MetaData
    stMetaData_.uiLength = stMessageData.uiMessageLength;
    stMetaData_.uiHeaderLength = stMessageData.uiMessageHeaderLength;

    return STATUS::SUCCESS;
}
