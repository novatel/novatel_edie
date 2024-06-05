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
// ! \file novatel_filter.cpp
// ===============================================================================

#include "novatel_filter.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool NovatelFilterSetLoggerLevel(Filter* pclFilter_, uint32_t uiLogLevel_)
{
    return pclFilter_ && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclFilter_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

void NovatelFilterShutdownLogger(Filter* pclFilter_)
{
    if (pclFilter_) { pclFilter_->ShutdownLogger(); } // TODO: ShutdownLogger is static, this function signature should be changed
}

Filter* NovatelFilterInit() { return new Filter(); }

void NovatelFilterDelete(Filter* pclFilter_)
{
    if (pclFilter_)
    {
        delete pclFilter_;
        pclFilter_ = nullptr;
    }
}

void NovatelFilterSetIncludeLowerTime(Filter* pclFilter_, uint32_t uiLowerTimeWeek_, double dLowerTimeSec_)
{
    if (pclFilter_) { pclFilter_->SetIncludeLowerTimeBound(uiLowerTimeWeek_, dLowerTimeSec_); }
}

void NovatelFilterSetIncludeUpperTime(Filter* pclFilter_, uint32_t uiUpperTime_, double dUpperTimeSec_)
{
    if (pclFilter_) { pclFilter_->SetIncludeUpperTimeBound(uiUpperTime_, dUpperTimeSec_); }
}

void NovatelFilterInvertTimeFilter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertTimeFilter(bInvert_); }
}

void NovatelFilterSetIncludeDecimation(Filter* pclFilter_, double dPeriodSec_)
{
    if (pclFilter_) { pclFilter_->SetIncludeDecimation(dPeriodSec_); }
}

void NovatelFilterInvertDecimationFilter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertDecimationFilter(bInvert_); }
}

void NovatelFilterIncludeTimeStatus(Filter* pclFilter_, TIME_STATUS eTimeStatus_)
{
    if (pclFilter_) { pclFilter_->IncludeTimeStatus(eTimeStatus_); }
}

void NovatelFilterInvertTimeStatusFilter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertTimeStatusFilter(bInvert_); }
}

void NovatelFilterIncludeMessageId(Filter* pclFilter_, uint32_t uiId_, HEADER_FORMAT eFormat_, MEASUREMENT_SOURCE eSource_)
{
    if (pclFilter_) { pclFilter_->IncludeMessageId(uiId_, eFormat_, eSource_); }
}

void NovatelFilterInvertMessageIdFilter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertMessageIdFilter(bInvert_); }
}

void NovatelFilterIncludeMessageName(Filter* pclFilter_, uint8_t* pucMessageName_, HEADER_FORMAT eFormat_, MEASUREMENT_SOURCE eSource_)
{
    if (pclFilter_ && pucMessageName_) { pclFilter_->IncludeMessageName(std::string(reinterpret_cast<char*>(pucMessageName_)), eFormat_, eSource_); }
}

void NovatelFilterInvertMessageNameFilter(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->InvertMessageNameFilter(bInvert_); }
}

void NovatelFilterIncludeNmeaMessages(Filter* pclFilter_, bool bInvert_)
{
    if (pclFilter_) { pclFilter_->IncludeNmeaMessages(bInvert_); }
}

bool NovatelFilterDoFiltering(Filter* pclFilter_, MetaDataStruct* pstMetaData_)
{
    return pclFilter_ && pstMetaData_ ? pclFilter_->DoFiltering(*pstMetaData_) : false;
}

void NovatelFilterClearFilters(Filter* pclFilter_)
{
    if (pclFilter_) { pclFilter_->ClearFilters(); }
}
