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
// ! \file filter.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/filter.hpp"

#include <algorithm>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
Filter::Filter()
{
    ClearFilters();
    pclMyLogger->debug("Filter initialized");
}

// -------------------------------------------------------------------------------------------------------
template <typename T> void Filter::PushUnique(std::vector<T>& vec_, const T& element_)
{
    if (std::find(vec_.begin(), vec_.end(), element_) == vec_.end()) { vec_.push_back(element_); }
}

template <typename T> void Filter::Remove(std::vector<T>& vec_, const T& element_)
{
    auto it = std::find(vec_.begin(), vec_.end(), element_);
    if (it != vec_.end()) { vec_.erase(it); }
}

// -------------------------------------------------------------------------------------------------------
void Filter::SetIncludeLowerTimeBound(uint32_t uiWeek_, double dSec_)
{
    bMyFilterLowerTime = true;
    uiMyLowerWeek = uiWeek_;
    uiMyLowerMSec = static_cast<uint32_t>(dSec_ * 1000.0);
    PushUnique(vMyFilterFunctions, &Filter::FilterTime);
}

void Filter::SetIncludeUpperTimeBound(uint32_t uiWeek_, double dSec_)
{
    bMyFilterUpperTime = true;
    uiMyUpperWeek = uiWeek_;
    uiMyUpperMSec = static_cast<uint32_t>(dSec_ * 1000.0);
    PushUnique(vMyFilterFunctions, &Filter::FilterTime);
}

void Filter::ClearLowerTimeBound()
{
    bMyFilterLowerTime = false;
    if (!bMyFilterUpperTime) { Remove(vMyFilterFunctions, &Filter::FilterTime); }
}

void Filter::ClearUpperTimeBound()
{
    bMyFilterUpperTime = false;
    if (!bMyFilterLowerTime) { Remove(vMyFilterFunctions, &Filter::FilterTime); }
}

void Filter::ClearTimeBounds()
{
    ClearLowerTimeBound();
    ClearUpperTimeBound();
}

// -------------------------------------------------------------------------------------------------------
void Filter::SetIncludeDecimation(double dPeriodSec_)
{
    SetIncludeDecimationMs(static_cast<uint32_t>(dPeriodSec_ * 1000.0)); // Convert provided seconds to MSec
}

void Filter::SetIncludeDecimationMs(uint32_t dPeriodMSec_)
{
    bMyDecimate = true;
    uiMyDecimationPeriodMilliSec = dPeriodMSec_; // Convert double to int
    PushUnique(vMyFilterFunctions, &Filter::FilterDecimation);
}

void Filter::ClearDecimationFilter()
{
    bMyDecimate = false;
    Remove(vMyFilterFunctions, &Filter::FilterDecimation);
}

// -------------------------------------------------------------------------------------------------------
void Filter::IncludeTimeStatus(TIME_STATUS eTimeStatus_)
{
    PushUnique(vMyTimeStatusFilters, eTimeStatus_);
    PushUnique(vMyFilterFunctions, &Filter::FilterTimeStatus);
}

void Filter::IncludeTimeStatus(std::vector<TIME_STATUS> vTimeStatuses_)
{
    for (const auto& status : vTimeStatuses_) { PushUnique(vMyTimeStatusFilters, status); }
    PushUnique(vMyFilterFunctions, &Filter::FilterTimeStatus);
}

void Filter::RemoveTimeStatus(TIME_STATUS eTimeStatus_)
{
    Remove(vMyTimeStatusFilters, eTimeStatus_);
    if (vMyTimeStatusFilters.empty()) { Remove(vMyFilterFunctions, &Filter::FilterTimeStatus); }
}

void Filter::ClearTimeStatuses()
{
    vMyTimeStatusFilters.clear();
    bMyInvertTimeStatusFilter = false;
    Remove(vMyFilterFunctions, &Filter::FilterTimeStatus);
}

// -------------------------------------------------------------------------------------------------------
void Filter::IncludeMessageId(uint32_t uiId_, HEADER_FORMAT eFormat_, uint8_t ucSource_)
{
    auto tMessageId = std::make_tuple(uiId_, eFormat_, ucSource_);
    PushUnique(vMyMessageIdFilters, tMessageId);
    PushUnique(vMyFilterFunctions, &Filter::FilterMessageId);
}

