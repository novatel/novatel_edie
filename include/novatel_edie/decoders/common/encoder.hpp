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

template <typename T, template <typename> class Template> struct is_specialization_of : std::false_type
{
};
template <typename T, template <typename> class Template> struct is_specialization_of<Template<T>, Template> : std::true_type
{
};
template <typename T, template <typename> class Template> inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

template <typename T> struct is_byte_buffer : std::false_type
{
};
template <> struct is_byte_buffer<char*> : std::true_type
{
};
template <> struct is_byte_buffer<unsigned char*> : std::true_type
{
};
template <> struct is_byte_buffer<std::byte*> : std::true_type
{
};
template <typename T> inline constexpr bool is_byte_buffer_v = is_byte_buffer<T>::value;

template <typename BufferType> constexpr void AssertWritableByteBuffer()
{
    static_assert(is_byte_buffer_v<std::decay_t<BufferType>>, "BufferType must be char*, unsigned char*, or std::byte*.");
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
template <typename BufferType, typename T> [[nodiscard]] bool WriteIntToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, T&& arg)
{
    AssertWritableByteBuffer<BufferType>();
    static_assert(std::is_integral_v<std::decay_t<T>>, "Argument must be integral type.");
    auto [end, ec] = std::to_chars(*buffer_, *buffer_ + uiBytesLeft_, arg);
    if (ec != std::errc{}) { return false; }
    const auto written = static_cast<uint32_t>(end - *buffer_);
    *buffer_ = end;
    uiBytesLeft_ -= written;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename T> [[nodiscard]] bool WriteHexToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, T&& arg, uint32_t width)
{
    AssertWritableByteBuffer<BufferType>();
    static_assert(std::is_integral_v<std::decay_t<T>>, "Argument must be integral type.");

    constexpr uint32_t maxDigits = sizeof(T) * 2;
    const uint32_t requiredSpace = std::max(width, maxDigits);

    if (uiBytesLeft_ < requiredSpace) { return false; }

    auto [end, ec] = std::to_chars(*buffer_, *buffer_ + uiBytesLeft_, arg, 16);

    if (ec != std::errc{}) { return false; }

    const auto num_digits = static_cast<uint32_t>(end - *buffer_);

    if (num_digits < width)
    {
        const uint32_t pad_chars = width - num_digits;
        std::memmove(*buffer_ + pad_chars, *buffer_, num_digits);
        std::fill_n(*buffer_, pad_chars, '0');
        end += pad_chars;
    }

    uiBytesLeft_ -= static_cast<uint32_t>(end - *buffer_);
    *buffer_ = end;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename T>
[[nodiscard]] bool WriteFloatToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, T value, std::chars_format format, std::optional<int> precision)
{
    AssertWritableByteBuffer<BufferType>();
    static_assert(std::is_floating_point_v<T>, "WriteFloatToBuffer requires float/double.");

    int precision_arg = -1;
    if (precision.has_value())
    {
        precision_arg = *precision;
        if (precision_arg < 0) { precision_arg = 0; }
    }
    else if (format == std::chars_format::fixed || format == std::chars_format::scientific) { precision_arg = 6; }

    auto [end, ec] = std::to_chars(*buffer_, *buffer_ + uiBytesLeft_, value, format, precision_arg);

    if (ec != std::errc{}) { return false; }

    uiBytesLeft_ -= static_cast<uint32_t>(end - *buffer_);
    *buffer_ = end;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType>
[[nodiscard]] inline bool SetInBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, const unsigned char iItem_, const uint32_t uiItemSize_)
{
    AssertWritableByteBuffer<BufferType>();
    if (uiBytesLeft_ < uiItemSize_) { return false; }
    std::memset(*buffer_, iItem_, uiItemSize_);
    *buffer_ += uiItemSize_;
    uiBytesLeft_ -= uiItemSize_;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename T> [[nodiscard]] bool CopyToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, const T& item)
{
    AssertWritableByteBuffer<BufferType>();
    static_assert(!std::is_pointer_v<T>, "Pointers not allowed.");
    if (uiBytesLeft_ < sizeof(T)) { return false; }
    std::memcpy(*buffer_, &item, sizeof(T));
    *buffer_ += sizeof(T);
    uiBytesLeft_ -= sizeof(T);
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType> [[nodiscard]] bool CopyToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, const char* ptItem_)
{
    AssertWritableByteBuffer<BufferType>();
    auto uiItemSize = static_cast<uint32_t>(std::strlen(ptItem_));
    if (uiBytesLeft_ < uiItemSize) { return false; }
    std::memcpy(*buffer_, ptItem_, uiItemSize);
    *buffer_ += uiItemSize;
    uiBytesLeft_ -= uiItemSize;
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType> [[nodiscard]] inline bool CopyToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, std::string_view sv)
{
    AssertWritableByteBuffer<BufferType>();
    if (uiBytesLeft_ < sv.size()) { return false; }
    std::memcpy(*buffer_, sv.data(), sv.size());
    *buffer_ += sv.size();
    uiBytesLeft_ -= static_cast<uint32_t>(sv.size());
    return true;
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType> [[nodiscard]] inline bool CopyToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, const std::string& str)
{
    AssertWritableByteBuffer<BufferType>();
    if (uiBytesLeft_ < str.size()) { return false; }
    std::memcpy(*buffer_, str.data(), str.size());
    *buffer_ += str.size();
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
template <typename BufferType, typename... Args> [[nodiscard]] bool CopyAllToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, Args&&... args_)
{
    AssertWritableByteBuffer<BufferType>();
    return ([&](auto&& arg) {
        using Decayed = std::decay_t<decltype(arg)>;
        auto* ppcBuffer = reinterpret_cast<char*>(*buffer_);
        if constexpr (std::is_integral_v<Decayed> && !std::is_same_v<Decayed, char>)
        {
            if (!WriteIntToBuffer(&ppcBuffer, uiBytesLeft_, arg)) { return false; }
        }
        else if constexpr (is_specialization_of_v<Decayed, FloatValue>)
        {
            if (!WriteFloatToBuffer(&ppcBuffer, uiBytesLeft_, arg.value, arg.format, arg.precision)) { return false; }
        }
        else if constexpr (is_specialization_of_v<Decayed, HexValue>)
        {
            if (!WriteHexToBuffer(&ppcBuffer, uiBytesLeft_, arg.value, arg.width)) { return false; }
        }
        else { return CopyToBuffer(buffer_, uiBytesLeft_, arg); }
        *buffer_ = reinterpret_cast<BufferType>(ppcBuffer);
        return true;
    }(args_) &&
            ...);
}

