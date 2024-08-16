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
// ! \file novatel_filter.hpp
// ===============================================================================

#ifndef DYNAMIC_LIBRARY_NOVATEL_FILTER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_FILTER_HPP

#include "decoders/novatel/api/filter.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool NovatelFilterSetLoggerLevel(novatel::edie::oem::Filter* pclFilter_, uint32_t uiLogLevel_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::Filter* NovatelFilterInit();
    DECODERS_EXPORT void NovatelFilterDelete(novatel::edie::oem::Filter* pclFilter_);

    // Config
    DECODERS_EXPORT void NovatelFilterSetIncludeLowerTime(novatel::edie::oem::Filter* pclFilter_, uint32_t uiLowerTimeWeek_, double dLowerTimeSec_);
    DECODERS_EXPORT void NovatelFilterSetIncludeUpperTime(novatel::edie::oem::Filter* pclFilter_, uint32_t uiUpperTime_, double dUpperTimeSec_);
    DECODERS_EXPORT void NovatelFilterInvertTimeFilter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void NovatelFilterSetIncludeDecimation(novatel::edie::oem::Filter* pclFilter_, double dPeriodSec_);
    DECODERS_EXPORT void NovatelFilterInvertDecimationFilter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void NovatelFilterIncludeTimeStatus(novatel::edie::oem::Filter* pclFilter_, novatel::edie::TIME_STATUS eTimeStatus_);
    DECODERS_EXPORT void NovatelFilterInvertTimeStatusFilter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void NovatelFilterIncludeMessageId(novatel::edie::oem::Filter* pclFilter_, uint32_t uiId_,
                                                       novatel::edie::HEADER_FORMAT eFormat_ = novatel::edie::HEADER_FORMAT::ALL,
                                                       novatel::edie::MEASUREMENT_SOURCE eSource_ = novatel::edie::MEASUREMENT_SOURCE::PRIMARY);
    DECODERS_EXPORT void NovatelFilterInvertMessageIdFilter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void NovatelFilterIncludeMessageName(novatel::edie::oem::Filter* pclFilter_, uint8_t* pucMessageName_,
                                                         novatel::edie::HEADER_FORMAT eFormat_ = novatel::edie::HEADER_FORMAT::ALL,
                                                         novatel::edie::MEASUREMENT_SOURCE eSource_ = novatel::edie::MEASUREMENT_SOURCE::PRIMARY);
    DECODERS_EXPORT void NovatelFilterInvertMessageNameFilter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void NovatelFilterIncludeNmeaMessages(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    // novatel::edie::oem::Filter
    DECODERS_EXPORT bool NovatelFilterDoFiltering(novatel::edie::oem::Filter* pclFilter_, novatel::edie::oem::MetaDataStruct* pstMetaData_);
    DECODERS_EXPORT void NovatelFilterClearFilters(novatel::edie::oem::Filter* pclFilter_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_FILTER_HPP
