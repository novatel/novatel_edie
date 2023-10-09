#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace nb = nanobind;
using namespace novatel::edie;
using IntermediateMessage = std::vector<FieldContainer>;

struct PyIntermediateMessage
{
    explicit PyIntermediateMessage(IntermediateMessage message_);
    nb::object get(nb::str field_name);
    std::string repr();

    IntermediateMessage message;
    nb::dict values;
    nb::dict fields;
};
