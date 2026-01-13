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

nb::object create_unknown_bytes(nb::bytes data);
} // namespace novatel::edie::py_common
