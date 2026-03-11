#include "py_common/field_objects.hpp"

#include <cassert>
#include <type_traits>
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

PYCOMMON_EXPORT void PyField::buildFieldNameMap()
{
    for (size_t i = 0; i < fieldCount; i++)
    {
        const auto& def = fieldsPtr[i].fieldDef;
        fieldNameMap_[def->name] = {i, false};
        if (def->type == FIELD_TYPE::FIELD_ARRAY || def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
        {
            fieldNameMap_[def->name + "_length"] = {i, true};
        }
    }
}

PYCOMMON_EXPORT nb::object PyField::resolve_entry(const FieldLookupEntry& entry) const
{
    FieldContainer& field = fieldsPtr[entry.index];
    if (entry.is_length)
    {
        auto& arr = std::get<std::vector<FieldContainer>>(field.fieldValue);
        return nb::cast(arr.size());
    }
    return convert_field(field);
}

PYCOMMON_EXPORT nb::object py_common::PyField::convert_field(FieldContainer& field) const
{
    if (field.fieldDef->type == FIELD_TYPE::ENUM)
    {
        // Handle Enums
        const auto* enumField = static_cast<const EnumField*>(field.fieldDef.get());
        const EnumDefinition* enumDef = parentDb->GetEnumDefId(enumField->enumId).get();
        nb::object enum_type = parentDb->GetEnumType(enumField->enumDef.get());
        if (enum_type.is_none())
        {
            throw std::runtime_error("Enum definition for " + field.fieldDef->name + " field with ID '" + enumField->enumId +
                                     "' not found in the JSON database");
        }
        return std::visit(
            [&](auto&& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_integral_v<T>)
                {
                    const uint32_t key = static_cast<uint32_t>(value);
                    if (enumDef->valueName.count(key) > 0) { return enum_type(key); }
                    else { return enum_type(enumDef->unknownValue); }
                }
                else { return enum_type(enumDef->unknownValue); }
            },
            field.fieldValue);
    }
    else if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
    {
        auto& message_field = std::get<std::vector<FieldContainer>>(field.fieldValue);
        if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
        {
            // Handle Field Arrays — find the index of this field for caching
            size_t fieldIdx = static_cast<size_t>(&field - fieldsPtr);

            // Check if a cached PyFieldArray is still alive
            if (cachedArrays_[fieldIdx].has_value())
            {
                nb::object existing = cachedArrays_[fieldIdx].value()();
                if (!existing.is_none()) { return existing; }
            }

            // Construct a new PyFieldArray
            nb::object pyArr = nb::cast(PyFieldArray(message_field, field.fieldDef.get(), parentDb, nb::cast(this, nb::rv_policy::none)));
            cachedArrays_[fieldIdx] = nb::weakref(pyArr);
            return pyArr;
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
            for (FieldContainer& f : message_field) { sub_values.push_back(convert_field(f)); }
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

PYCOMMON_EXPORT nb::dict& PyField::to_shallow_dict() const
{
    if (cached_values_.size() == 0)
    {
        for (size_t i = 0; i < fieldCount; i++)
        {
            FieldContainer& field = fieldsPtr[i];
            if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue) &&
                (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY || field.fieldDef->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY))
            {
                std::vector<FieldContainer> field_array = std::get<std::vector<FieldContainer>>(field.fieldValue);
                cached_values_[nb::cast(field.fieldDef->name + "_length")] = field_array.size();
            }
            cached_values_[nb::cast(field.fieldDef->name)] = convert_field(field);
        }
    }
    return cached_values_;
}

nb::object PyField::unwrap_for_list(nb::object value)
{
    if (nb::isinstance<PyField>(value)) { return nb::cast<PyField>(value).to_list(); }
    if (nb::isinstance<std::vector<nb::object>>(value))
    {
        nb::list sublist;
        for (const auto& sub_item : nb::cast<std::vector<nb::object>>(value)) { sublist.append(unwrap_for_list(nb::borrow(sub_item))); }
        return sublist;
    }
    return value;
}

nb::object PyField::unwrap_for_dict(nb::object value)
{
    if (nb::isinstance<PyField>(value)) { return nb::cast<PyField>(value).to_dict(); }
    if (nb::isinstance<std::vector<nb::object>>(value))
    {
        nb::list sublist;
        for (const auto& sub_item : nb::cast<std::vector<nb::object>>(value)) { sublist.append(unwrap_for_dict(nb::borrow(sub_item))); }
        return sublist;
    }
    if (nb::isinstance<SatelliteId>(value)) { return value.attr("to_dict")(); }
    return value;
}

PYCOMMON_EXPORT nb::list PyField::get_field_names() const
{
    nb::list field_names;
    for_each_entry([&](const std::string& name, nb::object) { field_names.append(nb::cast(name)); });
    return field_names;
}

PYCOMMON_EXPORT nb::list PyField::get_values() const
{
    nb::list values;
    for_each_entry([&](const std::string&, nb::object value) { values.append(std::move(value)); });
    return values;
}

PYCOMMON_EXPORT nb::list PyField::to_list() const
{
    nb::list list;
    for_each_entry([&](const std::string&, nb::object value) { list.append(unwrap_for_list(std::move(value))); });
    return list;
}

PYCOMMON_EXPORT nb::dict PyField::to_dict() const
{
    nb::dict dict;
    for_each_entry([&](const std::string& name, nb::object value) { dict[nb::cast(name)] = unwrap_for_dict(std::move(value)); });
    return dict;
}

PYCOMMON_EXPORT nb::object PyField::getattr(nb::str field_name) const
{
    auto it = fieldNameMap_.find(field_name.c_str());
    if (it == fieldNameMap_.end()) { throw nb::attribute_error(field_name.c_str()); }
    return resolve_entry(it->second);
}

PYCOMMON_EXPORT nb::object PyFieldArray::getitem(size_t index) const
{
    if (index >= data->size()) { throw nb::index_error("index out of range"); }

    // Check if a cached object has been created
    if (cache[index].has_value())
    {
        // Return it if it is still alive
        nb::object existing = cache[index].value()();
        if (!existing.is_none()) { return existing; }
    }

    // Construct a new PyField for this element
    FieldContainer& subfield = (*data)[index];
    auto& subfields = std::get<std::vector<FieldContainer>>(subfield.fieldValue);

    nb::handle field_ptype = parentDb->GetFieldType(fieldDef);
    nb::object pyinst = nb::inst_alloc(field_ptype);
    PyField* cinst = nb::inst_ptr<PyField>(pyinst);
    new (cinst) PyField(subfields, fieldDef, parentDb, nb::cast(this, nb::rv_policy::none));
    nb::inst_mark_ready(pyinst);

    cache[index] = nb::weakref(pyinst);
    return pyinst;
}

PYCOMMON_EXPORT size_t PyFieldArray::len() const { return data->size(); }
