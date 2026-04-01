#pragma once

#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"
#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"
#include "py_common/bindings_core.hpp"
#include "py_oem/message_database.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/py_message_objects.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_oem {

class PyDecoder
{
  public:
    py_common::PyMessageDatabaseCore::Ptr database;
    oem::HeaderDecoder header_decoder;
    oem::MessageDecoder message_decoder;
    oem::RxConfigHandler rx_config_handler;

    PyDecoder(py_common::PyMessageDatabaseCore::Ptr pclMessageDb_)
        : database(std::move(pclMessageDb_)), header_decoder(database), message_decoder(database), rx_config_handler(database)
    {
    }

    nb::object DecodeMessage(const nb::bytes raw_body, py_oem::PyHeader& header, oem::MetaDataStruct& metadata) const;
};

} // namespace novatel::edie::py_oem
