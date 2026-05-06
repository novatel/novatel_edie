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
template <typename T> inline std::chars_format FloatingPointFormat(const BaseField& fd_, T val_)
{
    static_assert(std::is_floating_point_v<T>, "FloatingPointConversionString must be called with a floating point type");

    if (!fd_.width.has_value()) { return std::chars_format::scientific; }

    const auto width = fd_.width.value();
    const auto absVal = std::abs(val_);

    return absVal >= std::numeric_limits<T>::min() && (absVal > powLookup[width] || absVal < npowLookup[width]) ? std::chars_format::scientific
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

template <typename T, template <typename...> class Template> struct is_specialization_of : std::false_type
{
};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type
{
};

template <typename T, template <typename...> class Template>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

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
template <typename T> std::function<bool(const BaseField::ConstPtr&, const FieldValueVariant&, char**, uint32_t&, const MessageDatabase&)> BasicIntMapEntry()
{
    return [](const BaseField::ConstPtr&, const FieldValueVariant& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, const MessageDatabase&) {
        return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<T>(val_));
    };
}

// -------------------------------------------------------------------------------------------------------
template <typename T> std::function<bool(const BaseField::ConstPtr&, const FieldValueVariant&, char**, uint32_t&, const MessageDatabase&)> BasicHexMapEntry(uint32_t width)
{
    return [width](const BaseField::ConstPtr&, const FieldValueVariant& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, const MessageDatabase&) {
        return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<T>(val_), width);
    };
}

