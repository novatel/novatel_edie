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
// ! \file channel_tracking_status.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/rangecmp/channel_tracking_status.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

//------------------------------------------------------------------------------
uint64_t ChannelTrackingStatus::MakeKey(const uint32_t prn, const MEASUREMENT_SOURCE source) const
{
    assert(static_cast<uint64_t>(eSatelliteSystem) < 16 && prn < 256 && static_cast<uint64_t>(eSignalType) < 32 && static_cast<uint64_t>(source) < 2);
    return (static_cast<uint64_t>(eSatelliteSystem) << 14) | (prn << 6) | static_cast<uint64_t>(eSignalType) << 1 | static_cast<uint64_t>(source);
}

//------------------------------------------------------------------------------
SYSTEM ChannelTrackingStatus::GetSystem() const
{
    switch (eSatelliteSystem)
    {
    case CTS_SYSTEM::GPS: return SYSTEM::GPS;
    case CTS_SYSTEM::GLONASS: return SYSTEM::GLONASS;
    case CTS_SYSTEM::SBAS: return SYSTEM::SBAS;
    case CTS_SYSTEM::GALILEO: return SYSTEM::GALILEO;
    case CTS_SYSTEM::BEIDOU: return SYSTEM::BEIDOU;
    case CTS_SYSTEM::QZSS: return SYSTEM::QZSS;
    case CTS_SYSTEM::NAVIC: return SYSTEM::NAVIC;
    default: return SYSTEM::UNKNOWN;
    }
}

//------------------------------------------------------------------------------
ChannelTrackingStatus::ChannelTrackingStatus(const uint32_t uiChannelTrackingStatus_)
{
    eTrackingState = GetBitfield<TRACKING_STATE, CTS_TRACKING_STATE_MASK>(uiChannelTrackingStatus_);
    uiSVChannelNumber = GetBitfield<uint32_t, CTS_SV_CHANNEL_NUMBER_MASK>(uiChannelTrackingStatus_);
    eCorrelatorType = GetBitfield<CORRELATOR_TYPE, CTS_CORRELATOR_MASK>(uiChannelTrackingStatus_);
    eSatelliteSystem = GetBitfield<CTS_SYSTEM, CTS_SATELLITE_SYSTEM_MASK>(uiChannelTrackingStatus_);
    eSignalType = GetBitfield<CTS_SIGNAL, CTS_SIGNAL_TYPE_MASK>(uiChannelTrackingStatus_);
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

//------------------------------------------------------------------------------
ChannelTrackingStatus::ChannelTrackingStatus(const rangecmp2::SatelliteBlock& stSatelliteBlock_, const rangecmp2::SignalBlock& stSignalBlock_)
{
    using namespace rangecmp2;

    bGrouped = GetBitfield<uint64_t, SAT_NUM_SIGNAL_BLOCKS_BASE_MASK>(stSatelliteBlock_.ulCombinedField) > 1;
    bPhaseLocked = GetBitfield<bool, SIG_PHASE_LOCK_MASK>(stSignalBlock_.uiCombinedField1);
    bParityKnown = GetBitfield<bool, SIG_PARITY_KNOWN_MASK>(stSignalBlock_.uiCombinedField1);
    bCodeLocked = GetBitfield<bool, SIG_CODE_LOCK_MASK>(stSignalBlock_.uiCombinedField1);
    bPrimaryL1Channel = GetBitfield<bool, SIG_PRIMARY_SIGNAL_MASK>(stSignalBlock_.uiCombinedField1);
    bHalfCycleAdded = GetBitfield<bool, SIG_CARRIER_PHASE_MEAS_MASK>(stSignalBlock_.uiCombinedField1);
    bDigitalFilteringOnSignal = false;
    bChannelAssignmentForced = false;
    bPRNLocked = false;
    uiSVChannelNumber = static_cast<uint32_t>(stSatelliteBlock_.ucSvChanNumber);
    eTrackingState = bPrimaryL1Channel ? TRACKING_STATE::PHASE_LOCK_LOOP : TRACKING_STATE::AIDED_PHASE_LOCK_LOOP;
    eCorrelatorType = GetBitfield<CORRELATOR_TYPE, SIG_CORRELATOR_TYPE_MASK>(stSignalBlock_.uiCombinedField1);
    eSatelliteSystem = SystemToSatelliteSystem(GetBitfield<SYSTEM, SAT_SATELLITE_SYSTEM_ID_MASK>(stSatelliteBlock_.ulCombinedField));
    eSignalType = RangeCmp2SignalTypeToSignalType(eSatelliteSystem, GetBitfield<SIGNAL_TYPE, SIG_SIGNAL_TYPE_MASK>(stSignalBlock_.uiCombinedField1));
}

//------------------------------------------------------------------------------
ChannelTrackingStatus::ChannelTrackingStatus(const SYSTEM eSystem_, const rangecmp4::SIGNAL_TYPE eSignalType_,
                                             const rangecmp4::MeasurementSignalBlock& stMeasurementBlock_)
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
        (eSatelliteSystem == CTS_SYSTEM::BEIDOU && eSignalType_ == rangecmp4::SIGNAL_TYPE::BEIDOU_B1GEO))
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

