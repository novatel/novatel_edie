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
RangeDecompressor::RangeDecompressor(JsonReader* pclJsonDb_) : clMyHeaderDecoder(pclJsonDb_), clMyMessageDecoder(pclJsonDb_), clMyEncoder(pclJsonDb_)
{
    pclMyLogger = Logger::RegisterLogger("range_decompressor");
    pclMyLogger->debug("RangeDecompressor initializing...");

    if (pclJsonDb_ != nullptr) { LoadJsonDb(pclJsonDb_); }

    for (const auto id : {RANGECMP_MSG_ID, RANGECMP2_MSG_ID, RANGECMP3_MSG_ID, RANGECMP4_MSG_ID, RANGECMP5_MSG_ID})
    {
        clMyRangeCmpFilter.IncludeMessageId(id, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::PRIMARY);
        clMyRangeCmpFilter.IncludeMessageId(id, HEADER_FORMAT::ALL, MEASUREMENT_SOURCE::SECONDARY);
    }

    pclMyLogger->debug("RangeDecompressor initialized");
}

//------------------------------------------------------------------------------
void RangeDecompressor::LoadJsonDb(JsonReader* pclJsonDb_)
{
    pclMyMsgDB = pclJsonDb_;
    clMyHeaderDecoder.LoadJsonDb(pclJsonDb_);
    clMyMessageDecoder.LoadJsonDb(pclJsonDb_);
    clMyEncoder.LoadJsonDb(pclJsonDb_);
}

//------------------------------------------------------------------------------
// The lock time can only report up to 131071ms (0x1FFFF). If this value is
// reached, the lock time must continue to increment. Once the saturated value
// has been reached, store the header time at which the lock time was found to
// be saturated  and any difference between the header and stored time in the
// future can be added to the saturated lock time value to obtain the true
// lock time value.
// NOTE: that this is only true in the case that lock time is not saturated in
// the first observation for this system, signal, PRN. If the lock time is
// saturated in the first observation, the lock time is relative to the first
// observation, and may not be a true representation of the time the
// observation has actually been locked.
//------------------------------------------------------------------------------
double RangeDecompressor::GetRangeCmp2LockTime(const MetaDataStruct& stMetaData_, uint32_t uiLockTimeBits_, uint64_t key_)
{
    using namespace rangecmp2;

    double dLockTimeMilliseconds = uiLockTimeBits_;

    LockTimeInfo& stLockTimeInfo = mMyRangeCmp2LockTimes[key_];
    if (uiLockTimeBits_ == SIG_LOCKTIME_MASK >> Lsb(SIG_LOCKTIME_MASK))
    {
        // If the lock time was already saturated, use the stored time to add the missing offset.
        if (stLockTimeInfo.bLockTimeSaturated) { dLockTimeMilliseconds += stMetaData_.dMilliseconds - stLockTimeInfo.dLockTimeSaturatedMilliseconds; }
        // If the lock time is not already saturated, store this information if this observation is seen again.
        else
        {
            stLockTimeInfo.dLockTimeSaturatedMilliseconds = stMetaData_.dMilliseconds;
            stLockTimeInfo.bLockTimeSaturated = true;
        }
    }
    // If the lock time is marked as saturated but not reported as such from the RANGECMP2 message, clear the flag.
    else if (stLockTimeInfo.bLockTimeSaturated) { stLockTimeInfo.bLockTimeSaturated = false; }

    return dLockTimeMilliseconds / SEC_TO_MILLI_SEC;
}

