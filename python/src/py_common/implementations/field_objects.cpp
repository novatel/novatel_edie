#include "py_common/field_objects.hpp"

#include <algorithm>
#include <type_traits>
#include <variant>

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

namespace {
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
} // namespace

PYCOMMON_EXPORT CompositeField* PyField::GetCompositeField() const
{
    if (std::holds_alternative<OwnedFields>(storage)) { return std::get<OwnedFields>(storage).get(); }

    if (!std::holds_alternative<nb::object>(storage)) { return nullptr; }
    nb::handle owner = std::get<nb::object>(storage);
    if (!nb::isinstance<PyFieldArray>(owner)) { return nullptr; }

    auto* parentField = nb::inst_ptr<PyFieldArray>(owner);
    if (std::holds_alternative<CompositeFieldArray>(*parentField->dataPtr))
    {
        auto& nested = std::get<CompositeFieldArray>(*parentField->dataPtr);
        if (myFieldIndex < nested.size()) { return &nested[myFieldIndex]; }
    }

    return nullptr;
}

PYCOMMON_EXPORT nb::object PyField::py_new(nb::handle cls, nb::kwargs kwargs)
{
    py_common::PyMessageDatabase::Ptr database = nb::cast<py_common::PyMessageDatabase::Ptr>(cls.attr("_owner_db"));
    if (!database) { throw py_common::FailureException("Constructor could not resolve owning MessageDatabase for this type."); }

    auto field_def = std::dynamic_pointer_cast<const FieldArrayField>(database->GetFieldTypeLookup(cls));
    if (!field_def) { throw py_common::FailureException("Constructor could not resolve FieldArrayField for this type."); }

    nb::object field_pyinst = nb::inst_alloc(cls);
    py_common::PyField* field_cinst = nb::inst_ptr<py_common::PyField>(field_pyinst);
    new (field_cinst) py_common::PyField(std::move(field_def), database);
    nb::inst_mark_ready(field_pyinst);

    // Apply the caller's field values here rather than in a separate __init__.
    // __init__ is repointed to object.__init__ (a fast C no-op), so all
    // construction work happens in this single __new__ dispatch.
    for (auto kv : kwargs) { field_cinst->setattr(nb::cast<nb::str>(kv.first), kv.second); }
    return field_pyinst;
}

PYCOMMON_EXPORT nb::object PyField::resolve_entry(const FieldLookupEntry& entry) const
{
    const auto& orderedFields = fieldInfo->messageOrderedFields;
    if (entry.index >= orderedFields.size()) { throw std::runtime_error("PyField::resolve_entry(): field lookup index out of range"); }

    const auto& field = *orderedFields[entry.index];
    if (entry.is_length)
    {
        if (auto* mb = GetCompositeField()) { return nb::cast(mb->GetFieldSize(field)); }

        const auto* arrayDef = dynamic_cast<const ArrayField*>(&field); // array elements in flat field arrays must be fixed-length arrays
        if (arrayDef == nullptr) { throw std::runtime_error("PyField::resolve_entry(): invalid fixed array metadata"); }
        return nb::cast(static_cast<size_t>(arrayDef->arrayLength));
    }

    return convert_field(field);
}

PyFieldArray* PyField::cached_array(size_t index) const
{
    if (!cachedArrays_[index].has_value()) { return nullptr; }
    nb::object obj = cachedArrays_[index].value()();
    return obj.is_none() ? nullptr : nb::inst_ptr<PyFieldArray>(obj);
}

