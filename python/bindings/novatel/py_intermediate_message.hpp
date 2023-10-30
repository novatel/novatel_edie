#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace nb = nanobind;
using namespace novatel::edie;
using IntermediateMessage = std::vector<FieldContainer>;

struct PyIntermediateMessage
{
    explicit PyIntermediateMessage(IntermediateMessage message_);
    nb::dict& values() const;
    nb::dict& fields() const;
    nb::dict to_dict() const;
    nb::object getattr(nb::str field_name) const;
    nb::object getitem(nb::str field_name) const;
    bool contains(nb::str field_name) const;
    size_t len() const;
    std::string repr() const;

    IntermediateMessage message;

  private:
    mutable nb::dict cached_values_;
    mutable nb::dict cached_fields_;
};
