// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file common.hpp
// ===============================================================================

#ifndef NOVATEL_EDIE_COMMON_COMMON_HPP
#define NOVATEL_EDIE_COMMON_COMMON_HPP

#include <cstdint>
#include <cstring>
#include <string>

namespace novatel::edie {

//-----------------------------------------------------------------------
//! \enum STATUS
//! \brief Enumeration for status codes returned from EDIE components.
//-----------------------------------------------------------------------
enum class STATUS
{
    SUCCESS,                //!< Successfully found a frame in the framer buffer.
    FAILURE,                //!< An unexpected failure occurred.
    UNKNOWN,                //!< Could not identify bytes as a protocol.
    INCOMPLETE,             //!< It is possible that a valid frame exists in the frame buffer, but more
                            //!< information is needed.
    INCOMPLETE_MORE_DATA,   //!< The current frame buffer is incomplete but more data is expected.
    NULL_PROVIDED,          //!< A null pointer was provided.
    NO_DATABASE,            //!< No database has been provided to the component.
    NO_DEFINITION,          //!< No definition could be found in the database for the provided message.
    NO_DEFINITION_EMBEDDED, //!< No definition could be found in the database for the embedded
                            //!< message in the RXCONFIG log.
    BUFFER_FULL,            //!< The provided destination buffer is not big enough to contain the frame.
    BUFFER_EMPTY,           //!< The internal circular buffer does not contain any unread bytes
    STREAM_EMPTY,           //!< The input stream is empty.
    UNSUPPORTED,            //!< An attempted operation is unsupported by this component.
    MALFORMED_INPUT,        //!< The input is recognizable, but has unexpected formatting.
    DECOMPRESSION_FAILURE   //!< The RANGECMP log could not be decompressed.
};

inline std::string StatusToString(const STATUS eStatus_)
{
    return eStatus_ == STATUS::SUCCESS                  ? "SUCCESS"
           : eStatus_ == STATUS::FAILURE                ? "FAILURE"
           : eStatus_ == STATUS::INCOMPLETE             ? "INCOMPLETE"
           : eStatus_ == STATUS::INCOMPLETE_MORE_DATA   ? "INCOMPLETE_MORE_DATA"
           : eStatus_ == STATUS::NULL_PROVIDED          ? "NULL_PROVIDED"
           : eStatus_ == STATUS::NO_DATABASE            ? "NO_DATABASE"
           : eStatus_ == STATUS::NO_DEFINITION          ? "NO_DEFINITION"
           : eStatus_ == STATUS::NO_DEFINITION_EMBEDDED ? "NO_DEFINITION_EMBEDDED"
           : eStatus_ == STATUS::BUFFER_FULL            ? "BUFFER_FULL"
           : eStatus_ == STATUS::BUFFER_EMPTY           ? "BUFFER_EMPTY"
           : eStatus_ == STATUS::STREAM_EMPTY           ? "STREAM_EMPTY"
           : eStatus_ == STATUS::UNSUPPORTED            ? "UNSUPPORTED"
           : eStatus_ == STATUS::MALFORMED_INPUT        ? "MALFORMED_INPUT"
           : eStatus_ == STATUS::DECOMPRESSION_FAILURE  ? "DECOMPRESSION_FAILURE"
                                                        : "UNKNOWN";
}

inline std::ostream& operator<<(std::ostream& os_, const STATUS eStatus_)
{
    os_ << StatusToString(eStatus_);
    return os_;
}

//-----------------------------------------------------------------------
//! \enum ENCODE_FORMAT
//! \brief Used to specify the kind of format to re-encode a decoded
//! message to.  Not every encoder supports these types.
//-----------------------------------------------------------------------
enum class ENCODE_FORMAT
{
    FLATTENED_BINARY, //!< NovAtel EDIE "Flattened" binary format.  All strings/arrays are padded to
                      //!< maximum length to allow programmatic access.
    ASCII,            //!< NovAtel ASCII.  If the log was decoded from a SHORT/compressed format, it will be
                      //!< encoded to the respective SHORT/compressed format.
    ABBREV_ASCII,     //!< NovAtel Abbreviated ASCII.
    BINARY,           //!< NovAtel Binary.  If the log was decoded from a SHORT/compressed format, it will be
                      //!< encoded to the respective SHORT/compressed format.
    JSON,             //!< A JSON object. See HTML documentation for information on fields.
    UNSPECIFIED       //!< No encode format was specified.
};

inline ENCODE_FORMAT StringToEncodeFormat(const std::string& sEnumName_)
{
    return sEnumName_ == "FLATTENED_BINARY" ? ENCODE_FORMAT::FLATTENED_BINARY
           : sEnumName_ == "ASCII"          ? ENCODE_FORMAT::ASCII
           : sEnumName_ == "BINARY"         ? ENCODE_FORMAT::BINARY
           : sEnumName_ == "JSON"           ? ENCODE_FORMAT::JSON
           : sEnumName_ == "ABBREV_ASCII"   ? ENCODE_FORMAT::ABBREV_ASCII
                                            : ENCODE_FORMAT::UNSPECIFIED;
}

inline std::string EncodeFormatToString(const ENCODE_FORMAT eFormat_)
{
    return eFormat_ == ENCODE_FORMAT::FLATTENED_BINARY ? "FLATTENED_BINARY"
           : eFormat_ == ENCODE_FORMAT::ASCII          ? "ASCII"
           : eFormat_ == ENCODE_FORMAT::ABBREV_ASCII   ? "ABBREV_ASCII"
           : eFormat_ == ENCODE_FORMAT::BINARY         ? "BINARY"
           : eFormat_ == ENCODE_FORMAT::JSON           ? "JSON"
                                                       : "UNSPECIFIED";
}

inline std::ostream& operator<<(std::ostream& os_, const ENCODE_FORMAT eFormat_)
{
    os_ << EncodeFormatToString(eFormat_);
    return os_;
}

//-----------------------------------------------------------------------
//! \enum TIME_STATUS
//! \brief Enumeration describing the time status on a NovAtel receiver
//! when a log is produced. See GPS Reference Time Status.
//-----------------------------------------------------------------------
enum class TIME_STATUS
{
    UNKNOWN = 20,             //!< Time validity is unknown.
    APPROXIMATE = 60,         //!< Time is set approximately.
    COARSEADJUSTING = 80,     //!< Time is approaching coarse precision.
    COARSE = 100,             //!< This time is valid to coarse precision.
    COARSESTEERING = 120,     //!< Time is coarse set and is being steered.
    FREEWHEELING = 130,       //!< Position is lost and the range bias cannot be calculated.
    FINEADJUSTING = 140,      //!< Time is adjusting to fine precision.
    FINE = 160,               //!< Time has fine precision.
    FINEBACKUPSTEERING = 170, //!< Time is fine set and is being steered by the backup system.
    FINESTEERING = 180,       //!< Time is fine set and is being steered.
    SATTIME = 200,            //!< Time from satellite. Only used in logs containing satellite data such as ephemeris and almanac.
    EXTERN = 220,             //!< Time source is external to the Receiver.
    EXACT = 240               //!< Time is exact.
};

//-----------------------------------------------------------------------
//! \enum HEADER_FORMAT
//! \brief Header formats of messages.
//-----------------------------------------------------------------------
enum class HEADER_FORMAT
{
    UNKNOWN = 1,
    BINARY,
    SHORT_BINARY,
    PROPRIETARY_BINARY,
    ASCII,
    SHORT_ASCII,
    ABB_ASCII,
    NMEA,
    JSON,
    SHORT_ABB_ASCII,
    ALL // Used in filters to indicate all filter types : all new enums should be added before this value
};

constexpr bool IsShortHeaderFormat(const HEADER_FORMAT eFormat_)
{
    return eFormat_ == HEADER_FORMAT::SHORT_ASCII || eFormat_ == HEADER_FORMAT::SHORT_BINARY || eFormat_ == HEADER_FORMAT::SHORT_ABB_ASCII;
}

//-----------------------------------------------------------------------
//! \enum MESSAGE_FORMAT
//! \brief Bit fields to describe message type. See Binary Message
//! Header Structure.
//-----------------------------------------------------------------------
enum class MESSAGE_FORMAT
{
    BINARY = 0b00,
    ASCII = 0b01,
    ABBREV = 0b10,
    RSRVD = 0b11
};

//-----------------------------------------------------------------------
//! \enum MESSAGE_TYPE_MASK
//! \brief Bitmask for Message Type field in Binary logs. See Binary
//! Message Header Structure.
//-----------------------------------------------------------------------
enum class MESSAGE_TYPE_MASK
{
    MEASSRC = 0b00011111,
    MSGFORMAT = 0b01100000,
    RESPONSE = 0b10000000
};

//-----------------------------------------------------------------------
//! \enum MESSAGE_ID_MASK
//! \brief Bitmask for Message ID field in Binary logs. See Binary
//! Message Header Structure.
//-----------------------------------------------------------------------
enum class MESSAGE_ID_MASK
{
    LOGID = 0x00FFFF,
    MEASSRC = static_cast<uint32_t>(MESSAGE_TYPE_MASK::MEASSRC) << 16,
    MSGFORMAT = static_cast<uint32_t>(MESSAGE_TYPE_MASK::MSGFORMAT) << 16,
    RESPONSE = static_cast<uint32_t>(MESSAGE_TYPE_MASK::RESPONSE) << 16
};

//-----------------------------------------------------------------------
//! \enum MEASUREMENT_SOURCE
//! \brief Enumeration for antenna source of measurements for a log.
//-----------------------------------------------------------------------
enum class MEASUREMENT_SOURCE
{
    PRIMARY,
    SECONDARY,
    MAX
};

//-----------------------------------------------------------------------
//! \struct MessageDataStruct
//! \brief Structure containing pointers and length information about a
//! message and its various parts
//-----------------------------------------------------------------------
struct MessageDataStruct
{
    unsigned char* pucMessageHeader{nullptr};
    uint32_t uiMessageHeaderLength{0};
    unsigned char* pucMessageBody{nullptr};
    uint32_t uiMessageBodyLength{0};
    unsigned char* pucMessage{nullptr};
    uint32_t uiMessageLength{0};

