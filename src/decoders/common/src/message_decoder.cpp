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

#include "decoders/common/api/message_decoder.hpp"

#include <bitset>
#include <sstream>

using namespace novatel::edie;

// -------------------------------------------------------------------------------------------------------
MessageDecoderBase::MessageDecoderBase(JsonReader* pclJsonDb_)
{
    InitFieldMaps();
    if (pclJsonDb_ != nullptr) { LoadJsonDb(pclJsonDb_); }
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::LoadJsonDb(JsonReader* pclJsonDb_)
{
    pclMyMsgDb = pclJsonDb_;
    InitEnumDefns();
    CreateResponseMsgDefns();
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> MessageDecoderBase::GetLogger() { return pclMyLogger; }

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::InitEnumDefns()
{
    vMyRespDefns = pclMyMsgDb->GetEnumDefName("Responses");
    vMyCommandDefns = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddrDefns = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGPSTimeStatusDefns = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::InitFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    // asciiFieldMap[CalculateBlockCRC32("%c")] = SimpleAsciiMapEntry<int8_t>();
    asciiFieldMap[CalculateBlockCRC32("%hd")] = SimpleAsciiMapEntry<int16_t>();
    asciiFieldMap[CalculateBlockCRC32("%ld")] = SimpleAsciiMapEntry<int32_t>();
    asciiFieldMap[CalculateBlockCRC32("%lld")] = SimpleAsciiMapEntry<int64_t>();
    // asciiFieldMap[CalculateBlockCRC32("%uc")] = SimpleAsciiMapEntry<uint8_t>();
    asciiFieldMap[CalculateBlockCRC32("%hu")] = SimpleAsciiMapEntry<uint16_t>();
    asciiFieldMap[CalculateBlockCRC32("%lu")] = SimpleAsciiMapEntry<uint32_t>();
    asciiFieldMap[CalculateBlockCRC32("%llu")] = SimpleAsciiMapEntry<uint64_t>();
    asciiFieldMap[CalculateBlockCRC32("%lx")] = SimpleAsciiMapEntry<uint32_t, 16>();
    asciiFieldMap[CalculateBlockCRC32("%B")] = SimpleAsciiMapEntry<int8_t>();
    asciiFieldMap[CalculateBlockCRC32("%UB")] = SimpleAsciiMapEntry<uint8_t>();
    asciiFieldMap[CalculateBlockCRC32("%XB")] = SimpleAsciiMapEntry<uint8_t, 16>();
    asciiFieldMap[CalculateBlockCRC32("%lf")] = SimpleAsciiMapEntry<double>();
    asciiFieldMap[CalculateBlockCRC32("%e")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCRC32("%le")] = SimpleAsciiMapEntry<double>();
    asciiFieldMap[CalculateBlockCRC32("%g%")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCRC32("%lg")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCRC32("%f")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        switch (MessageDataType_->dataType.length)
        {
        case 4: vIntermediateFormat_.emplace_back(strtof(*ppcToken_, nullptr), MessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(strtod(*ppcToken_, nullptr), MessageDataType_); return;
        default: throw std::runtime_error("invalid float length");
        }
    };

    asciiFieldMap[CalculateBlockCRC32("%d")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        if (MessageDataType_->dataType.name == DATA_TYPE::BOOL)
            vIntermediateFormat_.emplace_back(std::string(*ppcToken_, tokenLength_) == "TRUE", MessageDataType_);
        else
            vIntermediateFormat_.emplace_back(static_cast<int32_t>(strtol(*ppcToken_, nullptr, 10)), MessageDataType_);
    };

    asciiFieldMap[CalculateBlockCRC32("%u")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        switch (MessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(static_cast<uint8_t>(strtoul(*ppcToken_, nullptr, 10)), MessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 10)), MessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtoul(*ppcToken_, nullptr, 10)), MessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(static_cast<uint64_t>(strtoull(*ppcToken_, nullptr, 10)), MessageDataType_); return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    asciiFieldMap[CalculateBlockCRC32("%x")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        switch (MessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(static_cast<uint8_t>(strtoul(*ppcToken_, nullptr, 16)), MessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 16)), MessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtoul(*ppcToken_, nullptr, 16)), MessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(static_cast<uint64_t>(strtoull(*ppcToken_, nullptr, 16)), MessageDataType_); return;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    asciiFieldMap[CalculateBlockCRC32("%X")] = asciiFieldMap[CalculateBlockCRC32("%x")];

    asciiFieldMap[CalculateBlockCRC32("%c")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(static_cast<int8_t>(**ppcToken_), MessageDataType_);
    };

    asciiFieldMap[CalculateBlockCRC32("%uc")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                   char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                   [[maybe_unused]] JsonReader* pclMsgDb) {
        vIntermediateFormat_.emplace_back(static_cast<uint8_t>(**ppcToken_), MessageDataType_);
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCRC32("%ld")] = SimpleJsonMapEntry<int32_t>();
    jsonFieldMap[CalculateBlockCRC32("%hd")] = SimpleJsonMapEntry<int16_t>();
    jsonFieldMap[CalculateBlockCRC32("%lld")] = SimpleJsonMapEntry<int64_t>();
    jsonFieldMap[CalculateBlockCRC32("%lu")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCRC32("%hu")] = SimpleJsonMapEntry<uint16_t>();
    jsonFieldMap[CalculateBlockCRC32("%llu")] = SimpleJsonMapEntry<uint64_t>();
    jsonFieldMap[CalculateBlockCRC32("%lx")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCRC32("%c")] = SimpleJsonMapEntry<int8_t>();
    jsonFieldMap[CalculateBlockCRC32("%uc")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCRC32("%B")] = SimpleJsonMapEntry<int8_t>();
    jsonFieldMap[CalculateBlockCRC32("%UB")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCRC32("%XB")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCRC32("%lf")] = SimpleJsonMapEntry<double>();
    jsonFieldMap[CalculateBlockCRC32("%e")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCRC32("%le")] = SimpleJsonMapEntry<double>();
    jsonFieldMap[CalculateBlockCRC32("%g")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCRC32("%lg")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCRC32("%f")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        switch (MessageDataType_->dataType.length)
        {
        case 4: vIntermediateFormat_.emplace_back(clJsonField_.get<float>(), MessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(clJsonField_.get<double>(), MessageDataType_); return;
        default: throw std::runtime_error("invalid float length");
        }
    };

    jsonFieldMap[CalculateBlockCRC32("%d")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        if (MessageDataType_->dataType.name == DATA_TYPE::BOOL)
            vIntermediateFormat_.emplace_back(clJsonField_.get<bool>(), MessageDataType_);
        else
            vIntermediateFormat_.emplace_back(clJsonField_.get<int32_t>(), MessageDataType_);
    };

    jsonFieldMap[CalculateBlockCRC32("%u")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        switch (MessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), MessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(clJsonField_.get<uint16_t>(), MessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(clJsonField_.get<uint32_t>(), MessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(clJsonField_.get<uint64_t>(), MessageDataType_); return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    jsonFieldMap[CalculateBlockCRC32("%x")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* MessageDataType_,
                                                 json clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb) {
        switch (MessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), MessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(clJsonField_.get<uint16_t>(), MessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(clJsonField_.get<uint32_t>(), MessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(clJsonField_.get<uint64_t>(), MessageDataType_); return;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    jsonFieldMap[CalculateBlockCRC32("%X")] = jsonFieldMap[CalculateBlockCRC32("%x")];
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::CreateResponseMsgDefns()
{
    // Numerical response ID
    SimpleDataType stRespIdDataType;
    stRespIdDataType.description = "Response as numerical id";
    stRespIdDataType.length = 4;
    stRespIdDataType.name = DATA_TYPE::UINT;

    EnumField stRespIdField;
    stRespIdField.name = "response_id";
    stRespIdField.type = FIELD_TYPE::RESPONSE_ID;
    stRespIdField.dataType = stRespIdDataType;
    if (vMyRespDefns != nullptr) stRespIdField.enumID = vMyRespDefns->_id;
    stRespIdField.enumDef = vMyRespDefns;

    // String response ID
    SimpleDataType stRespStrDataType;
    stRespStrDataType.description = "Response as a string";
    stRespStrDataType.length = 1;
    stRespStrDataType.name = DATA_TYPE::CHAR;

    BaseField stRespStrField;
    stRespStrField.name = "response_str";
    stRespStrField.type = FIELD_TYPE::RESPONSE_STR;
    stRespStrField.dataType = stRespStrDataType;

    // Message Definition
    stMyRespDef = MessageDefinition();
    stMyRespDef.name = std::string("response");
    stMyRespDef.fields[0]; // responses don't have CRCs, hardcoding in 0 as the key to the fields map
    stMyRespDef.fields[0].push_back(stRespIdField.clone());
    stMyRespDef.fields[0].push_back(stRespStrField.clone());
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::SetLoggerLevel(spdlog::level::level_enum eLevel_) { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::ShutdownLogger() { Logger::Shutdown(); }

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::DecodeBinaryField(const BaseField* MessageDataType_, unsigned char** ppucLogBuf_,
                                           std::vector<FieldContainer>& vIntermediateFormat_) const
{
    switch (MessageDataType_->dataType.name)
    {
    case DATA_TYPE::BOOL: vIntermediateFormat_.emplace_back(*reinterpret_cast<bool*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint8_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::CHAR: vIntermediateFormat_.emplace_back(*reinterpret_cast<int8_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::USHORT: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint16_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::SHORT: vIntermediateFormat_.emplace_back(*reinterpret_cast<int16_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::UINT: [[fallthrough]];
    case DATA_TYPE::SATELLITEID: [[fallthrough]];
    case DATA_TYPE::ULONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint32_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::INT: [[fallthrough]];
    case DATA_TYPE::LONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<int32_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::ULONGLONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint64_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::LONGLONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<int64_t*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::FLOAT: vIntermediateFormat_.emplace_back(*reinterpret_cast<float*>(*ppucLogBuf_), MessageDataType_); break;
    case DATA_TYPE::DOUBLE: vIntermediateFormat_.emplace_back(*reinterpret_cast<double*>(*ppucLogBuf_), MessageDataType_); break;
    default: throw std::runtime_error("DecodeBinaryField(): Unknown field type\n");
    }

    *ppucLogBuf_ += MessageDataType_->dataType.length;
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::DecodeAsciiField(const BaseField* MessageDataType_, char** ppcToken_, const size_t tokenLength_,
                                          std::vector<FieldContainer>& vIntermediateFormat_) const
{
    auto it = asciiFieldMap.find(MessageDataType_->conversionHash);
    if (it == asciiFieldMap.end()) { throw std::runtime_error("DecodeAsciiField(): Unknown field type\n"); }
    it->second(vIntermediateFormat_, MessageDataType_, ppcToken_, tokenLength_, pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::DecodeBinary(const std::vector<BaseField*> MsgDefFields_, unsigned char** ppucLogBuf_,
                                 std::vector<FieldContainer>& vIntermediateFormat_, uint32_t uiMessageLength_) const
{
    unsigned char* pucTempStart = *ppucLogBuf_;
    for (auto& field : MsgDefFields_)
    {
        // Realign to type byte boundary if needed
        uint8_t usTypeAlignment = std::min(static_cast<uint16_t>(4), field->dataType.length);
        if (reinterpret_cast<uint64_t>(*ppucLogBuf_) % usTypeAlignment != 0)
        {
            *ppucLogBuf_ += usTypeAlignment - (reinterpret_cast<uint64_t>(*ppucLogBuf_) % usTypeAlignment);
        }

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE: DecodeBinaryField(field, ppucLogBuf_, vIntermediateFormat_); break;
        case FIELD_TYPE::ENUM:
            switch (field->dataType.length)
            {
            case 2: vIntermediateFormat_.emplace_back(*reinterpret_cast<std::int16_t*>(*ppucLogBuf_), field); break;
            case 4: vIntermediateFormat_.emplace_back(*reinterpret_cast<std::int32_t*>(*ppucLogBuf_), field); break;
            default:
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeBinary(): Invalid field length\n");
                throw std::runtime_error("DecodeBinary(): Invalid field length\n");
            }
            *ppucLogBuf_ += field->dataType.length;
            break;
        case FIELD_TYPE::RESPONSE_ID:
            vIntermediateFormat_.emplace_back(*reinterpret_cast<std::int32_t*>(*ppucLogBuf_), field);
            *ppucLogBuf_ += sizeof(int32_t);
            break;
        case FIELD_TYPE::RESPONSE_STR: {
            std::string sTemp(reinterpret_cast<char*>(*ppucLogBuf_), uiMessageLength_ - sizeof(int32_t)); // Remove CRC
            vIntermediateFormat_.emplace_back(sTemp, field);
            // Binary response string is not null terminated or 4 byte aligned
            *ppucLogBuf_ += sTemp.size();
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            uint32_t uiArraySize = static_cast<const ArrayField*>(field)->arrayLength;
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFC.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i) { DecodeBinaryField(field, ppucLogBuf_, pvFC); }
            break;
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            auto uiArraySize = *reinterpret_cast<std::uint32_t*>(*ppucLogBuf_);
            *ppucLogBuf_ += sizeof(uint32_t);
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFC.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i) { DecodeBinaryField(field, ppucLogBuf_, pvFC); }
            break;
        }
        case FIELD_TYPE::STRING: {
            // This version of a string is different. It is hopefully null terminated.
            std::string sTemp(reinterpret_cast<char*>(*ppucLogBuf_));
            vIntermediateFormat_.emplace_back(sTemp, field);
            *ppucLogBuf_ += sTemp.size() + 1; // + 1 to consume the NULL at the end of the string. This is to maintain byte alignment.
            // TODO: what was this for? It breaks RXCOMMANDSB.GPS. Is 4 supposed to be usTypeAlignment instead?
            // if (reinterpret_cast<std::uint64_t>(*ppucLogBuf_) % 4 != 0) { *ppucLogBuf_ += 4 - reinterpret_cast<std::uint64_t>(*ppucLogBuf_) % 4; }
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            auto* puiArraySize = reinterpret_cast<std::uint32_t*>(*ppucLogBuf_);
            *ppucLogBuf_ += sizeof(int32_t);
            auto* sub_field_defs = static_cast<FieldArrayField*>(field);
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFieldArrayContainer.reserve(*puiArraySize);

            for (uint32_t i = 0; i < *puiArraySize; ++i)
            {
                pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
                auto& pvFC = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().field_value);
                pvFC.reserve(static_cast<const FieldArrayField*>(field)->fields.size());
                STATUS eStatus =
                    DecodeBinary(sub_field_defs->fields, ppucLogBuf_, pvFC, uiMessageLength_ - static_cast<uint32_t>(*ppucLogBuf_ - pucTempStart));
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            break;
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeBinary(): Unknown field type\n");
            throw std::runtime_error("DecodeBinary(): Unknown field type\n");
        }

        if (*ppucLogBuf_ - pucTempStart >= static_cast<int32_t>(uiMessageLength_)) { break; }
    }

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
template <bool ABBREVIATED>
STATUS MessageDecoderBase::DecodeAscii(const std::vector<BaseField*> MsgDefFields_, char** ppucLogBuf_,
                                       std::vector<FieldContainer>& vIntermediateFormat_) const
{
    constexpr char cDelimiter1 = ABBREVIATED ? ' ' : ','; // TODO: give all these better names
    constexpr char cDelimiter2 = ABBREVIATED ? '\r' : '*';
    constexpr char cDelimiter3 = ABBREVIATED ? '\n' : '\0'; // TODO: might be able to get away without cDelimiter3, acDelimiter3
    constexpr char acDelimiter1[3] = {cDelimiter1, cDelimiter2, '\0'};
    constexpr char acDelimiter2[3] = {'\"', cDelimiter2, '\0'};
    constexpr char acDelimiter3[4] = {cDelimiter1, cDelimiter2, cDelimiter3, '\0'};
    constexpr char acDelimiterResponse[2] = {cDelimiter2, '\0'};

    for (auto& field : MsgDefFields_)
    {
        size_t tokenLength = strcspn(*ppucLogBuf_, acDelimiter3); // TODO: do we need to use acDelimiter3?
        if (ABBREVIATED && ConsumeAbbrevFormatting(tokenLength, ppucLogBuf_)) { tokenLength = strcspn(*ppucLogBuf_, acDelimiter3); }

        // TODO: previously, we didnt do these malformed input checks in ascii, but I assume this was a bug
        if (ABBREVIATED && tokenLength == 0) { return STATUS::MALFORMED_INPUT; }

        bool bEarlyEndOfMessage = (*(*ppucLogBuf_ + tokenLength) == cDelimiter2);

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE:
            DecodeAsciiField(field, ppucLogBuf_, tokenLength, vIntermediateFormat_);
            *ppucLogBuf_ += tokenLength + 1;
            break;
        case FIELD_TYPE::ENUM: {
            std::string sEnum = std::string(*ppucLogBuf_, tokenLength);
            const auto enumField = static_cast<EnumField*>(field);
            switch (enumField->length)
            {
            case 1: vIntermediateFormat_.emplace_back(static_cast<uint8_t>(GetEnumValue(enumField->enumDef, sEnum)), field); break;
            case 2: vIntermediateFormat_.emplace_back(static_cast<uint16_t>(GetEnumValue(enumField->enumDef, sEnum)), field); break;
            default: vIntermediateFormat_.emplace_back(GetEnumValue(enumField->enumDef, sEnum), field); break;
            }
            *ppucLogBuf_ += tokenLength + 1;
            break;
        }
        case FIELD_TYPE::STRING:
            // If a field delimiter character is in the string, the previous tokenLength value is invalid.
            tokenLength = strcspn(*ppucLogBuf_ + 1, acDelimiter2); // Look for LAST '\"' character, skipping past the first.
            vIntermediateFormat_.emplace_back(std::string(*ppucLogBuf_ + 1, tokenLength), field); // + 1 to traverse opening double-quote.
            // Skip past the first '\"', string token and the remaining characters ('\"' and ',').
            *ppucLogBuf_ += 1 + tokenLength + strcspn(*ppucLogBuf_ + tokenLength, acDelimiter1);
            break;
        case FIELD_TYPE::RESPONSE_ID: {
            // Ensure we get the whole response (skip over delimiters in responses)
            tokenLength = strcspn(*ppucLogBuf_, acDelimiterResponse);
            std::string sResponse(*ppucLogBuf_, tokenLength);
            if (sResponse == "OK") { vIntermediateFormat_.emplace_back(1, field); }
            // Note: This won't match responses with format specifiers in them (%d, %s, etc), they will be given id=0
            else { vIntermediateFormat_.emplace_back(GetResponseId(vMyRespDefns, sResponse.substr(svErrorPrefix.length())), field); }
            // Do not advance buffer, need to reprocess this field for the following RESPONSE_STR.
            bEarlyEndOfMessage = false;
            break;
        }
        case FIELD_TYPE::RESPONSE_STR:
            // Response strings aren't surrounded by double quotes, ensure we get the whole response (skip over certain delimiters in responses)
            tokenLength = strcspn(*ppucLogBuf_, acDelimiterResponse);
            vIntermediateFormat_.emplace_back(std::string(*ppucLogBuf_, tokenLength), field);
            *ppucLogBuf_ += tokenLength + 1;
            break;
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            uint32_t uiArraySize = 0;
            if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY) { uiArraySize = static_cast<const ArrayField*>(field)->arrayLength; }
            if (field->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
                uiArraySize = strtoul(*ppucLogBuf_, nullptr, 10);

                if (uiArraySize > static_cast<const ArrayField*>(field)->arrayLength)
                {
                    SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Array size too large. Malformed Input\n");
                    throw std::runtime_error("DecodeAscii(): Array size too large. Malformed Input\n");
                }

                *ppucLogBuf_ += tokenLength + 1;
                tokenLength = strcspn(*ppucLogBuf_, acDelimiter1);
            }

            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFC.reserve(uiArraySize);

            const bool bPrintAsString = field->isString();
            const bool bIsCommaSeperated = field->isCSV();

            char* pcPosition = *ppucLogBuf_;
            if (bPrintAsString)
            {
                // Ensure we grabbed the whole string, it might contain delimiters
                tokenLength = strcspn(*ppucLogBuf_ + 1, acDelimiter2);
                tokenLength += 2; // Add the back in the quotes so we process them
                pcPosition++;     // Start of string, skip first double-quote
            }

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                if (field->conversionHash == CalculateBlockCRC32("%Z"))
                {
                    uint32_t uiValueRead = 0;
                    if (sscanf(pcPosition, "%02x", &uiValueRead) != 1)
                    {
                        SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Error decoding %Z Array");
                        throw std::runtime_error("DecodeAscii(): Error decoding %Z Array");
                    }
                    pcPosition += 2;
                    pvFC.emplace_back(static_cast<uint8_t>(uiValueRead), field);
                }
                // End of string, remove trailing double-quote
                else if (bPrintAsString && strncmp("\"", pcPosition, 1) == 0)
                {
                    for (uint32_t j = 0; j < uiArraySize - i; j++) { pvFC.emplace_back(static_cast<uint8_t>(0), field); }
                    break;
                }
                // Escaped '\' character
                else if (strncmp("\\\\", pcPosition, 2) == 0)
                {
                    pcPosition++; // Consume the escape char
                    pvFC.emplace_back(static_cast<uint8_t>(*pcPosition), field);
                    pcPosition++; // Consume 1 char
                }
                // Non-ascii char in hex e.g. \x0C
                else if (strncmp("\\x", pcPosition, 2) == 0)
                {
                    pcPosition += 2; // Consume the '\x' that signifies hex without a char representation
                    uint32_t uiValueRead = 0;

                    if (sscanf(pcPosition, "%02x", &uiValueRead) != 1)
                    {
                        SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Error decoding %s array");
                        throw std::runtime_error("DecodeAscii(): Error decoding %s array");
                    }

                    pcPosition += 2; // Consume the hex output that is always 2 chars
                    pvFC.emplace_back(static_cast<uint8_t>(uiValueRead), field);
                }
                else
                {
                    // Ascii character
                    if (!bIsCommaSeperated)
                    {
                        pvFC.emplace_back(static_cast<uint8_t>(*pcPosition), field);
                        pcPosition++; // Consume 1 char
                    }
                    // Simple type
                    else
                    {
                        tokenLength = strcspn(*ppucLogBuf_, acDelimiter1);
                        DecodeAsciiField(field, ppucLogBuf_, tokenLength, pvFC);
                        *ppucLogBuf_ += tokenLength + 1;
                    }
                }
            }
            if (!bIsCommaSeperated) *ppucLogBuf_ += tokenLength + 1;
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            uint32_t uiArraySize = strtoul(*ppucLogBuf_, ppucLogBuf_, 10);
            ++*ppucLogBuf_;
            auto* sub_field_defs = static_cast<FieldArrayField*>(field);

            if (uiArraySize > sub_field_defs->arrayLength)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Array size too large. Malformed Input\n");
                throw std::runtime_error("DecodeAscii(): Array size too large. Malformed Input\n");
            }

            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFieldArrayContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
                auto& pvsubFC = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().field_value);
                pvsubFC.reserve(static_cast<const FieldArrayField*>(field)->fields.size());
                STATUS eStatus = DecodeAscii<ABBREVIATED>(sub_field_defs->fields, ppucLogBuf_, pvsubFC);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            break;
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Unknown field type");
            throw std::runtime_error("DecodeAscii(): Unknown field type");
        }

        // TODO: previously, we didnt check for early end in abbreviated ascii, but I assume this was a bug
        if (ABBREVIATED == false && bEarlyEndOfMessage) { break; }
    }

    return STATUS::SUCCESS;
}

// explicit template instantiations
template STATUS MessageDecoderBase::DecodeAscii<true>(const std::vector<BaseField*>, char**, std::vector<FieldContainer>&) const;
template STATUS MessageDecoderBase::DecodeAscii<false>(const std::vector<BaseField*>, char**, std::vector<FieldContainer>&) const;

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::DecodeJson(const std::vector<BaseField*> MsgDefFields_, json clJsonFields_,
                               std::vector<FieldContainer>& vIntermediateFormat_) const
{
    for (auto& field : MsgDefFields_)
    {
        json clField = clJsonFields_[field->name];

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE: DecodeJsonField(field, clField, vIntermediateFormat_); break;
        case FIELD_TYPE::ENUM:
            vIntermediateFormat_.emplace_back(GetEnumValue(static_cast<EnumField*>(field)->enumDef, clField.get<std::string>()), field);
            break;
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR: vIntermediateFormat_.emplace_back(clField.get<std::string>(), field); break;
        case FIELD_TYPE::RESPONSE_ID: {
            std::string sResponse(clField.get<std::string>());
            if (sResponse == "OK") { vIntermediateFormat_.emplace_back(clField.get<std::string>(), field); }
            // Note: This won't match responses with format specifiers in them (%d, %s, etc), they will be given id=0
            else { vIntermediateFormat_.emplace_back(GetResponseId(vMyRespDefns, sResponse.substr(svErrorPrefix.length())), field); }
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);

            if (field->isString())
            {
                pvFC.reserve(clField.get<std::string>().size());
                for (char& cValRead : clField.get<std::string>()) { pvFC.emplace_back(static_cast<uint8_t>(cValRead), field); }
            }
            else
            {
                pvFC.reserve(clField.size());
                for (const auto& it : clField)
                {
                    if (field->conversionHash == CalculateBlockCRC32("%Z")) { pvFC.emplace_back(it.get<uint8_t>(), field); }
                    else if (field->conversionHash == CalculateBlockCRC32("%P")) { pvFC.emplace_back(it.get<int8_t>(), field); }
                }
            }
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            auto uiArraySize = static_cast<uint32_t>(clField.size());
            auto* sub_field_defs = static_cast<FieldArrayField*>(field);
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFieldArrayContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
                auto& pvsubFC = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().field_value);
                pvsubFC.reserve((static_cast<const FieldArrayField*>(field))->fields.size());
                STATUS eStatus = DecodeJson(sub_field_defs->fields, clField[i], pvsubFC);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            break;
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeJson(): Unknown field type");
            throw std::runtime_error("DecodeJson(): Unknown field type");
        }
    }

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::DecodeJsonField(const BaseField* MessageDataType_, json clJsonField_,
                                         std::vector<FieldContainer>& vIntermediateFormat_) const
{
    auto it = jsonFieldMap.find(MessageDataType_->conversionHash);
    if (it == jsonFieldMap.end()) { throw std::runtime_error("DecodeJsonField(): Unknown field type\n"); }
    it->second(vIntermediateFormat_, MessageDataType_, clJsonField_, pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::Decode(unsigned char* pucInData_, IntermediateMessage& stInterMessage_, MetaDataBase& stMetaData_)
{
    if (pucInData_ == nullptr) { return STATUS::NULL_PROVIDED; }

    unsigned char* pucTempInData = pucInData_;
    const MessageDefinition* vMsgDef;
    const std::vector<BaseField*>* pvCurrentMsgFields;

    if (stMetaData_.bResponse)
    {
        if (stMetaData_.eFormat != HEADERFORMAT::BINARY && stMetaData_.eFormat != HEADERFORMAT::SHORT_BINARY &&
            stMetaData_.eFormat != HEADERFORMAT::ASCII && stMetaData_.eFormat != HEADERFORMAT::SHORT_ASCII &&
            stMetaData_.eFormat != HEADERFORMAT::SHORT_ABB_ASCII)
        {
            return STATUS::NO_DEFINITION;
        }
        vMsgDef = &stMyRespDef;
        pvCurrentMsgFields = &vMsgDef->fields.at(0);
    }
    else
    {
        if (!pclMyMsgDb) { return STATUS::NO_DATABASE; }

        vMsgDef = pclMyMsgDb->GetMsgDef(stMetaData_.usMessageID);

        if (!vMsgDef)
        {
            pclMyLogger->warn("No log definition for ID {}", stMetaData_.usMessageID);
            return STATUS::NO_DEFINITION;
        }

        pvCurrentMsgFields = vMsgDef->GetMsgDefFromCRC(pclMyLogger, stMetaData_.uiMessageCRC);
    }

    // Expand the intermediate format vector to prevent the copy constructor from being called when the vector grows in size
    stInterMessage_.clear();
    stInterMessage_.reserve(pvCurrentMsgFields->size());

    // Decode the detected format
    return stMetaData_.eFormat == HEADERFORMAT::ASCII || stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII
               ? DecodeAscii<false>(*pvCurrentMsgFields, reinterpret_cast<char**>(&pucTempInData), stInterMessage_)
           : stMetaData_.eFormat == HEADERFORMAT::ABB_ASCII || stMetaData_.eFormat == HEADERFORMAT::SHORT_ABB_ASCII
               ? DecodeAscii<true>(*pvCurrentMsgFields, reinterpret_cast<char**>(&pucTempInData), stInterMessage_)
           : stMetaData_.eFormat == HEADERFORMAT::BINARY || stMetaData_.eFormat == HEADERFORMAT::SHORT_BINARY
               ? DecodeBinary(*pvCurrentMsgFields, &pucTempInData, stInterMessage_, stMetaData_.uiBinaryMsgLength)
           : stMetaData_.eFormat == HEADERFORMAT::JSON ? DecodeJson(*pvCurrentMsgFields, json::parse(pucTempInData)["body"], stInterMessage_)
                                                       : STATUS::UNKNOWN;
}
