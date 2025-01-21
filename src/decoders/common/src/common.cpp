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
// ! \file common.cpp
// ===============================================================================

#include "novatel_edie/decoders/common/common.hpp"

#include "novatel_edie/decoders/common/message_database.hpp"

using namespace novatel::edie;

//-----------------------------------------------------------------------
uint32_t CreateMsgId(const uint32_t uiMessageId_, const uint32_t uiSiblingId_, const uint32_t uiMsgFormat_, const uint32_t uiResponse_)
{
    return static_cast<uint16_t>(uiMessageId_) | (static_cast<uint32_t>(MESSAGE_ID_MASK::MEASSRC) & (uiSiblingId_ << 16)) |
           (static_cast<uint32_t>(MESSAGE_ID_MASK::MSGFORMAT) & (uiMsgFormat_ << 21)) |
           (static_cast<uint32_t>(MESSAGE_ID_MASK::RESPONSE) & (uiResponse_ << 23));
}

//-----------------------------------------------------------------------
void UnpackMsgId(const uint32_t uiMessageId_, uint16_t& usMessageId_, uint32_t& uiSiblingId_, uint32_t& uiMsgFormat_, uint32_t& uiResponse_)
{
    usMessageId_ = (uiMessageId_ & static_cast<uint32_t>(MESSAGE_ID_MASK::LOGID));
    uiSiblingId_ = (uiMessageId_ & static_cast<uint32_t>(MESSAGE_ID_MASK::MEASSRC)) >> 16;
    uiMsgFormat_ = (uiMessageId_ & static_cast<uint32_t>(MESSAGE_ID_MASK::MSGFORMAT)) >> 21;
    uiResponse_ = (uiMessageId_ & static_cast<uint32_t>(MESSAGE_ID_MASK::RESPONSE)) >> 23;
}

//-----------------------------------------------------------------------
unsigned char PackMsgType(const uint32_t uiSiblingId_, const uint32_t uiMsgFormat_, const uint32_t uiResponse_)
{
    return static_cast<uint8_t>(((uiResponse_ << 7) & static_cast<uint32_t>(MESSAGE_TYPE_MASK::RESPONSE)) |
                                ((uiMsgFormat_ << 5) & static_cast<uint32_t>(MESSAGE_TYPE_MASK::MSGFORMAT)) |
                                ((uiSiblingId_ << 0) & static_cast<uint32_t>(MESSAGE_TYPE_MASK::MEASSRC)));
}

//-----------------------------------------------------------------------
std::string GetEnumString(const EnumDefinition* const stEnumDef_, const uint32_t uiEnum_)
{
    if (stEnumDef_ != nullptr)
    {
        for (const auto& e : stEnumDef_->enumerators)
        {
            if (e.value == uiEnum_) { return e.name; }
        }
    }

    return "UNKNOWN";
}

//-----------------------------------------------------------------------
int32_t GetEnumValue(const EnumDefinition* const stEnumDef_, std::string_view strEnum_)
{
    if (stEnumDef_ != nullptr)
    {
        for (const auto& e : stEnumDef_->enumerators)
        {
            if (e.name == strEnum_) { return static_cast<int32_t>(e.value); }
        }
    }

    return 0;
}

//-----------------------------------------------------------------------
int32_t GetResponseId(const EnumDefinition* const stRespDef_, std::string_view strResp_)
{
    if (stRespDef_ != nullptr)
    {
        for (const auto& e : stRespDef_->enumerators)
        {
            // response string is stored in description
            if (e.description == strResp_) { return static_cast<int32_t>(e.value); }
        }
    }

    return 0;
}

std::string GetEnumString(const std::shared_ptr<const EnumDefinition>& stEnumDef_, uint32_t uiEnum_)
{
    return GetEnumString(stEnumDef_.get(), uiEnum_);
}

int32_t GetEnumValue(const std::shared_ptr<const EnumDefinition>& stEnumDef_, std::string_view strEnum_)
{
    return GetEnumValue(stEnumDef_.get(), strEnum_);
}

int32_t GetResponseId(const std::shared_ptr<const EnumDefinition>& stRespDef_, std::string_view strResp_)
{
    return GetResponseId(stRespDef_.get(), strResp_);
}

//-----------------------------------------------------------------------
bool ConsumeAbbrevFormatting(const uint64_t ullTokenLength_, const char** ppcMessageBuffer_)
{
    bool bIsAbbrev = false;

    if ((ullTokenLength_ == 0 || ullTokenLength_ == 1) &&
        (static_cast<int8_t>(**ppcMessageBuffer_) == '\r' || static_cast<int8_t>(**ppcMessageBuffer_) == '\n' ||
         static_cast<int8_t>(**ppcMessageBuffer_) == '<'))
    {
        // Skip over '\r\n<     '
        while (true)
        {
            switch (**ppcMessageBuffer_)
            {
            case '\r': [[fallthrough]];
            case '\n': *ppcMessageBuffer_ += sizeof(int8_t); break;
            case '<':
                *ppcMessageBuffer_ += sizeof(int8_t);
                bIsAbbrev = true;
                break;
            case ' ':
                if (bIsAbbrev) { *ppcMessageBuffer_ += sizeof(int8_t); }
                break;
            default: return bIsAbbrev;
            }
        }
    }

    return bIsAbbrev;
}
