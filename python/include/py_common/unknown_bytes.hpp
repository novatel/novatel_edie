#pragma once

#include "py_common/bindings_core.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_common {
//============================================================================
//! \class UnknownBytes
//! \brief A series of bytes determined to be undecodable.
//============================================================================
struct PyUnknownBytes
{
    nb::bytes data;

    explicit PyUnknownBytes(nb::bytes data_) : data(std::move(data_)) {}
};

inline nb::object create_unknown_bytes(nb::bytes data)
{
    nb::handle data_pytype = nb::type<PyUnknownBytes>();
    nb::object data_pyinst = nb::inst_alloc(data_pytype);
    PyUnknownBytes* data_cinst = nb::inst_ptr<PyUnknownBytes>(data_pyinst);
    new (data_cinst) PyUnknownBytes(data);
    nb::inst_mark_ready(data_pyinst);
    return data_pyinst;
}

} // namespace novatel::edie::py_common
