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
// ! \file message_decoder.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/message_decoder.hpp"

#include <bitset>
#include <charconv>
#include <cmath>

#include <nlohmann/json.hpp>

using namespace novatel::edie::oem;

using json = nlohmann::json;

// -------------------------------------------------------------------------------------------------------
MessageDecoder::MessageDecoder(const MessageDatabase::Ptr& pclMessageDb_) : MessageDecoderBase(pclMessageDb_)
{
    InitOemFieldMaps();
    if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoder::InitOemFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("c")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        // TODO: check that the character is printable
        // if (!isprint(**ppcToken_)) { throw ... }
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(**ppcToken_), std::move(pstMessageDataType_));
    };
    asciiFieldMap[CalculateBlockCrc32("k")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCrc32("lk")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCrc32("ucb")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                   const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                   [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(*ppcToken_).to_ulong()), std::move(pstMessageDataType_));
    };

    asciiFieldMap[CalculateBlockCrc32("T")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::llround(strtod(*ppcToken_, nullptr) * SEC_TO_MILLI_SEC)), std::move(pstMessageDataType_));
    };

    asciiFieldMap[CalculateBlockCrc32("m")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        vIntermediateFormat_.emplace_back(pclMsgDb_.MsgNameToMsgId(std::string(*ppcToken_, tokenLength_)), std::move(pstMessageDataType_));
    };

    asciiFieldMap[CalculateBlockCrc32("id")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                  const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const auto* pcDelimiter = static_cast<const char*>(memchr(*ppcToken_, '+', tokenLength_));
        if (pcDelimiter == nullptr) { pcDelimiter = static_cast<const char*>(memchr(*ppcToken_, '-', tokenLength_)); }

        auto usSlot = static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 10));
        int16_t sFreq = pcDelimiter != nullptr ? static_cast<int16_t>(strtol(pcDelimiter, nullptr, 10)) : 0;

        const uint32_t uiSatId = usSlot | (sFreq << 16);
        vIntermediateFormat_.emplace_back(uiSatId, std::move(pstMessageDataType_));
    };

    asciiFieldMap[CalculateBlockCrc32("R")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        // RXCONFIG in ASCII is always #COMMANDNAMEA
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(std::string_view(*ppcToken_ + 1, tokenLength_ - 2)); // + 1 to Skip the '#'
        vIntermediateFormat_.emplace_back(pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0, std::move(pstMessageDataType_));
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("c")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("k")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("lk")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCrc32("ucb")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                  const json& clJsonField_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(clJsonField_.get<std::string>().c_str()).to_ulong()),
                                          std::move(pstMessageDataType_));
    };

    jsonFieldMap[CalculateBlockCrc32("m")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                const json& clJsonField_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        vIntermediateFormat_.emplace_back(pclMsgDb_.MsgNameToMsgId(clJsonField_.get<std::string>()), std::move(pstMessageDataType_));
    };

    jsonFieldMap[CalculateBlockCrc32("T")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                const json& clJsonField_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::llround(clJsonField_.get<double>() * SEC_TO_MILLI_SEC)), std::move(pstMessageDataType_));
    };

    jsonFieldMap[CalculateBlockCrc32("id")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                 const json& clJsonField_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sTemp = clJsonField_.get<std::string_view>();
        size_t sDelimiter = sTemp.find_last_of('+');
        if (sDelimiter == std::string_view::npos) { sDelimiter = sTemp.find_last_of('-'); }

        uint16_t usSlot;
        int16_t sFreq;

        if (sDelimiter != std::string_view::npos)
        {
            // Parse the slot part
            std::string_view slotView = sTemp.substr(0, sDelimiter);
            auto result = std::from_chars(slotView.data(), slotView.data() + slotView.size(), usSlot);
            if (result.ec != std::errc() || result.ptr != slotView.data() + slotView.size())
            {
                throw std::invalid_argument("Invalid slot value in id field");
            }

            // Parse the frequency part
            std::string_view freqView = sTemp.substr(sDelimiter + 1);
            result = std::from_chars(freqView.data(), freqView.data() + freqView.size(), sFreq);
            if (result.ec != std::errc() || result.ptr != freqView.data() + freqView.size())
            {
                throw std::invalid_argument("Invalid frequency value in id field");
            }

            if (sTemp[sDelimiter] == '-') { sFreq = -sFreq; }
        }
        else
        {
            // Parse the entire string as the slot
            auto result = std::from_chars(sTemp.data(), sTemp.data() + sTemp.size(), usSlot);
            if (result.ec != std::errc() || result.ptr != sTemp.data() + sTemp.size())
            {
                throw std::invalid_argument("Invalid slot value in id field");
            }
            sFreq = 0;
        }

        const uint32_t uiSatId = (usSlot | (sFreq << 16));
        vIntermediateFormat_.emplace_back(uiSatId, std::move(pstMessageDataType_));
    };

    jsonFieldMap[CalculateBlockCrc32("R")] = [](std::vector<FieldContainer>& vIntermediateFormat_, BaseField::ConstPtr&& pstMessageDataType_,
                                                const json& clJsonField_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(clJsonField_.get<std::string_view>());
        vIntermediateFormat_.emplace_back(pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0, std::move(pstMessageDataType_));
    };
}
