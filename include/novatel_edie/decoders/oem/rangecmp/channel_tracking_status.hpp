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
// ! \file channel_tracking_status.hpp
// ===============================================================================

#ifndef CHANNEL_TRACKING_STATUS_HPP
#define CHANNEL_TRACKING_STATUS_HPP

#include <array>
#include <cassert>
#include <cstdint>

#include "novatel_edie/decoders/oem/rangecmp/common.hpp"

namespace novatel::edie::oem {

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
class ChannelTrackingStatus
{
  public:
    //! Default constructor.
    ChannelTrackingStatus() = default;

    //! Constructor from a channel tracking status word.
    ChannelTrackingStatus(uint32_t uiChannelTrackingStatus_);

    //! Constructor from the available data from a RANGECMP2 SAT/SIG block pair.
    ChannelTrackingStatus(const rangecmp2::SatelliteBlock& stSatelliteBlock_, const rangecmp2::SignalBlock& stSignalBlock_);

    //! Constructor from the available data from a RANGECMP4 Primary Block and Measurement Block pair.
    ChannelTrackingStatus(SYSTEM eSystem_, rangecmp4::SIGNAL_TYPE eSignalType_, const rangecmp4::MeasurementSignalBlock& stMeasurementBlock_);

    //! Constructor from the available data from a RANGECMP5 Primary Block and Measurement Block pair.
    ChannelTrackingStatus(SYSTEM eSystem_, rangecmp4::SIGNAL_TYPE eSignalType_, const rangecmp5::MeasurementSignalBlock& stMeasurementBlock_);

    //! Lookup function for a signal wavelength.
    [[nodiscard]] double GetSignalWavelength(int16_t sGLONASSFrequency_) const;

    //! Combine the channel tracking status fields into a single 4-byte value according to documentation:
    //! https://docs.novatel.com/OEM7/Content/Logs/RANGE.htm?Highlight=RANGE#Table_ChannelTrackingStatus
    [[nodiscard]] uint32_t GetAsWord() const;

    //! Generate a unique key.
    [[nodiscard]] uint64_t MakeKey(uint32_t prn, MEASUREMENT_SOURCE source) const;

    //! Get the satellite system
    [[nodiscard]] SYSTEM GetSystem() const;

  private:
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
    //! \enum CTS_SYSTEM
    //-----------------------------------------------------------------------
    enum class CTS_SYSTEM
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
    //! \enum CTS_SIGNAL
    //-----------------------------------------------------------------------
    enum class CTS_SIGNAL
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

    //! Convert a SYSTEM enumeration to a channel tracking status SATELLITE_SYSTEM.
    static CTS_SYSTEM SystemToSatelliteSystem(SYSTEM eSystem_);

    //! Convert a RANGECMP2 signal type to the channel tracking status enumeration.
    static CTS_SIGNAL RangeCmp2SignalTypeToSignalType(CTS_SYSTEM eSystem_, rangecmp2::SIGNAL_TYPE eSignalType_);

    //! Convert a RANGECMP4 signal type to the channel tracking status enumeration.
    static CTS_SIGNAL RangeCmp4SignalTypeToSignalType(CTS_SYSTEM eSystem_, rangecmp4::SIGNAL_TYPE eSignalType_);

    TRACKING_STATE eTrackingState{TRACKING_STATE::IDLE};
    uint32_t uiSVChannelNumber{0U};
    bool bPhaseLocked{false};
    bool bParityKnown{false};
    bool bCodeLocked{false};
    CORRELATOR_TYPE eCorrelatorType{CORRELATOR_TYPE::NONE};
    CTS_SYSTEM eSatelliteSystem{CTS_SYSTEM::GPS};
    bool bGrouped{false};
    CTS_SIGNAL eSignalType{CTS_SIGNAL::UNKNOWN};
    bool bPrimaryL1Channel{false};
    bool bHalfCycleAdded{false};
    bool bDigitalFilteringOnSignal{false};
    bool bPRNLocked{false};
    bool bChannelAssignmentForced{false};
};

} // namespace novatel::edie::oem

#endif // CHANNEL_TRACKING_STATUS_HPP