//------------------------------------------------------------------------------
ChannelTrackingStatus::ChannelTrackingStatus(const SYSTEM eSystem_, const rangecmp4::SIGNAL_TYPE eSignalType_,
                                             const rangecmp5::MeasurementSignalBlock& stMeasurementBlock_)
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
        (eSatelliteSystem == CTS_SYSTEM::BEIDOU && eSignalType_ == rangecmp4::SIGNAL_TYPE::BEIDOU_B1GEO))
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

//------------------------------------------------------------------------------
ChannelTrackingStatus::CTS_SIGNAL ChannelTrackingStatus::RangeCmp2SignalTypeToSignalType(const CTS_SYSTEM eSystem_,
                                                                                         const rangecmp2::SIGNAL_TYPE eSignalType_)
{
    using namespace rangecmp2;

    switch (eSystem_)
    {
    case CTS_SYSTEM::GPS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::GPS_L1CA: return CTS_SIGNAL::GPS_L1CA;
        case SIGNAL_TYPE::GPS_L2Y: return CTS_SIGNAL::GPS_L2Y;
        case SIGNAL_TYPE::GPS_L2CM: return CTS_SIGNAL::GPS_L2CM;
        case SIGNAL_TYPE::GPS_L2P: return CTS_SIGNAL::GPS_L2P;
        case SIGNAL_TYPE::GPS_L5Q: return CTS_SIGNAL::GPS_L5Q;
        case SIGNAL_TYPE::GPS_L1C: return CTS_SIGNAL::GPS_L1CP;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::GLONASS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::GLONASS_L1CA: return CTS_SIGNAL::GLONASS_L1CA;
        case SIGNAL_TYPE::GLONASS_L2CA: return CTS_SIGNAL::GLONASS_L2CA;
        case SIGNAL_TYPE::GLONASS_L2P: return CTS_SIGNAL::GLONASS_L2P;
        case SIGNAL_TYPE::GLONASS_L3Q: return CTS_SIGNAL::GLONASS_L3Q;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::BEIDOU:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::BEIDOU_B1D1I: return CTS_SIGNAL::BEIDOU_B1ID1;
        case SIGNAL_TYPE::BEIDOU_B1D2I: return CTS_SIGNAL::BEIDOU_B1ID2;
        case SIGNAL_TYPE::BEIDOU_B2D1I: return CTS_SIGNAL::BEIDOU_B2ID1;
        case SIGNAL_TYPE::BEIDOU_B2D2I: return CTS_SIGNAL::BEIDOU_B2ID2;
        case SIGNAL_TYPE::BEIDOU_B3D1I: return CTS_SIGNAL::BEIDOU_B3ID1;
        case SIGNAL_TYPE::BEIDOU_B3D2I: return CTS_SIGNAL::BEIDOU_B3ID2;
        case SIGNAL_TYPE::BEIDOU_B1CP: return CTS_SIGNAL::BEIDOU_B1CP;
        case SIGNAL_TYPE::BEIDOU_B2AP: return CTS_SIGNAL::BEIDOU_B2AP;
        case SIGNAL_TYPE::BEIDOU_B2B_I: return CTS_SIGNAL::BEIDOU_B2BI;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::GALILEO:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::GALILEO_E1C: return CTS_SIGNAL::GALILEO_E1C;
        case SIGNAL_TYPE::GALILEO_E5AQ: return CTS_SIGNAL::GALILEO_E5AQ;
        case SIGNAL_TYPE::GALILEO_E5BQ: return CTS_SIGNAL::GALILEO_E5BQ;
        case SIGNAL_TYPE::GALILEO_ALTBOCQ: return CTS_SIGNAL::GALILEO_E5ALTBOCQ;
        case SIGNAL_TYPE::GALILEO_E6C: return CTS_SIGNAL::GALILEO_E6C;
        case SIGNAL_TYPE::GALILEO_E6B: return CTS_SIGNAL::GALILEO_E6B;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::QZSS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::QZSS_L1CA: return CTS_SIGNAL::QZSS_L1CA;
        case SIGNAL_TYPE::QZSS_L2CM: return CTS_SIGNAL::QZSS_L2CM;
        case SIGNAL_TYPE::QZSS_L5Q: return CTS_SIGNAL::QZSS_L5Q;
        case SIGNAL_TYPE::QZSS_L1C: return CTS_SIGNAL::QZSS_L1CP;
        case SIGNAL_TYPE::QZSS_L6P: return CTS_SIGNAL::QZSS_L6P;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::SBAS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::SBAS_L1CA: return CTS_SIGNAL::SBAS_L1CA;
        case SIGNAL_TYPE::SBAS_L5I: return CTS_SIGNAL::SBAS_L5I;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::NAVIC:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::NAVIC_L5SPS: return CTS_SIGNAL::NAVIC_L5SPS;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::OTHER:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::LBAND: return CTS_SIGNAL::LBAND;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    default: return CTS_SIGNAL::UNKNOWN;
    }
}

