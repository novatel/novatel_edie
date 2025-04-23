#pragma once

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "novatel_edie/decoders/oem/filter.hpp"
#include "py_database.hpp"
#include "py_message_data.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {

//============================================================================
//! \class PyGpsTime
//! \brief A GPS time.
//============================================================================
struct PyGpsTime
{
    PyGpsTime() = default;
    PyGpsTime(uint16_t week_, double milliseconds_) : week(week_), milliseconds(milliseconds_) {}
    PyGpsTime(uint16_t week_, double milliseconds_, TIME_STATUS time_status_) : week(week_), milliseconds(milliseconds_), time_status(time_status_) {}

    explicit PyGpsTime(const MetaDataStruct& meta_) : week(meta_.usWeek), milliseconds(meta_.dMilliseconds), time_status(meta_.eTimeStatus) {}

    uint16_t week{0};
    double milliseconds{0.0};
    TIME_STATUS time_status{TIME_STATUS::UNKNOWN};
};

//============================================================================
//! \class UnknownBytes
//! \brief A series of bytes determined to be undecodable.
//============================================================================
struct PyUnknownBytes
{
    nb::bytes data;

    explicit PyUnknownBytes(nb::bytes data_) : data(std::move(data_)) {}
};

//============================================================================
//! \class PyMessageTypeField
//! \brief The field in a message header which gives type information.
//============================================================================
struct PyMessageTypeField
{
    uint8_t value;
    bool IsResponse() { return (value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::RESPONSE)) != 0; }
    MESSAGE_FORMAT GetFormat() { return static_cast<MESSAGE_FORMAT>((value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MSGFORMAT)) >> 5); }
    MEASUREMENT_SOURCE GetMeasurementSource() { return static_cast<MEASUREMENT_SOURCE>((value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MEASSRC))); }
};

//============================================================================
//! \class PyHeader
//! \brief A python representation for a message header.
//============================================================================
struct PyHeader : public IntermediateHeader
{
    HEADER_FORMAT format;
    PyMessageTypeField message_type;
    uint32_t raw_length;

    PyMessageTypeField GetPyMessageType()
    {
        message_type.value = ucMessageType;
        return message_type;
    }

    nb::dict to_dict() const;
};

//============================================================================
//! \class PyField
//! \brief A python representation for a single message or message field.
//!
//! Contains a vector of FieldContainer objects, which behave like attributes
//! within the Python API.
//============================================================================
struct PyField
{
    std::string name;
    bool has_ptype; // Whether the field has a specific Python type associated with it

    explicit PyField(std::string name_, bool has_ptype_, std::vector<FieldContainer> message_, PyMessageDatabase::ConstPtr parent_db_)
        : name(std::move(name_)), has_ptype(has_ptype_), fields(std::move(message_)), parent_db_(std::move(parent_db_)) {};
    nb::dict& get_values() const;
    nb::dict& get_fields() const;
    nb::dict to_dict() const;
    nb::object getattr(nb::str field_name) const;
    nb::object getitem(nb::str field_name) const;
    bool contains(nb::str field_name) const;
    size_t len() const;
    std::string repr() const;

    std::vector<FieldContainer> fields;

  protected:
    mutable nb::dict cached_values_;
    mutable nb::dict cached_fields_;

    PyMessageDatabase::ConstPtr parent_db_;
};

//============================================================================
//! \class PyUnknownMessage
//! \brief A python representation for an unknown message.
//!
//! Contains the raw bytes of the message.
//============================================================================
struct PyUnknownMessage
{
    PyHeader header;

    nb::bytes payload;
    explicit PyUnknownMessage(PyMessageDatabase::ConstPtr parent_db_, PyHeader header_, nb::bytes bytes_)
        : header(std::move(header_)), payload(std::move(bytes_))
    {
    }
};

//============================================================================
//! \class PyEncodableField
//! \brief A field which can be encoded into a byte data representation.
//!
//! Contains shared functionality for messages and message responses.
//============================================================================
struct PyEncodableField : public PyField
{
    PyHeader header;

    explicit PyEncodableField(std::string name_, bool has_ptype_, std::vector<FieldContainer> fields_, PyMessageDatabase::ConstPtr parent_db_,
                              PyHeader header_)
        : PyField(std::move(name_), has_ptype_, std::move(fields_), std::move(parent_db_)), header(std::move(header_)) {};

    PyMessageData encode(ENCODE_FORMAT fmt);
    PyMessageData to_ascii();
    PyMessageData to_abbrev_ascii();
    PyMessageData to_binary();
    PyMessageData to_flattened_binary();
    PyMessageData to_json();
};

//============================================================================
//! \class PyMessage
//! \brief A python representation for a single fully decoded message.
//============================================================================
struct PyMessage : public PyEncodableField
{
    explicit PyMessage(std::string name_, bool has_ptype_, std::vector<FieldContainer> fields_, PyMessageDatabase::ConstPtr parent_db_,
                       PyHeader header_)
        : PyEncodableField(std::move(name_), has_ptype_, std::move(fields_), std::move(parent_db_), std::move(header_)) {};
};

nb::object create_unknown_bytes(nb::bytes data);

nb::object create_unknown_message_instance(nb::bytes data, PyHeader& header, PyMessageDatabase::ConstPtr database);

nb::object create_message_instance(PyHeader& header, std::vector<FieldContainer>& message_fields, MetaDataStruct& metadata,
                                   PyMessageDatabase::ConstPtr database);

//============================================================================
//! \class PyResponse
//! \brief A python representation for a single fully decoded message.
//============================================================================
struct PyResponse : public PyEncodableField
{
    bool complete;
    explicit PyResponse(std::string name_, std::vector<FieldContainer> fields_, PyMessageDatabase::ConstPtr parent_db_, PyHeader header_,
                        bool complete_)
        : PyEncodableField(std::move(name_), false, std::move(fields_), std::move(parent_db_), std::move(header_)), complete(complete_) {};

    int32_t GetResponseId() { return std::get<int>(fields[0].fieldValue); }

    std::string GetResponseString() const { return std::get<std::string>(fields[1].fieldValue); }

    nb::object GetEnumValue()
    {
        std::unordered_map<std::string, nb::object> enum_map = parent_db_->GetEnumsByNameDict();
        auto it = enum_map.find("Responses");
        if (it == enum_map.end()) { return nb::none(); }
        nb::object response_enum = it->second;
        int32_t value = GetResponseId();
        nb::object enum_val = response_enum(value);
        return enum_val;
    }
};

} // namespace novatel::edie::oem
