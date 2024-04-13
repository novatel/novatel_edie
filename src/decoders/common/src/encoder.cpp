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
// ! \file encoder.cpp
// ===============================================================================

#include "decoders/common/api/encoder.hpp"

#include <bitset>

using namespace novatel::edie;

// initialize static members
std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] JsonReader*)>> EncoderBase::asciiFieldMap;
std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] JsonReader*)>> EncoderBase::jsonFieldMap;

// -------------------------------------------------------------------------------------------------------
EncoderBase::EncoderBase(JsonReader* pclJsonDb_)
{
    InitFieldMaps();
    if (pclJsonDb_ != nullptr) { LoadJsonDb(pclJsonDb_); }
}

// -------------------------------------------------------------------------------------------------------
void EncoderBase::LoadJsonDb(JsonReader* pclJsonDb_)
{
    pclMyMsgDb = pclJsonDb_;
    InitEnumDefns();
}

// -------------------------------------------------------------------------------------------------------
void EncoderBase::InitEnumDefns() {}

// -------------------------------------------------------------------------------------------------------
void EncoderBase::InitFieldMaps()
{
    asciiFieldMap.clear();
    jsonFieldMap.clear();

    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCRC32("%UB")] = BasicMapEntry<uint8_t>("%u");
    asciiFieldMap[CalculateBlockCRC32("%B")] = BasicMapEntry<int8_t>("%d");
    asciiFieldMap[CalculateBlockCRC32("%XB")] = BasicMapEntry<uint8_t>("%02x");

    // =========================================================
    // Json Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCRC32("%UB")] = BasicMapEntry<uint8_t>("%u");
    asciiFieldMap[CalculateBlockCRC32("%B")] = BasicMapEntry<int8_t>("%d");
    asciiFieldMap[CalculateBlockCRC32("%XB")] = BasicMapEntry<uint8_t>("%02x");
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> EncoderBase::GetLogger() const { return pclMyLogger; }

