#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"

#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_common/message_db_singleton.hpp"
#include "py_common/py_message_data.hpp"
#include "py_oem/init_bindings.hpp"


namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::py_common;

void py_oem::init_novatel_rxconfig_handler(nb::module_& m)
{
    nb::class_<oem::RxConfigHandler>(m, "RxConfigHandler")
        .def(
            "__init__",
            [](oem::RxConfigHandler* t, py_common::PyMessageDatabaseCore::Ptr message_db) {
                if (!message_db) { message_db = MessageDbSingleton::get(); };
                new (t) oem::RxConfigHandler(message_db);
                t->GetLogger()->warn(
                    "The RXConfigHandler interface is currently unstable! It may undergo breaking changes between minor version increments.");
            },
            nb::arg("message_db") = nb::none()) // NOLINT(*.NewDeleteLeaks)
        .def("write", [](oem::RxConfigHandler& self,
                         const nb::bytes& data) { return self.Write(reinterpret_cast<uint8_t*>(const_cast<char*>(data.c_str())), data.size()); })
        .def(
            "convert",
            [](oem::RxConfigHandler& self, ENCODE_FORMAT encode_format) {
                MessageDataStruct rx_config_message_data;
                MessageDataStruct embedded_message_data;
                oem::MetaDataStruct rx_config_meta_data;
                oem::MetaDataStruct embedded_meta_data;
                STATUS status = self.Convert(rx_config_message_data, rx_config_meta_data, embedded_message_data, embedded_meta_data, encode_format);
                throw_exception_from_status(status);
                return nb::make_tuple(py_common::PyMessageData(rx_config_message_data), rx_config_meta_data,
                                      py_common::PyMessageData(embedded_message_data), embedded_meta_data);
            },
            "encode_format"_a)
        .def(
            "flush",
            [](oem::RxConfigHandler& self, bool return_flushed_bytes) -> nb::object {
                if (!return_flushed_bytes) { return nb::int_(self.Flush()); }
                uint8_t buffer[MESSAGE_SIZE_MAX];
                uint32_t count = self.Flush(buffer, MESSAGE_SIZE_MAX);
                return nb::bytes(buffer, count);
            },
            "return_flushed_bytes"_a = false);
}
