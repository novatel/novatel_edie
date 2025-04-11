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

#include <array>
#include <charconv>
#include <cstdarg>
#include <optional>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

constexpr auto powLookup = [] {
    std::array<double, 16> arr{};
    arr[0] = 1.0;
    for (size_t i = 1; i < arr.size(); ++i) { arr[i] = arr[i - 1] * 10.0; }
    return arr;
}();

constexpr auto npowLookup = [] {
    std::array<double, 16> arr{};
    arr[0] = 1.0;
    for (size_t i = 1; i < arr.size(); ++i) { arr[i] = arr[i - 1] / 10.0; }
    return arr;
}();

// -------------------------------------------------------------------------------------------------------
template <typename T> inline std::chars_format FloatingPointFormat(const FieldContainer& fc_)
{
    static_assert(std::is_floating_point_v<T>, "FloatingPointConversionString must be called with a floating point type");

    if (!fc_.fieldDef->width.has_value()) { return std::chars_format::fixed; }

    const auto width = fc_.fieldDef->width.value();
    const auto absVal = std::abs(std::get<T>(fc_.fieldValue));

    return absVal >= std::numeric_limits<T>::epsilon() && (absVal > powLookup[width] || absVal < npowLookup[width]) ? std::chars_format::scientific
                                                                                                                    : std::chars_format::fixed;
}