// -------------------------------------------------------------------------------------------------------
void EncoderBase::SetLoggerLevel(spdlog::level::level_enum eLevel_) { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void EncoderBase::ShutdownLogger() { Logger::Shutdown(); }

// -------------------------------------------------------------------------------------------------------
template <bool FLATTEN>
bool EncoderBase::EncodeBinaryBody(const IntermediateMessage& stInterMessage_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    unsigned char* pucTempStart;

    for (const auto& field : stInterMessage_)
    {
        // Realign to type byte boundary if needed
        const uint32_t uiAlign = std::min(4U, static_cast<uint32_t>(field.field_def->dataType.length));
        if (auto ullRem = reinterpret_cast<uint64_t>(*ppcOutBuf_) % uiAlign; ullRem && !SetInBuffer(ppcOutBuf_, uiBytesLeft_, 0, uiAlign - ullRem))
        {
            return false;
        }

        if (std::holds_alternative<std::vector<FieldContainer>>(field.field_value))
        {
            const auto& vFCCurrentVectorField = std::get<std::vector<FieldContainer>>(field.field_value);

            // FIELD_ARRAY types contain several classes and so will use a recursive call
            if (field.field_def->type == FIELD_TYPE::FIELD_ARRAY)
            {
                const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.field_value);

                uint32_t uiFieldCount = static_cast<uint32_t>(vCurrentFieldArrayField.size());
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &uiFieldCount)) { return false; }

                pucTempStart = *ppcOutBuf_; // Move the start placeholder to the front of the array start

                for (const auto& clFieldArray : vCurrentFieldArrayField)
                {
                    if (!EncodeBinaryBody<FLATTEN>(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiBytesLeft_))
                    {
                        return false;
                    }
                }

                // For a flattened version of the log, fill in the remaining fields with 0x00.
                if constexpr (FLATTEN)
                {
                    if (static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart) < dynamic_cast<const FieldArrayField*>(field.field_def)->fieldSize &&
                        !SetInBuffer(ppcOutBuf_, uiBytesLeft_, '\0',
                                     dynamic_cast<const FieldArrayField*>(field.field_def)->fieldSize -
                                         static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart)))
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (field.field_def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
                {
                    // if the field is a variable array, print the size first
                    const uint32_t uiVarArraySize = static_cast<uint32_t>(vFCCurrentVectorField.size());
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &uiVarArraySize)) { return false; }
                }

                pucTempStart = *ppcOutBuf_; // Move the start placeholder to the front of the array start

                // This is an array of simple elements
                for (const auto& arrayField : vFCCurrentVectorField)
                {
                    if (!FieldToBinary(arrayField, ppcOutBuf_, uiBytesLeft_)) { return false; }
                }

                // For a flattened version of the log, fill in the remaining fields with 0x00.
                if constexpr (FLATTEN)
                {
                    const uint32_t uiMaxArraySize = dynamic_cast<const ArrayField*>(field.field_def)->arrayLength * field.field_def->dataType.length;
                    if (static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart) < uiMaxArraySize)
                    {
                        if (!SetInBuffer(ppcOutBuf_, uiBytesLeft_, '\0', uiMaxArraySize - static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart)))
                        {
                            return false;
                        }
                    }
                }
            }
        }
        else
        {
            switch (field.field_def->type)
            {
            case FIELD_TYPE::STRING: { // STRING types can be handled all at once because they are a single element and have a null terminator
                const char* szString = std::get<std::string>(field.field_value).c_str();
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, szString)) { return false; }

                // For a flattened version of the log, fill in the remaining characters with 0x00.
                if constexpr (FLATTEN)
                {
                    const auto uiStringLength = static_cast<uint32_t>(strlen(szString));
                    const uint32_t uiMaxArraySize = dynamic_cast<const ArrayField*>(field.field_def)->arrayLength * field.field_def->dataType.length;
                    if (uiStringLength < uiMaxArraySize && !SetInBuffer(ppcOutBuf_, uiBytesLeft_, '\0', uiMaxArraySize - uiStringLength))
                    {
                        return false;
                    }
                }
                else if (!SetInBuffer(ppcOutBuf_, uiBytesLeft_, '\0', 4 - (reinterpret_cast<uint64_t>(*ppcOutBuf_) % 4))) { return false; }
                break;
            }
            case FIELD_TYPE::ENUM:
                switch (field.field_def->dataType.length)
                {
                case 2:
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int16_t>(field.field_value))) { return false; }
                    break;
                case 4:
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int32_t>(field.field_value))) { return false; }
                    break;
                default: return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_ID:
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int32_t>(field.field_value))) { return false; }
                break;
            case FIELD_TYPE::RESPONSE_STR:
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<std::string>(field.field_value).c_str())) { return false; }
                break;
            case FIELD_TYPE::SIMPLE:
                if (!FieldToBinary(field, ppcOutBuf_, uiBytesLeft_)) { return false; }
                break;
            default: return false;
            }
        }
    }
    return true;
}

// explicit template instantiations
template bool EncoderBase::EncodeBinaryBody<true>(const IntermediateMessage&, unsigned char**, uint32_t&);
template bool EncoderBase::EncodeBinaryBody<false>(const IntermediateMessage&, unsigned char**, uint32_t&);

// -------------------------------------------------------------------------------------------------------
bool EncoderBase::FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    switch (fc_.field_def->dataType.name)
    {
    case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<bool>(fc_.field_value));
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint8_t>(fc_.field_value));
    case DATA_TYPE::CHAR: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int8_t>(fc_.field_value));
    case DATA_TYPE::USHORT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint16_t>(fc_.field_value));
    case DATA_TYPE::SHORT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int16_t>(fc_.field_value));
    case DATA_TYPE::UINT: [[fallthrough]];
    case DATA_TYPE::SATELLITEID: [[fallthrough]];
    case DATA_TYPE::ULONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint32_t>(fc_.field_value));
    case DATA_TYPE::INT: [[fallthrough]];
    case DATA_TYPE::LONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int32_t>(fc_.field_value));
    case DATA_TYPE::ULONGLONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint64_t>(fc_.field_value));
    case DATA_TYPE::LONGLONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int64_t>(fc_.field_value));
    case DATA_TYPE::FLOAT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<float>(fc_.field_value));
    case DATA_TYPE::DOUBLE: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<double>(fc_.field_value));
    default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToBinary(): unknown type."); throw std::runtime_error("FieldToBinary(): unknown type.");
    }
}

