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

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace novatel::edie::oem {
// TODO: Make filter a common base class.
//============================================================================
//! \class Filter
//! \brief Filter notifies the caller if a message should be accepted or
//! rejected based on the Filter's configuration and the message's
//! MetaData. A non configured filter will notify the caller to accept all
//! messages.
//============================================================================
class Filter
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger{pclLoggerManager->RegisterLogger("novatel_filter")};

    std::vector<bool (Filter::*)(const MetaDataStruct&) const> vMyFilterFunctions;

    // Filtering members
    std::vector<TIME_STATUS> vMyTimeStatusFilters;
    bool bMyInvertTimeStatusFilter{};

    std::vector<std::tuple<uint32_t, HEADER_FORMAT, MEASUREMENT_SOURCE>> vMyMessageIdFilters;
    bool bMyInvertMessageIdFilter{};

    std::vector<std::tuple<std::string, HEADER_FORMAT, MEASUREMENT_SOURCE>> vMyMessageNameFilters;
    bool bMyInvertMessageNameFilter{};

    uint32_t uiMyLowerWeek{};
    uint32_t uiMyLowerMSec{};
    bool bMyFilterLowerTime{};
    uint32_t uiMyUpperWeek{};
    uint32_t uiMyUpperMSec{};
    bool bMyFilterUpperTime{};
    bool bMyInvertTimeFilter{};

    uint32_t uiMyDecimationPeriodMilliSec{};
    bool bMyDecimate{};
    bool bMyInvertDecimation{};

    bool bMyIncludeNmea{};

    //----------------------------------------------------------------------------
    //! \brief Push an element into a vector unless its already included
    //!
    //! \param[in] vec_ The vector to push the element into.
    //! \param[in] element_ The element to insert.
    //----------------------------------------------------------------------------
    template <typename T> void PushUnique(std::vector<T>& vec_, const T& element_);
    //----------------------------------------------------------------------------
    //! \brief Remove an element from a vector.
    //!
    //! \param[in] vec_ The vector to remove the element from.
    //! \param[in] element_ The element to remove.
    //----------------------------------------------------------------------------
    template <typename T> void Remove(std::vector<T>& vec_, const T& element_);

    [[nodiscard]] bool FilterTime(const MetaDataStruct& stMetaData_) const;
    [[nodiscard]] bool FilterTimeStatus(const MetaDataStruct& stMetaData_) const;
    [[nodiscard]] bool FilterMessageId(const MetaDataStruct& stMetaData_) const;
    [[nodiscard]] bool FilterMessage(const MetaDataStruct& stMetaData_) const;
    [[nodiscard]] bool FilterDecimation(const MetaDataStruct& stMetaData_) const;

  public:
    //----------------------------------------------------------------------------
    //! \brief Constructor for the Filter class.
    //----------------------------------------------------------------------------
    Filter();

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //!
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //!
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Include messages at and above the lower time bound (inclusive).
    //!
    //! \param[in] uiWeek_  The week lower bound.
    //! \param[in] dSec_   The second lower bound.
    //----------------------------------------------------------------------------
    void SetIncludeLowerTimeBound(uint32_t uiWeek_, double dSec_);

    //----------------------------------------------------------------------------
    //! \brief Include messages at and below the upper time bound (inclusive).
    //!
    //! \param[in] uiWeek_  The week upper bound.
    //! \param[in] dSec_   The second upper bound.
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
    //!
    //! \param[in] bInvert_  True to invert the time filter.
    //----------------------------------------------------------------------------
    void InvertTimeFilter(bool bInvert_) { bMyInvertTimeFilter = bInvert_; }

    //----------------------------------------------------------------------------
    //! \brief Clear the lower time bound filter.
    //----------------------------------------------------------------------------
    void ClearLowerTimeBound();

    //----------------------------------------------------------------------------
    //! \brief Clear the upper time bound filter.
    //----------------------------------------------------------------------------
    void ClearUpperTimeBound();

    //----------------------------------------------------------------------------
    //! \brief Clear both the lower and upper time bound filters.
    //----------------------------------------------------------------------------
    void ClearTimeBounds();

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the decimation period.
    //!
    //! For example, keep all messages with a decimation period of 500
    //! milliseconds.
    //!
    //! \param[in] dPeriodSec_  Period in milliseconds.
    //----------------------------------------------------------------------------
    void SetIncludeDecimation(double dPeriodSec_);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the decimation period in milliseconds.
    //!
    //! \param[in] dPeriodMSec_  Period in milliseconds.
    //----------------------------------------------------------------------------
    void SetIncludeDecimationMs(uint32_t dPeriodMSec_);

    //----------------------------------------------------------------------------
    //! \brief Invert the decimation filter.
    //!
    //! For example, if the decimation period is set to one thousand milliseconds
    //! and invert decimation is true, then keep all messages that do not have a
    //! period of one thousand milliseconds.
    //!
    //! \param[in] bInvert_  True to invert the decimation filter.
    //----------------------------------------------------------------------------
    void InvertDecimationFilter(bool bInvert_) { bMyInvertDecimation = bInvert_; }

    //----------------------------------------------------------------------------
    //! \brief Clear the decimation filter.
    //----------------------------------------------------------------------------
    void ClearDecimationFilter();

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the time status.
    //!
    //! \param[in] eTimeStatus_  The time status.
    //----------------------------------------------------------------------------
    void IncludeTimeStatus(TIME_STATUS eTimeStatus_);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match multiple time statuses.
    //!
    //! For example, include messages with either the "COARSE" or "FINE" status.
    //!
    //! \param[in] vTimeStatuses_ Multiple time statuses.
    //----------------------------------------------------------------------------
    void IncludeTimeStatus(std::vector<TIME_STATUS> vTimeStatuses_);

    //----------------------------------------------------------------------------
    //! \brief Remove a specific time status from the filter.
    //!
    //! \param[in] eTimeStatus_  The time status to remove.
    //----------------------------------------------------------------------------
    void RemoveTimeStatus(TIME_STATUS eTimeStatus_);

    //----------------------------------------------------------------------------
    //! \brief Invert the time status filter.
    //!
    //! \param[in] bInvert_  True to invert the time status filter.
    //----------------------------------------------------------------------------
    void InvertTimeStatusFilter(bool bInvert_) { bMyInvertTimeStatusFilter = bInvert_; }

    //----------------------------------------------------------------------------
    //! \brief Clear all time status filters.
    //----------------------------------------------------------------------------
    void ClearTimeStatuses();

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the message ID.
    //!
    //! \param[in] uiId_  The message ID.
    //! \param[in] eFormat_  The message format.
    //! \param[in] eSource_  The antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageId(uint32_t uiId_, HEADER_FORMAT eFormat_ = HEADER_FORMAT::ALL, MEASUREMENT_SOURCE eSource_ = MEASUREMENT_SOURCE::PRIMARY);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match multiple message IDs.
    //!
    //! \param[in] vIds_  Vector of tuples containing: message ID, message format,
    //! and antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageId(std::vector<std::tuple<uint32_t, HEADER_FORMAT, MEASUREMENT_SOURCE>>& vIds_);

    //----------------------------------------------------------------------------
    //! \brief Remove a specific message ID from the filter.
    //!
    //! \param[in] uiId_  The message ID to remove.
    //! \param[in] eFormat_  The message format.
    //! \param[in] eSource_  The antenna source.
    //----------------------------------------------------------------------------
    void RemoveMessageId(uint32_t uiId_, HEADER_FORMAT eFormat_, MEASUREMENT_SOURCE eSource_);

    //----------------------------------------------------------------------------
    //! \brief Invert the message ID filter.
    //!
    //! \param[in] bInvert_   True to invert the message ID filter.
    //----------------------------------------------------------------------------
    void InvertMessageIdFilter(bool bInvert_) { bMyInvertMessageIdFilter = bInvert_; }

    //----------------------------------------------------------------------------
    //! \brief Clear all message ID filters.
    //----------------------------------------------------------------------------
    void ClearMessageIds();

    //----------------------------------------------------------------------------
    //! \brief Include messages that match the message name.
    //!
    //! \param[in] szMsgName_  The message name.
    //! \param[in] eFormat_  The message format.
    //! \param[in] eSource_  The antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageName(std::string_view szMsgName_, HEADER_FORMAT eFormat_ = HEADER_FORMAT::ALL,
                            MEASUREMENT_SOURCE eSource_ = MEASUREMENT_SOURCE::PRIMARY);

    //----------------------------------------------------------------------------
    //! \brief Include messages that match multiple message names.
    //!
    //! \param[in] vNames_  Vector of tuples containing: message name,
    //! message format, and antenna source.
    //----------------------------------------------------------------------------
    void IncludeMessageName(std::vector<std::tuple<std::string, HEADER_FORMAT, MEASUREMENT_SOURCE>>& vNames_);

    //----------------------------------------------------------------------------
    //! \brief Remove a specific message name from the filter.
    //!
    //! \param[in] szMsgName_  The message name to remove.
    //! \param[in] eFormat_  The message format.
    //! \param[in] eSource_  The antenna source.
    //----------------------------------------------------------------------------
    void RemoveMessageName(std::string_view szMsgName_, HEADER_FORMAT eFormat_, MEASUREMENT_SOURCE eSource_);

    //----------------------------------------------------------------------------
    //! \brief Invert the message name filter.
    //!
    //! \param[in] bInvert_  True to invert the message name filter.
    //----------------------------------------------------------------------------
    void InvertMessageNameFilter(bool bInvert_) { bMyInvertMessageNameFilter = bInvert_; }

    //----------------------------------------------------------------------------
    //! \brief Clear all message name filters.
    //----------------------------------------------------------------------------
    void ClearMessageNames();

    //----------------------------------------------------------------------------
    //! \brief Include NMEA logs.
    //!
    //! Defaults to false (exclude NMEA logs).
    //!
    //! \param[in] bIncludeNmea_  True to keep/include NMEA logs.
    //----------------------------------------------------------------------------
    void IncludeNmeaMessages(bool bIncludeNmea_) { bMyIncludeNmea = bIncludeNmea_; }

    //----------------------------------------------------------------------------
    //! \brief Clear all current filter settings.
    //----------------------------------------------------------------------------
    void ClearFilters();

    //----------------------------------------------------------------------------
    //! \brief Filter the MetaDataStruct based on the current Filter
    //! settings.
    //!
    //! \param[in] stMetaData_  The MetaDataStruct to filter.
    //!
    //! \return True if the message passes the filter, false otherwise.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool DoFiltering(const MetaDataStruct& stMetaData_) const;

  public:
    using Ptr = std::shared_ptr<Filter>;
    using ConstPtr = std::shared_ptr<const Filter>;
};

} // namespace novatel::edie::oem

#endif // NOVATEL_FILTER_HPP
