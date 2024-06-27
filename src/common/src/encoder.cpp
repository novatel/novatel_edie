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

#include "novatel_edie/common/encoder.hpp"

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
    InitEnumDefinitions();
}

// -------------------------------------------------------------------------------------------------------
void EncoderBase::InitEnumDefinitions() {}

// -------------------------------------------------------------------------------------------------------
void EncoderBase::InitFieldMaps()
{
    asciiFieldMap.clear();
    jsonFieldMap.clear();

    // =========================================================
    // ASCII Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("%UB")] = BasicMapEntry<uint8_t>("%u");
    asciiFieldMap[CalculateBlockCrc32("%B")] = BasicMapEntry<int8_t>("%d");
    asciiFieldMap[CalculateBlockCrc32("%XB")] = BasicMapEntry<uint8_t>("%02x");

    // =========================================================
    // Json Field Mapping
    // =========================================================
    asciiFieldMap[CalculateBlockCrc32("%UB")] = BasicMapEntry<uint8_t>("%u");
    asciiFieldMap[CalculateBlockCrc32("%B")] = BasicMapEntry<int8_t>("%d");
    asciiFieldMap[CalculateBlockCrc32("%XB")] = BasicMapEntry<uint8_t>("%02x");
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> EncoderBase::GetLogger() const { return pclMyLogger; }

// -------------------------------------------------------------------------------------------------------
void EncoderBase::SetLoggerLevel(const spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

// -------------------------------------------------------------------------------------------------------
void EncoderBase::ShutdownLogger() { Logger::Shutdown(); }

// -------------------------------------------------------------------------------------------------------
template <bool Flatten, bool Align>
bool EncoderBase::EncodeBinaryBody(const std::vector<FieldContainer>& stInterMessage_, unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_)
{
    unsigned char* pucTempStart;

    for (const auto& field : stInterMessage_)
    {
        if constexpr (Align)
        {
            // Realign to type byte boundary if needed
            const uint32_t uiAlign = std::min(4U, static_cast<uint32_t>(field.fieldDef->dataType.length));
            const auto ullRem = reinterpret_cast<uint64_t>(*ppucOutBuf_) % uiAlign;

            if (ullRem && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, uiAlign - ullRem)) { return false; }
        }

        if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
        {
            const auto& vFcCurrentVectorField = std::get<std::vector<FieldContainer>>(field.fieldValue);

            // FIELD_ARRAY types contain several classes and so will use a recursive call
            if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
            {
                const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.fieldValue);

                uint32_t uiFieldCount = static_cast<uint32_t>(vCurrentFieldArrayField.size());
                if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, &uiFieldCount)) { return false; }

                pucTempStart = *ppucOutBuf_; // Move the start placeholder to the front of the array start

                for (const auto& clFieldArray : vCurrentFieldArrayField)
                {
                    if (!EncodeBinaryBody<Flatten, Align>(std::get<std::vector<FieldContainer>>(clFieldArray.fieldValue), ppucOutBuf_, uiBytesLeft_))
                    {
                        return false;
                    }
                }

                // For a flattened version of the log, fill in the remaining fields with 0x00.
                if constexpr (Flatten)
                {
                    if (static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart) < dynamic_cast<const FieldArrayField*>(field.fieldDef)->fieldSize &&
                        !SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0',
                                     dynamic_cast<const FieldArrayField*>(field.fieldDef)->fieldSize -
                                         static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart)))
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (field.fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
                {
                    // if the field is a variable array, print the size first
                    const uint32_t uiVarArraySize = static_cast<uint32_t>(vFcCurrentVectorField.size());
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, &uiVarArraySize)) { return false; }
                }

                pucTempStart = *ppucOutBuf_; // Move the start placeholder to the front of the array start

                // This is an array of simple elements
                for (const auto& arrayField : vFcCurrentVectorField)
                {
                    if (!FieldToBinary(arrayField, ppucOutBuf_, uiBytesLeft_)) { return false; }
                }

                // For a flattened version of the log, fill in the remaining fields with 0x00.
                if constexpr (Flatten)
                {
                    const uint32_t uiMaxArraySize = dynamic_cast<const ArrayField*>(field.fieldDef)->arrayLength * field.fieldDef->dataType.length;
                    if (static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart) < uiMaxArraySize)
                    {
                        if (!SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', uiMaxArraySize - static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart)))
                        {
                            return false;
                        }
                    }
                }
            }
        }
        else
        {
            switch (field.fieldDef->type)
            {
            case FIELD_TYPE::STRING: { // STRING types can be handled all at once because they are a single element and have a null terminator
                const char* szString = std::get<std::string>(field.fieldValue).c_str();
                if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, szString)) { return false; }

                // For a flattened version of the log, fill in the remaining characters with 0x00.
                if constexpr (Flatten)
                {
                    const auto uiStringLength = static_cast<uint32_t>(strlen(szString));
                    const uint32_t uiMaxArraySize = dynamic_cast<const ArrayField*>(field.fieldDef)->arrayLength * field.fieldDef->dataType.length;
                    if (uiStringLength < uiMaxArraySize && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', uiMaxArraySize - uiStringLength))
                    {
                        return false;
                    }
                }
                else if (!SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', 4 - (reinterpret_cast<uint64_t>(*ppucOutBuf_) % 4))) { return false; }
                break;
            }
            case FIELD_TYPE::ENUM:
                switch (field.fieldDef->dataType.length)
                {
                case 2:
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, &std::get<int16_t>(field.fieldValue))) { return false; }
                    break;
                case 4:
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, &std::get<int32_t>(field.fieldValue))) { return false; }
                    break;
                default: return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_ID:
                if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, &std::get<int32_t>(field.fieldValue))) { return false; }
                break;
            case FIELD_TYPE::RESPONSE_STR:
                if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, std::get<std::string>(field.fieldValue).c_str())) { return false; }
                break;
            case FIELD_TYPE::SIMPLE:
                if (!FieldToBinary(field, ppucOutBuf_, uiBytesLeft_)) { return false; }
                break;
            default: return false;
            }
        }
    }
    return true;
}