    constexpr MessageDataStruct() = default;

    MessageDataStruct(unsigned char* pucMessage_, const uint32_t uiLength_, const uint32_t uiHeaderLength_)
        : pucMessageHeader(pucMessage_), uiMessageHeaderLength(uiHeaderLength_), pucMessageBody(pucMessage_ + uiMessageHeaderLength),
          uiMessageBodyLength(uiLength_ - uiHeaderLength_), pucMessage(pucMessage_), uiMessageLength(uiLength_)
    {
    }

    bool operator==(const MessageDataStruct& stOther_) const
    {
        return uiMessageHeaderLength == stOther_.uiMessageHeaderLength && uiMessageBodyLength == stOther_.uiMessageBodyLength &&
               uiMessageLength == stOther_.uiMessageLength && memcmp(pucMessageHeader, stOther_.pucMessageHeader, uiMessageHeaderLength) == 0 &&
               memcmp(pucMessageBody, stOther_.pucMessageBody, uiMessageBodyLength) == 0 &&
               memcmp(pucMessage, stOther_.pucMessage, uiMessageLength) == 0;
    }

    bool operator!=(const MessageDataStruct& stOther_) const { return !(*this == stOther_); }
};

//! Forward declaration for common function headers.
struct EnumDefinition;

class MetaDataBase
{
  private:
    static constexpr uint32_t uiMessageNameMax = 40;

