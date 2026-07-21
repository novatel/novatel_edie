#pragma once

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "novatel_edie/decoders/common/message_decoder.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/message_database.hpp"
#include "py_common/py_logger.hpp"
#include "py_common/py_message_data.hpp"

namespace novatel::edie::py_common {

using ssize_t = std::make_signed_t<size_t>;

// Either we own a CompositeField, or we borrow it from an external
// parent (held alive by an nb::object reference).
using OwnedFields = std::unique_ptr<CompositeField>;
using FieldStorage = std::variant<OwnedFields, nb::object>;
using OwnedFieldArrayData = std::unique_ptr<FieldValueVariant>;

class PyFieldArray;

//============================================================================
//! \class PyField
//! \brief A python representation for a single message or message field.
//!
//! Exposes the members of a CompositeField like Python attributes.
//============================================================================
struct PyField
{
    explicit PyField() : storage(std::make_unique<CompositeField>()), fieldsPtr(std::get<OwnedFields>(storage).get()), myFieldIndex(0) {}

    // Standalone subfield constructor
    explicit PyField(CompositeField message_, ::novatel::edie::FieldArrayField::ConstPtr fieldDef_,
                     py_common::PyMessageDatabase::ConstPtr parentDb_)
                : storage(std::make_unique<CompositeField>(std::move(message_))), fieldDef(std::static_pointer_cast<const BaseField>(fieldDef_)),
                    fieldInfo(fieldDef_ ? fieldDef_->fieldInfo : nullptr), parentDb(std::move(parentDb_))
    {
        auto& ptr = std::get<OwnedFields>(storage);
        fieldsPtr = ptr.get();
        fieldNameMap_ = fieldDef_ ? parentDb->GetFieldNameMap(std::static_pointer_cast<const BaseField>(fieldDef_).get()) : nullptr;
        cachedArrays_.resize(fieldInfo ? fieldInfo->messageOrderedFields.size() : 0);
    };

    // Full message constructor
    explicit PyField(CompositeField message_, const ::novatel::edie::MessageDefinition* msgDef_, uint32_t crc_,
                     py_common::PyMessageDatabase::ConstPtr parentDb_)
        : storage(std::make_unique<CompositeField>(std::move(message_))), parentDb(std::move(parentDb_))
    {
        auto& ptr = std::get<OwnedFields>(storage);
        fieldsPtr = ptr.get();
        fieldInfo = fieldsPtr ? fieldsPtr->GetFieldInfo() : nullptr;
        fieldNameMap_ = msgDef_ ? parentDb->GetMessageFieldNameMap(msgDef_, crc_) : nullptr;
        cachedArrays_.resize(fieldInfo ? fieldInfo->messageOrderedFields.size() : 0);
    };

    // FieldArray subfield constructor
    explicit PyField(size_t index_, ::novatel::edie::FieldArrayField::ConstPtr fieldDef_,
                     py_common::PyMessageDatabase::ConstPtr parentDb_, nb::object parentField_)
                : storage(std::move(parentField_)), fieldDef(std::static_pointer_cast<const BaseField>(fieldDef_)),
                    fieldInfo(fieldDef_ ? fieldDef_->fieldInfo : nullptr), myFieldIndex(index_), parentDb(std::move(parentDb_))
    {
                fieldNameMap_ = fieldDef_ ? parentDb->GetFieldNameMap(std::static_pointer_cast<const BaseField>(fieldDef_).get()) : nullptr;
                cachedArrays_.resize(fieldInfo ? fieldInfo->messageOrderedFields.size() : 0);
    };

    // Default-constructed field standalone (used for FIELD_ARRAY sub-fields when
    // building a default message). Populates `fields` with zero-initialised values.
    explicit PyField(novatel::edie::FieldArrayField::ConstPtr fieldDef_, py_common::PyMessageDatabase::ConstPtr parentDb_)
        : PyField(CompositeField(fieldDef_->fieldInfo), fieldDef_, std::move(parentDb_)) {};