//============================================================================
//! \class EncoderBase
//! \brief Class to encode messages.
//============================================================================
template <typename Derived> class EncoderBase
{
  private:
    std::string sMyExpectedMessageFamily;

  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("encoder")};
    MessageDatabase::ConstPtr pclMyMsgDb{nullptr};

    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    // TODO: ASCII and JSON could probably share the same map.
    std::unordered_map<uint64_t, std::function<bool(const BaseField::ConstPtr&, const FieldValueVariant&, char**, uint32_t&, const MessageDatabase&)>> asciiFieldMap;
    std::unordered_map<uint64_t, std::function<bool(const BaseField::ConstPtr&, const FieldValueVariant&, char**, uint32_t&, const MessageDatabase&)>> jsonFieldMap;

    template <bool Flatten, bool Align>
    [[nodiscard]] bool EncodeBinaryBody(const MessageBody& stInterMessage_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                        unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_) const
    {
        // TODO: MessageDecoderBase uses virtual functions to align the buffer pointer, which
        // is probably a better approach because it allows each format to define its own
        // alignment rules. In any case, should use the same approach for both encoder and
        // decoder for consistency.
        const auto alignBufferPtr = [&ppucOutBuf_, &uiBytesLeft_](uint8_t alignment) {
            const auto uiAlign = std::min(static_cast<uint8_t>(4U), alignment);
            const auto ullRem = reinterpret_cast<uint64_t>(*ppucOutBuf_) % uiAlign;
            return (ullRem == 0) || SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, uiAlign - ullRem);
        };

        for (const auto& fieldDef : fieldDefinitions_)
        {
            if constexpr (Align)
            {
                if (!alignBufferPtr(static_cast<uint8_t>(fieldDef->dataType.length))) { return false; }
            }

            if (fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
            {
                const auto* arrayFieldDef = dynamic_cast<const FieldArrayField*>(fieldDef.get());
                const auto& elements = std::get<std::vector<MessageBody>>(stInterMessage_.varFields[fieldDef->index]);

                // Write array length if needed
                if (arrayFieldDef->arrayLengthRef.empty())
                {
                    if constexpr (Align) { if (!alignBufferPtr(static_cast<uint8_t>(arrayFieldDef->arrayLengthFieldSize))) return false; }
                    switch (arrayFieldDef->arrayLengthFieldSize)
                    {
                    case 1: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint8_t>(elements.size()))) return false; break;
                    case 2: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint16_t>(elements.size()))) return false; break;
                    case 4: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint32_t>(elements.size()))) return false; break;
                    default: return false;
                    }
                }

                const unsigned char* startPos = *ppucOutBuf_;

                // Recursively encode each element
                for (const auto& element : elements)
                {
                    if (!EncodeBinaryBody<Flatten, Align>(element, arrayFieldDef->fieldInfo.messageOrderedFields, ppucOutBuf_, uiBytesLeft_))
                        return false;
                }

                // Pad to max size if flattened
                if constexpr (Flatten)
                {
                    const auto written = static_cast<uint32_t>(*ppucOutBuf_ - startPos);
                    if (written < arrayFieldDef->fieldSize && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, arrayFieldDef->fieldSize - written))
                        return false;
                }
            }
            else if (fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
                const auto* arrayFieldDef = dynamic_cast<const ArrayField*>(fieldDef.get());

                // Write array length if needed
                if (arrayFieldDef->arrayLengthRef.empty())
                {
                    if constexpr (Align) { if (!alignBufferPtr(static_cast<uint8_t>(arrayFieldDef->arrayLengthFieldSize))) return false; }
                    const auto elemCount = stInterMessage_.GetFieldByteSize(*fieldDef) / fieldDef->dataType.length;
                    switch (arrayFieldDef->arrayLengthFieldSize)
                    {
                    case 1: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint8_t>(elemCount))) return false; break;
                    case 2: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint16_t>(elemCount))) return false; break;
                    case 4: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint32_t>(elemCount))) return false; break;
                    default: return false;
                    }
                }

                // Copy array data
                if (!stInterMessage_.CopyFieldToBuffer(*fieldDef, ppucOutBuf_, uiBytesLeft_)) return false;

                // Pad to max size if flattened
                if constexpr (Flatten)
                {
                    const auto maxSize = arrayFieldDef->arrayLength * fieldDef->dataType.length;
                    const auto written = stInterMessage_.GetFieldByteSize(*fieldDef);
                    if (written < maxSize && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, maxSize - written)) return false;
                }
            }
            else if (fieldDef->type == FIELD_TYPE::STRING)
            {
                // Copy string data
                if (!stInterMessage_.CopyFieldToBuffer(*fieldDef, ppucOutBuf_, uiBytesLeft_)) return false;

                // Padding
                if constexpr (Flatten)
                {
                    const auto* arrayFieldDef = dynamic_cast<const ArrayField*>(fieldDef.get());
                    const auto maxSize = arrayFieldDef->arrayLength * fieldDef->dataType.length;
                    const auto written = stInterMessage_.GetFieldByteSize(*fieldDef);
                    if (written < maxSize && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, maxSize - written)) return false;
                }
                else
                {
                    if (!SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, 4 - (reinterpret_cast<uint64_t>(*ppucOutBuf_) % 4))) return false;
                }
            }
            else
            {
                // All other fields (SIMPLE, ENUM, RESPONSE_ID, RESPONSE_STR, FIXED_LENGTH_ARRAY): copy bytes directly
                if (!stInterMessage_.CopyFieldToBuffer(*fieldDef, ppucOutBuf_, uiBytesLeft_)) return false;
            }
        }

        return true;
    }

    template <bool Abbreviated>
    [[nodiscard]] bool EncodeAsciiBody(const MessageBody& vIntermediateFormat_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                       char** ppcOutBuf_, uint32_t& uiBytesLeft_, const uint32_t uiIndents_ = 1) const
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

        for (const auto& fieldDef : fieldDefinitions_)
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

            if (fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
            {
                // FIELD_ARRAY types contain several classes and so will use a recursive call
                const auto& vFcCurrentVectorField = std::get<std::vector<MessageBody>>(vIntermediateFormat_.varFields[fieldDef->index]);
                const auto* arrayFieldDef = dynamic_cast<const FieldArrayField*>(fieldDef.get());

                if (!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, vFcCurrentVectorField.size()) ||
                    !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                {
                    return false;
                }

                if constexpr (Abbreviated)
                {
                    // Abbrev ascii will output a blank line for a 0 length array, nice
                    if (vFcCurrentVectorField.empty())
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<") ||
                            !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', (uiIndents_ + 1) * Derived::indentLengthAbbAscii))
                        {
                            return false;
                        }
                        newIndentLine = true;
                    }
                    // Data was printed so a new line is required at the end of the array if there are more fields in the log
                    else
                    {
                        for (const auto& clFieldArrayElement : vFcCurrentVectorField)
                        {
                            if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n") ||
                                !EncodeAsciiBody<true>(clFieldArrayElement, arrayFieldDef->fieldInfo.messageOrderedFields, ppcOutBuf_, uiBytesLeft_, uiIndents_ + 1))
                            {
                                return false;
                            }
                        }
                        newIndentLine = true;
                    }
                }
                else
                {
                    for (const auto& clFieldArrayElement : vFcCurrentVectorField)
                    {
                        if (!EncodeAsciiBody<false>(clFieldArrayElement, arrayFieldDef->fieldInfo.messageOrderedFields, ppcOutBuf_, uiBytesLeft_))
                        {
                            return false;
                        }
                    }
                }
            }
            else if (fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
                const bool encoded = std::visit(
                    [&](auto&& arrayElements) {
                        using ArrayType = std::decay_t<decltype(arrayElements)>;
                        if constexpr (!is_specialization_of_v<ArrayType, std::vector> && !std::is_same_v<ArrayType, std::string>)
                        {
                            return false;
                        }
                        else
                        {
                            using ValueType = typename ArrayType::value_type;
                            if constexpr (std::is_same_v<ValueType, MessageBody>)
                            {
                                return false;
                            }
                            else
                            {
                                // if the field is a variable array, print the size first
                                if ((!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, arrayElements.size()) ||
                                    !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) ||
                                    (fieldDef->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')))
                                {
                                    return false;
                                }

                                // This is an array of simple elements
                                for (const auto& arrayField : arrayElements)
                                {
                                    using ElemType = std::decay_t<decltype(arrayField)>;
                                    // If we are printing a string, don't print the null terminator or any padding bytes
                                    if (fieldDef->isString)
                                    {
                                        if constexpr (std::is_same_v<ElemType, int8_t> || std::is_same_v<ElemType, uint8_t>)
                                        {
                                            if (arrayField == '\0') { break; }
                                        }
                                    }

                                    if (!FieldToAscii(fieldDef, arrayField, ppcOutBuf_, uiBytesLeft_) ||
                                        (fieldDef->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)))
                                    {
                                        return false;
                                    }
                                }

                                // Quoted elements need a trailing comma
                                if (fieldDef->isString)
                                {
                                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"') || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                                }
                                // Non-quoted, non-internally-separated elements also need a trailing comma
                                else if (!fieldDef->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }

                                return true;
                            }
                        }
                    },
                    vIntermediateFormat_.varFields[fieldDef->index]
                );
                if (!encoded) { return false; }
            }
            else if (fieldDef->type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
            {
                const bool encoded = std::visit(
                    [&](auto&& arrayElements) -> bool {
                        using ArrayType = std::decay_t<decltype(arrayElements)>;

                        if constexpr (!is_specialization_of_v<ArrayType, std::vector> && !std::is_same_v<ArrayType, std::string>)
                        {
                            return false;
                        }
                        else
                        {
                            using ValueType = typename ArrayType::value_type;
                            if (fieldDef->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"'))
                            {
                                return false;
                            }

                            if constexpr (std::is_same_v<ValueType, MessageBody>)
                            {
                                return false;
                            }
                            else
                            {
                                for (const auto& arrayField : arrayElements)
                                {
                                    using ElemType = std::decay_t<decltype(arrayField)>;
                                    if (fieldDef->isString)
                                    {
                                        if constexpr (std::is_same_v<ElemType, int8_t> || std::is_same_v<ElemType, uint8_t>)
                                        {
                                            if (arrayField == '\0') { break; }
                                        }
                                    }

                                    if (!FieldToAscii(fieldDef, arrayField, ppcOutBuf_, uiBytesLeft_) ||
                                        (fieldDef->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)))
                                    {
                                        return false;
                                    }
                                }

                                if (fieldDef->isString)
                                {
                                    return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"') &&
                                           CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator);
                                }

                                if (!fieldDef->isCsv) { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator); }
                                return true;
                            }
                        }
                    },
                    vIntermediateFormat_.GetFieldValue(*fieldDef));

                if (!encoded) { return false; }
            }
            else
            {
                switch (fieldDef->type)
                {
                case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::get<std::string>(vIntermediateFormat_.varFields[fieldDef->index]), '"') ||
                        !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::ENUM: {
                    const auto* enumField = dynamic_cast<const EnumField*>(fieldDef.get());
                    if (enumField->length == 2)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, GetEnumString(enumField->enumDef, vIntermediateFormat_.GetFixedScalarValue<int16_t>(fieldDef->index))) ||
                            !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                        {
                            return false;
                        }
                    }
                    else if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, GetEnumString(enumField->enumDef, vIntermediateFormat_.GetFixedScalarValue<int32_t>(fieldDef->index))) ||
                             !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }
                    break;
                }
                case FIELD_TYPE::RESPONSE_ID: break; // Do nothing, ascii logs don't output this field
                case FIELD_TYPE::RESPONSE_STR:
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<std::string>(vIntermediateFormat_.varFields[fieldDef->index])) ||
                        !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }
                    break;
                case FIELD_TYPE::SIMPLE: {
                    bool encoded = false;

                    switch (fieldDef->dataType.name)
                    {
                        case DATA_TYPE::BOOL: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<bool>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                    case DATA_TYPE::HEXBYTE:
                        case DATA_TYPE::UCHAR: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<uint8_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                        case DATA_TYPE::CHAR: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<int8_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                        case DATA_TYPE::USHORT: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<uint16_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                        case DATA_TYPE::SHORT: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<int16_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                    case DATA_TYPE::UINT:
                    case DATA_TYPE::ULONG:
                        case DATA_TYPE::SATELLITEID: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<uint32_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                    case DATA_TYPE::INT:
                        case DATA_TYPE::LONG: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<int32_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                        case DATA_TYPE::ULONGLONG: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<uint64_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                        case DATA_TYPE::LONGLONG: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<int64_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                        case DATA_TYPE::FLOAT: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<float>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                        case DATA_TYPE::DOUBLE: encoded = FieldToAscii(fieldDef, vIntermediateFormat_.GetFixedScalarValue<double>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                    default: encoded = false; break;
                    }

                    if (!encoded || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                    break;
                }
                default: return false;
                }
            }
        }
        return true;
    }

    template <typename FieldType>
    [[nodiscard]] bool FieldToAscii(const BaseField::ConstPtr& fd_, const FieldType& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if constexpr (std::is_same_v<std::decay_t<FieldType>, MessageBody>) { return false; }
        else
        {
            if (fd_->conversionHash != 0)
            {
                auto it = asciiFieldMap.find(fd_->conversionHash);
                if (it != asciiFieldMap.end())
                {
                    const FieldValueVariant v{val_};
                    return it->second(fd_, v, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb);
                }
            }

            switch (fd_->dataType.name)
            {
            case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::string_view(static_cast<bool>(val_) ? "TRUE" : "FALSE"));
            case DATA_TYPE::HEXBYTE: return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint8_t>(val_), 2);
            case DATA_TYPE::UCHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint8_t>(val_));
            case DATA_TYPE::CHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int8_t>(val_));
            case DATA_TYPE::USHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint16_t>(val_));
            case DATA_TYPE::SHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int16_t>(val_));
            case DATA_TYPE::UINT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint32_t>(val_));
            case DATA_TYPE::INT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int32_t>(val_));
            case DATA_TYPE::ULONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint32_t>(val_));
            case DATA_TYPE::ULONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint64_t>(val_));
            case DATA_TYPE::LONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int32_t>(val_));
            case DATA_TYPE::LONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int64_t>(val_));
            case DATA_TYPE::FLOAT:
                return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<float>(val_), FloatingPointFormat(*fd_, static_cast<float>(val_)),
                                          fd_->precision);
            case DATA_TYPE::DOUBLE:
                return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<double>(val_), FloatingPointFormat(*fd_, static_cast<double>(val_)),
                                          fd_->precision);
            default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToAscii(): unknown type."); throw std::runtime_error("FieldToAscii(): unknown type.");
            }
        }
    }

    template <typename FieldType>
    [[nodiscard]] bool FieldToJson(const BaseField::ConstPtr& fd_, const FieldType& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if constexpr (std::is_same_v<std::decay_t<FieldType>, MessageBody>) { return false; }
        else
        {
            if (fd_->conversionHash != 0)
            {
                auto it = jsonFieldMap.find(fd_->conversionHash);
                if (it != jsonFieldMap.end())
                {
                    const FieldValueVariant v{val_};
                    return it->second(fd_, v, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb);
                }
            }

            switch (fd_->dataType.name)
            {
            case DATA_TYPE::BOOL: return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::string_view(static_cast<bool>(val_) ? "true" : "false"));
            case DATA_TYPE::HEXBYTE: [[fallthrough]];
            case DATA_TYPE::UCHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint8_t>(val_));
            case DATA_TYPE::CHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int8_t>(val_));
            case DATA_TYPE::USHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint16_t>(val_));
            case DATA_TYPE::SHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int16_t>(val_));
            case DATA_TYPE::UINT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint32_t>(val_));
            case DATA_TYPE::INT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int32_t>(val_));
            case DATA_TYPE::ULONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint32_t>(val_));
            case DATA_TYPE::LONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int32_t>(val_));
            case DATA_TYPE::ULONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<uint64_t>(val_));
            case DATA_TYPE::LONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<int64_t>(val_));
            case DATA_TYPE::FLOAT: return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<float>(val_), std::chars_format::fixed, fd_->precision);
            case DATA_TYPE::DOUBLE: return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, static_cast<double>(val_), std::chars_format::fixed, fd_->precision);
            default: SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToJson(): unknown type."); throw std::runtime_error("FieldToJson(): unknown type.");
            }
        }
    }

    [[nodiscard]] bool EncodeJsonBody(const MessageBody& stInterMessage_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                      char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '{')) { return false; }

        for (const auto& fieldDef : fieldDefinitions_)
        {
            if (fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
            {
                const auto* arrayFieldDef = dynamic_cast<const FieldArrayField*>(fieldDef.get());
                const auto& elements = std::get<std::vector<MessageBody>>(stInterMessage_.varFields[fieldDef->index]);

                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": [)")) { return false; }
                if (elements.empty())
                {
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "],")) { return false; }
                }
                else
                {
                    for (const auto& element : elements)
                    {
                        if (!EncodeJsonBody(element, arrayFieldDef->fieldInfo.messageOrderedFields, ppcOutBuf_, uiBytesLeft_) ||
                            !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ','))
                            return false;
                    }
                    *(*ppcOutBuf_ - 1) = ']';
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                }
            }
            else if (fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY || fieldDef->type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
            {
                const bool encoded = std::visit(
                    [&](auto&& arrayElements) -> bool {
                        using ArrayType = std::decay_t<decltype(arrayElements)>;
                        if constexpr (std::is_same_v<ArrayType, std::string>)
                        {
                            // For strings, output as quoted string not array
                            return true;  // Special handling below
                        }
                        else if constexpr (std::is_same_v<ArrayType, std::vector<MessageBody>>)
                        {
                            return true;  // Should not reach here
                        }
                        else if constexpr (std::is_arithmetic_v<ArrayType>)
                        {
                            throw std::runtime_error("EncodeJsonBody(): scalar values are not valid array payloads");
                        }
                        else
                        {
                            if (fieldDef->isString)
                            {
                                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": ")")) { return false; }
                            }
                            // This is an array of simple elements
                            else if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": [)") ||
                                        (arrayElements.empty() && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ']')))
                            {
                                return false;
                            }

                            // Numeric array
                            for (const auto& elem : arrayElements)
                            {
                                using ValueType = std::decay_t<decltype(elem)>;
                                // Skip null terminators for string data
                                if (fieldDef->isString && ((std::is_same_v<ValueType, int8_t> && elem == '\0') ||
                                                           (std::is_same_v<ValueType, uint8_t> && elem == '\0')))
                                    break;

                                if (!FieldToJson(fieldDef, elem, ppcOutBuf_, uiBytesLeft_) ||
                                    (!fieldDef->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')))
                                {
                                    return false;
                                }
                            }
                            return true;
                        }
                    },
                    (fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY) ? stInterMessage_.varFields[fieldDef->index] : stInterMessage_.GetFieldValue(*fieldDef));

                if (!encoded) { return false; }

                // Quoted elements need a trailing comma
                if (fieldDef->isString)
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
            else if (fieldDef->type == FIELD_TYPE::STRING)
            {
                const auto& str = std::get<std::string>(stInterMessage_.varFields[fieldDef->index]);
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": ")",
                                     str, "\","))
                    return false;
            }
            else if (fieldDef->type == FIELD_TYPE::ENUM)
            {
                const auto* enumField = dynamic_cast<const EnumField*>(fieldDef.get());
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": ")",
                             GetEnumString(enumField->enumDef, stInterMessage_.GetFixedScalarValue<int32_t>(fieldDef->index)), "\","))
                    return false;
            }
            else if (fieldDef->type == FIELD_TYPE::RESPONSE_ID)
            {
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": )",
                             stInterMessage_.GetFixedScalarValue<int32_t>(fieldDef->index), ','))
                    return false;
            }
            else if (fieldDef->type == FIELD_TYPE::RESPONSE_STR)
            {
                const auto& str = std::get<std::string>(stInterMessage_.varFields[fieldDef->index]);
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": ")", str, "\","))
                    return false;
            }
            else // SIMPLE fields
            {
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": )")) { return false; }

                bool encoded = false;
                switch (fieldDef->dataType.name)
                {
                case DATA_TYPE::BOOL: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<bool>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::HEXBYTE:
                case DATA_TYPE::UCHAR: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<uint8_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::CHAR: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<int8_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::USHORT: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<uint16_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::SHORT: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<int16_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::UINT:
                case DATA_TYPE::ULONG:
                case DATA_TYPE::SATELLITEID: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<uint32_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::INT:
                case DATA_TYPE::LONG: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<int32_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::ULONGLONG: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<uint64_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::LONGLONG: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<int64_t>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::FLOAT: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<float>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                case DATA_TYPE::DOUBLE: encoded = FieldToJson(fieldDef, stInterMessage_.GetFixedScalarValue<double>(fieldDef->index), ppcOutBuf_, uiBytesLeft_); break;
                default: encoded = false; break;
                }

                if (!encoded || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
            }
        }

        *(*ppcOutBuf_ - 1) = '}';
        return true;
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the EncoderBase class.
    //
    //! \param[in] expectedMessageFamily_ The expected message family for the encoder.
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    EncoderBase(std::string expectedMessageFamily_, MessageDatabase::ConstPtr pclMessageDb_ = nullptr)
        : sMyExpectedMessageFamily(std::move(expectedMessageFamily_))
    {
        static_cast<Derived*>(this)->InitFieldMaps();
        if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    }

    //----------------------------------------------------------------------------
    //! \brief A destructor for the EncoderBase class.
    //----------------------------------------------------------------------------
    ~EncoderBase() = default;

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(MessageDatabase::ConstPtr pclMessageDb_)
    {
        ValidateMessageDatabaseFamily(pclMessageDb_, sMyExpectedMessageFamily, pclMyLogger);
        pclMyMsgDb = pclMessageDb_;
        static_cast<Derived*>(this)->InitEnumDefinitions();
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
