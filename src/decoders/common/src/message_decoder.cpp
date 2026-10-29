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

using namespace std::string_view_literals;
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

    asciiFieldMap[CalculateBlockCrc32("e")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 4:
            if (fixed_) { ParseAndEmplace<true, float>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, float>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        case 8:
            if (fixed_) { ParseAndEmplace<true, double>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, double>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        default: throw std::runtime_error("invalid float length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("f")] = asciiFieldMap[CalculateBlockCrc32("e")];

    asciiFieldMap[CalculateBlockCrc32("d")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (pstMessageDataType_.dataType.name == DATA_TYPE::BOOL)
        {
            if (token_ != "TRUE" && token_ != "FALSE") { throw std::runtime_error("Invalid boolean value"); }
            const bool value = (token_ == "TRUE");
            if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
            else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
        }
        else
        {
            if (fixed_) { ParseAndEmplace<true, int32_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, int32_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
        }
    };

    asciiFieldMap[CalculateBlockCrc32("u")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1:
            if (fixed_) { ParseAndEmplace<true, uint8_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint8_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        case 2:
            if (fixed_) { ParseAndEmplace<true, uint16_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint16_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        case 4:
            if (fixed_) { ParseAndEmplace<true, uint32_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint32_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        case 8:
            if (fixed_) { ParseAndEmplace<true, uint64_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint64_t>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("x")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1:
            if (fixed_) { ParseAndEmplace<true, uint8_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint8_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        case 2:
            if (fixed_) { ParseAndEmplace<true, uint16_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint16_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        case 4:
            if (fixed_) { ParseAndEmplace<true, uint32_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint32_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        case 8:
            if (fixed_) { ParseAndEmplace<true, uint64_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            else { ParseAndEmplace<false, uint64_t, 16>(clCompField_, pstMessageDataType_, token_, elementIndex_); }
            return;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    asciiFieldMap[CalculateBlockCrc32("X")] = asciiFieldMap[CalculateBlockCrc32("x")];

    asciiFieldMap[CalculateBlockCrc32("c")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                 const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (token_.size() != 1) { throw std::runtime_error("Invalid character value"); }
        const auto value = static_cast<int8_t>(token_.front());
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
    };

    asciiFieldMap[CalculateBlockCrc32("uc")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_, std::string_view token_,
                                                  const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (token_.size() != 1) { throw std::runtime_error("Invalid character value"); }
        const auto value = static_cast<uint8_t>(static_cast<unsigned char>(token_.front()));
        if (fixed_) { clCompField_.SetArrayElement<true>(pstMessageDataType_, elementIndex_, value); }
        else { clCompField_.SetArrayElement<false>(pstMessageDataType_, elementIndex_, value); }
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

    jsonFieldMap[CalculateBlockCrc32("f")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 4: PushElement<float>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 8: PushElement<double>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        default: throw std::runtime_error("invalid float length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("d")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        if (pstMessageDataType_.dataType.name == DATA_TYPE::BOOL)
        {
            PushElement<bool>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_);
        }
        else { PushElement<int32_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); }
    };

    jsonFieldMap[CalculateBlockCrc32("u")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1: PushElement<uint8_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 2: PushElement<uint16_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 4: PushElement<uint32_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 8: PushElement<uint64_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        default: throw std::runtime_error("invalid unsigned length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("x")] = [](CompositeField& clCompField_, const BaseField& pstMessageDataType_,
                                                simdjson::dom::element clJsonField_, const size_t elementIndex_, const bool fixed_,
                                                [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        switch (pstMessageDataType_.dataType.length)
        {
        case 1: PushElement<uint8_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 2: PushElement<uint16_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 4: PushElement<uint32_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        case 8: PushElement<uint64_t>(clCompField_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_); return;
        default: throw std::runtime_error("invalid hex length");
        }
    };

    jsonFieldMap[CalculateBlockCrc32("X")] = jsonFieldMap[CalculateBlockCrc32("x")];
}

// -------------------------------------------------------------------------------------------------------
template <bool Fixed>
void MessageDecoderBase::DecodeBinaryField(const BaseField& pstMessageDataType_, const unsigned char** ppucLogBuf_, CompositeField& clCompField_,
                                           size_t n)
{
    SimpleTypeVisitor(pstMessageDataType_, [&](auto valuePtr) {
        using T = std::decay_t<decltype(valuePtr)>;
        // Note: T* is just used as a type tag here - SetFieldValue does not dereference the given pointer,
        // so the reinterpret_cast is ok.
        clCompField_.SetFieldValue<Fixed>(pstMessageDataType_.index, reinterpret_cast<const T*>(*ppucLogBuf_), n);
    });
    *ppucLogBuf_ += pstMessageDataType_.dataType.length * n;
}

// explicit template instantiations
template void MessageDecoderBase::DecodeBinaryField<true>(const BaseField&, const unsigned char**, CompositeField&, size_t);
template void MessageDecoderBase::DecodeBinaryField<false>(const BaseField&, const unsigned char**, CompositeField&, size_t);

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::DecodeBinary(const FieldInfo& vMsgDefFields_, const unsigned char** ppucLogBuf_, CompositeField& clCompField_,
                                 const uint32_t uiMessageLength_) const
{
    const unsigned char* pucTempStart = *ppucLogBuf_;

    for (const auto& field : vMsgDefFields_.messageOrderedFields)
    {
        auto fieldTypeLength = static_cast<uint8_t>(field->dataType.length);
        *ppucLogBuf_ += fMyAlignmentFunc(fieldTypeLength, reinterpret_cast<uintptr_t>(pucTempStart), reinterpret_cast<uintptr_t>(*ppucLogBuf_));

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE: DecodeBinaryField<true>(*field, ppucLogBuf_, clCompField_, 1); break;
        case FIELD_TYPE::ENUM:
            switch (field->dataType.length)
            {
            case 1: clCompField_.SetFieldValue<true>(field->index, LoadValueFromBuffer<uint8_t>(*ppucLogBuf_)); break;
            case 2: clCompField_.SetFieldValue<true>(field->index, LoadValueFromBuffer<uint16_t>(*ppucLogBuf_)); break;
            case 4: clCompField_.SetFieldValue<true>(field->index, LoadValueFromBuffer<uint32_t>(*ppucLogBuf_)); break;
            default:
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeBinary(): Invalid field length\n");
                throw std::runtime_error("DecodeBinary(): Invalid field length\n");
            }
            *ppucLogBuf_ += field->dataType.length;
            break;
        case FIELD_TYPE::RESPONSE_ID:
            clCompField_.SetFieldValue<true>(field->index, LoadValueFromBuffer<int32_t>(*ppucLogBuf_));
            *ppucLogBuf_ += sizeof(int32_t);
            break;
        case FIELD_TYPE::RESPONSE_STR: {
            std::string_view sTemp(reinterpret_cast<const char*>(*ppucLogBuf_), uiMessageLength_ - sizeof(int32_t)); // Remove CRC
            clCompField_.SetFieldValue<false>(field->index, std::string(sTemp));
            // Binary response string is not null terminated or 4 byte aligned
            *ppucLogBuf_ += sTemp.size();
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const uint32_t uiArraySize = dynamic_cast<const ArrayField*>(field.get())->arrayLength;
            DecodeBinaryField<true>(*field, ppucLogBuf_, clCompField_, uiArraySize);
            break;
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            const uint32_t uiArraySize = GetArrayLength(pucTempStart, ppucLogBuf_, dynamic_cast<const ArrayField&>(*field), clCompField_);
            DecodeBinaryField<false>(*field, ppucLogBuf_, clCompField_, uiArraySize);
            break;
        }
        case FIELD_TYPE::STRING: {
            // This version of a string is different. It is hopefully null terminated.
            std::string_view sTemp(reinterpret_cast<const char*>(*ppucLogBuf_));
            clCompField_.SetFieldValue<false>(field->index, std::string(sTemp));
            *ppucLogBuf_ += sTemp.size() + 1; // + 1 to consume the NULL at the end of the string.
            AddStringFieldPadding(pucTempStart, ppucLogBuf_);
            break;
        }
        case FIELD_TYPE::FIELD_ARRAY: {
            auto fieldArrayDef = std::dynamic_pointer_cast<const FieldArrayField>(field);
            if (!fieldArrayDef)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeBinary(): FIELD_ARRAY definition cast failed");
                return STATUS::MALFORMED_INPUT;
            }
            const auto* subFieldDefinitions = fieldArrayDef.get();
            const uint32_t uiArraySize = GetArrayLength(pucTempStart, ppucLogBuf_, *subFieldDefinitions, clCompField_);

            if (subFieldDefinitions->fieldInfo->varFieldCount == 0)
            {
                const size_t totalBytes = uiArraySize * subFieldDefinitions->fieldInfo->fixedFieldBytes;
                // Store flat FIELD_ARRAY directly as std::vector<std::byte>
                std::vector<std::byte> flatFieldData(reinterpret_cast<const std::byte*>(*ppucLogBuf_),
                                                     reinterpret_cast<const std::byte*>(*ppucLogBuf_) + totalBytes);
                clCompField_.SetFieldValue(*field, FlatFieldArray(std::move(flatFieldData), subFieldDefinitions->fieldInfo.get()));
                *ppucLogBuf_ += totalBytes;
            }
            else
            {
                std::vector<CompositeField> vFieldArrayContainer(uiArraySize);
                for (uint32_t i = 0; i < uiArraySize; ++i)
                {
                    *ppucLogBuf_ +=
                        fMyAlignmentFunc(fieldTypeLength, reinterpret_cast<uintptr_t>(pucTempStart), reinterpret_cast<uintptr_t>(*ppucLogBuf_));
                    vFieldArrayContainer[i] =
                        CompositeField(subFieldDefinitions->fieldInfo->fixedFieldBytes, subFieldDefinitions->fieldInfo->varFieldCount);
                    vFieldArrayContainer[i].SetFieldInfo(subFieldDefinitions->fieldInfo);
                    STATUS eStatus = DecodeBinary(*subFieldDefinitions->fieldInfo, ppucLogBuf_, vFieldArrayContainer[i],
                                                  uiMessageLength_ - static_cast<uint32_t>(*ppucLogBuf_ - pucTempStart));
                    if (eStatus != STATUS::SUCCESS) { return eStatus; }
                }
                clCompField_.SetFieldValue(*field, std::move(vFieldArrayContainer));
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
void MessageDecoderBase::DecodeAsciiField(const BaseField& field_, std::string_view token_, CompositeField& clCompField_, const size_t elementIndex_,
                                          const bool fixed_) const
{
    const auto it = asciiFieldMap.find(field_.conversionHash);
    if (it == asciiFieldMap.end()) { throw std::runtime_error("DecodeAsciiField(): Unknown field type\n"); }
    it->second(clCompField_, field_, token_, elementIndex_, fixed_, *pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
// Decode an ASCII array formatted with the %Z conversion string
static STATUS DecodeZConversionStringAsciiArray(std::vector<uint8_t>& vDecodedValues_, std::string_view& sLogBuf_, uint32_t uiArraySize_)
{
    if (sLogBuf_.size() < 2 * static_cast<size_t>(uiArraySize_)) { return STATUS::MALFORMED_INPUT; }

    vDecodedValues_.resize(uiArraySize_);
    for (uint32_t i = 0; i < uiArraySize_; ++i)
    {
        const auto hexPair = sLogBuf_.substr(0, 2);
        uint8_t value = 0;
        const auto [end, error] = std::from_chars(hexPair.data(), hexPair.data() + hexPair.size(), value, 16);

        if (error != std::errc{} || end != hexPair.data() + hexPair.size()) { return STATUS::MALFORMED_INPUT; }
        sLogBuf_.remove_prefix(2);
        vDecodedValues_[i] = value;
    }

    return STATUS::SUCCESS;
}

// Decode an ASCII array field that is not comma separated, e.g. a raw ASCII value or hex representation
template <typename CharType> static STATUS DecodeNonCommaSeparatedAsciiArrayField(CharType& cValue_, std::string_view& sLogBuf_)
{
    if (sLogBuf_.empty()) { return STATUS::MALFORMED_INPUT; }

    if (sLogBuf_.substr(0, 2) == "\\\\")
    {
        cValue_ = static_cast<CharType>('\\');
        sLogBuf_.remove_prefix(2);
    }
    else if (sLogBuf_.substr(0, 2) == "\\x")
    {
        if (sLogBuf_.size() < 4) { return STATUS::MALFORMED_INPUT; }
        const auto hexPair = sLogBuf_.substr(2, 2);
        uint8_t value = 0;
        const auto [end, error] = std::from_chars(hexPair.data(), hexPair.data() + hexPair.size(), value, 16);
        if (error != std::errc{} || end != hexPair.data() + hexPair.size()) { return STATUS::MALFORMED_INPUT; }

        cValue_ = static_cast<CharType>(value);
        sLogBuf_.remove_prefix(4);
    }
    // Ascii character
    else
    {
        cValue_ = static_cast<CharType>(sLogBuf_.front());
        sLogBuf_.remove_prefix(1);
    }
    return STATUS::SUCCESS;
}

// Decode an ASCII array made up of non-comma separated values, e.g. a raw ASCII values or hex representations
template <typename CharType>
static STATUS DecodeNonCommaSeparatedAsciiArray(CompositeField& clCompField_, std::string_view& sLogBuf_, const BaseField& field_,
                                                uint32_t uiArraySize_, bool fixed_)
{
    std::vector<CharType> vDecodedValues(uiArraySize_);
    for (uint32_t i = 0; i < uiArraySize_; ++i)
    {
        STATUS eStatus = DecodeNonCommaSeparatedAsciiArrayField(vDecodedValues[i], sLogBuf_);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }
    }
    if (fixed_) { clCompField_.SetFieldValue<true>(field_.index, std::move(vDecodedValues)); }
    else { clCompField_.SetFieldValue<false>(field_.index, std::move(vDecodedValues)); }
    return STATUS::SUCCESS;
}

// Decode an ASCII array which is formatted like a string with opening and closing quotes, e.g. "string value"
static STATUS DecodeStringAsciiArray(std::string& sDecodedValue_, std::string_view& sLogBuf_, uint32_t uiArraySize_)
{
    sDecodedValue_.assign(uiArraySize_, '\0');

    // Look for opening double-quote
    if (sLogBuf_.empty() || sLogBuf_.front() != '\"') { return STATUS::MALFORMED_INPUT; }
    sLogBuf_.remove_prefix(1);

    for (uint32_t i = 0; i < uiArraySize_; ++i)
    {
        if (sLogBuf_.empty()) { return STATUS::MALFORMED_INPUT; }
        if (sLogBuf_.front() == '\"') { break; }

        STATUS eStatus = DecodeNonCommaSeparatedAsciiArrayField(sDecodedValue_[i], sLogBuf_);
        if (eStatus != STATUS::SUCCESS) { return eStatus; }
    }

    // Look for closing double-quote
    if (sLogBuf_.empty() || sLogBuf_.front() != '\"') { return STATUS::MALFORMED_INPUT; }
    sLogBuf_.remove_prefix(1);

    return STATUS::SUCCESS;
}

template <bool Abbreviated>
STATUS MessageDecoderBase::DecodeAscii(const FieldInfo& vMsgDefFields_, std::string_view& sMessage_, CompositeField& clCompField_) const
{
    constexpr auto tokenDelimiter = Abbreviated ? ABBREV_ASCII_TOKEN_DELIM : ASCII_TOKEN_DELIM;
    constexpr auto endDelimiter = Abbreviated ? ABBREV_ASCII_END_DELIM : ASCII_END_DELIM;

    const auto prepareToken = [&](bool needsDelimiter) {
        if constexpr (Abbreviated)
        {
            const auto start = sMessage_.find_first_not_of(ABBREV_ASCII_FORMATTING);
            if (start == std::string_view::npos || (needsDelimiter && start == 0)) { return false; }
            sMessage_.remove_prefix(start);
        }
        else
        {
            if (sMessage_.empty() || sMessage_.front() == ASCII_END_DELIM) { return false; }
            if (needsDelimiter)
            {
                if (sMessage_.front() != ASCII_TOKEN_DELIM) { return false; }
                sMessage_.remove_prefix(1);
            }
        }
        return true;
    };

    bool needsDelimiter = false;
    for (const auto& field : vMsgDefFields_.messageOrderedFields)
    {
        if (!prepareToken(needsDelimiter))
        {
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Message ended unexpectedly\n");
            return STATUS::MALFORMED_INPUT;
        }
        needsDelimiter = true;

        size_t tokenEnd = FindFirstAsciiTokenDelimiter<Abbreviated>(sMessage_);
        const auto sField = sMessage_.substr(0, tokenEnd);

        switch (field->type)
        {
        case FIELD_TYPE::SIMPLE:
            try
            {
                DecodeAsciiField(*field, sField, clCompField_);
            }
            catch (const std::runtime_error&)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAsciiField(): Exception when decoding field. Malformed Input\n");
                return STATUS::MALFORMED_INPUT;
            }
            sMessage_.remove_prefix(tokenEnd);
            break;
        case FIELD_TYPE::ENUM: {
            const auto* enumField = dynamic_cast<const EnumField*>(field.get());
            const int32_t enumValue = GetEnumValue(enumField->enumDef, sField);
            switch (enumField->dataType.length)
            {
            case 1: clCompField_.SetFieldValue<true>(field->index, static_cast<int8_t>(enumValue)); break;
            case 2: clCompField_.SetFieldValue<true>(field->index, static_cast<int16_t>(enumValue)); break;
            default: clCompField_.SetFieldValue<true>(field->index, enumValue); break;
            }
            sMessage_.remove_prefix(tokenEnd);
            break;
        }
        case FIELD_TYPE::STRING:
            if (!sMessage_.empty() && sMessage_.front() == '"')
            {
                const auto closingQuote = sMessage_.find('"', 1);
                if (closingQuote == std::string_view::npos) { return STATUS::MALFORMED_INPUT; }
                clCompField_.SetFieldValue<false>(field->index, std::string(sMessage_.substr(1, closingQuote - 1)));
                tokenEnd = closingQuote + 1;
            }
            else { clCompField_.SetFieldValue<false>(field->index, std::string(sField)); }
            sMessage_.remove_prefix(tokenEnd);
            break;
        case FIELD_TYPE::RESPONSE_ID: {
            // Ensure we get the whole response (skip over delimiters in responses)
            tokenEnd = sMessage_.find(endDelimiter, tokenEnd);
            if (tokenEnd == std::string_view::npos) { tokenEnd = sMessage_.size(); }
            const auto sResponse = sMessage_.substr(0, tokenEnd);
            if (sResponse == "OK") { clCompField_.SetFieldValue<true>(field->index, static_cast<int32_t>(1)); }
            // Note: This won't match responses with format specifiers in them (%d, %s, etc.), they will be given id=0
            else
            {
                // Response descriptions in the database do not include the "ERROR:" prefix
                auto sDescription = sResponse;
                if (sDescription.substr(0, svErrorPrefix.size()) == svErrorPrefix) { sDescription.remove_prefix(svErrorPrefix.size()); }
                clCompField_.SetFieldValue<true>(field->index, GetResponseId(vMyResponseDefinitions, sDescription));
            }
            // Do not advance buffer, need to reprocess this field for the following RESPONSE_STR.
            needsDelimiter = false;
            continue;
        }
        case FIELD_TYPE::RESPONSE_STR:
            // Ensure we get the whole response (skip over delimiters in responses)
            tokenEnd = sMessage_.find(endDelimiter, tokenEnd);
            if (tokenEnd == std::string_view::npos) { tokenEnd = sMessage_.size(); }
            clCompField_.SetFieldValue<false>(field->index, std::string(sMessage_.substr(0, tokenEnd)));
            sMessage_.remove_prefix(tokenEnd);
            break;
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            const bool fixed = (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY);
            uint32_t uiArraySize = 0;
            if (fixed) { uiArraySize = dynamic_cast<const ArrayField*>(field.get())->arrayLength; }
            else
            {
                auto result = std::from_chars(sField.data(), sField.data() + sField.size(), uiArraySize);
                if (result.ec != std::errc() || result.ptr != sField.data() + sField.size())
                {
                    SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::VARIABLE_LENGTH_ARRAY: Array length not valid number. Malformed Input\n");
                    return STATUS::MALFORMED_INPUT;
                }

                if (uiArraySize > dynamic_cast<const ArrayField*>(field.get())->arrayLength)
                {
                    SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::VARIABLE_LENGTH_ARRAY: Array larger than JSON length. Malformed Input\n");
                    return STATUS::MALFORMED_INPUT;
                }

                sMessage_.remove_prefix(tokenEnd);
                if ((field->isString || uiArraySize != 0) && !prepareToken(true)) { return STATUS::MALFORMED_INPUT; }
            }

            STATUS eStatus = STATUS::SUCCESS;
            if (field->conversionHash == CalculateBlockCrc32("Z"))
            {
                assert(field->dataType.name == DATA_TYPE::UCHAR || field->dataType.name == DATA_TYPE::HEXBYTE);
                std::vector<uint8_t> vDecodedValues;
                eStatus = DecodeZConversionStringAsciiArray(vDecodedValues, sMessage_, uiArraySize);
                if (eStatus == STATUS::SUCCESS)
                {
                    if (fixed)
                    {
                        if (uiArraySize > 0) { clCompField_.SetFieldValue<true>(field->index, vDecodedValues.data(), uiArraySize); }
                    }
                    else { clCompField_.SetFieldValue<false>(field->index, std::move(vDecodedValues)); }
                }
                else
                {
                    SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::VARIABLE_LENGTH_ARRAY: Z conversion failed. Malformed Input\n");
                    return eStatus;
                }
            }
            else if (field->isString)
            {
                std::string sDecodedValue;
                eStatus = DecodeStringAsciiArray(sDecodedValue, sMessage_, uiArraySize);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
                if (fixed) { clCompField_.SetFieldValue<true>(field->index, std::move(sDecodedValue)); }
                else { clCompField_.SetFieldValue<false>(field->index, std::move(sDecodedValue)); }
            }
            else if (!field->isCsv)
            {
                eStatus = field->dataType.name == DATA_TYPE::CHAR
                              ? DecodeNonCommaSeparatedAsciiArray<int8_t>(clCompField_, sMessage_, *field, uiArraySize, fixed)
                              : DecodeNonCommaSeparatedAsciiArray<uint8_t>(clCompField_, sMessage_, *field, uiArraySize, fixed);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            else
            {
                if (!fixed)
                {
                    SimpleTypeVisitor(*field, [&](auto&& arg) {
                        using T = std::decay_t<decltype(arg)>;
                        using StoredType = std::conditional_t<std::is_same_v<T, bool>, uint8_t, T>;
                        clCompField_.SetFieldValue<false>(field->index, std::vector<StoredType>(uiArraySize));
                    });
                }
                for (uint32_t i = 0; i < uiArraySize; i++)
                {
                    if (i > 0 && !prepareToken(true)) { return STATUS::MALFORMED_INPUT; }
                    tokenEnd = FindFirstAsciiTokenDelimiter<Abbreviated>(sMessage_);
                    try
                    {
                        DecodeAsciiField(*field, sMessage_.substr(0, tokenEnd), clCompField_, i, fixed);
                    }
                    catch (const std::runtime_error&)
                    {
                        return STATUS::MALFORMED_INPUT;
                    }
                    sMessage_.remove_prefix(tokenEnd);
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
            auto result = std::from_chars(sField.data(), sField.data() + sField.size(), uiArraySize);
            if (result.ec != std::errc() || result.ptr != sField.data() + sField.size())
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::FIELD_ARRAY: Field Array length not valid number. Malformed Input\n");
                return STATUS::MALFORMED_INPUT;
            }

            sMessage_.remove_prefix(sField.size());

            auto fieldArrayDef = std::dynamic_pointer_cast<const FieldArrayField>(field);
            if (!fieldArrayDef)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::FIELD_ARRAY: Definition cast failed");
                return STATUS::MALFORMED_INPUT;
            }
            const auto* subFieldDefinitions = fieldArrayDef.get();

            if (uiArraySize > subFieldDefinitions->arrayLength)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii()::FIELD_ARRAY: Array size too large. Malformed Input\n");
                return STATUS::MALFORMED_INPUT;
            }

            std::vector<CompositeField> pvFieldArrayContainer;
            pvFieldArrayContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
                if (!prepareToken(true)) { return STATUS::MALFORMED_INPUT; }
                auto& newField = pvFieldArrayContainer.emplace_back(subFieldDefinitions->fieldInfo->fixedFieldBytes,
                                                                    subFieldDefinitions->fieldInfo->varFieldCount);
                newField.SetFieldInfo(subFieldDefinitions->fieldInfo);
                STATUS eStatus = DecodeAscii<Abbreviated>(*subFieldDefinitions->fieldInfo, sMessage_, newField);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }
            clCompField_.SetFieldValue(*field, std::move(pvFieldArrayContainer));
            break;
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeAscii(): Unknown field type");
            throw std::runtime_error("DecodeAscii(): Unknown field type");
        }
    }

    // Make sure no unexpected characters remain in the buffer
    if (!sMessage_.empty() && sMessage_.front() != endDelimiter && sMessage_.front() != tokenDelimiter) { return STATUS::MALFORMED_INPUT; }
    return STATUS::SUCCESS;
}

// explicit template instantiations
template STATUS MessageDecoderBase::DecodeAscii<true>(const FieldInfo&, std::string_view&, CompositeField&) const;
template STATUS MessageDecoderBase::DecodeAscii<false>(const FieldInfo&, std::string_view&, CompositeField&) const;

// -------------------------------------------------------------------------------------------------------
STATUS MessageDecoderBase::DecodeJson(const FieldInfo& vMsgDefFields_, simdjson::dom::element jsonData, CompositeField& clCompField_) const
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
        case FIELD_TYPE::SIMPLE:
            try
            {
                DecodeJsonField(*field, clField, clCompField_);
            }
            catch (const std::runtime_error&)
            {
                SPDLOG_LOGGER_CRITICAL(pclMyLogger, "DecodeJsonField(): Exception when decoding field. Malformed Input\n");
                return STATUS::MALFORMED_INPUT;
            }
            break;

        case FIELD_TYPE::ENUM: {
            std::string_view enumValue;
            if (clField.get(enumValue) == simdjson::SUCCESS)
            {
                const auto* enumField = dynamic_cast<const EnumField*>(field.get());
                const int32_t parsedEnumValue = GetEnumValue(enumField->enumDef, enumValue);

                switch (enumField->dataType.length)
                {
                case 1: clCompField_.SetFieldValue<true>(field->index, static_cast<int8_t>(parsedEnumValue)); break;
                case 2: clCompField_.SetFieldValue<true>(field->index, static_cast<int16_t>(parsedEnumValue)); break;
                default: clCompField_.SetFieldValue<true>(field->index, parsedEnumValue); break;
                }
            }
            break;
        }

        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR: {
            std::string_view strValue;
            if (clField.get(strValue) == simdjson::SUCCESS) { clCompField_.SetFieldValue<false>(field->index, std::string(strValue)); }
            break;
        }

        case FIELD_TYPE::RESPONSE_ID: {
            std::string_view sResponse;
            if (clField.get(sResponse) == simdjson::SUCCESS)
            {
                if (sResponse == "OK") { clCompField_.SetFieldValue<true>(field->index, 1); }
                else
                {
                    if (sResponse.substr(0, svErrorPrefix.size()) == svErrorPrefix) { sResponse.remove_prefix(svErrorPrefix.size()); }
                    clCompField_.SetFieldValue<true>(field->index, GetResponseId(vMyResponseDefinitions, sResponse));
                }
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
                        if (!vDecodedValues.empty()) { clCompField_.SetFieldValue<true>(field->index, vDecodedValues.data(), vDecodedValues.size()); }
                    }
                    else { clCompField_.SetFieldValue<false>(field->index, std::string(strValue)); }
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
                                clCompField_.SetFieldValue<true>(field->index, vDecodedValues.data(), vDecodedValues.size());
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
                            clCompField_.SetFieldValue<false>(field->index, std::move(vDecodedValues));
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
                                clCompField_.SetFieldValue<true>(field->index, vDecodedValues.data(), vDecodedValues.size());
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
                            clCompField_.SetFieldValue<false>(field->index, std::move(vDecodedValues));
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

            auto fieldArrayDef = std::dynamic_pointer_cast<const FieldArrayField>(field);
            if (!fieldArrayDef) { return STATUS::MALFORMED_INPUT; }
            const auto* subFieldDefinitions = fieldArrayDef.get();

            std::vector<CompositeField> vFieldArrayContainer;
            vFieldArrayContainer.reserve(array.size());

            for (simdjson::dom::element element : array)
            {
                vFieldArrayContainer.emplace_back(subFieldDefinitions->fieldInfo->fixedFieldBytes, subFieldDefinitions->fieldInfo->varFieldCount);
                auto& stSubMessageBody = vFieldArrayContainer.back();
                stSubMessageBody.SetFieldInfo(subFieldDefinitions->fieldInfo);

                // Recursively decode the subfields
                STATUS eStatus = DecodeJson(*subFieldDefinitions->fieldInfo, element, stSubMessageBody);
                if (eStatus != STATUS::SUCCESS) { return eStatus; }
            }

            clCompField_.SetFieldValue(*field, std::move(vFieldArrayContainer));
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
void MessageDecoderBase::DecodeJsonField(const BaseField& field_, simdjson::dom::element clJsonField_, CompositeField& clCompField_,
                                         const size_t elementIndex_, const bool fixed_) const
{
    const auto it = jsonFieldMap.find(field_.conversionHash);
    if (it == jsonFieldMap.end()) { throw std::runtime_error("DecodeJsonField(): Unknown field type\n"); }
    it->second(clCompField_, field_, clJsonField_, elementIndex_, fixed_, *pclMyMsgDb);
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoderBase::Decode(const unsigned char* pucMessage_, CompositeField& stInterMessage_, MetaDataBase& stMetaData_) const
{
    if (pucMessage_ == nullptr) { return STATUS::NULL_PROVIDED; }
    if (pclMyMsgDb == nullptr) { return STATUS::NO_DATABASE; }

    if (stMetaData_.bResponse && (stMetaData_.eFormat != DECODE_FORMAT::BINARY && stMetaData_.eFormat != DECODE_FORMAT::ASCII &&
                                  stMetaData_.eFormat != DECODE_FORMAT::ABB_ASCII))
    {
        return STATUS::NO_DEFINITION;
    }

    MessageDefinition::ConstPtr pclMsgDef = GetMessageDefinition(stMetaData_);

    if (pclMsgDef == nullptr)
    {
        LogMissingMsgDef(*pclMyLogger, stMetaData_.usMessageId);
        return STATUS::NO_DEFINITION;
    }

    if (stMetaData_.messageName == "RXCONFIG")
    {
        SPDLOG_LOGGER_INFO(pclMyLogger, "RXCONFIG payload must be decoded via RxConfigHandler, not MessageDecoder.");
        return STATUS::UNSUPPORTED;
    }
    const uint32_t uiMessageCrc = stMetaData_.bResponse ? 0 : stMetaData_.uiMessageCrc;
    const FieldInfo& msgFieldInfo = pclMsgDef->GetMsgDefFromCrc(uiMessageCrc);

    stInterMessage_.resize(msgFieldInfo.fixedFieldBytes, msgFieldInfo.varFieldCount);
    stInterMessage_.SetFieldInfo(pclMsgDef, uiMessageCrc);

    const auto* pucTempInData = pucMessage_;

    // Decode the detected format
    if (stMetaData_.eFormat == DECODE_FORMAT::BINARY)
    {
        if (msgFieldInfo.varFieldCount == 0)
        {
            // Fast path: if there are no variable-length fields, copy the entire message to the fixed region
            stInterMessage_.SetFieldValue<true>(0, reinterpret_cast<const std::byte*>(pucTempInData), msgFieldInfo.fixedFieldBytes);
            return STATUS::SUCCESS;
        }
        return DecodeBinary(msgFieldInfo, &pucTempInData, stInterMessage_, stMetaData_.uiBinaryMsgLength);
    }

    const auto headerLength = stMetaData_.eFormat == DECODE_FORMAT::JSON ? 0 : stMetaData_.uiHeaderLength;
    if (headerLength > stMetaData_.uiLength) { return STATUS::MALFORMED_INPUT; }
    auto sMessage = std::string_view(reinterpret_cast<const char*>(pucMessage_), stMetaData_.uiLength - headerLength);
    switch (stMetaData_.eFormat)
    {
    case DECODE_FORMAT::ASCII: return DecodeAscii<false>(msgFieldInfo, sMessage, stInterMessage_);
    case DECODE_FORMAT::ABB_ASCII: return DecodeAscii<true>(msgFieldInfo, sMessage, stInterMessage_);
    case DECODE_FORMAT::JSON: {
        simdjson::dom::parser parser;
        simdjson::dom::element clJsonFields;

        auto error = parser.parse(sMessage).get(clJsonFields);
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
