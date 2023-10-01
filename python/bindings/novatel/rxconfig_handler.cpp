#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"

#include "bindings_core.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_rxconfig_handler(nb::module_& m)
{
    nb::class_<oem::RxConfigHandler>(m, "RxConfigHandler")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("load_json_db", &oem::RxConfigHandler::LoadJsonDb, "json_db_path"_a)
        .def_prop_ro("logger", &oem::RxConfigHandler::GetLogger)
        .def("write", [](oem::RxConfigHandler& self, nb::bytes data) { return self.Write((unsigned char*)data.c_str(), data.size()); })
        .def(
            "convert",
            [](oem::RxConfigHandler& self, ENCODE_FORMAT encode_format) {
                MessageDataStruct stRxConfigMessageData, stEmbeddedMessageData;
                oem::MetaDataStruct stRxConfigMetaData, stEmbeddedMetaData;
                STATUS status = self.Convert(stRxConfigMessageData, stRxConfigMetaData, stEmbeddedMessageData, stEmbeddedMetaData, encode_format);
                return nb::make_tuple(status, stRxConfigMessageData, stRxConfigMetaData, stEmbeddedMessageData, stEmbeddedMetaData);
            },
            "encode_format"_a)
        .def(
            "flush",
            [](oem::RxConfigHandler& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) return nb::int_(self.Flush());
                char buffer[MESSAGE_SIZE_MAX];
                uint32_t count = self.Flush((unsigned char*)buffer, MESSAGE_SIZE_MAX);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false);
}