void Filter::IncludeMessageId(std::vector<std::tuple<uint32_t, HEADER_FORMAT, uint8_t>>& vIds_)
{
    for (const auto& id : vIds_) { PushUnique(vMyMessageIdFilters, id); }
    PushUnique(vMyFilterFunctions, &Filter::FilterMessageId);
}

void Filter::RemoveMessageId(uint32_t uiId_, HEADER_FORMAT eFormat_, uint8_t ucSource_)
{
    auto tMessageId = std::make_tuple(uiId_, eFormat_, ucSource_);
    Remove(vMyMessageIdFilters, tMessageId);
    if (vMyMessageIdFilters.empty()) { Remove(vMyFilterFunctions, &Filter::FilterMessageId); }
}

void Filter::ClearMessageIds()
{
    vMyMessageIdFilters.clear();
    bMyInvertMessageIdFilter = false;
    Remove(vMyFilterFunctions, &Filter::FilterMessageId);
}

// -------------------------------------------------------------------------------------------------------
void Filter::IncludeMessageName(std::string_view szMsgName_, HEADER_FORMAT eFormat_, uint8_t ucSource_)
{
    auto tMessageName = std::make_tuple(std::string(szMsgName_), eFormat_, ucSource_);
    PushUnique(vMyMessageNameFilters, tMessageName);
    PushUnique(vMyFilterFunctions, &Filter::FilterMessage);
}

void Filter::IncludeMessageName(std::vector<std::tuple<std::string, HEADER_FORMAT, uint8_t>>& vNames_)
{
    for (const auto& name : vNames_) { PushUnique(vMyMessageNameFilters, name); }
    PushUnique(vMyFilterFunctions, &Filter::FilterMessage);
}

void Filter::RemoveMessageName(std::string_view szMsgName_, HEADER_FORMAT eFormat_, uint8_t ucSource_)
{
    auto tMessageName = std::make_tuple(std::string(szMsgName_), eFormat_, ucSource_);
    Remove(vMyMessageNameFilters, tMessageName);
    if (vMyMessageNameFilters.empty()) { Remove(vMyFilterFunctions, &Filter::FilterMessage); }
}

void Filter::ClearMessageNames()
{
    vMyMessageNameFilters.clear();
    bMyInvertMessageNameFilter = false;
    Remove(vMyFilterFunctions, &Filter::FilterMessage);
}

// -------------------------------------------------------------------------------------------------------
void Filter::ClearFilters()
{
    vMyTimeStatusFilters.clear();
    bMyInvertTimeStatusFilter = false;

    vMyMessageIdFilters.clear();
    bMyInvertMessageIdFilter = false;

    vMyMessageNameFilters.clear();
    bMyInvertMessageNameFilter = false;

    uiMyLowerWeek = 0;
    uiMyLowerMSec = 0;
    uiMyUpperWeek = 0;
    uiMyUpperMSec = 0;
    bMyFilterLowerTime = false;
    bMyFilterUpperTime = false;
    bMyInvertTimeFilter = false;

    uiMyDecimationPeriodMilliSec = 0;
    bMyDecimate = false;
    bMyInvertDecimation = false;

    bMyIncludeNmea = false;
    vMyFilterFunctions.clear();

    bMyIncludeNonResponses = true;
    bMyIncludeResponses = true;
}

// -------------------------------------------------------------------------------------------------------
bool Filter::FilterTime(const MetaDataStruct& stMetaData_) const
{
    const auto usMetaDataWeek = static_cast<uint32_t>(stMetaData_.usWeek);
    const auto usMetaDataMilliseconds = static_cast<uint32_t>(stMetaData_.dMilliseconds);

    if (bMyInvertTimeFilter)
    {
        const bool bAboveLowerTime = usMetaDataWeek > uiMyLowerWeek || (usMetaDataWeek == uiMyLowerWeek && usMetaDataMilliseconds >= uiMyLowerMSec);
        const bool bBelowUpperTime = usMetaDataWeek < uiMyUpperWeek || (usMetaDataWeek == uiMyUpperWeek && usMetaDataMilliseconds <= uiMyUpperMSec);

        return bMyFilterLowerTime && bMyFilterUpperTime ? !(bAboveLowerTime && bBelowUpperTime)
                                                        : !((bAboveLowerTime && bMyFilterLowerTime) || (bBelowUpperTime && bMyFilterUpperTime));
    }

    const bool bBelowLowerTime = usMetaDataWeek < uiMyLowerWeek || (usMetaDataWeek == uiMyLowerWeek && usMetaDataMilliseconds < uiMyLowerMSec);
    const bool bAboveUpperTime = usMetaDataWeek > uiMyUpperWeek || (usMetaDataWeek == uiMyUpperWeek && usMetaDataMilliseconds > uiMyUpperMSec);

    return !((bMyFilterLowerTime && bBelowLowerTime) || (bMyFilterUpperTime && bAboveUpperTime));
}

