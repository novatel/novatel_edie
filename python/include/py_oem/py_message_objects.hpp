#pragma once

#include <memory>

#include "novatel_edie/decoders/oem/filter.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/field_objects.hpp"
#include "py_common/message_database.hpp"
#include "py_common/py_logger.hpp"
#include "py_common/py_message_data.hpp"
#include "py_oem/message_db_singleton.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_oem {

//============================================================================
//! \class PyGpsTime
//! \brief A GPS time.
//============================================================================
struct PyGpsTime
{
    PyGpsTime() = default;
    PyGpsTime(uint16_t week_, double milliseconds_) : week(week_), milliseconds(milliseconds_) {}
    PyGpsTime(uint16_t week_, double milliseconds_, TIME_STATUS time_status_) : week(week_), milliseconds(milliseconds_), time_status(time_status_) {}

    explicit PyGpsTime(const oem::MetaDataStruct& meta_) : week(meta_.usWeek), milliseconds(meta_.dMilliseconds), time_status(meta_.eTimeStatus) {}

    uint16_t week{0};
    double milliseconds{0.0};
    TIME_STATUS time_status{TIME_STATUS::UNKNOWN};
};

//============================================================================
//! \class PyRecieverStatus
//! \brief The field in a message header which gives type information.
//============================================================================
struct PyRecieverStatus
{
    uint32_t value;

    PyRecieverStatus(uint32_t value_) : value(value_) {}
};

//============================================================================
//! \class PyMessageTypeField
//! \brief The field in a message header which gives type information.
//============================================================================
struct PyMessageTypeField
{
    uint8_t value;

    PyMessageTypeField(uint8_t value_) : value(value_) {}

    bool IsResponse() { return (value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::RESPONSE)) != 0; }
    MESSAGE_FORMAT GetFormat() { return static_cast<MESSAGE_FORMAT>((value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MSGFORMAT)) >> 5); }
    uint8_t GetSiblingId() { return value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MEASSRC); }
    MEASUREMENT_SOURCE GetMeasurementSource()
    {
        static bool bWarningSent = false;
        if (!bWarningSent)
        {
            GetBaseLoggerManager()
                ->RegisterLogger("deprecation_warning")
                ->warn("The 'source' field is deprecated and will be removed in a future release. Use 'sibling_id' instead and convert the integer "
                       "value to a MEASUREMENT_SOURCE if needed.");
            bWarningSent = true;
        }

        return static_cast<MEASUREMENT_SOURCE>((value & static_cast<uint8_t>(MESSAGE_TYPE_MASK::MEASSRC)));
    }
};

//============================================================================
//! \class PyHeader
//! \brief A python representation for a message header.
//============================================================================
struct PyHeader : public oem::IntermediateHeader
{
    HEADER_FORMAT format;
    // uint32_t raw_length;

    PyMessageTypeField GetPyMessageType() { return PyMessageTypeField(ucMessageType); }
    PyRecieverStatus GetRecieverStatus() { return PyRecieverStatus(uiReceiverStatus); }

    nb::dict to_dict() const;
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
struct PyEncodableField : public py_common::PyField
{
  protected:
    py_common::PyMessageData PyEncode(ENCODE_FORMAT eFormat);


  public:
    PyHeader header;
    PyMessageDatabase::ConstPtr parent_db_;


    explicit PyEncodableField(std::string name_, bool has_ptype_, std::vector<FieldContainer>&& fields_,
                              PyMessageDatabase::ConstPtr parent_db_, PyHeader header_)
        : PyField(std::move(name_), has_ptype_, std::move(fields_), std::move(parent_db_->GetCoreDatabase())), header(std::move(header_)), parent_db_(std::move(parent_db_)) {};

    py_common::PyMessageData encode(ENCODE_FORMAT fmt);
    py_common::PyMessageData to_ascii();
    py_common::PyMessageData to_abbrev_ascii();
    py_common::PyMessageData to_binary();
    py_common::PyMessageData to_flattened_binary();
    py_common::PyMessageData to_json();

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

nb::object create_unknown_message_instance(nb::bytes data, PyHeader& header, PyMessageDatabase::ConstPtr database);

nb::object create_message_instance(PyHeader& header, std::vector<FieldContainer>&& message_fields, oem::MetaDataStruct& metadata,
                                   PyMessageDatabase::ConstPtr database);

//============================================================================
//! \class PyResponse
//! \brief A python representation for a single decoded message response.
//!
//! This class makes the assumption that `fields` is a vector of field with the
//! definitions generated in `void MessageDecoderBase::CreateResponseMsgDefinitions()`
//============================================================================
struct PyResponse : public PyEncodableField
{
    bool complete;
    explicit PyResponse(std::string name_, std::vector<FieldContainer> fields_, PyMessageDatabase::ConstPtr parent_db_, PyHeader header_,
                        bool complete_)
        : PyEncodableField(std::move(name_), false, std::move(fields_), std::move(parent_db_), std::move(header_)), complete(complete_) {};

    // Retrieve response ID from first field of response
    int32_t GetResponseId() { return std::get<int>(fields[0].fieldValue); }

    // Retrieve response string from second field of response
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

} // namespace novatel::edie::py_oem
