#include "novatel_edie/decoders/oem/message_decoder.hpp"

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/variant.h>

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "novatel_edie/decoders/oem/common.hpp"
#include "py_decoded_message.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;
using namespace novatel::edie::oem;

NB_MAKE_OPAQUE(std::vector<FieldContainer>);

nb::object convert_field(const FieldContainer& field, const PyMessageDatabase::ConstPtr& parent_db)
{
    if (field.fieldDef->type == FIELD_TYPE::ENUM)
    {
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
            // Empty array
            return nb::list();
        }
        else if (message_field[0].fieldDef->type == field.fieldDef->type && message_field[0].fieldDef->name == field.fieldDef->name)
        {
            // Fixed-length, variable-length and field arrays are stored as a field
            // with a list of sub-fields of the same type and name.
            // This needs to be un-nested for the translated Python structure.
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
            for (const auto& f : message_field) { sub_values.push_back(convert_field(f, parent_db)); }
            return nb::cast(sub_values);
        }
        else
        {
            // This is an array element of a field array.
            return nb::cast(PyDecodedMessage(message_field, {}, parent_db));
        }
    }
    else if (field.fieldDef->conversion == "%id")
    {
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

PyDecodedMessage::PyDecodedMessage(std::vector<FieldContainer> message_, const oem::MetaDataStruct& meta_, PyMessageDatabase::ConstPtr parent_db_)
    : fields(std::move(message_)), message_id(meta_.usMessageId), message_crc(meta_.uiMessageCrc), message_name(meta_.acMessageName), time(meta_),
      measurement_source(meta_.eMeasurementSource), constellation(meta_.constellation), parent_db_(std::move(parent_db_))
{
}

nb::dict& PyDecodedMessage::get_values() const
{
    if (cached_values_.size() == 0)
    {
        for (const auto& field : fields) { cached_values_[nb::cast(field.fieldDef->name)] = convert_field(field, parent_db_); }
    }
    return cached_values_;
}

nb::dict& PyDecodedMessage::get_fields() const
{
    if (cached_fields_.size() == 0)
    {
        for (const auto& field : fields) { cached_fields_[nb::cast(field.fieldDef->name)] = field.fieldDef; }
    }
    return cached_fields_;
}

nb::dict PyDecodedMessage::to_dict() const
{
    nb::dict dict;
    for (const auto& [field_name, value] : get_values())
    {
        if (nb::isinstance<PyDecodedMessage>(value)) { dict[field_name] = nb::cast<PyDecodedMessage>(value).to_dict(); }
        else if (nb::isinstance<std::vector<nb::object>>(value))
        {
            nb::list list;
            for (const auto& sub_item : nb::cast<std::vector<nb::object>>(value))
            {
                if (nb::isinstance<PyDecodedMessage>(sub_item)) { list.append(nb::cast<PyDecodedMessage>(sub_item).to_dict()); }
                else { list.append(sub_item); }
            }
            dict[field_name] = list;
        }
        else { dict[field_name] = value; }
    }
    return dict;
}

nb::object PyDecodedMessage::getattr(nb::str field_name) const
{
    if (!contains(field_name)) { throw nb::attribute_error(field_name.c_str()); }
    return get_values()[std::move(field_name)];
}

nb::object PyDecodedMessage::getitem(nb::str field_name) const { return get_values()[std::move(field_name)]; }

bool PyDecodedMessage::contains(nb::str field_name) const { return get_values().contains(std::move(field_name)); }

size_t PyDecodedMessage::len() const { return fields.size(); }

std::string PyDecodedMessage::repr() const
{
    std::stringstream repr;
    repr << "<";
    if (!message_name.empty()) { repr << message_name << " "; }
    bool first = true;
    for (const auto& [field_name, value] : get_values())
    {
        if (!first) { repr << ", "; }
        first = false;
        repr << nb::str("{}={!r}").format(field_name, value).c_str();
    }
    repr << ">";
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

    nb::class_<PyDecodedMessage>(m, "Message")
        .def_rw("message_id", &PyDecodedMessage::message_id)
        .def_rw("message_crc", &PyDecodedMessage::message_crc)
        .def_rw("message_name", &PyDecodedMessage::message_name)
        .def_rw("message_time", &PyDecodedMessage::time)
        .def_rw("message_measurement_source", &PyDecodedMessage::measurement_source)
        .def_rw("message_constellation", &PyDecodedMessage::constellation)
        .def_prop_ro("_values", &PyDecodedMessage::get_values)
        .def_prop_ro("_fields", &PyDecodedMessage::get_fields)
        .def("to_dict", &PyDecodedMessage::to_dict, "Convert the message and its sub-messages into a dict")
        .def("__getattr__", &PyDecodedMessage::getattr, "field_name"_a)
        .def("__getitem__", &PyDecodedMessage::getitem, "field_name"_a)
        .def("__contains__", &PyDecodedMessage::contains, "field_name"_a)
        .def("__len__", &PyDecodedMessage::len)
        .def("__repr__", &PyDecodedMessage::repr)
        .def("__str__", &PyDecodedMessage::repr);

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
            [](const oem::MessageDecoder& decoder, const nb::bytes& message_body, oem::MetaDataStruct& metadata) {
                std::vector<FieldContainer> fields;
                STATUS status = decoder.Decode(reinterpret_cast<const uint8_t*>(message_body.c_str()), fields, metadata);
                return nb::make_tuple(status, PyDecodedMessage(std::move(fields), metadata, get_parent_db(decoder)));
            },
            "message_body"_a, "metadata"_a)
        // For internal testing purposes only
        .def(
            "_decode_ascii",
            [](oem::MessageDecoder& decoder, const std::vector<BaseField::Ptr>& msg_def_fields, const nb::bytes& message_body) {
                std::vector<FieldContainer> fields;
                // Copy to ensure that the byte string is zero-delimited
                std::string body_str(message_body.c_str(), message_body.size());
                const char* data_ptr = body_str.c_str();
                STATUS status = static_cast<DecoderTester*>(&decoder)->TestDecodeAscii(msg_def_fields, &data_ptr, fields);
                return nb::make_tuple(status, PyDecodedMessage(std::move(fields), {}, get_parent_db(decoder)));
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
                return nb::make_tuple(status, PyDecodedMessage(std::move(fields), {}, get_parent_db(decoder)));
            },
            "msg_def_fields"_a, "message_body"_a, "message_length"_a);
}
