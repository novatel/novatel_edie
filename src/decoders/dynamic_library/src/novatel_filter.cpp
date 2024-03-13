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
//! \file novatel_filter.cpp
//! \brief DLL-exposed OEM filter functionality.
//! \remark See novatel::edie::oem::Filter for API details.
///////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "novatel_filter.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool novatel_filter_set_logger_level(Filter* pclFilter_, uint32_t uiLogLevel_)
{
    return pclFilter_ && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclFilter_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

void novatel_filter_shutdown_logger(Filter* pclFilter_)
{
    if (pclFilter_) { pclFilter_->ShutdownLogger(); }
}

Filter* novatel_filter_init() { return new Filter(); }

void novatel_filter_delete(Filter* pclFilter_)
{
    if (pclFilter_)
    {
        delete pclFilter_;
        pclFilter_ = nullptr;
    }
}

void novatel_filter_set_include_lower_time(Filter* pclFilter_, uint32_t uiLowerTimeWeek_, double dLowerTimeSec_)
{
    if (pclFilter_) { pclFilter_->SetIncludeLowerTimeBound(uiLowerTimeWeek_, dLowerTimeSec_); }
}

void novatel_filter_set_include_upper_time(Filter* pclFilter_, uint32_t uiUpperTime_, double dUpperTimeSec_)
{
    if (pclFilter_) { pclFilter_->SetIncludeUpperTimeBound(uiUpperTime_, dUpperTimeSec_); }
}

void novatel_filter_invert_time_filter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertTimeFilter(bInvert_); }
}

void novatel_filter_set_include_decimation(Filter* pclFilter_, double dPeriodSec_)
{
    if (pclFilter_) { pclFilter_->SetIncludeDecimation(dPeriodSec_); }
}

void novatel_filter_invert_decimation_filter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertDecimationFilter(bInvert_); }
}

void novatel_filter_include_time_status(Filter* pclFilter_, TIME_STATUS eTimeStatus_)
{
    if (pclFilter_) { pclFilter_->IncludeTimeStatus(eTimeStatus_); }
}

void novatel_filter_invert_time_status_filter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertTimeStatusFilter(bInvert_); }
}

void novatel_filter_include_message_id(Filter* pclFilter_, uint32_t uiId_, HEADERFORMAT eFormat_, MEASUREMENT_SOURCE eSource_)
{
    if (pclFilter_) { pclFilter_->IncludeMessageId(uiId_, eFormat_, eSource_); }
}

void novatel_filter_invert_message_id_filter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertMessageIdFilter(bInvert_); }
}

void novatel_filter_include_message_name(Filter* pclFilter_, uint8_t* pucMessageName_, HEADERFORMAT eFormat_, MEASUREMENT_SOURCE eSource_)
{
    if (pclFilter_ && pucMessageName_) { pclFilter_->IncludeMessageName(std::string(reinterpret_cast<char*>(pucMessageName_)), eFormat_, eSource_); }
}

void novatel_filter_invert_message_name_filter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertMessageNameFilter(bInvert_); }
}

void novatel_filter_include_nmea_messages(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->IncludeNMEAMessages(bInvert_); }
}

bool novatel_filter_do_filtering(Filter* pclFilter_, MetaDataStruct* pstMetaData_)
{
    return pclFilter_ && pstMetaData_ ? pclFilter_->DoFiltering(*pstMetaData_) : false;
}

void novatel_filter_clear_filters(Filter* pclFilter_)
{
    if (pclFilter_) { pclFilter_->ClearFilters(); }
}
