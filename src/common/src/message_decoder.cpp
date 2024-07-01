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

#include "novatel_edie/common/message_decoder.hpp"

#include <bitset>

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
    InitEnumDefinitions();
    CreateResponseMsgDefinitions();
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> MessageDecoderBase::GetLogger() { return pclMyLogger; }

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::InitEnumDefinitions()
{
    vMyResponseDefinitions = pclMyMsgDb->GetEnumDefName("Responses");
    vMyCommandDefinitions = pclMyMsgDb->GetEnumDefName("Commands");
    vMyPortAddressDefinitions = pclMyMsgDb->GetEnumDefName("PortAddress");
    vMyGpsTimeStatusDefinitions = pclMyMsgDb->GetEnumDefName("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::InitFieldMaps()
{
    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    // asciiFieldMap[CalculateBlockCRC32("%c")] = SimpleAsciiMapEntry<int8_t>();
    asciiFieldMap[CalculateBlockCrc32("%hd")] = SimpleAsciiMapEntry<int16_t>();
    asciiFieldMap[CalculateBlockCrc32("%ld")] = SimpleAsciiMapEntry<int32_t>();
    asciiFieldMap[CalculateBlockCrc32("%lld")] = SimpleAsciiMapEntry<int64_t>();
    // asciiFieldMap[CalculateBlockCRC32("%uc")] = SimpleAsciiMapEntry<uint8_t>();
    asciiFieldMap[CalculateBlockCrc32("%hu")] = SimpleAsciiMapEntry<uint16_t>();
    asciiFieldMap[CalculateBlockCrc32("%lu")] = SimpleAsciiMapEntry<uint32_t>();
    asciiFieldMap[CalculateBlockCrc32("%llu")] = SimpleAsciiMapEntry<uint64_t>();
    asciiFieldMap[CalculateBlockCrc32("%lx")] = SimpleAsciiMapEntry<uint32_t, 16>();
    asciiFieldMap[CalculateBlockCrc32("%B")] = SimpleAsciiMapEntry<int8_t>();
    asciiFieldMap[CalculateBlockCrc32("%UB")] = SimpleAsciiMapEntry<uint8_t>();
    asciiFieldMap[CalculateBlockCrc32("%XB")] = SimpleAsciiMapEntry<uint8_t, 16>();
    asciiFieldMap[CalculateBlockCrc32("%lf")] = SimpleAsciiMapEntry<double>();
    asciiFieldMap[CalculateBlockCrc32("%le")] = SimpleAsciiMapEntry<double>();
    asciiFieldMap[CalculateBlockCrc32("%g")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCrc32("%lg")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCrc32("%e")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        switch (pstMessageDataType_->dataType.length)
        {
        case 4: vIntermediateFormat_.emplace_back(strtof(*ppcToken_, nullptr), pstMessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(strtod(*ppcToken_, nullptr), pstMessageDataType_); return;
        default: throw std::runtime_error("%e: invalid float length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("%f")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        switch (pstMessageDataType_->dataType.length)
        {
        case 4: vIntermediateFormat_.emplace_back(strtof(*ppcToken_, nullptr), pstMessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(strtod(*ppcToken_, nullptr), pstMessageDataType_); return;
        default: throw std::runtime_error("%f: invalid float length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("%d")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        if (pstMessageDataType_->dataType.name == DATA_TYPE::BOOL)
        {
            vIntermediateFormat_.emplace_back(std::string(*ppcToken_, tokenLength_) == "TRUE", pstMessageDataType_);
        }
        else { vIntermediateFormat_.emplace_back(static_cast<int32_t>(strtol(*ppcToken_, nullptr, 10)), pstMessageDataType_); }
    };

    asciiFieldMap[CalculateBlockCrc32("%u")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        switch (pstMessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(static_cast<uint8_t>(strtoul(*ppcToken_, nullptr, 10)), pstMessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 10)), pstMessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtoul(*ppcToken_, nullptr, 10)), pstMessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(static_cast<uint64_t>(strtoull(*ppcToken_, nullptr, 10)), pstMessageDataType_); return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("%x")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        switch (pstMessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(static_cast<uint8_t>(strtoul(*ppcToken_, nullptr, 16)), pstMessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(static_cast<uint16_t>(strtoul(*ppcToken_, nullptr, 16)), pstMessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtoul(*ppcToken_, nullptr, 16)), pstMessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(static_cast<uint64_t>(strtoull(*ppcToken_, nullptr, 16)), pstMessageDataType_); return;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("%X")] = asciiFieldMap[CalculateBlockCrc32("%x")];

    asciiFieldMap[CalculateBlockCrc32("%c")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                  char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                  [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<int8_t>(**ppcToken_), pstMessageDataType_);
    };

    asciiFieldMap[CalculateBlockCrc32("%uc")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                   char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                                                   [[maybe_unused]] JsonReader* pclMsgDb_) {
        vIntermediateFormat_.emplace_back(static_cast<uint8_t>(**ppcToken_), pstMessageDataType_);
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("%ld")] = SimpleJsonMapEntry<int32_t>();
    jsonFieldMap[CalculateBlockCrc32("%hd")] = SimpleJsonMapEntry<int16_t>();
    jsonFieldMap[CalculateBlockCrc32("%lld")] = SimpleJsonMapEntry<int64_t>();
    jsonFieldMap[CalculateBlockCrc32("%lu")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("%hu")] = SimpleJsonMapEntry<uint16_t>();
    jsonFieldMap[CalculateBlockCrc32("%llu")] = SimpleJsonMapEntry<uint64_t>();
    jsonFieldMap[CalculateBlockCrc32("%lx")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("%c")] = SimpleJsonMapEntry<int8_t>();
    jsonFieldMap[CalculateBlockCrc32("%uc")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCrc32("%B")] = SimpleJsonMapEntry<int8_t>();
    jsonFieldMap[CalculateBlockCrc32("%UB")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCrc32("%XB")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCrc32("%lf")] = SimpleJsonMapEntry<double>();
    jsonFieldMap[CalculateBlockCrc32("%e")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("%le")] = SimpleJsonMapEntry<double>();
    jsonFieldMap[CalculateBlockCrc32("%g")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("%lg")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCrc32("%f")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                 const json& clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        switch (pstMessageDataType_->dataType.length)
        {
        case 4: vIntermediateFormat_.emplace_back(clJsonField_.get<float>(), pstMessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(clJsonField_.get<double>(), pstMessageDataType_); return;
        default: throw std::runtime_error("invalid float length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("%d")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                 const json& clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        if (pstMessageDataType_->dataType.name == DATA_TYPE::BOOL)
        {
            vIntermediateFormat_.emplace_back(clJsonField_.get<bool>(), pstMessageDataType_);
        }
        else { vIntermediateFormat_.emplace_back(clJsonField_.get<int32_t>(), pstMessageDataType_); }
    };

    jsonFieldMap[CalculateBlockCrc32("%u")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                 const json& clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        switch (pstMessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), pstMessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(clJsonField_.get<uint16_t>(), pstMessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(clJsonField_.get<uint32_t>(), pstMessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(clJsonField_.get<uint64_t>(), pstMessageDataType_); return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("%x")] = [](std::vector<FieldContainer>& vIntermediateFormat_, const BaseField* pstMessageDataType_,
                                                 const json& clJsonField_, [[maybe_unused]] JsonReader* pclMsgDb_) {
        switch (pstMessageDataType_->dataType.length)
        {
        case 1: vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), pstMessageDataType_); return;
        case 2: vIntermediateFormat_.emplace_back(clJsonField_.get<uint16_t>(), pstMessageDataType_); return;
        case 4: vIntermediateFormat_.emplace_back(clJsonField_.get<uint32_t>(), pstMessageDataType_); return;
        case 8: vIntermediateFormat_.emplace_back(clJsonField_.get<uint64_t>(), pstMessageDataType_); return;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("%X")] = jsonFieldMap[CalculateBlockCrc32("%x")];
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::CreateResponseMsgDefinitions()
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
    if (vMyResponseDefinitions != nullptr) { stRespIdField.enumId = vMyResponseDefinitions->_id; }
    stRespIdField.enumDef = vMyResponseDefinitions;

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
    stMyRespDef.fields[0].push_back(stRespIdField.Clone());
    stMyRespDef.fields[0].push_back(stRespStrField.Clone());
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::SetLoggerLevel(const spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::ShutdownLogger() { Logger::Shutdown(); }

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::DecodeBinaryField(const BaseField* pstMessageDataType_, unsigned char** ppucLogBuf_,
                                           std::vector<FieldContainer>& vIntermediateFormat_) const
{
    switch (pstMessageDataType_->dataType.name)
    {
    case DATA_TYPE::BOOL: vIntermediateFormat_.emplace_back(*reinterpret_cast<bool*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint8_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::CHAR: vIntermediateFormat_.emplace_back(*reinterpret_cast<int8_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::USHORT: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint16_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::SHORT: vIntermediateFormat_.emplace_back(*reinterpret_cast<int16_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::UINT: [[fallthrough]];
    case DATA_TYPE::SATELLITEID: [[fallthrough]];
    case DATA_TYPE::ULONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint32_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::INT: [[fallthrough]];
    case DATA_TYPE::LONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<int32_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::ULONGLONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint64_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::LONGLONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<int64_t*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::FLOAT: vIntermediateFormat_.emplace_back(*reinterpret_cast<float*>(*ppucLogBuf_), pstMessageDataType_); break;
    case DATA_TYPE::DOUBLE: vIntermediateFormat_.emplace_back(*reinterpret_cast<double*>(*ppucLogBuf_), pstMessageDataType_); break;
    default: throw std::runtime_error("DecodeBinaryField(): Unknown field type\n");
    }

    *ppucLogBuf_ += pstMessageDataType_->dataType.length;
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::DecodeAsciiField(const BaseField* pstMessageDataType_, char** ppcToken_, const size_t tokenLength_,
                                          std::vector<FieldContainer>& vIntermediateFormat_) const
{
    const auto it = asciiFieldMap.find(pstMessageDataType_->conversionHash);
    if (it == asciiFieldMap.end()) { throw std::runtime_error("DecodeAsciiField(): Unknown field type\n"); }
    it->second(vIntermediateFormat_, pstMessageDataType_, ppcToken_, tokenLength_, pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::DecodeBinary(const std::vector<BaseField*>& vMsgDefFields_, unsigned char** ppucLogBuf_,
                                 std::vector<FieldContainer>& vIntermediateFormat_, const uint32_t uiMessageLength_) const
{
    const unsigned char* pucTempStart = *ppucLogBuf_;

    for (auto& field : vMsgDefFields_)
    {
        // Realign to type byte boundary if needed
        uint8_t usTypeAlignment = std::min(static_cast<uint16_t>(4), field->dataType.length);
        uint64_t usAlignmentOffset = static_cast<uint64_t>(*ppucLogBuf_ - pucTempStart) % usTypeAlignment;
        if (usAlignmentOffset != 0) { *ppucLogBuf_ += usTypeAlignment - usAlignmentOffset; }

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
            const uint32_t uiArraySize = dynamic_cast<const ArrayField*>(field)->arrayLength;
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().fieldValue);
            pvFieldContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i) { DecodeBinaryField(field, ppucLogBuf_, pvFieldContainer); }
            break;
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            const uint32_t uiArraySize = *reinterpret_cast<std::uint32_t*>(*ppucLogBuf_);
            *ppucLogBuf_ += sizeof(uint32_t);
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().fieldValue);
            pvFieldContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i) { DecodeBinaryField(field, ppucLogBuf_, pvFieldContainer); }
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
            const uint32_t uiArraySize = *reinterpret_cast<std::uint32_t*>(*ppucLogBuf_);
            *ppucLogBuf_ += sizeof(int32_t);
            auto* subFieldDefinitions = dynamic_cast<FieldArrayField*>(field);
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().fieldValue);
            pvFieldArrayContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
                auto& pvFieldContainer = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().fieldValue);
                pvFieldContainer.reserve(dynamic_cast<const FieldArrayField*>(field)->fields.size());
                STATUS eStatus = DecodeBinary(subFieldDefinitions->fields, ppucLogBuf_, pvFieldContainer,
                                              uiMessageLength_ - static_cast<uint32_t>(*ppucLogBuf_ - pucTempStart));
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            break;
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeBinary(): Unknown field type\n");
            throw std::runtime_error("DecodeBinary(): Unknown field type\n");
        }

        if (*ppucLogBuf_ - pucTempStart >= static_cast<int32_t>(uiMessageLength_)) { return STATUS::SUCCESS; }
    }

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
template <bool Abbreviated>
STATUS MessageDecoderBase::DecodeAscii(const std::vector<BaseField*>& vMsgDefFields_, char** ppcLogBuf_,
                                       std::vector<FieldContainer>& vIntermediateFormat_) const
{
    constexpr char cDelimiter1 = Abbreviated ? ' ' : ','; // TODO: give all these better names
    constexpr char cDelimiter2 = Abbreviated ? '\r' : '*';
    constexpr char cDelimiter3 = Abbreviated ? '\n' : '\0'; // TODO: might be able to get away without cDelimiter3, acDelimiter3
    constexpr char acDelimiter1[3] = {cDelimiter1, cDelimiter2, '\0'};
    constexpr char acDelimiter2[3] = {'\"', cDelimiter2, '\0'};
    constexpr char acDelimiter3[4] = {cDelimiter1, cDelimiter2, cDelimiter3, '\0'};
    constexpr char acDelimiterResponse[2] = {cDelimiter2, '\0'};

    char* pcBufEnd = *ppcLogBuf_ + strlen(*ppcLogBuf_);

    for (auto& field : vMsgDefFields_)
    {
        if (*ppcLogBuf_ >= pcBufEnd) { return STATUS::MALFORMED_INPUT; } // We encountered the end of the buffer unexpectedly

        size_t tokenLength = strcspn(*ppcLogBuf_, acDelimiter3); // TODO: do we need to use acDelimiter3?
        if (Abbreviated && ConsumeAbbrevFormatting(tokenLength, ppcLogBuf_)) { tokenLength = strcspn(*ppcLogBuf_, acDelimiter3); }

        // TODO: previously, we didn't do these malformed input checks in ascii, but I assume this was a bug
        if (Abbreviated && tokenLength == 0) { return STATUS::MALFORMED_INPUT; }

        bool bEarlyEndOfMessage = (*(*ppcLogBuf_ + tokenLength) == cDelimiter2);

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE:
            DecodeAsciiField(field, ppcLogBuf_, tokenLength, vIntermediateFormat_);
            *ppcLogBuf_ += tokenLength + 1;
            break;
        case FIELD_TYPE::ENUM: {
            auto sEnum = std::string(*ppcLogBuf_, tokenLength);
            const auto enumField = dynamic_cast<EnumField*>(field);
            switch (enumField->length)
            {
            case 1: vIntermediateFormat_.emplace_back(static_cast<uint8_t>(GetEnumValue(enumField->enumDef, sEnum)), field); break;
            case 2: vIntermediateFormat_.emplace_back(static_cast<uint16_t>(GetEnumValue(enumField->enumDef, sEnum)), field); break;
            default: vIntermediateFormat_.emplace_back(GetEnumValue(enumField->enumDef, sEnum), field); break;
            }
            *ppcLogBuf_ += tokenLength + 1;
            break;
        }
        case FIELD_TYPE::STRING:
            // Empty Field
            if (0 == strcspn(*ppcLogBuf_, ",*"))
            {
                vIntermediateFormat_.emplace_back("", field);
                *ppcLogBuf_ += 1;
            }
            // Quoted String
            else if (0 == strcspn(*ppcLogBuf_, "\""))
            {
                // If a field delimiter character is in the string, the previous tokenLength value is invalid.
                tokenLength = strcspn(*ppcLogBuf_ + 1, acDelimiter2); // Look for LAST '\"' character, skipping past the first.
                vIntermediateFormat_.emplace_back(std::string(*ppcLogBuf_ + 1, tokenLength), field); // + 1 to traverse opening double-quote.
                // Skip past the first '\"', string token and the remaining characters ('\"' and ',').
                *ppcLogBuf_ += 1 + tokenLength + strcspn(*ppcLogBuf_ + tokenLength, acDelimiter1);
            }
            // Unquoted String
            else
            {
                // String that isn't surrounded by quotes
                tokenLength = strcspn(*ppcLogBuf_, acDelimiter1); // Look for LAST '\"' or '*' character, skipping past the first.
                vIntermediateFormat_.emplace_back(std::string(*ppcLogBuf_, tokenLength), field); // +1 to traverse opening double-quote.
                // Skip past the first '\"', string token and the remaining characters ('\"' and ',').
                *ppcLogBuf_ += tokenLength + strcspn(*ppcLogBuf_ + tokenLength, acDelimiter1) + 1;
            }
            break;
        case FIELD_TYPE::RESPONSE_ID: {
            // Ensure we get the whole response (skip over delimiters in responses)
            tokenLength = strcspn(*ppcLogBuf_, acDelimiterResponse);
            std::string sResponse(*ppcLogBuf_, tokenLength);
            if (sResponse == "OK") { vIntermediateFormat_.emplace_back(1, field); }
            // Note: This won't match responses with format specifiers in them (%d, %s, etc.), they will be given id=0
            else { vIntermediateFormat_.emplace_back(GetResponseId(vMyResponseDefinitions, sResponse.substr(svErrorPrefix.length())), field); }
            // Do not advance buffer, need to reprocess this field for the following RESPONSE_STR.
            bEarlyEndOfMessage = false;
            break;
        }
        case FIELD_TYPE::RESPONSE_STR:
            // Response strings aren't surrounded by double quotes, ensure we get the whole response (skip over certain delimiters in responses)
            tokenLength = strcspn(*ppcLogBuf_, acDelimiterResponse);
            vIntermediateFormat_.emplace_back(std::string(*ppcLogBuf_, tokenLength), field);
            *ppcLogBuf_ += tokenLength + 1;
            break;
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            uint32_t uiArraySize = 0;
            if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY) { uiArraySize = dynamic_cast<const ArrayField*>(field)->arrayLength; }
            else
            {
                uiArraySize = strtoul(*ppcLogBuf_, nullptr, 10);

                if (uiArraySize > dynamic_cast<const ArrayField*>(field)->arrayLength)
                {
                    SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Array size too large. Malformed Input\n");
                    throw std::runtime_error("DecodeAscii(): Array size too large. Malformed Input\n");
                }

                *ppcLogBuf_ += tokenLength + 1;
                tokenLength = strcspn(*ppcLogBuf_, acDelimiter1);
            }

            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().fieldValue);
            pvFieldContainer.reserve(uiArraySize);

            const bool bPrintAsString = field->IsString();
            const bool bIsCommaSeparated = field->IsCsv();

            const char* pcPosition = *ppcLogBuf_;
            if (bPrintAsString)
            {
                // Ensure we grabbed the whole string, it might contain delimiters
                tokenLength = strcspn(*ppcLogBuf_ + 1, acDelimiter2);
                tokenLength += 2; // Add the back in the quotes so we process them
                pcPosition++;     // Start of string, skip first double-quote
            }

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                if (field->conversionHash == CalculateBlockCrc32("%Z"))
                {
                    uint32_t uiValueRead = 0;
                    if (sscanf(pcPosition, "%02x", &uiValueRead) != 1)
                    {
                        SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Error decoding %Z Array");
                        throw std::runtime_error("DecodeAscii(): Error decoding %Z Array");
                    }
                    pcPosition += 2;
                    pvFieldContainer.emplace_back(static_cast<uint8_t>(uiValueRead), field);
                }
                // End of string, remove trailing double-quote
                else if (bPrintAsString && strncmp("\"", pcPosition, 1) == 0)
                {
                    for (uint32_t j = 0; j < uiArraySize - i; j++) { pvFieldContainer.emplace_back(static_cast<uint8_t>(0), field); }
                    break;
                }
                // Escaped '\' character
                else if (strncmp("\\\\", pcPosition, 2) == 0)
                {
                    pcPosition++; // Consume the escape char
                    pvFieldContainer.emplace_back(static_cast<uint8_t>(*pcPosition), field);
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
                    pvFieldContainer.emplace_back(static_cast<uint8_t>(uiValueRead), field);
                }
                else
                {
                    // Ascii character
                    if (!bIsCommaSeparated)
                    {
                        pvFieldContainer.emplace_back(static_cast<uint8_t>(*pcPosition), field);
                        pcPosition++; // Consume 1 char
                    }
                    // Simple type
                    else
                    {
                        tokenLength = strcspn(*ppcLogBuf_, acDelimiter1);
                        DecodeAsciiField(field, ppcLogBuf_, tokenLength, pvFieldContainer);
                        *ppcLogBuf_ += tokenLength + 1;
                    }
                }
            }
            if (!bIsCommaSeparated) { *ppcLogBuf_ += tokenLength + 1; }
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            const uint32_t uiArraySize = strtoul(*ppcLogBuf_, ppcLogBuf_, 10);
            ++*ppcLogBuf_;
            const auto* subFieldDefinitions = dynamic_cast<FieldArrayField*>(field);

            if (uiArraySize > subFieldDefinitions->arrayLength)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Array size too large. Malformed Input\n");
                throw std::runtime_error("DecodeAscii(): Array size too large. Malformed Input\n");
            }

            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().fieldValue);
            pvFieldArrayContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
                auto& pvSubFieldContainer = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().fieldValue);
                pvSubFieldContainer.reserve(dynamic_cast<const FieldArrayField*>(field)->fields.size());
                STATUS eStatus = DecodeAscii<Abbreviated>(subFieldDefinitions->fields, ppcLogBuf_, pvSubFieldContainer);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            break;
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Unknown field type");
            throw std::runtime_error("DecodeAscii(): Unknown field type");
        }

        // TODO: previously, we didn't check for early end in abbreviated ascii, but I assume this was a bug
        if (Abbreviated == false && bEarlyEndOfMessage) { break; }
    }

    return STATUS::SUCCESS;
}

// explicit template instantiations
template STATUS MessageDecoderBase::DecodeAscii<true>(const std::vector<BaseField*>&, char**, std::vector<FieldContainer>&) const;
template STATUS MessageDecoderBase::DecodeAscii<false>(const std::vector<BaseField*>&, char**, std::vector<FieldContainer>&) const;

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::DecodeJson(const std::vector<BaseField*>& vMsgDefFields_, json clJsonFields_,
                               std::vector<FieldContainer>& vIntermediateFormat_) const
{
    for (auto& field : vMsgDefFields_)
    {
        json clField = clJsonFields_[field->name];

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE: DecodeJsonField(field, clField, vIntermediateFormat_); break;
        case FIELD_TYPE::ENUM:
            vIntermediateFormat_.emplace_back(GetEnumValue(dynamic_cast<EnumField*>(field)->enumDef, clField.get<std::string>()), field);
            break;
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR: vIntermediateFormat_.emplace_back(clField.get<std::string>(), field); break;
        case FIELD_TYPE::RESPONSE_ID: {
            auto sResponse(clField.get<std::string>());
            if (sResponse == "OK") { vIntermediateFormat_.emplace_back(clField.get<std::string>(), field); }
            // Note: This won't match responses with format specifiers in them (%d, %s, etc.), they will be given id=0
            else { vIntermediateFormat_.emplace_back(GetResponseId(vMyResponseDefinitions, sResponse.substr(svErrorPrefix.length())), field); }
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().fieldValue);

            if (field->IsString())
            {
                pvFieldContainer.reserve(clField.get<std::string>().size());
                for (const char& cValRead : clField.get<std::string>()) { pvFieldContainer.emplace_back(static_cast<uint8_t>(cValRead), field); }
            }
            else
            {
                pvFieldContainer.reserve(clField.size());
                for (const auto& it : clField)
                {
                    if (field->conversionHash == CalculateBlockCrc32("%Z")) { pvFieldContainer.emplace_back(it.get<uint8_t>(), field); }
                    else if (field->conversionHash == CalculateBlockCrc32("%P")) { pvFieldContainer.emplace_back(it.get<int8_t>(), field); }
                }
            }
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            const auto uiArraySize = static_cast<uint32_t>(clField.size());
            const auto* subFieldDefinitions = dynamic_cast<FieldArrayField*>(field);
            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().fieldValue);
            pvFieldArrayContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
                auto& pvSubFieldContainer = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().fieldValue);
                pvSubFieldContainer.reserve(dynamic_cast<const FieldArrayField*>(field)->fields.size());
                STATUS eStatus = DecodeJson(subFieldDefinitions->fields, clField[i], pvSubFieldContainer);
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
void MessageDecoderBase::DecodeJsonField(const BaseField* pstMessageDataType_, const json& clJsonField_,
                                         std::vector<FieldContainer>& vIntermediateFormat_) const
{
    const auto it = jsonFieldMap.find(pstMessageDataType_->conversionHash);
    if (it == jsonFieldMap.end()) { throw std::runtime_error("DecodeJsonField(): Unknown field type\n"); }
    it->second(vIntermediateFormat_, pstMessageDataType_, clJsonField_, pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::Decode(unsigned char* pucMessage_, std::vector<FieldContainer>& stInterMessage_, MetaDataBase& stMetaData_) const
{
    if (pucMessage_ == nullptr) { return STATUS::NULL_PROVIDED; }

    unsigned char* pucTempInData = pucMessage_;
    const MessageDefinition* vMsgDef;
    const std::vector<BaseField*>* pvCurrentMsgFields;

    if (stMetaData_.bResponse)
    {
        if (stMetaData_.eFormat != HEADER_FORMAT::BINARY && stMetaData_.eFormat != HEADER_FORMAT::SHORT_BINARY &&
            stMetaData_.eFormat != HEADER_FORMAT::ASCII && stMetaData_.eFormat != HEADER_FORMAT::SHORT_ASCII &&
            stMetaData_.eFormat != HEADER_FORMAT::SHORT_ABB_ASCII)
        {
            return STATUS::NO_DEFINITION;
        }
        vMsgDef = &stMyRespDef;
        pvCurrentMsgFields = &vMsgDef->fields.at(0);
    }
    else
    {
        if (!pclMyMsgDb) { return STATUS::NO_DATABASE; }

        vMsgDef = pclMyMsgDb->GetMsgDef(stMetaData_.usMessageId);

        if (!vMsgDef)
        {
            pclMyLogger->warn("No log definition for ID {}", stMetaData_.usMessageId);
            return STATUS::NO_DEFINITION;
        }

        pvCurrentMsgFields = vMsgDef->GetMsgDefFromCrc(pclMyLogger, stMetaData_.uiMessageCrc);
    }

    // Expand the intermediate format vector to prevent the copy constructor from being called when the vector grows in size
    stInterMessage_.clear();
    stInterMessage_.reserve(pvCurrentMsgFields->size());

    // Decode the detected format
    return stMetaData_.eFormat == HEADER_FORMAT::ASCII || stMetaData_.eFormat == HEADER_FORMAT::SHORT_ASCII
               ? DecodeAscii<false>(*pvCurrentMsgFields, reinterpret_cast<char**>(&pucTempInData), stInterMessage_)
           : stMetaData_.eFormat == HEADER_FORMAT::ABB_ASCII || stMetaData_.eFormat == HEADER_FORMAT::SHORT_ABB_ASCII
               ? DecodeAscii<true>(*pvCurrentMsgFields, reinterpret_cast<char**>(&pucTempInData), stInterMessage_)
           : stMetaData_.eFormat == HEADER_FORMAT::BINARY || stMetaData_.eFormat == HEADER_FORMAT::SHORT_BINARY
               ? DecodeBinary(*pvCurrentMsgFields, &pucTempInData, stInterMessage_, stMetaData_.uiBinaryMsgLength)
           : stMetaData_.eFormat == HEADER_FORMAT::JSON ? DecodeJson(*pvCurrentMsgFields, json::parse(pucTempInData)["body"], stInterMessage_)
                                                        : STATUS::UNKNOWN;
}
