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
//! \file novatel_commander.cpp
//! \brief DLL-exposed OEM commander functionality.
//! \remark See novatel::edie::oem::Commander for API details.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "novatel_commander.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool novatel_commander_set_logger_level(Commander* pclCommander_, uint32_t uiLogLevel_)
{
    return pclCommander_ && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclCommander_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

void novatel_commander_shutdown_logger(Commander* pclCommander_)
{
    if (pclCommander_) { pclCommander_->ShutdownLogger(); }
}

Commander* novatel_commander_init(JsonReader* pclJsonDb_) { return new Commander(pclJsonDb_); }

void novatel_commander_delete(Commander* pclCommander_)
{
    if (pclCommander_)
    {
        delete pclCommander_;
        pclCommander_ = nullptr;
    }
}

void novatel_commander_load_json(Commander* pclCommander_, JsonReader* pclJsonDb_)
{
    if (pclCommander_ && pclJsonDb_) { pclCommander_->LoadJsonDb(pclJsonDb_); }
}

STATUS novatel_commander_encode(Commander* pclCommander_, char* pcAbbrevAsciiCommand_, uint32_t uicAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                                uint32_t* puiEncodeBufferSize_, ENCODEFORMAT eEncodeFormat_)
{
    return pclCommander_ && pcAbbrevAsciiCommand_ && pcEncodeBuffer_ && puiEncodeBufferSize_
               ? pclCommander_->Encode(pcAbbrevAsciiCommand_, uicAbbrevAsciiCommandLength_, pcEncodeBuffer_, *puiEncodeBufferSize_, eEncodeFormat_)
               : STATUS::NULL_PROVIDED;
}
