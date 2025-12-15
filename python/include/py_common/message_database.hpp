#pragma once

#include <functional>
#include <memory>
#include <sstream>
#include <variant>

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/variant.h>

#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_common/py_logger.hpp"
#include "py_common/py_message_data.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_common {

std::unordered_map<std::string, std::function<nb::handle()>>& GetBasePythonTypes();
nb::handle GetBasePythonType(const std::string& message_style);

struct PyMessageType
{
    nb::object python_type;
    uint32_t crc;

    PyMessageType(nb::object python_type_, uint32_t crc_) : python_type(std::move(python_type_)), crc(crc_) {}
};

class PyMessageDatabaseCore;

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

    explicit PyField(std::string name_, bool hasPtype_, std::vector<FieldContainer> message_, std::shared_ptr<const PyMessageDatabaseCore> parentDb_)
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

    std::shared_ptr<const PyMessageDatabaseCore> parentDb;
};

nb::object convert_field(const FieldContainer& field, const std::shared_ptr<const PyMessageDatabaseCore>& parent_db, std::string parent,
                         bool has_ptype);

class PyMessageDatabaseCore : public MessageDatabase
{
  public:
    PyMessageDatabaseCore();
    PyMessageDatabaseCore(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_, std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_);
    explicit PyMessageDatabaseCore(const MessageDatabase& message_db) noexcept;
    explicit PyMessageDatabaseCore(const MessageDatabase&& message_db) noexcept;

    [[nodiscard]] const std::unordered_map<std::string, PyMessageType*>& GetMessagesByNameDict() const { return messages_by_name; }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetFieldsByNameDict() const { return fields_by_name; }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return enums_by_id; }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return enums_by_name; }

  private:
    void GenerateMessageMappings() override;
    void GenerateEnumMappings() override;
    //-----------------------------------------------------------------------
    //! \brief Creates Python Enums for each enum definition in the database.
    //!
    //! These classes are stored by ID in the enums_by_id map and by name in the enums_by_name map.
    //-----------------------------------------------------------------------
    void UpdatePythonEnums();
    //-----------------------------------------------------------------------
    //! \brief Creates Python types for each component of all message definitions in the database.
    //!
    //! A message named "MESSAGE" will be mapped to a Python class named "MESSAGE".
    //! A field of that payload named "FIELD" will be mapped to a class named "MESSAGE_FIELD_Field".
    //! A subfield of that field named "SUBFIELD" will be mapped to a class named "MESSAGE_FIELD_Field_SUBFIELD_Field".
    //!
    //! These classes are stored by name in the messages_by_name map.
    //-----------------------------------------------------------------------
    void UpdatePythonMessageTypes();
    void AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, nb::handle type_cons, nb::handle type_tuple,
                      nb::handle type_dict);

    std::unordered_map<std::string, PyMessageType*> messages_by_name{};
    std::unordered_map<std::string, nb::object> fields_by_name{};

    std::unordered_map<std::string, nb::object> enums_by_id{};
    std::unordered_map<std::string, nb::object> enums_by_name{};

  public:
    using Ptr = std::shared_ptr<PyMessageDatabaseCore>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabaseCore>;
};

// ============================================================================
// Helper Functions
// ============================================================================

inline void cleanString(std::string& str)
{
    // Remove special characters from the string to make it a valid python attribute name
    for (char& c : str)
    {
        if (!isalnum(c)) { c = '_'; }
    }
    if (isdigit(str[0])) { str = "_" + str; }
}

inline std::unordered_map<std::string, std::function<nb::handle()>>& GetBasePythonTypes()
{
    static std::unordered_map<std::string, std::function<nb::handle()>> nameToTypeGetter;
    return nameToTypeGetter;
}

inline nb::handle GetBasePythonType(const std::string& message_style)
{
    std::unordered_map<std::string, std::function<nb::handle()>>& typeGetters = GetBasePythonTypes();
    auto it = typeGetters.find(message_style);
    if (it == typeGetters.end())
    {
        // Newline
        throw FailureException("Unrecognized python message style.");
    }
    return it->second();
}

// ============================================================================
// PyField Inline Implementations
// ============================================================================

