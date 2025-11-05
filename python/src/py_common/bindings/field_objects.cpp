#include "py_common/init_bindings.hpp"
#include "py_common/field_objects.hpp"

#include <variant>

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void py_common::init_field_objects(nb::module_& m) {
    nb::class_<py_common::PyField>(m, "Field")
        .def("__getattr__", &py_common::PyField::getattr, "field_name"_a)
        .def("__repr__", &py_common::PyField::repr)
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
}
