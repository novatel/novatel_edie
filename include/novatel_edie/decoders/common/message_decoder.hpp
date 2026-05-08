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
// ! \file message_decoder.hpp
// ===============================================================================

#ifndef MESSAGE_DECODER_HPP
#define MESSAGE_DECODER_HPP

#include <charconv>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <simdjson.h>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

struct MessageBody;

#define PRIMITIVE_TYPES bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double
#define PRIMITIVE_VECTORS std::vector<int8_t>, std::vector<int16_t>, std::vector<int32_t>, std::vector<int64_t>, std::vector<uint8_t>, std::vector<uint16_t>, std::vector<uint32_t>, std::vector<uint64_t>, std::vector<float>, std::vector<double>
using FieldValueVariant = std::variant<PRIMITIVE_TYPES, PRIMITIVE_VECTORS, std::vector<MessageBody>, std::vector<std::byte>, std::string>;

struct MessageBody
{
    std::vector<std::byte> fixedFields;
    std::vector<FieldValueVariant> varFields;

    MessageBody() = default;

    template <bool Fixed = true, typename T>
    void CopyToBuffer(const size_t startIndex_, const T* values_, size_t n = 1)
    {
        static_assert(std::is_trivially_copyable_v<T>, "CopyToBuffer only supports trivially copyable types");

        if (values_ == nullptr)
        {
            throw std::runtime_error("CopyToBuffer(): source pointer is null");
        }

        const size_t valueSize = sizeof(T) * n;

        if constexpr (Fixed)
        {
            if (startIndex_ + valueSize > fixedFields.size())
            {
                throw std::runtime_error("CopyToBuffer(): fixedFields buffer overflow");
            }

            std::memcpy(fixedFields.data() + startIndex_, values_, valueSize);
        }
        else
        {
            if (startIndex_ >= varFields.size())
            {
                throw std::runtime_error("CopyToBuffer(): varFields index is out of range");
            }

            using BufferElementType = std::conditional_t<std::is_same_v<T, bool>, uint8_t, T>;
            std::vector<BufferElementType> values(n);
            std::memcpy(values.data(), values_, valueSize);
            varFields[startIndex_] = std::move(values);
        }
    }

    template <bool Fixed = true, typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    void CopyToBuffer(const size_t startIndex_, const T& value_, size_t n = 1)
    {
        CopyToBuffer<Fixed>(startIndex_, &value_, n);
    }

    template <bool Fixed = true, typename T>
    void CopyToBuffer(const size_t startIndex_, std::vector<T>&& values_)
    {
        static_assert(std::is_trivially_copyable_v<T>, "CopyToBuffer only supports trivially copyable vector element types");
        static_assert(!std::is_same_v<T, bool>, "CopyToBuffer does not support std::vector<bool>");

        if constexpr (Fixed)
        {
            const size_t valueSize = sizeof(T) * values_.size();
            if (startIndex_ + valueSize > fixedFields.size())
            {
                throw std::runtime_error("CopyToBuffer(): fixedFields buffer overflow");
            }

            if (valueSize > 0)
            {
                std::memcpy(fixedFields.data() + startIndex_, values_.data(), valueSize);
            }
        }
        else
        {
            if (startIndex_ >= varFields.size())
            {
                throw std::runtime_error("CopyToBuffer(): varFields index is out of range");
            }

            varFields[startIndex_] = std::move(values_);
        }
    }

    template <bool Fixed = true>
    void CopyToBuffer(const size_t startIndex_, std::string&& value_)
    {
        if constexpr (Fixed)
        {
            const size_t valueSize = value_.size();
            if (startIndex_ + valueSize > fixedFields.size())
            {
                throw std::runtime_error("CopyToBuffer(): fixedFields buffer overflow");
            }

            if (valueSize > 0)
            {
                std::memcpy(fixedFields.data() + startIndex_, value_.data(), valueSize);
            }
        }
        else
        {
            if (startIndex_ >= varFields.size())
            {
                throw std::runtime_error("CopyToBuffer(): varFields index is out of range");
            }

            varFields[startIndex_] = std::move(value_);
        }
    }

