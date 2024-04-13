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

#include "decoders/novatel/api/message_decoder.hpp"

#include <bitset>
#include <sstream>

using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
MessageDecoder::MessageDecoder(JsonReader* pclJsonDb_) : MessageDecoderBase(pclJsonDb_)
{
    InitOEMFieldMaps();
    if (pclJsonDb_ != nullptr) { LoadJsonDb(pclJsonDb_); }
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoder::InitOEMFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCRC32("%c")] = SimpleAsciiMapEntry<uint32_t>();
    asciiFieldMap[CalculateBlockCRC32("%k")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCRC32("%lk")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCRC32("%ucb")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                    char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                    [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(*ppcToken_).to_ulong()), MessageDataType_);
    };

    asciiFieldMap[CalculateBlockCRC32("%T")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtod(*ppcToken_, nullptr) * SEC_TO_MSEC), MessageDataType_);
    };

    asciiFieldMap[CalculateBlockCRC32("%m")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(pclMsgDb->MsgNameToMsgId(std::string(*ppcToken_, tokenLength_)), MessageDataType_);
    };

    asciiFieldMap[CalculateBlockCRC32("%id")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                   char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                   [[maybe_unused]] JsonReader* pclMsgDb) {
        uint16_t usSlot = 0;
        int16_t sFreq = 0;

        auto* pcDelimiter = static_cast<char*>(memchr(*ppcToken_, '+', tokenLength_));
        if (pcDelimiter == nullptr) { pcDelimiter = static_cast<char*>(memchr(*ppcToken_, '-', tokenLength_)); }

        if (pcDelimiter != nullptr)
        {
            usSlot = static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 10));
            sFreq = static_cast<int16_t>(strtol(pcDelimiter, nullptr, 10));
        }
        else { usSlot = static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 10)); }

        const uint32_t uiSatID = usSlot | (sFreq << 16);
        vIntermediateFormat_.emplace_back(uiSatID, MessageDataType_);
    };

    asciiFieldMap[CalculateBlockCRC32("%R")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        // RXCONFIG in ASCII is always #COMMANDNAMEA
        const MessageDefinition* pclMessageDef = pclMsgDb->GetMsgDef(std::string(*ppcToken_ + 1, tokenLength_ - 2)); // + 1 to Skip the '#'
        vIntermediateFormat_.emplace_back(pclMessageDef ? CreateMsgID(pclMessageDef->logID, 0, 1, 0) : 0, MessageDataType_);
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCRC32("%c")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCRC32("%k")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCRC32("%lk")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCRC32("%ucb")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                   json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(clJsonField_.get<std::string>().c_str()).to_ulong()),
                                          MessageDataType_);
    };

    jsonFieldMap[CalculateBlockCRC32("%m")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(pclMsgDb->MsgNameToMsgId(clJsonField_.get<std::string>()), MessageDataType_);
    };

    jsonFieldMap[CalculateBlockCRC32("%T")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(static_cast<uint32_t>(clJsonField_.get<double>() * SEC_TO_MSEC), MessageDataType_);
    };

    jsonFieldMap[CalculateBlockCRC32("%id")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        std::string sTemp(clJsonField_.get<std::string>());

        uint16_t usSlot = 0;
        int16_t sFreq = 0;

        size_t sDelimiter = sTemp.find_last_of('+');
        if (sDelimiter == std::string::npos) { sDelimiter = sTemp.find_last_of('-'); }

        if (sDelimiter != std::string::npos)
        {
            usSlot = static_cast<uint16_t>(strtoul(sTemp.substr(0, sDelimiter).c_str(), nullptr, 10));
            sFreq = static_cast<int16_t>(strtol(sTemp.substr(sDelimiter, sTemp.length()).c_str(), nullptr, 10));
        }
        else { usSlot = static_cast<uint16_t>(strtoul(sTemp.c_str(), nullptr, 10)); }

        const uint32_t uiSatID = (usSlot | (sFreq << 16));
        vIntermediateFormat_.emplace_back(uiSatID, MessageDataType_);
    };

    jsonFieldMap[CalculateBlockCRC32("%R")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        const MessageDefinition* pclMessageDef = pclMsgDb->GetMsgDef(clJsonField_.get<std::string>());
        vIntermediateFormat_.emplace_back(pclMessageDef ? CreateMsgID(pclMessageDef->logID, 0, 1, 0) : 0, MessageDataType_);
    };
}