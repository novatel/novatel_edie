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

#include "novatel_edie/decoders/message_decoder.hpp"

#include <bitset>
#include <cmath>
#include <sstream>

using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
MessageDecoder::MessageDecoder(JsonReader* pclJsonDb_) : MessageDecoderBase(pclJsonDb_)
{
    InitOemFieldMaps();
    if (pclJsonDb_ != nullptr) { LoadJsonDb(pclJsonDb_); }
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoder::InitOemFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("%c")] = SimpleAsciiMapEntry<uint32_t>();
    asciiFieldMap[CalculateBlockCrc32("%k")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCrc32("%lk")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCrc32("%ucb")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                    char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                    [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(*ppcToken_).to_ulong()), pstMessageDataType_);
    };

    asciiFieldMap[CalculateBlockCrc32("%T")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::llround(strtod(*ppcToken_, nullptr) * SEC_TO_MILLI_SEC)), pstMessageDataType_);
    };

    asciiFieldMap[CalculateBlockCrc32("%m")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(pclMsgDb_->MsgNameToMsgId(std::string(*ppcToken_, tokenLength_)), pstMessageDataType_);
    };

    asciiFieldMap[CalculateBlockCrc32("%id")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                   char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                   [[maybe_unused]] JsonReader* pclMsgDb_) {
        auto* pcDelimiter = static_cast<char*>(memchr(*ppcToken_, '+', tokenLength_));
        if (pcDelimiter == nullptr) { pcDelimiter = static_cast<char*>(memchr(*ppcToken_, '-', tokenLength_)); }

        uint16_t usSlot = static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 10));
        int16_t sFreq = pcDelimiter != nullptr ? static_cast<int16_t>(strtol(pcDelimiter, nullptr, 10)) : 0;

        const uint32_t uiSatId = usSlot | (sFreq << 16);
        vIntermediateFormat_.emplace_back(uiSatId, pstMessageDataType_);
    };

    asciiFieldMap[CalculateBlockCrc32("%R")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        // RXCONFIG in ASCII is always #COMMANDNAMEA
        const MessageDefinition* pclMessageDef = pclMsgDb_->GetMsgDef(std::string(*ppcToken_ + 1, tokenLength_ - 2)); // + 1 to Skip the '#'
        vIntermediateFormat_.emplace_back(pclMessageDef ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0, pstMessageDataType_);
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("%c")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("%k")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("%lk")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCrc32("%ucb")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                   json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(clJsonField_.get<std::string>().c_str()).to_ulong()),
                                          pstMessageDataType_);
    };

    jsonFieldMap[CalculateBlockCrc32("%m")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(pclMsgDb_->MsgNameToMsgId(clJsonField_.get<std::string>()), pstMessageDataType_);
    };

    jsonFieldMap[CalculateBlockCrc32("%T")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::llround(clJsonField_.get<double>() * SEC_TO_MILLI_SEC)), pstMessageDataType_);
    };

    jsonFieldMap[CalculateBlockCrc32("%id")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        auto sTemp(clJsonField_.get<std::string>());

        size_t sDelimiter = sTemp.find_last_of('+');
        if (sDelimiter == std::string::npos) { sDelimiter = sTemp.find_last_of('-'); }

        uint16_t usSlot;
        int16_t sFreq;

        if (sDelimiter != std::string::npos)
        {
            usSlot = static_cast<uint16_t>(strtoul(sTemp.substr(0, sDelimiter).c_str(), nullptr, 10));
            sFreq = static_cast<int16_t>(strtol(sTemp.substr(sDelimiter).c_str(), nullptr, 10));
        }
        else
        {
            usSlot = static_cast<uint16_t>(strtoul(sTemp.c_str(), nullptr, 10));
            sFreq = 0;
        }

        const uint32_t uiSatId = (usSlot | (sFreq << 16));
        vIntermediateFormat_.emplace_back(uiSatId, pstMessageDataType_);
    };

    jsonFieldMap[CalculateBlockCrc32("%R")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        const MessageDefinition* pclMessageDef = pclMsgDb_->GetMsgDef(clJsonField_.get<std::string>());
        vIntermediateFormat_.emplace_back(pclMessageDef ? CreateMsgId(pclMessageDef->logID, 0, 1, 0) : 0, pstMessageDataType_);
    };
}
