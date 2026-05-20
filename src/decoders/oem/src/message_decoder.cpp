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
MessageDecoder::MessageDecoder(const MessageDatabase::Ptr& pclMessageDb_) : MessageDecoderBase("OEM", pclMessageDb_) { InitOemFieldMaps(); }

// -------------------------------------------------------------------------------------------------------
void MessageDecoder::InitOemFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("c")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_,
                                                 const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        // TODO: check that the character is printable
        // if (!isprint(**ppcToken_)) { throw ... }
        const auto value = static_cast<uint32_t>(static_cast<unsigned char>(**ppcToken_));
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };
    asciiFieldMap[CalculateBlockCrc32("k")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCrc32("lk")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCrc32("ucb")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                   const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_,
                                                   const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const uint32_t value = static_cast<uint32_t>(std::bitset<8>(*ppcToken_).to_ulong());
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("T")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_,
                                                 const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        double value;
        std::from_chars_result result = std::from_chars(*ppcToken_, *ppcToken_ + tokenLength_, value);
        if (result.ec != std::errc()) { throw std::runtime_error("Failed to parse double value"); }
        const auto converted = static_cast<uint32_t>(std::llround(value * SEC_TO_MILLI_SEC));
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, converted); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, converted); }
    };

    asciiFieldMap[CalculateBlockCrc32("m")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_,
                                                 const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const uint32_t value = pclMsgDb_.MsgNameToMsgId(std::string(*ppcToken_, tokenLength_));
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("id")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                  const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_,
                                                  const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
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
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, uiSatId); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, uiSatId); }
    };

    asciiFieldMap[CalculateBlockCrc32("R")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                 const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_,
                                                 const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        // RXCONFIG in ASCII is always #COMMANDNAMEA
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(std::string_view(*ppcToken_ + 1, tokenLength_ - 2)); // + 1 to Skip the '#'
        const uint32_t value = pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0;
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("c")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("k")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("lk")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCrc32("ucb")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                  simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                  [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view bits;
        if (clJsonField_.get(bits) != simdjson::SUCCESS) { throw std::runtime_error("Invalid ucb JSON value"); }
        const auto value = static_cast<uint32_t>(std::bitset<8>(bits.data()).to_ulong());
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("m")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view msgName;
        if (clJsonField_.get(msgName) != simdjson::SUCCESS) { throw std::runtime_error("Invalid m JSON value"); }
        const auto value = pclMsgDb_.MsgNameToMsgId(std::string(msgName));
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("T")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        double seconds = 0;
        if (clJsonField_.get(seconds) != simdjson::SUCCESS) { throw std::runtime_error("Invalid T JSON value"); }
        const auto value = static_cast<uint32_t>(std::llround(seconds * SEC_TO_MILLI_SEC));
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };

    jsonFieldMap[CalculateBlockCrc32("id")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                 simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view sTemp;
        if (clJsonField_.get(sTemp) != simdjson::SUCCESS) { throw std::runtime_error("Invalid id JSON value"); }
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
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, uiSatId); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, uiSatId); }
    };

    jsonFieldMap[CalculateBlockCrc32("R")] = [](MessageBody& vIntermediateFormat_, const BaseField::ConstPtr& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        std::string_view msgName;
        if (clJsonField_.get(msgName) != simdjson::SUCCESS) { throw std::runtime_error("Invalid R JSON value"); }
        MessageDefinition::ConstPtr pclMessageDef = pclMsgDb_.GetMsgDef(msgName);
        const auto value = pclMessageDef != nullptr ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0;
        if (fixed_) { vIntermediateFormat_.SetFieldElement<true>(pstMessageDataType_->index, elementIndex_, value); }
        else { vIntermediateFormat_.SetFieldElement<false>(pstMessageDataType_->index, elementIndex_, value); }
    };
}
