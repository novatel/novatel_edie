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
// ! \file novatel_commander.cpp
// ===============================================================================

#include "novatel_commander.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

bool NovatelCommanderSetLoggerLevel(Commander* pclCommander_, uint32_t uiLogLevel_)
{
    return (pclCommander_ != nullptr) && uiLogLevel_ >= spdlog::level::level_enum::trace && uiLogLevel_ < spdlog::level::level_enum::n_levels
           ? pclCommander_->SetLoggerLevel(static_cast<spdlog::level::level_enum>(uiLogLevel_)),
           true : false;
}

Commander* NovatelCommanderInit(JsonReader* pclJsonDb_) { return new Commander(pclJsonDb_); }

void NovatelCommanderDelete(Commander* pclCommander_)
{
    if (pclCommander_ != nullptr)
    {
        delete pclCommander_;
        pclCommander_ = nullptr;
    }
}

void NovatelCommanderLoadJson(Commander* pclCommander_, JsonReader* pclJsonDb_)
{
    if ((pclCommander_ != nullptr) && (pclJsonDb_ != nullptr)) { pclCommander_->LoadJsonDb(pclJsonDb_); }
}

STATUS NovatelCommanderEncode(Commander* pclCommander_, char* pcAbbrevAsciiCommand_, uint32_t uicAbbrevAsciiCommandLength_, char* pcEncodeBuffer_,
                              uint32_t* puiEncodeBufferSize_, ENCODE_FORMAT eEncodeFormat_)
{
    return (pclCommander_ != nullptr) && (pcAbbrevAsciiCommand_ != nullptr) && (pcEncodeBuffer_ != nullptr) && (puiEncodeBufferSize_ != nullptr)
               ? pclCommander_->Encode(pcAbbrevAsciiCommand_, uicAbbrevAsciiCommandLength_, pcEncodeBuffer_, *puiEncodeBufferSize_, eEncodeFormat_)
               : STATUS::NULL_PROVIDED;
}