PYCOMMON_EXPORT nb::object py_common::PyField::convert_field(const BaseField& field) const
{
    if (field.type == FIELD_TYPE::FIELD_ARRAY)
    {
        const auto& orderedFields = fieldInfo->messageOrderedFields;
        const auto it = std::find_if(orderedFields.begin(), orderedFields.end(), [&field](const BaseField::ConstPtr& f) {
            return f.get() == &field;
        });
        if (it == orderedFields.end()) { throw std::runtime_error("PyField::convert_field(): field lookup failed"); }
        const size_t fieldIdx = static_cast<size_t>(std::distance(orderedFields.begin(), it));

        // Return the cached PyFieldArray if one is still alive
        if (cachedArrays_[fieldIdx].has_value())
        {
            if (nb::object existing = cachedArrays_[fieldIdx].value()(); !existing.is_none()) { return existing; }
        }

        auto arrayDef = std::dynamic_pointer_cast<const FieldArrayField>(*it);
        if (arrayDef == nullptr) { throw std::runtime_error("PyField::convert_field(): missing field array metadata"); }

        const CompositeField* cf = GetCompositeField();
        if (cf == nullptr) { throw std::runtime_error("PyField::convert_field(): missing message body for FIELD_ARRAY access"); }
        const auto& varFields = cf->GetVarFields();
        if (field.index >= varFields.size()) { throw std::runtime_error("PyField::convert_field(): field index out of range"); }

        // Construct a new PyFieldArray in place
        nb::object pyArr = nb::inst_alloc(nb::type<PyFieldArray>());
        auto* varField = const_cast<FieldValueVariant*>(&varFields[field.index]);
        new (nb::inst_ptr<PyFieldArray>(pyArr)) PyFieldArray(*varField, arrayDef, parentDb, nb::cast(this, nb::rv_policy::none));
        nb::inst_mark_ready(pyArr);

        cachedArrays_[fieldIdx] = nb::weakref(pyArr);
        return pyArr;
    }

    FieldValueVariant fieldValue;
    if (const auto* cf = GetCompositeField()) { fieldValue = cf->GetFieldValueVariant(field); }
    else if (std::holds_alternative<nb::object>(storage) && nb::isinstance<PyFieldArray>(std::get<nb::object>(storage)))
    {
        auto parentFieldArray = nb::inst_ptr<PyFieldArray>(std::get<nb::object>(storage));
        // If field has no containing CompositeField, then it must be in a FlatFieldArray
        auto& fa = std::get<FlatFieldArray>(*parentFieldArray->dataPtr);
        SimpleTypeVisitor(field, [&](auto&& arg) {
            using ValueT = std::decay_t<decltype(arg)>;
            if (field.type == FIELD_TYPE::FIXED_LENGTH_ARRAY) { fieldValue = fa.GetFieldValue<TypedBuffer<ValueT>>(field, myFieldIndex); }
            else { fieldValue = fa.GetFieldValue<ValueT>(field, myFieldIndex); }
        });
    }
    else
    {
        throw std::runtime_error("PyField::convert_field(): unsupported storage type for field access");
    }

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

    if (field.conversionHash == CalculateBlockCrc32("id"))
    {
        const uint32_t temp_id = std::get<uint32_t>(fieldValue);
        SatelliteId sat_id;
        sat_id.usPrnOrSlot = temp_id & 0x0000FFFF;
        sat_id.sFrequencyChannel = (temp_id & 0xFFFF0000) >> 16;
        return nb::cast(sat_id);
    }

    return std::visit(
        [&](auto&& value) -> nb::object {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::vector<std::byte>> || std::is_same_v<T, std::vector<CompositeField>>)
            {
                throw std::runtime_error("PyField::convert_field(): field array types should be handled through PyFieldArray");
            }
            if constexpr (is_specialization_of_v<T, std::vector> || is_specialization_of_v<T, TypedBuffer>)
            {
                if constexpr (std::is_same_v<T, std::vector<uint8_t>> || std::is_same_v<T, std::vector<int8_t>> ||
                              std::is_same_v<T, std::vector<char>> || std::is_same_v<T, std::vector<unsigned char>> ||
                              std::is_same_v<T, TypedBuffer<uint8_t>> || std::is_same_v<T, TypedBuffer<int8_t>> ||
                              std::is_same_v<T, TypedBuffer<char>> || std::is_same_v<T, TypedBuffer<unsigned char>>)
                {
                    if (field.isString)
                    {
                        std::string str;
                        str.reserve(value.size());
                        for (size_t i = 0; i < value.size(); ++i)
                        {
                            if (value[i] == 0) { break; }
                            str.push_back(static_cast<char>(value[i]));
                        }
                        return nb::cast(str);
                    }
                }
                if (field.isString && value.empty()) { return nb::cast(std::string{}); }
                std::vector<nb::object> vec;
                vec.reserve(value.size());
                for (size_t i = 0; i < value.size(); ++i) { vec.push_back(nb::cast(value[i])); }
                return nb::cast(vec);
            }
            // STRING and SIMPLE types
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
    if (nb::isinstance<PyField>(value)) { return nb::cast<PyField&>(value).to_list(); }
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
    if (nb::isinstance<PyField>(value)) { return nb::cast<PyField&>(value).to_dict(); }
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

PYCOMMON_EXPORT std::optional<size_t> PyFieldArray::resolve_index(ssize_t signedIndex) const
{
    if (signedIndex < 0) { signedIndex += static_cast<ssize_t>(length); }
    if (signedIndex < 0 || static_cast<size_t>(signedIndex) >= length) { return std::nullopt; }
    return static_cast<size_t>(signedIndex);
}

PyField* PyFieldArray::cached_element(size_t index) const
{
    if (!cache[index].has_value()) { return nullptr; }
    nb::object obj = cache[index].value()();
    return obj.is_none() ? nullptr : nb::inst_ptr<PyField>(obj);
}

PYCOMMON_EXPORT nb::object PyFieldArray::getitem(ssize_t signedIndex) const
{
    auto resolved = resolve_index(signedIndex);
    if (!resolved)
    {
        // Often fails during iteration which is why we prefer not to throw
        return getIndexError();
    }
    size_t index = *resolved;

    // Return the cached element wrapper if one is still alive
    if (cache[index].has_value())
    {
        if (nb::object existing = cache[index].value()(); !existing.is_none()) { return existing; }
    }

    // Construct a new PyField for this element
    nb::handle field_ptype = parentDb->GetFieldType(fieldDef.get());
    nb::object pyinst = nb::inst_alloc(field_ptype);
    PyField* cinst = nb::inst_ptr<PyField>(pyinst);
    new (cinst) PyField(index, fieldDef, parentDb, nb::cast(this, nb::rv_policy::none));
    nb::inst_mark_ready(pyinst);

    cache[index] = nb::weakref(pyinst);
    return pyinst;
}

PYCOMMON_EXPORT void PyFieldArray::setitem(ssize_t signedIndex, nb::object value)
{
    std::optional<size_t> resolvedIndex = resolve_index(signedIndex);
    if (!resolvedIndex) { throw nb::index_error(); }
    size_t index = resolvedIndex.value();
    if (!nb::isinstance<PyField>(value)) { throw nb::type_error("Only a Field can be assigned to a FieldArray!"); }
    PyField* fieldVal = nb::inst_ptr<PyField>(value);
    // fieldDef will be set here because array has been verified to be at least one element in length
    if (fieldVal->fieldDef != fieldDef) { throw nb::type_error("Field does not match the type of the FieldArray!"); }

    CompositeField* source = fieldVal->GetCompositeField();
    if (source == nullptr) { throw nb::type_error("Field does not have accessible CompositeField storage."); }

    std::visit(
        [&](auto&& arrayData) {
            using T = std::decay_t<decltype(arrayData)>;
            if constexpr (std::is_same_v<T, CompositeFieldArray> || std::is_same_v<T, FlatFieldArray>)
            {
                if (index >= arrayData.size()) { throw nb::index_error(); }

                if constexpr (std::is_same_v<T, CompositeFieldArray>)
                {
                    // Let the old element wrapper (if still alive) take ownership of its prior data.
                    if (PyField* existingFieldVal = cached_element(index)) { existingFieldVal->take_ownership(std::move(arrayData[index])); }
                    arrayData[index] = *source;
                }
                else
                {
                    if (source->GetVarFields().size() > 0) { throw nb::type_error("FlatFieldArray elements cannot contain variable-length fields."); }
                    // Copy existing data out of flat byte region into a new CompositeField for the old element wrapper (if still alive) to take ownership of.
                    if (PyField* existingFieldVal = cached_element(index)) {
                        auto existingMb = CompositeField(fieldDef->fieldInfo->fixedFieldBytes, 0);
                        existingMb.SetFieldValue<true>(0, arrayData.data() + (index * fieldDef->fieldInfo->fixedFieldBytes), fieldDef->fieldInfo->fixedFieldBytes);
                        existingFieldVal->take_ownership(std::move(existingMb));
                    }
                    // Copy fixed CompositeField data into the existing FlatFieldArray field
                    arrayData.SetFieldValue(index, 0, source->GetFixedFields().data(), source->GetFixedFields().size());
                }
                cache[index].reset();
            }
            else { throw nb::type_error("FieldArray contains unsupported storage type."); }
        },
        *dataPtr);
}

PYCOMMON_EXPORT size_t PyFieldArray::len() const { return length; }

PYCOMMON_EXPORT PyFieldArray::PyFieldArray(nb::list values)
{
    if (values.empty())
    {
        // No elements to infer fieldDef/parentDb from, but we must still own an (empty)
        // backing vector so `data` is valid — it is dereferenced unconditionally when
        // this array is assigned to a field. fieldDef is filled in later by setattr.
        take_ownership(CompositeFieldArray{});
        return;
    }

    for (auto it = values.begin(); it != values.end(); it++)
    {
        if (!nb::isinstance<PyField>(nb::handle(*it))) { throw nb::type_error("Only Fields can appear within a FieldArray!"); }
    }
    PyField* candidate = nb::inst_ptr<PyField>(nb::handle(*values.begin()));
    auto fieldArrayDef = std::dynamic_pointer_cast<const FieldArrayField>(candidate->fieldDef);
    if (fieldArrayDef == nullptr) { throw nb::type_error("Only regular fields can appear within a FieldArray!"); }
    if (values.size() > fieldArrayDef->arrayLength) { throw nb::value_error("Value exceeds maximum array size."); }
    fieldDef = std::move(fieldArrayDef);
    parentDb = candidate->parentDb;
    cache.resize(values.size());
    // Always use CompositeFieldArray for simplicity. Probably not worth the extra complexity to use FlatFieldArray here.
    CompositeFieldArray owned;
    owned.reserve(values.size());
    for (auto it = values.begin(); it != values.end(); it++)
    {
        PyField* itVal = nb::inst_ptr<PyField>(nb::handle(*it));

        // Copy value (minor part of constructor overhead - not worth optimizing)
        owned.emplace_back(CompositeField(*itVal->fieldsPtr));
    }
    take_ownership(std::move(owned));
}

template <bool Fixed, typename T>
void PyField::set_field_value(size_t ind_, T* val_, size_t n_)
{
    if (auto* mb = GetCompositeField())
    {
        mb->SetFieldValue<Fixed>(ind_, val_, n_);
    }
    else if (std::holds_alternative<nb::object>(storage) && nb::isinstance<PyFieldArray>(std::get<nb::object>(storage)))
    {
        if constexpr (!Fixed) { throw std::runtime_error("set_field_value(): Variable-length arrays are not valid in this context."); }
        else
        {
            auto parentFieldArray = nb::inst_ptr<PyFieldArray>(std::get<nb::object>(storage));
            std::get<FlatFieldArray>(*parentFieldArray->dataPtr).SetFieldValue(myFieldIndex, ind_, val_, n_);
        }
    }
    else { throw std::runtime_error("set_field_value(): unsupported storage type for field access"); }
}

template <bool Fixed>
void PyField::set_regular_array(const ArrayField::ConstPtr& arrFieldDef_, nb::handle value_)
{
    if (arrFieldDef_ == nullptr) { throw nb::type_error("Array field metadata is missing."); }

    size_t i = 0;
    if (nb::isinstance<nb::str>(value_) || nb::isinstance<nb::bytes>(value_))
    {
        if (arrFieldDef_->isCsv) { throw nb::type_error("String value is not valid for a non-string array."); }
        std::string_view strVal;
        if (nb::isinstance<nb::str>(value_)) { strVal = nb::cast<nb::str>(value_).c_str(); }
        else { strVal = nb::cast<nb::bytes>(value_).c_str(); }
        if (strVal.size() > arrFieldDef_->arrayLength) { throw nb::value_error("Value exceeds maximum array size."); }
        set_field_value<Fixed>(arrFieldDef_->index, reinterpret_cast<const unsigned char*>(strVal.data()), strVal.size());
        i = strVal.size();
    }
    else if (nb::isinstance<nb::list>(value_))
    {
        auto listVal = nb::cast<nb::list>(value_);
        if (listVal.size() > arrFieldDef_->arrayLength) { throw nb::value_error("Value exceeds maximum array size."); }
        if (listVal.size() > 0)
        {
            SimpleTypeVisitor(*arrFieldDef_, [&](auto&& arg) {
                using ValueT = std::conditional_t<std::is_same_v<std::decay_t<decltype(arg)>, bool>, uint8_t, std::decay_t<decltype(arg)>>;
                std::vector<ValueT> temp(listVal.size());
                for (; i < listVal.size(); i++) { temp[i] = attr_cast<ValueT>(listVal[i]); }
                set_field_value<Fixed>(arrFieldDef_->index, temp.data(), temp.size());
            });
        }
    }
    else { throw nb::attribute_error("Value cannot be converted to a fixed length array."); }
    if constexpr (Fixed)
    {
        if (arrFieldDef_->arrayLength > i)
        {
            SimpleTypeVisitor(*arrFieldDef_, [&](auto&& arg) {
                using ValueT = std::conditional_t<std::is_same_v<std::decay_t<decltype(arg)>, bool>, uint8_t, std::decay_t<decltype(arg)>>;
                std::vector<ValueT> temp(arrFieldDef_->arrayLength - i);
                set_field_value<true>(arrFieldDef_->index + (i * sizeof(ValueT)), temp.data(), temp.size());
            });
        }
    }
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

        const auto& entryField = fieldInfo->messageOrderedFields[entry.index];

        switch (entryField->type)
        {
        case FIELD_TYPE::ENUM: [[fallthrough]];
        case FIELD_TYPE::SIMPLE:
            SimpleTypeVisitor(*entryField, [&](auto&& arg) {
                auto v = attr_cast<std::decay_t<decltype(arg)>>(value);
                set_field_value<true>(entryField->index, &v);
            });
            break;
        case FIELD_TYPE::STRING:
            if (auto* mb = GetCompositeField())
            {
                mb->SetFieldValue(entryField->index, attr_cast<std::string>(value));
            }
            else { throw nb::attribute_error("STRING types not allowed in fixed-length fields."); }
            break;
        case FIELD_TYPE::FIXED_LENGTH_ARRAY:
            set_regular_array<true>(std::dynamic_pointer_cast<const ArrayField>(entryField), value);
            break;
        case FIELD_TYPE::VARIABLE_LENGTH_ARRAY:
            set_regular_array<false>(std::dynamic_pointer_cast<const ArrayField>(entryField), value);
            break;
        case FIELD_TYPE::FIELD_ARRAY: {
            nb::object owned_array_obj;
            nb::handle array_handle = value;
            if (!nb::isinstance<PyFieldArray>(value))
            {
                if (!nb::isinstance<nb::list>(value)) { throw nb::type_error("Must be initialized with a FieldArray or list of Fields!"); }
                // Create new PyFieldArray from list
                nb::handle py_array_type = nb::type<PyFieldArray>();
                owned_array_obj = nb::inst_alloc(py_array_type);
                new (nb::inst_ptr<PyFieldArray>(owned_array_obj)) PyFieldArray(nb::cast<nb::list>(value));
                nb::inst_mark_ready(owned_array_obj);
                array_handle = owned_array_obj;
            }
            PyFieldArray* fieldArrayVal = nb::inst_ptr<PyFieldArray>(array_handle);
            // Note: accessing "value" or "curVal" from another thread while this is occuring is a race conditon
            auto fieldArrayDef = std::dynamic_pointer_cast<const FieldArrayField>(entryField);
            if (fieldArrayDef == nullptr) { throw nb::type_error("FIELD_ARRAY assignment requires field-array metadata."); }

            if (fieldArrayVal->fieldDef == nullptr) { fieldArrayVal->fieldDef = fieldArrayDef; }
            if (fieldArrayVal->fieldDef != fieldArrayDef) { throw nb::type_error("FieldArray contains elements of the wrong type!"); }
            auto* cf = GetCompositeField();
            // Note: cf should never be nullptr here as FIELD_ARRAY cannot appear in FlatFieldArray
            if (cf == nullptr) { throw nb::type_error("FIELD_ARRAY assignment requires a CompositeField to be present."); }
            // Transfer data ownership to the existing array wrapper, if still alive
            if (PyFieldArray* curArray = cached_array(entry.index))
            {
                curArray->take_ownership(cf->GetFieldValueVariant(*entryField));
            }

            // Copy value (minor part of constructor overhead - not worth optimizing).
            if (std::holds_alternative<FlatFieldArray>(*fieldArrayVal->dataPtr))
            {
                cf->SetFieldValue(*entryField, std::get<FlatFieldArray>(*fieldArrayVal->dataPtr));
            }
            else { cf->SetFieldValue(*entryField, std::get<CompositeFieldArray>(*fieldArrayVal->dataPtr)); }
            cachedArrays_[entry.index].reset();
            break;
        }
        default:
            throw nb::attribute_error(
                ("Modification of attributes with the \"" + std::string(FieldTypeToString(entryField->type)) + "\" type is not yet supported.")
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
