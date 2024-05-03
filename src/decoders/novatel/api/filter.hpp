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
// ! \file filter.hpp
// ===============================================================================

#ifndef NOVATEL_FILTER_HPP
#define NOVATEL_FILTER_HPP

#include <memory>
#include <tuple>

#include "decoders/common/api/common.hpp"
#include "decoders/common/api/logger.hpp"
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/header_decoder.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class Filter
//! \brief Filter notifies the caller if a message should be accepted or
//! rejected based on the Filter's configuration and the message's
//! MetaData. A non configured filter will notify the caller to accept all
//! messages.
//============================================================================
class Filter
{
  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{Logger::RegisterLogger("novatel_filter")};

    std::vector<bool (Filter::*)(const MetaDataStruct&)> vMyFilterFunctions;

    // Filtering members
    std::vector<TIME_STATUS> vMyTimeStatusFilters;
    bool bMyInvertTimeStatusFilter;

    std::vector<std::tuple<uint32_t, HEADER_FORMAT, MEASUREMENT_SOURCE>> vMyMessageIdFilters;
    bool bMyInvertMessageIdFilter;

    std::vector<std::tuple<std::string, HEADER_FORMAT, MEASUREMENT_SOURCE>> vMyMessageNameFilters;
    bool bMyInvertMessageNameFilter;

    uint32_t uiMyLowerWeek;
    uint32_t uiMyLowerMSec;
    bool bMyFilterLowerTime;
    uint32_t uiMyUpperWeek;
    uint32_t uiMyUpperMSec;
    bool bMyFilterUpperTime;
    bool bMyInvertTimeFilter;

    uint32_t uiMyDecimationPeriodMilliSec;
    bool bMyDecimate;
    bool bMyInvertDecimation;

    bool bMyIncludeNmea;

    void PushUnique(bool (Filter::*filter_)(const MetaDataStruct&));

    bool FilterTime(const MetaDataStruct& stMetaData_);
    bool FilterTimeStatus(const MetaDataStruct& stMetaData_);
    bool FilterMessageId(const MetaDataStruct& stMetaData_);
    bool FilterMessage(const MetaDataStruct& stMetaData_);
    bool FilterDecimation(const MetaDataStruct& stMetaData_);

  public:
    //----------------------------------------------------------------------------
    //! \brief Constructor for the Filter class.
    //----------------------------------------------------------------------------
    Filter();

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger();

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const;

    //----------------------------------------------------------------------------
    //! \brief Shutdown the internal logger.
    //----------------------------------------------------------------------------
    static void ShutdownLogger();

    //----------------------------------------------------------------------------
    //! \brief Include messages at and above the lower time bound (inclusive).
    //
    //! \param [in] uiWeek_  The week lower bound.
    //! \param [in] dSec_   The second lower bound.
    //----------------------------------------------------------------------------
    void SetIncludeLowerTimeBound(uint32_t uiWeek_, double dSec_);

    //----------------------------------------------------------------------------
    //! \brief Include messages at and below the upper time bound (inclusive).
    //
    //! \param [in] uiWeek_  The week upper bound.
    //! \param [in] dSec_   The second upper bound.
    //----------------------------------------------------------------------------
    void SetIncludeUpperTimeBound(uint32_t uiWeek_, double dSec_);

    //----------------------------------------------------------------------------
    //! \brief Invert the time range/span filter.
    //!
    //! For example, if the lower time bound is set and invert time filter is
    //! true, then the lower time bound acts as an upper time bound but excludes
    //! the bound. If both the lower and upper bounds are set with invert time
    //! filter as true, then only messages outside the timespan are kept (excludes
    //! the bounds).
    //
    //! \param [in] bInvert_  True to invert the time filter.
    //----------------------------------------------------------------------------
    void InvertTimeFilter(bool bInvert_);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the decimation period.
    //!
    //! For example, keep all messages with a decimation period of 500
    //! milliseconds.
    //
    //! \param [in] dPeriodSec_  Period in milliseconds.
    //----------------------------------------------------------------------------
    void SetIncludeDecimation(double dPeriodSec_);

