#pragma once

#include <memory>

#include "novatel_edie/decoders/oem/filter.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/field_objects.hpp"
#include "py_common/message_database.hpp"
#include "py_common/py_logger.hpp"
#include "py_common/py_message_data.hpp"
#include "py_oem/message_database.hpp"

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
//! \enum RECEIVER_STATUS_VERSION
//! \brief The 2-bit version field embedded in the receiver-status word.
//============================================================================
enum class RECEIVER_STATUS_VERSION : uint8_t
{
    USE_OEM6_ERROR_BITS = 0,
    USE_OEM7_ERROR_BITS = 1,
    RESERVED_1 = 2,
    RESERVED_2 = 3
};

//============================================================================
//! \class PyRecieverStatus
//! \brief The field in a message header which gives type information.
//============================================================================
struct PyRecieverStatus
{
    uint32_t value;

    PyRecieverStatus(uint32_t value_ = 0) : value(value_) {}

    bool GetBit(uint32_t mask) const { return (value & mask) != 0; }
    void SetBit(uint32_t mask, bool set)
    {
        if (set) { value |= mask; }
        else { value &= ~mask; }
    }

    bool GetRecieverError() const { return GetBit(0x00000001); }
    void SetRecieverError(bool v) { SetBit(0x00000001, v); }

    bool GetTemperatureWarning() const { return GetBit(0x00000002); }
    void SetTemperatureWarning(bool v) { SetBit(0x00000002, v); }

    bool GetVoltageWarning() const { return GetBit(0x00000004); }
    void SetVoltageWarning(bool v) { SetBit(0x00000004, v); }

    bool GetAntennaPowered() const { return GetBit(0x00000008); }
    void SetAntennaPowered(bool v) { SetBit(0x00000008, v); }

    bool GetLnaFailure() const { return GetBit(0x00000010); }
    void SetLnaFailure(bool v) { SetBit(0x00000010, v); }

    bool GetAntennaOpenCircuit() const { return GetBit(0x00000020); }
    void SetAntennaOpenCircuit(bool v) { SetBit(0x00000020, v); }

    bool GetAntennaShortCircuit() const { return GetBit(0x00000040); }
    void SetAntennaShortCircuit(bool v) { SetBit(0x00000040, v); }

    bool GetCpuOverload() const { return GetBit(0x00000080); }
    void SetCpuOverload(bool v) { SetBit(0x00000080, v); }

    bool GetComBufferOverrun() const { return GetBit(0x00000100); }
    void SetComBufferOverrun(bool v) { SetBit(0x00000100, v); }

    bool GetSpoofingDetected() const { return GetBit(0x00000200); }
    void SetSpoofingDetected(bool v) { SetBit(0x00000200, v); }

    bool GetReserved() const { return GetBit(0x00000400); }
    void SetReserved(bool v) { SetBit(0x00000400, v); }

    bool GetLinkOverrun() const { return GetBit(0x00000800); }
    void SetLinkOverrun(bool v) { SetBit(0x00000800, v); }

    bool GetInputOverrun() const { return GetBit(0x00001000); }
    void SetInputOverrun(bool v) { SetBit(0x00001000, v); }

    bool GetAuxTransmitOverrun() const { return GetBit(0x00002000); }
    void SetAuxTransmitOverrun(bool v) { SetBit(0x00002000, v); }

    bool GetAntennaGainOutOfRange() const { return GetBit(0x00004000); }
    void SetAntennaGainOutOfRange(bool v) { SetBit(0x00004000, v); }

    bool GetJammerDetected() const { return GetBit(0x00008000); }
    void SetJammerDetected(bool v) { SetBit(0x00008000, v); }

    bool GetInsReset() const { return GetBit(0x00010000); }
    void SetInsReset(bool v) { SetBit(0x00010000, v); }

    bool GetImuCommunicationFailure() const { return GetBit(0x00020000); }
    void SetImuCommunicationFailure(bool v) { SetBit(0x00020000, v); }

    bool GetGpsAlmanacInvalid() const { return GetBit(0x00040000); }
    void SetGpsAlmanacInvalid(bool v) { SetBit(0x00040000, v); }

    bool GetPositionSolutionInvalid() const { return GetBit(0x00080000); }
    void SetPositionSolutionInvalid(bool v) { SetBit(0x00080000, v); }

    bool GetPositionFixed() const { return GetBit(0x00100000); }
    void SetPositionFixed(bool v) { SetBit(0x00100000, v); }

    bool GetClockSteeringDisabled() const { return GetBit(0x00200000); }
    void SetClockSteeringDisabled(bool v) { SetBit(0x00200000, v); }

    bool GetClockModelInvalid() const { return GetBit(0x00400000); }
    void SetClockModelInvalid(bool v) { SetBit(0x00400000, v); }

