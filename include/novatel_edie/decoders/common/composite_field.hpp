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
// ! \file composite_field.hpp
// ===============================================================================

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"

namespace novatel::edie {

template <typename T, template <typename...> class Template> struct is_specialization_of : std::false_type
{
};

template <template <typename...> class Template, typename... Args> struct is_specialization_of<Template<Args...>, Template> : std::true_type
{
};

template <typename T, template <typename...> class Template> inline constexpr bool is_specialization_of_v = is_specialization_of<T, Template>::value;

class CompositeField;
class FieldArray;

//! Return type of GetFieldValue<T>: FieldArray is returned by reference to avoid copying its storage.
template <typename T> using FieldValueResult = std::conditional_t<std::is_same_v<T, FieldArray>, const FieldArray&, T>;

// ---------------------------------------------------------------------------
//! \class TypedBuffer
//! \brief A lightweight wrapper around a byte buffer that allows for typed
//!     access to its elements. Used for fields of type FIXED_LENGTH_ARRAY.
// ---------------------------------------------------------------------------
template <typename T> class TypedBuffer
{
  private:
    const std::byte* dataptr;
    size_t sz;

  public:
    using value_type = T;

    TypedBuffer(const std::byte* data_, size_t sz_) : dataptr(data_), sz(sz_) {}

    T operator[](size_t index) const
    {
        if (index >= sz) { throw std::runtime_error("TypedBuffer<T>::operator[](): index out of bounds"); }
        return LoadValueFromBuffer<T>(dataptr + (index * sizeof(T)));
    }

    constexpr const std::byte* data() const { return dataptr; }

    constexpr size_t size() const { return sz; }

    constexpr bool empty() const { return sz == 0; }

    class const_iterator
    {
      public:
        using value_type = T;
        using reference = T;
        using pointer = void;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

      private:
        const std::byte* dataptr;
        size_t index;

      public:
        const_iterator() : dataptr(nullptr), index(0) {}
        const_iterator(const std::byte* data_, size_t index_) : dataptr(data_), index(index_) {}

        reference operator*() const { return LoadValueFromBuffer<T>(dataptr + (index * sizeof(T))); }

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

        bool operator==(const const_iterator& other) const { return dataptr == other.dataptr && index == other.index; }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }
    };

    const_iterator begin() const { return const_iterator(dataptr, 0); }
    const_iterator end() const { return const_iterator(dataptr, sz); }
};

// ---------------------------------------------------------------------------
//! \brief A visitor that calls the given function with a type tag corresponding
//!     to the data type of the given BaseField.
//!
//! \details This function is useful for performing type-specific operations on
//!     fields without knowing their types at compile time and without wrapping
//!     the field values in a variant.
//!
//! \param[in] fd_ The BaseField whose data type is to be visited.
//! \param[in] visitor The function to call with the type tag.
// ---------------------------------------------------------------------------
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

#ifndef NDEBUG
// ---------------------------------------------------------------------------
//! \brief Debug-only assertion that the requested load type T matches the
//!     field's declared data type. Compiled out of release builds; the caller
//!     already supplies T, so no runtime dispatch is needed on the hot path.
// ---------------------------------------------------------------------------
template <typename T> inline void AssertFixedFieldType(const BaseField& fd_)
{
    SimpleTypeVisitor(fd_, [&](auto&& arg) {
        using ArgT = std::decay_t<decltype(arg)>;
        if constexpr (is_specialization_of_v<T, TypedBuffer>)
        {
            if constexpr (!std::is_same_v<T, TypedBuffer<ArgT>>)
            {
                throw std::runtime_error("GetFieldValue<T>(): type T does not match field data type");
            }
        }
        else if constexpr (!std::is_same_v<T, ArgT>) { throw std::runtime_error("GetFieldValue<T>(): type T does not match field data type"); }
    });
}
#endif

// ---------------------------------------------------------------------------
//! \class FixedFieldRegion
//! \brief A byte vector for storing fixed-size fields.
// ---------------------------------------------------------------------------
class FixedFieldRegion
{
  private:
    std::vector<std::byte> byteRegion;
    // Non-owning view mode: when viewData is non-null, reads are served from the
    // borrowed memory [viewData, viewData + viewSize) instead of byteRegion.
    // A viewing region is read-only; mutating operations are not permitted.
    const std::byte* viewData = nullptr;
    size_t viewSize = 0;

  public:
    FixedFieldRegion() = default;

    FixedFieldRegion(std::vector<std::byte>&& data_) : byteRegion(std::move(data_)) {}

    FixedFieldRegion(size_t sz_) : byteRegion(sz_) {}

    // ---------------------------------------------------------------------------
    //! \brief Create a non-owning FixedFieldRegion that borrows external bytes.
    //!
    //! The returned region does not copy or own the data; the caller must ensure
    //! the referenced memory outlives the region. Only read operations
    //! (data/size/Load) are valid on a view; mutating operations are undefined.
    //!
    //! \param[in] data_ Pointer to the first byte of the borrowed region.
    //! \param[in] size_ Number of bytes in the borrowed region.
    // ---------------------------------------------------------------------------
    [[nodiscard]] static FixedFieldRegion View(const std::byte* data_, size_t size_)
    {
        FixedFieldRegion region;
        region.viewData = data_;
        region.viewSize = size_;
        return region;
    }

    //! \brief Whether this region borrows external memory (read-only view).
    [[nodiscard]] bool IsView() const { return viewData != nullptr; }

    size_t size() const { return viewData != nullptr ? viewSize : byteRegion.size(); }

    const std::byte* data() const { return viewData != nullptr ? viewData : byteRegion.data(); }

    // ---------------------------------------------------------------------------
    //! \brief Resize the FixedFieldRegion.
    //!
    //! \param[in] sz_ The new size of the fixed field region in bytes.
    // ---------------------------------------------------------------------------
    void resize(size_t sz_)
    {
        assert(!IsView() && "resize() is not permitted on a non-owning FixedFieldRegion view");
        byteRegion.resize(sz_);
    }