// explicit template instantiations
template bool EncoderBase::EncodeBinaryBody<false, false>(const std::vector<FieldContainer>&, unsigned char**, uint32_t&);
template bool EncoderBase::EncodeBinaryBody<false, true>(const std::vector<FieldContainer>&, unsigned char**, uint32_t&);
template bool EncoderBase::EncodeBinaryBody<true, false>(const std::vector<FieldContainer>&, unsigned char**, uint32_t&);
template bool EncoderBase::EncodeBinaryBody<true, true>(const std::vector<FieldContainer>&, unsigned char**, uint32_t&);

// -------------------------------------------------------------------------------------------------------
bool EncoderBase::FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    switch (fc_.fieldDef->dataType.name)
    {
    case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<bool>(fc_.fieldValue));
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint8_t>(fc_.fieldValue));
    case DATA_TYPE::CHAR: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int8_t>(fc_.fieldValue));
    case DATA_TYPE::USHORT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint16_t>(fc_.fieldValue));
    case DATA_TYPE::SHORT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int16_t>(fc_.fieldValue));
    case DATA_TYPE::UINT: [[fallthrough]];
    case DATA_TYPE::SATELLITEID: [[fallthrough]];
    case DATA_TYPE::ULONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint32_t>(fc_.fieldValue));
    case DATA_TYPE::INT: [[fallthrough]];
    case DATA_TYPE::LONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int32_t>(fc_.fieldValue));
    case DATA_TYPE::ULONGLONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<uint64_t>(fc_.fieldValue));
    case DATA_TYPE::LONGLONG: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<int64_t>(fc_.fieldValue));
    case DATA_TYPE::FLOAT: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<float>(fc_.fieldValue));
    case DATA_TYPE::DOUBLE: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, &std::get<double>(fc_.fieldValue));
    default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToBinary(): unknown type."); throw std::runtime_error("FieldToBinary(): unknown type.");
    }
}

