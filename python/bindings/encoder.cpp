#include "novatel_edie/decoders/oem/encoder.hpp"

#include "bindings_core.hpp"
#include "encoder.hpp"
#include "exceptions.hpp"
#include "message_db_singleton.hpp"
#include "py_message_data.hpp"
#include "py_message_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

oem::PyMessageData oem::PyEncoder::PyEncode(const oem::PyCompleteMessage& py_message, ENCODE_FORMAT format) const
{
    STATUS status;
    MessageDataStruct message_data = MessageDataStruct();

    if (format == ENCODE_FORMAT::JSON)
    {
        // Allocate more space for JSON messages.
        // A TRACKSTAT message can use about 47k bytes when encoded as JSON.
        // FIXME: this is still not safe and there is no effective buffer overflow checking implemented in Encoder.
        uint8_t buffer[MESSAGE_SIZE_MAX * 3];
        auto* buf_ptr = reinterpret_cast<uint8_t*>(&buffer);
        uint32_t buf_size = MESSAGE_SIZE_MAX * 3;
        status = Encode(&buf_ptr, buf_size, py_message.header, py_message.fields, message_data, py_message.header.format, format);
    }
    else
    {
        uint8_t buffer[MESSAGE_SIZE_MAX];
        auto buf_ptr = reinterpret_cast<uint8_t*>(&buffer);
        uint32_t buf_size = MESSAGE_SIZE_MAX;
        status = Encode(&buf_ptr, buf_size, py_message.header, py_message.fields, message_data, py_message.header.format, format);
    }
    throw_exception_from_status(status);
    return PyMessageData(message_data);
}

void init_novatel_encoder(nb::module_& m)
{
    nb::class_<oem::PyEncoder>(m, "Encoder")
        .def("__init__", [](oem::PyEncoder* t) { new (t) oem::PyEncoder(MessageDbSingleton::get()); }) // NOLINT(*.NewDeleteLeaks)
        .def(nb::init<PyMessageDatabase::Ptr&>(), "message_db"_a)
        .def("load_db", &oem::PyEncoder::LoadJsonDb, "message_db"_a)
        .def_prop_ro("logger", [](const oem::PyEncoder& self) { return self.GetLogger(); })
        .def("encode", &oem::PyEncoder::PyEncode, "message"_a, "encode_format"_a);
}