//------------------------------------------------------------------------------
ChannelTrackingStatus::CTS_SIGNAL ChannelTrackingStatus::RangeCmp4SignalTypeToSignalType(const CTS_SYSTEM eSystem_,
                                                                                         const rangecmp4::SIGNAL_TYPE eSignalType_)
{
    using namespace rangecmp4;

    switch (eSystem_)
    {
    case CTS_SYSTEM::GPS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::GPS_L1CA: return CTS_SIGNAL::GPS_L1CA;
        case SIGNAL_TYPE::GPS_L2Y: return CTS_SIGNAL::GPS_L2Y;
        case SIGNAL_TYPE::GPS_L2C: return CTS_SIGNAL::GPS_L2CM;
        case SIGNAL_TYPE::GPS_L2P: return CTS_SIGNAL::GPS_L2P;
        case SIGNAL_TYPE::GPS_L5Q: return CTS_SIGNAL::GPS_L5Q;
        case SIGNAL_TYPE::GPS_L1C: return CTS_SIGNAL::GPS_L1CP;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::GLONASS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::GLONASS_L1CA: return CTS_SIGNAL::GLONASS_L1CA;
        case SIGNAL_TYPE::GLONASS_L2CA: return CTS_SIGNAL::GLONASS_L2CA;
        case SIGNAL_TYPE::GLONASS_L2P: return CTS_SIGNAL::GLONASS_L2P;
        case SIGNAL_TYPE::GLONASS_L3: return CTS_SIGNAL::GLONASS_L3Q;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::BEIDOU:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::BEIDOU_B1I: return CTS_SIGNAL::BEIDOU_B1ID1;
        case SIGNAL_TYPE::BEIDOU_B1GEO: return CTS_SIGNAL::BEIDOU_B1ID2;
        case SIGNAL_TYPE::BEIDOU_B2I: return CTS_SIGNAL::BEIDOU_B2ID1;
        case SIGNAL_TYPE::BEIDOU_B2GEO: return CTS_SIGNAL::BEIDOU_B2ID2;
        case SIGNAL_TYPE::BEIDOU_B3I: return CTS_SIGNAL::BEIDOU_B3ID1;
        case SIGNAL_TYPE::BEIDOU_B3GEO: return CTS_SIGNAL::BEIDOU_B3ID2;
        case SIGNAL_TYPE::BEIDOU_B1CP: return CTS_SIGNAL::BEIDOU_B1CP;
        case SIGNAL_TYPE::BEIDOU_B2AP: return CTS_SIGNAL::BEIDOU_B2AP;
        case SIGNAL_TYPE::BEIDOU_B2BI: return CTS_SIGNAL::BEIDOU_B2BI;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::GALILEO:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::GALILEO_E1: return CTS_SIGNAL::GALILEO_E1C;
        case SIGNAL_TYPE::GALILEO_E5A: return CTS_SIGNAL::GALILEO_E5AQ;
        case SIGNAL_TYPE::GALILEO_E5B: return CTS_SIGNAL::GALILEO_E5BQ;
        case SIGNAL_TYPE::GALILEO_ALTBOC: return CTS_SIGNAL::GALILEO_E5ALTBOCQ;
        case SIGNAL_TYPE::GALILEO_E6C: return CTS_SIGNAL::GALILEO_E6C;
        case SIGNAL_TYPE::GALILEO_E6B: return CTS_SIGNAL::GALILEO_E6B;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::QZSS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::QZSS_L1CA: return CTS_SIGNAL::QZSS_L1CA;
        case SIGNAL_TYPE::QZSS_L2C: return CTS_SIGNAL::QZSS_L2CM;
        case SIGNAL_TYPE::QZSS_L5Q: return CTS_SIGNAL::QZSS_L5Q;
        case SIGNAL_TYPE::QZSS_L1C: return CTS_SIGNAL::QZSS_L1CP;
        case SIGNAL_TYPE::QZSS_L6P: return CTS_SIGNAL::QZSS_L6P;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::SBAS:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::SBAS_L1CA: return CTS_SIGNAL::SBAS_L1CA;
        case SIGNAL_TYPE::SBAS_L5I: return CTS_SIGNAL::SBAS_L5I;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    case CTS_SYSTEM::NAVIC:
        switch (eSignalType_)
        {
        case SIGNAL_TYPE::NAVIC_L5SPS: return CTS_SIGNAL::NAVIC_L5SPS;
        default: return CTS_SIGNAL::UNKNOWN;
        }
    default: return CTS_SIGNAL::UNKNOWN;
    }
}

