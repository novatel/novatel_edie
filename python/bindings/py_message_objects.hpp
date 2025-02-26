#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/filter.hpp"
#include "py_message_data.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {

struct PyGpsTime
{
    PyGpsTime() = default;
    PyGpsTime(uint16_t week_, double milliseconds_, TIME_STATUS time_status_) : week(week_), milliseconds(milliseconds_), time_status(time_status_) {}
    explicit PyGpsTime(const MetaDataStruct& meta_) : week(meta_.usWeek), milliseconds(meta_.dMilliseconds), time_status(meta_.eTimeStatus) {}

    uint16_t week{0};
    double milliseconds{0.0};
    TIME_STATUS time_status{TIME_STATUS::UNKNOWN};
};

//============================================================================
//! \class PyField
//! \brief A python representation for a log header.
//============================================================================
struct PyMessageTypeField
{
    uint8_t value;
    bool IsResponse() { return (value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::RESPONSE)) != 0; }
    MESSAGE_FORMAT GetFormat() { return static_cast<MESSAGE_FORMAT>((value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MSGFORMAT)) >> 5); }
    MEASUREMENT_SOURCE GetMeasurementSource() { return static_cast<MEASUREMENT_SOURCE>((value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MEASSRC))); }
};

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
//! \brief A python representation for a single log message or message field.
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
//! \class PyMessage
//! \brief A python representation for a single log message.
//!
//! Extends PyField with reference to the Python represenation of a Header.
//============================================================================
struct PyMessage : public PyField
{
  public:
    PyHeader header;

    PyMessage(std::string name_, bool has_ptype_, std::vector<FieldContainer> fields_, PyMessageDatabase::ConstPtr parent_db_, PyHeader header_)
        : PyField(std::move(name_), has_ptype_, std::move(fields_), std::move(parent_db_)), header(std::move(header_))
    {
    }
};

//============================================================================
//! \class PyIncompleteMessage
//! \brief A python representation for an unknown log message.
//!
//! Contains the raw bytes of the message.
//============================================================================
struct PyIncompleteMessage : public PyMessage
{
    nb::bytes bytes;
    explicit PyIncompleteMessage(PyMessageDatabase::ConstPtr parent_db_, PyHeader header_, nb::bytes bytes_)
        : PyMessage("UNKNOWN", false, std::vector<FieldContainer>(), std::move(parent_db_), std::move(header_)), bytes(std::move(bytes_))
    {
    }
};

struct PyCompleteMessage : public PyMessage
{
    using PyMessage::PyMessage;

    PyMessageData to_ascii();
    PyMessageData to_binary();
    PyMessageData to_flattended_binary();
    PyMessageData to_json();
};

nb::object create_unknown_message_instance(nb::bytes data, PyHeader& header, PyMessageDatabase::ConstPtr database);

nb::object create_message_instance(PyHeader& header, std::vector<FieldContainer>& message_fields, MetaDataStruct& metadata,
                                   PyMessageDatabase::ConstPtr database);

} // namespace novatel::edie::oem
