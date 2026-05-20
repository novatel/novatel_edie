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
    //! this function should be overridden in a derived encoder class to skip
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

    template <bool Flatten>
    [[nodiscard]] bool EncodeBinaryBody(const MessageBody& stInterMessage_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                        unsigned char** ppucOutBuf_, uint32_t& uiBytesLeft_) const
    {
        // Fast path: all-fixed message — single memcpy of the dense fixedFields blob
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
            if (!AlignBufferPointer(static_cast<uint8_t>(fieldDef->dataType.length), startBuf, ppucOutBuf_, uiBytesLeft_)) { return false; }

            if (fieldDef->type == FIELD_TYPE::FIELD_ARRAY || fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
                bool isFieldArray = fieldDef->type == FIELD_TYPE::FIELD_ARRAY;
                const auto* arrayFieldDef = dynamic_cast<const ArrayField*>(fieldDef.get());
                const auto* fieldArrayFieldDef = isFieldArray ? dynamic_cast<const FieldArrayField*>(fieldDef.get()) : nullptr;
                const auto& varField = stInterMessage_.GetVarFields()[fieldDef->index];
                const auto* flatArray = std::get_if<std::vector<std::byte>>(&varField);
                const auto* elements = (flatArray || !isFieldArray) ? nullptr : &std::get<std::vector<MessageBody>>(varField);
                size_t count = 0;
                if (flatArray) { count = flatArray->size() / fieldArrayFieldDef->fieldInfo.fixedFieldBytes; }
                else if (isFieldArray && elements) { count = elements->size(); }
                else if (!isFieldArray) { count = stInterMessage_.GetFieldByteSize(*fieldDef) / fieldDef->dataType.length; }

                // Write array length
                if (arrayFieldDef->arrayLengthRef.empty())
                {
                    if (!AlignBufferPointer(static_cast<uint8_t>(arrayFieldDef->arrayLengthFieldSize), startBuf, ppucOutBuf_, uiBytesLeft_))
                        return false;
                    switch (arrayFieldDef->arrayLengthFieldSize)
                    {
                    case 1: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint8_t>(count))) return false; break;
                    case 2: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint16_t>(count))) return false; break;
                    case 4: if (!CopyToBuffer(ppucOutBuf_, uiBytesLeft_, static_cast<uint32_t>(count))) return false; break;
                    default: return false;
                    }
                }

                const unsigned char* startPos = *ppucOutBuf_;

                // Fast path: all fields are fixed - directly copy the whole array
                if (flatArray || !isFieldArray)
                {
                    if (!stInterMessage_.WriteFieldToBuffer(*fieldDef, ppucOutBuf_, uiBytesLeft_)) { return false; }
                }
                // Slow path (only for FIELD_ARRAY): individually encode each element of the array
                else
                {
                    for (const auto& element : *elements)
                    {
                        if (!EncodeBinaryBody<Flatten>(element, fieldArrayFieldDef->fieldInfo.messageOrderedFields, ppucOutBuf_, uiBytesLeft_))
                            return false;
                    }
                }

                // For a flattened version of the log, fill in the remaining fields with 0x00.
                if constexpr (Flatten)
                {
                    const auto written = static_cast<uint32_t>(*ppucOutBuf_ - startPos);
                    const int remaining = (isFieldArray ? fieldArrayFieldDef->fieldSize
                                                        : arrayFieldDef->arrayLength * fieldDef->dataType.length) - written;
                    if (remaining > 0 && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, remaining)) { return false; }
                }
            }
            else if (fieldDef->type == FIELD_TYPE::STRING)
            {
                // Copy string data
                if (!stInterMessage_.WriteFieldToBuffer(*fieldDef, ppucOutBuf_, uiBytesLeft_)) return false;

                // Padding
                if constexpr (Flatten)
                {
                    const auto* arrayFieldDef = dynamic_cast<const ArrayField*>(fieldDef.get());
                    const auto maxSize = arrayFieldDef->arrayLength * fieldDef->dataType.length;
                    const auto written = stInterMessage_.GetFieldByteSize(*fieldDef);
                    if (written < static_cast<size_t>(maxSize) && !SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, maxSize - static_cast<uint32_t>(written))) return false;
                }
                else
                {
                    if (!SetInBuffer(ppucOutBuf_, uiBytesLeft_, 0, 4 - (reinterpret_cast<uint64_t>(*ppucOutBuf_) % 4))) return false;
                }
            }
            else
            {
                // All other fields (SIMPLE, ENUM, RESPONSE_ID, RESPONSE_STR, FIXED_LENGTH_ARRAY): copy bytes directly
                if (!stInterMessage_.WriteFieldToBuffer(*fieldDef, ppucOutBuf_, uiBytesLeft_)) return false;
            }
        }

        return true;
    }

    template <bool Abbreviated>
    [[nodiscard]] bool EncodeFixedAsciiField(const MessageBody& clMessageBody_, const BaseField::ConstPtr& fieldDefinition_,
                                            char** ppcOutBuf_, uint32_t& uiBytesLeft_, const size_t elementCount_ = 0) const
    {
        switch (fieldDefinition_->type)
        {
        case FIELD_TYPE::FIELD_ARRAY: {
            auto arrayFieldDef = std::dynamic_pointer_cast<const FieldArrayField>(fieldDefinition_);
            if (!arrayFieldDef) return false;
            for (size_t i = 0; i < elementCount_; i++)
            {
                if constexpr (Abbreviated)
                {
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n")) { return false; }
                }
                for (const auto& fieldDef : arrayFieldDef->fieldInfo.messageOrderedFields)
                {
                    if (!FieldToAscii<Abbreviated>(fieldDef,
                                                   MessageBody::GetValueFromFlatFieldArray(*fieldDef, std::get<std::vector<std::byte>>(clMessageBody_.GetFieldValue(*fieldDefinition_)), i,
                                                                                           arrayFieldDef->fieldInfo.fixedFieldBytes),
                                                   ppcOutBuf_, uiBytesLeft_))
                    {
                        return false;
                    }
                }
            }
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            if (!FieldToAscii<Abbreviated>(fieldDefinition_, clMessageBody_.GetFieldValue(*fieldDefinition_), ppcOutBuf_, uiBytesLeft_)) { return false; }
            break;
        }
        case FIELD_TYPE::RESPONSE_ID: break; // Do nothing, ascii logs don't output this field
        case FIELD_TYPE::ENUM: [[fallthrough]];
        case FIELD_TYPE::SIMPLE: {
            if (!FieldToAscii<Abbreviated>(fieldDefinition_, clMessageBody_.GetFieldValue(*fieldDefinition_), ppcOutBuf_, uiBytesLeft_)) { return false; }
            break;
        }
        default:
            return false;
        }
        return true;
    }

    template <bool Abbreviated>
    [[nodiscard]] bool EncodeAsciiBody(const MessageBody& clMessageBody_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
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

            switch (fieldDef->type)
            {
            case FIELD_TYPE::FIELD_ARRAY: {
                const auto* arrayFieldDef = dynamic_cast<const FieldArrayField*>(fieldDef.get());
                if (arrayFieldDef == nullptr || fieldDef->index >= clMessageBody_.GetVarFields().size()) { return false; }

                const auto& varField = clMessageBody_.GetVarFields()[fieldDef->index];
                const auto* flat = std::get_if<std::vector<std::byte>>(&varField);
                const auto* elements = flat ? nullptr : std::get_if<std::vector<MessageBody>>(&varField);

                const size_t perElem = arrayFieldDef->fieldInfo.fixedFieldBytes;
                if ((!flat && !elements) || (flat && perElem == 0)) { return false; }
                size_t elementCount = flat ? flat->size() / perElem : elements->size();

                if (!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, elementCount) ||
                    !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                {
                    return false;
                }

                if constexpr (Abbreviated)
                {
                    // Abbrev ascii will output a blank line for a 0 length array, nice
                    if (elementCount == 0)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n<") ||
                            !SetInBuffer(ppcOutBuf_, uiBytesLeft_, ' ', (uiIndents_ + 1) * Derived::indentLengthAbbAscii))
                        {
                            return false;
                        }
                        newIndentLine = true;
                    }
                }

                if (flat)
                {
                    if (!EncodeFixedAsciiField<Abbreviated>(clMessageBody_, fieldDef, ppcOutBuf_, uiBytesLeft_, elementCount)) { return false; }
                }
                else
                {
                    // Data was printed so a new line is required at the end of the array if there are more fields in the log
                    for (size_t i = 0; i < elementCount; ++i)
                    {
                        if constexpr (Abbreviated)
                        {
                            if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\r\n")) { return false; }
                        }

                        const auto& element = (*elements)[i];
                        if (!EncodeAsciiBody<Abbreviated>(element, arrayFieldDef->fieldInfo.messageOrderedFields, ppcOutBuf_,
                                                          uiBytesLeft_, uiIndents_ + 1))
                        {
                            return false;
                        }
                    }
                }

                newIndentLine = true;
                break;
            }
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
                if (!FieldToAscii<Abbreviated>(fieldDef, clMessageBody_.GetFieldValue(*fieldDef), ppcOutBuf_, uiBytesLeft_))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::STRING: // STRING types can be handled all at once because they are a single element and have a null terminator
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::get<std::string>(clMessageBody_.GetFieldValue(*fieldDef)), '"') ||
                    !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::RESPONSE_STR:
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, std::get<std::string>(clMessageBody_.GetFieldValue(*fieldDef))) ||
                    !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                {
                    return false;
                }
                break;
            default:
                if (!EncodeFixedAsciiField<Abbreviated>(clMessageBody_, fieldDef, ppcOutBuf_, uiBytesLeft_)) { return false; }
            }
        }
        return true;
    }

    template <typename T>
    [[nodiscard]] bool FieldToAscii(const BaseField::ConstPtr& fd_, const T& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if constexpr (std::is_arithmetic_v<T>) {
            auto it = asciiFieldMap.find(fd_->conversionHash);
            if (it != asciiFieldMap.end()) { return it->second(fd_, FieldValueVariant(val_), ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb); }

            if (fd_->type == FIELD_TYPE::ENUM)
            {
                auto enumFieldPtr = std::dynamic_pointer_cast<const EnumField>(fd_);
                if (!enumFieldPtr) return false;
                return CopyToBuffer(ppcOutBuf_, uiBytesLeft_, GetEnumString(enumFieldPtr->enumDef, val_));
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
        return false;
    }

    template <bool Abbreviated>
    [[nodiscard]] bool FieldToAscii(const BaseField::ConstPtr& fd_, const FieldValueVariant& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        constexpr char separator = Abbreviated ? Derived::separatorAbbAscii : Derived::separatorAscii;

        return std::visit([&](auto&& fieldValue) -> bool {
            using ValueType = std::decay_t<decltype(fieldValue)>;
            switch (fd_->type)
            {
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
                if constexpr (is_specialization_of_v<ValueType, std::vector>)
                {
                    // Print the array size first
                    if (!WriteIntToBuffer(ppcOutBuf_, uiBytesLeft_, fieldValue.size()) ||
                        !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator))
                    {
                        return false;
                    }
                }
                else { return false; }
                [[fallthrough]];
            case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
                if constexpr (is_specialization_of_v<ValueType, std::vector>)
                {
                    if (fd_->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')) { return false; }

                    for (const auto& element : fieldValue)
                    {
                        using ElemType = std::decay_t<decltype(element)>;
                        if (fd_->isString)
                        {
                            if constexpr (std::is_same_v<ElemType, int8_t> || std::is_same_v<ElemType, uint8_t>)
                            {
                                if (element == '\0') { break; }
                            }
                        }

                        if (!FieldToAscii(fd_, element, ppcOutBuf_, uiBytesLeft_) ||
                            (fd_->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)))
                        {
                            return false;
                        }
                    }

                    // Quoted elements need a trailing comma
                    if (fd_->isString)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"') || !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                    }
                    // Non-quoted, non-internally-separated elements also need a trailing comma
                    else if (!fd_->isCsv && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator)) { return false; }
                }
                else { return false; }
                break;
            }
            default:
                if constexpr (is_specialization_of_v<ValueType, std::vector>) { return false; }
                else { return FieldToAscii(fd_, fieldValue, ppcOutBuf_, uiBytesLeft_) && CopyToBuffer(ppcOutBuf_, uiBytesLeft_, separator); }
            }
            return true;
        }, val_);
    }

    template <typename T>
    [[nodiscard]] bool FieldToJson(const BaseField::ConstPtr& fd_, const T& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if constexpr (std::is_arithmetic_v<T>) {
            auto it = jsonFieldMap.find(fd_->conversionHash);
            if (it != jsonFieldMap.end()) { return it->second(fd_, FieldValueVariant{val_}, ppcOutBuf_, uiBytesLeft_, *pclMyMsgDb); }

            if (fd_->type == FIELD_TYPE::ENUM) {
                auto enumFieldPtr = std::dynamic_pointer_cast<const EnumField>(fd_);
                if (!enumFieldPtr) return false;
                return CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', GetEnumString(enumFieldPtr->enumDef, val_), "\"");
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
        return false;
    }

    [[nodiscard]] bool FieldToJson(const BaseField::ConstPtr& fd_, const FieldValueVariant& val_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fd_->name), "\": ")) { return false; }

        return std::visit([&](auto&& fieldValue) -> bool {
            using ValueType = std::decay_t<decltype(fieldValue)>;
            switch (fd_->type)
            {
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
            case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
                if constexpr (is_specialization_of_v<ValueType, std::vector>)
                {
                    if (fd_->isString)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '"')) { return false; }
                    }
                    // This is an array of simple elements
                    else if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '['))
                    {
                        return false;
                    }

                    bool wroteAny = false;
                    // Numeric array
                    for (const auto& elem : fieldValue)
                    {
                        using ElemType = std::decay_t<decltype(elem)>;
                        // Skip null terminators for string data
                        if (fd_->isString)
                        {
                            if constexpr (std::is_same_v<ElemType, int8_t> || std::is_same_v<ElemType, uint8_t>)
                            {
                                if (elem == '\0') { break; }
                            }
                        }

                        if (!FieldToJson(fd_, elem, ppcOutBuf_, uiBytesLeft_) ||
                            (!fd_->isString && !CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')))
                        {
                            return false;
                        }
                        wroteAny = true;
                    }

                    // Quoted elements need a trailing comma
                    if (fd_->isString)
                    {
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "\",")) { return false; }
                    }
                    // Non-quoted, non-internally-separated elements also need a trailing comma
                    else
                    {
                        if (wroteAny) { *(*ppcOutBuf_ - 1) = ']'; }
                        else if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ']')) { return false; }
                        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                    }
                    break;
                }
                else { return false; }
            }
            default:
                if constexpr (is_specialization_of_v<ValueType, std::vector>) { return false; }
                else { return FieldToJson(fd_, fieldValue, ppcOutBuf_, uiBytesLeft_) && CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ','); }
            }
            return true;
        }, val_);
    }

    [[nodiscard]] bool EncodeFixedJsonField(const MessageBody& clMessageBody_, const BaseField::ConstPtr& fieldDefinition_,
                                           char** ppcOutBuf_, uint32_t& uiBytesLeft_, const size_t elementCount_ = 0) const
    {
        switch (fieldDefinition_->type)
        {
        case FIELD_TYPE::FIELD_ARRAY: {
            auto arrayFieldDef = std::dynamic_pointer_cast<const FieldArrayField>(fieldDefinition_);
            if (!arrayFieldDef) return false;
            for (size_t i = 0; i < elementCount_; i++)
            {
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '{')) { return false; }
                for (const auto& fieldDef : arrayFieldDef->fieldInfo.messageOrderedFields)
                {
                    if (!FieldToJson(fieldDef,
                                     MessageBody::GetValueFromFlatFieldArray(*fieldDef, std::get<std::vector<std::byte>>(clMessageBody_.GetFieldValue(*fieldDefinition_)), i,
                                                                             arrayFieldDef->fieldInfo.fixedFieldBytes),
                                     ppcOutBuf_, uiBytesLeft_))
                    {
                        return false;
                    }
                }
                *(*ppcOutBuf_ - 1) = '}';
                if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
            }
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            if (!FieldToJson(fieldDefinition_, clMessageBody_.GetFieldValue(*fieldDefinition_), ppcOutBuf_, uiBytesLeft_)) { return false; }
            break;
        }
        case FIELD_TYPE::RESPONSE_ID: {
            const auto value = std::get<int32_t>(clMessageBody_.GetFieldValue(*fieldDefinition_));
            if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDefinition_->name), R"(": )",
                            value, ','))
            {
                return false;
            }
            break;
        }
        case FIELD_TYPE::ENUM: [[fallthrough]];
        case FIELD_TYPE::SIMPLE: {
            if (!FieldToJson(fieldDefinition_, clMessageBody_.GetFieldValue(*fieldDefinition_), ppcOutBuf_, uiBytesLeft_)) { return false; }
            break;
        }
        default:
            return false;
        }
        return true;
    }

    [[nodiscard]] bool EncodeJsonBody(const MessageBody& clMessageBody_, const std::vector<BaseField::ConstPtr>& fieldDefinitions_,
                                      char** ppcOutBuf_, uint32_t& uiBytesLeft_) const
    {
        if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, '{')) { return false; }

        for (const auto& fieldDef : fieldDefinitions_)
        {
            switch (fieldDef->type)
            {
            case FIELD_TYPE::FIELD_ARRAY: {
                const auto* arrayFieldDef = dynamic_cast<const FieldArrayField*>(fieldDef.get());
                if (arrayFieldDef == nullptr || fieldDef->index >= clMessageBody_.GetVarFields().size()) { return false; }

                const auto& varField = clMessageBody_.GetVarFields()[fieldDef->index];
                const auto* flat = std::get_if<std::vector<std::byte>>(&varField);
                const auto* elements = flat ? nullptr : std::get_if<std::vector<MessageBody>>(&varField);

                const size_t perElem = arrayFieldDef->fieldInfo.fixedFieldBytes;
                if ((!flat && !elements) || (flat && perElem == 0)) { return false; }
                size_t elementCount = flat ? flat->size() / perElem : elements->size();

                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": [)")) { return false; }
                if (elementCount == 0)
                {
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, "],")) { return false; }
                }
                else
                {
                    if (flat)
                    {
                        if (!EncodeFixedJsonField(clMessageBody_, fieldDef, ppcOutBuf_, uiBytesLeft_, elementCount)) { return false; }
                    }
                    else
                    {
                        for (size_t i = 0; i < elementCount; ++i)
                        {
                            const auto& element = (*elements)[i];
                            if (!EncodeJsonBody(element, arrayFieldDef->fieldInfo.messageOrderedFields, ppcOutBuf_, uiBytesLeft_))
                            {
                                return false;
                            }
                            if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                        }
                    }
                    *(*ppcOutBuf_ - 1) = ']';
                    if (!CopyToBuffer(ppcOutBuf_, uiBytesLeft_, ',')) { return false; }
                }
                break;
            }
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
                if (!FieldToJson(fieldDef, clMessageBody_.GetFieldValue(*fieldDef), ppcOutBuf_, uiBytesLeft_))
                {
                    return false;
                }
                break;
            case FIELD_TYPE::STRING: {
                const auto& str = std::get<std::string>(clMessageBody_.GetFieldValue(*fieldDef));
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": ")",
                                     str, "\","))
                {
                    return false;
                }
                break;
            }
            case FIELD_TYPE::RESPONSE_STR: {
                const auto& str = std::get<std::string>(clMessageBody_.GetFieldValue(*fieldDef));
                if (!CopyAllToBuffer(ppcOutBuf_, uiBytesLeft_, '"', std::string_view(fieldDef->name), R"(": ")", str, "\","))
                {
                    return false;
                }
                break;
            }
            default:
                if (!EncodeFixedJsonField(clMessageBody_, fieldDef, ppcOutBuf_, uiBytesLeft_)) { return false; }
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