    template <bool Fixed = true, typename T>
    void SetFieldElement(const size_t startIndex_, size_t elementIndex_, const T& value_)
    {
        using StoredType = std::conditional_t<std::is_same_v<T, bool>, uint8_t, T>;

        if constexpr (Fixed)
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                const auto storedValue = static_cast<uint8_t>(value_);
                CopyToBuffer<true>(startIndex_ + (elementIndex_ * sizeof(StoredType)), storedValue, 1);
            }
            else
            {
                CopyToBuffer<true>(startIndex_ + (elementIndex_ * sizeof(T)), value_, 1);
            }
            return;
        }

        if (startIndex_ >= varFields.size())
        {
            throw std::runtime_error("SetFieldElement(): varFields index is out of range");
        }

        // If this var field is already configured as flat bytes, preserve that storage type
        // and accept any 1-byte scalar assignment.
        if constexpr (!std::is_same_v<T, bool> && std::is_integral_v<T> && sizeof(T) == 1)
        {
            if (auto* byteValues = std::get_if<std::vector<std::byte>>(&varFields[startIndex_]))
            {
                if (byteValues->size() <= elementIndex_) { byteValues->resize(elementIndex_ + 1); }
                (*byteValues)[elementIndex_] = static_cast<std::byte>(static_cast<uint8_t>(value_));
                return;
            }
        }

        auto* values = std::get_if<std::vector<StoredType>>(&varFields[startIndex_]);
        if (values == nullptr)
        {
            varFields[startIndex_] = std::vector<StoredType>{};
            values = std::get_if<std::vector<StoredType>>(&varFields[startIndex_]);
        }

        if (values == nullptr)
        {
            throw std::runtime_error("SetFieldElement(): field storage type mismatch");
        }

        if (values->size() <= elementIndex_) { values->resize(elementIndex_ + 1); }
        (*values)[elementIndex_] = static_cast<StoredType>(value_);
    }

    void SetFlatFieldArrayByte(const size_t startIndex_, const size_t elementIndex_, const std::byte value_)
    {
        if (startIndex_ >= varFields.size())
        {
            throw std::runtime_error("SetFlatFieldArrayByte(): varFields index is out of range");
        }

        auto* values = std::get_if<std::vector<std::byte>>(&varFields[startIndex_]);
        if (values == nullptr)
        {
            varFields[startIndex_] = std::vector<std::byte>{};
            values = std::get_if<std::vector<std::byte>>(&varFields[startIndex_]);
        }

        if (values == nullptr)
        {
            throw std::runtime_error("SetFlatFieldArrayByte(): field storage type mismatch");
        }

        if (values->size() <= elementIndex_) { values->resize(elementIndex_ + 1); }
        (*values)[elementIndex_] = value_;
    }

    void SetFlatFieldArrayByte(const size_t startIndex_, const size_t elementIndex_, const uint8_t value_)
    {
        SetFlatFieldArrayByte(startIndex_, elementIndex_, static_cast<std::byte>(value_));
    }

    template <typename T>
    T GetFixedScalarValue(const size_t idx_) const
    {
        if (idx_ + sizeof(T) > fixedFields.size()) { throw std::runtime_error("GetFieldValue(): fixed field index out of range"); }

        T value{};
        std::memcpy(&value, fixedFields.data() + idx_, sizeof(value));
        return value;
    }

    template <typename T>
    std::vector<T> GetFixedArrayValue(const size_t idx_, const size_t count_) const
    {
        const size_t size = sizeof(T) * count_;
        if (idx_ + size > fixedFields.size()) { throw std::runtime_error("GetFieldValue(): fixed array index out of range"); }

        std::vector<T> values(count_);
        if (size > 0) { std::memcpy(values.data(), fixedFields.data() + idx_, size); }
        return values;
    }

    FieldValueVariant GetFixedFieldValue(const BaseField& field_) const
    {
        const size_t idx = field_.index;
        if (idx + field_.dataType.length > fixedFields.size()) { throw std::runtime_error("GetFieldValue(): index out of range"); }

        switch (field_.dataType.name)
        {
        case DATA_TYPE::BOOL:      return GetFixedScalarValue<bool>(idx);
        case DATA_TYPE::CHAR:      return GetFixedScalarValue<int8_t>(idx);
        case DATA_TYPE::HEXBYTE:   [[fallthrough]];
        case DATA_TYPE::UCHAR:     return GetFixedScalarValue<uint8_t>(idx);
        case DATA_TYPE::SHORT:     return GetFixedScalarValue<int16_t>(idx);
        case DATA_TYPE::USHORT:    return GetFixedScalarValue<uint16_t>(idx);
        case DATA_TYPE::INT:       [[fallthrough]];
        case DATA_TYPE::LONG:      return GetFixedScalarValue<int32_t>(idx);
        case DATA_TYPE::UINT:      [[fallthrough]];
        case DATA_TYPE::ULONG:     [[fallthrough]];
        case DATA_TYPE::SATELLITEID: return GetFixedScalarValue<uint32_t>(idx);
        case DATA_TYPE::LONGLONG:  return GetFixedScalarValue<int64_t>(idx);
        case DATA_TYPE::ULONGLONG: return GetFixedScalarValue<uint64_t>(idx);
        case DATA_TYPE::FLOAT:     return GetFixedScalarValue<float>(idx);
        case DATA_TYPE::DOUBLE:    return GetFixedScalarValue<double>(idx);
        default: throw std::runtime_error("GetFieldValue(): unknown DATA_TYPE");
        }
    }

    FieldValueVariant GetFieldValue(const BaseField& field_) const
    {
        switch (field_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
            if (arrayField == nullptr) { throw std::runtime_error("GetFieldValue(): missing fixed array metadata"); }

            const size_t idx = field_.index;
            const size_t size = static_cast<size_t>(arrayField->arrayLength) * field_.dataType.length;
            if (idx + size > fixedFields.size()) { throw std::runtime_error("GetFieldValue(): fixed array index out of range"); }

            switch (field_.dataType.name)
            {
            case DATA_TYPE::BOOL:      return GetFixedArrayValue<uint8_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::CHAR:      return GetFixedArrayValue<int8_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::HEXBYTE:   [[fallthrough]];
            case DATA_TYPE::UCHAR:     return GetFixedArrayValue<uint8_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::SHORT:     return GetFixedArrayValue<int16_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::USHORT:    return GetFixedArrayValue<uint16_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::INT:       [[fallthrough]];
            case DATA_TYPE::LONG:      return GetFixedArrayValue<int32_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::UINT:      [[fallthrough]];
            case DATA_TYPE::ULONG:     [[fallthrough]];
            case DATA_TYPE::SATELLITEID: return GetFixedArrayValue<uint32_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::LONGLONG:  return GetFixedArrayValue<int64_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::ULONGLONG: return GetFixedArrayValue<uint64_t>(idx, arrayField->arrayLength);
            case DATA_TYPE::FLOAT:     return GetFixedArrayValue<float>(idx, arrayField->arrayLength);
            case DATA_TYPE::DOUBLE:    return GetFixedArrayValue<double>(idx, arrayField->arrayLength);
            default: throw std::runtime_error("GetFieldValue(): unknown fixed array DATA_TYPE");
            }
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::FIELD_ARRAY: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR:
            if (field_.index >= varFields.size()) { throw std::runtime_error("GetFieldValue(): var field index out of range"); }

            return std::visit([](const auto& value) -> FieldValueVariant { return value; }, varFields[field_.index]);
        case FIELD_TYPE::ENUM:
            switch (field_.dataType.length)
            {
            case 1: return GetFixedScalarValue<uint8_t>(field_.index);
            case 2: return GetFixedScalarValue<uint16_t>(field_.index);
            case 4: return GetFixedScalarValue<uint32_t>(field_.index);
            default: throw std::runtime_error("GetFieldValue(): unsupported enum width");
            }
        default: return GetFixedFieldValue(field_);
        }
    }

    size_t GetFieldByteSize(const BaseField& field_) const
    {
        switch (field_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
            if (arrayField == nullptr) { throw std::runtime_error("GetFieldByteSize(): missing fixed array metadata"); }
            return static_cast<size_t>(arrayField->arrayLength) * field_.dataType.length;
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR:
            if (field_.index >= varFields.size()) { throw std::runtime_error("GetFieldByteSize(): var field index out of range"); }

            return std::visit(
                [](const auto& value) -> size_t {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, std::string>) { return value.size(); }
                    else if constexpr (std::is_same_v<T, std::vector<MessageBody>>)
                    {
                        throw std::runtime_error("GetFieldByteSize(): field arrays require typed access");
                    }
                    else if constexpr (std::is_arithmetic_v<T>)
                    {
                        throw std::runtime_error("GetFieldByteSize(): scalar values are not valid var field payloads");
                    }
                    else { return sizeof(typename T::value_type) * value.size(); }
                },
                varFields[field_.index]);
        case FIELD_TYPE::FIELD_ARRAY:
            throw std::runtime_error("GetFieldByteSize(): field arrays require typed access");
        default: return field_.dataType.length;
        }
    }

    size_t CopyFieldToBuffer(const BaseField& field_, std::byte* buffer_, const size_t capacity_) const
    {
        if (buffer_ == nullptr && capacity_ != 0) { throw std::runtime_error("CopyFieldToBuffer(): destination buffer is null"); }

        const auto copyBytes = [&](const std::byte* source_, const size_t size_) -> size_t {
            if (size_ > capacity_) { throw std::runtime_error("CopyFieldToBuffer(): destination buffer too small"); }
            if (size_ > 0) { std::memcpy(buffer_, source_, size_); }
            return size_;
        };

        switch (field_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto size = GetFieldByteSize(field_);
            if (field_.index + size > fixedFields.size()) { throw std::runtime_error("CopyFieldToBuffer(): fixed array index out of range"); }
            return copyBytes(fixedFields.data() + field_.index, size);
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR:
            if (field_.index >= varFields.size()) { throw std::runtime_error("CopyFieldToBuffer(): var field index out of range"); }

            return std::visit(
                [&](const auto& value) -> size_t {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, std::string>)
                    {
                        return copyBytes(reinterpret_cast<const std::byte*>(value.data()), value.size());
                    }
                    else if constexpr (std::is_same_v<T, std::vector<MessageBody>>)
                    {
                        throw std::runtime_error("CopyFieldToBuffer(): field arrays require typed access");
                    }
                    else if constexpr (std::is_arithmetic_v<T>)
                    {
                        throw std::runtime_error("CopyFieldToBuffer(): scalar values are not valid var field payloads");
                    }
                    else
                    {
                        return copyBytes(reinterpret_cast<const std::byte*>(value.data()), sizeof(typename T::value_type) * value.size());
                    }
                },
                varFields[field_.index]);
        case FIELD_TYPE::FIELD_ARRAY:
            throw std::runtime_error("CopyFieldToBuffer(): field arrays require typed access");
        default:
            if (field_.index + field_.dataType.length > fixedFields.size())
            {
                throw std::runtime_error("CopyFieldToBuffer(): fixed field index out of range");
            }
            return copyBytes(fixedFields.data() + field_.index, field_.dataType.length);
        }
    }

    size_t CopyFieldToBuffer(const BaseField& field_, unsigned char** buffer_, uint32_t& capacity_) const
    {
        if (buffer_ == nullptr || *buffer_ == nullptr)
        {
            if (capacity_ == 0) { return 0; }
            throw std::runtime_error("CopyFieldToBuffer(): destination buffer is null");
        }

        const auto written = CopyFieldToBuffer(field_, reinterpret_cast<std::byte*>(*buffer_), static_cast<size_t>(capacity_));
        *buffer_ += written;
        capacity_ -= static_cast<uint32_t>(written);
        return written;
    }

    // Non-breaking alias methods for API migration.
    template <bool Fixed = true, typename T>
    void WriteFieldData(const size_t startIndex_, const T* values_, const size_t n = 1)
    {
        CopyToBuffer<Fixed>(startIndex_, values_, n);
    }

    template <bool Fixed = true, typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    void WriteFieldData(const size_t startIndex_, const T& value_, const size_t n = 1)
    {
        CopyToBuffer<Fixed>(startIndex_, value_, n);
    }

    template <bool Fixed = true, typename T>
    void WriteFieldData(const size_t startIndex_, std::vector<T>&& values_)
    {
        CopyToBuffer<Fixed>(startIndex_, std::move(values_));
    }

    template <bool Fixed = true>
    void WriteFieldData(const size_t startIndex_, std::string&& value_)
    {
        CopyToBuffer<Fixed>(startIndex_, std::move(value_));
    }

    template <bool Fixed = true, typename T>
    void WriteFieldElement(const size_t startIndex_, const size_t elementIndex_, const T& value_)
    {
        SetFieldElement<Fixed>(startIndex_, elementIndex_, value_);
    }

    [[nodiscard]] FieldValueVariant ReadFieldValue(const BaseField& field_) const { return GetFieldValue(field_); }

    [[nodiscard]] size_t GetFieldSerializedSizeBytes(const BaseField& field_) const { return GetFieldByteSize(field_); }

    [[nodiscard]] size_t WriteFieldToBuffer(const BaseField& field_, unsigned char** buffer_, uint32_t& capacity_) const
    {
        return CopyFieldToBuffer(field_, buffer_, capacity_);
    }
};

