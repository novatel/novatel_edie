#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace nb = nanobind;
using namespace novatel::edie;
using IntermediateMessage = std::vector<FieldContainer>;

struct PyIntermediateMessage
{
    explicit PyIntermediateMessage(IntermediateMessage message_);
    nb::object getattr(nb::str field_name) const;
    nb::object getitem(nb::str field_name) const;
    bool contains(nb::str field_name) const;
    nb::object fields() const;
    nb::object to_dict() const;
    std::string repr() const;

    IntermediateMessage message;
    nb::dict values;

  private:
    mutable nb::object cached_fields_;
};