// -------------------------------------------------------------------------------------------------------
template <bool Abbreviated>
bool EncoderBase::EncodeAsciiBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                  const uint32_t uiIndentationLevel_)
{
    // cant figure out how to do this constexpr since derived classes need to change values
    const char separator = Abbreviated ? SeparatorAbbAscii() : SeparatorAscii();

    [[maybe_unused]] bool newIndentLine = false;

    if constexpr (Abbreviated)
    {
        std::string strIndentation(static_cast<uint64_t>(uiIndentationLevel_) * IndentationLengthAbbAscii(), ' ');
        if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "<%s", strIndentation.c_str())) { return false; }
    }

    for (const auto& field : vIntermediateFormat_)
    {
        if constexpr (Abbreviated)
        {
            if (newIndentLine)
            {
                std::string strIndentation(static_cast<uint64_t>(uiIndentationLevel_) * IndentationLengthAbbAscii(), ' ');
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<%s", strIndentation.c_str())) { return false; }
                newIndentLine = false;
            }
        }

        if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
        {
            const auto& vFcCurrentVectorField = std::get<std::vector<FieldContainer>>(field.fieldValue);

            // FIELD_ARRAY types contain several classes and so will use a recursive call
            if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
            {
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d%c", vFcCurrentVectorField.size(), separator)) { return false; }

                const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.fieldValue);

                if constexpr (Abbreviated)
                {
                    // Abbrev ascii will output a blank line for a 0 length array, nice
                    if (vCurrentFieldArrayField.empty())
                    {
                        std::string strIndentation(static_cast<uint64_t>(uiIndentationLevel_ + 1) * IndentationLengthAbbAscii(), ' ');
                        if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<%s", strIndentation.c_str())) { return false; }
                    }
                    // Data was printed so a new line is required at the end of the array if there are more fields in the log
                    else
                    {
                        for (const auto& clFieldArray : vCurrentFieldArrayField)
                        {
                            if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n")) { return false; }

                            if (!EncodeAsciiBody<true>(std::get<std::vector<FieldContainer>>(clFieldArray.fieldValue), ppcOutBuf_, uiBytesLeft_,
                                                       uiIndentationLevel_ + 1))
                            {
                                return false;
                            }
                        }
                        newIndentLine = true;
                    }
                }
                else
                {
                    for (const auto& clFieldArray : vCurrentFieldArrayField)
                    {
                        if (!EncodeAsciiBody<false>(std::get<std::vector<FieldContainer>>(clFieldArray.fieldValue), ppcOutBuf_, uiBytesLeft_))
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                const bool bPrintAsString = PrintAsString(field.fieldDef);
                const bool bIsCommaSeparated = IsCommaSeparated(field.fieldDef);

                // if the field is a variable array, print the size first
                if (field.fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY &&
                    !PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d%c", vFcCurrentVectorField.size(), separator))
                {
                    return false;
                }

                if (bPrintAsString && !PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"")) { return false; }

                // This is an array of simple elements
                for (const auto& arrayField : vFcCurrentVectorField)
                {
                    // If we are printing a string, don't print the null terminator or any padding bytes
                    if (bPrintAsString)
                    {
                        if ((std::holds_alternative<int8_t>(arrayField.fieldValue) && std::get<int8_t>(arrayField.fieldValue) == '\0') ||
                            (std::holds_alternative<uint8_t>(arrayField.fieldValue) && std::get<uint8_t>(arrayField.fieldValue) == '\0'))
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
            switch (field.fieldDef->type)
            {
            case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"%s\"%c", std::get<std::string>(field.fieldValue).c_str(), separator))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::ENUM: {
                const auto enumField = dynamic_cast<const EnumField*>(field.fieldDef);
                if (enumField->length == 2)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c",
                                       GetEnumString(enumField->enumDef, std::get<int16_t>(field.fieldValue)).c_str(), separator))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c",
                                       GetEnumString(enumField->enumDef, std::get<int32_t>(field.fieldValue)).c_str(), separator))
                    {
                        return false;
                    }
                }
                break;
            }
            case FIELD_TYPE::RESPONSE_ID: break; // Do nothing, ascii logs don't output this field
            case FIELD_TYPE::RESPONSE_STR:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%s%c", std::get<std::string>(field.fieldValue).c_str(), separator)) { return false; }
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
bool EncoderBase::FieldToAscii(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    auto it = asciiFieldMap.find(fc_.fieldDef->conversionHash);
    if (it != asciiFieldMap.end()) { return it->second(fc_, ppcOutBuf_, uiBytesLeft_, pclMyMsgDb); }
    const char* pcConvertString = fc_.fieldDef->conversion.c_str();

    switch (fc_.fieldDef->dataType.name)
    {
    case DATA_TYPE::BOOL: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<bool>(fc_.fieldValue) ? "TRUE" : "FALSE");
    case DATA_TYPE::HEXBYTE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%02x", std::get<uint8_t>(fc_.fieldValue));
    case DATA_TYPE::UCHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint8_t>(fc_.fieldValue));
    case DATA_TYPE::CHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int8_t>(fc_.fieldValue));
    case DATA_TYPE::USHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint16_t>(fc_.fieldValue));
    case DATA_TYPE::SHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int16_t>(fc_.fieldValue));
    case DATA_TYPE::UINT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint32_t>(fc_.fieldValue));
    case DATA_TYPE::INT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int32_t>(fc_.fieldValue));
    case DATA_TYPE::ULONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint32_t>(fc_.fieldValue));
    case DATA_TYPE::ULONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<uint64_t>(fc_.fieldValue));
    case DATA_TYPE::LONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int32_t>(fc_.fieldValue));
    case DATA_TYPE::LONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<int64_t>(fc_.fieldValue));
    case DATA_TYPE::FLOAT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<float>(fc_.fieldValue));
    case DATA_TYPE::DOUBLE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<double>(fc_.fieldValue));
    default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToAscii(): unknown type."); throw std::runtime_error("FieldToAscii(): unknown type.");
    }
}

