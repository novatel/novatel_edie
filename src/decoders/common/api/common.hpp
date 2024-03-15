////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file common.hpp
//! \brief Header file containing the common structs, enums and defines
//! used throughout the EDIE source code.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef COMMON_HPP
#define COMMON_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
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
    DECOMPRESSION_FAILURE   //!< The RANGECMPx log could not be decompressed.
};

inline std::string StatusToString(STATUS eStatus_)
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

inline std::ostream& operator<<(std::ostream& os_, STATUS eStatus_)
{
    os_ << StatusToString(eStatus_);
    return os_;
}

//-----------------------------------------------------------------------
//! \enum ENCODEFORMAT
//! \brief Used to specify the kind of format to re-encode a decoded
//! message to.  Not every encoder supports these types.
//-----------------------------------------------------------------------
enum class ENCODEFORMAT
{
    FLATTENED_BINARY, //!< NovAtel EDIE "Flattened" binary format.  All strings/arrays are padded to
                      //!< maximum length to allow programmatic access.
    ASCII,            //!< NovAtel ASCII.  If the log was decoded from a SHORT/compressed format, it will be
                      //!< encoded to the respective SHORT/compressed format.
    ABBREV_ASCII,     //!< NovAtel Abbreviated ASCII.
    BINARY,           //!< NovAtel Binary.  If the log was decoded from a SHORT/compressed format, it will be
                      //!< encoded to the respective SHORT/compressed format.
    JSON,             //!< A JSON object.  See HTML documentation for information on fields.
    UNSPECIFIED       //!< No encode format was specified.
};

inline ENCODEFORMAT StringToEncodeFormat(std::string sEnumName_)
{
    return sEnumName_ == "FLATTENED_BINARY" ? ENCODEFORMAT::FLATTENED_BINARY
           : sEnumName_ == "ASCII"          ? ENCODEFORMAT::ASCII
           : sEnumName_ == "BINARY"         ? ENCODEFORMAT::BINARY
           : sEnumName_ == "JSON"           ? ENCODEFORMAT::JSON
           : sEnumName_ == "ABBREV_ASCII"   ? ENCODEFORMAT::ABBREV_ASCII
                                            : ENCODEFORMAT::UNSPECIFIED;
}

inline std::string EncodeFormatToString(ENCODEFORMAT eFormat_)
{
    return eFormat_ == ENCODEFORMAT::FLATTENED_BINARY ? "FLATTENED_BINARY"
           : eFormat_ == ENCODEFORMAT::ASCII          ? "ASCII"
           : eFormat_ == ENCODEFORMAT::ABBREV_ASCII   ? "ABBREV_ASCII"
           : eFormat_ == ENCODEFORMAT::BINARY         ? "BINARY"
           : eFormat_ == ENCODEFORMAT::JSON           ? "JSON"
                                                      : "UNSPECIFIED";
}

inline std::ostream& operator<<(std::ostream& os_, ENCODEFORMAT eFormat_)
{
    os_ << EncodeFormatToString(eFormat_);
    return os_;
}

//-----------------------------------------------------------------------
//! \enum TIME_STATUS
//! \brief Enumeration describing the time status on a NovAtel receiver
//! when a log is produced.  See GPS Reference Time Status.
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
    SATTIME = 200,            //!< Time from satellite. Only used in logs containing satellite data such as
                              //!< ephemeris and almanac.
    EXTERN = 220,             //!< Time source is external to the Receiver.
    EXACT = 240               //!< Time is exact.
};

//-----------------------------------------------------------------------
//! \enum HEADERFORMAT
//! \brief Header formats of messages.
//-----------------------------------------------------------------------
enum class HEADERFORMAT
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
    ALL // Used in filters to indicate all filter types : all new enums should be added before this
        // value
};

//-----------------------------------------------------------------------
//! \enum MESSAGEFORMAT
//! \brief Bit fields to describe message type.  See Binary Message
//! Header Structure.
//-----------------------------------------------------------------------
enum class MESSAGEFORMAT
{
    BINARY = 0b00,
    ASCII = 0b01,
    ABBREV = 0b10,
    RSRVD = 0b11
};