  public:
    MetaDataBase() = default;
    virtual ~MetaDataBase() = 0;
    bool bResponse{false};
    HEADER_FORMAT eFormat{HEADER_FORMAT::UNKNOWN};
    uint16_t usWeek{0};
    double dMilliseconds{0.0};
    uint32_t uiLength{0};
    uint32_t uiBinaryMsgLength{0}; //!< Message length according to the binary header. If ASCII, this field is not used.
    uint32_t uiHeaderLength{0};
    uint16_t usMessageId{0};
    uint32_t uiMessageCrc{0};
    char acMessageName[uiMessageNameMax + 1]{'\0'};

    [[nodiscard]] std::string MessageName() const { return {acMessageName}; }

    void MessageName(const std::string& strMessageName_)
    {
        memcpy(acMessageName, strMessageName_.c_str(), strMessageName_.length());
        acMessageName[strMessageName_.length()] = '\0';
    }
};

inline MetaDataBase::~MetaDataBase() = default;

} // namespace novatel::edie

//-----------------------------------------------------------------------
//! \brief Compare two double values.
//
//! \param[in] dVal1_ First double type Value.
//! \param[in] dVal2_ Second double type value.
//! \param[in] dEpsilon_ The tolerance with which to justify "equal".
//
//! \return Boolean Value - Returns both values are equal or not?
//-----------------------------------------------------------------------
bool IsEqual(double dVal1_, double dVal2_, double dEpsilon_ = 0.001);

