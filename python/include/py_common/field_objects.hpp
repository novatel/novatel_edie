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

//============================================================================
//! \class PyField
//! \brief A python representation for a single message or message field.
//!
//! Contains a vector of FieldContainer objects, which behave like attributes
//! within the Python API.
//============================================================================
struct PyField
{
    explicit PyField(std::vector<FieldContainer> message_, const ::novatel::edie::BaseField* fieldDef_,
                     py_common::PyMessageDatabaseCore::ConstPtr parentDb_)
        : fields(std::move(message_)), fieldDef(fieldDef_), parentDb(std::move(parentDb_))
    {
        fieldsPtr = fields.data();
        fieldCount = fields.size();
        fieldNameMap_ = fieldDef_ ? parentDb->GetFieldNameMap(fieldDef_) : nullptr;
        cachedArrays_.resize(fieldCount);
    };

    explicit PyField(std::vector<FieldContainer> message_, const ::novatel::edie::MessageDefinition* msgDef_, uint32_t crc_,
                     py_common::PyMessageDatabaseCore::ConstPtr parentDb_)
        : fields(std::move(message_)), parentDb(std::move(parentDb_))
    {
        fieldsPtr = fields.data();
        fieldCount = fields.size();
        fieldNameMap_ = msgDef_ ? parentDb->GetMessageFieldNameMap(msgDef_, crc_) : nullptr;
        cachedArrays_.resize(fieldCount);
    };

    explicit PyField(std::vector<FieldContainer>& message_, const ::novatel::edie::BaseField* fieldDef_,
                     py_common::PyMessageDatabaseCore::ConstPtr parentDb_, nb::object parentField_)
        : fieldDef(fieldDef_), parentDb(std::move(parentDb_)), parent(std::move(parentField_))
    {
        fieldsPtr = message_.data();
        fieldCount = message_.size();
        fieldNameMap_ = fieldDef_ ? parentDb->GetFieldNameMap(fieldDef_) : nullptr;
        cachedArrays_.resize(fieldCount);
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

    std::vector<FieldContainer> fields;
    const BaseField* fieldDef{nullptr};

  protected:
    nb::object convert_field(FieldContainer& field) const;
    nb::object resolve_entry(const FieldLookupEntry& entry) const;

    template <typename Fn> void for_each_entry(Fn&& visitor) const;

    static nb::object unwrap_for_list(nb::object value);
    static nb::object unwrap_for_dict(nb::object value);

    FieldContainer* fieldsPtr;
    size_t fieldCount;
    const FieldNameMap* fieldNameMap_{nullptr};
    nb::object parent;
    mutable nb::dict cached_values_;
    mutable std::vector<std::optional<nb::weakref>> cachedArrays_;

    py_common::PyMessageDatabaseCore::ConstPtr parentDb;
};

template <typename Fn> void PyField::for_each_entry(Fn&& visitor) const
{
    for (size_t i = 0; i < fieldCount; i++)
    {
        const auto& def = fieldsPtr[i].fieldDef;
        if (def->type == FIELD_TYPE::FIELD_ARRAY || def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
        {
            auto& arr = std::get<std::vector<FieldContainer>>(fieldsPtr[i].fieldValue);
            visitor(def->name + "_length", nb::cast(arr.size()));
        }
        visitor(def->name, convert_field(fieldsPtr[i]));
    }
}

//============================================================================
//! \class PyFieldArray
//! \brief A python representation for an array of repeated message fields.
//============================================================================
struct PyFieldArray
{
    explicit PyFieldArray(std::vector<FieldContainer>& data_, const ::novatel::edie::BaseField* fieldDef_,
                          py_common::PyMessageDatabaseCore::ConstPtr parentDb_, nb::object parent_)
        : data(&data_), fieldDef(fieldDef_), parentDb(std::move(parentDb_)), parent(std::move(parent_)), cache(data_.size()) {};

    nb::object getitem(size_t index) const;
    size_t len() const;

  private:
    std::vector<FieldContainer>* data;
    const BaseField* fieldDef{nullptr};
    py_common::PyMessageDatabaseCore::ConstPtr parentDb;
    nb::object parent;
    mutable std::vector<std::optional<nb::weakref>> cache;
};

} // namespace novatel::edie::py_common