// -------------------------------------------------------------------------------------------------------
template <bool ABBREVIATED>
bool EncoderBase::EncodeAsciiBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                  uint32_t uiIndentationLevel)
{
    // cant figure out how to do this constexpr since derived classes need to change values
    char separator = ABBREVIATED ? separatorAbbASCII() : separatorASCII();

    [[maybe_unused]] bool new_indent_line = false;

    if constexpr (ABBREVIATED)
    {
        std::string strIndentation(static_cast<uint64_t>(uiIndentationLevel) * indentationLengthAbbASCII(), ' ');
        if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "<%s", strIndentation.c_str())) return false;
    }

    for (const auto& field : vIntermediateFormat_)
    {
        if constexpr (ABBREVIATED)
        {
            if (new_indent_line)
            {
                std::string strIndentation(static_cast<uint64_t>(uiIndentationLevel) * indentationLengthAbbASCII(), ' ');
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<%s", strIndentation.c_str())) return false;
                new_indent_line = false;
            }
        }

        if (std::holds_alternative<std::vector<FieldContainer>>(field.field_value))
        {
            const auto& vFCCurrentVectorField = std::get<std::vector<FieldContainer>>(field.field_value);

            // FIELD_ARRAY types contain several classes and so will use a recursive call
            if (field.field_def->type == FIELD_TYPE::FIELD_ARRAY)
            {
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d%c", vFCCurrentVectorField.size(), separator)) { return false; }

                const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.field_value);

                if constexpr (ABBREVIATED)
                {
                    // Abbrev ascii will output a blank line for a 0 length array, nice
                    if (vCurrentFieldArrayField.empty())
                    {
                        std::string strIndentation(static_cast<uint64_t>(uiIndentationLevel + 1) * indentationLengthAbbASCII(), ' ');
                        if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<%s", strIndentation.c_str())) return false;
                    }
                    // Data was printed so a new line is required at the end of the array if there are more fields in the log
                    else
                    {
                        for (const auto& clFieldArray : vCurrentFieldArrayField)
                        {
                            if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n")) { return false; }

                            if (!EncodeAsciiBody<true>(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiBytesLeft_,
                                                       uiIndentationLevel + 1))
                            {
                                return false;
                            }
                        }
                        new_indent_line = true;
                    }
                }
                else
                {
                    for (const auto& clFieldArray : vCurrentFieldArrayField)
                    {
                        if (!EncodeAsciiBody<false>(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiBytesLeft_))
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                const bool bPrintAsString = PrintAsString(field.field_def);
                const bool bIsCommaSeparated = IsCommaSeparated(field.field_def);

                // if the field is a variable array, print the size first
                if (field.field_def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY &&
                    !PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d%c", vFCCurrentVectorField.size(), separator))
                {
                    return false;
                }

                if (bPrintAsString && !PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"")) { return false; }

                // This is an array of simple elements
                for (const auto& arrayField : vFCCurrentVectorField)
                {
                    // If we are printing a string, don't print the null terminator or any padding bytes
                    if (bPrintAsString)
                    {
                        if ((std::holds_alternative<int8_t>(arrayField.field_value) && std::get<int8_t>(arrayField.field_value) == '\0') ||
                            (std::holds_alternative<uint8_t>(arrayField.field_value) && std::get<uint8_t>(arrayField.field_value) == '\0'))
                        {
                            break;
                        }
                    }

                    if (!FieldToAscii(arrayField, ppcOutBuf_, uiBytesLeft_)) { return false; }

                    if (bIsCommaSeparated)
                    {
                        if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", separator)) { return false; }
                    }
                }
                // Quoted elements need a trailing comma
                if (bPrintAsString)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"%c", separator)) { return false; }
                }
                // Non-quoted, non-internally-separated elements also need a trailing comma
                else if (!bIsCommaSeparated)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", separator)) { return false; }
                }
            }
        }
        else
        {
            switch (field.field_def->type)
            {
            case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"%s\"%c", std::get<std::string>(field.field_value).c_str(), separator))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::ENUM: {
                const auto enumField = dynamic_cast<const EnumField*>(field.field_def);
                if (enumField->length == 2)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c",
                                       GetEnumString(enumField->enumDef, std::get<int16_t>(field.field_value)).c_str(), separator))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c",
                                       GetEnumString(enumField->enumDef, std::get<int32_t>(field.field_value)).c_str(), separator))
                    {
                        return false;
                    }
                }
                break;
            }
            case FIELD_TYPE::RESPONSE_ID: break; // Do nothing, ascii logs don't output this field
            case FIELD_TYPE::RESPONSE_STR:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", std::get<std::string>(field.field_value).c_str(), separator)) { return false; }
                break;
            case FIELD_TYPE::SIMPLE:
                if (!FieldToAscii(field, ppcOutBuf_, uiBytesLeft_) || !PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%c", separator)) { return false; }
                break;
            default: return false;
            }
        }
    }
    return true;
}