//-----------------------------------------------------------------------
//! \brief Construct a full message ID from its parts.
//
//! \param[in] uiMessageId_ The base message ID.
//! \param[in] uiSiblingId_ The sibling ID.
//! \param[in] uiMsgFormat_ The message format.
//! \param[in] uiResponse_ If the message is a response.
//
//! \return The constructed message ID.
//! \remark See OEM7 User Documentation on "Binary".
//-----------------------------------------------------------------------
uint32_t CreateMsgId(uint32_t uiMessageId_, uint32_t uiSiblingId_, uint32_t uiMsgFormat_, uint32_t uiResponse_);

//-----------------------------------------------------------------------
//! \brief Unpack a full message ID into its parts.
//
//! \param[in] uiMessageId_ The full message ID to be unpacked.
//! \param[out] usMessageId_ The base message ID.
//! \param[out] uiSiblingId_ The sibling ID.
//! \param[out] uiMsgFormat_ The message format.
//! \param[out] uiResponse_ If the message is a response.
//
//! \remark See OEM7 User Documentation on "Binary".
//-----------------------------------------------------------------------
void UnpackMsgId(uint32_t uiMessageId_, uint16_t& usMessageId_, uint32_t& uiSiblingId_, uint32_t& uiMsgFormat_, uint32_t& uiResponse_);

//-----------------------------------------------------------------------
//! \brief Pack a message type from its parts into an unsigned char.
//
//! \param[in] uiSiblingId_ The sibling ID.
//! \param[in] uiMsgFormat_ The message format.
//! \param[in] uiResponse_ If the message is a response.
//
//! \return A packed message type.
//-----------------------------------------------------------------------
unsigned char PackMsgType(uint32_t uiSiblingId_, uint32_t uiMsgFormat_, uint32_t uiResponse_);

//-----------------------------------------------------------------------
//! \brief Get the name of an enum value in string form.
//
//! \param[in] stEnumDef_ A pointer to the enum definition.
//! \param[in] uiEnum_ The enum value.
//
//! \return The enum in string form.
//-----------------------------------------------------------------------
std::string GetEnumString(const novatel::edie::EnumDefinition* stEnumDef_, uint32_t uiEnum_);

//-----------------------------------------------------------------------
//! \brief Get the value of an enum string in integer form.
//
//! \param[in] stEnumDef_ A pointer to the enum definition.
//! \param[in] strEnum_ The enum name.
//
//! \return The enum in integer form.
//-----------------------------------------------------------------------
int32_t GetEnumValue(const novatel::edie::EnumDefinition* stEnumDef_, const std::string& strEnum_);