// -------------------------------------------------------------------------------------------------------
bool EncoderBase::EncodeJsonBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_)
{
    if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "{")) { return false; }

    for (const auto& field : vIntermediateFormat_)
    {
        if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
        {
            const auto& vFcCurrentVectorField = std::get<std::vector<FieldContainer>>(field.fieldValue);

            // FIELD_ARRAY types contain several classes and so will use a recursive call
            if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
            {
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": [)", field.fieldDef->name.c_str())) { return false; }
                const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.fieldValue);
                if (vCurrentFieldArrayField.empty())
                {
                    if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "],")) { return false; }
                }
                else
                {
                    for (const auto& clFieldArray : vCurrentFieldArrayField)
                    {
                        if (!EncodeJsonBody(std::get<std::vector<FieldContainer>>(clFieldArray.fieldValue), ppcOutBuf_, uiBytesLeft_))
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
                const bool bPrintAsString = PrintAsString(field.fieldDef);

                if (bPrintAsString)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": ")", field.fieldDef->name.c_str())) { return false; }
                }
                else
                {
                    // This is an array of simple elements
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": [)", field.fieldDef->name.c_str())) { return false; }
                    if (vFcCurrentVectorField.empty())
                    {
                        if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiBytesLeft_, "]")) { return false; }
                    }
                }

                for (const auto& arrayField : vFcCurrentVectorField)
                {
                    // If we are printing a string, don't print the null terminator or any padding bytes
                    if (bPrintAsString)
                    {
                        if (std::holds_alternative<int8_t>(arrayField.fieldValue) && std::get<int8_t>(arrayField.fieldValue) == '\0') { break; }
                        if (std::holds_alternative<uint8_t>(arrayField.fieldValue) && std::get<uint8_t>(arrayField.fieldValue) == '\0') { break; }
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
            switch (field.fieldDef->type)
            {
            case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": "%s",)", field.fieldDef->name.c_str(),
                                   std::get<std::string>(field.fieldValue).c_str()))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::ENUM:
                if (!PrintToBuffer(
                        ppcOutBuf_, uiBytesLeft_, R"("%s": "%s",)", field.fieldDef->name.c_str(),
                        GetEnumString(dynamic_cast<const EnumField*>(field.fieldDef)->enumDef, std::get<int32_t>(field.fieldValue)).c_str()))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_ID:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": %d,)", field.fieldDef->name.c_str(), std::get<int32_t>(field.fieldValue)))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_STR:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": "%s",)", field.fieldDef->name.c_str(),
                                   std::get<std::string>(field.fieldValue).c_str()))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::SIMPLE:
                if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("%s": )", field.fieldDef->name.c_str()) ||
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
bool EncoderBase::FieldToJson(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
{
    auto it = jsonFieldMap.find(fc_.fieldDef->conversionHash);
    if (it != jsonFieldMap.end()) { return it->second(fc_, ppcOutBuf_, uiBytesLeft_, pclMyMsgDb); }
    const char* pcConvertString = fc_.fieldDef->conversion.c_str();

    switch (fc_.fieldDef->dataType.name)
    {
    case DATA_TYPE::BOOL: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<bool>(fc_.fieldValue) ? "true" : "false");
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hhu", std::get<uint8_t>(fc_.fieldValue));
    case DATA_TYPE::CHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hhd", std::get<int8_t>(fc_.fieldValue));
    case DATA_TYPE::USHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hu", std::get<uint16_t>(fc_.fieldValue));
    case DATA_TYPE::SHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%hd", std::get<int16_t>(fc_.fieldValue));
    case DATA_TYPE::UINT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%u", std::get<uint32_t>(fc_.fieldValue));
    case DATA_TYPE::INT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d", std::get<int32_t>(fc_.fieldValue));
    case DATA_TYPE::ULONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%u", std::get<uint32_t>(fc_.fieldValue));
    case DATA_TYPE::LONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%d", std::get<int32_t>(fc_.fieldValue));
    case DATA_TYPE::ULONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%llu", std::get<uint64_t>(fc_.fieldValue));
    case DATA_TYPE::LONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "%lld", std::get<int64_t>(fc_.fieldValue));
    case DATA_TYPE::FLOAT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<float>(fc_.fieldValue));
    case DATA_TYPE::DOUBLE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<double>(fc_.fieldValue));
    default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToJson(): unknown type."); throw std::runtime_error("FieldToJson(): unknown type.");
    }
}
