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

    // Bound as __new__ (with __init__ repointed to object.__init__ below) so it composes
    // with the dynamically-created concrete subtypes. The base BitField is only the shared
    // base for the database's generated per-bitmask subtypes and is not constructible on its
    // own: only a concrete subtype (which carries an `_owner_db`) can be instantiated, and it
    // resolves its bitmask interpretation by using the class handle as a lookup key, mirroring
    // PyField::py_new. (Interpreted base instances still arise internally on the decode path
    // via nb::cast in PyField::convert_field, which does not route through this __new__.)
    bitfield_class.def_static(
        "__new__",
        [](nb::handle cls, nb::object value, nb::kwargs kwargs) {
            if (cls.is(nb::type<py_common::PyBitField>()))
            {
                throw nb::type_error("BitField cannot be instantiated directly; it is the base class for the "
                                     "database's generated bitmask types.");
            }
            BitMaskMap::ConstPtr interpretation = nullptr;
            py_common::PyMessageDatabase::ConstPtr database = nullptr;
            if (nb::hasattr(cls, "_owner_db"))
            {
                database = nb::cast<py_common::PyMessageDatabase::Ptr>(cls.attr("_owner_db"));
                if (database) { interpretation = database->GetBitFieldTypeLookup(cls); }
            }

            uint32_t packed_value = value.is_none() ? 0 : nb::cast<uint32_t>(value);
            if (interpretation)
            {
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
                 // add dynamic sub-mask names to the list (none for an int-constructed BitField)
                 py_common::PyBitField* body = nb::inst_ptr<py_common::PyBitField>(self);
                 if (body->interpretation)
                 {
                     for (const auto& [mask_name, mask_entry] : body->interpretation->masks) { base_list.append(nb::cast(mask_name)); }
                 }

                 return base_list;
             })
        // Integer conversions so a BitField can be used wherever an int is expected.
        .def("__int__", [](const py_common::PyBitField& self) { return nb::int_(self.val); })
        .def("__index__", [](const py_common::PyBitField& self) { return nb::int_(self.val); })
        .def("__float__", [](const py_common::PyBitField& self) { return nb::float_(static_cast<double>(self.val)); })
        .def("__bool__", [](const py_common::PyBitField& self) { return self.val != 0; })
        .def("__hash__", [](const py_common::PyBitField& self) { return nb::int_(self.val).attr("__hash__")(); })
        .def("__repr__", [](const py_common::PyBitField& self) {
            if (self.interpretation) { return nb::str("BitField(value={}, mask_id={!r})").format(self.val, self.interpretation->_id); }
            return nb::str("BitField(value={})").format(self.val);
        });

    // Delegate arithmetic, comparison, and bitwise operators to the underlying
    // integer value so a BitField participates in expressions exactly like an int
    // (returning plain ints / bools, mirroring IntEnum semantics).
#define EDIE_BITFIELD_BINOP(NAME)                                                                                                                    \
    bitfield_class.def(NAME, [](const py_common::PyBitField& self, nb::object other) {                                                               \
        /* Coerce a BitField operand to its int value so BitField-vs-BitField ops don't fall back to identity. */                                    \
        if (nb::isinstance<py_common::PyBitField>(other)) { other = nb::int_(nb::cast<const py_common::PyBitField&>(other).val); }                   \
        return nb::int_(self.val).attr(NAME)(other);                                                                                                 \
    });
    EDIE_BITFIELD_BINOP("__eq__")
    EDIE_BITFIELD_BINOP("__ne__")
    EDIE_BITFIELD_BINOP("__lt__")
    EDIE_BITFIELD_BINOP("__le__")
    EDIE_BITFIELD_BINOP("__gt__")
    EDIE_BITFIELD_BINOP("__ge__")
    EDIE_BITFIELD_BINOP("__add__")
    EDIE_BITFIELD_BINOP("__radd__")
    EDIE_BITFIELD_BINOP("__sub__")
    EDIE_BITFIELD_BINOP("__rsub__")
    EDIE_BITFIELD_BINOP("__mul__")
    EDIE_BITFIELD_BINOP("__rmul__")
    EDIE_BITFIELD_BINOP("__truediv__")
    EDIE_BITFIELD_BINOP("__rtruediv__")
    EDIE_BITFIELD_BINOP("__floordiv__")
    EDIE_BITFIELD_BINOP("__rfloordiv__")
    EDIE_BITFIELD_BINOP("__mod__")
    EDIE_BITFIELD_BINOP("__rmod__")
    EDIE_BITFIELD_BINOP("__divmod__")
    EDIE_BITFIELD_BINOP("__rdivmod__")
    EDIE_BITFIELD_BINOP("__pow__")
    EDIE_BITFIELD_BINOP("__rpow__")
    EDIE_BITFIELD_BINOP("__and__")
    EDIE_BITFIELD_BINOP("__rand__")
    EDIE_BITFIELD_BINOP("__or__")
    EDIE_BITFIELD_BINOP("__ror__")
    EDIE_BITFIELD_BINOP("__xor__")
    EDIE_BITFIELD_BINOP("__rxor__")
    EDIE_BITFIELD_BINOP("__lshift__")
    EDIE_BITFIELD_BINOP("__rlshift__")
    EDIE_BITFIELD_BINOP("__rshift__")
    EDIE_BITFIELD_BINOP("__rrshift__")
#undef EDIE_BITFIELD_BINOP

#define EDIE_BITFIELD_UNOP(NAME) bitfield_class.def(NAME, [](const py_common::PyBitField& self) { return nb::int_(self.val).attr(NAME)(); });
    EDIE_BITFIELD_UNOP("__neg__")
    EDIE_BITFIELD_UNOP("__pos__")
    EDIE_BITFIELD_UNOP("__abs__")
    EDIE_BITFIELD_UNOP("__invert__")
#undef EDIE_BITFIELD_UNOP

    // Register a nullary __init__ purely to mark the type as having a no-argument
    // constructor (type_flags::has_nullary_new). Without it, nanobind's type-call fast path
    // sends a bare `BitField()` (no args, no kwargs) down its hidden unpickling __new__ path,
    // which never reaches the guard in __new__ above and reports a cryptic overload error
    // instead of the clear "cannot be instantiated directly" message. The body never runs:
    // the fast path skips __init__ when a custom __new__ exists, and the Python-visible
    // __init__ is repointed to object.__init__ just below (so subtypes inherit the no-op).
    bitfield_class.def("__init__", [](py_common::PyBitField&) {});

    // Concrete bitfield subtypes are generated per bitmask at database-load time
    // and instantiated via nb::inst_alloc in PyField::convert_field. Repoint
    // __init__ to object.__init__ (a fast C no-op) so construction work happens in
    // __new__, mirroring the Field class above.
    bitfield_class.attr("__init__") = nb::handle(reinterpret_cast<PyObject*>(&PyBaseObject_Type)).attr("__init__");
}