//------------------------------------------------------------------------------
ChannelTrackingStatus::CTS_SYSTEM ChannelTrackingStatus::SystemToSatelliteSystem(const SYSTEM eSystem_)
{
    switch (eSystem_)
    {
    case SYSTEM::GPS: return CTS_SYSTEM::GPS;
    case SYSTEM::GLONASS: return CTS_SYSTEM::GLONASS;
    case SYSTEM::SBAS: return CTS_SYSTEM::SBAS;
    case SYSTEM::GALILEO: return CTS_SYSTEM::GALILEO;
    case SYSTEM::BEIDOU: return CTS_SYSTEM::BEIDOU;
    case SYSTEM::QZSS: return CTS_SYSTEM::QZSS;
    case SYSTEM::NAVIC: return CTS_SYSTEM::NAVIC;
    default: return CTS_SYSTEM::OTHER;
    }
}

//------------------------------------------------------------------------------
double ChannelTrackingStatus::GetSignalWavelength(const int16_t sGLONASSFrequency_) const
{
    constexpr double speedOfLight = 299792458.0;

    constexpr double frequencyHzGpsL1 = 1575420000.0;
    constexpr double frequencyHzGpsL2 = 1227600000.0;
    constexpr double frequencyHzGpsL5 = 1176450000.0;

    constexpr double frequencyHzGalE1 = frequencyHzGpsL1;
    constexpr double frequencyHzGalE5A = frequencyHzGpsL5;
    constexpr double frequencyHzGalE5B = 1207140000.0;
    constexpr double frequencyHzGalE6 = 1278750000.0;
    constexpr double frequencyHzGalAlt = 1191795000.0;

    constexpr double frequencyHzBdsB1 = 1561098000.0;
    constexpr double frequencyHzBdsB1C = frequencyHzGpsL1;
    constexpr double frequencyHzBdsB2 = frequencyHzGalE5B;
    constexpr double frequencyHzBdsB2A = frequencyHzGpsL5;
    constexpr double frequencyHzBdsB2B = frequencyHzBdsB2;
    constexpr double frequencyHzBdsB3 = 1268520000.0;

    constexpr double glonassL1FrequencyScaleHz = 562500.0;
    constexpr double glonassL2FrequencyScaleHz = 437500.0;

    constexpr double frequencyHzGloL1 = 1602000000.0;
    constexpr double frequencyHzGloL2 = 1246000000.0;
    constexpr double frequencyHzGloL3 = 1202025000.0;

    constexpr double frequencyHzQzssL1 = frequencyHzGpsL1;
    constexpr double frequencyHzQzssL2 = frequencyHzGpsL2;
    constexpr double frequencyHzQzssL5 = frequencyHzGpsL5;
    constexpr double frequencyHzQzssL6 = 1278750000.0;

    constexpr auto glonassL1LookupTable = [&] {
        std::array<double, 21> arr{};
        for (int32_t i = 0; i < static_cast<int32_t>(arr.size()); i++)
        {
            arr[i] = speedOfLight / (frequencyHzGloL1 + (i - static_cast<int32_t>(GLONASS_FREQUENCY_NUMBER_OFFSET)) * glonassL1FrequencyScaleHz);
        }
        return arr;
    }();

    constexpr auto glonassL2LookupTable = [&] {
        std::array<double, 21> arr{};
        for (int32_t i = 0; i < static_cast<int32_t>(arr.size()); i++)
        {
            arr[i] = speedOfLight / (frequencyHzGloL2 + (i - static_cast<int32_t>(GLONASS_FREQUENCY_NUMBER_OFFSET)) * glonassL2FrequencyScaleHz);
        }
        return arr;
    }();

    switch (eSatelliteSystem)
    {
    case CTS_SYSTEM::GPS:
        switch (eSignalType)
        {
        case CTS_SIGNAL::GPS_L1CA: [[fallthrough]];
        case CTS_SIGNAL::GPS_L1CP: return speedOfLight / frequencyHzGpsL1;
        case CTS_SIGNAL::GPS_L2P: [[fallthrough]];
        case CTS_SIGNAL::GPS_L2Y: [[fallthrough]];
        case CTS_SIGNAL::GPS_L2CM: return speedOfLight / frequencyHzGpsL2;
        case CTS_SIGNAL::GPS_L5Q: return speedOfLight / frequencyHzGpsL5;
        default: return 0.0;
        }
    case CTS_SYSTEM::GLONASS:
        switch (eSignalType)
        {
        case CTS_SIGNAL::GLONASS_L1CA: return glonassL1LookupTable.at(sGLONASSFrequency_);
        case CTS_SIGNAL::GLONASS_L2CA: [[fallthrough]];
        case CTS_SIGNAL::GLONASS_L2P: return glonassL2LookupTable.at(sGLONASSFrequency_);
        case CTS_SIGNAL::GLONASS_L3Q: return speedOfLight / frequencyHzGloL3;
        default: return 0.0;
        }
    case CTS_SYSTEM::SBAS:
        switch (eSignalType)
        {
        case CTS_SIGNAL::SBAS_L1CA: return speedOfLight / frequencyHzGpsL1;
        case CTS_SIGNAL::SBAS_L5I: return speedOfLight / frequencyHzGpsL5;
        default: return 0.0;
        }
    case CTS_SYSTEM::GALILEO:
        switch (eSignalType)
        {
        case CTS_SIGNAL::GALILEO_E1C: return speedOfLight / frequencyHzGalE1;
        case CTS_SIGNAL::GALILEO_E6B: [[fallthrough]];
        case CTS_SIGNAL::GALILEO_E6C: return speedOfLight / frequencyHzGalE6;
        case CTS_SIGNAL::GALILEO_E5AQ: return speedOfLight / frequencyHzGalE5A;
        case CTS_SIGNAL::GALILEO_E5BQ: return speedOfLight / frequencyHzGalE5B;
        case CTS_SIGNAL::GALILEO_E5ALTBOCQ: return speedOfLight / frequencyHzGalAlt;
        default: return 0.0;
        }
    case CTS_SYSTEM::BEIDOU:
        switch (eSignalType)
        {
        case CTS_SIGNAL::BEIDOU_B1ID1: [[fallthrough]];
        case CTS_SIGNAL::BEIDOU_B1ID2: return speedOfLight / frequencyHzBdsB1;
        case CTS_SIGNAL::BEIDOU_B2ID1: [[fallthrough]];
        case CTS_SIGNAL::BEIDOU_B2ID2: return speedOfLight / frequencyHzBdsB2;
        case CTS_SIGNAL::BEIDOU_B3ID1: [[fallthrough]];
        case CTS_SIGNAL::BEIDOU_B3ID2: return speedOfLight / frequencyHzBdsB3;
        case CTS_SIGNAL::BEIDOU_B1CP: return speedOfLight / frequencyHzBdsB1C;
        case CTS_SIGNAL::BEIDOU_B2AP: return speedOfLight / frequencyHzBdsB2A;
        case CTS_SIGNAL::BEIDOU_B2BI: return speedOfLight / frequencyHzBdsB2B;
        default: return 0.0;
        }
    case CTS_SYSTEM::QZSS:
        switch (eSignalType)
        {
        case CTS_SIGNAL::QZSS_L1CA: [[fallthrough]];
        case CTS_SIGNAL::QZSS_L1CP: return speedOfLight / frequencyHzQzssL1;
        case CTS_SIGNAL::QZSS_L2CM: return speedOfLight / frequencyHzQzssL2;
        case CTS_SIGNAL::QZSS_L5Q: return speedOfLight / frequencyHzQzssL5;
        case CTS_SIGNAL::QZSS_L6P: [[fallthrough]];
        case CTS_SIGNAL::QZSS_L6D: return speedOfLight / frequencyHzQzssL6;
        default: return 0.0;
        }
    case CTS_SYSTEM::NAVIC:
        switch (eSignalType)
        {
        case CTS_SIGNAL::NAVIC_L5SPS: return speedOfLight / frequencyHzGpsL5;
        default: return 0.0;
        }
    default: return 0.0;
    }
}