// -------------------------------------------------------------------------------------------------------
template <typename T> [[nodiscard]] bool WriteIntToBuffer(char** ppcBuffer_, uint32_t& uiBytesLeft_, T&& arg)
{
    static_assert(std::is_integral_v<std::decay_t<T>>, "Argument must be integral type.");
    auto [end, ec] = std::to_chars(*ppcBuffer_, *ppcBuffer_ + uiBytesLeft_, arg);
    if (ec != std::errc{}) { return false; }
    const auto written = static_cast<uint32_t>(end - *ppcBuffer_);
    *ppcBuffer_ = end;
    uiBytesLeft_ -= written;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename T> [[nodiscard]] bool WriteHexToBuffer(char** ppcBuffer_, uint32_t& uiBytesLeft_, T&& arg, uint32_t width)
{
    static_assert(std::is_integral_v<std::decay_t<T>>, "Argument must be integral type.");

    constexpr uint32_t maxDigits = sizeof(T) * 2;
    const uint32_t requiredSpace = std::max(width, maxDigits);

    if (uiBytesLeft_ < requiredSpace) { return false; }

    auto [end, ec] = std::to_chars(*ppcBuffer_, *ppcBuffer_ + uiBytesLeft_, arg, 16);

    if (ec != std::errc{}) { return false; }

    const auto num_digits = static_cast<uint32_t>(end - *ppcBuffer_);

    if (num_digits < width)
    {
        const uint32_t pad_chars = width - num_digits;
        std::memmove(*ppcBuffer_ + pad_chars, *ppcBuffer_, num_digits);
        std::fill_n(*ppcBuffer_, pad_chars, '0');
        end += pad_chars;
    }

    uiBytesLeft_ -= static_cast<uint32_t>(end - *ppcBuffer_);
    *ppcBuffer_ = end;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] bool WriteFloatToBuffer(char** ppcBuffer_, uint32_t& uiBytesLeft_, T value, std::chars_format format, std::optional<int> precision)
{
    static_assert(std::is_floating_point_v<T>, "WriteFloatToBuffer requires float/double.");

    int precision_arg = -1;
    if (precision.has_value())
    {
        precision_arg = *precision;
        if (precision_arg < 0) { precision_arg = 0; }
    }
    else if (format == std::chars_format::fixed || format == std::chars_format::scientific) { precision_arg = 6; }

    auto [end, ec] = std::to_chars(*ppcBuffer_, *ppcBuffer_ + uiBytesLeft_, value, format, precision_arg);

    if (ec != std::errc{}) { return false; }

    uiBytesLeft_ -= static_cast<uint32_t>(end - *ppcBuffer_);
    *ppcBuffer_ = end;
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
template <typename BufferType, typename T> [[nodiscard]] bool CopyToBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, const T& item)
{
    static_assert(!std::is_pointer_v<T>, "Pointers not allowed.");
    if (uiBytesLeft_ < sizeof(T)) { return false; }
    std::memcpy(*ppucBuffer_, &item, sizeof(T));
    *ppucBuffer_ += sizeof(T);
    uiBytesLeft_ -= sizeof(T);
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType> [[nodiscard]] bool CopyToBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, const char* ptItem_)
{
    auto uiItemSize = static_cast<uint32_t>(std::strlen(ptItem_));
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
template <typename BufferType> [[nodiscard]] inline bool CopyToBuffer(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, const std::string& str)
{
    if (uiBytesLeft_ < str.size()) { return false; }
    std::memcpy(*ppucBuffer_, str.data(), str.size());
    *ppucBuffer_ += str.size();
    uiBytesLeft_ -= static_cast<uint32_t>(str.size());
    return true;
}

template <typename T> struct FloatValue
{
    T value;
    std::chars_format format;
    std::optional<int> precision;
};

template <typename T> struct HexValue
{
    T value;
    uint32_t width;
};

template <typename T, template <typename> class Template> struct is_specialization_of : std::false_type
{
};

template <typename T, template <typename> class Template> struct is_specialization_of<Template<T>, Template> : std::true_type
{
};

template <typename T, template <typename> class Template> inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

// -------------------------------------------------------------------------------------------------------
template <typename... Args> [[nodiscard]] bool CopyAllToBuffer(char** ppucBuffer_, uint32_t& uiBytesLeft_, Args&&... args_)
{
    return ([&](auto&& arg) {
        using Decayed = std::decay_t<decltype(arg)>;
        if constexpr (std::is_integral_v<Decayed> && !std::is_same_v<Decayed, char>) { return WriteIntToBuffer(ppucBuffer_, uiBytesLeft_, arg); }
        if constexpr (is_specialization_of_v<Decayed, FloatValue>)
        {
            return WriteFloatToBuffer(ppucBuffer_, uiBytesLeft_, arg.value, arg.format, arg.precision);
        }
        if constexpr (is_specialization_of_v<Decayed, HexValue>) { return WriteHexToBuffer(ppucBuffer_, uiBytesLeft_, arg.value, arg.width); }
        return CopyToBuffer(ppucBuffer_, uiBytesLeft_, arg);
    }(args_) &&
            ...);
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename... Args>
[[nodiscard]] bool CopyAllToBufferSeparated(BufferType* ppucBuffer_, uint32_t& uiBytesLeft_, const char cSeparator_, Args&&... args_)
{
    bool success = true;
    size_t i = 0;
    (
        [&](auto&& arg) {
            if (!success) { return; }
            success &= CopyAllToBuffer(ppucBuffer_, uiBytesLeft_, arg);
            // Copy the separator to the buffer for every item except the last.
            if (success && (i++ < sizeof...(Args) - 1)) { success &= CopyToBuffer(ppucBuffer_, uiBytesLeft_, cSeparator_); }
        }(std::forward<Args>(args_)),
        ...);

    return success;
}

// -------------------------------------------------------------------------------------------------------
template <typename T> std::function<bool(const FieldContainer&, char**, uint32_t&, const MessageDatabase&)> BasicMapEntry(const char* pcF_)
{
    return [pcF_](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return WriteFormattedToBuffer(ppcOutBuf_, uiBytesLeft_, pcF_, std::get<T>(fc_.fieldValue));
    };
}

// -------------------------------------------------------------------------------------------------------
template <typename T> std::function<bool(const FieldContainer&, char**, uint32_t&, const MessageDatabase&)> BasicIntMapEntry()
{
    return [](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<T>(fc_.fieldValue));
    };
}

// -------------------------------------------------------------------------------------------------------
template <typename T> std::function<bool(const FieldContainer&, char**, uint32_t&, const MessageDatabase&)> BasicHexMapEntry(uint32_t width)
{
    return [width](const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, [[maybe_unused]] const MessageDatabase& pclMsgDb_) {
        return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<T>(fc_.fieldValue), width);
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
    MessageDatabase::ConstPtr pclMyMsgDbStrongRef{nullptr};
    const MessageDatabase* pclMyMsgDb{nullptr};

    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    // TODO: ASCII and JSON could probably share the same map.
    std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] const MessageDatabase&)>>
        asciiFieldMap;
    std::unordered_map<uint64_t, std::function<bool(const FieldContainer&, char**, uint32_t&, [[maybe_unused]] const MessageDatabase&)>> jsonFieldMap;

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

                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint32_t>(vCurrentFieldArrayField.size()))) { return false; }

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
                        const uint32_t maxSize = dynamic_cast<const FieldArrayField*>(field.fieldDef.get())->fieldSize;
                        const auto diff = static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart);
                        if (diff < maxSize && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', maxSize - diff)) { return false; }
                    }
                }
                else
                {
                    // if the field is a variable array, print the size first
                    if (field.fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY &&
                        !CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint32_t>(vFcCurrentVectorField.size())))
                    {
                        return false;
                    }

                    pucTempStart = *ppucOutBuf_; // Move the start placeholder to the front of the array start

                    // This is an array of simple elements
                    for (const auto& arrayField : vFcCurrentVectorField)
                    {
                        if (!Derived::FieldToBinary(arrayField, ppucOutBuf_, uiBytesLeft_)) { return false; }
                    }

                    // For a flattened version of the log, fill in the remaining fields with 0x00.
                    if constexpr (Flatten)
                    {
                        const uint32_t maxSize = dynamic_cast<const ArrayField*>(field.fieldDef.get())->arrayLength * field.fieldDef->dataType.length;
                        const auto diff = static_cast<uint32_t>(*ppucOutBuf_ - pucTempStart);
                        if (diff < maxSize && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', maxSize - diff)) { return false; }
                    }
                }
            }
            else
            {
                switch (field.fieldDef->type)
                {
                case FIELD_TYPE::STRING: { // STRING types can be handled all at once because they are a single element and have a null terminator
                    auto sv = std::get<std::string>(field.fieldValue);
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, sv)) { return false; }

                    // For a flattened version of the log, fill in the remaining characters with 0x00.
                    if constexpr (Flatten)
                    {
                        const auto diff = static_cast<uint32_t>(sv.size());
                        const uint32_t maxSize = dynamic_cast<const ArrayField*>(field.fieldDef.get())->arrayLength * field.fieldDef->dataType.length;
                        if (diff < maxSize && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', maxSize - diff)) { return false; }
                    }
                    else if (!SetInBuffer(ppucOutBuf_, uiBytesLeft_, '\0', 4 - (reinterpret_cast<uint64_t>(*ppucOutBuf_) % 4))) { return false; }
                    break;
                }
                case FIELD_TYPE::ENUM:
                    switch (field.fieldDef->dataType.length)
                    {
                    case 2:
                        if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, std::get<int16_t>(field.fieldValue))) { return false; }
                        break;
                    case 4:
                        if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, std::get<int32_t>(field.fieldValue))) { return false; }
                        break;
                    default: return false;
                    }
                    break;
                case FIELD_TYPE::RESPONSE_ID:
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, std::get<int32_t>(field.fieldValue))) { return false; }
                    break;
                case FIELD_TYPE::RESPONSE_STR:
                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, std::get<std::string>(field.fieldValue))) { return false; }
                    break;
                case FIELD_TYPE::SIMPLE:
                    if (!Derived::FieldToBinary(field, ppucOutBuf_, uiBytesLeft_)) { return false; }
                    break;
                default: return false;
                }
            }
        }
        return true;
    }

    template <bool Abbreviated>
    [[nodiscard]] bool EncodeAsciiBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                       const uint32_t uiIndents_ = 1) const
    {
        constexpr char separator = Abbreviated ? Derived::separatorAbbAscii : Derived::separatorAscii;

        [[maybe_unused]] bool newIndentLine = false;

        if constexpr (Abbreviated)
        {
            if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '<') ||
                !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', uiIndents_ * Derived::indentLengthAbbAscii))
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
                        !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', uiIndents_ * Derived::indentLengthAbbAscii))
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
                    if (!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, vFcCurrentVectorField.size()) ||
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
                                !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', (uiIndents_ + 1) * Derived::indentLengthAbbAscii))
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
                                                           uiIndents_ + 1))
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
                    // if the field is a variable array, print the size first
                    if ((field.fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY &&
                         (!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, vFcCurrentVectorField.size()) ||
                          !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))) ||
                        (field.fieldDef->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')))
                    {
                        return false;
                    }

                    // This is an array of simple elements
                    for (const auto& arrayField : vFcCurrentVectorField)
                    {
                        // If we are printing a string, don't print the null terminator or any padding bytes
                        if (field.fieldDef->isString &&
                            ((std::holds_alternative<int8_t>(arrayField.fieldValue) && std::get<int8_t>(arrayField.fieldValue) == '\0') ||
                             (std::holds_alternative<uint8_t>(arrayField.fieldValue) && std::get<uint8_t>(arrayField.fieldValue) == '\0')))
                        {
                            break;
                        }

                        if (!FieldToAscii(arrayField, ppcOutBuf_, uiBytesLeft_) ||
                            (field.fieldDef->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)))
                        {
                            return false;
                        }
                    }
                    // Quoted elements need a trailing comma
                    if (field.fieldDef->isString)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"') || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                    }
                    // Non-quoted, non-internally-separated elements also need a trailing comma
                    else if (!field.fieldDef->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                }
            }
            else
            {
                switch (field.fieldDef->type)
                {
                case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::get<std::string>(field.fieldValue), '"') ||
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
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<std::string>(field.fieldValue)) ||
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

        switch (fc_.fieldDef->dataType.name)
        {
        case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::string_view(std::get<bool>(fc_.fieldValue) ? "TRUE" : "FALSE"));
        case DATA_TYPE::HEXBYTE: return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint8_t>(fc_.fieldValue), 2);
        case DATA_TYPE::UCHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint8_t>(fc_.fieldValue));
        case DATA_TYPE::CHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int8_t>(fc_.fieldValue));
        case DATA_TYPE::USHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint16_t>(fc_.fieldValue));
        case DATA_TYPE::SHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int16_t>(fc_.fieldValue));
        case DATA_TYPE::UINT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint32_t>(fc_.fieldValue));
        case DATA_TYPE::INT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int32_t>(fc_.fieldValue));
        case DATA_TYPE::ULONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint32_t>(fc_.fieldValue));
        case DATA_TYPE::ULONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint64_t>(fc_.fieldValue));
        case DATA_TYPE::LONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int32_t>(fc_.fieldValue));
        case DATA_TYPE::LONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int64_t>(fc_.fieldValue));
        case DATA_TYPE::FLOAT:
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<float>(fc_.fieldValue), FloatingPointFormat<float>(fc_),
                                      fc_.fieldDef->precision);
        case DATA_TYPE::DOUBLE:
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<double>(fc_.fieldValue), FloatingPointFormat<double>(fc_),
                                      fc_.fieldDef->precision);
        default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToAscii(): unknown type."); throw std::runtime_error("FieldToAscii(): unknown type.");
        }
    }

    // TODO: Delete this and use FieldToAscii
    [[nodiscard]] bool FieldToJson(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        auto it = jsonFieldMap.find(fc_.fieldDef->conversionHash);
        if (it != jsonFieldMap.end()) { return it->second(fc_, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb); }

        switch (fc_.fieldDef->dataType.name)
        {
        case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::string_view(std::get<bool>(fc_.fieldValue) ? "true" : "false"));
        case DATA_TYPE::HEXBYTE: [[fallthrough]];
        case DATA_TYPE::UCHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint8_t>(fc_.fieldValue));
        case DATA_TYPE::CHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int8_t>(fc_.fieldValue));
        case DATA_TYPE::USHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint16_t>(fc_.fieldValue));
        case DATA_TYPE::SHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int16_t>(fc_.fieldValue));
        case DATA_TYPE::UINT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint32_t>(fc_.fieldValue));
        case DATA_TYPE::INT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int32_t>(fc_.fieldValue));
        case DATA_TYPE::ULONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint32_t>(fc_.fieldValue));
        case DATA_TYPE::LONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int32_t>(fc_.fieldValue));
        case DATA_TYPE::ULONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<uint64_t>(fc_.fieldValue));
        case DATA_TYPE::LONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<int64_t>(fc_.fieldValue));
        case DATA_TYPE::FLOAT:
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<float>(fc_.fieldValue), FloatingPointFormat<float>(fc_),
                                      fc_.fieldDef->precision);
        case DATA_TYPE::DOUBLE:
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<double>(fc_.fieldValue), FloatingPointFormat<double>(fc_),
                                      fc_.fieldDef->precision);
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
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name), R"(": [)")) { return false; }
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
                    if (field.fieldDef->isString)
                    {
                        if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name), R"(": ")")) { return false; }
                    }
                    // This is an array of simple elements
                    else if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name), R"(": [)") ||
                             (vFcCurrentVectorField.empty() && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ']')))
                    {
                        return false;
                    }

                    for (const auto& arrayField : vFcCurrentVectorField)
                    {
                        // If we are printing a string, don't print the null terminator or any padding bytes
                        if (field.fieldDef->isString &&
                            ((std::holds_alternative<int8_t>(arrayField.fieldValue) && std::get<int8_t>(arrayField.fieldValue) == '\0') ||
                             (std::holds_alternative<uint8_t>(arrayField.fieldValue) && std::get<uint8_t>(arrayField.fieldValue) == '\0')))
                        {
                            break;
                        }

                        if (!FieldToJson(arrayField, ppcOutBuf_, uiBytesLeft_) ||
                            (!field.fieldDef->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')))
                        {
                            return false;
                        }
                    }

                    // Quoted elements need a trailing comma
                    if (field.fieldDef->isString)
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
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name), "\": \"",
                                         std::get<std::string>(field.fieldValue), "\","))
                    {
                        return false;
                    }
                    break;

                case FIELD_TYPE::ENUM:
                    if (!CopyAllToBuffer(
                            ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name), "\": \"",
                            GetEnumString(dynamic_cast<const EnumField*>(field.fieldDef.get())->enumDef, std::get<int32_t>(field.fieldValue)), "\","))
                    {
                        return false;
                    }
                    break;

                case FIELD_TYPE::RESPONSE_ID:
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name),
                                         "\": ", std::get<int32_t>(field.fieldValue), ','))
                    {
                        return false;
                    }
                    break;

                case FIELD_TYPE::RESPONSE_STR:
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name), "\": \"",
                                         std::get<std::string>(field.fieldValue), "\","))
                    {
                        return false;
                    }
                    break;

                case FIELD_TYPE::SIMPLE:
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(field.fieldDef->name), "\": ") ||
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

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Encoder class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    EncoderBase(const MessageDatabase* pclMessageDb_ = nullptr)
    {
        static_cast<Derived*>(this)->InitFieldMaps();
        if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    }

    EncoderBase(MessageDatabase::ConstPtr pclMessageDb_)
    {
        static_cast<Derived*>(this)->InitFieldMaps();
        if (pclMessageDb_ != nullptr) { LoadSharedJsonDb(pclMessageDb_); }
    }

    //----------------------------------------------------------------------------
    //! \brief A destructor for the Encoder class.
    //----------------------------------------------------------------------------
    ~EncoderBase() = default;

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(const MessageDatabase* pclMessageDb_)
    {
        pclMyMsgDb = pclMessageDb_;
        static_cast<Derived*>(this)->InitEnumDefinitions();
    }

    void LoadSharedJsonDb(MessageDatabase::ConstPtr pclMessageDb_)
    {
        pclMyMsgDbStrongRef = pclMessageDb_;
        LoadJsonDb(pclMessageDb_.get());
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