inline nb::object convert_field(const FieldContainer& field, const std::shared_ptr<const PyMessageDatabaseCore>& parent_db, std::string parent,
                                bool has_ptype)
{
    if (field.fieldDef->type == FIELD_TYPE::ENUM)
    {
        // Handle Enums
        const std::string& enumId = static_cast<const EnumField*>(field.fieldDef.get())->enumId;
        auto it = parent_db->GetEnumsByIdDict().find(enumId);
        if (it == parent_db->GetEnumsByIdDict().end())
        {
            throw std::runtime_error("Enum definition for " + field.fieldDef->name + " field with ID '" + enumId +
                                     "' not found in the JSON database");
        }
        nb::object enum_type = it->second;
        return std::visit([&](auto&& value) { return enum_type(value); }, field.fieldValue);
    }
    else if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
    {
        const auto& message_field = std::get<std::vector<FieldContainer>>(field.fieldValue);
        if (message_field.empty())
        {
            // Handle Empty Arrays
            return nb::list();
        }
        else if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
        {
            // Handle Field Arrays
            nb::handle field_ptype;
            std::string field_name;
            if (has_ptype)
            {
                // If a parent type name is provided, get a field type based on the parent and field name
                field_name = parent + "_" + field.fieldDef->name + "_Field";
                try
                {
                    field_ptype = parent_db->GetFieldsByNameDict().at(field_name);
                }
                catch (const std::out_of_range& e)
                {
                    // This case should never happen, if it does there is a bug
                    throw std::runtime_error("Field type not found for " + field_name);
                }
            }
            else
            {
                // If field has no ptype, use the generic "Field" type
                field_name = std::move(parent);
                field_ptype = nb::type<PyField>();
            }

            // Create an appropriate PyField instance for each subfield in the array
            std::vector<nb::object> sub_values;
            sub_values.reserve(message_field.size());
            for (const auto& subfield : message_field)
            {
                nb::object pyinst = nb::inst_alloc(field_ptype);
                PyField* cinst = nb::inst_ptr<PyField>(pyinst);
                const auto& message_subfield = std::get<std::vector<FieldContainer>>(subfield.fieldValue);
                new (cinst) PyField(field_name, has_ptype, message_subfield, parent_db);
                nb::inst_mark_ready(pyinst);
                sub_values.push_back(pyinst);
            }
            return nb::cast(sub_values);
        }
        else
        {
            // Handle Fixed or Variable-Length Arrays
            if (field.fieldDef->conversion == "%s")
            {
                // The array is actually a string
                std::string str;
                str.reserve(message_field.size());
                for (const auto& sub_field : message_field)
                {
                    auto c = std::get<uint8_t>(sub_field.fieldValue);
                    if (c == 0) { break; }
                    str.push_back(c);
                }
                return nb::cast(str);
            }
            std::vector<nb::object> sub_values;
            sub_values.reserve(message_field.size());
            for (const auto& f : message_field) { sub_values.push_back(convert_field(f, parent_db, parent, has_ptype)); }
            return nb::cast(sub_values);
        }
    }
    else if (field.fieldDef->conversion == "%id")
    {
        // Handle Satellite IDs
        const uint32_t temp_id = std::get<uint32_t>(field.fieldValue);
        SatelliteId sat_id;
        sat_id.usPrnOrSlot = temp_id & 0x0000FFFF;
        sat_id.sFrequencyChannel = (temp_id & 0xFFFF0000) >> 16;
        return nb::cast(sat_id);
    }
    else
    {
        // Handle most types by simply extracting the value from the variant and casting
        return std::visit([](auto&& value) { return nb::cast(value); }, field.fieldValue);
    }
}

inline nb::dict& PyField::to_shallow_dict() const
{
    if (cached_values_.size() == 0)
    {
        for (const auto& field : fields)
        {
            if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue) &&
                (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY || field.fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY))
            {
                std::vector<FieldContainer> field_array = std::get<std::vector<FieldContainer>>(field.fieldValue);
                cached_values_[nb::cast(field.fieldDef->name + "_length")] = field_array.size();
            }
            cached_values_[nb::cast(field.fieldDef->name)] = convert_field(field, parentDb, this->name, this->hasPtype);
        }
    }
    return cached_values_;
}

inline nb::list PyField::get_field_names() const
{
    nb::list field_names = nb::list();
    for (const auto& [name, value] : to_shallow_dict()) { field_names.append(name); }
    return field_names;
}

inline nb::list PyField::get_values() const
{
    nb::list values = nb::list();
    nb::dict& unordered_values = to_shallow_dict();
    for (const auto& field_name : get_field_names()) { values.append(unordered_values[field_name]); }
    return values;
}

inline nb::list PyField::to_list() const
{
    nb::list list = nb::list();
    for (const auto& [field_name, value] : to_shallow_dict())
    {
        if (nb::isinstance<PyField>(value)) { list.append(nb::cast<PyField>(value).to_list()); }
        else if (nb::isinstance<std::vector<nb::object>>(value))
        {
            nb::list sublist;
            for (const auto& sub_item : nb::cast<std::vector<nb::object>>(value))
            {
                if (nb::isinstance<PyField>(sub_item)) { sublist.append(nb::cast<PyField>(sub_item).to_list()); }
                else { sublist.append(sub_item); }
            }
            list.append(sublist);
        }
        else { list.append(value); }
    }
    return list;
}

inline nb::dict PyField::to_dict() const
{
    nb::dict dict;
    for (const auto& [field_name, value] : to_shallow_dict())
    {
        if (nb::isinstance<PyField>(value)) { dict[field_name] = nb::cast<PyField>(value).to_dict(); }
        else if (nb::isinstance<std::vector<nb::object>>(value))
        {
            nb::list list;
            for (const auto& sub_item : nb::cast<std::vector<nb::object>>(value))
            {
                if (nb::isinstance<PyField>(sub_item)) { list.append(nb::cast<PyField>(sub_item).to_dict()); }
                else { list.append(sub_item); }
            }
            dict[field_name] = list;
        }
        else if (nb::isinstance<SatelliteId>(value)) { dict[field_name] = value.attr("to_dict")(); }
        else { dict[field_name] = value; }
    }
    return dict;
}