    //----------------------------------------------------------------------------
    //! \brief Invert the decimation filter.
    //!
    //! For example, if the decimation period is set to one thousand milliseconds
    //! and invert decimation is true, then keep all messages that do not have a
    //! period of one thousand milliseconds.
    //
    //! \param [in] bInvert_  True to invert the decimation filter.
    //----------------------------------------------------------------------------
    void InvertDecimationFilter(bool bInvert_);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the time status.
    //
    //! \param [in] eTimeStatus_  The time status.
    //----------------------------------------------------------------------------
    void IncludeTimeStatus(TIME_STATUS eTimeStatus_);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match multiple time statuses.
    //!
    //! For example, include messages with either the "COARSE" or "FINE" status.
    //
    //! \param [in] vTimeStatuses_ Multiple time statuses.
    //----------------------------------------------------------------------------
    void IncludeTimeStatus(std::vector<TIME_STATUS> vTimeStatuses_);

    //----------------------------------------------------------------------------
    //! \brief Invert the time status filter.
    //
    //! \param [in] bInvert_  True to invert the time status filter.
    //----------------------------------------------------------------------------
    void InvertTimeStatusFilter(bool bInvert_);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the message ID.
    //
    //! \param [in] uiId_  The message ID.
    //! \param [in] eFormat_  The message format.
    //! \param [in] eSource_  The antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageId(uint32_t uiId_, HEADER_FORMAT eFormat_ = HEADER_FORMAT::ALL, MEASUREMENT_SOURCE eSource_ = MEASUREMENT_SOURCE::PRIMARY);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match multiple message IDs.
    //
    //! \param [in] vIds_  Vector of tuples containing: message ID, message format,
    //! and antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageId(std::vector<std::tuple<uint32_t, HEADER_FORMAT, MEASUREMENT_SOURCE>>& vIds_);

    //----------------------------------------------------------------------------
    //! \brief Invert the message ID filter.
    //
    //! \param [in] bInvert_   True to invert the message ID filter.
    //----------------------------------------------------------------------------
    void InvertMessageIdFilter(bool bInvert_);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the message name.
    //
    //! \param [in] szMsgName_  The message name.
    //! \param [in] eFormat_  The message format.
    //! \param [in] eSource_  The antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageName(const std::string& szMsgName_, HEADER_FORMAT eFormat_ = HEADER_FORMAT::ALL,
                            MEASUREMENT_SOURCE eSource_ = MEASUREMENT_SOURCE::PRIMARY);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match multiple message names.
    //
    //! \param [in] vNames_  Vector of tuples containing: message name,
    //! message format, and antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageName(std::vector<std::tuple<std::string, HEADER_FORMAT, MEASUREMENT_SOURCE>>& vNames_);

    //----------------------------------------------------------------------------
    //! \brief Invert the message name filter.
    //
    //! \param [in] bInvert_  True to invert the message name filter.
    //----------------------------------------------------------------------------
    void InvertMessageNameFilter(bool bInvert_);

    //----------------------------------------------------------------------------
    //! \brief Include NMEA logs.
    //!
    //! Defaults to false (exclude NMEA logs).
    //
    //! \param [in] bIncludeNmea_  True to keep/include NMEA logs.
    //----------------------------------------------------------------------------
    void IncludeNmeaMessages(bool bIncludeNmea_);

    //----------------------------------------------------------------------------
    //! \brief Clear all current filter settings.
    //----------------------------------------------------------------------------
    void ClearFilters();

    //----------------------------------------------------------------------------
    //! \brief Filter the MetaDataStruct based on the current Filter
    //! settings.
    //
    //! \param [in] stMetaData_  The MetaDataStruct to filter.
    //----------------------------------------------------------------------------
    bool DoFiltering(const MetaDataStruct& stMetaData_);
};

} // namespace novatel::edie::oem

#endif // NOVATEL_FILTER_HPP