//-----------------------------------------------------------------------
//! \enum MESSAGETYPEMASK
//! \brief Bitmask for Message Type field in Binary logs.  See Binary
//! Message Header Structure.
//-----------------------------------------------------------------------
enum class MESSAGETYPEMASK
{
    MEASSRC = 0b00011111,
    MSGFORMAT = 0b01100000,
    RESPONSE = 0b10000000
};

//-----------------------------------------------------------------------
//! \enum MESSAGEIDMASK
//! \brief Bitmask for Message ID field in Binary logs.  See Binary
//! Message Header Structure.
//-----------------------------------------------------------------------
enum class MESSAGEIDMASK
{
    LOGID = 0x00FFFF,
    MEASSRC = (static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC) << 16),
    MSGFORMAT = (static_cast<uint32_t>(MESSAGETYPEMASK::MSGFORMAT) << 16),
    RESPONSE = (static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE) << 16)
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

    MessageDataStruct(unsigned char* message, const uint32_t length, const uint32_t headerLength)
        : pucMessageHeader(message), uiMessageHeaderLength(headerLength), pucMessageBody(message + uiMessageHeaderLength),
          uiMessageBodyLength(length - headerLength), pucMessage(message), uiMessageLength(length)
    {
    }

    bool operator==(const MessageDataStruct& other) const
    {
        return uiMessageHeaderLength == other.uiMessageHeaderLength && uiMessageBodyLength == other.uiMessageBodyLength &&
               uiMessageLength == other.uiMessageLength && memcmp(pucMessageHeader, other.pucMessageHeader, uiMessageHeaderLength) == 0 &&
               memcmp(pucMessageBody, other.pucMessageBody, uiMessageBodyLength) == 0 && memcmp(pucMessage, other.pucMessage, uiMessageLength) == 0;
    }

    bool operator!=(const MessageDataStruct& other) const { return !(*this == other); }
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
    HEADERFORMAT eFormat{HEADERFORMAT::UNKNOWN};
    uint16_t usWeek{0};
    double dMilliseconds{0.0};
    uint32_t uiLength{0};
    uint32_t uiBinaryMsgLength{0}; //!< Message length according to the binary header. If ASCII, this field is not used.
    uint32_t uiHeaderLength{0};
    uint16_t usMessageID{0};
    uint32_t uiMessageCRC{0};
    char acMessageName[uiMessageNameMax + 1]{'\0'};

    // Message Name helper functions
    std::string MessageName() const { return std::string(acMessageName); }

    void MessageName(std::string strMessageName_)
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
//! \param[in] dVal1 First double type Value.
//! \param[in] dVal2 Second double type value.
//! \param[in] dEpsilon The tolerance with which to justify "equal".
//
//! \return Boolean Value - Returns both values are equal or not?
//-----------------------------------------------------------------------
bool IsEqual(double dVal1_, double dVal2_, double dEpsilon_ = 0.001);

//-----------------------------------------------------------------------
//! \brief Construct a full message ID from its parts.
//
//! \param[in] uiMessageID_ The base message ID.
//! \param[in] uiSiblingID_ The sibling ID.
//! \param[in] uiMsgFormat_ The message format.
//! \param[in] uiResponse_ If the message is a response.
//
//! \return The constructed message ID.
//! \remark See OEM7 User Documentation on "Binary".
//-----------------------------------------------------------------------
uint32_t CreateMsgID(uint32_t uiMessageID_, uint32_t uiSiblingID_, uint32_t uiMsgFormat_, uint32_t uiResponse_);

//-----------------------------------------------------------------------
//! \brief Unpack a full message ID into its parts.
//
//! \param[in] uiMessageID_ The full message ID to be unpacked.
//! \param[out] usMessageID_ The base message ID.
//! \param[out] uiSiblingID_ The sibling ID.
//! \param[out] uiMsgFormat_ The message format.
//! \param[out] uiResponse_ If the message is a response.
//
//! \remark See OEM7 User Documentation on "Binary".
//-----------------------------------------------------------------------
void UnpackMsgID(uint32_t uiMessageID_, uint16_t& usMessageID_, uint32_t& uiSiblingID_, uint32_t& uiMsgFormat_, uint32_t& uiResponse_);

