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
};

class ConversionIterator
{
    PyParser& parser;

  public:
    ConversionIterator(PyParser& parser_) : parser(parser_) {}

    PyMessageData Convert(bool decode_incomplete);
};

}; // namespace novatel::edie::oem
