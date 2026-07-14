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

template <typename T, template <typename...> class Template> struct is_specialization_of : std::false_type
{
};

template <template <typename...> class Template, typename... Args> struct is_specialization_of<Template<Args...>, Template> : std::true_type
{
};

template <typename T, template <typename...> class Template> inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

class MessageBody;
using NestedFieldArray = std::vector<MessageBody>;

// ---------------------------------------------------------------------------
//! \class TypedBuffer
//! \brief A lightweight wrapper around a byte buffer that allows for typed access to its elements.
//!     Used for fixed-length array fields in the message body.
// ---------------------------------------------------------------------------
template <typename T> class TypedBuffer
{
  private:
    const std::byte* data;
    size_t sz;

  public:
    TypedBuffer(const std::byte* data_, size_t sz_) : data(data_), sz(sz_) {}

    T operator[](size_t index) const
    {
        if (index >= sz) { throw std::runtime_error("TypedBuffer<T>::operator[](): index out of bounds"); }
        return LoadValueFromBuffer<T>(data + (index * sizeof(T)));
    }

    constexpr size_t size() const { return sz; }

    constexpr bool empty() const { return sz == 0; }

    class const_iterator
    {
      public:
        using value_type = T;
        using reference = T;
        using iterator_category = std::forward_iterator_tag;

      private:
        const std::byte* data;
        size_t index;

      public:
        const_iterator() : data(nullptr), index(0) {}
        const_iterator(const std::byte* data_, size_t index_) : data(data_), index(index_) {}

        reference operator*() const { return LoadValueFromBuffer<T>(data + (index * sizeof(T))); }

        const_iterator& operator++()
        {
            ++index;
            return *this;
        }

        const_iterator operator++(int)
        {
            auto tmp = *this;
            ++index;
            return tmp;
        }

        bool operator==(const const_iterator& other) const { return data == other.data && index == other.index; }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }
    };

    const_iterator begin() const { return const_iterator(data, 0); }
    const_iterator end() const { return const_iterator(data, sz); }
};

template <typename Fn> inline void SimpleTypeVisitor(const BaseField& fd_, Fn&& visitor)
{
    if (fd_.type == FIELD_TYPE::ENUM)
    {
        switch (fd_.dataType.length)
        {
        case 1: visitor(int8_t{}); break;
        case 2: visitor(int16_t{}); break;
        case 4: visitor(int32_t{}); break;
        default: throw std::runtime_error("SimpleTypeVisitor(): unsupported enum width");
        }
        return;
    }

    switch (fd_.dataType.name)
    {
    case DATA_TYPE::BOOL: visitor(bool{}); break;
    case DATA_TYPE::CHAR: visitor(int8_t{}); break;
    case DATA_TYPE::HEXBYTE: [[fallthrough]];
    case DATA_TYPE::UCHAR: visitor(uint8_t{}); break;
    case DATA_TYPE::SHORT: visitor(int16_t{}); break;
    case DATA_TYPE::USHORT: visitor(uint16_t{}); break;
    case DATA_TYPE::LONG: [[fallthrough]];
    case DATA_TYPE::INT: visitor(int32_t{}); break;
    case DATA_TYPE::SATELLITEID: [[fallthrough]];
    case DATA_TYPE::ULONG: [[fallthrough]];
    case DATA_TYPE::UINT: visitor(uint32_t{}); break;
    case DATA_TYPE::LONGLONG: visitor(int64_t{}); break;
    case DATA_TYPE::ULONGLONG: visitor(uint64_t{}); break;
    case DATA_TYPE::FLOAT: visitor(float{}); break;
    case DATA_TYPE::DOUBLE: visitor(double{}); break;
    default: throw std::runtime_error("SimpleTypeVisitor(): unknown or unsupported data type");
    }
}

template <typename T> inline T CheckAndLoadType(const BaseField& fd_, const std::byte* fieldPtr_)
{
    SimpleTypeVisitor(fd_, [&](auto&& arg) {
        if constexpr (!std::is_same_v<T, std::decay_t<decltype(arg)>>)
        {
            throw std::runtime_error("GetFieldValue<T>(): type T does not match field data type");
        }
    });
    return LoadValueFromBuffer<T>(fieldPtr_);
}

// ---------------------------------------------------------------------------
//! \class FixedFieldRegion
//! \brief A byte vector for storing fixed-size fields.
// ---------------------------------------------------------------------------
class FixedFieldRegion
{
  private:
    std::vector<std::byte> byteRegion;

  public:
    FixedFieldRegion() = default;

    FixedFieldRegion(std::vector<std::byte>&& data_) : byteRegion(std::move(data_)) {}

    FixedFieldRegion(size_t sz_) : byteRegion(sz_) {}

    // ---------------------------------------------------------------------------
    //! \brief Resize the FixedFieldRegion.
    //!
    //! \param[in] sz_ The new size of the fixed field region in bytes.
    // ---------------------------------------------------------------------------
    void resize(size_t sz_) { byteRegion.resize(sz_); }

    size_t size() const { return byteRegion.size(); }

    const std::byte* data() const { return byteRegion.data(); }

    void Resize(size_t sz_) { byteRegion.resize(sz_); }