// -------------------------------------------------------------------------------------------------------
bool Filter::FilterTimeStatus(const MetaDataStruct& stMetaData_) const
{
    return vMyTimeStatusFilters.empty() ||
           bMyInvertTimeStatusFilter ==
               (vMyTimeStatusFilters.end() == std::find(vMyTimeStatusFilters.begin(), vMyTimeStatusFilters.end(), stMetaData_.eTimeStatus));
}

// -------------------------------------------------------------------------------------------------------
bool Filter::FilterMessageId(const MetaDataStruct& stMetaData_) const
{
    if (vMyMessageIdFilters.empty()) { return true; }

    auto uiMessageId = static_cast<uint32_t>(stMetaData_.usMessageId);
    HEADER_FORMAT eFormat = stMetaData_.eFormat;
    uint8_t ucSource = stMetaData_.ucSiblingId;

    const auto isMessageIdFilterMatch = [&uiMessageId, ucSource](const std::tuple<uint32_t, HEADER_FORMAT, uint8_t>& elem) {
        return uiMessageId == std::get<0>(elem) && HEADER_FORMAT::ALL == std::get<1>(elem) && ucSource == std::get<2>(elem);
    };

    return bMyInvertMessageIdFilter ==
           (vMyMessageIdFilters.end() == std::find_if(vMyMessageIdFilters.begin(), vMyMessageIdFilters.end(), isMessageIdFilterMatch) &&
            vMyMessageIdFilters.end() ==
                std::find(vMyMessageIdFilters.begin(), vMyMessageIdFilters.end(), std::make_tuple(uiMessageId, eFormat, ucSource)));
}

// -------------------------------------------------------------------------------------------------------
bool Filter::FilterMessage(const MetaDataStruct& stMetaData_) const
{
    if (vMyMessageNameFilters.empty()) { return true; }

    std::string_view szMessageName = stMetaData_.messageName;
    HEADER_FORMAT eFormat = stMetaData_.eFormat;
    uint8_t eSource = stMetaData_.ucSiblingId;

    const auto isMessageNameFilterMatch = [&szMessageName, eSource](const std::tuple<std::string_view, HEADER_FORMAT, uint8_t>& elem_) {
        return szMessageName == std::get<0>(elem_) && HEADER_FORMAT::ALL == std::get<1>(elem_) && eSource == std::get<2>(elem_);
    };

    return bMyInvertMessageNameFilter ==
           (vMyMessageNameFilters.end() == std::find_if(vMyMessageNameFilters.begin(), vMyMessageNameFilters.end(), isMessageNameFilterMatch) &&
            vMyMessageNameFilters.end() ==
                std::find(vMyMessageNameFilters.begin(), vMyMessageNameFilters.end(), std::make_tuple(szMessageName, eFormat, eSource)));
}

// -------------------------------------------------------------------------------------------------------
bool Filter::FilterDecimation(const MetaDataStruct& stMetaData_) const
{
    return !bMyDecimate || bMyInvertDecimation == static_cast<bool>(static_cast<uint32_t>(stMetaData_.dMilliseconds) % uiMyDecimationPeriodMilliSec);
}

// -------------------------------------------------------------------------------------------------------
bool Filter::DoFiltering(const MetaDataStruct& stMetaData_) const
{
    if (stMetaData_.eFormat == HEADER_FORMAT::UNKNOWN) { return false; }
    if (stMetaData_.eFormat == HEADER_FORMAT::NMEA) { return bMyIncludeNmea; }
    if (stMetaData_.bResponse && !bMyIncludeResponses) { return false; }
    if (!stMetaData_.bResponse && !bMyIncludeNonResponses) { return false; }

    return std::all_of(vMyFilterFunctions.begin(), vMyFilterFunctions.end(),
                       [this, &stMetaData_](const auto& filterFunction) { return (this->*filterFunction)(stMetaData_); });
}
