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
//! \file novatel_framer.hpp
//! \brief DLL-exposed OEM framer functionality.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef DYNAMIC_LIBRARY_NOVATEL_FRAMER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_FRAMER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/filter.hpp"
#include "decoders/novatel/api/framer.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool novatel_framer_set_logger_level(novatel::edie::oem::Framer* pclFramer_, uint32_t uiLogLevel_);
    DECODERS_EXPORT void novatel_framer_shutdown_logger(novatel::edie::oem::Framer* pclFramer_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::Framer* novatel_framer_init();
    DECODERS_EXPORT void novatel_framer_delete(novatel::edie::oem::Framer* pclFramer_);

    // Config
    DECODERS_EXPORT void novatel_framer_frame_json(novatel::edie::oem::Framer* pclFramer_, bool bFrameJson_);
    DECODERS_EXPORT void novatel_framer_payload_only(novatel::edie::oem::Framer* pclFramer_, bool bPayloadOnly_);
    DECODERS_EXPORT void novatel_framer_report_unknown_bytes(novatel::edie::oem::Framer* pclFramer_, bool bReportUnknownBytes_);

    // R/W
    DECODERS_EXPORT uint32_t novatel_framer_get_available_bytes(novatel::edie::oem::Framer* pclFramer_);
    DECODERS_EXPORT uint32_t novatel_framer_write(novatel::edie::oem::Framer* pclFramer_, unsigned char* pucBytes_, uint32_t uiByteCount_);
    DECODERS_EXPORT novatel::edie::STATUS novatel_framer_read(novatel::edie::oem::Framer* pclFramer_, unsigned char* pucBuffer_,
                                                              uint32_t uiBufferSize_, novatel::edie::oem::MetaDataStruct* pstMetaData_);
    DECODERS_EXPORT uint32_t novatel_framer_flush(novatel::edie::oem::Framer* pclFramer_, unsigned char* pucBuffer_, uint32_t uiBufferSize_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_FRAMER_HPP
