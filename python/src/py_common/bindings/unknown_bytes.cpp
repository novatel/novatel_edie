#include "py_common/unknown_bytes.hpp"

#include "py_common/bindings_core.hpp"
#include "py_common/init_bindings.hpp"
#include "py_common/py_message_data.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

PYCOMMON_EXPORT nb::object py_common::create_unknown_bytes(nb::bytes data, const MetaDataBase& metadata)
{
    nb::handle data_pytype = nb::type<py_common::PyUnknownBytes>();
    nb::object data_pyinst = nb::inst_alloc(data_pytype);
    py_common::PyUnknownBytes* data_cinst = nb::inst_ptr<py_common::PyUnknownBytes>(data_pyinst);
    UNKNOWN_REASON reason = metadata.eFormat == HEADER_FORMAT::NMEA ? UNKNOWN_REASON::NMEA : UNKNOWN_REASON::UNKNOWN;
    new (data_cinst) py_common::PyUnknownBytes(data, reason);
    nb::inst_mark_ready(data_pyinst);
    return data_pyinst;
}

void py_common::init_raw_data_classes(nb::module_& m)
{
    nb::enum_<UNKNOWN_REASON>(m, "UNKNOWN_REASON", "Indicates the reason why a set of bytes could not be decoded.")
        .value("UNKNOWN", UNKNOWN_REASON::UNKNOWN, "The bytes are of an unknown format.")
        .value("NMEA", UNKNOWN_REASON::NMEA, "The bytes correspond to an NMEA message.")
        .def("__str__", [](nb::handle self) { return getattr(self, "__name__"); });

    nb::class_<py_common::PyUnknownBytes>(m, "UnknownBytes", "A set of bytes which was determined to be undecodable by EDIE.")
        .def("__repr__",
             [](const py_common::PyUnknownBytes self) {
                 std::string byte_rep = nb::str(self.data.attr("__repr__")()).c_str();
                 return "UnknownBytes(" + byte_rep + ")";
             })
        .def_ro("data", &py_common::PyUnknownBytes::data, "The raw bytes determined to be undecodable.")
        .def_ro("reason", &py_common::PyUnknownBytes::reason, "The reason why the bytes could not be decoded.");

    nb::class_<novatel::edie::py_common::PyMessageData>(m, "MessageData")
        .def("__repr__", &novatel::edie::py_common::PyMessageData::GetRepr)
        .def_prop_ro("message", &novatel::edie::py_common::PyMessageData::message)
        .def_prop_ro("header", &novatel::edie::py_common::PyMessageData::header)
        .def_prop_ro("payload", &novatel::edie::py_common::PyMessageData::body);
}
