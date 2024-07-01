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

#include "novatel_edie/common/common.hpp"

#include "novatel_edie/common/json_reader.hpp"

using namespace novatel::edie;

//-----------------------------------------------------------------------
bool IsEqual(const double dVal1_, const double dVal2_, const double dEpsilon_)
{
    double dDiff = dVal1_ - dVal2_;

    if (dDiff < 0) { dDiff *= -1.0; }

    return dDiff < dEpsilon_;
}

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
        for (auto& e : stEnumDef_->enumerators)
        {
            if (e.value == uiEnum_) { return e.name; }
        }
    }

    return "UNKNOWN";
}

//-----------------------------------------------------------------------
int32_t GetEnumValue(const EnumDefinition* const stEnumDef_, const std::string& strEnum_)
{
    if (stEnumDef_ != nullptr)
    {
        for (auto& e : stEnumDef_->enumerators)
        {
            if (e.name == strEnum_) { return static_cast<int32_t>(e.value); }
        }
    }

    return 0;
}

//-----------------------------------------------------------------------
int32_t GetResponseId(const EnumDefinition* const stRespDef_, const std::string& strResp_)
{
    if (stRespDef_ != nullptr)
    {
        for (auto& e : stRespDef_->enumerators)
        {
            // response string is stored in description
            if (e.description == strResp_) { return static_cast<int32_t>(e.value); }
        }
    }

    return 0;
}

//-----------------------------------------------------------------------
int32_t ToDigit(const char c_) { return c_ - '0'; }

//-----------------------------------------------------------------------
bool ConsumeAbbrevFormatting(const uint64_t ullTokenLength_, char** ppcMessageBuffer_)
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
