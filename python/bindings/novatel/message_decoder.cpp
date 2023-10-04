#include "novatel_edie/decoders/oem/message_decoder.hpp"

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/variant.h>

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;
using IntermediateMessage = std::vector<FieldContainer>;

NB_MAKE_OPAQUE(IntermediateMessage);
// NB_MAKE_OPAQUE(oem::MsgFieldsVector);

void init_novatel_message_decoder(nb::module_& m)
{

    nb::bind_vector<IntermediateMessage>(m, "IntermediateMessage");
    //   nb::bind_vector<oem::MsgFieldsVector>(m, "MsgFieldsVector");

    nb::class_<novatel::edie::FieldContainer>(m, "FieldContainer")
        //      .def(nb::init<std::variant<container_types>, const novatel::edie::BaseField*>(), "field_value"_a, "field_def"_a)
        //      .def(nb::init<const oem::FieldContainer&>())
        .def_rw("field_value", &novatel::edie::FieldContainer::fieldValue)
        .def_rw("field_def", &novatel::edie::FieldContainer::fieldDef);

    nb::class_<oem::MessageDecoder>(m, "MessageDecoder")
        .def(nb::init<JsonReader*>(), "json_db"_a)
        .def("load_json_db", &oem::MessageDecoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", &oem::MessageDecoder::GetLogger)
        .def(
            "decode",
            [](oem::MessageDecoder& decoder, nb::bytes message_raw, oem::MetaDataStruct& metadata) {
                IntermediateMessage message;
                STATUS status = decoder.Decode((unsigned char*)message_raw.c_str(), message, metadata);
                return nb::make_tuple(status, message);
            },
            "encoded_message"_a, "metadata"_a);
}
