#pragma once

#include "bindings_core.hpp"
#include "py_database.hpp"
#include "message_db_singleton.hpp"
#include "py_message_objects.hpp"

#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {

class PyDecoder
{
  public:
    PyMessageDatabase::Ptr database;
    HeaderDecoder header_decoder;
    MessageDecoder message_decoder;

    PyDecoder(PyMessageDatabase::Ptr pclMessageDb_) : database(pclMessageDb_), header_decoder(pclMessageDb_->GetCoreDatabase()), message_decoder(pclMessageDb_->GetCoreDatabase()) {}

    nb::object DecodeMessage(const nb::bytes raw_body, oem::PyHeader& header, oem::MetaDataStruct& metadata) const;
};

} // namespace novatel::edie::oem
