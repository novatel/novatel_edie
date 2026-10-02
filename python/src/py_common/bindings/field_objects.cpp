#include "py_common/field_objects.hpp"

#include <sstream>
#include <variant>

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

#include "py_common/init_bindings.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

namespace {

// Bind each named binary operator to the same operator on the BitField's integer value.
template <typename... Names> void DefIntBinaryOps(nb::class_<py_common::PyBitField>& cls_, Names... names_)
{
    (cls_.def(names_,
              [name = names_](const py_common::PyBitField& self, nb::object other) {
                  // Coerce a BitField operand to its int value so BitField-vs-BitField ops don't fall back to identity.
                  if (nb::isinstance<py_common::PyBitField>(other)) { other = nb::int_(nb::cast<const py_common::PyBitField&>(other).val); }
                  return nb::int_(self.val).attr(name)(other);
              }),
     ...);
}

// Bind each named unary operator to the same operator on the BitField's integer value.
template <typename... Names> void DefIntUnaryOps(nb::class_<py_common::PyBitField>& cls_, Names... names_)
{
    (cls_.def(names_, [name = names_](const py_common::PyBitField& self) { return nb::int_(self.val).attr(name)(); }), ...);
}

} // namespace

void py_common::init_field_objects(nb::module_& m)
{
    auto field_class = nb::class_<py_common::PyField>(m, "Field", nb::is_weak_referenceable());
    field_class.def_static("__new__", &py_common::PyField::py_new, "cls"_a, "kwargs"_a)
        .def("__getattr__", &py_common::PyField::getattr, "field_name"_a)
        .def("__setattr__", &py_common::PyField::setattr, "field_name"_a, "value"_a)
        .def("__repr__",
             [](nb::handle self) {
                 py_common::PyField* body = nb::inst_ptr<py_common::PyField>(self);
                 std::stringstream repr;
                 repr << nb::cast<nb::str>(self.attr("__class__").attr("__name__")).c_str() << "(";
                 bool first = true;
                 for (const auto& [field_name, value] : body->to_shallow_dict())
                 {
                     if (!first) { repr << ", "; }
                     first = false;
                     repr << nb::str("{}={!r}").format(field_name, value).c_str();
                 }
                 repr << ")";
                 return repr.str();
             })
        .def("__dir__",
             [](nb::object self) {
                 // get required Python builtin functions
                 nb::module_ builtins = nb::module_::import_("builtins");
                 nb::handle super = builtins.attr("super");
                 nb::handle type = builtins.attr("type");

                 // start from the 'Field' class instead of a specific subclass
                 nb::handle current_type = type(self);
                 std::string current_type_name = nb::cast<std::string>(current_type.attr("__name__"));
                 while (current_type_name != "Field")
                 {
                     current_type = (current_type.attr("__bases__"))[0];
                     current_type_name = nb::cast<std::string>(current_type.attr("__name__"));
                 }

                 // retrieve base list based on 'Field' superclass method
                 nb::object super_obj = super(current_type, self);
                 nb::list base_list = nb::cast<nb::list>(super_obj.attr("__dir__")());
                 // add dynamic fields to the list
                 py_common::PyField* body = nb::inst_ptr<py_common::PyField>(self);
                 for (const auto& field_name : body->get_field_names()) { base_list.append(field_name); }

                 return base_list;
             })
        .def("get_field_names", &py_common::PyField::get_field_names,
             R"doc(
            Retrieves the name of every top-level field within the payload of this message.

            Returns:
                The name of every top-level field within the message payload.
            )doc")
        .def("get_field_values", &py_common::PyField::get_values,
             R"doc(
            Retrieves the values of every top-level field within the payload of this message.

            Returns:
                The value of every top-level field within the message payload.
            )doc")
        .def("to_dict", &py_common::PyField::to_dict,
             R"doc(
            Converts the field to a dictionary.

            Returns:
                A dictionary representation of the field.
            )doc")
        .def("to_list", &py_common::PyField::to_list,
             R"doc(
            Converts the field to a list.

            Returns:
                A list representation of the field.
            )doc");

    // No Python-level __init__ is defined: py_new does all construction work.
    // Repoint __init__ to object.__init__ so nanobind's default tp_init (which
    // raises "no constructor defined") is replaced by a fast C no-op. Because
    // __new__ is overridden, object.__init__ silently ignores the kwargs it is
    // also handed. Generated field subclasses inherit this slot.
    field_class.attr("__init__") = nb::handle(reinterpret_cast<PyObject*>(&PyBaseObject_Type)).attr("__init__");

    nb::class_<py_common::PyFieldArray>(m, "FieldArray", nb::is_weak_referenceable())
        .def(nb::init<nb::list>(), "values"_a)
        .def("__getitem__", &py_common::PyFieldArray::getitem, "index"_a)
        .def("__setitem__", &py_common::PyFieldArray::setitem, "index"_a, "value"_a)
        .def("__len__", &py_common::PyFieldArray::len);

    auto bitfield_class =
        nb::class_<py_common::PyBitField>(m, "BitField", "An integer bitmask field exposing named sub-masks; behaves like its integer value.");

    bitfield_class.def_static(
        "__new__",
        [](nb::handle cls, nb::object value, nb::kwargs kwargs) {
            if (cls.is(nb::type<py_common::PyBitField>()))
            {
                throw nb::type_error("BitField cannot be instantiated directly; it is the base class for the "
                                     "database's generated bitmask types.");
            }
            py_common::PyMessageDatabase::ConstPtr database =
                nb::hasattr(cls, "_owner_db") ? nb::cast<py_common::PyMessageDatabase::Ptr>(cls.attr("_owner_db")) : nullptr;
            if (!database) { throw nb::type_error("Attempting to instantiate an invalid type. Associated database could not be identified."); }

            // Resolve via the MRO so user subclasses of a generated type inherit its bitmask.
            BitMaskMap::ConstPtr interpretation = nullptr;
            for (nb::handle base : cls.attr("__mro__"))
            {
                interpretation = database->GetBitFieldTypeLookup(base);
                if (interpretation) { break; }
            }
            if (!interpretation) { throw nb::type_error("Attempting to instantiate an invalid type. Associated bitmask could not be identified."); }

            uint32_t packed_value = value.is_none() ? 0 : nb::cast<uint32_t>(value);
            for (auto kv : kwargs)
            {
                const std::string mask_name = nb::cast<nb::str>(kv.first).c_str();
                const auto mask_it = interpretation->masks.find(mask_name);
                if (mask_it == interpretation->masks.end()) { throw nb::type_error(("Unknown bitfield sub-mask: " + mask_name).c_str()); }

                const BitMask& bitfield = mask_it->second.bitfield;
                const uint64_t sub_value = nb::cast<uint32_t>(kv.second);
                const uint64_t limit = uint64_t{1} << bitfield.width;
                if (sub_value >= limit)
                {
                    throw nb::value_error(
                        ("Value for bitfield sub-mask '" + mask_name + "' does not fit in " + std::to_string(bitfield.width) + " bits").c_str());
                }

                const uint32_t mask = bitfield.mask();
                packed_value = (packed_value & ~mask) | static_cast<uint32_t>(sub_value << bitfield.offset);
            }

            nb::object inst = nb::inst_alloc(cls);
            new (nb::inst_ptr<py_common::PyBitField>(inst)) py_common::PyBitField{packed_value, std::move(interpretation), std::move(database)};
            nb::inst_mark_ready(inst);
            return inst;
        },
        "cls"_a, "value"_a = nb::none(), "kwargs"_a);

    bitfield_class.def_ro("value", &py_common::PyBitField::val, "The raw integer value of the bitmask field.")
        .def("__getattr__", &py_common::PyBitField::getattr, "field_name"_a)
        .def("__dir__",
             [](nb::object self) {
                 // get required Python builtin functions
                 nb::module_ builtins = nb::module_::import_("builtins");
                 nb::handle super = builtins.attr("super");
                 nb::handle type = builtins.attr("type");

                 // start from the 'BitField' class instead of a concrete subclass
                 nb::handle current_type = type(self);
                 std::string current_type_name = nb::cast<std::string>(current_type.attr("__name__"));
                 while (current_type_name != "BitField")
                 {
                     current_type = (current_type.attr("__bases__"))[0];
                     current_type_name = nb::cast<std::string>(current_type.attr("__name__"));
                 }

                 // retrieve base list based on 'BitField' superclass method
                 nb::object super_obj = super(current_type, self);
                 nb::list base_list = nb::cast<nb::list>(super_obj.attr("__dir__")());
                 // add dynamic sub-mask names to the list
                 const py_common::PyBitField* body = nb::inst_ptr<py_common::PyBitField>(self);
                 for (const auto& [mask_name, mask_entry] : body->GetInterpretation().masks) { base_list.append(nb::cast(mask_name)); }

                 return base_list;
             })
        // Integer conversions so a BitField can be used wherever an int is expected.
        .def("__int__", [](const py_common::PyBitField& self) { return nb::int_(self.val); })
        .def("__index__", [](const py_common::PyBitField& self) { return nb::int_(self.val); })
        .def("__float__", [](const py_common::PyBitField& self) { return nb::float_(static_cast<double>(self.val)); })
        .def("__bool__", [](const py_common::PyBitField& self) { return self.val != 0; })
        .def("__hash__", [](const py_common::PyBitField& self) { return nb::int_(self.val).attr("__hash__")(); })
        .def("__repr__", [](const py_common::PyBitField& self) {
            return nb::str("BitField(value={}, mask_id={!r})").format(self.val, self.GetInterpretation()._id);
        });

    // Delegate arithmetic, comparison, and bitwise operators to the underlying
    // integer value so a BitField participates in expressions exactly like an int
    // (returning plain ints / bools, mirroring IntEnum semantics).
    DefIntBinaryOps(bitfield_class, "__eq__", "__ne__", "__lt__", "__le__", "__gt__", "__ge__", "__add__", "__radd__", "__sub__", "__rsub__",
                    "__mul__", "__rmul__", "__truediv__", "__rtruediv__", "__floordiv__", "__rfloordiv__", "__mod__", "__rmod__", "__divmod__",
                    "__rdivmod__", "__pow__", "__rpow__", "__and__", "__rand__", "__or__", "__ror__", "__xor__", "__rxor__", "__lshift__",
                    "__rlshift__", "__rshift__", "__rrshift__");
    DefIntUnaryOps(bitfield_class, "__neg__", "__pos__", "__abs__", "__invert__");

    // Let nanobind know that no-argument construction is supported
    // This prevents it from making its own zero-arg __new__ overload
    // https://nanobind.readthedocs.io/en/latest/classes.html#customizing-python-object-creation
    bitfield_class.def("__init__", [](py_common::PyBitField&) {});

    // Repoint  __init__ to object.__init__ (a fast C no-op) so construction work happens in __new__, mirroring the Field class above.
    bitfield_class.attr("__init__") = nb::handle(reinterpret_cast<PyObject*>(&PyBaseObject_Type)).attr("__init__");
}
