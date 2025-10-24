#include "py_common/field_objects.hpp"

#include <variant>

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "py_common/bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

nb::object py_common::convert_field(const FieldContainer& field, const py_common::PyMessageDatabaseCore::ConstPtr& parent_db, std::string parent,
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
            for (const auto& f : message_field) { sub_values.push_back(py_common::convert_field(f, parent_db, parent, has_ptype)); }
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

nb::dict& PyField::to_shallow_dict() const
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
            cached_values_[nb::cast(field.fieldDef->name)] = convert_field(field, parent_db_, this->name, this->has_ptype);
        }
    }
    return cached_values_;
}

nb::list PyField::get_field_names() const
{
    nb::list field_names = nb::list();
    for (const auto& [name, value] : to_shallow_dict()) { field_names.append(name); }
    return field_names;
}

nb::list PyField::get_values() const
{
    nb::list values = nb::list();
    nb::dict& unordered_values = to_shallow_dict();
    for (const auto& field_name : get_field_names()) { values.append(unordered_values[field_name]); }
    return values;
}

nb::list PyField::to_list() const
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

nb::dict PyField::to_dict() const
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

nb::object PyField::getattr(nb::str field_name) const
{
    if (!contains(field_name)) { throw nb::attribute_error(field_name.c_str()); }
    return to_shallow_dict()[std::move(field_name)];
}

nb::object PyField::getitem(nb::str field_name) const { return to_shallow_dict()[std::move(field_name)]; }

bool PyField::contains(nb::str field_name) const { return to_shallow_dict().contains(std::move(field_name)); }

size_t PyField::len() const { return fields.size(); }

std::string PyField::repr() const
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
