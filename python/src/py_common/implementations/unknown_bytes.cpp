#include "py_common/unknown_bytes.hpp"

#include "py_common/bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

nb::object py_common::create_unknown_bytes(nb::bytes data)
{
    nb::handle data_pytype = nb::type<py_common::PyUnknownBytes>();
    nb::object data_pyinst = nb::inst_alloc(data_pytype);
    py_common::PyUnknownBytes* data_cinst = nb::inst_ptr<py_common::PyUnknownBytes>(data_pyinst);
    new (data_cinst) py_common::PyUnknownBytes(data);
    nb::inst_mark_ready(data_pyinst);
    return data_pyinst;
}