    // Default-constructed whole message — used by Message(...) __new__.
    explicit PyField(const novatel::edie::MessageDefinition* msgDef_, uint32_t crc_, py_common::PyMessageDatabase::ConstPtr parentDb_)
    // TODO: make the following map lookup more robust
        : PyField(CompositeField(msgDef_->fieldInfo.at(crc_)), msgDef_, crc_, std::move(parentDb_)) {};

    // Python `__new__`: allocates and placement-news a PyField for the given class,
    // resolving its BaseField definition and owning MessageDatabase from the class's
    // `_owner_db` attribute, then assigns each keyword argument to the matching
    // subfield. Bound as the type's __new__; the type has no Python-level __init__
    // (it is repointed to object.__init__) so all work happens here.
    static nb::object py_new(nb::handle cls, nb::kwargs kwargs);

    // Assigns a single field by name. Supports SIMPLE / ENUM / STRING only; other
    // FIELD_TYPEs and DATA_TYPEs raise nb::attribute_error.
    void setattr(nb::str field_name, nb::handle value);

    //============================================================================
    //! \brief Creates a shallow dictionary representing the field.
    //!
    //! Subfields are left as objects instead of being converted to dictionaries.
    //!
    //! \return A dictionary containing the field values.
    //============================================================================
    nb::dict& to_shallow_dict() const;

    //============================================================================
    //! \brief Retrieves the names of the subfields in order.
    //! \return A list containing the field names.
    //============================================================================
    nb::list get_field_names() const;

    //============================================================================
    //! \brief Retrieves the values of the subfields in the order they appear.
    //! \return A list containing the ordered field values.
    //============================================================================
    nb::list get_values() const;

    //============================================================================
    //! \brief Converts the object to a dictionary representation.
    //!
    //! Subfields are converted to dictionaries.
    //!
    //! \return A dictionary containing the object's data.
    //============================================================================
    nb::dict to_dict() const;

    //============================================================================
    //! \brief Converts the object to a list representation.
    //! \return A list of simple values and nested lists containing the object's data.
    //============================================================================
    nb::list to_list() const;

    //============================================================================
    //! \brief Retrieves the value of a field by its name.
    //! \param field_name The name of the field to retrieve.
    //! \return The value of the specified field as an nb::object.
    //============================================================================
    nb::object getattr(nb::str field_name) const;

  protected:
    CompositeField* GetCompositeField() const;

    nb::object convert_field(const BaseField& field) const;
    nb::object resolve_entry(const FieldLookupEntry& entry) const;

    // Reference to the backing field vector. Only valid when this field owns its
    // storage — true for top-level messages/responses, which are built with
    // owned storage and never borrow from a parent.
    CompositeField& owned_fields() const { return *std::get<OwnedFields>(storage); }

    // Take ownership of `fields` as this field's backing storage. Used to detach a
    // field from a parent array it was previously borrowing from.
    void take_ownership(CompositeField&& mb_)
    {
        auto owned = std::make_unique<CompositeField>(std::move(mb_));
        fieldsPtr = owned.get();
        storage = std::move(owned);
    }

    // Pointer to the live cached FieldArray wrapper for subfield `index`, or nullptr
    // when none is cached there or the cached wrapper has already been collected.
    PyFieldArray* cached_array(size_t index) const;

    template <typename Fn> void for_each_entry(Fn&& visitor) const;

    static nb::object unwrap_for_list(nb::object value);
    static nb::object unwrap_for_dict(nb::object value);

    template <bool Fixed, typename T> void set_field_value(size_t ind_, T* val_, size_t n_ = 1);
    template <bool Fixed> void set_regular_array(const std::shared_ptr<const ArrayField>& arrFieldDef_, nb::handle value_);

    friend class PyFieldArray;

