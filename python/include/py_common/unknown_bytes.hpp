#pragma once

#include "novatel_edie/decoders/common/common.hpp"
#include "py_common/bindings_core.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_common {

//============================================================================
//! \class UNKNOWN_REASON
//! \brief Describes the reason that a series of bytes is unknown.
//============================================================================
enum class UNKNOWN_REASON
{
    UNKNOWN,
    NMEA
};

//============================================================================
//! \class PyUnknownBytes
//! \brief A series of bytes determined to be undecodable.
//============================================================================
struct PyUnknownBytes
{
    nb::bytes data;
    UNKNOWN_REASON reason;

    explicit PyUnknownBytes(nb::bytes data_, UNKNOWN_REASON reason_) : data(std::move(data_)), reason(reason_) {}
};

nb::object create_unknown_bytes(nb::bytes data, const MetaDataBase& metadata);
} // namespace novatel::edie::py_common
