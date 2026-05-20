#pragma once

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "novatel_edie/decoders/common/message_decoder.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/message_database.hpp"
#include "py_common/py_logger.hpp"
#include "py_common/py_message_data.hpp"

namespace novatel::edie::py_common {

using ssize_t = std::make_signed_t<size_t>;

//============================================================================
//! \class PyField
//! \brief A python representation for a single message or message field.
//!
//! Contains a MessageBody object and exposes its fields like Python attributes.
//============================================================================
struct PyField
{
    //! Construct the root message object that owns the decoded MessageBody.
    explicit PyField(MessageBody message_, py_common::PyMessageDatabase::ConstPtr parentDb_)
        : fields(std::move(message_)), msgDef(fields.GetDefinition().get()), msgCrc(fields.GetDefinitionCrc().value_or(0)), parentDb(std::move(parentDb_))
    {
        fieldNameMap_ = msgDef ? parentDb->GetMessageFieldNameMap(msgDef, msgCrc) : nullptr;
        cachedArrays_.resize(GetOrderedFields().size());
    };

    //! Construct a field-array element view that references parent field-array storage.
    explicit PyField(const FieldValueVariant* parentData_, size_t parentIndex_, const ::novatel::edie::BaseField* fieldDef_,
                     py_common::PyMessageDatabase::ConstPtr parentDb_, nb::object parentField_)
        : parentData(parentData_), parentIndex(parentIndex_), parentFieldArrayDef(dynamic_cast<const FieldArrayField*>(fieldDef_)), fieldDef(fieldDef_),
          parentDb(std::move(parentDb_)), parent(std::move(parentField_))
    {
        fieldNameMap_ = fieldDef_ ? parentDb->GetFieldNameMap(fieldDef_) : nullptr;
        cachedArrays_.resize(GetOrderedFields().size());
    };

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

    MessageBody fields;
    const FieldValueVariant* parentData{nullptr};
    size_t parentIndex{0};
    const FieldArrayField* parentFieldArrayDef{nullptr};
    const BaseField* fieldDef{nullptr};
    const MessageDefinition* msgDef{nullptr};
    uint32_t msgCrc{0};

  protected:
    nb::object convert_field(const BaseField& field) const;
    nb::object resolve_entry(const FieldLookupEntry& entry) const;
    const std::vector<BaseField::ConstPtr>& GetOrderedFields() const;
    const MessageBody* GetMessageBody() const;
    bool IsFlatElement() const;

    template <typename Fn> void for_each_entry(Fn&& visitor) const;

    static nb::object unwrap_for_list(nb::object value);
    static nb::object unwrap_for_dict(nb::object value);

    const FieldNameMap* fieldNameMap_{nullptr};
    nb::object parent;
    mutable nb::dict cached_values_;
    mutable std::vector<std::optional<nb::weakref>> cachedArrays_;

    py_common::PyMessageDatabase::ConstPtr parentDb;
};

template <typename Fn> void PyField::for_each_entry(Fn&& visitor) const
{
    const auto& orderedFields = GetOrderedFields();
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
struct PyFieldArray
{
    explicit PyFieldArray(const FieldValueVariant* data_, const ::novatel::edie::BaseField* fieldDef_,
                          py_common::PyMessageDatabase::ConstPtr parentDb_, nb::object parent_)
        : data(data_), fieldDef(dynamic_cast<const FieldArrayField*>(fieldDef_)), parentDb(std::move(parentDb_)), parent(std::move(parent_)) {};

    nb::object getitem(ssize_t index) const;
    size_t len() const;

    const FieldValueVariant* data{nullptr};
    const FieldArrayField* fieldDef{nullptr};
    py_common::PyMessageDatabase::ConstPtr parentDb;
    nb::object parent;
    mutable std::vector<std::optional<nb::weakref>> cache;
};

} // namespace novatel::edie::py_common