//------------------------------------------------------------------------------
// RANGECMP4 lock time information is categorized into certain ranges for a
// given observation. Decompressing this information without knowledge of the
// precise time at which lock was acquired can result in initially misleading
// lock times. There are a number of cases which may occur when decompressing
// the lock time bitfields from a RANGECMP4 observation:
// NOTE: See lockTime for ullBitfield:lock time translations.
//   1. The lock time ullBitfield is b0000:
//      - This is the simplest case. The lock time will increase with the time
//        reported by the message header.
//   2. The lock time ullBitfield is b1111:
//      - There is no way to determine when the lock time reached that range,
//        so as for case 1, the lock time will increase with the time reported
//        by the message header.
//   3. The lock time ullBitfield is any value from b0001 to b1110:
//      - This case is the most complex. Because there is no guarantee that
//        the current lock time reported has just transitioned to the lower
//        boundary of the ullBitfield representation, it must be stated that the
//        lock time is relative, and not absolute. Only when a change in the
//        ullBitfield is detected can it be guaranteed that the lock time is
//        absolute. At this point, the lock time can jump or slip to adjust
//        to the newly found ullBitfield. After the jump or slip occurs, the
//        lock time must not change again, as the lower ullBitfield values will
//        produce the highest degree of accuracy.
//      - For example, if the lock time ullBitfield is b0111, the observation has
//        been locked for at least 1024ms. However, it is possible the message
//        was produced when the observation was locked for 1800ms. This means
//        a 776ms discrepancy exists between what the RangeDecompressor tracks
//        and what is reported in the RANGECMP messages. When this discrepancy
//        is detected, the lock time can jump to match what the RANGECMP message
//        reports. Thus, the distinction between "relative" and "absolute"
//        must be made to allow the decompressor to accurately reflect
//        observation lock times. See the example below:
//
// Lock Time (t)
//   ^
//   |                                                 . < Continues 1:1
//   |                                               .
//   |                                             .
//   |                                           /
//   |                                         /
//   |                                       /
//   |                                     /
//   |                                   /   < At this point a ullBitfield change
//   |                                 *|      was found. The transition from
//   |                               *  |      "relative" to "absolute" time
//   |                             *    |      results in a lock time jump as
//   |                           *      |      seen here.
//   |                         *       /
//   |                       *       /
//   |                     *       /
//   |                   *       /
//   | Absolute        *       /   < Relative lock time inferred by the
//   | lock time >    *       /       RangeDecompressor. These values
//   |             *       /         will be output to RANGE message
//   |           *       /           lock time fields.
//   +-------------------------------------------------------------------->
//                   Header time (t)
//------------------------------------------------------------------------------
double RangeDecompressor::GetRangeCmp4LockTime(const MetaDataStruct& stMetaData_, uint8_t ucLockTimeBits_, uint64_t key_)
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

    // Store the lock time if it is different from the once we have currently.
    LockTimeInfo& stLockTimeInfo = mMyRangeCmp4LockTimes[key_];

    // Is the lock time relative and has a ullBitfield change been found?
    if (!stLockTimeInfo.bAbsolute && ucLockTimeBits_ != stLockTimeInfo.ucBits)
    {
        // Set lock time as absolute if bits are 0 or transitioning from relative to absolute.
        if (ucLockTimeBits_ == 0 || (stLockTimeInfo.ucBits != 0xFF && ucLockTimeBits_ > stLockTimeInfo.ucBits))
        {
            stLockTimeInfo.bAbsolute = true;

            double dLockTimeDeltaMs = std::abs(lockTime[ucLockTimeBits_] - stLockTimeInfo.dMilliseconds);
            if (dLockTimeDeltaMs > std::numeric_limits<double>::epsilon())
            {
                pclMyLogger->warn("Detected a lock time jump of {}ms at time {}w, {}ms.", dLockTimeDeltaMs, stMetaData_.usWeek,
                                  stMetaData_.dMilliseconds);
            }
        }

        // Record the last bit change and the ullBitfield that was changed to.
        stLockTimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
        stLockTimeInfo.ucBits = ucLockTimeBits_;
    }
    // If the lock time is absolute and the lock time bits have decreased, there was likely an outage.
    else if (ucLockTimeBits_ < stLockTimeInfo.ucBits)
    {
        // In the event of an outage of any kind, reset the lock time to relative.
        stLockTimeInfo.bAbsolute = false;
        stLockTimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
        stLockTimeInfo.ucBits = ucLockTimeBits_;

        pclMyLogger->warn("Detected a lock time slip (perhaps caused by an outage) of {}ms at time {}w, {}ms.",
                          stLockTimeInfo.dMilliseconds - lockTime[stLockTimeInfo.ucBits], stMetaData_.usWeek, stMetaData_.dMilliseconds);
    }
    else
    {
        // If the lock time is absolute and the ullBitfield hasn't changed within the expected time, reset the last change time
        if (stMetaData_.dMilliseconds - stLockTimeInfo.dLastBitfieldChangeMilliseconds > 2 * lockTime[ucLockTimeBits_])
        {
            stLockTimeInfo.dLastBitfieldChangeMilliseconds = stMetaData_.dMilliseconds;
            pclMyLogger->warn("Expected a bit change much sooner at time {}w, {}ms.", stMetaData_.usWeek, stMetaData_.dMilliseconds);
        }
    }
    stLockTimeInfo.dMilliseconds = stMetaData_.dMilliseconds - stLockTimeInfo.dLastBitfieldChangeMilliseconds + lockTime[stLockTimeInfo.ucBits];
    return stLockTimeInfo.dMilliseconds / SEC_TO_MILLI_SEC;
}

