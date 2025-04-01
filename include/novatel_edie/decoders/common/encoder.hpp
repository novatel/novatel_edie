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
// ! \file encoder.hpp
// ===============================================================================

#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <cstdarg>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

// -------------------------------------------------------------------------------------------------------
constexpr bool PrintAsString(const BaseField& pstFieldDef_)
{
    // Printing as a string means two things:
    // 1. The field will be surrounded by quotes
    // 2. The field will not contain null-termination or padding
    return pstFieldDef_.type == FIELD_TYPE::STRING || pstFieldDef_.conversionHash == CalculateBlockCrc32("s") ||
           pstFieldDef_.conversionHash == CalculateBlockCrc32("S");
}

// -------------------------------------------------------------------------------------------------------
constexpr bool IsCommaSeparated(const BaseField& pstFieldDef_)
{
    // In certain cases there are no separators printed between array elements
    return !PrintAsString(pstFieldDef_) && pstFieldDef_.conversionHash != CalculateBlockCrc32("Z") &&
           pstFieldDef_.conversionHash != CalculateBlockCrc32("P");
}

// -------------------------------------------------------------------------------------------------------
template <typename T> constexpr std::array<char, 9> FloatingPointConversionString(const FieldContainer& fc_)
{
    static_assert(std::is_floating_point_v<T>, "FloatingPointConversionString must be called with a floating point type");
    std::array<char, 9> acConvert{};
    const int32_t iBefore = fc_.fieldDef->conversionBeforePoint;
    const int32_t iAfter = fc_.fieldDef->conversionAfterPoint;
    const auto absVal = fabs(std::get<T>(fc_.fieldValue));

    auto it = fmt::format_to(acConvert.begin(), "{{:");
    it = (absVal < std::numeric_limits<T>::epsilon()) ? fmt::format_to(it, "0.{}f", iAfter)
         : (iAfter == 0 && iBefore == 0)              ? fmt::format_to(it, "0.1f")
         : (absVal > std::pow(10, iBefore))           ? fmt::format_to(it, "0.{}e", iBefore + iAfter - 1)
         : (absVal < std::pow(10, -iBefore))          ? fmt::format_to(it, "0.{}e", iAfter)
                                                      : fmt::format_to(it, "0.{}f", iAfter);
    *it++ = '}';
    *it = '\0';

    assert(static_cast<size_t>(std::distance(acConvert.begin(), it)) <= acConvert.size());

    return acConvert;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename... Args>
[[nodiscard]] bool PrintToBuffer(BufferType* ppcBuffer_, uint32_t& uiBytesLeft_, fmt::format_string<Args...> szFormat_, Args&&... args_)
{
    // TODO: In C++20 we can do compile time checking to ensure format string is valid for args.
    static_assert(sizeof...(Args) > 0, "Use CopyToBuffer if you don't need dynamic formatting.");
    const auto result = fmt::format_to_n(*ppcBuffer_, uiBytesLeft_, szFormat_, std::forward<Args>(args_)...);
    if (result.size > uiBytesLeft_) { return false; }
    *ppcBuffer_ += result.size;
    uiBytesLeft_ -= static_cast<uint32_t>(result.size);
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType>
[[nodiscard]] inline bool SetInBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, const unsigned char iItem_, const uint32_t uiItemSize_)
{
    if (uiBytesLeft_ < uiItemSize_) { return false; }
    memset(*ppucBuffer_, iItem_, uiItemSize_);
    *ppucBuffer_ += uiItemSize_;
    uiBytesLeft_ -= uiItemSize_;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename T> [[nodiscard]] bool CopyToBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, T* ptItem_)
{
    uint32_t uiItemSize;

    if constexpr (std::is_same<T, const char>()) { uiItemSize = static_cast<uint32_t>(std::strlen(ptItem_)); }
    else { uiItemSize = sizeof(*ptItem_); }

    if (uiBytesLeft_ < uiItemSize) { return false; }
    std::memcpy(*ppucBuffer_, ptItem_, uiItemSize);
    *ppucBuffer_ += uiItemSize;
    uiBytesLeft_ -= uiItemSize;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType> [[nodiscard]] inline bool CopyToBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, std::string_view sv)
{
    if (uiBytesLeft_ < sv.size()) { return false; }
    std::memcpy(*ppucBuffer_, sv.data(), sv.size());
    *ppucBuffer_ += sv.size();
    uiBytesLeft_ -= static_cast<uint32_t>(sv.size());
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType> [[nodiscard]] inline bool CopyToBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, unsigned char ucItem_)
{
    if (uiBytesLeft_ < 1) { return false; }
    *(*ppucBuffer_)++ = ucItem_;
    uiBytesLeft_--;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename... Args>
[[nodiscard]] bool CopyFormattedToBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, const char cSeparator_, Args&&... args_)
{
    auto CopyArg = [&](auto&& arg, std::string_view szFormat) -> bool {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::string_view> || std::is_same_v<std::decay_t<decltype(arg)>, const char*> ||
                      std::is_same_v<std::decay_t<decltype(arg)>, char>)
        {
            return CopyToBuffer(ppucBuffer_, uiBytesLeft_, arg);
        }
        else { return PrintToBuffer(ppucBuffer_, uiBytesLeft_, szFormat, arg); }
    };

    bool bSuccess = true;
    size_t i = 0;
    auto CopyWithSeparator = [&](auto&& arg) -> bool {
        bSuccess &= CopyArg(arg.first, arg.second);
        if (bSuccess && (i++ < sizeof...(Args) - 1)) { bSuccess &= CopyToBuffer(ppucBuffer_, uiBytesLeft_, cSeparator_); }
        return bSuccess;
    };

    (CopyWithSeparator(args_), ...);

    return bSuccess;
}

