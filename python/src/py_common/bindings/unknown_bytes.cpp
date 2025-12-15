#include "py_common/unknown_bytes.hpp"

#include "py_common/bindings_core.hpp"
#include "py_common/init_bindings.hpp"
#include "py_common/py_message_data.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

void py_common::init_raw_data_classes(nb::module_& m)
{
    nb::class_<py_common::PyUnknownBytes>(m, "UnknownBytes", "A set of bytes which was determined to be undecodable by EDIE.")
        .def("__repr__",
             [](const py_common::PyUnknownBytes self) {
                 std::string byte_rep = nb::str(self.data.attr("__repr__")()).c_str();
                 return "UnknownBytes(" + byte_rep + ")";
             })
        .def_ro("data", &py_common::PyUnknownBytes::data, "The raw bytes determined to be undecodable.");

    nb::class_<novatel::edie::py_common::PyMessageData>(m, "MessageData")
        .def("__repr__", &novatel::edie::py_common::PyMessageData::GetRepr)
        .def_prop_ro("message", &novatel::edie::py_common::PyMessageData::message)
        .def_prop_ro("header", &novatel::edie::py_common::PyMessageData::header)
        .def_prop_ro("payload", &novatel::edie::py_common::PyMessageData::body);
}