//------------------------------------------------------------------------------
template <bool Secondary>
void RangeDecompressor::DecompressReferenceBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                                 rangecmp4::MeasurementSignalBlock& stRefBlock_, double primaryPsr, double primaryDoppler) const
{
    using namespace rangecmp4;

    stRefBlock_.bParityKnown = ExtractBitfield<bool, SIG_BLK_PARITY_FLAG_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stRefBlock_.bHalfCycleAdded = ExtractBitfield<bool, SIG_BLK_HALF_CYCLE_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stRefBlock_.fCNo = ExtractBitfield<float, SIG_BLK_CNO_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_) * SIG_BLK_CNO_SCALE_FACTOR;
    stRefBlock_.ucLockTimeBitfield = ExtractBitfield<uint8_t, SIG_BLK_LOCK_TIME_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stRefBlock_.ucPsrStdDev = ExtractBitfield<uint8_t, SIG_BLK_PSR_STDDEV_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stRefBlock_.ucPhrStdDev = ExtractBitfield<uint8_t, SIG_BLK_ADR_STDDEV_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);

    auto llPsr = ExtractBitfield<int64_t, RBLK_PSR_BITS[Secondary]>(ppucData_, uiBytesLeft_, uiBitOffset_);
    if constexpr (Secondary) { HandleSignExtension<SSIG_RBLK_PSR_SIGNEXT_MASK>(llPsr); }
    auto iPhr = ExtractBitfield<int32_t, RBLK_PHR_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    HandleSignExtension<RBLK_PHR_SIGNEXT_MASK>(iPhr);
    auto iDoppler = ExtractBitfield<int32_t, RBLK_DOPPLER_BITS[Secondary]>(ppucData_, uiBytesLeft_, uiBitOffset_);
    HandleSignExtension<RBLK_DOPPLER_SIGNEXT_MASK[Secondary]>(iDoppler);

    stRefBlock_.bValidPsr = llPsr != RBLK_INVALID_PSR[Secondary];
    stRefBlock_.dPsr = llPsr * SIG_BLK_PSR_SCALE_FACTOR + (Secondary ? primaryPsr : 0);
    stRefBlock_.bValidPhr = iPhr != SIG_RBLK_INVALID_PHR;
    stRefBlock_.dPhr = iPhr * SIG_BLK_PHR_SCALE_FACTOR + stRefBlock_.dPsr;
    stRefBlock_.bValidDoppler = iDoppler != PSIG_RBLK_INVALID_DOPPLER;
    stRefBlock_.dDoppler = iDoppler * SIG_BLK_DOPPLER_SCALE_FACTOR + (Secondary ? primaryDoppler : 0);
}

//------------------------------------------------------------------------------
template <bool Secondary>
void RangeDecompressor::DecompressDifferentialBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                                    rangecmp4::MeasurementSignalBlock& stDiffBlock_,
                                                    const rangecmp4::MeasurementSignalBlock& stRefBlock_, double dSecondOffset_) const
{
    using namespace rangecmp4;

    stDiffBlock_.bParityKnown = ExtractBitfield<bool, SIG_BLK_PARITY_FLAG_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stDiffBlock_.bHalfCycleAdded = ExtractBitfield<bool, SIG_BLK_HALF_CYCLE_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stDiffBlock_.fCNo = ExtractBitfield<float, SIG_BLK_CNO_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_) * SIG_BLK_CNO_SCALE_FACTOR;
    stDiffBlock_.ucLockTimeBitfield = ExtractBitfield<uint8_t, SIG_BLK_LOCK_TIME_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stDiffBlock_.ucPsrStdDev = ExtractBitfield<uint8_t, SIG_BLK_PSR_STDDEV_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stDiffBlock_.ucPhrStdDev = ExtractBitfield<uint8_t, SIG_BLK_ADR_STDDEV_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);

    auto iPsr = ExtractBitfield<int32_t, SIG_DBLK_PSR_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    HandleSignExtension<SIG_DBLK_PSR_SIGNEXT_MASK>(iPsr);
    auto iPhr = ExtractBitfield<int32_t, SIG_DBLK_PHR_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    HandleSignExtension<SIG_DBLK_PHR_SIGNEXT_MASK>(iPhr);
    auto iDoppler = ExtractBitfield<int32_t, DBLK_DOPPLER_BITS[Secondary]>(ppucData_, uiBytesLeft_, uiBitOffset_);
    HandleSignExtension<DBLK_DOPPLER_SIGNEXT_MASK[Secondary]>(iDoppler);

    stDiffBlock_.bValidPsr = iPsr != SIG_DBLK_INVALID_PSR;
    stDiffBlock_.dPsr = iPsr * SIG_BLK_PSR_SCALE_FACTOR + stRefBlock_.dPsr + stRefBlock_.dDoppler * dSecondOffset_;
    stDiffBlock_.bValidPhr = iPhr != SIG_DBLK_INVALID_PHR;
    stDiffBlock_.dPhr = iPhr * SIG_BLK_PHR_SCALE_FACTOR + stRefBlock_.dPhr + stRefBlock_.dDoppler * dSecondOffset_;
    stDiffBlock_.bValidDoppler = iDoppler != DBLK_INVALID_DOPPLER[Secondary];
    stDiffBlock_.dDoppler = iDoppler * SIG_BLK_DOPPLER_SCALE_FACTOR + stRefBlock_.dDoppler;
}