    bool GetExternalOscillatorLocked() const { return GetBit(0x00800000); }
    void SetExternalOscillatorLocked(bool v) { SetBit(0x00800000, v); }

    bool GetSoftwareResourceWarning() const { return GetBit(0x01000000); }
    void SetSoftwareResourceWarning(bool v) { SetBit(0x01000000, v); }

    bool GetTrackingModeHdr() const { return GetBit(0x08000000); }
    void SetTrackingModeHdr(bool v) { SetBit(0x08000000, v); }

    bool GetDigitalFilteringEnabled() const { return GetBit(0x10000000); }
    void SetDigitalFilteringEnabled(bool v) { SetBit(0x10000000, v); }

    bool GetAuxiliary3Event() const { return GetBit(0x20000000); }
    void SetAuxiliary3Event(bool v) { SetBit(0x20000000, v); }

    bool GetAuxiliary2Event() const { return GetBit(0x40000000); }
    void SetAuxiliary2Event(bool v) { SetBit(0x40000000, v); }

    bool GetAuxiliary1Event() const { return GetBit(0x80000000); }
    void SetAuxiliary1Event(bool v) { SetBit(0x80000000, v); }

    // 2-bit version field at bits 25-26 (mask 0x06000000).
    RECEIVER_STATUS_VERSION GetVersionBits() const { return static_cast<RECEIVER_STATUS_VERSION>((value & 0x06000000) >> 25); }
    void SetVersionBits(RECEIVER_STATUS_VERSION v) { value = (value & ~0x06000000u) | ((static_cast<uint32_t>(v) & 0x3) << 25); }
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
    HEADER_FORMAT format{HEADER_FORMAT::UNKNOWN};

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
    explicit PyUnknownMessage(PyHeader header_, nb::bytes bytes_) : header(std::move(header_)), payload(std::move(bytes_)) {}
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
    std::string name;

    // Return the message name from the stored message definition.
    // Falls back to "UNKNOWN" if no message definition is availiable
    std::string_view get_name() const { return name; }

    explicit PyEncodableField(std::vector<FieldContainer> fields_, py_common::PyMessageDatabase::ConstPtr database_, PyHeader header_,
                              const MessageDefinition* messageDef_, uint32_t messageCrc_)
        : PyField(std::move(fields_), messageDef_, messageCrc_, std::move(database_)), header(std::move(header_)), messageDef(messageDef_),
          messageCrc(messageCrc_) {};

    // Default-construct an encodable field from a (db, def, crc) tuple.
    // Used by Message.__new__ — populates the field vector with type-appropriate
    // defaults via PyField::BuildDefaultFields and default-constructs the header.
    explicit PyEncodableField(py_common::PyMessageDatabase::ConstPtr database_, const MessageDefinition* messageDef_, uint32_t messageCrc_)
        : PyField(messageDef_, messageCrc_, std::move(database_)), messageDef(messageDef_), messageCrc(messageCrc_) {};

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
    explicit PyMessage(std::vector<FieldContainer> fields_, py_common::PyMessageDatabase::ConstPtr parent_db_, PyHeader header_,
                       const MessageDefinition* messageDef_, uint32_t messageCrc_)
        : PyEncodableField(std::move(fields_), std::move(parent_db_), std::move(header_), messageDef_, messageCrc_) {};

    // Default-construct a message — used by Message(...) __new__ binding.
    explicit PyMessage(py_common::PyMessageDatabase::ConstPtr parent_db_, const MessageDefinition* messageDef_, uint32_t messageCrc_)
        : PyEncodableField(std::move(parent_db_), messageDef_, messageCrc_) {};
};

nb::object create_unknown_message_instance(nb::bytes data, PyHeader& header);

nb::object create_message_instance(PyHeader& header, MessageBody&& message_fields, oem::MetaDataStruct& metadata,
                                   py_common::PyMessageDatabase::ConstPtr database);

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
    explicit PyResponse(MessageBody fields_, py_common::PyMessageDatabase::ConstPtr parent_db_, PyHeader header_, bool complete_,
                        std::string name_ = "UNKNOWN")
        : PyEncodableField(std::move(fields_), std::move(parent_db_), std::move(header_), std::move(name_)), complete(complete_){};

    // Retrieve response ID from first field of response
    int32_t GetResponseId() { return std::get<int>(fieldsPtr[0].fieldValue); }

    // Retrieve response string from second field of response
    std::string GetResponseString() const { return std::get<std::string>(fieldsPtr[1].fieldValue); }

    nb::object GetEnumValue()
    {
        nb::object response_enum = parentDb->GetEnumTypeByName("Responses");
        if (response_enum.is_none()) { return nb::none(); }
        uint32_t value = GetResponseId();
        nb::object enum_val = response_enum(value);
        return enum_val;
    }
};

} // namespace novatel::edie::py_oem
