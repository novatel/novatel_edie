#include "py_common/field_objects.hpp"

#include <algorithm>
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

PYCOMMON_EXPORT const std::vector<BaseField::ConstPtr>& PyField::GetOrderedFields() const
{
    if (msgDef != nullptr) { return msgDef->GetMsgDefFromCrc(msgCrc).messageOrderedFields; }

    if (fieldDef != nullptr && fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
    {
        const auto* fieldArrayDef = dynamic_cast<const FieldArrayField*>(fieldDef);
        if (fieldArrayDef != nullptr) { return fieldArrayDef->fieldInfo.messageOrderedFields; }
    }

    static const std::vector<BaseField::ConstPtr> empty;
    return empty;
}

PYCOMMON_EXPORT const MessageBody* PyField::GetMessageBody() const
{
    if (parentData == nullptr) { return &fields; }

    const auto* elements = std::get_if<std::vector<MessageBody>>(parentData);
    if (elements == nullptr || parentIndex >= elements->size())
    {
        throw std::runtime_error("PyField::GetMessageBody(): parent message element index out of range");
    }
    return &(*elements)[parentIndex];
}

PYCOMMON_EXPORT bool PyField::IsFlatElement() const { return parentData != nullptr && std::holds_alternative<std::vector<std::byte>>(*parentData); }

PYCOMMON_EXPORT nb::object PyField::resolve_entry(const FieldLookupEntry& entry) const
{
    const auto& orderedFields = GetOrderedFields();
    if (entry.index >= orderedFields.size()) { throw std::runtime_error("PyField::resolve_entry(): field lookup index out of range"); }

    const auto& field = *orderedFields[entry.index];
    if (entry.is_length)
    {
        if (IsFlatElement())
        {
            const auto* arrayDef = dynamic_cast<const ArrayField*>(&field); // array elements in flat field arrays must be fixed-length arrays
            if (arrayDef == nullptr) { throw std::runtime_error("PyField::resolve_entry(): invalid fixed array metadata"); }
            return nb::cast(static_cast<size_t>(arrayDef->arrayLength));
        }

        return GetMessageBody()->GetFieldSize(field);
    }

    return convert_field(field);
}

PYCOMMON_EXPORT nb::object py_common::PyField::convert_field(const BaseField& field) const
{
    if (field.type == FIELD_TYPE::FIELD_ARRAY)
    {
        const auto& orderedFields = GetOrderedFields();
        const auto it =
            std::find_if(orderedFields.begin(), orderedFields.end(), [&field](const BaseField::ConstPtr& f) { return f.get() == &field; });
        if (it == orderedFields.end()) { throw std::runtime_error("PyField::convert_field(): field lookup failed"); }
        const size_t fieldIdx = static_cast<size_t>(std::distance(orderedFields.begin(), it));

        if (cachedArrays_[fieldIdx].has_value())
        {
            nb::object existing = cachedArrays_[fieldIdx].value()();
            if (!existing.is_none()) { return existing; }
        }

        const auto* arrayDef = dynamic_cast<const FieldArrayField*>(&field);
        if (arrayDef == nullptr) { throw std::runtime_error("PyField::convert_field(): missing field array metadata"); }

        const MessageBody* messageBody = GetMessageBody();
        const auto& varFields = messageBody->GetVarFields();
        if (field.index >= varFields.size()) { throw std::runtime_error("PyField::convert_field(): field array index out of range"); }

        nb::object pyArr = nb::cast(PyFieldArray(&varFields[field.index], &field, parentDb, nb::cast(this, nb::rv_policy::none)));
        cachedArrays_[fieldIdx] = nb::weakref(pyArr);
        return pyArr;
    }

    FieldValueVariant fieldValue;
    if (IsFlatElement())
    {
        const auto* flatArray = std::get_if<std::vector<std::byte>>(parentData);
        if (flatArray == nullptr || parentFieldArrayDef == nullptr)
        {
            throw std::runtime_error("PyField::convert_field(): missing field array definition for flat field array element access");
        }

        fieldValue = MessageBody::GetValueFromFlatFieldArray(field, *flatArray, parentIndex, parentFieldArrayDef->fieldInfo.fixedFieldBytes);
    }
    else { fieldValue = GetMessageBody()->GetFieldValue(field); }

    if (field.type == FIELD_TYPE::ENUM)
    {
        const auto* enumField = dynamic_cast<const EnumField*>(&field);
        if (enumField == nullptr) { throw std::runtime_error("PyField::convert_field(): enum metadata not found"); }

        const EnumDefinition* enumDef = parentDb->GetEnumDefId(enumField->enumId).get();
        nb::object enum_type = parentDb->GetEnumType(enumField->enumDef.get());
        if (enum_type.is_none())
        {
            throw std::runtime_error("Enum definition for " + field.name + " field with ID '" + enumField->enumId +
                                     "' not found in the JSON database");
        }

        return std::visit(
            [&](auto&& value) -> nb::object {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_integral_v<T>)
                {
                    const uint32_t key = static_cast<uint32_t>(value);
                    if (enumDef->valueName.count(key) > 0) { return enum_type(key); }
                }
                return enum_type(enumDef->unknownValue);
            },
            fieldValue);
    }

    if (field.conversion == "%s")
    {
        if (const auto* stringValue = std::get_if<std::string>(&fieldValue)) { return nb::cast(*stringValue); }
        if (const auto* bytes = std::get_if<std::vector<uint8_t>>(&fieldValue))
        {
            std::string str;
            str.reserve(bytes->size());
            for (const auto value : *bytes)
            {
                if (value == 0) { break; }
                str.push_back(static_cast<char>(value));
            }
            return nb::cast(str);
        }
        if (const auto* chars = std::get_if<std::vector<int8_t>>(&fieldValue))
        {
            std::string str;
            str.reserve(chars->size());
            for (const auto value : *chars)
            {
                if (value == 0) { break; }
                str.push_back(static_cast<char>(value));
            }
            return nb::cast(str);
        }
    }

    if (field.conversion == "%id")
    {
        const uint32_t temp_id = std::get<uint32_t>(fieldValue);
        SatelliteId sat_id;
        sat_id.usPrnOrSlot = temp_id & 0x0000FFFF;
        sat_id.sFrequencyChannel = (temp_id & 0xFFFF0000) >> 16;
        return nb::cast(sat_id);
    }

    return std::visit(
        [](auto&& value) -> nb::object {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::vector<std::byte>>)
            {
                throw std::runtime_error("PyField::convert_field(): raw byte arrays should be handled through the PyFieldArray interface");
            }
            else { return nb::cast(value); }
        },
        fieldValue);
}