//------------------------------------------------------------------------------
void RangeDecompressor::PopulateNextRangeData(RangeData& stRangeData_, const rangecmp4::MeasurementSignalBlock& stBlock_,
                                              const MetaDataStruct& stMetaData_, const ChannelTrackingStatus& stCtStatus_, uint32_t uiPrn_,
                                              char cGlonassFrequencyNumber_)
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
    //! Note: LSD have been removed to reduce rounding errors in the range log.
    //!       ADR STD is only 3 decimal places and could round up causing values
    //!       to be greater than values in the table.
    //-----------------------------------------------------------------------
    constexpr std::array<double, 16> stdDevAdrScaling = {0.003, 0.005, 0.007, 0.009, 0.012, 0.016, 0.022, 0.029,
                                                         0.039, 0.052, 0.070, 0.093, 0.124, 0.166, 0.222, 0.222};

    stRangeData_.usPrn = stCtStatus_.CalculatePrn(uiPrn_);
    if (stRangeData_.usPrn == 0 && stCtStatus_.GetSystem() != SYSTEM::GLONASS) { throw std::runtime_error("PRN outside of limits"); } // TODO: GLONASS

    const double dSignalWavelength = stCtStatus_.GetSignalWavelength(cGlonassFrequencyNumber_);

    // Any fields flagged as invalid are set to NaN and appear in the log as such.
    stRangeData_.sGlonassFrequency = static_cast<unsigned char>(cGlonassFrequencyNumber_);
    stRangeData_.dPseudorange = stBlock_.bValidPsr ? stBlock_.dPsr : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fPseudorangeStdDev = stdDevPsrScaling[stBlock_.ucPsrStdDev];
    stRangeData_.dAdr = stBlock_.bValidPhr ? -stBlock_.dPhr / dSignalWavelength : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fAdrStdDev = stdDevAdrScaling[stBlock_.ucPhrStdDev];
    stRangeData_.fDopplerFrequency = stBlock_.bValidDoppler ? -stBlock_.dDoppler / dSignalWavelength : std::numeric_limits<float>::quiet_NaN();
    stRangeData_.fCNo = stBlock_.fCNo;
    stRangeData_.fLockTime =
        GetRangeCmp4LockTime(stMetaData_, stBlock_.ucLockTimeBitfield, stCtStatus_.MakeKey(uiPrn_, stMetaData_.eMeasurementSource));
    stRangeData_.uiChannelTrackingStatus = stCtStatus_.GetAsWord();
}