//-----------------------------------------------------------------------
//! \brief Pack a message type from its parts into an unsigned char.
//
//! \param[in] uiSiblingID_ The sibling ID.
//! \param[in] uiMsgFormat_ The message format.
//! \param[in] uiResponse_ If the message is a response.
//
//! \return A packed message type.
//-----------------------------------------------------------------------
unsigned char PackMsgType(uint32_t uiSiblingID_, uint32_t uiMsgFormat_, uint32_t uiResponse_);

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
int32_t GetEnumValue(const novatel::edie::EnumDefinition* stEnumDef_, std::string strEnum_);

//-----------------------------------------------------------------------
//! \brief Get the value of an response string in integer form.
//
//! \param[in] stRespDef_ A pointer to the response definition.
//! \param[in] strResp_ The response name.
//
//! \return The response in integer form.
//-----------------------------------------------------------------------
int32_t GetResponseId(const novatel::edie::EnumDefinition* stRespDef_, std::string strResp_);

//-----------------------------------------------------------------------
//! \brief Get the char as an integer.
//
//! \param[in] c The char to get as an integer.
//
//! \return The char as an integer.
//-----------------------------------------------------------------------
int32_t ToDigit(char c);

//-----------------------------------------------------------------------
//! \brief Strip a abbreviated ASCII formatting from the front of the
//! log buffer.
//
//! \param[in] ullTokenLength_ The length of the message contained in
//! ppucMessageBuffer_
//! \param[in] ppucMessageBuffer_ A pointer to the buffer containing the
//! message to have abbreviated ASCII formatting stripped.
//
//! \return A boolean describing if the formatting found was abbreviated
//! ASCII or not.  Regardless of this return, the function will attempt
//! to strip any abbreviated ASCII bytes.
//-----------------------------------------------------------------------
bool ConsumeAbbrevFormatting(uint64_t ullTokenLength_, char** ppucMessageBuffer_);

//-----------------------------------------------------------------------
//! \struct SATELLITEID
//! \brief Provides Satellite number and frequency channel.
//-----------------------------------------------------------------------
struct SATELLITEID
{
    uint16_t usPrnOrSlot{0};      //!< PRN/Slot number of satellite.
    int16_t sFrequencyChannel{0}; //!< Frequency channel number.

    constexpr SATELLITEID() = default;

    bool operator==(const SATELLITEID& other) const { return usPrnOrSlot == other.usPrnOrSlot && sFrequencyChannel == other.sFrequencyChannel; }
};

//-----------------------------------------------------------------------
// Common miscellaneous defines
//-----------------------------------------------------------------------
#define SEC_TO_MSEC (1000U)    //!< A Macro definition for number of milliseconds in a second.
#define SECS_IN_WEEK (604800U) //!< A Macro definition for number of milliseconds in a week.

//-----------------------------------------------------------------------
// NovAtel message length defines
//-----------------------------------------------------------------------
#define MESSAGE_SIZE_MAX (0x8000) //!< FW-defined maximum transmittable message length. (32kB)
#define MAX_ASCII_MESSAGE_LENGTH                                                                                                                     \
    (MESSAGE_SIZE_MAX) //!< Undefined message length assumes that the max log size for this format
                       //!< is the maximum message length allowed to be transmitted by the FW.
#define MAX_BINARY_MESSAGE_LENGTH                                                                                                                    \
    (MESSAGE_SIZE_MAX) //!< Undefined message length assumes that the max log size for this format
                       //!< is the maximum message length allowed to be transmitted by the FW.
#define MAX_SHORT_ASCII_MESSAGE_LENGTH                                                                                                               \
    (MESSAGE_SIZE_MAX) //!< Undefined message length assumes that the max log size for this format
                       //!< is the maximum message length allowed to be transmitted by the FW.
#define MAX_SHORT_BINARY_MESSAGE_LENGTH                                                                                                              \
    (12 + 255 + 4) //!< Short Binary message length cannot exceed the log length max value
                   //!< representation defined by the header.
#define MAX_ABB_ASCII_RESPONSE_LENGTH                                                                                                                \
    (MESSAGE_SIZE_MAX) //!< Undefined message length assumes that the max log size for this format
                       //!< is the maximum message length allowed to be transmitted by the FW.
#define MAX_NMEA_MESSAGE_LENGTH                                                                                                                      \
    (256) //(82)         //!< NovAtel Docs - NMEA Standard Logs: Explicitly states that the maximum
          // allowable is 82 chars.  Numerous internal logs break that standard, so we will use 256
          // here as a safety measure.

#endif
