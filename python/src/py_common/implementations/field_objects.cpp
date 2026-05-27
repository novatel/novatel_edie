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

namespace {

// Returns a zero-initialised FieldValueVariant alternative matching the given DATA_TYPE.
// Used by BuildDefaultFields to populate scalar fields with sensible defaults.
// ENUM fields are handled at the call site (int32_t to satisfy the debug-mode
// FieldContainer::Validate() invariant).
FieldValueVariant MakeDefaultScalarValue(DATA_TYPE dt)
{
    switch (dt)
    {
    case DATA_TYPE::BOOL: return bool{};
    case DATA_TYPE::CHAR: return int8_t{};
    case DATA_TYPE::UCHAR: return uint8_t{};
    case DATA_TYPE::SHORT: return int16_t{};
    case DATA_TYPE::USHORT: return uint16_t{};
    case DATA_TYPE::INT: [[fallthrough]];
    case DATA_TYPE::LONG: return int32_t{};
    case DATA_TYPE::UINT: [[fallthrough]];
    case DATA_TYPE::ULONG: return uint32_t{};
    case DATA_TYPE::LONGLONG: return int64_t{};
    case DATA_TYPE::ULONGLONG: return uint64_t{};
    case DATA_TYPE::FLOAT: return float{};
    case DATA_TYPE::DOUBLE: return double{};
    case DATA_TYPE::HEXBYTE: return uint8_t{};
    case DATA_TYPE::SATELLITEID: return uint32_t{};
    default: return uint32_t{};
    }
}

template <typename T> T attr_cast(nb::handle value)
{
    try
    {
        return nb::cast<T>(value);
    }
    catch (nb::cast_error&)
    {
        throw nb::type_error("Invalid type conversion.");
    }
}

// Builds a SIMPLE-typed FieldContainer from a Python value, casting to the
// variant alternative that matches fieldDef's DATA_TYPE. Each return site is a
// prvalue, so mandatory copy elision applies. Unsupported DATA_TYPEs raise.
FieldContainer get_simple_attribute(BaseField::ConstPtr fieldDef, nb::handle value)
{
    switch (fieldDef->dataType.name)
    {
    case DATA_TYPE::BOOL: return FieldContainer{attr_cast<bool>(value), std::move(fieldDef)};
    case DATA_TYPE::CHAR: return FieldContainer{attr_cast<int8_t>(value), std::move(fieldDef)};
    case DATA_TYPE::UCHAR: return FieldContainer{attr_cast<uint8_t>(value), std::move(fieldDef)};
    case DATA_TYPE::SHORT: return FieldContainer{attr_cast<int16_t>(value), std::move(fieldDef)};
    case DATA_TYPE::USHORT: return FieldContainer{attr_cast<uint16_t>(value), std::move(fieldDef)};
    case DATA_TYPE::LONG: [[fallthrough]];
    case DATA_TYPE::INT: return FieldContainer{attr_cast<int32_t>(value), std::move(fieldDef)};
    case DATA_TYPE::ULONG: [[fallthrough]];
    case DATA_TYPE::UINT: return FieldContainer{attr_cast<uint32_t>(value), std::move(fieldDef)};
    case DATA_TYPE::LONGLONG: return FieldContainer{attr_cast<int64_t>(value), std::move(fieldDef)};
    case DATA_TYPE::ULONGLONG: return FieldContainer{attr_cast<uint64_t>(value), std::move(fieldDef)};
    case DATA_TYPE::FLOAT: return FieldContainer{attr_cast<float>(value), std::move(fieldDef)};
    case DATA_TYPE::DOUBLE: return FieldContainer{attr_cast<double>(value), std::move(fieldDef)};
    default:
        throw nb::attribute_error(
            ("Modification of attributes with the \"" + std::string(DataTypeToString(fieldDef->dataType.name)) + "\" type is not yet supported.")
                .c_str());
    }
}

} // namespace

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
            nb::object pyArr = nb::cast(PyFieldArray(message_field, field.fieldDef, parentDb, nb::cast(this, nb::rv_policy::none)));
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
    if (signedIndex < 0)
    {
        signedIndex = data->size() + signedIndex;
        if (signedIndex < 0) { return getIndexError(); }
    }
    size_t index = signedIndex;
    if (index >= data->size()) { return getIndexError(); }

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

    nb::handle field_ptype = parentDb->GetFieldType(fieldDef.get());
    nb::object pyinst = nb::inst_alloc(field_ptype);
    PyField* cinst = nb::inst_ptr<PyField>(pyinst);
    new (cinst) PyField(subfields, fieldDef, parentDb, nb::cast(this, nb::rv_policy::none));
    nb::inst_mark_ready(pyinst);

    cache[index] = nb::weakref(pyinst);
    return pyinst;
}

