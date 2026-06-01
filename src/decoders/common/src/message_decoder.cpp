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

#include "novatel_edie/decoders/common/message_decoder.hpp"

#include <cassert>

#include <simdjson.h>

using namespace novatel::edie;

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::LoadJsonDb(MessageDatabase::Ptr pclMessageDb_)
{
    ValidateMessageDatabaseFamily(pclMyMsgDb, sMyExpectedMessageFamily, pclMyLogger);
    pclMyMsgDb = std::move(pclMessageDb_);
    InitEnumDefinitions();
}

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
    // asciiFieldMap[CalculateBlockCrc32("c")] = SimpleAsciiMapEntry<int8_t>();
    asciiFieldMap[CalculateBlockCrc32("hd")] = SimpleAsciiMapEntry<int16_t>();
    asciiFieldMap[CalculateBlockCrc32("ld")] = SimpleAsciiMapEntry<int32_t>();
    asciiFieldMap[CalculateBlockCrc32("lld")] = SimpleAsciiMapEntry<int64_t>();
    // asciiFieldMap[CalculateBlockCrc32("uc")] = SimpleAsciiMapEntry<uint8_t>();
    asciiFieldMap[CalculateBlockCrc32("hu")] = SimpleAsciiMapEntry<uint16_t>();
    asciiFieldMap[CalculateBlockCrc32("lu")] = SimpleAsciiMapEntry<uint32_t>();
    asciiFieldMap[CalculateBlockCrc32("llu")] = SimpleAsciiMapEntry<uint64_t>();
    asciiFieldMap[CalculateBlockCrc32("lx")] = SimpleAsciiMapEntry<uint32_t, 16>();
    asciiFieldMap[CalculateBlockCrc32("llx")] = SimpleAsciiMapEntry<uint64_t, 16>();
    asciiFieldMap[CalculateBlockCrc32("B")] = SimpleAsciiMapEntry<int8_t>();
    asciiFieldMap[CalculateBlockCrc32("UB")] = SimpleAsciiMapEntry<uint8_t>();
    asciiFieldMap[CalculateBlockCrc32("XB")] = SimpleAsciiMapEntry<uint8_t, 16>();
    asciiFieldMap[CalculateBlockCrc32("lf")] = SimpleAsciiMapEntry<double>();
    asciiFieldMap[CalculateBlockCrc32("le")] = SimpleAsciiMapEntry<double>();
    asciiFieldMap[CalculateBlockCrc32("g")] = SimpleAsciiMapEntry<float>();
    asciiFieldMap[CalculateBlockCrc32("lg")] = SimpleAsciiMapEntry<double>();

    asciiFieldMap[CalculateBlockCrc32("e")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 4:
            if (fixed_) { ParseAndEmplace<true, float>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, float>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        case 8:
            if (fixed_) { ParseAndEmplace<true, double>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, double>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        default: throw std::runtime_error("invalid float length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("f")] = asciiFieldMap[CalculateBlockCrc32("e")];

    asciiFieldMap[CalculateBlockCrc32("d")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        constexpr uint32_t nTrue = 4; // "TRUE"
        if (pstMessageDataType_.dataType.name == DATA_TYPE::BOOL)
        {
            const bool value = (tokenLength_ == nTrue);
            if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
            else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
        }
        else
        {
            if (fixed_) { ParseAndEmplace<true, int32_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, int32_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
        }
    };

    asciiFieldMap[CalculateBlockCrc32("u")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1:
            if (fixed_) { ParseAndEmplace<true, uint8_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint8_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        case 2:
            if (fixed_) { ParseAndEmplace<true, uint16_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint16_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        case 4:
            if (fixed_) { ParseAndEmplace<true, uint32_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint32_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        case 8:
            if (fixed_) { ParseAndEmplace<true, uint64_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint64_t>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("x")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1:
            if (fixed_) { ParseAndEmplace<true, uint8_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint8_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        case 2:
            if (fixed_) { ParseAndEmplace<true, uint16_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint16_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        case 4:
            if (fixed_) { ParseAndEmplace<true, uint32_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint32_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
        case 8:
            if (fixed_) { ParseAndEmplace<true, uint64_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, uint64_t, 16>(clMessageBody_, pstMessageDataType_, *ppcToken_, tokenLength_, elementIndex_); }
            return;
            ;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("X")] = asciiFieldMap[CalculateBlockCrc32("x")];

    asciiFieldMap[CalculateBlockCrc32("c")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                 [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                 [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const auto value = static_cast<int8_t>(**ppcToken_);
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("uc")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_, const char** ppcToken_,
                                                  [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                                                  [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        const auto value = static_cast<uint8_t>(static_cast<unsigned char>(**ppcToken_));
        if (fixed_) { clMessageBody_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clMessageBody_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    // =========================================================
    // Json Field Mapping
    // =========================================================
    jsonFieldMap[CalculateBlockCrc32("ld")] = SimpleJsonMapEntry<int32_t>();
    jsonFieldMap[CalculateBlockCrc32("hd")] = SimpleJsonMapEntry<int16_t>();
    jsonFieldMap[CalculateBlockCrc32("lld")] = SimpleJsonMapEntry<int64_t>();
    jsonFieldMap[CalculateBlockCrc32("lu")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("hu")] = SimpleJsonMapEntry<uint16_t>();
    jsonFieldMap[CalculateBlockCrc32("llu")] = SimpleJsonMapEntry<uint64_t>();
    jsonFieldMap[CalculateBlockCrc32("lx")] = SimpleJsonMapEntry<uint32_t>();
    jsonFieldMap[CalculateBlockCrc32("llx")] = SimpleJsonMapEntry<uint64_t>();
    jsonFieldMap[CalculateBlockCrc32("c")] = SimpleJsonMapEntry<int8_t>();
    jsonFieldMap[CalculateBlockCrc32("uc")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCrc32("B")] = SimpleJsonMapEntry<int8_t>();
    jsonFieldMap[CalculateBlockCrc32("UB")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCrc32("XB")] = SimpleJsonMapEntry<uint8_t>();
    jsonFieldMap[CalculateBlockCrc32("lf")] = SimpleJsonMapEntry<double>();
    jsonFieldMap[CalculateBlockCrc32("e")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("le")] = SimpleJsonMapEntry<double>();
    jsonFieldMap[CalculateBlockCrc32("g")] = SimpleJsonMapEntry<float>();
    jsonFieldMap[CalculateBlockCrc32("lg")] = SimpleJsonMapEntry<double>();

    jsonFieldMap[CalculateBlockCrc32("f")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 4: PushElement<float>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 8: PushElement<double>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        default: throw std::runtime_error("invalid float length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("d")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (pstMessageDataType_.dataType.name == DATA_TYPE::BOOL)
        {
            PushElement<bool>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_);
        }
        else { PushElement<int32_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); }
    };

    jsonFieldMap[CalculateBlockCrc32("u")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1: PushElement<uint8_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 2: PushElement<uint16_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 4: PushElement<uint32_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 8: PushElement<uint64_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("x")] = [](MessageBody& clMessageBody_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1: PushElement<uint8_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 2: PushElement<uint16_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 4: PushElement<uint32_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 8: PushElement<uint64_t>(clMessageBody_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("X")] = jsonFieldMap[CalculateBlockCrc32("x")];
}

// -------------------------------------------------------------------------------------------------------
template <bool Fixed>
void MessageDecoderBase::DecodeBinaryField(const BaseField& pstMessageDataType_, const unsigned char** ppucLogBuf_, MessageBody& clMessageBody_,
                                           size_t n)
{
    switch (pstMessageDataType_.dataType.name)
    {
    case DATA_TYPE::BOOL: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const bool*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const uint8_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::CHAR: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const int8_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::USHORT: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const uint16_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::SHORT: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const int16_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::UINT: [[fallthrough]];
    case DATA_TYPE::SATELLITEID: [[fallthrough]];
    case DATA_TYPE::ULONG: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const uint32_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::INT: [[fallthrough]];
    case DATA_TYPE::LONG: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const int32_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::ULONGLONG: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const uint64_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::LONGLONG: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const int64_t*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::FLOAT: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const float*>(*ppucLogBuf_), n); break;
    case DATA_TYPE::DOUBLE: clMessageBody_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const double*>(*ppucLogBuf_), n); break;
    default: throw std::runtime_error("DecodeBinaryField(): Unknown field type\n");
    }

    *ppucLogBuf_ += pstMessageDataType_.dataType.length * n;
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::DecodeBinary(const FieldInfo& vMsgDefFields_, const unsigned char** ppucLogBuf_, MessageBody& clMessageBody_,
                                 const uint32_t uiMessageLength_) const
{
    const unsigned char* pucTempStart = *ppucLogBuf_;

    for (const auto& field : vMsgDefFields_.messageOrderedFields)
    {
        auto fieldTypeLength = static_cast<uint8_t>(field->dataType.length);
        *ppucLogBuf_ += fMyAlignmentFunc(fieldTypeLength, reinterpret_cast<uintptr_t>(pucTempStart), reinterpret_cast<uintptr_t>(*ppucLogBuf_));

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE: DecodeBinaryField<true>(*field, ppucLogBuf_, clMessageBody_, 1); break;
        case FIELD_TYPE::ENUM:
            switch (field->dataType.length)
            {
            case 1: clMessageBody_.SetFieldValue<true>(field->index, LoadValueFromBuffer<uint8_t>(*ppucLogBuf_), 1); break;
            case 2: clMessageBody_.SetFieldValue<true>(field->index, LoadValueFromBuffer<uint16_t>(*ppucLogBuf_), 1); break;
            case 4: clMessageBody_.SetFieldValue<true>(field->index, LoadValueFromBuffer<uint32_t>(*ppucLogBuf_), 1); break;
            default:
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeBinary(): Invalid field length\n");
                throw std::runtime_error("DecodeBinary(): Invalid field length\n");
            }
            *ppucLogBuf_ += field->dataType.length;
            break;
        case FIELD_TYPE::RESPONSE_ID:
            clMessageBody_.SetFieldValue<true>(field->index, LoadValueFromBuffer<int32_t>(*ppucLogBuf_));
            *ppucLogBuf_ += sizeof(int32_t);
            break;
        case FIELD_TYPE::RESPONSE_STR: {
            std::string_view sTemp(reinterpret_cast<const char*>(*ppucLogBuf_), uiMessageLength_ - sizeof(int32_t)); // Remove CRC
            clMessageBody_.GetVarFields()[field->index] = std::string(sTemp);
            // Binary response string is not null terminated or 4 byte aligned
            *ppucLogBuf_ += sTemp.size();
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const uint32_t uiArraySize = dynamic_cast<const ArrayField*>(field.get())->arrayLength;
            DecodeBinaryField<true>(*field, ppucLogBuf_, clMessageBody_, uiArraySize);
            break;
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            const uint32_t uiArraySize = GetArrayLength(pucTempStart, ppucLogBuf_, dynamic_cast<const ArrayField&>(*field), clMessageBody_);
            DecodeBinaryField<false>(*field, ppucLogBuf_, clMessageBody_, uiArraySize);
            break;
        }
        case FIELD_TYPE::STRING: {
            // This version of a string is different. It is hopefully null terminated.
            std::string_view sTemp(reinterpret_cast<const char*>(*ppucLogBuf_));
            clMessageBody_.GetVarFields()[field->index] = std::string(sTemp);
            *ppucLogBuf_ += sTemp.size() + 1; // + 1 to consume the NULL at the end of the string.
            AddStringFieldPadding(pucTempStart, ppucLogBuf_);
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            const auto* subFieldDefinitions = dynamic_cast<const FieldArrayField*>(field.get());
            const uint32_t uiArraySize = GetArrayLength(pucTempStart, ppucLogBuf_, *subFieldDefinitions, clMessageBody_);

            if (subFieldDefinitions->fieldInfo.varFieldCount == 0)
            {
                const size_t totalBytes = uiArraySize * subFieldDefinitions->fieldInfo.fixedFieldBytes;
                // Store flat FIELD_ARRAY directly as std::vector<std::byte>
                std::vector<std::byte> flatFieldData(reinterpret_cast<const std::byte*>(*ppucLogBuf_),
                                                     reinterpret_cast<const std::byte*>(*ppucLogBuf_) + totalBytes);
                clMessageBody_.GetVarFields()[field->index] = std::move(flatFieldData);
                *ppucLogBuf_ += totalBytes;
            }
            else
            {
                clMessageBody_.GetVarFields()[field->index] = std::vector<MessageBody>(uiArraySize);
                auto& vFieldArrayContainer = std::get<std::vector<MessageBody>>(clMessageBody_.GetVarFields()[field->index]);
                for (uint32_t i = 0; i < uiArraySize; ++i)
                {
                    *ppucLogBuf_ +=
                        fMyAlignmentFunc(fieldTypeLength, reinterpret_cast<uintptr_t>(pucTempStart), reinterpret_cast<uintptr_t>(*ppucLogBuf_));
                    vFieldArrayContainer[i] =
                        MessageBody(subFieldDefinitions->fieldInfo.fixedFieldBytes, subFieldDefinitions->fieldInfo.varFieldCount);
                    STATUS eStatus = DecodeBinary(subFieldDefinitions->fieldInfo, ppucLogBuf_, vFieldArrayContainer[i],
                                                  uiMessageLength_ - static_cast<uint32_t>(*ppucLogBuf_ - pucTempStart));
                    if (eStatus != STATUS::SUCCESS) { return eStatus; }
                }
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
void MessageDecoderBase::DecodeAsciiField(const BaseField& field_, const char** ppcToken_, const size_t tokenLength_, MessageBody& clMessageBody_,
                                          const size_t elementIndex_, const bool fixed_) const
{
    const auto it = asciiFieldMap.find(field_.conversionHash);
    if (it == asciiFieldMap.end()) { throw std::runtime_error("DecodeAsciiField(): Unknown field type\n"); }
    it->second(clMessageBody_, field_, ppcToken_, tokenLength_, elementIndex_, fixed_, *pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
// Decode an ASCII array formatted with the %Z conversion string
static STATUS DecodeZConversionStringAsciiArray(std::vector<uint8_t>& vDecodedValues_, const char** ppcLogBuf_, uint32_t uiArraySize_)
{
    vDecodedValues_.resize(uiArraySize_);
    for (uint32_t i = 0; i < uiArraySize_; ++i)
    {
        uint32_t uiValueRead = 0;
        if (sscanf(*ppcLogBuf_, "%02x", &uiValueRead) != 1) { return STATUS::MALFORMED_INPUT; }
        *ppcLogBuf_ += 2;
        vDecodedValues_[i] = static_cast<uint8_t>(uiValueRead);
    }

    return STATUS::SUCCESS;
}

// Decode an ASCII array field that is not comma separated, e.g. a raw ASCII value or hex representation
template <typename CharType> static STATUS DecodeNonCommaSeparatedAsciiArrayField(CharType& cValue_, const char** ppcLogBuf_)
{
    // Escaped '\' character
    if (strncmp("\\\\", *ppcLogBuf_, 2) == 0)
    {
        *ppcLogBuf_ += 1;
        cValue_ = static_cast<CharType>(**ppcLogBuf_);
        *ppcLogBuf_ += 1; // Consume 1 char
    }
    // Non-ascii char in hex e.g. \x0C
    else if (strncmp("\\x", *ppcLogBuf_, 2) == 0)
    {
        *ppcLogBuf_ += 2; // Consume the '\x' that signifies hex without a char representation
        uint32_t uiValueRead = 0;
        if (sscanf(*ppcLogBuf_, "%02x", &uiValueRead) != 1) { return STATUS::MALFORMED_INPUT; }

        cValue_ = static_cast<CharType>(uiValueRead);
        *ppcLogBuf_ += 2;
    }
    // Ascii character
    else
    {
        cValue_ = static_cast<CharType>(**ppcLogBuf_);
        *ppcLogBuf_ += 1; // Consume 1 char
    }
    return STATUS::SUCCESS;
}

// Decode an ASCII array made up of non-comma separated values, e.g. a raw ASCII values or hex representations
template <typename CharType>
static STATUS DecodeNonCommaSeparatedAsciiArray(MessageBody& clMessageBody_, const char** ppcLogBuf_, const BaseField::ConstPtr& field_,
                                                uint32_t uiArraySize_, bool fixed_)
{
    std::vector<CharType> vDecodedValues(uiArraySize_);
    for (uint32_t i = 0; i < uiArraySize_; ++i)
    {
        STATUS eStatus = DecodeNonCommaSeparatedAsciiArrayField(vDecodedValues[i], ppcLogBuf_);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }
    }
    if (fixed_) { clMessageBody_.SetFieldValue<true>(field_->index, std::move(vDecodedValues)); }
    else { clMessageBody_.SetFieldValue<false>(field_->index, std::move(vDecodedValues)); }
    return STATUS::SUCCESS;
}

// Decode an ASCII array which is formatted like a string with opening and closing quotes, e.g. "string value"
static STATUS DecodeStringAsciiArray(std::string& sDecodedValue_, const char** ppcLogBuf_, uint32_t uiArraySize_)
{
    sDecodedValue_.assign(uiArraySize_, '\0');

    // Look for opening double-quote
    if (**ppcLogBuf_ != '\"') { return STATUS::MALFORMED_INPUT; }
    *ppcLogBuf_ += 1;

    for (uint32_t i = 0; i < uiArraySize_; ++i)
    {
        if (**ppcLogBuf_ == '\"') { break; }

        STATUS eStatus = DecodeNonCommaSeparatedAsciiArrayField(sDecodedValue_[i], ppcLogBuf_);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }
    }

    // Look for closing double-quote
    if (**ppcLogBuf_ != '\"') { return STATUS::MALFORMED_INPUT; }
    *ppcLogBuf_ += 1;

    return STATUS::SUCCESS;
}

template <bool Abbreviated>
STATUS MessageDecoderBase::DecodeAscii(const FieldInfo& vMsgDefFields_, const char** ppcLogBuf_, MessageBody& clMessageBody_,
                                       const char* pcBufEnd_) const
{
    constexpr char fieldDelim = Abbreviated ? ' ' : ',';
    constexpr char endDelim = Abbreviated ? '\r' : '*';

    // Null-terminated C-strings for strcspn
    constexpr std::array<char, 3> tokenDelimiters = {fieldDelim, endDelim, '\0'};
    constexpr std::array<char, 3> unquotedStringDelimiters = {fieldDelim, endDelim, '\0'};
    constexpr std::array<char, 3> quotedStringDelimiters = {'\"', endDelim, '\0'};
    constexpr std::array<char, 2> responseDelimiters = {endDelim, '\0'};

    if (pcBufEnd_ == nullptr) { pcBufEnd_ = static_cast<const char*>(std::memchr(*ppcLogBuf_, '\0', MAX_ASCII_MESSAGE_LENGTH)); }

    for (const auto& field : vMsgDefFields_.messageOrderedFields)
    {
        if (*ppcLogBuf_ >= pcBufEnd_) { return STATUS::MALFORMED_INPUT; } // We encountered the end of the buffer unexpectedly

        if constexpr (Abbreviated) { ConsumeAbbrevFormatting(ppcLogBuf_); }

        size_t tokenLength = strcspn(*ppcLogBuf_, tokenDelimiters.data());
        if (tokenLength == 0) { return STATUS::MALFORMED_INPUT; }

        bool bEarlyEndOfMessage = (*(*ppcLogBuf_ + tokenLength) == endDelim);

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE:
            try
            {
                DecodeAsciiField(*field, ppcLogBuf_, tokenLength, clMessageBody_);
            }
            catch (const std::runtime_error&)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAsciiField(): Exception when decoding field. Malformed Input\n");
                return STATUS::MALFORMED_INPUT;
            }
            *ppcLogBuf_ += tokenLength + 1;
            break;
        case FIELD_TYPE::ENUM: {
            std::string_view sEnum(*ppcLogBuf_, tokenLength);
            const auto* enumField = dynamic_cast<const EnumField*>(field.get());
            const uint32_t enumValue = GetEnumValue(enumField->enumDef, sEnum);
            switch (enumField->length)
            {
            case 1: clMessageBody_.SetFieldValue<true>(field->index, static_cast<uint8_t>(enumValue), 1); break;
            case 2: clMessageBody_.SetFieldValue<true>(field->index, static_cast<uint16_t>(enumValue), 1); break;
            default: clMessageBody_.SetFieldValue<true>(field->index, enumValue, 1); break;
            }
            *ppcLogBuf_ += tokenLength + 1;
            break;
        }
        case FIELD_TYPE::STRING:
            switch (**ppcLogBuf_)
            {
            case ',': [[fallthrough]];
            case '*':
                clMessageBody_.SetFieldValue<false>(field->index, std::string(""));
                *ppcLogBuf_ += 1;
                break;
            case '"':
                // If a field delimiter character is in the string, the previous tokenLength value is invalid.
                tokenLength = strcspn(*ppcLogBuf_ + 1, quotedStringDelimiters.data()); // Look for LAST '"' character, skipping past the first.
                clMessageBody_.SetFieldValue<false>(field->index, std::string(*ppcLogBuf_ + 1, tokenLength)); // + 1 to pass opening double-quote.
                // Skip past the first '"', string token and the remaining characters ('"' and ',').
                *ppcLogBuf_ += tokenLength + strcspn(*ppcLogBuf_ + tokenLength + 1, unquotedStringDelimiters.data()) + 2;
                break;
            default:
                // Unquoted string: initial tokenLength is the length of the string
                clMessageBody_.SetFieldValue<false>(field->index, std::string(*ppcLogBuf_, tokenLength));
                *ppcLogBuf_ += tokenLength + 1;
                break;
            }
            break;
        case FIELD_TYPE::RESPONSE_ID: {
            // Ensure we get the whole response (skip over delimiters in responses)
            tokenLength = strcspn(*ppcLogBuf_, responseDelimiters.data());
            std::string_view sResponse(*ppcLogBuf_, tokenLength);
            if (sResponse == "OK") { clMessageBody_.SetFieldValue<true>(field->index, 1); }
            // Note: This won't match responses with format specifiers in them (%d, %s, etc.), they will be given id=0
            else { clMessageBody_.SetFieldValue<true>(field->index, GetResponseId(vMyResponseDefinitions, sResponse.substr(svErrorPrefix.length()))); }
            // Do not advance buffer, need to reprocess this field for the following RESPONSE_STR.
            bEarlyEndOfMessage = false;
            break;
        }
        case FIELD_TYPE::RESPONSE_STR:
            // Response strings aren't surrounded by double quotes, ensure we get the whole response (skip over certain delimiters in responses)
            tokenLength = strcspn(*ppcLogBuf_, responseDelimiters.data());
            clMessageBody_.SetFieldValue<false>(field->index, std::string(*ppcLogBuf_, tokenLength));
            *ppcLogBuf_ += tokenLength + 1;
            break;
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            bool fixed = (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY);
            uint32_t uiArraySize = 0;
            if (fixed) { uiArraySize = dynamic_cast<const ArrayField*>(field.get())->arrayLength; }
            else
            {
                auto result = std::from_chars(*ppcLogBuf_, *ppcLogBuf_ + tokenLength + 1, uiArraySize);
                if (result.ec != std::errc())
                {
                    SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::VARIABLE_LENGTH_ARRAY: Array length not valid number. Malformed Input\n");
                    return STATUS::MALFORMED_INPUT;
                }

                if (uiArraySize > dynamic_cast<const ArrayField*>(field.get())->arrayLength)
                {
                    SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::VARIABLE_LENGTH_ARRAY: Array larger than JSON length. Malformed Input\n");
                    return STATUS::MALFORMED_INPUT;
                }

                *ppcLogBuf_ += tokenLength + 1;
                tokenLength = strcspn(*ppcLogBuf_, unquotedStringDelimiters.data());
            }

            STATUS eStatus = STATUS::SUCCESS;
            if (field->conversionHash == CalculateBlockCrc32("Z"))
            {
                assert(field->dataType.name == DATA_TYPE::UCHAR || field->dataType.name == DATA_TYPE::HEXBYTE);
                std::vector<uint8_t> vDecodedValues;
                eStatus = DecodeZConversionStringAsciiArray(vDecodedValues, ppcLogBuf_, uiArraySize);
                if (eStatus == STATUS::SUCCESS)
                {
                    if (fixed)
                    {
                        if (uiArraySize > 0) { clMessageBody_.SetFieldValue<true>(field->index, vDecodedValues.data(), uiArraySize); }
                    }
                    else { clMessageBody_.SetFieldValue<false>(field->index, std::move(vDecodedValues)); }
                }
                *ppcLogBuf_ += 1;
            }
            else if (field->isString)
            {
                std::string sDecodedValue;
                eStatus = DecodeStringAsciiArray(sDecodedValue, ppcLogBuf_, uiArraySize);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
                if (fixed) { clMessageBody_.SetFieldValue<true>(field->index, std::move(sDecodedValue)); }
                else { clMessageBody_.SetFieldValue<false>(field->index, std::move(sDecodedValue)); }
                *ppcLogBuf_ += 1;
            }
            else if (!field->isCsv)
            {
                eStatus = field->dataType.name == DATA_TYPE::CHAR
                              ? DecodeNonCommaSeparatedAsciiArray<int8_t>(clMessageBody_, ppcLogBuf_, field, uiArraySize, fixed)
                              : DecodeNonCommaSeparatedAsciiArray<uint8_t>(clMessageBody_, ppcLogBuf_, field, uiArraySize, fixed);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
                *ppcLogBuf_ += 1;
            }
            else
            {
                for (uint32_t i = 0; i < uiArraySize; ++i)
                {
                    tokenLength = strcspn(*ppcLogBuf_, unquotedStringDelimiters.data());
                    DecodeAsciiField(*field, ppcLogBuf_, tokenLength, clMessageBody_, i, fixed);
                    *ppcLogBuf_ += tokenLength + 1;
                }
                if (!fixed)
                {
                    std::visit(
                        [&uiArraySize](auto&& arg) {
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (is_specialization_of_v<T, std::vector>)
                            {
                                // Shrink the vector to the actual number of elements parsed
                                if (arg.size() != uiArraySize) { arg.resize(uiArraySize); }
                            }
                        },
                        clMessageBody_.GetVarFields()[field->index]);
                }
            }
            if (eStatus != STATUS::SUCCESS)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::ARRAY: Malformed Input\n");
                return eStatus;
            }

            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            uint32_t uiArraySize;
            auto result = std::from_chars(*ppcLogBuf_, *ppcLogBuf_ + tokenLength + 1, uiArraySize);
            if (result.ec != std::errc())
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::FIELD_ARRAY: Field Array length not valid number. Malformed Input\n");
                return STATUS::MALFORMED_INPUT;
            }

            *ppcLogBuf_ = result.ptr;

            ++*ppcLogBuf_;
            const auto* subFieldDefinitions = dynamic_cast<const FieldArrayField*>(field.get());

            if (uiArraySize > subFieldDefinitions->arrayLength)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::FIELD_ARRAY: Array size too large. Malformed Input\n");
                return STATUS::MALFORMED_INPUT;
            }

            clMessageBody_.GetVarFields()[field->index] = std::vector<MessageBody>(uiArraySize);
            auto& pvFieldArrayContainer = std::get<std::vector<MessageBody>>(clMessageBody_.GetVarFields()[field->index]);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                pvFieldArrayContainer[i] = MessageBody(subFieldDefinitions->fieldInfo.fixedFieldBytes, subFieldDefinitions->fieldInfo.varFieldCount);
                STATUS eStatus = DecodeAscii<Abbreviated>(subFieldDefinitions->fieldInfo, ppcLogBuf_, pvFieldArrayContainer[i], pcBufEnd_);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            break;
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Unknown field type");
            throw std::runtime_error("DecodeAscii(): Unknown field type");
        }

        if constexpr (!Abbreviated)
        {
            if (bEarlyEndOfMessage) { break; }
        }
    }

    return STATUS::SUCCESS;
}

// explicit template instantiations
template STATUS MessageDecoderBase::DecodeAscii<true>(const FieldInfo&, const char**, MessageBody&, const char*) const;
template STATUS MessageDecoderBase::DecodeAscii<false>(const FieldInfo&, const char**, MessageBody&, const char*) const;

// -------------------------------------------------------------------------------------------------------
STATUS MessageDecoderBase::DecodeJson(const FieldInfo& vMsgDefFields_, simdjson::dom::element jsonData, MessageBody& clMessageBody_) const
{
    for (const auto& field : vMsgDefFields_.messageOrderedFields)
    {
        simdjson::dom::element clField;
        if (jsonData[field->name].get(clField) != simdjson::SUCCESS)
        {
            SPDLOG_LOGGER_WARN(pclMyLogger, "Field '{}' not found in JSON", field->name);
            continue;
        }

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE: DecodeJsonField(*field, clField, clMessageBody_); break;

        case FIELD_TYPE::ENUM: {
            std::string_view enumValue;
            if (clField.get(enumValue) == simdjson::SUCCESS)
            {
                const auto* enumField = dynamic_cast<const EnumField*>(field.get());
                const uint32_t parsedEnumValue = GetEnumValue(enumField->enumDef, enumValue);

                switch (enumField->length)
                {
                case 1: clMessageBody_.SetFieldValue<true>(field->index, static_cast<uint8_t>(parsedEnumValue), 1); break;
                case 2: clMessageBody_.SetFieldValue<true>(field->index, static_cast<uint16_t>(parsedEnumValue), 1); break;
                default: clMessageBody_.SetFieldValue<true>(field->index, parsedEnumValue, 1); break;
                }
            }
            break;
        }

        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR: {
            std::string_view strValue;
            if (clField.get(strValue) == simdjson::SUCCESS) { clMessageBody_.GetVarFields()[field->index] = std::string(strValue); }
            break;
        }

        case FIELD_TYPE::RESPONSE_ID: {
            std::string_view sResponse;
            if (clField.get(sResponse) == simdjson::SUCCESS)
            {
                if (sResponse == "OK") { clMessageBody_.SetFieldValue<true>(field->index, 1); }
                else { clMessageBody_.SetFieldValue<true>(field->index, GetResponseId(vMyResponseDefinitions, sResponse.substr(svErrorPrefix.length()))); }
            }
            break;
        }

        case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            const bool fixed = (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY);
            const auto* arrayField = dynamic_cast<const ArrayField*>(field.get());
            const uint32_t uiArraySize = (fixed && arrayField != nullptr) ? arrayField->arrayLength : 0;

            if (field->isString)
            {
                std::string_view strValue;
                if (clField.get(strValue) == simdjson::SUCCESS)
                {
                    if (fixed)
                    {
                        if (strValue.size() > uiArraySize) { return STATUS::MALFORMED_INPUT; }
                        std::vector<uint8_t> vDecodedValues(uiArraySize, 0);
                        for (size_t i = 0; i < strValue.size(); ++i) { vDecodedValues[i] = static_cast<uint8_t>(strValue[i]); }
                        if (!vDecodedValues.empty()) { clMessageBody_.SetFieldValue<true>(field->index, vDecodedValues.data(), vDecodedValues.size()); }
                    }
                    else { clMessageBody_.SetFieldValue<false>(field->index, std::string(strValue)); }
                }
            }
            else
            {
                simdjson::dom::array array;
                if (clField.get(array) == simdjson::SUCCESS)
                {
                    if (field->conversionHash == CalculateBlockCrc32("Z"))
                    {
                        if (fixed)
                        {
                            if (array.size() > uiArraySize) { return STATUS::MALFORMED_INPUT; }
                            std::vector<uint8_t> vDecodedValues(uiArraySize, 0);
                            size_t i = 0;
                            for (simdjson::dom::element it : array)
                            {
                                uint64_t value;
                                if (it.get(value) != simdjson::SUCCESS) { return STATUS::MALFORMED_INPUT; }
                                vDecodedValues[i++] = static_cast<uint8_t>(value);
                            }
                            if (!vDecodedValues.empty())
                            {
                                clMessageBody_.SetFieldValue<true>(field->index, vDecodedValues.data(), vDecodedValues.size());
                            }
                        }
                        else
                        {
                            std::vector<uint8_t> vDecodedValues;
                            vDecodedValues.reserve(array.size());
                            for (simdjson::dom::element it : array)
                            {
                                uint64_t value;
                                if (it.get(value) != simdjson::SUCCESS) { return STATUS::MALFORMED_INPUT; }
                                vDecodedValues.emplace_back(static_cast<uint8_t>(value));
                            }
                            clMessageBody_.SetFieldValue<false>(field->index, std::move(vDecodedValues));
                        }
                    }
                    else if (field->conversionHash == CalculateBlockCrc32("P"))
                    {
                        if (fixed)
                        {
                            if (array.size() > uiArraySize) { return STATUS::MALFORMED_INPUT; }
                            std::vector<int8_t> vDecodedValues(uiArraySize, 0);
                            size_t i = 0;
                            for (simdjson::dom::element it : array)
                            {
                                int64_t value;
                                if (it.get(value) != simdjson::SUCCESS) { return STATUS::MALFORMED_INPUT; }
                                vDecodedValues[i++] = static_cast<int8_t>(value);
                            }
                            if (!vDecodedValues.empty())
                            {
                                clMessageBody_.SetFieldValue<true>(field->index, vDecodedValues.data(), vDecodedValues.size());
                            }
                        }
                        else
                        {
                            std::vector<int8_t> vDecodedValues;
                            vDecodedValues.reserve(array.size());
                            for (simdjson::dom::element it : array)
                            {
                                int64_t value;
                                if (it.get(value) != simdjson::SUCCESS) { return STATUS::MALFORMED_INPUT; }
                                vDecodedValues.emplace_back(static_cast<int8_t>(value));
                            }
                            clMessageBody_.SetFieldValue<false>(field->index, std::move(vDecodedValues));
                        }
                    }
                    else { return STATUS::MALFORMED_INPUT; }
                }
            }
            break;
        }

        case FIELD_TYPE::FIELD_ARRAY: {
            simdjson::dom::array array;
            auto error = clField.get(array);
            if (error) { return STATUS::MALFORMED_INPUT; }

            const auto* subFieldDefinitions = dynamic_cast<const FieldArrayField*>(field.get());
            if (!subFieldDefinitions) { return STATUS::MALFORMED_INPUT; }

            std::vector<MessageBody> vFieldArrayContainer;
            vFieldArrayContainer.reserve(array.size());

            for (simdjson::dom::element element : array)
            {
                vFieldArrayContainer.emplace_back(subFieldDefinitions->fieldInfo.fixedFieldBytes, subFieldDefinitions->fieldInfo.varFieldCount);
                auto& stSubMessageBody = vFieldArrayContainer.back();

                // Recursively decode the subfields
                STATUS eStatus = DecodeJson(subFieldDefinitions->fieldInfo, element, stSubMessageBody);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }

            clMessageBody_.GetVarFields()[field->index] = std::move(vFieldArrayContainer);
            break;
        }

        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeJson(): Unknown field type '{}'", field->name);
            throw std::runtime_error("DecodeJson(): Unknown field type");
        }
    }

    return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
void MessageDecoderBase::DecodeJsonField(const BaseField& field_, simdjson::dom::element clJsonField_, MessageBody& clMessageBody_,
                                         const size_t elementIndex_, const bool fixed_) const
{
    const auto it = jsonFieldMap.find(field_.conversionHash);
    if (it == jsonFieldMap.end()) { throw std::runtime_error("DecodeJsonField(): Unknown field type\n"); }
    it->second(clMessageBody_, field_, clJsonField_, elementIndex_, fixed_, *pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::Decode(const unsigned char* pucMessage_, MessageBody& stInterMessage_, MetaDataBase& stMetaData_) const
{
    if (pucMessage_ == nullptr) { return STATUS::NULL_PROVIDED; }
    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    const unsigned char* pucTempInData = pucMessage_;

    if (stMetaData_.bResponse && (stMetaData_.eFormat != HEADER_FORMAT::BINARY && stMetaData_.eFormat != HEADER_FORMAT::SHORT_BINARY &&
                                  stMetaData_.eFormat != HEADER_FORMAT::ASCII && stMetaData_.eFormat != HEADER_FORMAT::SHORT_ASCII &&
                                  stMetaData_.eFormat != HEADER_FORMAT::ABB_ASCII && stMetaData_.eFormat != HEADER_FORMAT::SHORT_ABB_ASCII))
    {
        return STATUS::NO_DEFINITION;
    }

    MessageDefinition::ConstPtr pclMsgDef = GetMessageDefinition(stMetaData_);

        if (pclMsgDef == nullptr)
        {
            LogMissingMsgDef(*pclMyLogger, stMetaData_.usMessageId);
            return STATUS::NO_DEFINITION;
        }
    }

    if (stMetaData_.messageName == "RXCONFIG")
    {
        SPDLOG_LOGGER_INFO(pclMyLogger, "RXCONFIG payload must be decoded via RxConfigHandler, not MessageDecoder.");
        return STATUS::UNSUPPORTED;
    }
    const uint32_t uiMessageCrc = stMetaData_.bResponse ? 0 : stMetaData_.uiMessageCrc;
    const FieldInfo& msgFieldInfo = pclMsgDef->GetMsgDefFromCrc(uiMessageCrc);

    stInterMessage_.Resize(msgFieldInfo.fixedFieldBytes, msgFieldInfo.varFieldCount);
    stInterMessage_.SetDefinition(pclMsgDef, uiMessageCrc);

    // Decode the detected format
    switch (stMetaData_.eFormat)
    {
    case HEADER_FORMAT::ASCII: [[fallthrough]];
    case HEADER_FORMAT::SHORT_ASCII: //
        return DecodeAscii<false>(msgFieldInfo, reinterpret_cast<const char**>(&pucTempInData), stInterMessage_,
                                  stMetaData_.uiLength > stMetaData_.uiHeaderLength
                                      ? reinterpret_cast<const char*>(pucTempInData) + stMetaData_.uiLength - stMetaData_.uiHeaderLength
                                      : nullptr);
    case HEADER_FORMAT::ABB_ASCII: [[fallthrough]];
    case HEADER_FORMAT::SHORT_ABB_ASCII: //
        return DecodeAscii<true>(msgFieldInfo, reinterpret_cast<const char**>(&pucTempInData), stInterMessage_,
                                 stMetaData_.uiLength > stMetaData_.uiHeaderLength
                                     ? reinterpret_cast<const char*>(pucTempInData) + stMetaData_.uiLength - stMetaData_.uiHeaderLength
                                     : nullptr);
    case HEADER_FORMAT::BINARY: [[fallthrough]];
    case HEADER_FORMAT::SHORT_BINARY:
        if (msgFieldInfo.varFieldCount == 0)
        {
            // Fast path: if there are no variable-length fields, copy the entire message to the fixed region
            stInterMessage_.SetFieldValue<true>(0, reinterpret_cast<const std::byte*>(pucTempInData), msgFieldInfo.fixedFieldBytes);
            return STATUS::SUCCESS;
        }
        return DecodeBinary(msgFieldInfo, &pucTempInData, stInterMessage_, stMetaData_.uiBinaryMsgLength);
    case HEADER_FORMAT::JSON: {
        simdjson::dom::parser parser;
        simdjson::dom::element clJsonFields;

        std::string_view jsonStringView(reinterpret_cast<const char*>(pucTempInData)); // Assumes null-terminated data

        auto error = parser.parse(jsonStringView).get(clJsonFields);
        if (error)
        {
            SPDLOG_LOGGER_ERROR(pclMyLogger, "JSON parsing error:"); // TODO: {}", error.message());
            return STATUS::MALFORMED_INPUT;
        }

        simdjson::dom::element body;
        if (clJsonFields["body"].get(body) != simdjson::SUCCESS)
        {
            SPDLOG_LOGGER_WARN(pclMyLogger, "Field 'body' not found in JSON");
            return STATUS::MALFORMED_INPUT;
        }

        return DecodeJson(msgFieldInfo, body, stInterMessage_);
    }
    default: //
        return STATUS::UNKNOWN;
    }
}