uint32_t ChannelTrackingStatus::CalculatePrn(uint32_t uiPRN_) const
{
    //! Some logic for PRN offsets based on the constellation. See documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGECMP4.htm#Measurem
    switch (eSatelliteSystem)
    {
    case CTS_SYSTEM::GLONASS:
        // If ternary returns true, documentation suggests we should save this PRN as
        // GLONASS_SLOT_UNKNOWN_UPPER_LIMIT - GLONASS frequency number. However, this
        // would output the PRN as an actual valid Slot ID, which is not true. We will
        // set this to 0 here because 0 is considered an unknown/invalid GLONASS Slot ID.
        return GLONASS_SLOT_UNKNOWN_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= GLONASS_SLOT_UNKNOWN_UPPER_LIMIT ? 0 : uiPRN_ + GLONASS_SLOT_OFFSET - 1;
    case CTS_SYSTEM::SBAS:
        return SBAS_PRN_OFFSET_120_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_120_UPPER_LIMIT   ? uiPRN_ + SBAS_PRN_OFFSET_120 - 1
               : SBAS_PRN_OFFSET_130_LOWER_LIMIT <= uiPRN_ && uiPRN_ <= SBAS_PRN_OFFSET_130_UPPER_LIMIT ? uiPRN_ + SBAS_PRN_OFFSET_130 - 1
                                                                                                        : 0;
    case CTS_SYSTEM::QZSS: return uiPRN_ + QZSS_PRN_OFFSET - 1;
    default: return uiPRN_;
    }
}

//------------------------------------------------------------------------------
uint32_t ChannelTrackingStatus::GetAsWord() const
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