//-----------------------------------------------------------------------
//! \brief Get the value of a response string in integer form.
//
//! \param[in] stRespDef_ A pointer to the response definition.
//! \param[in] strResp_ The response name.
//
//! \return The response in integer form.
//-----------------------------------------------------------------------
int32_t GetResponseId(const novatel::edie::EnumDefinition* stRespDef_, const std::string& strResp_);

//-----------------------------------------------------------------------
//! \brief Get the char as an integer.
//
//! \param[in] c_ The char to get as an integer.
//
//! \return The char as an integer.
//-----------------------------------------------------------------------
int32_t ToDigit(char c_);

//-----------------------------------------------------------------------
//! \brief Strip abbreviated ASCII formatting from the front of the
//! log buffer.
//
//! \param[in] ullTokenLength_ The length of the message contained in
//! ppcMessageBuffer_
//! \param[in] ppcMessageBuffer_ A pointer to the buffer containing the
//! message to have abbreviated ASCII formatting stripped.
//
//! \return A boolean describing if the formatting found was abbreviated
//! ASCII or not. Regardless of this return, the function will attempt
//! to strip any abbreviated ASCII bytes.
//-----------------------------------------------------------------------
bool ConsumeAbbrevFormatting(uint64_t ullTokenLength_, char** ppcMessageBuffer_);

//-----------------------------------------------------------------------
//! \struct SatelliteId
//! \brief Provides Satellite number and frequency channel.
//-----------------------------------------------------------------------
struct SatelliteId
{
    uint16_t usPrnOrSlot{0};      //!< PRN/Slot number of satellite.
    int16_t sFrequencyChannel{0}; //!< Frequency channel number.

    constexpr SatelliteId() = default;

    bool operator==(const SatelliteId& stOther_) const
    {
        return usPrnOrSlot == stOther_.usPrnOrSlot && sFrequencyChannel == stOther_.sFrequencyChannel;
    }
};

//-----------------------------------------------------------------------
// Common miscellaneous defines
//-----------------------------------------------------------------------
constexpr uint32_t SEC_TO_MILLI_SEC = 1000; //!< A Macro definition for number of milliseconds in a second.
constexpr uint32_t SECS_IN_WEEK = 604800;   //!< A Macro definition for number of milliseconds in a week.

//-----------------------------------------------------------------------
// NovAtel message length defines
//-----------------------------------------------------------------------
//!< FW-defined maximum transmittable message length. (32kB)
constexpr uint32_t MESSAGE_SIZE_MAX = 0x8000;
//!< Undefined message length assumes that the max log size for this format is the maximum message length allowed to be transmitted by the FW.
constexpr uint32_t MAX_ASCII_MESSAGE_LENGTH = MESSAGE_SIZE_MAX;
//!< Undefined message length assumes that the max log size for this format is the maximum message length allowed to be transmitted by the FW.
constexpr uint32_t MAX_BINARY_MESSAGE_LENGTH = MESSAGE_SIZE_MAX;
//!< Undefined message length assumes that the max log size for this format is the maximum message length allowed to be transmitted by the FW.
constexpr uint32_t MAX_SHORT_ASCII_MESSAGE_LENGTH = MESSAGE_SIZE_MAX;
//!< Short Binary message length cannot exceed the log length max value representation defined by the header.
constexpr uint32_t MAX_SHORT_BINARY_MESSAGE_LENGTH = 12 + 255 + 4;
//!< Undefined message length assumes that the max log size for this format is the maximum message length allowed to be transmitted by the FW.
constexpr uint32_t MAX_ABB_ASCII_RESPONSE_LENGTH = MESSAGE_SIZE_MAX;
//!< NovAtel Docs - NMEA Standard Logs: Explicitly states that the maximum allowable is 82 chars.
//!< Numerous internal logs break that standard, so we will use 256here as a safety measure.
constexpr uint32_t MAX_NMEA_MESSAGE_LENGTH = 256; //(82)

#endif
