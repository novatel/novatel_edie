#pragma once

#include <memory>

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
    std::string name;
    bool hasPtype; // Whether the field has a specific Python type associated with it

    explicit PyField(std::string name_, bool hasPtype_, std::vector<FieldContainer> message_, py_common::PyMessageDatabaseCore::ConstPtr parentDb_)
        : name(std::move(name_)), hasPtype(hasPtype_), fields(std::move(message_)), parentDb(std::move(parentDb_)) {};

    //============================================================================
    //! \brief Creates a shallow dictionary representing the field.
    //!
    //! Subfields are left as objects instead of being converted to dictionaries.
    //!
    //! \return A dictionary containing the field values.
    //============================================================================
    nb::dict& to_shallow_dict() const;

    //============================================================================
    //! \brief Retrieves all subfield definitions indexed by name.
    //! \return A dictionary containing the fields.
    //============================================================================
    nb::dict& get_field_defs() const;

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

    //============================================================================
    //! \brief Retrieves the value of a field by its name using the subscript operator.
    //! \param field_name The name of the field to retrieve.
    //! \return The value of the specified field as an nb::object.
    //============================================================================
    nb::object getitem(nb::str field_name) const;

    //============================================================================
    //! \brief Checks if a field with the given name exists in the object.
    //! \param field_name The name of the field to check.
    //! \return True if the field exists, otherwise false.
    //============================================================================
    bool contains(nb::str field_name) const;

    //============================================================================
    //! \brief Retrieves the number of fields in the object.
    //! \return The number of fields as a size_t.
    //============================================================================
    size_t len() const;

    //============================================================================
    //! \brief Generates a string representation of the object.
    //! \return A string representing the object.
    //============================================================================
    std::string repr() const;

    std::vector<FieldContainer> fields;

  protected:
    mutable nb::dict cached_values_;
    mutable nb::dict cached_fields_;

    py_common::PyMessageDatabaseCore::ConstPtr parentDb;
};

nb::object convert_field(const FieldContainer& field, const py_common::PyMessageDatabaseCore::ConstPtr& parent_db, std::string parent,
                         bool has_ptype);
} // namespace novatel::edie::py_common