PYCOMMON_EXPORT nb::dict& PyField::to_shallow_dict() const
{
    if (cached_values_.size() == 0)
    {
        for_each_entry([&](const std::string& name, nb::object value) { cached_values_[nb::cast(name)] = std::move(value); });
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
    if (fieldNameMap_ == nullptr) { throw nb::attribute_error(field_name.c_str()); }
    auto it = fieldNameMap_->find(field_name.c_str());
    if (it == fieldNameMap_->end()) { throw nb::attribute_error(field_name.c_str()); }
    return resolve_entry(it->second);
}

nb::object getIndexError()
{
    // Raise index error in python
    // Equivalent to `raise nb::index_error()` but much faster because it avoids throwing a C++ exception
    PyErr_SetString(PyExc_IndexError, "");
    return nb::object();
}

PYCOMMON_EXPORT nb::object PyFieldArray::getitem(ssize_t signedIndex) const
{
    if (data == nullptr || fieldDef == nullptr) { return getIndexError(); }

    const auto* elements = std::get_if<std::vector<MessageBody>>(data);
    const auto* flat = std::get_if<std::vector<std::byte>>(data);

    size_t count = 0;
    if (elements != nullptr) { count = elements->size(); }
    else if (flat != nullptr)
    {
        if (fieldDef->fieldInfo.fixedFieldBytes == 0) { return getIndexError(); }
        count = flat->size() / fieldDef->fieldInfo.fixedFieldBytes;
    }
    else { return getIndexError(); }

    if (cache.size() != count) { cache.resize(count); }

    if (signedIndex < 0)
    {
        signedIndex = static_cast<ssize_t>(count) + signedIndex;
        if (signedIndex < 0) { return getIndexError(); }
    }
    size_t index = static_cast<size_t>(signedIndex);
    if (index >= count) { return getIndexError(); }

    // Check if a cached object has been created
    if (cache[index].has_value())
    {
        // Return it if it is still alive
        nb::object existing = cache[index].value()();
        if (!existing.is_none()) { return existing; }
    }

    // Construct a new PyField for this element
    nb::handle field_ptype = parentDb->GetFieldType(fieldDef);
    nb::object pyinst = nb::inst_alloc(field_ptype);
    PyField* cinst = nb::inst_ptr<PyField>(pyinst);
    new (cinst) PyField(data, index, fieldDef, parentDb, nb::cast(this, nb::rv_policy::none));
    nb::inst_mark_ready(pyinst);

    cache[index] = nb::weakref(pyinst);
    return pyinst;
}

PYCOMMON_EXPORT size_t PyFieldArray::len() const
{
    if (data == nullptr || fieldDef == nullptr) { return 0; }

    return std::visit(
        [&](const auto& payload) -> size_t {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, std::vector<MessageBody>>) { return payload.size(); }
            else if constexpr (std::is_same_v<T, std::vector<std::byte>>)
            {
                return fieldDef->fieldInfo.fixedFieldBytes == 0 ? 0 : payload.size() / fieldDef->fieldInfo.fixedFieldBytes;
            }
            else { return 0; }
        },
        *data);
}