    // ---------------------------------------------------------------------------
    //! \brief Load a trivially-copyable value of type T from a raw byte offset.
    //!
    //! \tparam T The trivially-copyable type to read.
    //! \param[in] byteOffset_ The byte offset into the region to read from.
    //! \return The value read from the region.
    // ---------------------------------------------------------------------------
    template <typename T> [[nodiscard]] T Load(size_t byteOffset_) const
    {
        static_assert(std::is_trivially_copyable_v<T> && !is_specialization_of_v<T, TypedBuffer>, "Load only supports trivially copyable types");
        T value;
        std::memcpy(&value, data() + byteOffset_, sizeof(T));
        return value;
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
        assert(!IsView() && "SetFieldValue() is not permitted on a non-owning FixedFieldRegion view");
        if (startIndex_ + (n * sizeof(T)) > byteRegion.size()) { throw std::runtime_error("SetFieldValue(): buffer overflow in FixedFieldRegion"); }
        std::memcpy(byteRegion.data() + startIndex_, values_, n * sizeof(T));
    }

    // ---------------------------------------------------------------------------
    //! \brief Set a single field value at the specified index.
    //!
    //! \tparam T The value type (must not be a pointer).
    //! \param[in] startIndex_ The index in the byte region.
    //! \param[in] value_ The value to set.
    // ---------------------------------------------------------------------------
    template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>>> void SetFieldValue(const size_t startIndex_, const T& value_)
    {
        if constexpr (is_specialization_of_v<T, TypedBuffer>)
        {
            SetFieldValue(startIndex_, value_.data(), value_.size() * sizeof(typename T::value_type));
        }
        else { SetFieldValue(startIndex_, &value_); }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values from a vector.
    //!
    //! \tparam T The element type (must be trivially copyable and not bool).
    //! \param[in] startIndex_ The index in the byte region.
    //! \param[in] values_ Vector of values to copy.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const size_t startIndex_, const std::vector<T>& values_)
    {
        static_assert(std::is_trivially_copyable_v<T>, "SetFieldValue only supports trivially copyable vector element types");
        static_assert(!std::is_same_v<T, bool>, "SetFieldValue does not support std::vector<bool>");
        assert(!IsView() && "SetFieldValue() is not permitted on a non-owning FixedFieldRegion view");

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
    //! \param[in] value_ String to copy.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    void SetFieldValue(const size_t startIndex_, const std::string& value_)
    {
        assert(!IsView() && "SetFieldValue() is not permitted on a non-owning FixedFieldRegion view");
        if (startIndex_ + value_.size() > byteRegion.size()) { throw std::runtime_error("SetFieldValue(): buffer overflow in FixedFieldRegion"); }
        std::memcpy(byteRegion.data() + startIndex_, value_.data(), value_.size());
    }
};

// ---------------------------------------------------------------------------
//! \brief Load a scalar (SIMPLE/ENUM/RESPONSE_ID) fixed field value directly
//!     from raw byte storage, without re-dispatching on field type.
//!
//! This is the innermost scalar read: callers that have already determined the
//! field is scalar (e.g. CompositeField::GetFieldValue's default branch) route
//! here to avoid re-switching on field_.type.
//!
//! \tparam T The value type (trivially copyable, never a TypedBuffer<E>).
//! \param[in] region_ The byte storage to read from.
//! \param[in] field_ The field definition describing offset.
//! \param[in] baseIndex_ Optional base byte offset (e.g. flat-array row start).
// ---------------------------------------------------------------------------
template <typename T> [[nodiscard]] inline T LoadScalarField(const FixedFieldRegion& region_, const BaseField& field_, size_t baseIndex_ = 0)
{
    static_assert(std::is_trivially_copyable_v<T> && !is_specialization_of_v<T, TypedBuffer>,
                  "LoadScalarField only supports trivially copyable types");
#ifndef NDEBUG
    AssertFixedFieldType<T>(field_);
#endif
    return region_.Load<T>(baseIndex_ + field_.index);
}

// ---------------------------------------------------------------------------
//! \brief Interpret and load a single fixed field value (scalar/enum, or a
//!     TypedBuffer view over a fixed-length array) from raw byte storage.
//!
//! This holds the schema knowledge that used to live inside FixedFieldRegion,
//! keeping that class a pure byte store.
//!
//! \tparam T The value type (trivially copyable, or a TypedBuffer<E>).
//! \param[in] region_ The byte storage to read from.
//! \param[in] field_ The field definition describing type and offset.
//! \param[in] baseIndex_ Optional base byte offset (e.g. flat-array row start).
// ---------------------------------------------------------------------------
template <typename T> [[nodiscard]] inline T LoadFixedField(const FixedFieldRegion& region_, const BaseField& field_, size_t baseIndex_ = 0)
{
    static_assert(std::is_trivially_copyable_v<T> || is_specialization_of_v<T, TypedBuffer>,
                  "LoadFixedField only supports trivially copyable types or TypedBuffer specializations");
    switch (field_.type)
    {
    case FIELD_TYPE::FIXED_LENGTH_ARRAY:
        if constexpr (is_specialization_of_v<T, TypedBuffer>)
        {
            const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
            if (arrayField == nullptr) { throw std::runtime_error("GetFieldValue<T>(): missing fixed array metadata"); }
#ifndef NDEBUG
            AssertFixedFieldType<T>(field_);
#endif
            return T{region_.data() + baseIndex_ + field_.index, arrayField->arrayLength};
        }
        else { throw std::runtime_error("GetFieldValue<T>(): T must be TypedBuffer<E> for FIXED_LENGTH_ARRAY"); }
    case FIELD_TYPE::RESPONSE_ID: [[fallthrough]];
    case FIELD_TYPE::ENUM: [[fallthrough]];
    case FIELD_TYPE::SIMPLE:
        if constexpr (is_specialization_of_v<T, TypedBuffer>)
        {
            throw std::runtime_error("GetFieldValue<T>(): T must not be TypedBuffer<E> for SIMPLE or ENUM fields");
        }
        else { return LoadScalarField<T>(region_, field_, baseIndex_); }
    default: throw std::runtime_error("GetFieldValue<T>(): unsupported field type for GetFieldValue in FixedFieldRegion");
    }
}

// ---------------------------------------------------------------------------
//! \brief Load a single element of a fixed-length array field from raw storage.
//!
//! \tparam T The element value type (trivially copyable).
//! \param[in] region_ The byte storage to read from.
//! \param[in] elemIndex_ The array element index.
//! \param[in] field_ The array field definition.
//! \param[in] baseIndex_ Optional base byte offset (e.g. flat-array row start).
// ---------------------------------------------------------------------------
template <typename T>
[[nodiscard]] inline T LoadFixedFieldElement(const FixedFieldRegion& region_, size_t elemIndex_, const BaseField& field_, size_t baseIndex_ = 0)
{
    static_assert(std::is_trivially_copyable_v<T>, "LoadFixedFieldElement only supports trivially copyable types");
    if (field_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
    {
        const auto* arrayField = dynamic_cast<const ArrayField*>(&field_);
        if (arrayField == nullptr) { throw std::runtime_error("GetFieldValue<T>(): missing fixed array metadata"); }
        if (elemIndex_ >= arrayField->arrayLength)
        {
            throw std::runtime_error("GetFieldValue<T>(): index out of bounds for fixed-length array field");
        }
        if constexpr (std::is_same_v<T, bool>)
        {
#ifndef NDEBUG
            AssertFixedFieldType<uint8_t>(field_);
#endif
            return static_cast<bool>(region_.Load<uint8_t>(baseIndex_ + field_.index + elemIndex_));
        }
        else
        {
#ifndef NDEBUG
            AssertFixedFieldType<T>(field_);
#endif
            return region_.Load<T>(baseIndex_ + field_.index + (elemIndex_ * sizeof(T)));
        }
    }

    if (elemIndex_ == 0) { return LoadFixedField<T>(region_, field_, baseIndex_); }
    throw std::runtime_error("GetFieldValue<T>(): field must be an array type or element index must be zero");
}

// ---------------------------------------------------------------------------
//! \class FieldArray
//! \brief The value of a FIELD_ARRAY field.
//!
//! \details Records are stored either as CompositeFields or, when every field
//!     has a fixed size, as contiguous raw bytes. Callers access records the
//!     same way regardless of the representation.
// ---------------------------------------------------------------------------
class FieldArray
{
  public:
    template <bool Mutable> struct FieldArrayRecordView
    {
        using FieldArrayPointer = std::conditional_t<Mutable, FieldArray*, const FieldArray*>;

        FieldArrayPointer fieldArray;
        size_t index;

        FieldArrayRecordView(FieldArrayPointer fieldArray_, size_t index_) : fieldArray(fieldArray_), index(index_) {}

        [[nodiscard]] std::conditional_t<Mutable, CompositeField*, const CompositeField*> GetCompositeField() const;

        template <typename T> [[nodiscard]] FieldValueResult<T> GetFieldValue(const BaseField& field_, size_t elementIndex_ = 0) const
        {
            return fieldArray->template GetFieldValue<T>(field_, index, elementIndex_);
        }

        template <typename T> [[nodiscard]] FieldValueResult<T> GetFieldValueByName(const std::string& name_, size_t elementIndex_ = 0) const
        {
            return fieldArray->template GetFieldValueByName<T>(name_, index, elementIndex_);
        }

        template <typename T, bool Enabled = Mutable, std::enable_if_t<Enabled && Mutable, int> = 0>
        void SetFieldValue(const BaseField& fieldDef_, const T* values_, size_t count_) const
        {
            fieldArray->SetFieldValue(fieldDef_, index, values_, count_);
        }

        template <typename T, bool Enabled = Mutable, std::enable_if_t<Enabled && Mutable, int> = 0>
        void SetFieldValue(const BaseField& fieldDef_, T&& value_) const
        {
            fieldArray->SetFieldValue(fieldDef_, index, std::forward<T>(value_));
        }

        template <bool Enabled = Mutable, std::enable_if_t<Enabled && Mutable, int> = 0>
        CompositeField ReplaceRecord(const CompositeField& record_) const;
    };

    explicit FieldArray(std::vector<CompositeField> records_ = {}, FieldInfo::ConstPtr fieldInfo_ = nullptr)
        : storage(std::move(records_)), fieldInfo(std::move(fieldInfo_))
    {
    }

    //! Construct from the raw bytes of fixed-size records.
    explicit FieldArray(std::vector<std::byte> bytes_, FieldInfo::ConstPtr fieldInfo_)
        : storage(FixedFieldRegion(std::move(bytes_))), fieldInfo(std::move(fieldInfo_))
    {
        if (fieldInfo == nullptr || fieldInfo->varFieldCount > 0 || fieldInfo->fixedFieldBytes == 0)
        {
            throw std::runtime_error("FieldArray(): fixed storage requires field info with nonzero fixed field bytes and no variable fields");
        }
        if (std::get<FixedFieldRegion>(storage).size() % fieldInfo->fixedFieldBytes != 0)
        {
            throw std::runtime_error("FieldArray(): data size must be a multiple of fixed field bytes");
        }
    }

    [[nodiscard]] size_t size() const;

    [[nodiscard]] bool empty() const { return size() == 0; }

    [[nodiscard]] const FieldInfo::ConstPtr& GetFieldInfo() const { return fieldInfo; }

    //! Call fn_(const CompositeField&) on each record in order, stopping early if it returns false.
    template <typename Fn> bool ForEachRecord(Fn&& fn_) const;

    // ---------------------------------------------------------------------------
    //! \brief Get a field value by its definition.
    //!
    //! \param[in] field_ The definition of the field to retrieve.
    //! \param[in] index_ The index of the row in the field array.
    //! \param[in] elementIndex_ The index of the element within the field to
    //!     retrieve. Ignored if field is not FIXED/VARIABLE_LENGTH_ARRAY.
    //! \return The field value as T.
    // ---------------------------------------------------------------------------
    template <typename T> [[nodiscard]] FieldValueResult<T> GetFieldValue(const BaseField& field_, size_t index_, size_t elementIndex_ = 0) const;

    // ---------------------------------------------------------------------------
    //! \brief Get a field value by its name.
    //! \see FieldArray::GetFieldValue
    // ---------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] FieldValueResult<T> GetFieldValueByName(const std::string& name_, size_t index_, size_t elementIndex_ = 0) const
    {
        if (fieldInfo == nullptr) { throw std::runtime_error("FieldArray::GetFieldValueByName(): field info is not set"); }
        const auto field = fieldInfo->GetFieldDefByName(name_);
        if (field == nullptr) { throw std::runtime_error("FieldArray::GetFieldValueByName(): field not found: " + name_); }
        return GetFieldValue<T>(*field, index_, elementIndex_);
    }

    [[nodiscard]] FieldArrayRecordView<true> operator[](size_t index_) { return FieldArrayRecordView<true>(this, index_); }

    [[nodiscard]] FieldArrayRecordView<false> operator[](size_t index_) const { return FieldArrayRecordView<false>(this, index_); }

    struct const_iterator
    {
        using value_type = FieldArrayRecordView<false>;
        using reference = value_type;
        using pointer = void;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        FieldArrayRecordView<false> record;

        const_iterator(const FieldArray* fieldArray_, size_t index_ = 0) : record(fieldArray_, index_) {}

        reference operator*() const { return record; }

        const_iterator& operator++()
        {
            ++record.index;
            return *this;
        }

        const_iterator operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const
        {
            return record.fieldArray == other.record.fieldArray && record.index == other.record.index;
        }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }
    };

    [[nodiscard]] const_iterator begin() const { return const_iterator(this); }
    [[nodiscard]] const_iterator end() const { return const_iterator(this, size()); }

  private:
    using CompositeFieldArray = std::vector<CompositeField>;

    std::variant<CompositeFieldArray, FixedFieldRegion> storage;
    FieldInfo::ConstPtr fieldInfo;

    template <typename T> void SetFieldValue(const BaseField& fieldDef_, size_t index_, const T* values_, size_t count_);
    template <typename T> void SetFieldValue(const BaseField& fieldDef_, size_t index_, T&& value_);

    //! The records' bytes if they are stored contiguously, otherwise nullptr.
    [[nodiscard]] const std::byte* GetRawDataPointer() const
    {
        const auto* fields = std::get_if<FixedFieldRegion>(&storage);
        return fields != nullptr ? fields->data() : nullptr;
    }

    friend class CompositeField;
};

using FieldValueVariant =
    std::variant<bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double, TypedBuffer<bool>,
                 TypedBuffer<int8_t>, TypedBuffer<uint8_t>, TypedBuffer<int16_t>, TypedBuffer<uint16_t>, TypedBuffer<int32_t>, TypedBuffer<uint32_t>,
                 TypedBuffer<int64_t>, TypedBuffer<uint64_t>, TypedBuffer<float>, TypedBuffer<double>, std::vector<int8_t>, std::vector<int16_t>,
                 std::vector<int32_t>, std::vector<int64_t>, std::vector<uint8_t>, std::vector<uint16_t>, std::vector<uint32_t>,
                 std::vector<uint64_t>, std::vector<float>, std::vector<double>, std::string, FieldArray>;

//! Shared implementation for the LoadVariant overloads: select the concrete element type from the
//! schema via SimpleTypeVisitor and let the caller-supplied loader build the variant payload.
template <typename Field, typename Loader> inline FieldValueVariant LoadVariantImpl(const Field& fd_, Loader&& loader_)
{
    FieldValueVariant result;
    SimpleTypeVisitor(fd_, [&](auto&& arg) { result = loader_(std::decay_t<decltype(arg)>{}); });
    return result;
}

inline FieldValueVariant LoadVariant(const BaseField& fd_, const std::byte* fieldPtr_)
{
    return LoadVariantImpl(fd_, [&](auto tag) { return LoadValueFromBuffer<decltype(tag)>(fieldPtr_); });
}

inline FieldValueVariant LoadVariant(const ArrayField& fd_, const std::byte* fieldPtr_)
{
    return LoadVariantImpl(fd_, [&](auto tag) { return TypedBuffer<decltype(tag)>{fieldPtr_, fd_.arrayLength}; });
}

// ---------------------------------------------------------------------------
//! \class CompositeField
//! \brief An object representing the body of a message, containing both
//!     fixed-length and variable-length fields.
// ---------------------------------------------------------------------------
class CompositeField
{
  private:
    FixedFieldRegion fixedFields;
    std::vector<FieldValueVariant> varFields;
    FieldInfo::ConstPtr fieldInfo;

  public:
    // ---------------------------------------------------------------------------
    //! \brief Default constructor.
    // ---------------------------------------------------------------------------
    CompositeField() = default;

    // ---------------------------------------------------------------------------
    //! \brief Construct a CompositeField with pre-allocated fixed and variable field storage.
    //!
    //! \param[in] fixedFieldSize_ The size in bytes for the fixed fields buffer.
    //! \param[in] varFieldCount_ The number of variable field slots to allocate.
    // ---------------------------------------------------------------------------
    CompositeField(size_t fixedFieldSize_, size_t varFieldCount_) : fixedFields(fixedFieldSize_), varFields(varFieldCount_) {}

    // ---------------------------------------------------------------------------
    //! \brief Construct a CompositeField with pre-constructed fixed and variable regions.
    //!
    //! \param[in] fixedFields_ The FixedFieldRegion containing the fixed-length fields.
    //! \param[in] varFields_ The vector of FieldValueVariant containing the variable-length fields.
    // ---------------------------------------------------------------------------
    CompositeField(FixedFieldRegion&& fixedFields_, std::vector<FieldValueVariant>&& varFields_)
        : fixedFields(std::move(fixedFields_)), varFields(std::move(varFields_))
    {
    }

    // ---------------------------------------------------------------------------
    //! \brief Create a read-only CompositeField that borrows an external fixed-field
    //!     byte slice without copying it.
    //!
    //! Used to encode a single flat-array row in place: the returned body
    //! references [data, data + size) and carries no variable fields. The caller
    //! must ensure the referenced memory outlives the returned body, and must
    //! only read fixed fields from it.
    //!
    //! \param[in] data_ Pointer to the first byte of the borrowed fixed-field slice.
    //! \param[in] size_ Number of bytes in the borrowed slice.
    //! \param[in] fieldInfo_ The field info describing the slice.
    // ---------------------------------------------------------------------------
    [[nodiscard]] static CompositeField ViewFixedFields(const std::byte* data_, size_t size_, FieldInfo::ConstPtr fieldInfo_)
    {
        CompositeField view(FixedFieldRegion::View(data_, size_), {});
        view.fieldInfo = std::move(fieldInfo_);
        return view;
    }

    // ---------------------------------------------------------------------------
    //! \brief Construct a CompositeField from a FieldInfo object, initializing each
    //!     field with the default value of its type.
    //!
    //! \param[in] fieldInfo_ The FieldInfo object containing the message's fields
    // ---------------------------------------------------------------------------
    CompositeField(FieldInfo::ConstPtr fieldInfo_) : fieldInfo(std::move(fieldInfo_))
    {
        if (!fieldInfo) { return; }

        fixedFields.resize(fieldInfo->fixedFieldBytes); // Zero-initializes every fixed field
        varFields.resize(fieldInfo->varFieldCount);
        for (const auto& fieldDef : fieldInfo->messageOrderedFields)
        {
            switch (fieldDef->type)
            {
            case FIELD_TYPE::FIELD_ARRAY:
                varFields[fieldDef->index] = FieldArray(std::vector<CompositeField>{}, static_cast<const FieldArrayField&>(*fieldDef).fieldInfo);
                break;
            case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
                SimpleTypeVisitor(*fieldDef, [&](auto&& arg) {
                    using ValueT = std::conditional_t<std::is_same_v<std::decay_t<decltype(arg)>, bool>, uint8_t, std::decay_t<decltype(arg)>>;
                    varFields[fieldDef->index] = std::vector<ValueT>{};
                });
                break;
            case FIELD_TYPE::RESPONSE_STR: [[fallthrough]];
            case FIELD_TYPE::STRING: varFields[fieldDef->index] = std::string{}; break;
            default: break;
            }
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Resize the memory regions of the CompositeField.
    //!
    //! \param[in] fixedFieldSize_ The new size in bytes for the fixed fields
    //! \param[in] varFieldCount_ The new number of variable field slots to allocate
    // ---------------------------------------------------------------------------
    void resize(size_t fixedFieldSize_, size_t varFieldCount_)
    {
        fixedFields.resize(fixedFieldSize_);
        varFields.resize(varFieldCount_);
    }

    // ---------------------------------------------------------------------------
    //! Getters for member variables
    // ---------------------------------------------------------------------------
    const FixedFieldRegion& GetFixedFields() const { return fixedFields; }
    const std::vector<FieldValueVariant>& GetVarFields() const { return varFields; }
    const FieldInfo::ConstPtr& GetFieldInfo() const { return fieldInfo; }

    // ---------------------------------------------------------------------------
    //! \brief Get a field value as a specific type.
    //!
    //! Single unified accessor. The requested type T selects the interpretation:
    //! a whole container (std::vector / std::string / FieldArray / TypedBuffer
    //! view) is returned when T names that container; otherwise a scalar value is
    //! returned. For array fields, \p elementIndex_ selects the element.
    //!
    //! \tparam T The type to extract from the field value.
    //! \param[in] field_ The definition of the field to retrieve.
    //! \param[in] elementIndex_ The array element index (ignored for whole-container T).
    //! \return The field value as T.
    // ---------------------------------------------------------------------------
    template <typename T> FieldValueResult<T> GetFieldValue(const BaseField& field_, size_t elementIndex_ = 0) const
    {
        switch (field_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY:
            if constexpr (is_specialization_of_v<T, TypedBuffer>) { return LoadFixedField<T>(fixedFields, field_); }
            else if constexpr (std::is_trivially_copyable_v<T> && !std::is_same_v<T, FieldArray>)
            {
                return LoadFixedFieldElement<T>(fixedFields, elementIndex_, field_);
            }
            else { throw std::runtime_error("GetFieldValue<T>(): incorrect type given for FIXED_LENGTH_ARRAY"); }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
            if constexpr (is_specialization_of_v<T, std::vector>) { return std::get<T>(varFields[field_.index]); }
            else if constexpr (std::is_same_v<T, bool>)
            {
                const auto& vec = std::get<std::vector<uint8_t>>(varFields[field_.index]);
                if (elementIndex_ >= vec.size())
                {
                    throw std::runtime_error("GetFieldValue<T>(): index out of bounds for variable-length array field");
                }
                return static_cast<bool>(vec[elementIndex_]);
            }
            else if constexpr (std::is_trivially_copyable_v<T> && !std::is_same_v<T, FieldArray> && !is_specialization_of_v<T, TypedBuffer>)
            {
                const auto& vec = std::get<std::vector<T>>(varFields[field_.index]);
                if (elementIndex_ >= vec.size())
                {
                    throw std::runtime_error("GetFieldValue<T>(): index out of bounds for variable-length array field");
                }
                return vec[elementIndex_];
            }
            else { throw std::runtime_error("GetFieldValue<T>(): incorrect type given for VARIABLE_LENGTH_ARRAY"); }
        case FIELD_TYPE::RESPONSE_STR: [[fallthrough]];
        case FIELD_TYPE::STRING:
            if constexpr (std::is_same_v<T, std::string>) { return std::get<std::string>(varFields[field_.index]); }
            else { throw std::runtime_error("GetFieldValue<T>(): non-string type provided for string field"); }
        case FIELD_TYPE::FIELD_ARRAY:
            if constexpr (std::is_same_v<T, FieldArray>) { return std::get<FieldArray>(varFields[field_.index]); }
            else { throw std::runtime_error("GetFieldValue<T>(): incorrect type given for FIELD_ARRAY"); }
        default:
            if constexpr (std::is_trivially_copyable_v<T> && !std::is_same_v<T, FieldArray> && !is_specialization_of_v<T, TypedBuffer>)
            {
                return LoadScalarField<T>(fixedFields, field_);
            }
            else { throw std::runtime_error("GetFieldValue<T>(): incorrect type T for given FIELD_TYPE"); }
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Get a field value by its name.
    //! \see CompositeField::GetFieldValue
    // ---------------------------------------------------------------------------
    template <typename T> [[nodiscard]] FieldValueResult<T> GetFieldValueByName(const std::string& name_, size_t elementIndex_ = 0) const
    {
        if (fieldInfo == nullptr) { throw std::runtime_error("CompositeField::GetFieldValueByName(): field info is not set"); }
        const auto field = fieldInfo->GetFieldDefByName(name_);
        if (field == nullptr) { throw std::runtime_error("CompositeField::GetFieldValueByName(): field not found: " + name_); }
        return GetFieldValue<T>(*field, elementIndex_);
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
                    else if constexpr (std::is_same_v<T, FieldArray>)
                    {
                        const auto* fieldArrayField = dynamic_cast<const FieldArrayField*>(&field_);
                        if (fieldArrayField == nullptr) { throw std::runtime_error("GetFieldByteSize(): missing field array metadata"); }
                        if (fieldArrayField->fieldInfo->varFieldCount == 0) { return fieldArrayField->fieldInfo->fixedFieldBytes * value.size(); }
                        size_t byteSize = 0;
                        value.ForEachRecord([&](const CompositeField& compField) {
                            for (const auto& field : fieldArrayField->fieldInfo->messageOrderedFields)
                            {
                                byteSize += compField.GetFieldByteSize(*field);
                            }
                            return true;
                        });
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
                    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, FieldArray> || is_specialization_of_v<T, std::vector>)
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
    //! \note This method does not initialize the storage regions in any way. If
    //!     there were values already set in the message body, they will remain
    //!     unchanged. This method also does not default initialize field; it only
    //!     resizes the storage regions.
    //!
    //! \param[in] fieldInfo_ The field info pointer.
    // ---------------------------------------------------------------------------
    void SetFieldInfo(FieldInfo::ConstPtr fieldInfo_)
    {
        fieldInfo = std::move(fieldInfo_);
        if (fieldInfo) { resize(fieldInfo->fixedFieldBytes, fieldInfo->varFieldCount); }
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

    void SetFixedData(const std::byte* data_, size_t size_) { fixedFields.SetFieldValue(0, data_, size_); }

    template <typename T> void SetFieldValue(const BaseField& fieldDef_, const T* values_, size_t count_)
    {
        static_assert(std::is_trivially_copyable_v<T>, "SetFieldValue only supports trivially copyable types");
        if (count_ != 0 && values_ == nullptr) { throw std::runtime_error("SetFieldValue(): source pointer is null"); }
        if (fieldDef_.type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
        {
            using ValueT = std::conditional_t<std::is_same_v<T, bool>, uint8_t, T>;
            std::vector<ValueT> values(count_);
            if (count_ != 0) { std::memcpy(values.data(), values_, count_ * sizeof(ValueT)); }
            SetFieldValue(fieldDef_, std::move(values));
        }
        else if (fieldDef_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY || fieldDef_.type == FIELD_TYPE::SIMPLE ||
                 fieldDef_.type == FIELD_TYPE::ENUM || fieldDef_.type == FIELD_TYPE::RESPONSE_ID)
        {
            if (count_ != 0) { fixedFields.SetFieldValue(fieldDef_.index, values_, count_); }
        }
        else { throw std::runtime_error("SetFieldValue(): pointer values require a scalar or regular array field"); }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set field values using field definition.
    //!
    //! \tparam T The value type.
    //! \param[in] fieldDef_ The definition of the field to set.
    //! \param[in] value_ Rvalue reference to value to forward.
    //! \throws std::runtime_error on buffer overflow or invalid index.
    // ---------------------------------------------------------------------------
    template <typename T> void SetFieldValue(const BaseField& fieldDef_, T&& value_)
    {
        using ValueT = std::decay_t<T>;
        static_assert(!std::is_same_v<ValueT, std::vector<bool>>,
                      "SetFieldValue does not support std::vector<bool>. Use std::vector<uint8_t> instead.");
        switch (fieldDef_.type)
        {
        case FIELD_TYPE::FIXED_LENGTH_ARRAY:
            if constexpr (std::is_same_v<ValueT, std::string> || is_specialization_of_v<ValueT, TypedBuffer> ||
                          is_specialization_of_v<ValueT, std::vector>)
            {
                fixedFields.SetFieldValue(fieldDef_.index, std::forward<T>(value_));
            }
            else { throw std::runtime_error("SetFieldValue<T>(): incorrect type given for FIXED_LENGTH_ARRAY"); }
            break;
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: [[fallthrough]];
        case FIELD_TYPE::RESPONSE_STR: [[fallthrough]];
        case FIELD_TYPE::STRING:
            if constexpr (std::is_same_v<ValueT, std::string> || is_specialization_of_v<ValueT, std::vector>)
            {
                varFields.at(fieldDef_.index) = std::forward<T>(value_);
                break;
            }
            else { throw std::runtime_error("SetFieldValue<T>(): incorrect type given for VARIABLE_LENGTH_ARRAY, RESPONSE_STR, or STRING"); }
            break;
        case FIELD_TYPE::FIELD_ARRAY:
            if constexpr (std::is_same_v<ValueT, FieldArray>) { varFields.at(fieldDef_.index) = std::forward<T>(value_); }
            else { throw std::runtime_error("SetFieldValue<T>(): incorrect type given for FIELD_ARRAY"); }
            break;
        default:
            if constexpr (std::is_trivially_copyable_v<ValueT> && !std::is_pointer_v<ValueT>)
            {
                fixedFields.SetFieldValue(fieldDef_.index, std::forward<T>(value_));
            }
            else { throw std::runtime_error("SetFieldValue<T>(): type T must be trivially copyable for SIMPLE fields"); }
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Set a single field element offset by the specified index.
    //!
    //! \note This function is needed because decoder ASCII/JSON field map functions are
    //!     called for both standalone fields and individual array elements. SetFieldValue
    //!     should only be used to set the value of a complete field, as it cannot set
    //!     individual elements of a variable-length array.
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

        if constexpr (Fixed) { fixedFields.SetFieldValue(fieldDef_.index + (elementIndex_ * sizeof(StoredType)), static_cast<StoredType>(value_)); }
        else
        {
            if (fieldDef_.index >= varFields.size()) { throw std::runtime_error("SetArrayElement(): varFields index is out of range"); }

            std::visit(
                [&](auto& varValue) {
                    using VarType = std::decay_t<decltype(varValue)>;
                    if constexpr (std::is_same_v<VarType, std::vector<StoredType>>)
                    {
                        if (varValue.size() <= elementIndex_)
                        {
                            throw std::runtime_error("SetArrayElement(): element index exceeds stored vector size");
                        }
                        varValue[elementIndex_] = static_cast<StoredType>(value_);
                    }
                    else { throw std::runtime_error("SetArrayElement(): variant does not contain a vector of the correct type"); }
                },
                varFields[fieldDef_.index]);
        }
    }

    // ---------------------------------------------------------------------------
    //! \brief Write a field's data to a given (byte*) buffer.
    //!
    //! \param[in] field_ The field definition.
    //! \param[out] buffer_ The destination buffer.
    //! \param[in] capacity_ The capacity of the destination buffer.
    //! \param[in] handler_ Writer for field array records not stored contiguously, called as
    //!     handler_(const CompositeField& record, std::byte* buffer, size_t capacity) and returning
    //!     the bytes written, or std::nullopt on failure. Unused for other fields.
    //! \return The number of bytes written, or zero on failure. Failure may leave
    //!     the destination partially written.
    //! \throws std::runtime_error on invalid index, type mismatch, or insufficient
    //!     buffer capacity.
    // ---------------------------------------------------------------------------
    template <typename Handler = std::nullptr_t>
    size_t WriteFieldToBuffer(const BaseField& field_, std::byte* buffer_, const size_t capacity_, Handler&& handler_ = nullptr) const
    {
        if (buffer_ == nullptr && capacity_ != 0) { throw std::runtime_error("WriteFieldToBuffer(): destination buffer is null"); }

        const auto copyBytes = [&](const std::byte* source_, const size_t size_) -> size_t {
            if (size_ > capacity_) { return 0; }
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
                    else if constexpr (std::is_same_v<T, FieldArray>)
                    {
                        if (value.empty()) { return 0; }
                        const std::byte* data = value.GetRawDataPointer();
                        if (data != nullptr) { return copyBytes(data, value.size() * value.GetFieldInfo()->fixedFieldBytes); }
                        if constexpr (std::is_null_pointer_v<std::decay_t<Handler>>)
                        {
                            throw std::runtime_error("WriteFieldToBuffer(): composite field array requires a handler");
                        }
                        else
                        {
                            size_t written = 0;
                            const bool success = value.ForEachRecord([&](const CompositeField& record) {
                                const std::optional<size_t> count = handler_(record, buffer_ + written, capacity_ - written);
                                if (!count || *count > capacity_ - written) { return false; }
                                written += *count;
                                return true;
                            });
                            return success ? written : 0;
                        }
                    }
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
    //! \param[in] handler_ Writer for field array records not stored contiguously.
    //! \see WriteFieldToBuffer(const BaseField&, std::byte*, size_t, Handler&&)
    //! \return The number of bytes written.
    //! \throws std::runtime_error on invalid index, type mismatch, or insufficient
    //!     buffer capacity.
    // ---------------------------------------------------------------------------
    template <typename Handler = std::nullptr_t>
    size_t WriteFieldToBuffer(const BaseField& field_, unsigned char** buffer_, uint32_t& capacity_, Handler&& handler_ = nullptr) const
    {
        if (buffer_ == nullptr || *buffer_ == nullptr)
        {
            if (capacity_ == 0) { return 0; }
            throw std::runtime_error("WriteFieldToBuffer(): destination buffer is null");
        }

        const auto written =
            WriteFieldToBuffer(field_, reinterpret_cast<std::byte*>(*buffer_), static_cast<size_t>(capacity_), std::forward<Handler>(handler_));
        *buffer_ += written;
        capacity_ -= static_cast<uint32_t>(written);
        return written;
    }
};

template <typename T> inline FieldValueResult<T> FieldArray::GetFieldValue(const BaseField& field_, size_t index_, size_t elementIndex_) const
{
    if (const auto* fields = std::get_if<FixedFieldRegion>(&storage))
    {
        if constexpr (std::is_trivially_copyable_v<T> || is_specialization_of_v<T, TypedBuffer>)
        {
            const size_t rowOffset = index_ * fieldInfo->fixedFieldBytes;
            if (rowOffset >= fields->size()) { throw std::out_of_range("FieldArray::GetFieldValue(): index out of range"); }
            if constexpr (!is_specialization_of_v<T, TypedBuffer>)
            {
                if (field_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY) { return LoadFixedFieldElement<T>(*fields, elementIndex_, field_, rowOffset); }
            }
            return LoadFixedField<T>(*fields, field_, rowOffset);
        }
        else { throw std::runtime_error("FieldArray::GetFieldValue<T>(): type T must be trivially copyable or TypedBuffer specialization"); }
    }
    return std::get<CompositeFieldArray>(storage).at(index_).GetFieldValue<T>(field_, elementIndex_);
}

inline size_t FieldArray::size() const
{
    if (const auto* fields = std::get_if<FixedFieldRegion>(&storage)) { return fields->size() / fieldInfo->fixedFieldBytes; }
    return std::get<CompositeFieldArray>(storage).size();
}

template <typename Fn> bool FieldArray::ForEachRecord(Fn&& fn_) const
{
    if (const auto* fields = std::get_if<FixedFieldRegion>(&storage))
    {
        const size_t recordBytes = fieldInfo->fixedFieldBytes;
        for (size_t offset = 0; offset < fields->size(); offset += recordBytes)
        {
            if (!fn_(CompositeField::ViewFixedFields(fields->data() + offset, recordBytes, fieldInfo))) { return false; }
        }
        return true;
    }
    for (const auto& record : std::get<CompositeFieldArray>(storage))
    {
        if (!fn_(record)) { return false; }
    }
    return true;
}

template <bool Mutable>
std::conditional_t<Mutable, CompositeField*, const CompositeField*> FieldArray::FieldArrayRecordView<Mutable>::GetCompositeField() const
{
    auto* records = std::get_if<CompositeFieldArray>(&fieldArray->storage);
    return records != nullptr ? &records->at(index) : nullptr;
}

template <typename T> void FieldArray::SetFieldValue(const BaseField& fieldDef_, size_t index_, const T* values_, size_t count_)
{
    if (auto* records = std::get_if<CompositeFieldArray>(&storage))
    {
        records->at(index_).SetFieldValue(fieldDef_, values_, count_);
        return;
    }
    if (index_ >= size()) { throw std::runtime_error("SetFieldValue(): record index out of range"); }
    if (fieldDef_.type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY || fieldDef_.type == FIELD_TYPE::STRING ||
        fieldDef_.type == FIELD_TYPE::FIELD_ARRAY || fieldDef_.type == FIELD_TYPE::RESPONSE_STR)
    {
        throw std::runtime_error("SetFieldValue(): contiguous records do not support variable-length fields");
    }
    if (count_ != 0 && values_ == nullptr) { throw std::runtime_error("SetFieldValue(): source pointer is null"); }
    if (count_ != 0) { std::get<FixedFieldRegion>(storage).SetFieldValue(index_ * fieldInfo->fixedFieldBytes + fieldDef_.index, values_, count_); }
}

template <typename T> void FieldArray::SetFieldValue(const BaseField& fieldDef_, size_t index_, T&& value_)
{
    if (auto* records = std::get_if<CompositeFieldArray>(&storage))
    {
        records->at(index_).SetFieldValue(fieldDef_, std::forward<T>(value_));
        return;
    }

    using ValueT = std::decay_t<T>;
    if (fieldDef_.type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
    {
        if constexpr (std::is_same_v<ValueT, std::string> || is_specialization_of_v<ValueT, TypedBuffer> ||
                      is_specialization_of_v<ValueT, std::vector>)
        {
            if (index_ >= size()) { throw std::runtime_error("SetFieldValue(): record index out of range"); }
            std::get<FixedFieldRegion>(storage).SetFieldValue(index_ * fieldInfo->fixedFieldBytes + fieldDef_.index, std::forward<T>(value_));
            return;
        }
    }
    else if (fieldDef_.type == FIELD_TYPE::SIMPLE || fieldDef_.type == FIELD_TYPE::ENUM || fieldDef_.type == FIELD_TYPE::RESPONSE_ID)
    {
        if constexpr (std::is_trivially_copyable_v<ValueT> && !std::is_pointer_v<ValueT> && !is_specialization_of_v<ValueT, TypedBuffer>)
        {
            SetFieldValue(fieldDef_, index_, &value_, 1);
            return;
        }
    }
    throw std::runtime_error("SetFieldValue(): unsupported field or value type for flat field array");
}

template <bool Mutable>
template <bool Enabled, std::enable_if_t<Enabled && Mutable, int>>
CompositeField FieldArray::FieldArrayRecordView<Mutable>::ReplaceRecord(const CompositeField& record_) const
{
    if (auto* records = std::get_if<CompositeFieldArray>(&fieldArray->storage)) { return std::exchange(records->at(index), record_); }

    auto& fields = std::get<FixedFieldRegion>(fieldArray->storage);
    if (index >= fieldArray->size()) { throw std::out_of_range("ReplaceRecord(): index out of range"); }
    const size_t recordBytes = fieldArray->fieldInfo->fixedFieldBytes;
    if (!record_.GetVarFields().empty() || record_.GetFixedFields().size() != recordBytes)
    {
        throw std::runtime_error("FieldArray::ReplaceRecord(): record must have exactly the array's fixed fields and no variable-length fields");
    }
    CompositeField previous(fieldArray->fieldInfo);
    previous.SetFixedData(fields.data() + (index * recordBytes), recordBytes);
    fields.SetFieldValue(index * recordBytes, record_.GetFixedFields().data(), recordBytes);
    return previous;
}

} // namespace novatel::edie