PYCOMMON_EXPORT size_t PyFieldArray::len() const { return data->size(); }

PYCOMMON_EXPORT PyFieldArray::PyFieldArray(nb::list values)
{
    if (values.empty()) { throw nb::value_error("Unable to determine field array type, provide at least one value"); }

    for (auto it = values.begin(); it != values.end(); it++)
    {
        if (!nb::isinstance<PyField>(nb::handle(*it))) { throw nb::value_error("Only Fields can appear within a FieldArray!"); }
    }
    PyField* candidate = nb::inst_ptr<PyField>(nb::handle(*values.begin()));
    fieldDef = candidate->fieldDef;
    parentDb = candidate->parentDb;
    cache.resize(values.size());
    ownedData.reserve(values.size());
    data = &ownedData;
    size_t index = 0;
    for (auto it = values.begin(); it != values.end(); it++)
    {
        nb::handle itHandle = nb::handle(*it);
        PyField* itVal = nb::inst_ptr<PyField>(itHandle);
        if (itVal->parent.is_valid())
        {
            // Use copy semantics for elements that have another owner
            ownedData.emplace_back(FieldContainer(std::vector(itVal->fieldsPtr, itVal->fieldsPtr + itVal->fieldCount), fieldDef));
        }
        else
        {
            // Use move semantics for elements without another owner
            ownedData.emplace_back(FieldContainer(std::move(itVal->fields), fieldDef));
            itVal->parent = nb::find(this);
            cache[index] = nb::weakref(itHandle);
        }
        index++;
    }
}

std::vector<FieldContainer> PyField::get_regular_array(const std::shared_ptr<const ArrayField>& fixedArrDef, nb::handle value)
{
    std::vector<FieldContainer> fixedArrVal;
    if (nb::isinstance<nb::str>(value) || nb::isinstance<nb::bytes>(value))
    {
        if (fixedArrDef->isCsv)
        {
            throw nb::type_error("String value is not valid for a non-string array.");
        }
        std::string_view strVal;
        if (nb::isinstance<nb::str>(value)) { strVal = nb::cast<nb::str>(value).c_str(); }
        else { strVal = nb::cast<nb::bytes>(value).c_str(); }
        if (strVal.size() > fixedArrDef->arrayLength) { throw nb::value_error("Value exceeds maximum array size."); }
        fixedArrVal.reserve(strVal.size());
        for (unsigned char b : strVal) { fixedArrVal.emplace_back(FieldContainer{b, fixedArrDef}); }
    }
    else if (nb::isinstance<nb::list>(value))
    {
        auto listVal = nb::cast<nb::list>(value);
        if (listVal.size() > fixedArrDef->arrayLength) { throw nb::value_error("Value exceeds maximum array size."); }
        fixedArrVal.reserve(listVal.size());
        for (auto listIt = listVal.begin(); listIt != listVal.end(); listIt++)
        {
            fixedArrVal.emplace_back(get_simple_attribute(fixedArrDef, nb::handle(*listIt)));
        }
    }
    else { throw nb::attribute_error("Value cannot be converted to a fixed length array."); }
    return fixedArrVal;
}

