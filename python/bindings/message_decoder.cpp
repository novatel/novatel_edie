#include "novatel_edie/decoders/oem/message_decoder.hpp"

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/string.h>

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "novatel_edie/decoders/oem/common.hpp"
#include "py_decoded_message.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;

NB_MAKE_OPAQUE(std::vector<FieldContainer>);

nb::object convert_field(const FieldContainer& field, const PyMessageDatabase::ConstPtr& parent_db, std::string parent)
{
    if (field.fieldDef->type == FIELD_TYPE::ENUM)
    {
        // Handle Enums
        const std::string& enumId = static_cast<const EnumField*>(field.fieldDef.get())->enumId;
        auto it = parent_db->GetEnumsByIdDict().find(enumId);
        if (it == parent_db->GetEnumsByIdDict().end())
        {
            throw std::runtime_error("Enum definition for " + field.fieldDef->name + " field with ID '" + enumId +
                                     "' not found in the JSON database");
        }
        nb::object enum_type = it->second;
        return std::visit([&](auto&& value) { return enum_type(value); }, field.fieldValue);
    }
    else if (std::holds_alternative<std::vector<FieldContainer>>(field.fieldValue))
    {
        const auto& message_field = std::get<std::vector<FieldContainer>>(field.fieldValue);
        if (message_field.empty())
        {
            // Handle Empty Arrays
            return nb::list();
        }
        else if (field.fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
        {
            // Handle Field Arrays
            std::vector<nb::object> sub_values;
            sub_values.reserve(message_field.size());
            nb::handle field_ptype;
            std::string field_name = parent + "_" + field.fieldDef->name + "_Field";
            try {
                field_ptype = parent_db->GetMessagesByNameDict().at(field_name);
            } catch (const std::out_of_range& e) {
                field_ptype = parent_db->GetMessagesByNameDict().at("UNKNOWN_Body");
            }
            for (const auto& subfield : message_field) { 
                nb::object pyinst = nb::inst_alloc(field_ptype);
                PyMessageBody* cinst = nb::inst_ptr<PyMessageBody>(pyinst);
                const auto& message_subfield = std::get<std::vector<FieldContainer>>(subfield.fieldValue);
                new (cinst) PyMessageBody(message_subfield, parent_db, field_name);
                nb::inst_mark_ready(pyinst);
                sub_values.push_back(pyinst); 
            }
            return nb::cast(sub_values);
        }
        else
        {
            // Handle Fixed or Variable-Length Arrays
            if (field.fieldDef->conversion == "%s")
            {
                // The array is actually a string
                std::string str;
                str.reserve(message_field.size());
                for (const auto& sub_field : message_field)
                {
                    auto c = std::get<uint8_t>(sub_field.fieldValue);
                    if (c == 0) { break; }
                    str.push_back(c);
                }
                return nb::cast(str);
            }
            std::vector<nb::object> sub_values;
            sub_values.reserve(message_field.size());
            for (const auto& f : message_field) { sub_values.push_back(convert_field(f, parent_db, parent)); }
            return nb::cast(sub_values);
        }
    }
    else if (field.fieldDef->conversion == "%id")
    {
        // Handle Satellite IDs
        const uint32_t temp_id = std::get<uint32_t>(field.fieldValue);
        SatelliteId sat_id;
        sat_id.usPrnOrSlot = temp_id & 0x0000FFFF;
        sat_id.sFrequencyChannel = (temp_id & 0xFFFF0000) >> 16;
        return nb::cast(sat_id);
    }
    else
    {
        // Handle most types by simply extracting the value from the variant and casting
        return std::visit([](auto&& value) { return nb::cast(value); }, field.fieldValue);
    }
}

PyMessageBody::PyMessageBody(std::vector<FieldContainer> message_, PyMessageDatabase::ConstPtr parent_db_, std::string name_)
    : fields(std::move(message_)), parent_db_(std::move(parent_db_)), name(std::move(name_))
{}

nb::dict& PyMessageBody::get_values() const
{
    if (cached_values_.size() == 0)
    {
        for (const auto& field : fields) { cached_values_[nb::cast(field.fieldDef->name)] = convert_field(field, parent_db_, name); }
    }
    return cached_values_;
}

nb::dict& PyMessageBody::get_fields() const
{
    if (cached_fields_.size() == 0)
    {
        for (const auto& field : fields) { cached_fields_[nb::cast(field.fieldDef->name)] = field.fieldDef; }
    }
    return cached_fields_;
}

nb::dict PyMessageBody::to_dict() const
{
    nb::dict dict;
    for (const auto& [field_name, value] : get_values())
    {
        if (nb::isinstance<PyMessageBody>(value)) { dict[field_name] = nb::cast<PyMessageBody>(value).to_dict(); }
        else if (nb::isinstance<std::vector<nb::object>>(value))
        {
            nb::list list;
            for (const auto& sub_item : nb::cast<std::vector<nb::object>>(value))
            {
                if (nb::isinstance<PyMessageBody>(sub_item)) { list.append(nb::cast<PyMessageBody>(sub_item).to_dict()); }
                else { list.append(sub_item); }
            }
            dict[field_name] = list;
        }
        else { dict[field_name] = value; }
    }
    return dict;
}

nb::object PyMessageBody::getattr(nb::str field_name) const
{
    if (!contains(field_name)) { throw nb::attribute_error(field_name.c_str()); }
    return get_values()[std::move(field_name)];
}

nb::object PyMessageBody::getitem(nb::str field_name) const { return get_values()[std::move(field_name)]; }

bool PyMessageBody::contains(nb::str field_name) const { return get_values().contains(std::move(field_name)); }

size_t PyMessageBody::len() const { return fields.size(); }

std::string PyMessageBody::repr() const
{
    std::stringstream repr;
    repr << name << "(";
    bool first = true;
    for (const auto& [field_name, value] : get_values())
    {
        if (!first) { repr << ", "; }
        first = false;
        repr << nb::str("{}={!r}").format(field_name, value).c_str();
    }
    repr << ")";
    return repr.str();
}

// For unit tests
class DecoderTester : public oem::MessageDecoder
{
  public:
    [[nodiscard]] STATUS TestDecodeBinary(const std::vector<BaseField::Ptr>& MsgDefFields_, const uint8_t** ppucLogBuf_,
                                          std::vector<FieldContainer>& vIntermediateFormat_, uint32_t uiMessageLength_) const
    {
        return DecodeBinary(MsgDefFields_, ppucLogBuf_, vIntermediateFormat_, uiMessageLength_);
    }

    [[nodiscard]] STATUS TestDecodeAscii(const std::vector<BaseField::Ptr>& MsgDefFields_, const char** ppcLogBuf_,
                                         std::vector<FieldContainer>& vIntermediateFormat_) const
    {
        return DecodeAscii<false>(MsgDefFields_, ppcLogBuf_, vIntermediateFormat_);
    }
};

PyMessageDatabase::ConstPtr get_parent_db(const oem::MessageDecoder& decoder)
{
    return std::dynamic_pointer_cast<const PyMessageDatabase>(decoder.MessageDb());
}

void init_novatel_message_decoder(nb::module_& m)
{
    nb::class_<PyGpsTime>(m, "GpsTime")
        .def(nb::init())
        .def(nb::init<uint16_t, double, TIME_STATUS>(), "week"_a, "milliseconds"_a, "time_status"_a = TIME_STATUS::UNKNOWN)
        .def_rw("week", &PyGpsTime::week)
        .def_rw("milliseconds", &PyGpsTime::milliseconds)
        .def_rw("status", &PyGpsTime::time_status);

    nb::class_<PyMessageBody>(m, "MessageBody")
        .def_prop_ro("_values", &PyMessageBody::get_values)
        .def_prop_ro("_fields", &PyMessageBody::get_fields)
        .def("to_dict", &PyMessageBody::to_dict, "Convert the message and its sub-messages into a dict")
        .def("__getattr__", &PyMessageBody::getattr, "field_name"_a)
        .def("__repr__", &PyMessageBody::repr)
        .def("__str__", &PyMessageBody::repr)
        .def("__dir__", [](nb::object self) {
            // get required Python builtin functions
            nb::module_ builtins = nb::module_::import_("builtins");
            nb::handle super = builtins.attr("super");
            nb::handle type = builtins.attr("type");
            // get base MessageBody type from concrete instance
            nb::handle body_type = (type(self).attr("__bases__"))[0];
            // retrieve base list based on superclass method
            nb::object super_obj = super(body_type, self);
            nb::list base_list = nb::cast<nb::list>(super_obj.attr("__dir__")());
            // add dynamic fields to the list
            PyMessageBody* body = nb::inst_ptr<PyMessageBody>(self);
            for (const auto& [field_name, _] : body->get_fields()) { base_list.append(field_name); }

            return base_list;
        });

    nb::class_<PyMessage>(m, "Message")
        .def_ro("body", &PyMessage::message_body)
        .def_ro("header", &PyMessage::header)
        .def_ro("name", &PyMessage::name)
        .def("to_dict", [](const PyMessage& self) {
            nb::dict message_dict;
            message_dict["header"] = self.header.attr("to_dict")();
            message_dict["body"] = self.message_body.attr("to_dict")();
            return message_dict;
        })
        .def("__repr__", &PyMessage::repr)
        .def("__str__", &PyMessage::repr);

    nb::class_<FieldContainer>(m, "FieldContainer")
        .def_rw("value", &FieldContainer::fieldValue)
        .def_rw("field_def", &FieldContainer::fieldDef, nb::rv_policy::reference_internal)
        .def("__repr__", [](const FieldContainer& container) {
            return nb::str("FieldContainer(value={}, fieldDef={})").format(container.fieldValue, container.fieldDef);
        });

    nb::class_<oem::MessageDecoder>(m, "MessageDecoder")
        .def("__init__", [](oem::MessageDecoder* t) { new (t) oem::MessageDecoder(MessageDbSingleton::get()); }) // NOLINT(*.NewDeleteLeaks)
        .def(nb::init<const PyMessageDatabase::Ptr&>(), "json_db"_a)
        .def("load_json_db", &oem::MessageDecoder::LoadJsonDb, "json_db"_a)
        .def_prop_ro("logger", [](oem::MessageDecoder& decoder) { return decoder.GetLogger(); })
        .def(
            "decode",
            [](const oem::MessageDecoder& decoder, const nb::bytes& message_body, nb::object header, oem::MetaDataStruct& metadata) {
                std::vector<FieldContainer> fields;
                STATUS status = decoder.Decode(reinterpret_cast<const uint8_t*>(message_body.c_str()), fields, metadata);
                PyMessageDatabase::ConstPtr parent_db = get_parent_db(decoder);
                nb::handle message_pytype;
                nb::handle body_pytype;
                const std::string message_name = metadata.MessageName();
                const std::string message_body_name = metadata.MessageName() + "_Body";

                try {
                    message_pytype = parent_db->GetMessagesByNameDict().at(message_name);
                    body_pytype = parent_db->GetMessagesByNameDict().at(message_body_name);
                } catch (const std::out_of_range& e)
                {
                    message_pytype = parent_db->GetMessagesByNameDict().at("UNKNOWN");
                    body_pytype = parent_db->GetMessagesByNameDict().at("UNKNOWN_Body");
                }


                nb::object body_pyinst = nb::inst_alloc(body_pytype);
                PyMessageBody* body_cinst = nb::inst_ptr<PyMessageBody>(body_pyinst);
                new (body_cinst) PyMessageBody(std::move(fields), parent_db, message_body_name);
                nb::inst_mark_ready(body_pyinst);

                nb::object message_pyinst = nb::inst_alloc(message_pytype);
                PyMessage* message_cinst = nb::inst_ptr<PyMessage>(message_pyinst);
                new (message_cinst) PyMessage(body_pyinst, header, message_name);
                nb::inst_mark_ready(message_pyinst);

                return nb::make_tuple(status, message_pyinst);
            },
            "message_body"_a, "decoded_header"_a, "metadata"_a)
        .def(
            "_decode_ascii",
            [](oem::MessageDecoder& decoder, const std::vector<BaseField::Ptr>& msg_def_fields, const nb::bytes& message_body) {
                std::vector<FieldContainer> fields;
                // Copy to ensure that the byte string is zero-delimited
                std::string body_str(message_body.c_str(), message_body.size());
                const char* data_ptr = body_str.c_str();
                STATUS status = static_cast<DecoderTester*>(&decoder)->TestDecodeAscii(msg_def_fields, &data_ptr, fields);
            return nb::make_tuple(status, PyMessageBody(std::move(fields), get_parent_db(decoder), "UNKNOWN_Body"));
        },
        "msg_def_fields"_a, "message_body"_a)
        .def(
            "_decode_binary",
            [](oem::MessageDecoder& decoder, const std::vector<BaseField::Ptr>& msg_def_fields, const nb::bytes& message_body,
                uint32_t message_length) {
                std::vector<FieldContainer> fields;
                const char* data_ptr = message_body.c_str();
                STATUS status = static_cast<DecoderTester*>(&decoder)->TestDecodeBinary(msg_def_fields, reinterpret_cast<const uint8_t**>(&data_ptr),
                                                                                        fields, message_length);
            return nb::make_tuple(status, PyMessageBody(std::move(fields), get_parent_db(decoder), "UNKNOWN_Body"));
        },
        "msg_def_fields"_a, "message_body"_a, "message_length"_a);
 }