inline nb::object PyField::getattr(nb::str field_name) const
{
    if (!contains(field_name)) { throw nb::attribute_error(field_name.c_str()); }
    return to_shallow_dict()[std::move(field_name)];
}

inline nb::object PyField::getitem(nb::str field_name) const { return to_shallow_dict()[std::move(field_name)]; }

inline bool PyField::contains(nb::str field_name) const { return to_shallow_dict().contains(std::move(field_name)); }

inline size_t PyField::len() const { return fields.size(); }

inline std::string PyField::repr() const
{
    std::stringstream repr;
    repr << name << "(";
    bool first = true;
    for (const auto& [field_name, value] : to_shallow_dict())
    {
        if (!first) { repr << ", "; }
        first = false;
        repr << nb::str("{}={!r}").format(field_name, value).c_str();
    }
    repr << ")";
    return repr.str();
}

// ============================================================================
// PyMessageDatabaseCore Inline Implementations
// ============================================================================

inline PyMessageDatabaseCore::PyMessageDatabaseCore()
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

inline PyMessageDatabaseCore::PyMessageDatabaseCore(std::vector<MessageDefinition::ConstPtr> vMessageDefinitions_,
                                                    std::vector<EnumDefinition::ConstPtr> vEnumDefinitions_)
    : MessageDatabase(std::move(vMessageDefinitions_), std::move(vEnumDefinitions_))
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

inline PyMessageDatabaseCore::PyMessageDatabaseCore(const MessageDatabase& message_db) noexcept : MessageDatabase(message_db)
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

inline PyMessageDatabaseCore::PyMessageDatabaseCore(const MessageDatabase&& message_db) noexcept : MessageDatabase(message_db)
{
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

inline void PyMessageDatabaseCore::GenerateMessageMappings()
{
    MessageDatabase::GenerateMessageMappings();
    UpdatePythonMessageTypes();
}

inline void PyMessageDatabaseCore::GenerateEnumMappings()
{
    MessageDatabase::GenerateEnumMappings();
    UpdatePythonEnums();
}

inline void PyMessageDatabaseCore::UpdatePythonEnums()
{
    nb::object IntEnum = nb::module_::import_("enum").attr("IntEnum");
    enums_by_id.clear();
    enums_by_name.clear();
    for (const auto& enum_def : EnumDefinitions())
    {
        nb::dict values;
        const char* enum_name = enum_def->name.c_str();
        for (const auto& enumerator : enum_def->enumerators)
        {
            std::string enumerator_name = enumerator.name;
            cleanString(enumerator_name);
            values[enumerator_name.c_str()] = enumerator.value;
        }
        nb::object enum_type = IntEnum(enum_name, values);
        enum_type.attr("_name") = enum_name;
        enum_type.attr("_id") = enum_def->_id;
        enums_by_id[enum_def->_id.c_str()] = enum_type;
        enums_by_name[enum_name] = enum_type;
    }
}

inline void PyMessageDatabaseCore::AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name, nb::handle type_constructor,
                                                nb::handle type_tuple, nb::handle type_dict)
{
    // rescursively add field types for each field array element within the provided vector
    for (const auto& field : fields)
    {
        if (field->type == FIELD_TYPE::FIELD_ARRAY)
        {
            auto* field_array_field = dynamic_cast<FieldArrayField*>(field.get());
            std::string field_name = base_name + "_" + field_array_field->name + "_Field";
            nb::object field_type = type_constructor(field_name, type_tuple, type_dict);
            fields_by_name[field_name] = field_type;
            AddFieldType(field_array_field->fields, field_name, type_constructor, type_tuple, type_dict);
        }
    }
}

inline void PyMessageDatabaseCore::UpdatePythonMessageTypes()
{
    // clear existing definitions
    messages_by_name.clear();

    // get type constructor
    nb::object type_constructor = nb::module_::import_("builtins").attr("type");
    // specify the python superclass for the message body types
    nb::tuple field_type_tuple = nb::make_tuple(nb::type<PyField>());
    // provide no additional attributes via `__dict__`
    nb::dict type_dict = nb::dict();

    // add message and message body types for each message definition
    for (const auto& message_def : MessageDefinitions())
    {
        // specify the python superclass for the message type
        nb::handle python_message_type;
        try
        {
            python_message_type = GetBasePythonType(message_def->messageStyle);
        }
        catch (std::exception&)
        {
            throw FailureException("Failed");
        }
        nb::tuple message_type_tuple = nb::make_tuple(python_message_type);
        uint32_t crc = message_def->latestMessageCrc;
        nb::object msg_type_def = type_constructor(message_def->name, message_type_tuple, type_dict);
        messages_by_name[message_def->name] = new PyMessageType(msg_type_def, crc);
        // add additional MessageBody types for each field array element within the message definition
        AddFieldType(message_def->fields.at(crc), message_def->name, type_constructor, field_type_tuple, type_dict);
    }
}

} // namespace novatel::edie::py_common
