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
#include <functional>
#include <optional>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

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
template <typename BufferType, typename T> [[nodiscard]] bool WriteIntToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, const T arg)
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
template <typename BufferType, typename T>
[[nodiscard]] bool WriteHexToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, const T arg, uint32_t width)
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
[[nodiscard]] bool WriteFloatToBuffer(BufferType* buffer_, uint32_t& uiBytesLeft_, const T value, std::chars_format format,
                                      std::optional<int> precision)
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
template <typename T>
std::function<bool(const BaseField&, const CompositeField&, char**, uint32_t&, const MessageDatabase&, size_t)> BasicIntMapEntry()
{
    return [](const BaseField& fd_, const CompositeField& cf_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, const MessageDatabase&, size_t index) {
        return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<T>(fd_, index));
    };
}

// -------------------------------------------------------------------------------------------------------
template <typename T>
std::function<bool(const BaseField&, const CompositeField&, char**, uint32_t&, const MessageDatabase&, size_t)> BasicHexMapEntry(uint32_t width)
{
    return [width](const BaseField& fd_, const CompositeField& cf_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, const MessageDatabase&, size_t index) {
        return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<T>(fd_, index), width);
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
    std::function<size_t(const size_t, const uintptr_t, const uintptr_t)> fMyAlignmentFunc = MessageDatabase::NoAlign;

  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("encoder")};
    MessageDatabase::ConstPtr pclMyMsgDb{nullptr};

    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    using AsciiConverter = std::function<bool(const BaseField&, const CompositeField&, char**, uint32_t&, const MessageDatabase&, size_t index)>;

    // TODO: ASCII and JSON could probably share the same map.
    std::unordered_map<uint64_t, AsciiConverter> asciiFieldMap;
    std::unordered_map<uint64_t, AsciiConverter> jsonFieldMap;

    //----------------------------------------------------------------------------
    //! \brief Add padding after binary string fields to maintain alignment.
    //
    //! \param[in, out] ptr_ A pointer to the encode buffer pointer to be padded
    //!     if necessary. Updated in place.
    //! \param[in, out] uiBytesLeft_ The number of bytes left in the buffer.
    //!     Updated in place.
    //
    //! \return true if padding was added successfully or not needed, false if
    //!     there was not enough space in the buffer.
    //! \see MessageDecoderBase::AddStringFieldPadding() for the corresponding decoder method.
    //----------------------------------------------------------------------------
    virtual bool AddStringFieldPadding([[maybe_unused]] unsigned char** ptr, [[maybe_unused]] uint32_t& uiBytesLeft_) const { return true; }

    template <bool Flatten>
    [[nodiscard]] bool EncodeBinaryBody(const CompositeField& stInterMessage_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                        unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_) const
    {
        const auto alignPtr = [this](uint16_t fieldTypeLength_, const unsigned char* startBuf, unsigned char** ppucOutBuf_,
                                     uint32_t& uiBytesLeft_) -> bool {
            auto align = this->fMyAlignmentFunc(fieldTypeLength_, reinterpret_cast<uintptr_t>(startBuf), reinterpret_cast<uintptr_t>(*ppucOutBuf_));
            if (align > uiBytesLeft_) { return false; }
            *ppucOutBuf_ += align;
            uiBytesLeft_ -= static_cast<uint32_t>(align);
            return true;
        };

        // Fast path: all-fixed message — single memcpy of the fixedFields blob
        if (stInterMessage_.GetVarFields().empty())
        {
            if (stInterMessage_.GetFixedFields().size() > uiBytesLeft_) { return false; }
            std::memcpy(*ppucOutBuf_, stInterMessage_.GetFixedFields().data(), stInterMessage_.GetFixedFields().size());
            *ppucOutBuf_ += stInterMessage_.GetFixedFields().size();
            uiBytesLeft_ -= static_cast<uint32_t>(stInterMessage_.GetFixedFields().size());
            return true;
        }

        const unsigned char* startBuf = *ppucOutBuf_;

        for (const auto& fieldDef : fieldDefinitions_)
        {
            const auto& fieldDefRef = *fieldDef;
            if (!alignPtr(fieldDefRef.dataType.length, startBuf, ppucOutBuf_, uiBytesLeft_)) { return false; }

            if (fieldDefRef.type == FIELD_TYPE::FIELD_ARRAY || fieldDefRef.type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY ||
                fieldDefRef.type == FIELD_TYPE::STRING)
            {
                const auto* arrayFieldDef = dynamic_cast<const ArrayField*>(&fieldDefRef);
                if (!arrayFieldDef)
                {
                    throw std::runtime_error("Expected field definition of type ArrayField for FIELD_ARRAY or VARIABLE_LENGTH_ARRAY field");
                }
                const FieldArrayField* fieldArrayFieldDef{nullptr};

                auto encoded = std::visit(
                    [&](auto&& fieldVector) -> bool {
                        using ValueType = std::decay_t<decltype(fieldVector)>;
                        size_t maxBytes = 0;
                        size_t count = 0;
                        if constexpr (std::is_same_v<ValueType, FlatFieldArray> || std::is_same_v<ValueType, CompositeFieldArray>)
                        {
                            fieldArrayFieldDef = dynamic_cast<const FieldArrayField*>(fieldDef.get());
                            if (!fieldArrayFieldDef)
                            {
                                throw std::runtime_error("Expected field definition of type FieldArrayField for FIELD_ARRAY field");
                            }
                            maxBytes = fieldArrayFieldDef->fieldSize;
                            count = fieldVector.size();
                        }
                        else if constexpr (std::is_same_v<ValueType, std::string> || is_specialization_of_v<ValueType, std::vector>)
                        {
                            maxBytes = arrayFieldDef->arrayLength * fieldDef->dataType.length;
                            count = fieldVector.size();
                        }

                        if constexpr (is_specialization_of_v<ValueType, std::vector> || std::is_same_v<ValueType, FlatFieldArray>)
                        {
                            // Write array length
                            if (arrayFieldDef->arrayLengthRef.empty())
                            {
                                if (!alignPtr(arrayFieldDef->arrayLengthFieldSize, startBuf, ppucOutBuf_, uiBytesLeft_)) { return false; }
                                switch (arrayFieldDef->arrayLengthFieldSize)
                                {
                                case 1:
                                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint8_t>(count))) return false;
                                    break;
                                case 2:
                                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint16_t>(count))) return false;
                                    break;
                                case 4:
                                    if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint32_t>(count))) return false;
                                    break;
                                default: return false;
                                }
                            }
                        }

                        const unsigned char* startPos = *ppucOutBuf_;

                        // Variable-length FIELD_ARRAY
                        if constexpr (std::is_same_v<ValueType, CompositeFieldArray>)
                        {
                            for (const auto& element : fieldVector)
                            {
                                if (!EncodeBinaryBody<Flatten>(element, fieldArrayFieldDef->fieldInfo->messageOrderedFields, ppucOutBuf_,
                                                               uiBytesLeft_))
                                {
                                    return false;
                                }
                            }
                        }
                        // VARIABLE_LENGTH_ARRAY, STRING, and flat FIELD_ARRAY
                        else
                        {
                            if (count > 0 && !stInterMessage_.WriteFieldToBuffer(fieldDefRef, ppucOutBuf_, uiBytesLeft_)) { return false; }
                        }

                        // For a flattened version of the log, fill in the remaining fields with 0x00.
                        if constexpr (Flatten)
                        {
                            const auto written = static_cast<uint32_t>(*ppucOutBuf_ - startPos);
                            const int remaining = static_cast<int>(maxBytes - written);
                            if (remaining > 0 && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, remaining)) { return false; }
                        }

                        if constexpr (std::is_same_v<ValueType, std::string>) { return AddStringFieldPadding(ppucOutBuf_, uiBytesLeft_); }
                        return true;
                    },
                    stInterMessage_.GetVarFields()[fieldDefRef.index]);
                if (!encoded) { return false; }
            }
            else
            {
                // All other fields (SIMPLE, ENUM, RESPONSE_ID, RESPONSE_STR, FIXED_LENGTH_ARRAY): copy bytes directly
                // Note: the following check assumes none of the aforementioned field types can be empty
                if (!stInterMessage_.WriteFieldToBuffer(fieldDefRef, ppucOutBuf_, uiBytesLeft_)) { return false; }
            }
        }

        return true;
    }

    template <bool Json = false>
    [[nodiscard]] bool WriteAsciiValue(const BaseField& fd_, const CompositeField& cf_, char** ppcOutBuf_, uint32_t& uiBytesLeft_, size_t index) const
    {
        switch (fd_.dataType.name)
        {
        case DATA_TYPE::BOOL:
            if constexpr (Json)
            {
                return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::string_view(cf_.GetFieldValue<bool>(fd_, index) ? "true" : "false"));
            }
            else { return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::string_view(cf_.GetFieldValue<bool>(fd_, index) ? "TRUE" : "FALSE")); }
        case DATA_TYPE::HEXBYTE:
            if constexpr (!Json) { return WriteHexToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<uint8_t>(fd_, index), 2); }
            else { [[fallthrough]]; }
        case DATA_TYPE::UCHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<uint8_t>(fd_, index));
        case DATA_TYPE::CHAR: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<int8_t>(fd_, index));
        case DATA_TYPE::USHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<uint16_t>(fd_, index));
        case DATA_TYPE::SHORT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<int16_t>(fd_, index));
        case DATA_TYPE::UINT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<uint32_t>(fd_, index));
        case DATA_TYPE::INT: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<int32_t>(fd_, index));
        case DATA_TYPE::ULONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<uint32_t>(fd_, index));
        case DATA_TYPE::ULONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<uint64_t>(fd_, index));
        case DATA_TYPE::LONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<int32_t>(fd_, index));
        case DATA_TYPE::LONGLONG: return WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, cf_.GetFieldValue<int64_t>(fd_, index));
        case DATA_TYPE::FLOAT: {
            const auto v = cf_.GetFieldValue<float>(fd_, index);
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, v, FloatingPointFormat(fd_, v), fd_.precision);
        }
        case DATA_TYPE::DOUBLE: {
            const auto v = cf_.GetFieldValue<double>(fd_, index);
            return WriteFloatToBuffer(ppcOutBuf_, uiBytesLeft_, v, FloatingPointFormat(fd_, v), fd_.precision);
        }
        default:
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, "WriteAsciiValue(): unknown type.");
            throw std::runtime_error("WriteAsciiValue(): unknown type.");
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Write a single primitive element, using a custom conversion from
    //!     the field map when one is registered, otherwise the default
    //!     WriteAsciiValue formatting.
    //!
    //! This consolidates the "map entry vs. default value writer" fork shared by
    //! the ASCII and JSON primitive-field encoders.
    //!
    //! \tparam Json Whether JSON formatting is used for the default writer.
    //! \param[in] fd_ The field definition.
    //! \param[in] cf_ The message body providing the value.
    //! \param[in] fieldMap_ The conversion map (asciiFieldMap or jsonFieldMap).
    //! \param[in] index The element index (0 for scalars).
    // ---------------------------------------------------------------------------
    template <bool Json = false>
    [[nodiscard]] bool WriteAsciiElement(const BaseField& fd_, const CompositeField& cf_,
                                         const std::unordered_map<uint64_t, AsciiConverter>& fieldMap_, size_t index, char** ppcOutBuf_,
                                         uint32_t& uiBytesLeft_) const
    {
        const auto it = fieldMap_.find(fd_.conversionHash);
        if (it != fieldMap_.end()) { return (it->second)(fd_, cf_, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb, index); }
        return WriteAsciiValue<Json>(fd_, cf_, ppcOutBuf_, uiBytesLeft_, index);
    }

    template <bool Abbreviated>
    [[nodiscard]] bool EncodeAsciiField(const BaseField& fieldDefRef_, const CompositeField& cf_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        constexpr char separator = Abbreviated ? Derived::separatorAbbAscii : Derived::separatorAscii;

        if (fieldDefRef_.type == FIELD_TYPE::ENUM)
        {
            auto enumFieldPtr = dynamic_cast<const EnumField*>(&fieldDefRef_);
            if (!enumFieldPtr) return false;
            std::string_view enumString;
            SimpleTypeVisitor(fieldDefRef_, [&](auto&& arg) {
                enumString =
                    GetEnumString(enumFieldPtr->enumDef, static_cast<uint32_t>(cf_.GetFieldValue<std::decay_t<decltype(arg)>>(fieldDefRef_)));
            });
            return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, enumString, separator);
        }

        switch (fieldDefRef_.type)
        {
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto* arrayFieldDef = dynamic_cast<const ArrayField*>(&fieldDefRef_);
            if (!arrayFieldDef) { return false; }
            auto count = fieldDefRef_.type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY ? cf_.GetFieldSize(fieldDefRef_) : arrayFieldDef->arrayLength;

            if (fieldDefRef_.type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
                // Output the array length before the array elements for variable-length arrays
                if (!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, count) || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
            }

            if (fieldDefRef_.isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')) { return false; }

            for (size_t i = 0; i < count; i++)
            {
                if (fieldDefRef_.isString && (fieldDefRef_.dataType.name == DATA_TYPE::CHAR || fieldDefRef_.dataType.name == DATA_TYPE::UCHAR) &&
                    cf_.GetFieldValue<uint8_t>(*arrayFieldDef, i) == 0)
                {
                    break;
                }
                if (!WriteAsciiElement(fieldDefRef_, cf_, asciiFieldMap, i, ppcOutBuf_, uiBytesLeft_)) { return false; }
                if (fieldDefRef_.isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
            }

            // Quoted elements need a closing quote
            if (fieldDefRef_.isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')) { return false; }
            // Non-internally-separated elements (including strings) need a trailing separator
            return fieldDefRef_.isCsv || CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator);
        }
        case FIELD_TYPE::SIMPLE:
            if (!WriteAsciiElement(fieldDefRef_, cf_, asciiFieldMap, 0, ppcOutBuf_, uiBytesLeft_)) { return false; }
            return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator);
        default: throw std::runtime_error("EncodeAsciiField(): unsupported field type");
        }
        return true;
    }

    template <bool Abbreviated>
    [[nodiscard]] bool EncodeAsciiBody(const CompositeField& clCompField_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
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
            const auto& fieldDefRef = *fieldDef;
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

            switch (fieldDefRef.type)
            {
            case FIELD_TYPE::FIELD_ARRAY: {
                if (fieldDefRef.index >= clCompField_.GetVarFields().size()) { return false; }
                const size_t count = clCompField_.GetFieldSize(fieldDefRef);
                const auto* fieldArrayDef = dynamic_cast<const FieldArrayField*>(&fieldDefRef);
                if (!fieldArrayDef) { return false; }

                if (!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, count) || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }

                if constexpr (Abbreviated)
                {
                    // Abbrev ascii will output a blank line for a 0 length array, nice
                    if (count == 0)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<") ||
                            !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', (uiIndents_ + 1) * Derived::indentLengthAbbAscii))
                        {
                            return false;
                        }
                        newIndentLine = true;
                    }
                }

                auto encoded = std::visit(
                    [&](auto&& fa) -> bool {
                        using FieldArrayType = std::decay_t<decltype(fa)>;
                        const auto& subfieldDefs = fieldArrayDef->fieldInfo->messageOrderedFields;
                        if constexpr (std::is_same_v<FieldArrayType, FlatFieldArray> || std::is_same_v<FieldArrayType, CompositeFieldArray>)
                        {
                            for (size_t i = 0; i < count; i++)
                            {
                                if constexpr (Abbreviated)
                                {
                                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n")) { return false; }
                                }
                                if constexpr (std::is_same_v<FieldArrayType, CompositeFieldArray>)
                                {
                                    if (!EncodeAsciiBody<Abbreviated>(fa[i], subfieldDefs, ppcOutBuf_, uiBytesLeft_, uiIndents_ + 1))
                                    {
                                        return false;
                                    }
                                }
                                else if constexpr (std::is_same_v<FieldArrayType, FlatFieldArray>)
                                {
                                    const auto fixedFieldBytes = fieldArrayDef->fieldInfo->fixedFieldBytes;
                                    const auto* elementStart = fa.data() + (i * fixedFieldBytes);
                                    // Borrow the row's bytes in place instead of copying them into a
                                    // temporary CompositeField: the parent flat array owns the storage and
                                    // outlives this sub-encode.
                                    const CompositeField mb = CompositeField::ViewFixedFields(elementStart, fixedFieldBytes);
                                    if (!EncodeAsciiBody<Abbreviated>(mb, subfieldDefs, ppcOutBuf_, uiBytesLeft_, uiIndents_ + 1)) { return false; }
                                }
                                else { throw std::runtime_error("Unexpected field array type in EncodeAsciiBody"); }
                            }
                        }
                        else { throw std::runtime_error("Unexpected field array type in EncodeAsciiBody"); }
                        return true;
                    },
                    clCompField_.GetVarFields()[fieldDefRef.index]);
                if (!encoded) { return false; }

                newIndentLine = true;
                break;
            }
            case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::get<std::string>(clCompField_.GetVarFields()[fieldDefRef.index]), '"') ||
                    !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_STR:
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<std::string>(clCompField_.GetVarFields()[fieldDefRef.index])) ||
                    !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
            case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
            case FIELD_TYPE::ENUM: [[fallthrough]];
            case FIELD_TYPE::SIMPLE:
                if (!EncodeAsciiField<Abbreviated>(fieldDefRef, clCompField_, ppcOutBuf_, uiBytesLeft_)) { return false; }
                break;
            case FIELD_TYPE::RESPONSE_ID: break; // RESPONSE_ID is not included in ASCII output
            default: throw std::runtime_error("EncodeAsciiBody(): unsupported field type");
            }
        }
        return true;
    }

    [[nodiscard]] bool EncodeJsonField(const BaseField& fieldDefRef_, const CompositeField& cf_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDefRef_.name), "\": ")) { return false; }

        if (fieldDefRef_.type == FIELD_TYPE::ENUM)
        {
            auto enumFieldPtr = dynamic_cast<const EnumField*>(&fieldDefRef_);
            if (!enumFieldPtr) return false;
            std::string_view enumString;
            SimpleTypeVisitor(fieldDefRef_, [&](auto&& arg) {
                enumString =
                    GetEnumString(enumFieldPtr->enumDef, static_cast<uint32_t>(cf_.GetFieldValue<std::decay_t<decltype(arg)>>(fieldDefRef_)));
            });
            return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', enumString, "\",");
        }

        switch (fieldDefRef_.type)
        {
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            if (fieldDefRef_.isString)
            {
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')) { return false; }
            }
            // Array of simple elements
            else if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '[')) { return false; }

            bool wroteAny = false;
            const auto* arrayFieldDef = dynamic_cast<const ArrayField*>(&fieldDefRef_);
            if (!arrayFieldDef) { return false; }
            auto count = fieldDefRef_.type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY ? cf_.GetFieldSize(fieldDefRef_) : arrayFieldDef->arrayLength;

            for (size_t i = 0; i < count; i++)
            {
                if (fieldDefRef_.isString && (fieldDefRef_.dataType.name == DATA_TYPE::CHAR || fieldDefRef_.dataType.name == DATA_TYPE::UCHAR) &&
                    cf_.GetFieldValue<uint8_t>(*arrayFieldDef, i) == 0)
                {
                    break;
                }
                if (!WriteAsciiElement<true>(fieldDefRef_, cf_, jsonFieldMap, i, ppcOutBuf_, uiBytesLeft_)) { return false; }
                if (!fieldDefRef_.isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                wroteAny = true;
            }

            // Quoted elements need a closing quote
            if (fieldDefRef_.isString)
            {
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')) { return false; }
            }
            else if (wroteAny) { *(*ppcOutBuf_ - 1) = ']'; }
            else if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ']')) { return false; }

            return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',');
        }
        case FIELD_TYPE::RESPONSE_ID: [[fallthrough]];
        case FIELD_TYPE::SIMPLE:
            if (!WriteAsciiElement<true>(fieldDefRef_, cf_, jsonFieldMap, 0, ppcOutBuf_, uiBytesLeft_)) { return false; }
            return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',');
        default: throw std::runtime_error("EncodeJsonField(): unsupported field type");
        }
        return true;
    }

    [[nodiscard]] bool EncodeJsonBody(const CompositeField& clCompField_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                      char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '{')) { return false; }

        for (const auto& fieldDef : fieldDefinitions_)
        {
            const auto& fieldDefRef = *fieldDef;
            switch (fieldDefRef.type)
            {
            case FIELD_TYPE::FIELD_ARRAY: {
                const auto* arrayFieldDef = dynamic_cast<const FieldArrayField*>(&fieldDefRef);
                if (arrayFieldDef == nullptr || fieldDefRef.index >= clCompField_.GetVarFields().size()) { return false; }
                const size_t count = clCompField_.GetFieldSize(fieldDefRef);

                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDefRef.name), R"(": [)")) { return false; }
                if (count == 0)
                {
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "],")) { return false; }
                }
                else
                {
                    auto encoded = std::visit(
                        [&](auto&& fa) -> bool {
                            using FieldArrayType = std::decay_t<decltype(fa)>;
                            for (size_t i = 0; i < count; i++)
                            {
                                if constexpr (std::is_same_v<FieldArrayType, CompositeFieldArray>)
                                {
                                    if (!EncodeJsonBody(fa[i], arrayFieldDef->fieldInfo->messageOrderedFields, ppcOutBuf_, uiBytesLeft_))
                                    {
                                        return false;
                                    }
                                }
                                else if constexpr (std::is_same_v<FieldArrayType, FlatFieldArray>)
                                {
                                    const auto fixedFieldBytes = arrayFieldDef->fieldInfo->fixedFieldBytes;
                                    const auto* elementStart = fa.data() + (i * fixedFieldBytes);
                                    // Borrow the row's bytes in place instead of copying them into a
                                    // temporary CompositeField: the parent flat array owns the storage and
                                    // outlives this sub-encode.
                                    const CompositeField mb = CompositeField::ViewFixedFields(elementStart, fixedFieldBytes);
                                    if (!EncodeJsonBody(mb, arrayFieldDef->fieldInfo->messageOrderedFields, ppcOutBuf_, uiBytesLeft_))
                                    {
                                        return false;
                                    }
                                }
                                else { throw std::runtime_error("Unexpected field array type in EncodeJsonBody"); }

                                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                            }
                            return true;
                        },
                        clCompField_.GetVarFields()[fieldDefRef.index]);
                    if (!encoded) { return false; }
                    *(*ppcOutBuf_ - 1) = ']';
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                }
                break;
            }
            case FIELD_TYPE::RESPONSE_STR: [[fallthrough]];
            case FIELD_TYPE::STRING: {
                const auto value = clCompField_.GetVarFields()[fieldDefRef.index];
                const auto& str = std::get<std::string>(value);
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDefRef.name), R"(": ")", str, "\",")) { return false; }
                break;
            }
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
            case FIELD_TYPE::FIXED_LENGTH_ARRAY: [[fallthrough]];
            case FIELD_TYPE::ENUM: [[fallthrough]];
            case FIELD_TYPE::SIMPLE:
                if (!EncodeJsonField(fieldDefRef, clCompField_, ppcOutBuf_, uiBytesLeft_)) { return false; }
                break;
            default: throw std::runtime_error("EncodeJsonBody(): unsupported field type");
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
    EncoderBase(std::string expectedMessageFamily_, MessageDatabase::ConstPtr pclMessageDb_ = nullptr,
                std::function<size_t(const size_t, const uintptr_t, const uintptr_t)> fAlignmentFunc_ = MessageDatabase::NoAlign)
        : sMyExpectedMessageFamily(std::move(expectedMessageFamily_)), fMyAlignmentFunc(std::move(fAlignmentFunc_))
    {
        static_cast<Derived*>(this)->InitFieldMaps();
        if (pclMessageDb_ != nullptr) { LoadJsonDb(pclMessageDb_); }
    }

    //----------------------------------------------------------------------------
    //! \brief A destructor for the EncoderBase class.
    //----------------------------------------------------------------------------
    virtual ~EncoderBase() = default;

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
