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
//! \file nvoatel_encoder.hpp
//! \brief DLL-exposed OEM encoder functionality.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef DYNAMIC_LIBRARY_NOVATEL_ENCODER_HPP
#define DYNAMIC_LIBRARY_NOVATEL_ENCODER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/jsonreader.hpp"
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/encoder.hpp"
#include "decoders_export.h"

extern "C"
{
    // Logger
    DECODERS_EXPORT bool novatel_encoder_set_logger_level(novatel::edie::oem::Encoder* pclEncoder_, uint32_t iLogLevel_);
    DECODERS_EXPORT void novatel_encoder_shutdown_logger(novatel::edie::oem::Encoder* pclEncoder_);

    // Construct/Destruct
    DECODERS_EXPORT novatel::edie::oem::Encoder* novatel_encoder_init(novatel::edie::JsonReader* pclJsonDb_);
    DECODERS_EXPORT void novatel_encoder_delete(novatel::edie::oem::Encoder* pclEncoder_);

    // Config
    DECODERS_EXPORT void novatel_encoder_load_json(novatel::edie::oem::Encoder* pclEncoder_, novatel::edie::JsonReader* pclJsonDb_);

    // R/W
    DECODERS_EXPORT novatel::edie::STATUS novatel_encoder_encode(novatel::edie::oem::Encoder* pclEncoder_, unsigned char* pucEncodeBuffer_,
                                                                 uint32_t uiEncodeBufferSize_,
                                                                 novatel::edie::oem::IntermediateHeader* pstInterHeader_,
                                                                 novatel::edie::IntermediateMessage* pstInterMessage_,
                                                                 novatel::edie::MessageDataStruct* pstMessageData_,
                                                                 novatel::edie::oem::MetaDataStruct* pstMetaData_,
                                                                 novatel::edie::ENCODEFORMAT uiEncodeFormat_);
}

#endif // DYNAMIC_LIBRARY_NOVATEL_ENCODER_HPP