//------------------------------------------------------------------------------
void RangeDecompressor::PopulateNextRangeData(RangeData& stRangeData_, const rangecmp5::MeasurementSignalBlock& stBlock_,
                                              const MetaDataStruct& stMetaData_, const ChannelTrackingStatus& stCtStatus_, uint32_t uiPrn_,
                                              char cGlonassFrequencyNumber_)
{
    //-----------------------------------------------------------------------
    //! List of pre-defined doubles used as translations for RANGECMP5 PSR
    //! standard deviation values defined in the RANGECMP5 documentation:
    //! TODO: link to documentation
    //-----------------------------------------------------------------------
    constexpr std::array<double, 32> stdDevPsrScaling = {0.020,  0.030,  0.045,  0.066,  0.099,  0.148,   0.220,   0.329,   0.491,   0.732,  1.092,
                                                         1.629,  2.430,  3.625,  5.409,  6.876,  8.741,   11.111,  14.125,  17.957,  22.828, 29.020,
                                                         36.891, 46.898, 59.619, 75.791, 96.349, 122.484, 155.707, 197.943, 251.634, 251.634};

    //-----------------------------------------------------------------------
    //! List of pre-defined doubles used as translations for RANGECMP5 PHR
    //! standard deviation values defined in the RANGECMP5 documentation:
    //! TODO: link to documentation
    //-----------------------------------------------------------------------
    constexpr std::array<double, 32> stdDevPhrScaling = {0.00391, 0.00458, 0.00536, 0.00628, 0.00735, 0.00861, 0.01001, 0.1182,
                                                         0.01385, 0.01621, 0.1900,  0.02223, 0.02607, 0.03054, 0.03577, 0.04190,
                                                         0.04908, 0.05749, 0.06734, 0.07889, 0.09240, 0.10824, 0.12679, 0.14851,
                                                         0.17396, 0.20378, 0.23870, 0.27961, 0.32753, 0.38366, 0.44940, 0.44940};

    stRangeData_.usPrn = stCtStatus_.CalculatePrn(uiPrn_);
    if (stRangeData_.usPrn == 0 && stCtStatus_.GetSystem() != SYSTEM::GLONASS) { throw std::runtime_error("PRN outside of limits"); } // TODO: GLONASS

    const double dSignalWavelength = stCtStatus_.GetSignalWavelength(cGlonassFrequencyNumber_);

    // Any fields flagged as invalid are set to NaN and appear in the log as such.
    stRangeData_.sGlonassFrequency = static_cast<unsigned char>(cGlonassFrequencyNumber_);
    stRangeData_.dPseudorange = stBlock_.bValidPsr ? stBlock_.dPsr : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fPseudorangeStdDev = stdDevPsrScaling[stBlock_.ucPseudorangeStdDev];
    stRangeData_.dAdr = stBlock_.bValidPhr ? -stBlock_.dPhr / dSignalWavelength : std::numeric_limits<double>::quiet_NaN();
    stRangeData_.fAdrStdDev = stdDevPhrScaling[stBlock_.ucPhrStdDev]; // TODO: how do we convert phase range std dev to adr std dev?
    stRangeData_.fDopplerFrequency = stBlock_.bValidDoppler ? -stBlock_.dDoppler / dSignalWavelength : std::numeric_limits<float>::quiet_NaN();
    stRangeData_.fCNo = stBlock_.fCNo;
    stRangeData_.fLockTime =
        GetRangeCmp4LockTime(stMetaData_, stBlock_.ucLockTimeBitfield, stCtStatus_.MakeKey(uiPrn_, stMetaData_.eMeasurementSource));
    stRangeData_.uiChannelTrackingStatus = stCtStatus_.GetAsWord();
}

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
        ChannelTrackingStatus stCtStatus(stRangeCmpData.uiChannelTrackingStatus);

        stRangeData.uiChannelTrackingStatus = stRangeCmpData.uiChannelTrackingStatus;
        stRangeData.usPrn = stRangeCmpData.ucPrn;

        auto iDoppler = GetBitfield<int32_t, DOPPLER_FREQUENCY_MASK>(stRangeCmpData.ulDopplerFrequencyPsrField);
        HandleSignExtension<DOPPLER_FREQUENCY_SIGNEXT_MASK>(iDoppler);
        stRangeData.fDopplerFrequency = iDoppler / DOPPLER_FREQUENCY_SCALE_FACTOR;

        stRangeData.dPseudorange =
            GetBitfield<uint64_t, PSR_MEASUREMENT_MASK>(stRangeCmpData.ulDopplerFrequencyPsrField) / PSR_MEASUREMENT_SCALE_FACTOR;
        stRangeData.fPseudorangeStdDev = stdDevPsrScaling[stRangeCmpData.ucStdDevPsrAdr & PSR_STDDEV_MASK];
        stRangeData.fAdrStdDev =
            (GetBitfield<uint32_t, ADR_STDDEV_MASK>(stRangeCmpData.ucStdDevPsrAdr) + ADR_STDDEV_SCALE_OFFSET) / ADR_STDDEV_SCALE_FACTOR;
        stRangeData.fLockTime = GetBitfield<uint32_t, LOCK_TIME_MASK>(stRangeCmpData.uiLockTimeCNoGloFreq) / LOCK_TIME_SCALE_FACTOR;
        stRangeData.fCNo = GetBitfield<uint32_t, CNO_MASK>(stRangeCmpData.uiLockTimeCNoGloFreq) + CNO_SCALE_OFFSET;
        stRangeData.sGlonassFrequency = GetBitfield<int16_t, GLONASS_FREQUENCY_MASK>(stRangeCmpData.uiLockTimeCNoGloFreq);

        double dWavelength = stCtStatus.GetSignalWavelength(stRangeData.sGlonassFrequency + GLONASS_FREQUENCY_NUMBER_OFFSET);
        stRangeData.dAdr = stRangeCmpData.uiAdr / ADR_SCALE_FACTOR;
        double dAdrRolls = (stRangeData.dPseudorange / dWavelength + stRangeData.dAdr) / MAX_VALUE;
        stRangeData.dAdr -= MAX_VALUE * static_cast<uint64_t>(std::round(dAdrRolls));
    }
}

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

        auto iPsrBase = GetBitfield<int32_t, SAT_SATELLITE_PSR_BASE_MASK>(stSatBlock.ulCombinedField);
        HandleSignExtension<SAT_SATELLITE_PSR_BASE_SIGNEXT_MASK>(iPsrBase);
        auto iDopplerBase = GetBitfield<int32_t, SAT_SATELLITE_DOPPLER_BASE_MASK>(stSatBlock.ulCombinedField);
        HandleSignExtension<SAT_SATELLITE_DOPPLER_BASE_SIGNEXT_MASK>(iDopplerBase);

        uiRangeDataBytesDecompressed += sizeof(SatelliteBlock);

        // Decompress the Signal Blocks associated with the Satellite Block
        for (uint8_t ucSignalBlockIndex = 0; ucSignalBlockIndex < ucSignalBlockCount; ucSignalBlockIndex++)
        {
            // Decompress the signal block
            const auto& stSigBlock = reinterpret_cast<const SignalBlock&>(stRangeCmpMessage_.aucRangeData[uiRangeDataBytesDecompressed]);

            const auto eSignalType = GetBitfield<SIGNAL_TYPE, SIG_SIGNAL_TYPE_MASK>(stSigBlock.uiCombinedField1);
            const auto ucPsrBitfield = GetBitfield<uint8_t, SIG_PSR_STDDEV_MASK>(stSigBlock.ullCombinedField2);
            const auto ucAdrBitfield = GetBitfield<uint8_t, SIG_ADR_STDDEV_MASK>(stSigBlock.ullCombinedField2);
            const auto uiLockTimeBits = GetBitfield<uint32_t, SIG_LOCKTIME_MASK>(stSigBlock.uiCombinedField1);
            const auto usPrn = stSatBlock.ucSatelliteIdentifier + (eSystem == SYSTEM::GLONASS ? GLONASS_SLOT_OFFSET - 1 : 0);

            auto iDopplerBitfield = GetBitfield<int32_t, SIG_DOPPLER_DIFF_MASK>(stSigBlock.ullCombinedField2);
            HandleSignExtension<SIG_DOPPLER_DIFF_SIGNEXT_MASK>(iDopplerBitfield);

            ChannelTrackingStatus stCtStatus(stSatBlock, stSigBlock);

            // Construct the decompressed range data
            RangeData& stRangeData = stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++];
            stRangeData.usPrn = usPrn;
            stRangeData.sGlonassFrequency = GetBitfield<int16_t, SAT_GLONASS_FREQUENCY_ID_MASK>(stSatBlock.ulCombinedField);
            stRangeData.dPseudorange = iPsrBase + GetBitfield<uint64_t, SIG_PSR_DIFF_MASK>(stSigBlock.ullCombinedField2) / SIG_PSR_DIFF_SCALE_FACTOR;
            stRangeData.fPseudorangeStdDev = stdDevPsrScaling[ucPsrBitfield];
            stRangeData.dAdr = -((iPsrBase + GetBitfield<uint64_t, SIG_PHR_DIFF_MASK>(stSigBlock.ullCombinedField2) / SIG_PHR_DIFF_SCALE_FACTOR) /
                                 stCtStatus.GetSignalWavelength(stRangeData.sGlonassFrequency));
            stRangeData.fAdrStdDev = stdDevAdrScaling[ucAdrBitfield];
            stRangeData.fDopplerFrequency = (iDopplerBase + iDopplerBitfield / SIG_DOPPLER_DIFF_SCALE_FACTOR) / SignalScaling(eSystem, eSignalType);
            stRangeData.fCNo = SIG_CNO_SCALE_OFFSET + GetBitfield<uint64_t, SIG_CNO_MASK>(stSigBlock.ullCombinedField2);
            stRangeData.fLockTime = GetRangeCmp2LockTime(stMetaData_, uiLockTimeBits, stCtStatus.MakeKey(usPrn, stMetaData_.eMeasurementSource));
            stRangeData.uiChannelTrackingStatus = stCtStatus.GetAsWord();

            uiRangeDataBytesDecompressed += sizeof(SignalBlock);
        }
    }
}

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

    auto systems = ExtractBitfield<uint16_t, SATELLITE_SYSTEMS_BITS>(&pucData_, uiBytesLeft, uiBitOffset);

    while (systems)
    {
        auto system = static_cast<SYSTEM>(PopLsb(systems));

        // WARNING: We use arrays instead of vectors for PRNs and Signals to avoid using dynamic memory allocation.
        // This means that we have to be careful using the size of the array and iterators.
        auto satellitesTemp = ExtractBitfield<uint64_t, SATELLITES_BITS>(&pucData_, uiBytesLeft, uiBitOffset);
        std::array<uint32_t, SATELLITES_BITS> aPrns;
        uint32_t uiPrnCount = 0;
        while (satellitesTemp) { aPrns[uiPrnCount++] = PopLsb(satellitesTemp) + 1; } // Bit position is PRN - 1, so + 1 here

        auto signalsTemp = ExtractBitfield<uint64_t, SIGNALS_BITS>(&pucData_, uiBytesLeft, uiBitOffset);
        std::array<SIGNAL_TYPE, SIGNALS_BITS> aSignals;
        uint32_t uiSignalCount = 0;
        while (signalsTemp) { aSignals[uiSignalCount++] = static_cast<SIGNAL_TYPE>(PopLsb(signalsTemp)); }

        std::array<uint16_t, SATELLITES_BITS> includedSignals;
        // Iterate through the PRNs once to collect the signals tracked by each. We need this info before we can start decompressing.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Get the m*n bit matrix that describes the included signals in this RANGECMP4 message.
            includedSignals[aPrns[uiPrnIndex]] = ExtractBitfield<uint16_t>(&pucData_, uiBytesLeft, uiBitOffset, uiSignalCount);
        }

        // Check each PRN against the signals tracked in this satellite system to see if the signal is included.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Begin decoding Reference Measurement Block Header.
            MeasurementBlockHeader stMbHeader;
            stMbHeader.bIsDifferentialData = ExtractBitfield<bool, MBLK_HDR_DATAFORMAT_FLAG_BITS>(&pucData_, uiBytesLeft, uiBitOffset);
            stMbHeader.ucReferenceDataBlockID = ExtractBitfield<uint8_t, MBLK_HDR_REFERENCE_DATABLOCK_ID_BITS>(&pucData_, uiBytesLeft, uiBitOffset);

            // This field is only present for GLONASS and reference blocks.
            if (system == SYSTEM::GLONASS && !stMbHeader.bIsDifferentialData)
            {
                stMbHeader.cGlonassFrequencyNumber =
                    ExtractBitfield<int8_t, MBLK_HDR_GLONASS_FREQUENCY_NUMBER_BITS>(&pucData_, uiBytesLeft, uiBitOffset);
            }

            const uint32_t& prn = aPrns[uiPrnIndex];
            const uint32_t uiIncludedSignalCount = PopCount(includedSignals[prn]);
            bool bPrimaryBlock = true;
            double primaryPseudorange{};
            double primaryDoppler{};

            while (includedSignals[prn])
            {
                const SIGNAL_TYPE signal = aSignals[PopLsb(includedSignals[prn])];
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

                            ChannelTrackingStatus stCtStatus(system, signal, stBlock);
                            PopulateNextRangeData(stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++], stBlock, stMetaData_,
                                                  stCtStatus, prn, stRb.first.cGlonassFrequencyNumber);
                        }
                        else
                        {
                            pclMyLogger->warn("Invalid reference data: Diff ID {} != Ref ID {}", stMbHeader.ucReferenceDataBlockID,
                                              stRb.first.ucReferenceDataBlockID);
                        }
                    }
                    catch (...)
                    {
                        pclMyLogger->warn("No reference data exists for SYSTEM {}, SIGNAL {}, PRN {}, ID {}", static_cast<int32_t>(system),
                                          static_cast<int32_t>(signal), prn, stMbHeader.ucReferenceDataBlockID);
                    }
                }
                else // This is a reference block.
                {
                    if (bPrimaryBlock)
                    {
                        DecompressReferenceBlock<false>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler);
                        primaryPseudorange = stBlock.dPsr;
                        primaryDoppler = stBlock.dDoppler;
                        bPrimaryBlock = false;
                    }
                    else { DecompressReferenceBlock<true>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler); }

                    ChannelTrackingStatus stCtStatus(system, signal, stBlock);
                    PopulateNextRangeData(stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++], stBlock, stMetaData_, stCtStatus,
                                          prn, stMbHeader.cGlonassFrequencyNumber);

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
template <bool Secondary>
void RangeDecompressor::DecompressBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                        rangecmp5::MeasurementSignalBlock& stBlock_, double primaryPsr, double primaryDoppler) const
{
    using namespace rangecmp5;

    stBlock_.bParityKnown = ExtractBitfield<bool, SIG_BLK_PARITY_FLAG_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stBlock_.bHalfCycleAdded = ExtractBitfield<bool, SIG_BLK_HALF_CYCLE_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stBlock_.fCNo = ExtractBitfield<float, SIG_BLK_CNO_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_) * SIG_BLK_CNO_SCALE_FACTOR;
    stBlock_.ucLockTimeBitfield = ExtractBitfield<uint8_t, SIG_BLK_LOCK_TIME_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stBlock_.ucPseudorangeStdDev = ExtractBitfield<uint8_t, SIG_BLK_PSR_STDDEV_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    stBlock_.ucPhrStdDev = ExtractBitfield<uint8_t, SIG_BLK_ADR_STDDEV_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);

    auto llPsr = ExtractBitfield<int64_t, RBLK_PSR_BITS[Secondary]>(ppucData_, uiBytesLeft_, uiBitOffset_);
    if constexpr (Secondary) { HandleSignExtension<SSIG_RBLK_PSR_SIGNEXT_MASK>(llPsr); }
    auto iPhr = ExtractBitfield<int32_t, RBLK_PHR_BITS>(ppucData_, uiBytesLeft_, uiBitOffset_);
    HandleSignExtension<RBLK_PHR_SIGNEXT_MASK>(iPhr);
    auto iDopplerBitfield = ExtractBitfield<int32_t, RBLK_DOPPLER_BITS[Secondary]>(ppucData_, uiBytesLeft_, uiBitOffset_);
    HandleSignExtension<RBLK_DOPPLER_SIGNEXT_MASK[Secondary]>(iDopplerBitfield);

    stBlock_.bValidPsr = llPsr != RBLK_INVALID_PSR[Secondary];
    stBlock_.dPsr = llPsr * SIG_BLK_PSR_SCALE_FACTOR + (Secondary ? primaryPsr : 0);
    stBlock_.bValidPhr = iPhr != SIG_RBLK_INVALID_PHR;
    stBlock_.dPhr = iPhr * SIG_BLK_PHR_SCALE_FACTOR + (Secondary ? primaryPsr : stBlock_.dPsr);
    stBlock_.bValidDoppler = iDopplerBitfield != PSIG_RBLK_INVALID_DOPPLER;
    stBlock_.dDoppler = iDopplerBitfield * SIG_BLK_DOPPLER_SCALE_FACTOR + (Secondary ? primaryDoppler : 0);
}