  protected:
    FieldStorage storage;
    BaseField::ConstPtr fieldDef;
    FieldInfo::ConstPtr fieldInfo;
    CompositeField* fieldsPtr;
    size_t myFieldIndex; // Index of this field in the parent message's field vector, or 0 if no parent PyFieldArray.
    const FieldNameMap* fieldNameMap_{nullptr};
    mutable nb::dict cached_values_;
    mutable std::vector<std::optional<nb::weakref>> cachedArrays_;

    py_common::PyMessageDatabase::ConstPtr parentDb;
};

template <typename Fn> void PyField::for_each_entry(Fn&& visitor) const
{
    const auto& orderedFields = fieldInfo->messageOrderedFields;
    for (size_t i = 0; i < orderedFields.size(); i++)
    {
        const auto& def = orderedFields[i];
        if (def->type == FIELD_TYPE::FIELD_ARRAY || def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
        {
            visitor(def->name + "_length", resolve_entry(FieldLookupEntry{i, true}));
        }
        visitor(def->name, convert_field(*def));
    }
}

//============================================================================
//! \class PyFieldArray
//! \brief A python representation for an array of repeated message fields.
//============================================================================
class PyFieldArray
{
  public:
    explicit PyFieldArray(nb::list values);

    explicit PyFieldArray(FieldValueVariant& data_, ::novatel::edie::FieldArrayField::ConstPtr fieldDef_,
                          py_common::PyMessageDatabase::ConstPtr parentDb_, nb::object parent_)
                : storage(std::move(parent_)), dataPtr(&data_), fieldDef(std::move(fieldDef_)), parentDb(std::move(parentDb_))
    {
        length = std::visit(
            [](auto& v) -> size_t {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, FlatFieldArray> || std::is_same_v<T, CompositeFieldArray>)
                {
                    return v.size();
                }
                else { throw std::runtime_error("PyFieldArray: data_ is not a FlatFieldArray or CompositeFieldArray"); }
            },
            data_);
        cache.resize(length);
    };

    // This is a wrapper object that should be managed by pointer and not copied or moved around
    PyFieldArray(const PyFieldArray&) = delete;
    PyFieldArray(PyFieldArray&&) = delete;
    PyFieldArray& operator=(const PyFieldArray&) = delete;
    PyFieldArray& operator=(PyFieldArray&&) = delete;

    // Python-facing operations bound by init_field_objects.
    nb::object getitem(ssize_t index) const;
    void setitem(ssize_t index, nb::object value);
    size_t len() const;

  private:
    // Normalize a Python-style (possibly negative) index into an in-bounds unsigned index.
    std::optional<size_t> resolve_index(ssize_t signedIndex) const;

    // Take ownership of `fields` as this array's backing storage, repointing `data`
    // at the newly-owned vector. Used to seed a freshly-constructed array or to
    // detach an array from a parent it was previously borrowing from.
    void take_ownership(FieldValueVariant&& fields)
    {
        auto owned = std::make_unique<FieldValueVariant>(std::move(fields));
        dataPtr = owned.get();
        length = std::visit(
            [](auto& v) -> size_t {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, FlatFieldArray> || std::is_same_v<T, CompositeFieldArray>)
                {
                    return v.size();
                }
                else { throw std::runtime_error("take_ownership: data_ is not a FlatFieldArray or CompositeFieldArray"); }
            },
            *dataPtr);
        storage = std::move(owned);
    }

    // Pointer to the live cached element wrapper at `index`, or nullptr when none is
    // cached there or the cached wrapper has already been collected.
    PyField* cached_element(size_t index) const;

    // Data lifetime
    std::variant<OwnedFieldArrayData, nb::object> storage;

    // Data access
    FieldValueVariant* dataPtr;
    size_t length;
    mutable std::vector<std::optional<nb::weakref>> cache;

    // Database info
    FieldArrayField::ConstPtr fieldDef;
    py_common::PyMessageDatabase::ConstPtr parentDb;

    // PyField constructs field-array wrappers and adjusts their fieldDef/data when
    // binding a value to, or detaching one from, a FIELD_ARRAY message field.
    friend struct PyField;
};

} // namespace novatel::edie::py_common