// -------------------------------------------------------------------------------------------------------
template <typename BufferType, typename... Args>
[[nodiscard]] bool CopyAllToBufferSeparated(BufferType* buffer_, uint32_t& uiBytesLeft_, const char cSeparator_, Args&&... args_)
{
    AssertWritableByteBuffer<BufferType>();
    bool success = true;
    size_t i = 0;
    (
        [&](auto&& arg) {
            if (!success) { return; }
            success &= CopyAllToBuffer(buffer_, uiBytesLeft_, arg);
            // Copy the separator to the buffer for every item except the last.
            if (success && (i++ < sizeof...(Args) - 1)) { success &= CopyToBuffer(buffer_, uiBytesLeft_, cSeparator_); }
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

    //----------------------------------------------------------------------------
    //! \brief Align the binary buffer pointer to the expected type boundary.
    //
    //! \param[in] align The alignment requirement in bytes.
    //! \param[in] start The starting pointer of the binary buffer. Used to
    //! calculate the current offset relative to the beginning of the payload.
    //! \param[in, out] ptr A pointer to the buffer pointer to be advanced
    //! to meet the alignment requirement.
    //
    //! \remark This default implementation assumes NovAtel binary log alignment,
    //! where fields are generally aligned to 4-byte boundaries, unless the
    //! field's type length is smaller. The alignment applied is the minimum of
    //! 4 and the requested alignment.
    //
    //! For binary formats that use packed structures and do not align fields
    //! this function should be overridden in a derived decoder class to skip
    //! alignment.
    //
    //! \return True when alignment succeeds, false if there is not enough
    //! remaining buffer space to apply padding.
    //----------------------------------------------------------------------------
    virtual bool AlignBufferPointer(const uint8_t align, const unsigned char* start, unsigned char** ptr, uint32_t& uiBytesLeft_) const
    {
        const uint8_t alignment = std::min(static_cast<uint8_t>(4), align);
        if (alignment <= 1U) { return true; }

        const uint64_t offset = static_cast<uintptr_t>(*ptr - start) % alignment;
        if (offset == 0U) { return true; }

        const auto padding = static_cast<uint32_t>(alignment - offset);
        if (uiBytesLeft_ < padding) { return false; }

        *ptr += padding;
        uiBytesLeft_ -= padding;
        return true;
    }

    template <bool Flatten, bool Align>
    [[nodiscard]] bool EncodeBinaryBody(const MessageBody& stInterMessage_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                        unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_) const
    {
        const unsigned char* startBuf = *ppucOutBuf_;

        for (const auto& fieldDef : fieldDefinitions_)
        {
            if constexpr (Align)
            {
                if (!AlignBufferPointer(static_cast<uint8_t>(fieldDef->dataType.length), startBuf, ppucOutBuf_, uiBytesLeft_)) { return false; }
            }

            if (fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
            {
                const auto* arrayFieldDef = dynamic_cast<const FieldArrayField*>(fieldDef.get());
                const auto& elements = std::get<std::vector<MessageBody>>(stInterMessage_.varFields[fieldDef->index]);

                // Write array length if needed
                if (arrayFieldDef->arrayLengthRef.empty())
                {
                    if constexpr (Align)
                    {
                        if (!AlignBufferPointer(static_cast<uint8_t>(arrayFieldDef->arrayLengthFieldSize), startBuf, ppucOutBuf_, uiBytesLeft_))
                            return false;
                    }
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
                    if constexpr (Align)
                    {
                        if (!AlignBufferPointer(static_cast<uint8_t>(arrayFieldDef->arrayLengthFieldSize), startBuf, ppucOutBuf_, uiBytesLeft_))
                            return false;
                    }
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
                if (fieldDef->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"'))
                {
                    return false;
                }

                const auto arrayFieldDef = std::dynamic_pointer_cast<const ArrayField>(fieldDef);
                const size_t elemByteSize = fieldDef->dataType.length;
                const size_t baseIndex = fieldDef->index;
                for (size_t i = 0; i < arrayFieldDef->arrayLength; i++)
                {
                    const std::byte* const elementPtr = &vIntermediateFormat_.fixedFields[baseIndex + (i * elemByteSize)];

                    if (fieldDef->isString)
                    {
                        if (*elementPtr == std::byte{0}) { break; }
                    }

                    if (!FieldToAscii(fieldDef, elementPtr, ppcOutBuf_, uiBytesLeft_) ||
                        (fieldDef->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)))
                    {
                        return false;
                    }
                }

                if (fieldDef->isString)
                {
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"') || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                }
                else if (!fieldDef->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
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
                    const auto* enumField = dynamic_cast<const EnumField*>(field.fieldDef.get());
                    if (enumField->dataType.length == 1)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, GetEnumString(enumField->enumDef, std::get<int8_t>(vIntermediateFormat_.body.GetFieldValue(*fieldDef)))) ||
                            !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                        {
                            return false;
                        }
                    }
                    else if (enumField->dataType.length == 2)
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
                    const std::byte* const valuePtr = vIntermediateFormat_.fixedFields.data() + fieldDef->index;
                    if (!FieldToAscii(fieldDef, valuePtr, ppcOutBuf_, uiBytesLeft_) || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                    break;
                }
                default: return false;
                }
            }
        }
        return true;
    }

    [[nodiscard]] bool FieldToAscii(const BaseField::ConstPtr& fd_, const std::byte* valuePtr_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (valuePtr_ == nullptr) { return false; }

        const auto dispatchTyped = [&](auto* dummy_) -> bool {
            using T = std::remove_pointer_t<decltype(dummy_)>;
            T value{};
            std::memcpy(&value, valuePtr_, sizeof(T));
            return FieldToAscii(fd_, value, ppcOutBuf_, uiBytesLeft_);
        };

        switch (fd_->dataType.name)
        {
        case DATA_TYPE::BOOL: return dispatchTyped(static_cast<bool*>(nullptr));
        case DATA_TYPE::HEXBYTE:
        case DATA_TYPE::UCHAR: return dispatchTyped(static_cast<uint8_t*>(nullptr));
        case DATA_TYPE::CHAR: return dispatchTyped(static_cast<int8_t*>(nullptr));
        case DATA_TYPE::USHORT: return dispatchTyped(static_cast<uint16_t*>(nullptr));
        case DATA_TYPE::SHORT: return dispatchTyped(static_cast<int16_t*>(nullptr));
        case DATA_TYPE::UINT:
        case DATA_TYPE::ULONG:
        case DATA_TYPE::SATELLITEID: return dispatchTyped(static_cast<uint32_t*>(nullptr));
        case DATA_TYPE::INT:
        case DATA_TYPE::LONG: return dispatchTyped(static_cast<int32_t*>(nullptr));
        case DATA_TYPE::ULONGLONG: return dispatchTyped(static_cast<uint64_t*>(nullptr));
        case DATA_TYPE::LONGLONG: return dispatchTyped(static_cast<int64_t*>(nullptr));
        case DATA_TYPE::FLOAT: return dispatchTyped(static_cast<float*>(nullptr));
        case DATA_TYPE::DOUBLE: return dispatchTyped(static_cast<double*>(nullptr));
        default: return false;
        }
    }

    template <typename FieldType>
    [[nodiscard]] bool FieldToAscii(const BaseField::ConstPtr& fd_, const FieldType& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if constexpr (std::is_same_v<std::decay_t<FieldType>, MessageBody>) { return false; }
        else
        {
            auto it = asciiFieldMap.find(fd_->conversionHash);
            if (it != asciiFieldMap.end()) { return it->second(fd_, FieldValueVariant{val_}, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb); }

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
            auto it = jsonFieldMap.find(fd_->conversionHash);
            if (it != jsonFieldMap.end()) { return it->second(fd_, FieldValueVariant{val_}, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb); }

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

    [[nodiscard]] bool FieldToJson(const BaseField::ConstPtr& fd_, const std::byte* valuePtr_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (valuePtr_ == nullptr) { return false; }

        const auto dispatchTyped = [&](auto* dummy_) -> bool {
            using T = std::remove_pointer_t<decltype(dummy_)>;
            T value{};
            std::memcpy(&value, valuePtr_, sizeof(T));
            return FieldToJson(fd_, value, ppcOutBuf_, uiBytesLeft_);
        };

        switch (fd_->dataType.name)
        {
        case DATA_TYPE::BOOL: return dispatchTyped(static_cast<bool*>(nullptr));
        case DATA_TYPE::HEXBYTE:
        case DATA_TYPE::UCHAR: return dispatchTyped(static_cast<uint8_t*>(nullptr));
        case DATA_TYPE::CHAR: return dispatchTyped(static_cast<int8_t*>(nullptr));
        case DATA_TYPE::USHORT: return dispatchTyped(static_cast<uint16_t*>(nullptr));
        case DATA_TYPE::SHORT: return dispatchTyped(static_cast<int16_t*>(nullptr));
        case DATA_TYPE::UINT:
        case DATA_TYPE::ULONG:
        case DATA_TYPE::SATELLITEID: return dispatchTyped(static_cast<uint32_t*>(nullptr));
        case DATA_TYPE::INT:
        case DATA_TYPE::LONG: return dispatchTyped(static_cast<int32_t*>(nullptr));
        case DATA_TYPE::ULONGLONG: return dispatchTyped(static_cast<uint64_t*>(nullptr));
        case DATA_TYPE::LONGLONG: return dispatchTyped(static_cast<int64_t*>(nullptr));
        case DATA_TYPE::FLOAT: return dispatchTyped(static_cast<float*>(nullptr));
        case DATA_TYPE::DOUBLE: return dispatchTyped(static_cast<double*>(nullptr));
        default: return false;
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
            else if (fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
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
                    stInterMessage_.varFields[fieldDef->index]);

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
            else if (fieldDef->type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
            {
                if (fieldDef->isString)
                {
                    if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": ")")) { return false; }
                }
                else if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": [)"))
                {
                    return false;
                }

                const auto arrayFieldDef = std::dynamic_pointer_cast<const ArrayField>(fieldDef);
                const size_t elemByteSize = fieldDef->dataType.length;
                const size_t baseIndex = fieldDef->index;
                bool wroteAny = false;

                for (size_t i = 0; i < arrayFieldDef->arrayLength; i++)
                {
                    const std::byte* const elementPtr = &stInterMessage_.fixedFields[baseIndex + (i * elemByteSize)];

                    if (fieldDef->isString && (*elementPtr == std::byte{0})) { break; }

                    if (!FieldToJson(fieldDef, elementPtr, ppcOutBuf_, uiBytesLeft_)) { return false; }

                    if (!fieldDef->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                    wroteAny = true;
                }

                if (fieldDef->isString)
                {
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\",")) { return false; }
                }
                else
                {
                    if (wroteAny) { *(*ppcOutBuf_ - 1) = ']'; }
                    else if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ']')) { return false; }

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
