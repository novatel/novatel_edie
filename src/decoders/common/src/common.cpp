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
//! \file common.cpp
//! \brief Common functions for the EDIE source code
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "common.hpp"

#include "jsonreader.hpp"

using namespace novatel::edie;

//-----------------------------------------------------------------------
bool IsEqual(double dVal1_, double dVal2_, double dEpsilon_)
{
    double dDiff = dVal1_ - dVal2_;

    if (dDiff < 0) { dDiff *= -1.0; }

    return dDiff < dEpsilon_;
}

//-----------------------------------------------------------------------
uint32_t CreateMsgID(uint32_t uiMessageID_, uint32_t uiSiblingID_, uint32_t uiMsgFormat_, uint32_t uiResponse_)
{
    return static_cast<uint16_t>(uiMessageID_) | (static_cast<uint32_t>(MESSAGEIDMASK::MEASSRC) & (uiSiblingID_ << 16)) |
           (static_cast<uint32_t>(MESSAGEIDMASK::MSGFORMAT) & (uiMsgFormat_ << 21)) |
           (static_cast<uint32_t>(MESSAGEIDMASK::RESPONSE) & (uiResponse_ << 23));
}

//-----------------------------------------------------------------------
void UnpackMsgID(const uint32_t uiMessageID_, uint16_t& usMessageID_, uint32_t& uiSiblingID_, uint32_t& uiMsgFormat_, uint32_t& uiResponse_)
{
    usMessageID_ = (uiMessageID_ & static_cast<uint32_t>(MESSAGEIDMASK::LOGID));
    uiSiblingID_ = (uiMessageID_ & static_cast<uint32_t>(MESSAGEIDMASK::MEASSRC)) >> 16;
    uiMsgFormat_ = (uiMessageID_ & static_cast<uint32_t>(MESSAGEIDMASK::MSGFORMAT)) >> 21;
    uiResponse_ = (uiMessageID_ & static_cast<uint32_t>(MESSAGEIDMASK::RESPONSE)) >> 23;
}

//-----------------------------------------------------------------------
unsigned char PackMsgType(const uint32_t uiSiblingID_, const uint32_t uiMsgFormat_, const uint32_t uiResponse_)
{
    return static_cast<uint8_t>(((uiResponse_ << 7) & static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE)) |
                                ((uiMsgFormat_ << 5) & static_cast<uint32_t>(MESSAGETYPEMASK::MSGFORMAT)) |
                                ((uiSiblingID_ << 0) & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC)));
}

//-----------------------------------------------------------------------
std::string GetEnumString(const EnumDefinition* const stEnumDef_, uint32_t uiEnum_)
{
    if (stEnumDef_ != nullptr)
    {
        for (auto& e : stEnumDef_->enumerators)
        {
            if (e.value == uiEnum_) { return e.name; }
        }
    }

    return std::string("UNKNOWN");
}

//-----------------------------------------------------------------------
int32_t GetEnumValue(const EnumDefinition* const stEnumDef_, std::string strEnum_)
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
int32_t GetResponseId(const EnumDefinition* const stRespDef_, std::string strResp_)
{
    if (stRespDef_ != nullptr)
    {
        for (auto& e : stRespDef_->enumerators)
        {
            if (e.description == strResp_) // response string is stored in description
            {
                return static_cast<int32_t>(e.value);
            }
        }
    }

    return 0;
}

//-----------------------------------------------------------------------
int32_t ToDigit(char c) { return static_cast<int32_t>(c - '0'); }

//-----------------------------------------------------------------------
bool ConsumeAbbrevFormatting(uint64_t ullTokenLength_, char** ppucMessageBuffer_)
{
    bool bIsAbbrev = false;

    if ((ullTokenLength_ == 0 || ullTokenLength_ == 1) &&
        (static_cast<int8_t>(**ppucMessageBuffer_) == '\r' || static_cast<int8_t>(**ppucMessageBuffer_) == '\n' ||
         static_cast<int8_t>(**ppucMessageBuffer_) == '<'))
    {
        // Skip over '\r\n<     '
        while (true)
        {
            char c = static_cast<char>(**ppucMessageBuffer_);
            if (c == '\r' || c == '\n') { *ppucMessageBuffer_ += sizeof(int8_t); }
            else if (c == '<')
            {
                *ppucMessageBuffer_ += sizeof(int8_t);
                bIsAbbrev = true;
            }
            else if (bIsAbbrev && c == ' ') { *ppucMessageBuffer_ += sizeof(int8_t); }
            else { break; }
        }
    }

    return bIsAbbrev;
}
