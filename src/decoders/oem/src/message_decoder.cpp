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

#include <simdjson.h>

#include "novatel_edie/decoders/oem/crc.hpp"

using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
MessageDecoder::MessageDecoder(const MessageDatabase::Ptr& pclMessageDb_) : MessageDecoderBase("OEM", pclMessageDb_) { InitOemFieldMaps(); }

// -------------------------------------------------------------------------------------------------------
void MessageDecoder::InitOemFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("c")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (token_.size() != 1) { throw std::runtime_error("Invalid character value"); }
        const auto value = static_cast<uint32_t>(static_cast<unsigned char>(token_.front()));
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };
    asciiFieldMap[CalculateBlockCrc32("k")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCrc32("lk")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCrc32("ucb")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                   const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (token_.empty() || token_.size() > 8) { throw std::runtime_error("Invalid binary byte"); }
        uint32_t value;
        if (!FromCharsExact<2>(token_, value)) { throw std::runtime_error("Invalid binary byte"); }
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("T")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        double value;
        if (!FromCharsExact(token_, value)) { throw std::runtime_error("Failed to parse double value"); }
        const auto converted = static_cast<uint32_t>(std::llround(value * SEC_TO_MILLI_SEC));
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, converted); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, converted); }
    };

    asciiFieldMap[CalculateBlockCrc32("m")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const uint32_t value = pclMsgDb_.MsgNameToMsgId(token_);
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("id")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                  const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (token_.empty()) { throw std::runtime_error("Empty satellite ID"); }
        const auto delimiter = token_.find_last_of("+-");
        const auto slot = token_.substr(0, delimiter);
        uint16_t usSlot;
        if (!FromCharsExact(slot, usSlot)) { throw std::runtime_error("Failed to parse usSlot"); }

        int16_t sFreq = 0;
        if (delimiter != std::string_view::npos)
        {
            const auto frequency = token_.substr(delimiter + (token_[delimiter] == '+' ? 1 : 0));
            if (!FromCharsExact(frequency, sFreq)) { throw std::runtime_error("Failed to parse sFreq"); }
        }

        const uint32_t uiSatId = usSlot | (static_cast<uint32_t>(static_cast<uint16_t>(sFreq)) << 16);
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, uiSatId); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, uiSatId); }
    };

    asciiFieldMap[CalculateBlockCrc32("R")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        // RXCONFIG in ASCII is always #COMMANDNAMEA
        if (token_.size() < 2) { throw std::runtime_error("Invalid RXCONFIG command name"); }
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(token_.substr(1, token_.size() - 2));
        const uint32_t value = pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0;
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("c")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("k")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("lk")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCrc32("ucb")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                  simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                  [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sValue;
        if (clJsonField_.get(sValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_.name + "'");
        }
        const auto value = static_cast<uint32_t>(std::bitset<8>(std::string(sValue)).to_ulong());
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("m")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sValue;
        if (clJsonField_.get(sValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_.name + "'");
        }
        const auto value = pclMsgDb_.MsgNameToMsgId(std::string(sValue));
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("T")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        double dValue;
        if (clJsonField_.get(dValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_.name + "'");
        }
        const auto value = static_cast<uint32_t>(std::llround(dValue * SEC_TO_MILLI_SEC));
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("id")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                 simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sTemp;
        if (clJsonField_.get(sTemp) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_.name + "'");
        }
        size_t sDelimiter = sTemp.find_last_of('+');
        if (sDelimiter == std::string_view::npos) { sDelimiter = sTemp.find_last_of('-'); }

        uint16_t usSlot;
        int16_t sFreq;

        if (sDelimiter != std::string_view::npos)
        {
            // Parse the slot part
            std::string_view slotView = sTemp.substr(0, sDelimiter);
            if (!FromCharsExact(slotView, usSlot)) { throw std::invalid_argument("Invalid slot value in id field"); }

            // Parse the frequency part
            std::string_view freqView = sTemp.substr(sDelimiter + 1);
            if (!FromCharsExact(freqView, sFreq)) { throw std::invalid_argument("Invalid frequency value in id field"); }

            if (sTemp[sDelimiter] == '-') { sFreq = -sFreq; }
        }
        else
        {
            // Parse the entire string as the slot
            if (!FromCharsExact(sTemp, usSlot)) { throw std::invalid_argument("Invalid slot value in id field"); }
            sFreq = 0;
        }

        const uint32_t uiSatId = (usSlot | (sFreq << 16));
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, uiSatId); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, uiSatId); }
    };

    jsonFieldMap[CalculateBlockCrc32("R")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sValue;
        if (clJsonField_.get(sValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_.name + "'");
        }
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(sValue);
        const auto value = pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0;
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };
}

novatel::edie::MessageDefinition::ConstPtr MessageDecoder::GetMessageDefinition(MetaDataBase& stMetaData_) const
{
    if (stMetaData_.bResponse)
    {
        if (pResponseDefinition != nullptr) { return pResponseDefinition; }

        auto responseDefinition = std::make_shared<MessageDefinition>();
        responseDefinition->name = "response";

        auto responsesEnum = pclMyMsgDb->GetEnumDefName("Responses");

        SimpleDataType responseIdDataType;
        responseIdDataType.description = "Response as numerical id";
        responseIdDataType.length = 4;
        responseIdDataType.name = DATA_TYPE::INT;

        auto responseIdField = std::make_shared<EnumField>();
        responseIdField->name = "response_id";
        responseIdField->type = FIELD_TYPE::RESPONSE_ID;
        responseIdField->dataType = responseIdDataType;
        responseIdField->index = 0;
        if (responsesEnum != nullptr) { responseIdField->enumId = responsesEnum->_id; }
        responseIdField->enumDef = responsesEnum;

        SimpleDataType responseStrDataType;
        responseStrDataType.description = "Response as a string";
        responseStrDataType.length = 1;
        responseStrDataType.name = DATA_TYPE::CHAR;

        auto responseStrField = std::make_shared<BaseField>();
        responseStrField->name = "response_str";
        responseStrField->type = FIELD_TYPE::RESPONSE_STR;
        responseStrField->dataType = responseStrDataType;
        responseStrField->index = 0;

        responseDefinition->fieldInfo[0] =
            std::make_shared<FieldInfo>(FieldInfo{sizeof(uint32_t), 1, std::vector<BaseField::ConstPtr>{responseIdField, responseStrField}});

        pResponseDefinition = responseDefinition;
        return pResponseDefinition;
    }
    return MessageDecoderBase::GetMessageDefinition(stMetaData_);
}
