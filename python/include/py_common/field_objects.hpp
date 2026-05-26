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
//! Contains a vector of FieldContainer objects, which behave like attributes
//! within the Python API.
//============================================================================
struct PyField
{
    explicit PyField() {}

    explicit PyField(std::vector<FieldContainer> message_, ::novatel::edie::BaseField::ConstPtr fieldDef_,
                     py_common::PyMessageDatabase::ConstPtr parentDb_)
        : fields(std::move(message_)), fieldDef(std::move(fieldDef_)), parentDb(std::move(parentDb_))
    {
        fieldsPtr = fields.data();
        fieldCount = fields.size();
        fieldNameMap_ = fieldDef ? parentDb->GetFieldNameMap(fieldDef.get()) : nullptr;
        cachedArrays_.resize(fieldCount);
    };

    explicit PyField(std::vector<FieldContainer> message_, const ::novatel::edie::MessageDefinition* msgDef_, uint32_t crc_,
                     py_common::PyMessageDatabase::ConstPtr parentDb_)
        : fields(std::move(message_)), parentDb(std::move(parentDb_))
    {
        fieldsPtr = fields.data();
        fieldCount = fields.size();
        fieldNameMap_ = msgDef_ ? parentDb->GetMessageFieldNameMap(msgDef_, crc_) : nullptr;
        cachedArrays_.resize(fieldCount);
    };

    explicit PyField(std::vector<FieldContainer>& message_, ::novatel::edie::BaseField::ConstPtr fieldDef_,
                     py_common::PyMessageDatabase::ConstPtr parentDb_, nb::object parentField_)
        : fieldDef(std::move(fieldDef_)), parentDb(std::move(parentDb_)), parent(std::move(parentField_))
    {
        fieldsPtr = message_.data();
        fieldCount = message_.size();
        fieldNameMap_ = fieldDef ? parentDb->GetFieldNameMap(fieldDef.get()) : nullptr;
        cachedArrays_.resize(fieldCount);
    };

    // Default-constructed field standalone (used for FIELD_ARRAY sub-fields when
    // building a default message). Populates `fields` with zero-initialised values.
    explicit PyField(::novatel::edie::BaseField::ConstPtr fieldDef_, py_common::PyMessageDatabase::ConstPtr parentDb_)
        : PyField(BuildDefaultFields(fieldDef_.get()), fieldDef_, std::move(parentDb_)) {};

    // Default-constructed whole message — used by Message(...) __new__.
    explicit PyField(const ::novatel::edie::MessageDefinition* msgDef_, uint32_t crc_, py_common::PyMessageDatabase::ConstPtr parentDb_)
        : PyField(BuildDefaultFields(msgDef_, crc_), msgDef_, crc_, std::move(parentDb_)) {};

    // Builds a vector of FieldContainer instances initialised to type-appropriate
    // defaults. Used by the default-construction PyField ctors above.
    static std::vector<FieldContainer> BuildDefaultFields(const std::vector<BaseField::Ptr>& fieldDefs);
    static std::vector<FieldContainer> BuildDefaultFields(const ::novatel::edie::BaseField* fieldDef);
    static std::vector<FieldContainer> BuildDefaultFields(const ::novatel::edie::MessageDefinition* msgDef, uint32_t crc);

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
    nb::object convert_field(FieldContainer& field) const;
    nb::object resolve_entry(const FieldLookupEntry& entry) const;

    template <typename Fn> void for_each_entry(Fn&& visitor) const;

    static nb::object unwrap_for_list(nb::object value);
    static nb::object unwrap_for_dict(nb::object value);

    static std::vector<FieldContainer> get_regular_array(const std::shared_ptr<const ArrayField>& fixedArrDef, nb::handle value);

    friend struct PyFieldArray;

  protected:
    std::vector<FieldContainer> fields;
    BaseField::ConstPtr fieldDef;
    FieldContainer* fieldsPtr;
    size_t fieldCount;
    const FieldNameMap* fieldNameMap_{nullptr};
    nb::object parent;
    mutable nb::dict cached_values_;
    mutable std::vector<std::optional<nb::weakref>> cachedArrays_;

    py_common::PyMessageDatabase::ConstPtr parentDb;
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
    explicit PyFieldArray(nb::list values);

    explicit PyFieldArray(std::vector<FieldContainer>& data_, ::novatel::edie::BaseField::ConstPtr fieldDef_,
                          py_common::PyMessageDatabase::ConstPtr parentDb_, nb::object parent_)
        : data(&data_), fieldDef(std::move(fieldDef_)), parentDb(std::move(parentDb_)), parent(std::move(parent_)), cache(data_.size()) {};

    nb::object getitem(ssize_t index) const;
    size_t len() const;

    // Data lifetime control
    nb::object parent;
    std::vector<FieldContainer> ownedData;

    // Data manipulation/access point
    std::vector<FieldContainer>* data;

    BaseField::ConstPtr fieldDef;
    py_common::PyMessageDatabase::ConstPtr parentDb;
    mutable std::vector<std::optional<nb::weakref>> cache;
};

} // namespace novatel::edie::py_common