    template <typename T> T GetFieldValue(const BaseField& field_, size_t baseIndex_ = 0) const
    {
        static_assert(std::is_trivially_copyable_v<T> || is_specialization_of_v<T, TypedBuffer>,
                      "GetFieldValue only supports trivially copyable types or TypedBuffer specializations");
        switch (field_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            if constexpr (is_specialization_of_v<T, TypedBuffer>)
            {
                const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
                if (arrayField == nullptr) { throw std::runtime_error("GetFieldValue<T>(): missing fixed array metadata"); }
                SimpleTypeVisitor(field_, [&](auto&& arg) {
                    if constexpr (!std::is_same_v<T, TypedBuffer<std::decay_t<decltype(arg)>>>)
                    {
                        throw std::runtime_error("GetFieldValue<T>(): type T does not match field data type");
                    }
                });
                return T{byteRegion.data() + baseIndex_ + field_.index, arrayField->arrayLength};
            }
            else { throw std::runtime_error("GetFieldValue<T>(): T must be TypedBuffer<E> for FIXED_LENGTH_ARRAY"); }
        }
        case FIELD_TYPE::RESPONSE_ID: [[fallthrough]];
        case FIELD_TYPE::ENUM: [[fallthrough]];
        case FIELD_TYPE::SIMPLE:
            if constexpr (is_specialization_of_v<T, TypedBuffer>)
            {
                throw std::runtime_error("GetFieldValue<T>(): T must not be TypedBuffer<E> for SIMPLE or ENUM fields");
            }
            else { return CheckAndLoadType<T>(field_, byteRegion.data() + baseIndex_ + field_.index); }
        default: throw std::runtime_error("GetFieldValue<T>(): unsupported field type for GetFieldValue in FixedFieldRegion");
        }
    }

    template <typename T> T GetFieldValue(size_t elemIndex_, const ArrayField& field_, size_t baseIndex_ = 0) const
    {
        static_assert(std::is_trivially_copyable_v<T>, "GetFieldValue only supports trivially copyable types");
        if (elemIndex_ >= field_.arrayLength) { throw std::runtime_error("GetFieldValue<T>(): index out of bounds for fixed-length array field"); }
        if (field_.type != FIELD_TYPE::FIXED_LENGTH_ARRAY)
        {
            throw std::runtime_error("GetFieldValue<T>(): this overload is only for fixed-length array fields");
        }

        if constexpr (std::is_same_v<T, bool>)
        {
            return static_cast<bool>(CheckAndLoadType<uint8_t>(field_, byteRegion.data() + baseIndex_ + field_.index + elemIndex_));
        }
        else { return CheckAndLoadType<T>(field_, byteRegion.data() + baseIndex_ + field_.index + (elemIndex_ * sizeof(T))); }
    }

    template <typename T> T GetFieldValue(size_t elemIndex_, const BaseField& field_, size_t baseIndex_ = 0) const
    {
        static_assert(std::is_trivially_copyable_v<T>, "GetFieldValue only supports trivially copyable types");
        if (field_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
        {
            const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
            if (arrayField == nullptr) { throw std::runtime_error("GetFieldValue<T>(): missing fixed array metadata"); }
            return GetFieldValue<T>(elemIndex_, *arrayField, baseIndex_);
        }

        if (elemIndex_ == 0) { return GetFieldValue<T>(field_, baseIndex_); }
        throw std::runtime_error("GetFieldValue<T>(): field must be an array type or element index must be zero");
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values at the specified index.
    //!
    //! \tparam T The value type (must be trivially copyable).
    //! \param[in] startIndex_ The starting index in the byte region.
    //! \param[in] values_ Pointer to values to set.
    //! \param[in] n Number of values to copy (defaults to 1).
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const size_t startIndex_, const T* values_, size_t n = 1)
    {
        static_assert(std::is_trivially_copyable_v<T>, "SetFieldValue only supports trivially copyable types");
        if (startIndex_ + (n * sizeof(T)) > byteRegion.size()) { throw std::runtime_error("SetFieldValue(): buffer overflow in FixedFieldRegion"); }
        std::memcpy(byteRegion.data() + startIndex_, values_, n * sizeof(T));
    }

    // ---------------------------------------------------------------------------
    //! \brief Set a single field value at the specified index.
    //!
    //! \tparam T The value type (must not be a pointer).
    //! \param[in] startIndex_ The index in the byte region.
    //! \param[in] value_ The value to set.
    //! \param[in] n Number of values to copy (defaults to 1).
    // ---------------------------------------------------------------------------
    template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    void SetFieldValue(const size_t startIndex_, const T& value_, size_t n = 1)
    {
        SetFieldValue(startIndex_, &value_, n);
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a vector.
    //!
    //! \tparam T The element type (must be trivially copyable and not bool).
    //! \param[in] startIndex_ The index in the byte region.
    //! \param[in] values_ Rvalue reference to vector of values to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const size_t startIndex_, std::vector<T>&& values_)
    {
        static_assert(std::is_trivially_copyable_v<T>, "SetFieldValue only supports trivially copyable vector element types");
        static_assert(!std::is_same_v<T, bool>, "SetFieldValue does not support std::vector<bool>");

        if (startIndex_ + (values_.size() * sizeof(T)) > byteRegion.size())
        {
            throw std::runtime_error("SetFieldValue(): buffer overflow in FixedFieldRegion");
        }
        std::memcpy(byteRegion.data() + startIndex_, values_.data(), values_.size() * sizeof(T));
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a string.
    //!
    //! \param[in] startIndex_ The index in the byte region.
    //! \param[in] value_ Rvalue reference to string to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    void SetFieldValue(const size_t startIndex_, std::string&& value_)
    {
        if (startIndex_ + value_.size() > byteRegion.size()) { throw std::runtime_error("SetFieldValue(): buffer overflow in FixedFieldRegion"); }
        std::memcpy(byteRegion.data() + startIndex_, value_.data(), value_.size());
    }
};

// ---------------------------------------------------------------------------
//! \class FlatFieldArray
//! \brief A field array containing only fixed-length fields.
// ---------------------------------------------------------------------------
class FlatFieldArray
{
  private:
    FixedFieldRegion fields;
    FieldInfo::ConstPtr fieldInfo;

  public:
    FlatFieldArray(std::vector<std::byte>&& data_, FieldInfo::ConstPtr fieldInfo_) : fields(std::move(data_)), fieldInfo(std::move(fieldInfo_)) {}

    FlatFieldArray(size_t sz_, FieldInfo::ConstPtr fieldInfo_) : fields(sz_), fieldInfo(std::move(fieldInfo_))
    {
        if (fieldInfo != nullptr && fieldInfo->varFieldCount > 0)
        {
            throw std::runtime_error("FlatFieldArray(): fieldInfo must not have variable fields");
        }
    }

    size_t size() const { return fields.size() / fieldInfo->fixedFieldBytes; }

    size_t ByteSize() const { return fields.size(); }

    const std::byte* data() const { return fields.data(); }

    // ---------------------------------------------------------------------------
    //! \brief Resize the FlatFieldArray.
    //!
    //! \param[in] sz_ The new size of the fixed field region in bytes.
    // ---------------------------------------------------------------------------
    void Resize(size_t sz_) { fields.Resize(sz_); }

    template <typename T> T GetFieldValue(const BaseField& field_, size_t index_) const
    {
        return fields.GetFieldValue<T>(field_, index_ * fieldInfo->fixedFieldBytes);
    }

    // ---------------------------------------------------------------------------
    //! \brief Set the field info used by this FlatFieldArray.
    //!
    //! \param[in] fieldInfo_ The field info pointer.
    // ---------------------------------------------------------------------------
    void SetFieldInfo(FieldInfo::ConstPtr fieldInfo_)
    {
        fieldInfo = std::move(fieldInfo_);
        if (fieldInfo) { Resize(fieldInfo->fixedFieldBytes); }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values at the specified index.
    //!
    //! \tparam T The value type (must be trivially copyable).
    //! \param[in] fieldIndex_ The index of the field.
    //! \param[in] startIndex_ The index in the field's byte region.
    //! \param[in] values_ Pointer to values to set.
    //! \param[in] n Number of values to copy (defaults to 1).
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const size_t fieldIndex_, const size_t startIndex_, const T* values_, size_t n = 1)
    {
        fields.SetFieldValue((fieldIndex_ * fieldInfo->fixedFieldBytes) + startIndex_, values_, n);
    }

    // ---------------------------------------------------------------------------
    //! \brief Set a single field value at the specified index.
    //!
    //! \tparam T The value type (must not be a pointer).
    //! \param[in] fieldIndex_ The index of the field.
    //! \param[in] startIndex_ The index in the field's byte region.
    //! \param[in] value_ The value to set.
    //! \param[in] n Number of values to copy (defaults to 1).
    // ---------------------------------------------------------------------------
    template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    void SetFieldValue(const size_t fieldIndex_, const size_t startIndex_, const T& value_, size_t n = 1)
    {
        fields.SetFieldValue((fieldIndex_ * fieldInfo->fixedFieldBytes) + startIndex_, &value_, n);
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a vector.
    //!
    //! \tparam T The element type (must be trivially copyable and not bool).
    //! \param[in] fieldIndex_ The index of the field.
    //! \param[in] startIndex_ The index in the field's byte region.
    //! \param[in] values_ Rvalue reference to vector of values to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const size_t fieldIndex_, const size_t startIndex_, std::vector<T>&& values_)
    {
        fields.SetFieldValue((fieldIndex_ * fieldInfo->fixedFieldBytes) + startIndex_, std::move(values_));
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a string.
    //!
    //! \param[in] fieldIndex_ The index of the field.
    //! \param[in] startIndex_ The index in the field's byte region.
    //! \param[in] value_ Rvalue reference to string to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    void SetFieldValue(const size_t fieldIndex_, const size_t startIndex_, std::string&& value_)
    {
        fields.SetFieldValue((fieldIndex_ * fieldInfo->fixedFieldBytes) + startIndex_, std::move(value_));
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a string.
    //!
    //! \tparam T The value type (must be trivially copyable).
    //! \param[in] fieldIndex_ The index of the field.
    //! \param[in] fd_ The BaseField definition of the field.
    //! \param[in] value_ Rvalue reference to value to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const size_t fieldIndex_, const BaseField& fd_, T&& value_)
    {
        fields.SetFieldValue((fieldIndex_ * fieldInfo->fixedFieldBytes) + fd_.index, std::forward<T>(value_));
    }
};

using FieldValueVariant =
    std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, TypedBuffer<bool>,
                 TypedBuffer<int8_t>, TypedBuffer<uint8_t>, TypedBuffer<int16_t>, TypedBuffer<uint16_t>, TypedBuffer<int32_t>, TypedBuffer<uint32_t>,
                 TypedBuffer<int64_t>, TypedBuffer<uint64_t>, TypedBuffer<float>, TypedBuffer<double>, std::vector<int8_t>, std::vector<int16_t>,
                 std::vector<int32_t>, std::vector<int64_t>, std::vector<uint8_t>, std::vector<uint16_t>, std::vector<uint32_t>,
                 std::vector<uint64_t>, std::vector<float>, std::vector<double>, std::string, FlatFieldArray, NestedFieldArray>;

inline FieldValueVariant LoadVariant(const BaseField& fd_, const std::byte* fieldPtr_)
{
    FieldValueVariant result;
    SimpleTypeVisitor(fd_, [&](auto&& arg) { result = LoadValueFromBuffer<std::decay_t<decltype(arg)>>(fieldPtr_); });
    return result;
}

inline FieldValueVariant LoadVariant(const ArrayField& fd_, const std::byte* fieldPtr_)
{
    FieldValueVariant result;
    SimpleTypeVisitor(fd_, [&](auto&& arg) { result = TypedBuffer<std::decay_t<decltype(arg)>>{fieldPtr_, fd_.arrayLength}; });
    return result;
}

// ---------------------------------------------------------------------------
//! \class MessageBody
//! \brief An object representing the body of a message, containing both
//!     fixed-length and variable-length fields.
// ---------------------------------------------------------------------------
class MessageBody
{
  private:
    FixedFieldRegion fixedFields;
    std::vector<FieldValueVariant> varFields;
    FieldInfo::ConstPtr fieldInfo;

  public:
    // ---------------------------------------------------------------------------
    //! \brief Default constructor.
    // ---------------------------------------------------------------------------
    MessageBody() = default;

    // ---------------------------------------------------------------------------
    //! \brief Construct a MessageBody with pre-allocated fixed and variable field storage.
    //!
    //! \param[in] fixedFieldSize_ The size in bytes for the fixed fields buffer.
    //! \param[in] varFieldCount_ The number of variable field slots to allocate.
    // ---------------------------------------------------------------------------
    MessageBody(size_t fixedFieldSize_, size_t varFieldCount_) : fixedFields(fixedFieldSize_), varFields(varFieldCount_) {}

    MessageBody(const MessageBody& other) : fixedFields(other.fixedFields), varFields(other.varFields), fieldInfo(other.fieldInfo) {}

    MessageBody& operator=(const MessageBody& other) = default;

    MessageBody(FixedFieldRegion&& fixedFields_, std::vector<FieldValueVariant>&& varFields_)
        : fixedFields(std::move(fixedFields_)), varFields(std::move(varFields_))
    {
    }

    MessageBody(FieldInfo::ConstPtr fieldInfo_) : fieldInfo(std::move(fieldInfo_))
    {
        if (fieldInfo)
        {
            fixedFields.resize(fieldInfo->fixedFieldBytes);
            varFields.resize(fieldInfo->varFieldCount);

            for (const auto& fieldDef : fieldInfo->messageOrderedFields)
            {
                switch (fieldDef->type)
                {
                case FIELD_TYPE::FIELD_ARRAY: {
                    varFields[fieldDef->index] = NestedFieldArray{};
                    break;
                }
                case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
                    const auto* arrFieldDef = dynamic_cast<const ArrayField*>(fieldDef.get());
                    const size_t arrayLength = arrFieldDef ? arrFieldDef->arrayLength : 0;
                    for (size_t i = 0; i < arrayLength; i++)
                    {
                        SimpleTypeVisitor(*arrFieldDef, [&](auto&& arg) {
                            using ValueT = std::decay_t<decltype(arg)>;
                            SetArrayElement<true>(*arrFieldDef, i, ValueT{});
                        });
                    }
                    break;
                }
                case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
                    SimpleTypeVisitor(*fieldDef, [&](auto&& arg) {
                        using ValueT = std::conditional_t<std::is_same_v<std::decay_t<decltype(arg)>, bool>, uint8_t, std::decay_t<decltype(arg)>>;
                        varFields[fieldDef->index] = std::vector<ValueT>{};
                    });
                    break;
                case FIELD_TYPE::STRING: varFields[fieldDef->index] = std::string{}; break;
                case FIELD_TYPE::ENUM:
                    switch (fieldDef->dataType.length)
                    {
                    case 1: SetFieldValue<true>(fieldDef->index, int8_t{}); break;
                    case 2: SetFieldValue<true>(fieldDef->index, int16_t{}); break;
                    case 4: SetFieldValue<true>(fieldDef->index, int32_t{}); break;
                    default: throw std::runtime_error("MessageBody(): unsupported enum width");
                    }
                    break;
                default:
                    SimpleTypeVisitor(*fieldDef, [&](auto&& arg) {
                        using ValueT = std::decay_t<decltype(arg)>;
                        SetFieldValue<true>(fieldDef->index, ValueT{});
                    });
                    break;
                }
            }
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Resize the memory regions of the MessageBody.
    //!
    //! \param[in] fixedFieldSize_ The new size in bytes for the fixed fields
    //! \param[in] varFieldCount_ The new number of variable field slots to allocate
    // ---------------------------------------------------------------------------
    void Resize(size_t fixedFieldSize_, size_t varFieldCount_)
    {
        fixedFields.resize(fixedFieldSize_);
        varFields.resize(varFieldCount_);
    }

    // ---------------------------------------------------------------------------
    //! Getters for member variables
    // ---------------------------------------------------------------------------
    FixedFieldRegion& GetFixedFields() { return fixedFields; }
    const FixedFieldRegion& GetFixedFields() const { return fixedFields; }
    std::vector<FieldValueVariant>& GetVarFields() { return varFields; }
    const std::vector<FieldValueVariant>& GetVarFields() const { return varFields; }
    const FieldInfo::ConstPtr& GetFieldInfo() const { return fieldInfo; }

    // ---------------------------------------------------------------------------
    //! \brief Get a field value as a specific type.
    //!
    //! \tparam T The type to extract from the field value variant.
    //! \param[in] field_ The definition of the field to retrieve.
    //! \return The field value as T.
    // ---------------------------------------------------------------------------
    template <typename T> T GetFieldValue(const BaseField& field_) const
    {
        switch (field_.type)
        {
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
            if constexpr (is_specialization_of_v<T, std::vector>) { return std::get<T>(varFields[field_.index]); }
            else { throw std::runtime_error("GetFieldValue<T>(): incorrect type given for VARIABLE_LENGTH_ARRAY"); }
        case FIELD_TYPE::RESPONSE_STR: [[fallthrough]]; // TODO: is RESPONSE_STR always std::string?
        case FIELD_TYPE::STRING:
            if constexpr (std::is_same_v<T, std::string>) { return std::get<std::string>(varFields[field_.index]); }
            else { throw std::runtime_error("GetFieldValue<T>(): non-string type provided for string field"); }
        case FIELD_TYPE::FIELD_ARRAY:
            if constexpr (std::is_same_v<T, FlatFieldArray> || std::is_same_v<T, NestedFieldArray>) { return std::get<T>(varFields[field_.index]); }
            else { throw std::runtime_error("GetFieldValue<T>(): incorrect type given for FIELD_ARRAY"); }
        default:
            if constexpr (std::is_trivially_copyable_v<T> || is_specialization_of_v<T, TypedBuffer>) { return fixedFields.GetFieldValue<T>(field_); }
            else { throw std::runtime_error("GetFieldValue<T>(): type T must be trivially copyable or TypedBuffer specialization"); }
        }
    }

    template <typename T> T GetFieldValue(size_t index_, const BaseField& field_) const
    {
        if (field_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY) { return fixedFields.GetFieldValue<T>(index_, field_); }
        if (field_.type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                const auto& vec = std::get<std::vector<uint8_t>>(varFields[field_.index]);
                if (index_ >= vec.size()) { throw std::runtime_error("GetFieldValue<T>(): index out of bounds for variable-length array field"); }
                return static_cast<bool>(vec[index_]);
            }
            else
            {
                const auto& vec = std::get<std::vector<T>>(varFields[field_.index]);
                if (index_ >= vec.size()) { throw std::runtime_error("GetFieldValue<T>(): index out of bounds for variable-length array field"); }
                return vec[index_];
            }
        }

        if (index_ == 0) { return GetFieldValue<T>(field_); }
        throw std::runtime_error("GetFieldValue<T>(): field must be an array type or index must be zero");
    }

    template <typename T> T GetFieldValue(size_t index_, const ArrayField& field_) const
    {
        if (field_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY) { return fixedFields.GetFieldValue<T>(index_, field_); }
        return GetFieldValue<T>(index_, static_cast<const BaseField&>(field_));
    }

    // ---------------------------------------------------------------------------
    //! \brief Get a field value by field definition.
    //!
    //! \param[in] field_ The definition of the field to retrieve.
    //! \return A FieldValueVariant containing the field value.
    // ---------------------------------------------------------------------------
    FieldValueVariant GetFieldValueVariant(const BaseField& field_) const
    {
        switch (field_.type)
        {
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::FIELD_ARRAY: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR: return varFields[field_.index];
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
            if (arrayField == nullptr) { throw std::runtime_error("GetFieldValueVariant(): missing fixed array metadata"); }
            return LoadVariant(*arrayField, fixedFields.data() + field_.index);
        }
        default: return LoadVariant(field_, fixedFields.data() + field_.index);
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Get an array field value by definition. Avoids the dynamic cast in
    //!     GetFieldValue(const BaseField&).
    //!
    //! \param[in] field_ The definition of the ArrayField to retrieve.
    //! \return A FieldValueVariant containing the field value.
    //! \see GetFieldValue(const BaseField&) for non-array fields.
    // ---------------------------------------------------------------------------
    FieldValueVariant GetFieldValueVariant(const ArrayField& field_) const
    {
        if (field_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY) { return LoadVariant(field_, fixedFields.data() + field_.index); }
        return GetFieldValueVariant(static_cast<const BaseField&>(field_));
    }

    // ---------------------------------------------------------------------------
    //! \brief Get a field value by field name.
    //!
    //! \param[in, out] val_ A reference to a FieldValueVariant to store the retrieved value.
    //! \param[in] fieldName_ The name of the field to retrieve.
    //! \return true if the field was found and val_ was set, false otherwise.
    // ---------------------------------------------------------------------------
    bool GetFieldValueVariant(FieldValueVariant& val_, const std::string& fieldName_) const
    {
        if (fieldInfo == nullptr) { throw std::runtime_error("GetFieldValue(): message definition not set"); }
        const auto& fields = fieldInfo->messageOrderedFields;
        const auto it =
            std::find_if(fields.begin(), fields.end(), [&fieldName_](const BaseField::ConstPtr& fieldDef) { return fieldDef->name == fieldName_; });
        if (it == fields.end()) { return false; }
        val_ = GetFieldValueVariant(**it);
        return true;
    }

    // ---------------------------------------------------------------------------
    //! \brief Get a field value by field name.
    //!
    //! \param[in] fieldName_ The name of the field to retrieve.
    //! \return A FieldValueVariant containing the field value.
    //! \throws std::runtime_error if the field name is not found in the message
    //!     definition or the definition is not set.
    // ---------------------------------------------------------------------------
    FieldValueVariant GetFieldValueVariant(const std::string& fieldName_) const
    {
        FieldValueVariant val;
        if (!GetFieldValueVariant(val, fieldName_)) { throw std::runtime_error("GetFieldValue(): field name not found in message definition"); }
        return val;
    }

    // ---------------------------------------------------------------------------
    //! \brief Get the byte size of a field.
    //!
    //! \param[in] field_ The field definition.
    //! \return The size in bytes of the field's data.
    //! \throws std::runtime_error on invalid index or type mismatch.
    // ---------------------------------------------------------------------------
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
        case FIELD_TYPE::FIELD_ARRAY: [[fallthrough]];
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR:
            if (field_.index >= varFields.size()) { throw std::runtime_error("GetFieldByteSize(): var field index out of range"); }

            return std::visit(
                [&field_](const auto& value) -> size_t {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<std::byte>>) { return value.size(); }
                    else if constexpr (std::is_same_v<T, FlatFieldArray>) { return value.ByteSize(); }
                    else if constexpr (std::is_same_v<T, std::vector<MessageBody>>)
                    {
                        const auto* fieldArrayField = dynamic_cast<const FieldArrayField*>(&field_);
                        if (fieldArrayField == nullptr) { throw std::runtime_error("GetFieldByteSize(): missing field array metadata"); }
                        if (fieldArrayField->fieldInfo->varFieldCount == 0) { return fieldArrayField->fieldInfo->fixedFieldBytes * value.size(); }
                        size_t byteSize = 0;
                        for (const auto& messageBody : value)
                        {
                            for (const auto& field : fieldArrayField->fieldInfo->messageOrderedFields)
                            {
                                byteSize += messageBody.GetFieldByteSize(*field);
                            }
                        }
                        return byteSize;
                    }
                    else if constexpr (std::is_arithmetic_v<T>)
                    {
                        throw std::runtime_error("GetFieldByteSize(): scalar values are not valid var field payloads");
                    }
                    else if constexpr (is_specialization_of_v<T, std::vector>) { return sizeof(typename T::value_type) * value.size(); }
                    else { throw std::runtime_error("GetFieldByteSize(): unsupported var field payload type"); }
                },
                varFields[field_.index]);
        default: return field_.dataType.length;
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Get the number of subfields in a field.
    //!
    //! \param[in] field_ The field definition.
    //! \return The number of subfields in the field.
    //! \throws std::runtime_error on invalid index or type mismatch.
    // ---------------------------------------------------------------------------
    size_t GetFieldSize(const BaseField& field_) const
    {
        switch (field_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
            if (arrayField == nullptr) { throw std::runtime_error("GetFieldSize(): missing fixed array metadata"); }
            return static_cast<size_t>(arrayField->arrayLength);
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::FIELD_ARRAY: [[fallthrough]];
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR:
            if (field_.index >= varFields.size()) { throw std::runtime_error("GetFieldSize(): var field index out of range"); }

            return std::visit(
                [](const auto& value) -> size_t {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, FlatFieldArray> || is_specialization_of_v<T, std::vector>)
                    {
                        return value.size();
                    }
                    else { throw std::runtime_error("GetFieldSize(): scalar values are not valid var field payloads"); }
                },
                varFields[field_.index]);
        default: return 1;
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set the field info used by this message body.
    //!
    //! \param[in] fieldInfo_ The field info pointer.
    // ---------------------------------------------------------------------------
    void SetFieldInfo(FieldInfo::ConstPtr fieldInfo_)
    {
        fieldInfo = std::move(fieldInfo_);
        if (fieldInfo) { Resize(fieldInfo->fixedFieldBytes, fieldInfo->varFieldCount); }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set the field info from a message definition and optional CRC.
    //!
    //! \param[in] definition_ The message definition pointer.
    //! \param[in] defCrc_ Optional CRC of the definition. Defaults to nullopt.
    // ---------------------------------------------------------------------------
    void SetFieldInfo(const MessageDefinition::ConstPtr& definition_, const std::optional<uint32_t>& defCrc_ = std::nullopt)
    {
        if (definition_ == nullptr)
        {
            fieldInfo.reset();
            return;
        }

        if (defCrc_.has_value())
        {
            const auto it = definition_->fieldInfo.find(defCrc_.value());
            if (it != definition_->fieldInfo.end())
            {
                SetFieldInfo(it->second);
                return;
            }
        }

        const auto it = definition_->fieldInfo.find(definition_->latestMessageCrc);
        SetFieldInfo(it != definition_->fieldInfo.end() ? it->second : nullptr);
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values at the specified index.
    //!
    //! \tparam Fixed True for fixed fields, false for variable-length fields.
    //! \tparam T The value type (must be trivially copyable).
    //! \param[in] startIndex_ The starting index in the target field storage (byte
    //!     offset for fixed fields, element index for variable-length fields).
    //! \param[in] values_ Pointer to values to set.
    //! \param[in] n Number of values to copy (defaults to 1).
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <bool Fixed = true, typename T> void SetFieldValue(const size_t startIndex_, const T* values_, size_t n = 1)
    {
        static_assert(std::is_trivially_copyable_v<T>, "SetFieldValue only supports trivially copyable types");

        if (values_ == nullptr) { throw std::runtime_error("SetFieldValue(): source pointer is null"); }

        if constexpr (Fixed) { fixedFields.SetFieldValue(startIndex_, values_, n); }
        else
        {
            if (startIndex_ >= varFields.size()) { throw std::runtime_error("SetFieldValue(): varFields index is out of range"); }

            using BufferElementType = std::conditional_t<std::is_same_v<T, bool>, uint8_t, T>;
            std::vector<BufferElementType> values(n);
            std::memcpy(values.data(), values_, n * sizeof(BufferElementType));
            varFields[startIndex_] = std::move(values);
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set a single field value at the specified index.
    //!
    //! \tparam Fixed True for fixed fields, false for variable-length fields.
    //! \tparam T The value type (must not be a pointer).
    //! \param[in] startIndex_ The index in the target field storage.
    //! \param[in] value_ The value to set.
    //! \param[in] n Number of values to copy (defaults to 1).
    // ---------------------------------------------------------------------------
    template <bool Fixed = true, typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    void SetFieldValue(const size_t startIndex_, const T& value_, size_t n = 1)
    {
        SetFieldValue<Fixed>(startIndex_, &value_, n);
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a vector.
    //!
    //! \tparam Fixed True for fixed fields, false for variable-length fields.
    //! \tparam T The element type (must be trivially copyable and not bool).
    //! \param[in] startIndex_ The index in the target field storage.
    //! \param[in] values_ Rvalue reference to vector of values to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <bool Fixed = true, typename T> void SetFieldValue(const size_t startIndex_, std::vector<T>&& values_)
    {
        static_assert(std::is_trivially_copyable_v<T>, "SetFieldValue only supports trivially copyable vector element types");
        static_assert(!std::is_same_v<T, bool>, "SetFieldValue does not support std::vector<bool>");

        if constexpr (Fixed) { fixedFields.SetFieldValue(startIndex_, std::move(values_)); }
        else
        {
            if (startIndex_ >= varFields.size()) { throw std::runtime_error("SetFieldValue(): varFields index is out of range"); }

            varFields[startIndex_] = std::move(values_);
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a string.
    //!
    //! \tparam Fixed True for fixed fields, false for variable-length fields.
    //! \param[in] startIndex_ The index in the target field storage.
    //! \param[in] value_ Rvalue reference to string to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <bool Fixed = true> void SetFieldValue(const size_t startIndex_, std::string&& value_)
    {
        if constexpr (Fixed) { fixedFields.SetFieldValue(startIndex_, std::move(value_)); }
        else
        {
            if (startIndex_ >= varFields.size()) { throw std::runtime_error("SetFieldValue(): varFields index is out of range"); }

            varFields[startIndex_] = std::move(value_);
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values using field definition.
    //!
    //! \tparam Fixed True for fixed fields, false for variable-length fields.
    //! \param[in] fieldDef_ The definition of the field to set.
    //! \param[in] value_ Rvalue reference to value to move.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const BaseField& fieldDef_, T&& value_)
    {
        switch (fieldDef_.type)
        {
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
        case FIELD_TYPE::RESPONSE_STR: [[fallthrough]]; // TODO: is RESPONSE_STR always std::string?
        case FIELD_TYPE::STRING: SetFieldValue<false>(fieldDef_.index, std::forward<T>(value_)); break;
        case FIELD_TYPE::FIELD_ARRAY:
            if constexpr (std::is_same_v<T, FlatFieldArray> || std::is_same_v<T, NestedFieldArray>)
            {
                varFields[fieldDef_.index] = std::forward<T>(value_);
            }
            else { throw std::runtime_error("SetFieldValue<T>(): incorrect type given for FIELD_ARRAY"); }
            break;
        default: SetFieldValue<true>(fieldDef_.index, std::forward<T>(value_));
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set a single field element offset by the specified index.
    //!
    //! \details This function is needed because ASCII/JSON field map functions are
    //!     called for both standalone fields and individual array elements. SetFieldValue
    //!     should only be used to set the value of a complete field, as it does no
    //!     bounds checking for individual array elements and it cannot set individual
    //!     elements of a variable-length array.
    //!
    //! \tparam Fixed True for fixed fields, false for variable-length fields.
    //! \tparam T The element type.
    //! \param[in] fieldDef_ The definition of the array field.
    //! \param[in] elementIndex_ The element index within the array.
    //! \param[in] value_ The value to set.
    //! \throws std::runtime_error on invalid index or type mismatch.
    // ---------------------------------------------------------------------------
    template <bool Fixed = true, typename T> void SetArrayElement(const BaseField& fieldDef_, size_t elementIndex_, const T& value_)
    {
        using StoredType = std::conditional_t<std::is_same_v<T, bool>, uint8_t, T>;

        const auto getMaxArrayLength = [&]() -> size_t {
            const auto* arrayField = dynamic_cast<const ArrayField*>(&fieldDef_);
            if (arrayField == nullptr) { throw std::runtime_error("SetArrayElement(): missing fixed array metadata"); }
            return arrayField->arrayLength;
        };

        const auto index = fieldDef_.index;

        if constexpr (Fixed) { SetFieldValue<true>(index + (elementIndex_ * sizeof(StoredType)), static_cast<StoredType>(value_)); }
        else
        {
            if (index >= varFields.size()) { throw std::runtime_error("SetArrayElement(): varFields index is out of range"); }

            std::visit(
                [&](auto& varValue) {
                    using VarType = std::decay_t<decltype(varValue)>;
                    if constexpr (std::is_same_v<VarType, std::vector<StoredType>>)
                    {
                        if (varValue.size() <= elementIndex_)
                        {
                            const auto maxLen = getMaxArrayLength();
                            if (maxLen <= elementIndex_)
                            {
                                throw std::runtime_error("SetArrayElement(): element index exceeds maximum array length");
                            }
                            varValue.resize(maxLen);
                        }
                        varValue[elementIndex_] = static_cast<StoredType>(value_);
                    }
                    else
                    {
                        const auto maxLen = getMaxArrayLength();
                        if (maxLen <= elementIndex_) { throw std::runtime_error("SetArrayElement(): element index exceeds maximum array length"); }
                        varFields[index] = std::vector<StoredType>(maxLen);
                        std::get<std::vector<StoredType>>(varFields[index])[elementIndex_] = static_cast<StoredType>(value_);
                    }
                },
                varFields[index]);
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Write a field's data to a given (byte*) buffer.
    //!
    //! \param[in] field_ The field definition.
    //! \param[out] buffer_ The destination buffer.
    //! \param[in] capacity_ The capacity of the destination buffer.
    //! \return The number of bytes written.
    //! \throws std::runtime_error on invalid index, type mismatch, or insufficient
    //!     buffer capacity.
    // ---------------------------------------------------------------------------
    size_t WriteFieldToBuffer(const BaseField& field_, std::byte* buffer_, const size_t capacity_) const
    {
        if (buffer_ == nullptr && capacity_ != 0) { throw std::runtime_error("WriteFieldToBuffer(): destination buffer is null"); }

        const auto copyBytes = [&](const std::byte* source_, const size_t size_) -> size_t {
            if (size_ > capacity_) { throw std::runtime_error("WriteFieldToBuffer(): destination buffer too small"); }
            if (size_ > 0) { std::memcpy(buffer_, source_, size_); }
            return size_;
        };

        switch (field_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto size = GetFieldByteSize(field_);
            if (field_.index + size > fixedFields.size()) { throw std::runtime_error("WriteFieldToBuffer(): fixed array index out of range"); }
            return copyBytes(fixedFields.data() + field_.index, size);
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::FIELD_ARRAY: [[fallthrough]];
        case FIELD_TYPE::STRING: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR:
            if (field_.index >= varFields.size()) { throw std::runtime_error("WriteFieldToBuffer(): var field index out of range"); }

            return std::visit(
                [&](auto& value) -> size_t {
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<std::byte>>)
                    {
                        return copyBytes(reinterpret_cast<const std::byte*>(value.data()), value.size());
                    }
                    else if constexpr (std::is_same_v<T, FlatFieldArray>) { return copyBytes(value.data(), value.ByteSize()); }
                    else if constexpr (std::is_arithmetic_v<T>)
                    {
                        throw std::runtime_error("WriteFieldToBuffer(): scalar values are not valid var field payloads");
                    }
                    else if constexpr (is_specialization_of_v<T, std::vector>)
                    {
                        return copyBytes(reinterpret_cast<const std::byte*>(value.data()), sizeof(typename T::value_type) * value.size());
                    }
                    else { throw std::runtime_error("WriteFieldToBuffer(): unsupported var field payload type"); }
                },
                varFields[field_.index]);
        default: {
            if (field_.index + field_.dataType.length > fixedFields.size())
            {
                throw std::runtime_error("WriteFieldToBuffer(): fixed field index out of range");
            }
            return copyBytes(fixedFields.data() + field_.index, field_.dataType.length);
        }
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Write a field's data to a given (unsigned char**) buffer.
    //!
    //! \param[in] field_ The field definition.
    //! \param[out] buffer_ The destination buffer.
    //! \param[in] capacity_ The capacity of the destination buffer.
    //! \return The number of bytes written.
    //! \throws std::runtime_error on invalid index, type mismatch, or insufficient
    //!     buffer capacity.
    // ---------------------------------------------------------------------------
    size_t WriteFieldToBuffer(const BaseField& field_, unsigned char** buffer_, uint32_t& capacity_) const
    {
        if (buffer_ == nullptr || *buffer_ == nullptr)
        {
            if (capacity_ == 0) { return 0; }
            throw std::runtime_error("WriteFieldToBuffer(): destination buffer is null");
        }

        const auto written = WriteFieldToBuffer(field_, reinterpret_cast<std::byte*>(*buffer_), static_cast<size_t>(capacity_));
        *buffer_ += written;
        capacity_ -= static_cast<uint32_t>(written);
        return written;
    }

    struct const_iterator
    {
        using value_type = FieldValueVariant;
        using reference = value_type;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        const MessageBody* messageBody;
        const std::vector<BaseField::ConstPtr>* fields;
        size_t index;

        const_iterator(const MessageBody* messageBody_, const std::vector<BaseField::ConstPtr>* fields_, size_t index_ = 0)
            : messageBody(messageBody_), fields(fields_), index(index_)
        {
        }

        reference operator*() const { return messageBody->GetFieldValueVariant(*(*fields)[index]); }

        const_iterator& operator++()
        {
            index++;
            return *this;
        }

        const_iterator operator--()
        {
            index--;
            return *this;
        }

        bool operator==(const const_iterator& other) const
        {
            return messageBody == other.messageBody && fields == other.fields && index == other.index;
        }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }
    };

    const_iterator begin() const
    {
        if (fieldInfo == nullptr) { throw std::runtime_error("begin(): field definition not set"); }
        return const_iterator(this, &fieldInfo->messageOrderedFields);
    }

    const_iterator end() const
    {
        if (fieldInfo == nullptr) { throw std::runtime_error("end(): field definition not set"); }
        return const_iterator(this, &fieldInfo->messageOrderedFields, fieldInfo->messageOrderedFields.size());
    }
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

    EnumDefinition::ConstPtr vMyResponseDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    std::string sMyExpectedMessageFamily;

    std::function<size_t(const size_t, const uintptr_t, const uintptr_t)> fMyAlignmentFunc = MessageDatabase::NoAlign;

    // Enum util functions
    void InitEnumDefinitions();
    void InitFieldMaps();

  protected:
    MessageDatabase::Ptr pclMyMsgDb{nullptr};

    std::unordered_map<uint32_t, std::function<void(MessageBody&, const BaseField&, const char**, size_t, size_t, bool, MessageDatabase&)>>
        asciiFieldMap;
    std::unordered_map<uint32_t, std::function<void(MessageBody&, const BaseField&, simdjson::dom::element, size_t, bool, MessageDatabase&)>>
        jsonFieldMap;

    [[nodiscard]] STATUS DecodeBinary(const FieldInfo& vMsgDefFields_, const unsigned char** ppucLogBuf_, MessageBody& clMessageBody_,
                                      uint32_t uiMessageLength_) const;
    template <bool Abbreviated>
    [[nodiscard]] STATUS DecodeAscii(const FieldInfo& vMsgDefFields_, const char** ppcLogBuf_, MessageBody& clMessageBody_,
                                     const char* pcBufEnd = nullptr) const;
    [[nodiscard]] STATUS DecodeJson(const FieldInfo& vMsgDefFields_, simdjson::dom::element jsonData, MessageBody& clMessageBody_) const;

    template <bool Fixed = true>
    static void DecodeBinaryField(const BaseField& pstMessageDataType_, const unsigned char** ppucLogBuf_, MessageBody& clMessageBody_, size_t n = 1);
    void DecodeAsciiField(const BaseField& field_, const char** ppcToken_, size_t tokenLength_, MessageBody& clMessageBody_, size_t elementIndex_ = 0,
                          bool fixed_ = true) const;
    void DecodeJsonField(const BaseField& field_, simdjson::dom::element clJsonField_, MessageBody& clMessageBody_, size_t elementIndex_ = 0,
                         bool fixed_ = true) const;

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
    static void ParseAndEmplace(MessageBody& clMessageBody_, const BaseField& field_, const char* token, size_t tokenLength, size_t elementIndex_ = 0)
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

        clMessageBody_.SetArrayElement<Fixed>(field_, elementIndex_, value);
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T, int R = 10>
    static std::function<void(MessageBody&, const BaseField&, const char**, size_t, size_t, bool, MessageDatabase&)> SimpleAsciiMapEntry()
    {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Template argument must be integral or float");

        return [](MessageBody& vIntermediate_, const BaseField& pstField_, const char** ppcToken_, [[maybe_unused]] const size_t tokenLength_,
                  const size_t elementIndex_, const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
            if (fixed_) { ParseAndEmplace<true, T, R>(vIntermediate_, pstField_, *ppcToken_, tokenLength_, elementIndex_); }
            else { ParseAndEmplace<false, T, R>(vIntermediate_, pstField_, *ppcToken_, tokenLength_, elementIndex_); }
        };
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static void PushElement(MessageBody& vIntermediate_, const BaseField& field_, simdjson::dom::element clJsonField_, size_t elementIndex_ = 0,
                            bool fixed_ = true)
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
        if (clJsonField_.get(intermediateValue) != simdjson::SUCCESS)
        {
            throw std::runtime_error("Failed to decode JSON field '" + pstMessageDataType_->name + "'");
        }
        T value = static_cast<T>(intermediateValue);
        if (fixed_) { vIntermediate_.SetArrayElement<true>(field_, elementIndex_, value); }
        else { vIntermediate_.SetArrayElement<false>(field_, elementIndex_, value); }
    }

    // -------------------------------------------------------------------------------------------------------
    template <typename T>
    static std::function<void(MessageBody&, const BaseField&, simdjson::dom::element, size_t, bool, MessageDatabase&)> SimpleJsonMapEntry()
    {
        return [](MessageBody& vIntermediate_, const BaseField& pstMessageDataType_, simdjson::dom::element clJsonField_, const size_t elementIndex_,
                  const bool fixed_, [[maybe_unused]] MessageDatabase& pclMsgDb_) {
            PushElement<T>(vIntermediate_, pstMessageDataType_, clJsonField_, elementIndex_, fixed_);
        };
    }

    // -------------------------------------------------------------------------------------------------------
    uint32_t GetArrayLength(const unsigned char* pucTempStart, const unsigned char** ppucLogBuf_, const ArrayField& arrayDef,
                            const MessageBody& clMessageBody_) const
    {
        if (arrayDef.arrayLengthRef.empty())
        {
            const std::size_t lenBytes = arrayDef.arrayLengthFieldSize;
            if (!(lenBytes == 1 || lenBytes == 2 || lenBytes == 4))
                throw std::runtime_error("GetArrayLength: Unsupported length size; must be 1,2 or 4");

            *ppucLogBuf_ += fMyAlignmentFunc(lenBytes, reinterpret_cast<uintptr_t>(pucTempStart), reinterpret_cast<uintptr_t>(*ppucLogBuf_));

            uint32_t uiArrayLength = 0;
            for (std::size_t i = 0; i < lenBytes; ++i) { uiArrayLength |= static_cast<uint32_t>((*ppucLogBuf_)[i]) << (8 * i); }

            *ppucLogBuf_ += lenBytes;
            return uiArrayLength;
        }

        // Traverse the decoded fields to find the arrayLengthRef field by its name.
        FieldValueVariant fieldValue;
        if (clMessageBody_.GetFieldValueVariant(fieldValue, arrayDef.arrayLengthRef))
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
                fieldValue);

            if (arraySize) { return *arraySize; }
        }

        throw std::runtime_error("GetArrayLength(): No matching field found for arrayLengthRef");
    }

    //----------------------------------------------------------------------------
    //! \brief Get the message definition corresponding to the given metadata.
    //
    //! \param[in] stMetaData_ The metadata containing the message ID to look up.
    //! \return A pointer to the message definition.
    //----------------------------------------------------------------------------
    virtual MessageDefinition::ConstPtr GetMessageDefinition(MetaDataBase& stMetaData_) const
    {
        if (pclMyMsgDb == nullptr) { throw std::runtime_error("GetMessageDefinition(): no message database loaded"); }

        auto msgDef = pclMyMsgDb->GetMsgDef(stMetaData_.usMessageId);
        return msgDef;
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the MessageDecoderBase class.
    //
    //! \param[in] expectedMessageFamily_ The expected message family for the encoder.
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    MessageDecoderBase(std::string expectedMessageFamily_, MessageDatabase::Ptr pclMessageDb_ = nullptr,
                       std::function<size_t(const size_t, const uintptr_t, const uintptr_t)> fAlignmentFunc_ = MessageDatabase::NoAlign)
        : sMyExpectedMessageFamily(std::move(expectedMessageFamily_)), fMyAlignmentFunc(std::move(fAlignmentFunc_)),
          pclMyMsgDb(std::move(pclMessageDb_))
    {
        InitFieldMaps();
        if (pclMessageDb_ != nullptr) { LoadJsonDb(std::move(pclMessageDb_)); }
    }

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
    [[nodiscard]] STATUS Decode(const unsigned char* pucMessage_, MessageBody& stInterMessage_, MetaDataBase& stMetaData_) const;
};

} // namespace novatel::edie

#endif // MESSAGE_DECODER_HPP