//------------------------------------------------------------------------------
void RangeDecompressor::RangeCmp5ToRange(unsigned char* pucData_, Range& stRangeMessage_, const MetaDataStruct& stMetaData_)
{
    using namespace rangecmp5;

    stRangeMessage_.uiNumberOfObservations = 0;
    uint32_t uiBitOffset = 0;
    uint32_t uiBytesLeft = *reinterpret_cast<uint32_t*>(pucData_);
    pucData_ += sizeof(uint32_t);

    auto systems = ExtractBitfield<uint16_t, SATELLITE_SYSTEMS_BITS>(&pucData_, uiBytesLeft, uiBitOffset);

    while (systems)
    {
        auto system = static_cast<SYSTEM>(PopLsb(systems));

        // WARNING: We use arrays instead of vectors for PRNs and Signals to avoid using dynamic memory allocation.
        // This means that we have to be careful using the size of the array and iterators.
        auto satellitesTemp = ExtractBitfield<uint64_t, SATELLITES_BITS>(&pucData_, uiBytesLeft, uiBitOffset);
        std::array<uint32_t, SATELLITES_BITS> aPrns;
        uint32_t uiPrnCount = 0;
        while (satellitesTemp) { aPrns[uiPrnCount++] = PopLsb(satellitesTemp) + 1; } // Bit position is PRN - 1, so + 1 here

        auto signalsTemp = ExtractBitfield<uint64_t, SIGNALS_BITS>(&pucData_, uiBytesLeft, uiBitOffset);
        std::array<rangecmp4::SIGNAL_TYPE, SIGNALS_BITS> aSignals;
        uint32_t uiSignalCount = 0;
        while (signalsTemp) { aSignals[uiSignalCount++] = static_cast<rangecmp4::SIGNAL_TYPE>(PopLsb(signalsTemp)); }

        std::array<uint16_t, SATELLITES_BITS> includedSignals;
        // Iterate through the PRNs once to collect the signals tracked by each. We need this info before we can start decompressing.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Get the m*n bit matrix that describes the included signals in this RANGECMP5 message.
            includedSignals[aPrns[uiPrnIndex]] = ExtractBitfield<uint16_t>(&pucData_, uiBytesLeft, uiBitOffset, uiSignalCount);
        }

        // Check each PRN against the signals tracked in this satellite system to see if the signal is included.
        for (uint32_t uiPrnIndex = 0; uiPrnIndex < uiPrnCount; ++uiPrnIndex)
        {
            // Begin decoding Reference Measurement Block Header.
            MeasurementBlockHeader stMbHeader;
            stMbHeader.bDataFormatFlag = ExtractBitfield<bool, 1>(&pucData_, uiBytesLeft, uiBitOffset);
            stMbHeader.ucReserved = ExtractBitfield<uint8_t, 3>(&pucData_, uiBytesLeft, uiBitOffset);

            // This field is only present for GLONASS and reference blocks.
            if (system == SYSTEM::GLONASS) { stMbHeader.cGlonassFrequencyNumber = ExtractBitfield<int8_t, 5>(&pucData_, uiBytesLeft, uiBitOffset); }

            const uint32_t& prn = aPrns[uiPrnIndex];
            const uint32_t uiIncludedSignalCount = PopCount(includedSignals[prn]);
            bool bPrimaryBlock = true;
            double primaryPseudorange{};
            double primaryDoppler{};

            while (includedSignals[prn])
            {
                const rangecmp4::SIGNAL_TYPE signal = aSignals[PopLsb(includedSignals[prn])];
                MeasurementSignalBlock stBlock;

                if (bPrimaryBlock)
                {
                    DecompressBlock<false>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler);
                    primaryPseudorange = stBlock.dPsr;
                    primaryDoppler = stBlock.dDoppler;
                    bPrimaryBlock = false;
                }
                else { DecompressBlock<true>(&pucData_, uiBytesLeft, uiBitOffset, stBlock, primaryPseudorange, primaryDoppler); }

                ChannelTrackingStatus stCtStatus(system, signal, stBlock);
                PopulateNextRangeData(stRangeMessage_.astRangeData[stRangeMessage_.uiNumberOfObservations++], stBlock, stMetaData_, stCtStatus, prn,
                                      stMbHeader.cGlonassFrequencyNumber);
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
STATUS RangeDecompressor::Decompress(unsigned char* pucBuffer_, uint32_t uiBufferSize_, MetaDataStruct& stMetaData_, ENCODE_FORMAT eFormat_)
{
    if (pucBuffer_ == nullptr) { return STATUS::NULL_PROVIDED; }

    if (pclMyMsgDB == nullptr) { return STATUS::NO_DATABASE; }

    MessageDataStruct stMessageData;
    IntermediateHeader stHeader;
    std::vector<FieldContainer> stMessage;

    unsigned char* pucTempMessagePointer = pucBuffer_;
    STATUS eStatus = clMyHeaderDecoder.Decode(pucTempMessagePointer, stHeader, stMetaData_);
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

    // Convert the RANGECMP* message to a RANGE message.
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

    // The message should be returned in its original format.
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