PYCOMMON_EXPORT void PyField::setattr(nb::str field_name, nb::handle value)
{
    try
    {

        if (fieldNameMap_ == nullptr) { throw nb::attribute_error(field_name.c_str()); }

        auto it = fieldNameMap_->find(field_name.c_str());
        if (it == fieldNameMap_->end()) { throw nb::attribute_error(field_name.c_str()); }
        const FieldLookupEntry& entry = it->second;

        if (entry.is_length) { throw nb::attribute_error("Length cannot be set directly, modify the corresponding array instead."); }

        FieldContainer& field = fieldsPtr[entry.index];

        switch (field.fieldDef->type)
        {
        case FIELD_TYPE::SIMPLE: field = get_simple_attribute(field.fieldDef, value); break;
        case FIELD_TYPE::ENUM: field.fieldValue = attr_cast<int32_t>(value); break;
        case FIELD_TYPE::STRING: field.fieldValue = attr_cast<std::string>(value); break;
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            auto arrayDef = std::dynamic_pointer_cast<const ArrayField>(field.fieldDef);
            auto values = get_regular_array(arrayDef, value);
            while (values.size() < arrayDef->arrayLength)
            {
                values.emplace_back(FieldContainer{MakeDefaultScalarValue(arrayDef->dataType.name), arrayDef});
            }
            field.fieldValue = std::move(values);
            break;
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
            field.fieldValue = get_regular_array(std::dynamic_pointer_cast<const ArrayField>(field.fieldDef), value);
            break;
        case FIELD_TYPE::FIELD_ARRAY: {
            if (!nb::isinstance<PyFieldArray>(value)) { throw nb::type_error("Must be initialized with a FieldArray!"); }
            PyFieldArray* fieldArrayVal = nb::inst_ptr<PyFieldArray>(value);
            // Note: accessing "value" or "curVal" from another thread while this is occuring is a race conditon
            if (field.fieldDef != fieldArrayVal->fieldDef) { throw nb::type_error("FieldArray contains elements of the wrong type!"); }
            // Transfer data ownership of existing element
            if (cachedArrays_[entry.index].has_value())
            {
                nb::object curVal = cachedArrays_[entry.index].value()();
                if (curVal.is_valid())
                {
                    PyFieldArray* curArray = nb::inst_ptr<PyFieldArray>(curVal);
                    curArray->ownedData = std::move(std::get<std::vector<FieldContainer>>(field.fieldValue));
                    curArray->data = &curArray->ownedData;
                    curArray->parent = nb::object();
                }
            }
            if (fieldArrayVal->parent.is_valid())
            {
                // Use copy semantics for an array that is owned by another object
                field.fieldValue = *fieldArrayVal->data;
            }
            else
            {
                // Use move semantics for an array that owns itself
                field.fieldValue = std::move(fieldArrayVal->ownedData);
                fieldArrayVal->data = &std::get<std::vector<FieldContainer>>(field.fieldValue);
                fieldArrayVal->parent = nb::find(this);
                cachedArrays_[entry.index] = nb::weakref(value);
            }
            break;
        }
        default:
            throw nb::attribute_error(
                ("Modification of attributes with the \"" + std::string(FieldTypeToString(field.fieldDef->type)) + "\" type is not yet supported.")
                    .c_str());
        }
    }
    catch (nb::builtin_exception& e)
    {
        std::string msg = "Failed to set attribute \"" + std::string(field_name.c_str()) + "\": " + e.what();
        throw nb::builtin_exception(e.type(), msg.c_str());
    }
    // Invalidate the to_shallow_dict cache so the new value is visible to future reads.
    cached_values_.clear();
}

PYCOMMON_EXPORT std::vector<FieldContainer> PyField::BuildDefaultFields(const std::vector<BaseField::Ptr>& fieldDefs)
{
    std::vector<FieldContainer> fields;
    fields.reserve(fieldDefs.size());

    for (const auto& fieldDef : fieldDefs)
    {
        switch (fieldDef->type)
        {
        case FIELD_TYPE::FIELD_ARRAY: {
            fields.emplace_back(std::vector<FieldContainer>{}, fieldDef);
            break;
        }
        case FIELD_TYPE::FIXED_LENGTH_ARRAY: {
            const auto* arrayDef = dynamic_cast<const ArrayField*>(fieldDef.get());
            const size_t arrayLength = arrayDef ? arrayDef->arrayLength : 0;
            std::vector<FieldContainer> arrayValues;
            arrayValues.reserve(arrayLength);
            for (size_t i = 0; i < arrayLength; ++i) { arrayValues.emplace_back(MakeDefaultScalarValue(fieldDef->dataType.name), fieldDef); }
            fields.emplace_back(std::move(arrayValues), fieldDef);
            break;
        }
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY: {
            fields.emplace_back(std::vector<FieldContainer>{}, fieldDef);
            break;
        }
        case FIELD_TYPE::STRING: {
            fields.emplace_back(std::string{}, fieldDef);
            break;
        }
        case FIELD_TYPE::ENUM: {
            // int32_t to satisfy FieldContainer::Validate() in debug builds.
            fields.emplace_back(int32_t{0}, fieldDef);
            break;
        }
        default: {
            fields.emplace_back(MakeDefaultScalarValue(fieldDef->dataType.name), fieldDef);
            break;
        }
        }
    }

    return fields;
}

PYCOMMON_EXPORT std::vector<FieldContainer> PyField::BuildDefaultFields(const ::novatel::edie::BaseField* fieldDef)
{
    if (const auto* arrField = dynamic_cast<const FieldArrayField*>(fieldDef)) { return BuildDefaultFields(arrField->fields); }
    return {};
}

PYCOMMON_EXPORT std::vector<FieldContainer> PyField::BuildDefaultFields(const ::novatel::edie::MessageDefinition* msgDef, uint32_t crc)
{
    if (msgDef == nullptr) { return {}; }
    auto it = msgDef->fields.find(crc);
    if (it == msgDef->fields.end()) { return {}; }
    return BuildDefaultFields(it->second);
}