struct DefinedMessageBody
{
    MessageBody body;
    MessageDefinition::ConstPtr definition;
};


//============================================================================
//! \class MessageDecoderBase
//! \brief Class to decode messages.
//============================================================================
class MessageDecoderBase
{
  private:
    static constexpr std::string_view svErrorPrefix = "ERROR:";

    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("message_decoder")};

    MessageDatabase::Ptr pclMyMsgDb{nullptr};

    EnumDefinition::ConstPtr vMyResponseDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    std::string sMyExpectedMessageFamily;

    // Enum util functions
    void InitEnumDefinitions();
    void InitFieldMaps();

  protected:
        std::unordered_map<uint32_t, std::function<void(MessageBody&, const BaseField::ConstPtr&, const char**, [[maybe_unused]] size_t,
                                [[maybe_unused]] size_t, [[maybe_unused]] bool, [[maybe_unused]] MessageDatabase&)>>
        asciiFieldMap;
        std::unordered_map<uint32_t,
               std::function<void(MessageBody&, const BaseField::ConstPtr&, simdjson::dom::element,
                      [[maybe_unused]] size_t, [[maybe_unused]] bool, [[maybe_unused]] MessageDatabase&)>>
            jsonFieldMap;

    [[nodiscard]] STATUS DecodeBinary(const FieldInfo& vMsgDefFields_,
                                      const unsigned char** ppucLogBuf_,
                                      MessageBody& vIntermediateFormat_, uint32_t uiMessageLength_) const;
    template <bool Abbreviated>
    [[nodiscard]] STATUS DecodeAscii(const FieldInfo& vMsgDefFields_,
                                     const char** ppcLogBuf_,
                                     MessageBody& vIntermediateFormat_, const char* pcBufEnd = nullptr) const;
    [[nodiscard]] STATUS DecodeJson(const FieldInfo& vMsgDefFields_,
                                    simdjson::dom::element jsonData, MessageBody& vIntermediateFormat_) const;

    template <bool Fixed = true>
    static void DecodeBinaryField(const BaseField::ConstPtr& pstMessageDataType_, const unsigned char** ppucLogBuf_,
                                  MessageBody& vIntermediateFormat_, size_t n = 1);
    void DecodeAsciiField(const BaseField::ConstPtr& pstMessageDataType_, const char** ppcToken_, size_t tokenLength_,
                          MessageBody& vIntermediateFormat_, size_t elementIndex_ = 0, bool fixed_ = true) const;
    void DecodeJsonField(const BaseField::ConstPtr& pstMessageDataType_, simdjson::dom::element clJsonField_,
                         MessageBody& vIntermediateFormat_, size_t elementIndex_ = 0, bool fixed_ = true) const;

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
    //! \return None. The buffer pointer is updated in place.
    //----------------------------------------------------------------------------
    virtual void AlignBufferPointer(const uint8_t align, const unsigned char* start, const unsigned char** ptr) const
    {
        uint8_t alignment = std::min(static_cast<uint8_t>(4), align);
        uint64_t offset = static_cast<uintptr_t>(*ptr - start) % alignment;
        if (offset != 0) { *ptr += alignment - offset; }
    }

    //----------------------------------------------------------------------------
    //! \brief Add padding after string fields if necessary to maintain alignment.
    //
    //! \param[in] start_ The starting pointer of the binary buffer. Used to
    //! calculate the current offset relative to the beginning of the payload.
    //! \param[in, out] ptr_ A pointer to the buffer pointer to be advanced
    //! if padding is needed.
    //
    //! \return None. The buffer pointer is updated in place.
    //----------------------------------------------------------------------------
    virtual void AddStringFieldPadding([[maybe_unused]] const unsigned char* start, [[maybe_unused]] const unsigned char** ptr) const { return; }

    // -------------------------------------------------------------------------------------------------------
    template <bool Fixed = true, typename T = int32_t, int R = 10>
    static void ParseAndEmplace(MessageBody& vIntermediateFormat_, size_t fieldIndex_, const char* token,
                                size_t tokenLength, size_t elementIndex_ = 0)
    {
        T value;
        std::from_chars_result result;

        // As of 6/26/2025 spaces may appear within ascii fields of OUTPUTDATUM and SETALIGNMENTVEL as they use conversion strings with width
        // values. EDIE supports decoding of this data but will never pad fields with spaces during encoding.
        uint32_t offset = 0;
        while ((token[offset] == ' ') && (offset < tokenLength - 1)) { ++offset; }

        if constexpr (std::is_integral_v<T>) { result = std::from_chars(token + offset, token + tokenLength, value, R); }
        else if constexpr (std::is_floating_point_v<T>) { result = std::from_chars(token + offset, token + tokenLength, value); }

        if (result.ec != std::errc()) { throw std::runtime_error("Failed to parse numeric value"); }

        vIntermediateFormat_.WriteFieldElement<Fixed>(fieldIndex_, elementIndex_, value);
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T, int R = 10>
    static std::function<void(MessageBody&, const BaseField::ConstPtr&, const char**, size_t, size_t, bool, MessageDatabase&)> SimpleAsciiMapEntry()
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Template argument must be integral or float");

        return [](MessageBody& vIntermediate_, const BaseField::ConstPtr& pstField_, const char** ppcToken_,
                  [[maybe_unused]] const size_t tokenLength_, const size_t elementIndex_, const bool fixed_,
                  [[maybe_unused]] MessageDatabase& pclMsgDb_) {
            if (fixed_) { ParseAndEmplace<true, T, R>(vIntermediate_, pstField_->index, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, T, R>(vIntermediate_, pstField_->index, *ppcToken_, tokenLength_, elementIndex_); }
        };
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static void PushElement(MessageBody& vIntermediate_, size_t fieldIndex_, simdjson::dom::element clJsonField_,
                            size_t elementIndex_ = 0, bool fixed_ = true)
    {
        // Determine the intermediate type to use for simdjson::get
        using IntermediateType =
            std::conditional_t<std::is_same_v<T, bool>, bool,                                                   // Handle bool directly
                               std::conditional_t<std::is_integral_v<T> && sizeof(T) <= sizeof(int64_t),        // For integers <= 64 bits
                                                  std::conditional_t<std::is_signed_v<T>, int64_t, uint64_t>,   // Use int64_t or uint64_t
                                                  std::conditional_t<std::is_floating_point_v<T>, double, void> // Use double for floating-point
                                                  >>;

        static_assert(!std::is_same_v<IntermediateType, void>, "Unsupported type for PushElement");

        IntermediateType intermediateValue;
        if (clJsonField_.get(intermediateValue) == simdjson::SUCCESS)
        {
            T value = static_cast<T>(intermediateValue);
            if (fixed_) { vIntermediate_.WriteFieldElement<true>(fieldIndex_, elementIndex_, value); }
            else { vIntermediate_.WriteFieldElement<false>(fieldIndex_, elementIndex_, value); }
        }
        else
        {
            // Handle error (e.g., log a warning or throw an exception)
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static std::function<void(MessageBody&, const BaseField::ConstPtr&, simdjson::dom::element, size_t, bool, MessageDatabase&)> SimpleJsonMapEntry()
    {
        return [](MessageBody& vIntermediate_, const BaseField::ConstPtr& pstMessageDataType_, simdjson::dom::element clJsonField_,
                  const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
            PushElement<T>(vIntermediate_, pstMessageDataType_->index, clJsonField_, elementIndex_, fixed_);
        };
    }

    // -------------------------------------------------------------------------------------------------------
    uint32_t GetArrayLength(const unsigned char* pucTempStart, const unsigned char** ppucLogBuf_, const ArrayField& arrayDef,
                            const MessageBody& vIntermediateFormat_,
                            const std::unordered_map<std::string, BaseField::ConstPtr>& messageBodyDef) const
    {
        if (arrayDef.arrayLengthRef.empty())
        {
            const std::size_t lenBytes = arrayDef.arrayLengthFieldSize;
            if (!(lenBytes == 1 || lenBytes == 2 || lenBytes == 4))
                throw std::runtime_error("GetArrayLength: Unsupported length size; must be 1,2 or 4");

            AlignBufferPointer(static_cast<uint8_t>(lenBytes), pucTempStart, ppucLogBuf_);

            uint32_t uiArrayLength = 0;
            for (std::size_t i = 0; i < lenBytes; ++i) { uiArrayLength |= static_cast<uint32_t>((*ppucLogBuf_)[i]) << (8 * i); }

            *ppucLogBuf_ += lenBytes;
            return uiArrayLength;
        }

        // Traverse the decoded fields to find the arrayLengthRef field by its name.
        auto it = messageBodyDef.find(arrayDef.arrayLengthRef);
        if (it != messageBodyDef.end())
        {
            const auto arraySize = std::visit(
                [](const auto& value) -> std::optional<uint32_t> {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_integral_v<T>)
                    {
                        if constexpr (std::is_signed_v<T>)
                        {
                            if (value < 0) { return std::nullopt; }
                        }
                        if constexpr (sizeof(T) > sizeof(uint32_t))
                        {
                            if (value > static_cast<T>(std::numeric_limits<uint32_t>::max())) { return std::nullopt; }
                        }
                        return static_cast<uint32_t>(value);
                    }
                    return std::nullopt;
                },
                vIntermediateFormat_.ReadFieldValue(*it->second));

            if (arraySize) { return *arraySize; }
        }

        throw std::runtime_error("GetArrayLength(): No matching field found for arrayLengthRef");
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDecoderBase class.
    //
    //! \param[in] expectedMessageFamily_ The expected message family for the encoder.
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    MessageDecoderBase(std::string expectedMessageFamily_, MessageDatabase::Ptr pclMessageDb_ = nullptr);

    virtual ~MessageDecoderBase() = default;

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    virtual void LoadJsonDb(MessageDatabase::Ptr pclMessageDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    // ---------------------------------------------------------------------------
    //! \brief Get the MessageDatabase object.
    //
    //! \return A shared pointer to the MessageDatabase object.
    // ---------------------------------------------------------------------------
    MessageDatabase::ConstPtr MessageDb() const { return std::const_pointer_cast<const MessageDatabase>(pclMyMsgDb); }

    //----------------------------------------------------------------------------
    //! \brief Decode a message payload from the provided frame.
    //
    //! \param[in] pucMessage_ A pointer to a message payload.
    //! \param[out] stInterMessage_ The MessageBody to be populated.
    //! \param[in, out] stMetaData_ MetaDataStruct to provide information about
    //! the frame and be fully populated to help describe the decoded log.
    //
    //! \remark Note, that pucMessage_ must not point to the message header,
    //! rather the message payload. This can be done by advancing the pointer
    //! of a message frame by stMetaData.uiHeaderLength.
    //! \return An error code describing the result of decoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: pucMessage_ is a null pointer.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   NO_DEFINITION: No definition for the current log was found in the
    //! provided database.
    //!   FAILURE: Failed to decode a header field.
    //!   UNSUPPORTED: Attempted to decode an unsupported format.
    //!   UNKNOWN: The header format provided is not known.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Decode(const unsigned char* pucMessage_, DefinedMessageBody& stInterMessage_, MetaDataBase& stMetaData_) const;
};

} // namespace novatel::edie

#endif // MESSAGE_DECODER_HPP
