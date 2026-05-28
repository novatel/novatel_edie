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

#include "novatel_edie/decoders/oem/common.hpp"

using namespace novatel::edie::oem;

// Register the OEM alignment function with the MessageDatabase at static initialization time
namespace {
const bool kRegisteredOemAlignment = [] {
    novatel::edie::MessageDatabase::RegisterAlignmentFunction("OEM", novatel::edie::oem::OemAlignmentFunction);
    return true;
}();
} // namespace

// -------------------------------------------------------------------------------------------------------
MessageDecoder::MessageDecoder(const MessageDatabase::Ptr& pclMessageDb_) : MessageDecoderBase("OEM", pclMessageDb_, OemAlignmentFunction)
{
    InitOemFieldMaps();
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoder::InitOemFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("c")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        // TODO: check that the character is printable
        // if (!isprint(**ppcToken_)) { throw ... }
        const auto value = static_cast<uint32_t>(static_cast<unsigned char>(**ppcToken_));
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };
    asciiFieldMap[CalculateBlockCrc32("k")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCrc32("lk")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCrc32("ucb")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                   const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_,
                                                   const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const uint32_t value = static_cast<uint32_t>(std::bitset<8>(*ppcToken_).to_ulong());
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("T")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        double value;
        std::from_chars_result result = std::from_chars(*ppcToken_, *ppcToken_ + tokenLength_, value);
        if (result.ec != std::errc()) { throw std::runtime_error("Failed to parse double value"); }
        const auto converted = static_cast<uint32_t>(std::llround(value * SEC_TO_MILLI_SEC));
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, converted); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, converted); }
    };

    asciiFieldMap[CalculateBlockCrc32("m")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const uint32_t value = pclMsgDb_.MsgNameToMsgId(std::string(*ppcToken_, tokenLength_));
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("id")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                  [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                  [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const auto* pcDelimiter = static_cast<const char*>(memchr(*ppcToken_, '+', tokenLength_));
        if (pcDelimiter == nullptr) { pcDelimiter = static_cast<const char*>(memchr(*ppcToken_, '-', tokenLength_)); }

        uint16_t usSlot;
        auto result = std::from_chars(*ppcToken_, *ppcToken_ + tokenLength_, usSlot);
        if (result.ec != std::errc()) { throw std::runtime_error("Failed to parse usSlot"); }

        int16_t sFreq = 0;
        if (pcDelimiter != nullptr)
        {
            const char* start = pcDelimiter + 1;
            const char* end = *ppcToken_ + tokenLength_;

            result = std::from_chars(start, end, sFreq);
            if (result.ec != std::errc()) { throw std::runtime_error("Failed to parse sFreq"); }

            if (*pcDelimiter == '-') { sFreq = -sFreq; }
        }

        const uint32_t uiSatId = usSlot | (sFreq << 16);
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, uiSatId); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, uiSatId); }
    };

    asciiFieldMap[CalculateBlockCrc32("R")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        // RXCONFIG in ASCII is always #COMMANDNAMEA
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(std::string_view(*ppcToken_ + 1, tokenLength_ - 2)); // + 1 to Skip the '#'
        const uint32_t value = pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0;
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("c")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("k")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("lk")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCrc32("ucb")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                  simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                  [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sValue;
        if (clJsonField_.get(sValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_->name + "'");
        }
        const auto value = static_cast<uint32_t>(std::bitset<8>(std::string(sValue)).to_ulong());
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("m")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sValue;
        if (clJsonField_.get(sValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_->name + "'");
        }
        const auto value = pclMsgDb_.MsgNameToMsgId(std::string(sValue));
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("T")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        double dValue;
        if (clJsonField_.get(dValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_->name + "'");
        }
        const auto value = static_cast<uint32_t>(std::llround(dValue * SEC_TO_MILLI_SEC));
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("id")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                 simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sTemp;
        if (clJsonField_.get(sTemp) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_->name + "'");
        }
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
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, uiSatId); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, uiSatId); }
    };

    jsonFieldMap[CalculateBlockCrc32("R")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sValue;
        if (clJsonField_.get(sValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_->name + "'");
        }
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(sValue);
        const auto value = pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0;
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
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
        responseIdDataType.name = DATA_TYPE::UINT;

        auto responseIdField = std::make_shared<EnumField>();
        responseIdField->name = "response_id";
        responseIdField->type = FIELD_TYPE::RESPONSE_ID;
        responseIdField->dataType = responseIdDataType;
        responseIdField->index = 0;
        responseIdField->length = 4;
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

        auto& responseFieldInfo = responseDefinition->fieldInfo[0]; // Responses do not use definition CRCs.
        responseFieldInfo.fixedFieldBytes = sizeof(uint32_t);
        responseFieldInfo.varFieldCount = 1;
        responseFieldInfo.messageOrderedFields = {responseIdField, responseStrField};

        pResponseDefinition = responseDefinition;
        return pResponseDefinition;
    }
    return MessageDecoderBase::GetMessageDefinition(stMetaData_);
}
