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
//! \file novatel_filter.hpp
//! \brief DLL-exposed OEM filter functionality.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef DYNAMIC_LIBRARY_NOVATEL_FILTER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_FILTER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/filter.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool novatel_filter_set_logger_level(novatel::edie::oem::Filter* pclFilter_, uint32_t uiLogLevel_);
    DECODERS_EXPORT void novatel_filter_shutdown_logger(novatel::edie::oem::Filter* pclFilter_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::Filter* novatel_filter_init();
    DECODERS_EXPORT void novatel_filter_delete(novatel::edie::oem::Filter* pclFilter_);

    // Config
    DECODERS_EXPORT void novatel_filter_set_include_lower_time(novatel::edie::oem::Filter* pclFilter_, uint32_t uiLowerTimeWeek_,
                                                               double dLowerTimeSec_);
    DECODERS_EXPORT void novatel_filter_set_include_upper_time(novatel::edie::oem::Filter* pclFilter_, uint32_t uiUpperTime_, double dUpperTimeSec_);
    DECODERS_EXPORT void novatel_filter_invert_time_filter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void novatel_filter_set_include_decimation(novatel::edie::oem::Filter* pclFilter_, double dPeriodSec_);
    DECODERS_EXPORT void novatel_filter_invert_decimation_filter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void novatel_filter_include_time_status(novatel::edie::oem::Filter* pclFilter_, novatel::edie::TIME_STATUS eTimeStatus_);
    DECODERS_EXPORT void novatel_filter_invert_time_status_filter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void novatel_filter_include_message_id(novatel::edie::oem::Filter* pclFilter_, uint32_t uiId_,
                                                           novatel::edie::HEADERFORMAT eFormat_ = novatel::edie::HEADERFORMAT::ALL,
                                                           novatel::edie::MEASUREMENT_SOURCE eSource_ = novatel::edie::MEASUREMENT_SOURCE::PRIMARY);
    DECODERS_EXPORT void novatel_filter_invert_message_id_filter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void novatel_filter_include_message_name(novatel::edie::oem::Filter* pclFilter_, uint8_t* pucMessageName_,
                                                             novatel::edie::HEADERFORMAT eFormat_ = novatel::edie::HEADERFORMAT::ALL,
                                                             novatel::edie::MEASUREMENT_SOURCE eSource_ = novatel::edie::MEASUREMENT_SOURCE::PRIMARY);
    DECODERS_EXPORT void novatel_filter_invert_message_name_filter(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    DECODERS_EXPORT void novatel_filter_include_nmea_messages(novatel::edie::oem::Filter* pclFilter_, bool bInvert_);

    // novatel::edie::oem::Filter
    DECODERS_EXPORT bool novatel_filter_do_filtering(novatel::edie::oem::Filter* pclFilter_, novatel::edie::oem::MetaDataStruct* pstMetaData_);
    DECODERS_EXPORT void novatel_filter_clear_filters(novatel::edie::oem::Filter* pclFilter_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_FILTER_HPP
