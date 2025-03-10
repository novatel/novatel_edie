#pragma once

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "novatel_edie/decoders/oem/parser.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {

class PyParser : public Parser
{
  public:
    // inherit constructors
    using Parser::Parser;

    nb::object PyRead(bool decode_incomplete);
    nb::object PyIterRead();
    nb::object PyConvert(ENCODE_FORMAT fmt, bool decode_incomplete);
};

class ConversionIterator
{
    PyParser& parser;
    ENCODE_FORMAT fmt;

  public:
    ConversionIterator(PyParser& parser_, ENCODE_FORMAT fmt_) : parser(parser_), fmt(fmt_) {}

    nb::object PyIterConvert();
};

}; // namespace novatel::edie::oem