// explicit template instantiations
template bool EncoderBase::EncodeAsciiBody<true>(const std::vector<FieldContainer>&, char**, uint32_t& uiBytesLeft_, uint32_t);
template bool EncoderBase::EncodeAsciiBody<false>(const std::vector<FieldContainer>&, char**, uint32_t& uiBytesLeft_, uint32_t);

// -------------------------------------------------------------------------------------------------------
bool EncoderBase::FieldToAscii(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    auto it = asciiFieldMap.find(fc_.field_def->conversionHash);
    if (it != asciiFieldMap.end()) { return it->second(fc_, ppcOutBuf_, uiBytesLeft_, pclMyMsgDb); }
    const char* pcConvertString = fc_.field_def->conversion.c_str();

    switch (fc_.field_def->dataType.name)
    {
    case DATA_TYPE::BOOL: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<bool>(fc_.field_value) ? "TRUE" : "FALSE");
    case DATA_TYPE::HEXBYTE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%02x", std::get<uint8_t>(fc_.field_value));
    case DATA_TYPE::UCHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint8_t>(fc_.field_value));
    case DATA_TYPE::CHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int8_t>(fc_.field_value));
    case DATA_TYPE::USHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint16_t>(fc_.field_value));
    case DATA_TYPE::SHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int16_t>(fc_.field_value));
    case DATA_TYPE::UINT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint32_t>(fc_.field_value));
    case DATA_TYPE::INT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int32_t>(fc_.field_value));
    case DATA_TYPE::ULONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint32_t>(fc_.field_value));
    case DATA_TYPE::ULONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint64_t>(fc_.field_value));
    case DATA_TYPE::LONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int32_t>(fc_.field_value));
    case DATA_TYPE::LONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int64_t>(fc_.field_value));
    case DATA_TYPE::FLOAT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<float>(fc_.field_value));
    case DATA_TYPE::DOUBLE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<double>(fc_.field_value));
    default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToAscii(): unknown type."); throw std::runtime_error("FieldToAscii(): unknown type.");
    }
}