// -------------------------------------------------------------------------------------------------------
template <typename T> std::function<bool(const FieldContainer&, char**, uint32_t&, MessageDatabase&)> BasicMapEntry(const char* pcF_)
{
    return [pcF_](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
        return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcF_, std::get<T>(fc_.fieldValue));
    };
}

//============================================================================
//! \class EncoderBase
//! \brief Class to encode messages.
//============================================================================
template <typename Derived> class EncoderBase
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger{pclLoggerManager->RegisterLogger("encoder")};
    MessageDatabase::Ptr pclMyMsgDb{nullptr};

    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] MessageDatabase&)>> asciiFieldMap;
    std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] MessageDatabase&)>> jsonFieldMap;

    template <bool Flatten, bool Align>
    [[nodiscard]] bool EncodeBinaryBody(const std::vector<FieldContainer>& stInterMessage_, unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_) const
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

                    auto uiFieldCount = static_cast<uint32_t>(vCurrentFieldArrayField.size());
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, &uiFieldCount)) { return false; }

                    pucTempStart = *ppucOutBuf_; // Move the start placeholder to the front of the array start

                    for (const auto& clFieldArray : vCurrentFieldArrayField)
                    {
                        if (!EncodeBinaryBody<Flatten, Align>(std::get<std::vector<FieldContainer>>(clFieldArray.fieldValue), ppucOutBuf_,
                                                              uiBytesLeft_))
                        {
                            return false;
                        }
                    }

                    // For a flattened version of the log, fill in the remaining fields with 0x00.
                    if constexpr (Flatten)
                    {
                        if (static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart) <
                                dynamic_cast<const FieldArrayField*>(field.fieldDef.get())->fieldSize &&
                            !SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0',
                                         dynamic_cast<const FieldArrayField*>(field.fieldDef.get())->fieldSize -
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
                        const auto uiVarArraySize = static_cast<uint32_t>(vFcCurrentVectorField.size());
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
                        const uint32_t uiMaxArraySize =
                            dynamic_cast<const ArrayField*>(field.fieldDef.get())->arrayLength * field.fieldDef->dataType.length;
                        if (static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart) < uiMaxArraySize &&
                            !SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', uiMaxArraySize - static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart)))
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                switch (field.fieldDef->type)
                {
                case FIELD_TYPE::STRING: { // STRING types can be handled all at once because they are a single element and have a null terminator
                    std::string_view sv = std::get<std::string_view>(field.fieldValue);
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, sv)) { return false; }

                    // For a flattened version of the log, fill in the remaining characters with 0x00.
                    if constexpr (Flatten)
                    {
                        const auto uiStringLength = static_cast<uint32_t>(sv.size());
                        const uint32_t uiMaxArraySize =
                            dynamic_cast<const ArrayField*>(field.fieldDef.get())->arrayLength * field.fieldDef->dataType.length;
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
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, std::get<std::string_view>(field.fieldValue))) { return false; }
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

    [[nodiscard]] virtual bool FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
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

    template <bool Abbreviated>
    [[nodiscard]] bool EncodeAsciiBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                       const uint32_t uiIndentationLevel_ = 1) const
    {
        constexpr char separator = Abbreviated ? Derived::separatorAbbAscii : Derived::separatorAscii;

        [[maybe_unused]] bool newIndentLine = false;

        if constexpr (Abbreviated)
        {
            if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '<') ||
                !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', uiIndentationLevel_ * Derived::indentationLengthAbbAscii))
            {
                return false;
            }
        }

        for (const auto& field : vIntermediateFormat_)
        {
            if constexpr (Abbreviated)
            {
                if (newIndentLine)
                {
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<") ||
                        !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', uiIndentationLevel_ * Derived::indentationLengthAbbAscii))
                    {
                        return false;
                    }
                    newIndentLine = false;
                }
            }

            if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
            {
                const auto& vFcCurrentVectorField = std::get<std::vector<FieldContainer>>(field.fieldValue);

                // FIELD_ARRAY types contain several classes and so will use a recursive call
                if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", vFcCurrentVectorField.size()) ||
                        !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }

                    const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.fieldValue);

                    if constexpr (Abbreviated)
                    {
                        // Abbrev ascii will output a blank line for a 0 length array, nice
                        if (vCurrentFieldArrayField.empty())
                        {
                            if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<") ||
                                !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', (uiIndentationLevel_ + 1) * Derived::indentationLengthAbbAscii))
                            {
                                return false;
                            }
                        }
                        // Data was printed so a new line is required at the end of the array if there are more fields in the log
                        else
                        {
                            for (const auto& clFieldArray : vCurrentFieldArrayField)
                            {
                                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n") ||
                                    !EncodeAsciiBody<true>(std::get<std::vector<FieldContainer>>(clFieldArray.fieldValue), ppcOutBuf_, uiBytesLeft_,
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
                    const bool bPrintAsString = PrintAsString(*field.fieldDef);
                    const bool bIsCommaSeparated = IsCommaSeparated(*field.fieldDef);

                    // if the field is a variable array, print the size first
                    if ((field.fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY &&
                         (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", vFcCurrentVectorField.size()) ||
                          !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))) ||
                        (bPrintAsString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '\"')))
                    {
                        return false;
                    }

                    // This is an array of simple elements
                    for (const auto& arrayField : vFcCurrentVectorField)
                    {
                        // If we are printing a string, don't print the null terminator or any padding bytes
                        if (bPrintAsString &&
                            ((std::holds_alternative<int8_t>(arrayField.fieldValue) && std::get<int8_t>(arrayField.fieldValue) == '\0') ||
                             (std::holds_alternative<uint8_t>(arrayField.fieldValue) && std::get<uint8_t>(arrayField.fieldValue) == '\0')))
                        {
                            break;
                        }

                        if (!FieldToAscii(arrayField, ppcOutBuf_, uiBytesLeft_) ||
                            (bIsCommaSeparated && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)))
                        {
                            return false;
                        }
                    }
                    // Quoted elements need a trailing comma
                    if (bPrintAsString)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '\"') || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                    }
                    // Non-quoted, non-internally-separated elements also need a trailing comma
                    else if (!bIsCommaSeparated && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                }
            }
            else
            {
                switch (field.fieldDef->type)
                {
                case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "\"{}\"", std::get<std::string_view>(field.fieldValue)) ||
                        !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::ENUM: {
                    const auto* enumField = dynamic_cast<const EnumField*>(field.fieldDef.get());
                    if (enumField->length == 2)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, GetEnumString(enumField->enumDef, std::get<int16_t>(field.fieldValue))) ||
                            !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                        {
                            return false;
                        }
                    }
                    else if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, GetEnumString(enumField->enumDef, std::get<int32_t>(field.fieldValue))) ||
                             !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }
                    break;
                }
                case FIELD_TYPE::RESPONSE_ID: break; // Do nothing, ascii logs don't output this field
                case FIELD_TYPE::RESPONSE_STR:
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<std::string_view>(field.fieldValue)) ||
                        !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::SIMPLE:
                    if (!FieldToAscii(field, ppcOutBuf_, uiBytesLeft_) || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                    break;
                default: return false;
                }
            }
        }
        return true;
    }

    [[nodiscard]] bool FieldToAscii(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        auto it = asciiFieldMap.find(fc_.fieldDef->conversionHash);
        if (it != asciiFieldMap.end()) { return it->second(fc_, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb); }
        const char* pcConvertString = fc_.fieldDef->pythonConversion.c_str();

        switch (fc_.fieldDef->dataType.name)
        {
        case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<bool>(fc_.fieldValue) ? "TRUE" : "FALSE");
        case DATA_TYPE::HEXBYTE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{:02x}", std::get<uint8_t>(fc_.fieldValue));
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

    [[nodiscard]] bool FieldToJson(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        auto it = jsonFieldMap.find(fc_.fieldDef->conversionHash);
        if (it != jsonFieldMap.end()) { return it->second(fc_, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb); }
        const char* pcConvertString = fc_.fieldDef->pythonConversion.c_str();

        switch (fc_.fieldDef->dataType.name)
        {
        case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<bool>(fc_.fieldValue) ? "true" : "false");
        case DATA_TYPE::HEXBYTE: [[fallthrough]];
        case DATA_TYPE::UCHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<uint8_t>(fc_.fieldValue));
        case DATA_TYPE::CHAR: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<int8_t>(fc_.fieldValue));
        case DATA_TYPE::USHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<uint16_t>(fc_.fieldValue));
        case DATA_TYPE::SHORT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<int16_t>(fc_.fieldValue));
        case DATA_TYPE::UINT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<uint32_t>(fc_.fieldValue));
        case DATA_TYPE::INT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<int32_t>(fc_.fieldValue));
        case DATA_TYPE::ULONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<uint32_t>(fc_.fieldValue));
        case DATA_TYPE::LONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<int32_t>(fc_.fieldValue));
        case DATA_TYPE::ULONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<uint64_t>(fc_.fieldValue));
        case DATA_TYPE::LONGLONG: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, "{}", std::get<int64_t>(fc_.fieldValue));
        case DATA_TYPE::FLOAT: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<float>(fc_.fieldValue));
        case DATA_TYPE::DOUBLE: return PrintToBuffer(ppcOutBuf_, uiBytesLeft_, pcConvertString, std::get<double>(fc_.fieldValue));
        default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToJson(): unknown type."); throw std::runtime_error("FieldToJson(): unknown type.");
        }
    }

    [[nodiscard]] bool EncodeJsonBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '{')) { return false; }

        for (const auto& field : vIntermediateFormat_)
        {
            if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
            {
                const auto& vFcCurrentVectorField = std::get<std::vector<FieldContainer>>(field.fieldValue);

                // FIELD_ARRAY types contain several classes and so will use a recursive call
                if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
                {
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("{}": [)", field.fieldDef->name.c_str())) { return false; }
                    const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.fieldValue);
                    if (vCurrentFieldArrayField.empty())
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "],")) { return false; }
                    }
                    else
                    {
                        for (const auto& clFieldArray : vCurrentFieldArrayField)
                        {
                            if (!EncodeJsonBody(std::get<std::vector<FieldContainer>>(clFieldArray.fieldValue), ppcOutBuf_, uiBytesLeft_) ||
                                !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ','))
                            {
                                return false;
                            }
                        }
                        *(*ppcOutBuf_ - 1) = ']';
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                    }
                }
                else
                {
                    const bool bPrintAsString = PrintAsString(*field.fieldDef);

                    if (bPrintAsString)
                    {
                        if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("{}": ")", field.fieldDef->name.c_str())) { return false; }
                    }
                    // This is an array of simple elements
                    else if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("{}": [)", field.fieldDef->name.c_str()) ||
                             (vFcCurrentVectorField.empty() && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ']')))
                    {
                        return false;
                    }

                    for (const auto& arrayField : vFcCurrentVectorField)
                    {
                        // If we are printing a string, don't print the null terminator or any padding bytes
                        if (bPrintAsString &&
                            ((std::holds_alternative<int8_t>(arrayField.fieldValue) && std::get<int8_t>(arrayField.fieldValue) == '\0') ||
                             (std::holds_alternative<uint8_t>(arrayField.fieldValue) && std::get<uint8_t>(arrayField.fieldValue) == '\0')))
                        {
                            break;
                        }

                        if (!FieldToJson(arrayField, ppcOutBuf_, uiBytesLeft_) || (!bPrintAsString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')))
                        {
                            return false;
                        }
                    }

                    // Quoted elements need a trailing comma
                    if (bPrintAsString)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\",")) { return false; }
                    }
                    // Non-quoted, non-internally-separated elements also need a trailing comma
                    else
                    {
                        *(*ppcOutBuf_ - 1) = ']';
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                    }
                }
            }
            else
            {
                switch (field.fieldDef->type)
                {
                case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("{}": "{}",)", field.fieldDef->name.c_str(),
                                       std::get<std::string_view>(field.fieldValue)))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::ENUM:
                    if (!PrintToBuffer(
                            ppcOutBuf_, uiBytesLeft_, R"("{}": "{}",)", field.fieldDef->name.c_str(),
                            GetEnumString(dynamic_cast<const EnumField*>(field.fieldDef.get())->enumDef, std::get<int32_t>(field.fieldValue))))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::RESPONSE_ID:
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("{}": {},)", field.fieldDef->name.c_str(), std::get<int32_t>(field.fieldValue)))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::RESPONSE_STR:
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("{}": "{}",)", field.fieldDef->name.c_str(),
                                       std::get<std::string_view>(field.fieldValue)))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::SIMPLE:
                    if (!PrintToBuffer(ppcOutBuf_, uiBytesLeft_, R"("{}": )", field.fieldDef->name.c_str()) ||
                        !FieldToJson(field, ppcOutBuf_, uiBytesLeft_) || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ','))
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

    virtual void InitEnumDefinitions() {}

    virtual void InitFieldMaps()
    {
        asciiFieldMap.clear();
        jsonFieldMap.clear();

        // =========================================================
        // ASCII Field Mapping
        // =========================================================
        asciiFieldMap[CalculateBlockCrc32("UB")] = BasicMapEntry<uint8_t>("{}");
        asciiFieldMap[CalculateBlockCrc32("B")] = BasicMapEntry<int8_t>("{}");
        asciiFieldMap[CalculateBlockCrc32("XB")] = BasicMapEntry<uint8_t>("{:02x}");

        // =========================================================
        // Json Field Mapping
        // =========================================================
        asciiFieldMap[CalculateBlockCrc32("UB")] = BasicMapEntry<uint8_t>("{}");
        asciiFieldMap[CalculateBlockCrc32("B")] = BasicMapEntry<int8_t>("{}");
        asciiFieldMap[CalculateBlockCrc32("XB")] = BasicMapEntry<uint8_t>("{:02x}");
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Encoder class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    EncoderBase(MessageDatabase::Ptr pclMessageDb_ = nullptr)
    {
        InitFieldMaps();
        if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    }

    //----------------------------------------------------------------------------
    //! \brief A destructor for the Encoder class.
    //----------------------------------------------------------------------------
    virtual ~EncoderBase() = default;

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(MessageDatabase::Ptr pclMessageDb_)
    {
        pclMyMsgDb = pclMessageDb_;
        InitEnumDefinitions();
    }

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_  The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }
};

} // namespace novatel::edie

#endif // ENCODER_HPP