// -------------------------------------------------------------------------------------------------------
bool EncoderBase::EncodeJsonBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "{")) { return false; }

    for (const auto& field : vIntermediateFormat_)
    {
        if (std::holds_alternative<std::vector<FieldContainer>>(field.field_value))
        {
            const auto& vFCCurrentVectorField = std::get<std::vector<FieldContainer>>(field.field_value);

            // FIELD_ARRAY types contain several classes and so will use a recursive call
            if (field.field_def->type == FIELD_TYPE::FIELD_ARRAY)
            {
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": [)", field.field_def->name.c_str())) { return false; }
                const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.field_value);
                if (vCurrentFieldArrayField.empty())
                {
                    if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "],")) { return false; }
                }
                else
                {
                    for (const auto& clFieldArray : vCurrentFieldArrayField)
                    {
                        if (!EncodeJsonBody(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiBytesLeft_))
                        {
                            return false;
                        }
                        if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, ",")) { return false; }
                    }
                    *(*ppcOutBuf_ - 1) = ']';
                    if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, ",")) { return false; }
                }
            }
            else
            {
                const bool bPrintAsString = PrintAsString(field.field_def);

                if (bPrintAsString)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": ")", field.field_def->name.c_str())) { return false; }
                }
                else
                {
                    // This is an array of simple elements
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": [)", field.field_def->name.c_str())) { return false; }
                    if (vFCCurrentVectorField.empty())
                    {
                        if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "]")) { return false; }
                    }
                }

                for (const auto& arrayField : vFCCurrentVectorField)
                {
                    // If we are printing a string, don't print the null terminator or any padding bytes
                    if (bPrintAsString)
                    {
                        if (std::holds_alternative<int8_t>(arrayField.field_value) && std::get<int8_t>(arrayField.field_value) == '\0') { break; }
                        if (std::holds_alternative<uint8_t>(arrayField.field_value) && std::get<uint8_t>(arrayField.field_value) == '\0') { break; }
                    }

                    if (!FieldToJson(arrayField, ppcOutBuf_, uiBytesLeft_)) { return false; }
                    if (!bPrintAsString && !CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, ",")) { return false; }
                }

                // Quoted elements need a trailing comma
                if (bPrintAsString)
                {
                    if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "\",")) { return false; }
                }
                // Non-quoted, non-internally-separated elements also need a trailing comma
                else
                {
                    *(*ppcOutBuf_ - 1) = ']';
                    if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, ",")) { return false; }
                }
            }
        }
        else
        {
            switch (field.field_def->type)
            {
            case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": "%s",)", field.field_def->name.c_str(),
                                   std::get<std::string>(field.field_value).c_str()))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::ENUM:
                if (!PrintToBuffer(
                        ppcOutBuf_, uiBytesLeft_, R"("%s": "%s",)", field.field_def->name.c_str(),
                        GetEnumString(dynamic_cast<const EnumField*>(field.field_def)->enumDef, std::get<int32_t>(field.field_value)).c_str()))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_ID:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": %d,)", field.field_def->name.c_str(), std::get<int32_t>(field.field_value)))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_STR:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": "%s",)", field.field_def->name.c_str(),
                                   std::get<std::string>(field.field_value).c_str()))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::SIMPLE:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": )", field.field_def->name.c_str()) ||
                    !FieldToJson(field, ppcOutBuf_, uiBytesLeft_) || !CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, ","))
                {
                    return false;
                }
                break;
            default: return false;
            }
        }
    }
    *(*ppcOutBuf_ - 1) = '}';
    return true;
}

// -------------------------------------------------------------------------------------------------------
bool EncoderBase::FieldToJson(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    auto it = jsonFieldMap.find(fc_.field_def->conversionHash);
    if (it != jsonFieldMap.end()) { return it->second(fc_, ppcOutBuf_, uiBytesLeft_, pclMyMsgDb); }
    const char* pcConvertString = fc_.field_def->conversion.c_str();

    switch (fc_.field_def->dataType.name)
    {
    case DATA_TYPE::BOOL: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<bool>(fc_.field_value) ? "true" : "false");
    case DATA_TYPE::HEXBYTE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hhu", std::get<uint8_t>(fc_.field_value));
    case DATA_TYPE::UCHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hhu", std::get<uint8_t>(fc_.field_value));
    case DATA_TYPE::CHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hhd", std::get<int8_t>(fc_.field_value));
    case DATA_TYPE::USHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu", std::get<uint16_t>(fc_.field_value));
    case DATA_TYPE::SHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hd", std::get<int16_t>(fc_.field_value));
    case DATA_TYPE::UINT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%u", std::get<uint32_t>(fc_.field_value));
    case DATA_TYPE::INT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d", std::get<int32_t>(fc_.field_value));
    case DATA_TYPE::ULONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%u", std::get<uint32_t>(fc_.field_value));
    case DATA_TYPE::LONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d", std::get<int32_t>(fc_.field_value));
    case DATA_TYPE::ULONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%llu", std::get<uint64_t>(fc_.field_value));
    case DATA_TYPE::LONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%lld", std::get<int64_t>(fc_.field_value));
    case DATA_TYPE::FLOAT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<float>(fc_.field_value));
    case DATA_TYPE::DOUBLE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<double>(fc_.field_value));
    default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToJson(): unknown type."); throw std::runtime_error("FieldToJson(): unknown type.");
    }
}
