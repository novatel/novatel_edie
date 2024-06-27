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

#ifndef NOVATEL_EDIE_DECODERS_COMMON_HPP
#define NOVATEL_EDIE_DECODERS_COMMON_HPP

#include <cstdint>

#include "novatel_edie/common/common.hpp"

namespace novatel::edie::oem {

//-----------------------------------------------------------------------
// NMEA message protocol constants
//-----------------------------------------------------------------------
constexpr char NMEA_SYNC = '$';
constexpr uint16_t NMEA_SYNC_LENGTH = 1;
constexpr uint16_t NMEA_CRC_LENGTH = 2;

//-----------------------------------------------------------------------
// OEM4 ASCII message protocol constants
//-----------------------------------------------------------------------
constexpr char OEM4_ASCII_SYNC = '#';
constexpr char OEM4_ASCII_FIELD_SEPARATOR = ',';
constexpr char OEM4_ASCII_HEADER_TERMINATOR = ';';
constexpr uint16_t OEM4_ASCII_SYNC_LENGTH = 1;
constexpr char OEM4_ASCII_CRC_DELIMITER = '*';
constexpr uint16_t OEM4_ASCII_CRC_LENGTH = 8;
constexpr char OEM4_SHORT_ASCII_SYNC = '%';
constexpr uint32_t OEM4_ASCII_MESSAGE_NAME_MAX = 40;
constexpr uint16_t OEM4_SHORT_ASCII_SYNC_LENGTH = 1;
constexpr char OEM4_ABBREV_ASCII_SYNC = '<';
constexpr char OEM4_ABBREV_ASCII_SEPARATOR = ' ';
constexpr uint32_t OEM4_ABBREV_ASCII_INDENTATION_LENGTH = 5; //! Number of spaces per Abbrev ASCII indentation level.
constexpr uint32_t OEM4_ERROR_PREFIX_LENGTH = 6;

//-----------------------------------------------------------------------
// OEM4 BINARY message protocol constants
//-----------------------------------------------------------------------
constexpr uint8_t OEM4_BINARY_SYNC1 = 0xAA;
constexpr uint8_t OEM4_BINARY_SYNC2 = 0x44;
constexpr uint8_t OEM4_BINARY_SYNC3 = 0x12;
constexpr uint16_t OEM4_BINARY_SYNC_LENGTH = 3;
constexpr uint16_t OEM4_BINARY_HEADER_LENGTH = 28;
constexpr uint16_t OEM4_BINARY_CRC_LENGTH = 4;
constexpr uint8_t OEM4_SHORT_BINARY_SYNC3 = 0x13;
constexpr uint16_t OEM4_SHORT_BINARY_SYNC_LENGTH = 3;
constexpr uint16_t OEM4_SHORT_BINARY_HEADER_LENGTH = 12;
constexpr uint8_t OEM4_PROPRIETARY_BINARY_SYNC2 = 0x45;

//-----------------------------------------------------------------------
//! \enum NovAtelFrameState
//! \brief Enumeration for state machine used while framing the log.
//-----------------------------------------------------------------------
enum class NovAtelFrameState
{
    WAITING_FOR_SYNC,                  //!< First sync byte of Header.
    WAITING_FOR_BINARY_SYNC2,          //!< Second sync byte of Header.
    WAITING_FOR_BINARY_SYNC3,          //!< Third sync byte of Header.
    WAITING_FOR_ABB_ASCII_SYNC2,       //!< Second 'sync byte' of header.
    WAITING_FOR_BINARY_HEADER,         //!< Read complete Binary header.
    WAITING_FOR_SHORT_BINARY_HEADER,   //!< Read complete short Binary header.
    WAITING_FOR_ABB_ASCII_HEADER,      //!< Read complete Abbreviated ASCII header.
    WAITING_FOR_BINARY_BODY_AND_CRC,   //!< Read complete Binary log including CRC.
    WAITING_FOR_ASCII_HEADER_AND_BODY, //!< Read complete ASCII log.
    WAITING_FOR_ASCII_CRC,             //!< Read complete ASCII CRC.
    WAITING_FOR_NMEA_BODY,             //!< Read complete NMEA log.
    WAITING_FOR_NMEA_CRC,              //!< Read complete NMEA CRC.
    WAITING_FOR_ABB_ASCII_BODY,        //!< Read complete Abbreviated ASCII log.
    WAITING_FOR_JSON_OBJECT,           //!< Read complete JSON object.
    COMPLETE_MESSAGE                   //!< Completed decoding of one log.
};

//-----------------------------------------------------------------------
//! \enum ASCII_HEADER
//! \brief Ascii Message header format sequence.
//-----------------------------------------------------------------------
enum class ASCII_HEADER
{
    MESSAGE_NAME,        //!< Ascii log Name.
    PORT,                //!< Receiver logging port.
    SEQUENCE,            //!< Embedded log sequence number.
    IDLE_TIME,           //!< Receiver Idle time.
    TIME_STATUS,         //!< GPS reference time status.
    WEEK,                //!< GPS Week number.
    SECONDS,             //!< GPS week seconds.
    RECEIVER_STATUS,     //!< Receiver status.
    MSG_DEF_CRC,         //!< Reserved Field.
    RECEIVER_SW_VERSION, //!< Receiver software version.
};

//-----------------------------------------------------------------------
//! \struct MetaDataStruct
//! \brief Structure containing information about a framed and decoded
//! message. This information will be updated by Framers and
//! HeaderDecoders and used by MessageDecoders and Encoders.
//-----------------------------------------------------------------------
struct MetaDataStruct : public MetaDataBase
{
    MEASUREMENT_SOURCE eMeasurementSource{MEASUREMENT_SOURCE::PRIMARY};
    TIME_STATUS eTimeStatus{TIME_STATUS::UNKNOWN};

    MetaDataStruct() = default;

    MetaDataStruct(const HEADER_FORMAT eFormat_) { eFormat = eFormat_; }

    MetaDataStruct(const HEADER_FORMAT eFormat_, const uint32_t uiLength_)
    {
        eFormat = eFormat_;
        uiLength = uiLength_;
    }

    ~MetaDataStruct() override = default;

    bool operator==(const MetaDataStruct& other_) const
    {
        return eFormat == other_.eFormat && eMeasurementSource == other_.eMeasurementSource && eTimeStatus == other_.eTimeStatus &&
               bResponse == other_.bResponse && usWeek == other_.usWeek && IsEqual(dMilliseconds, other_.dMilliseconds) &&
               uiLength == other_.uiLength && uiHeaderLength == other_.uiHeaderLength && usMessageId == other_.usMessageId &&
               uiMessageCrc == other_.uiMessageCrc && MessageName() == other_.MessageName();
    }
};

//-----------------------------------------------------------------------
//! \struct IntermediateHeader
//! \brief Structure containing raw information about an OEM header.
//-----------------------------------------------------------------------
struct IntermediateHeader
{
    uint16_t usMessageId{0};
    uint8_t ucMessageType{0};
    uint32_t uiPortAddress{0}; //  NOTE: This field is truncated in binary to uint8_t.
                               //  In ASCII, this field is a string, but binary truncates it.
                               //  Save as much of this info as possible without truncating it.
    uint16_t usLength{0};      //  This field will only be filled when decoding binary logs
    uint16_t usSequence{0};
    uint8_t ucIdleTime{0};
    uint32_t uiTimeStatus{0};
    uint16_t usWeek{0};
    double dMilliseconds{0.0};
    uint32_t uiReceiverStatus{0};
    uint32_t uiMessageDefinitionCrc{0};
    uint16_t usReceiverSwVersion{0};

    IntermediateHeader() = default;
};

#pragma pack(push, 1)

//-----------------------------------------------------------------------
//! \struct Oem4BinaryShortHeader
//! \brief Structure that represents an OEM Short Binary header and its
//! various fields.
//-----------------------------------------------------------------------
struct Oem4BinaryShortHeader
{
    uint8_t ucSync1{0};      //!< First sync byte of Header.
    uint8_t ucSync2{0};      //!< Second sync byte of Header.
    uint8_t ucSync3{0};      //!< Third sync byte of Header.
    uint8_t ucLength{0};     //!< Message body length.
    uint16_t usMessageId{0}; //!< Message ID of the log.
    uint16_t usWeekNo{0};    //!< GPS Week number.
    uint32_t uiWeekMSec{0};  //!< GPS Week seconds.

    Oem4BinaryShortHeader() = default;

    Oem4BinaryShortHeader(const IntermediateHeader& stInterHeader_)
    {
        ucSync1 = OEM4_BINARY_SYNC1;
        ucSync2 = OEM4_BINARY_SYNC2;
        ucSync3 = OEM4_SHORT_BINARY_SYNC3;
        ucLength = 0; // Will be filled in following the body encoding
        usMessageId = stInterHeader_.usMessageId;
        usWeekNo = stInterHeader_.usWeek;
        uiWeekMSec = static_cast<uint32_t>(stInterHeader_.dMilliseconds);
    }
};

//-----------------------------------------------------------------------
//! \struct Oem4BinaryHeader
//! \brief Structure that represents an OEM Binary header and its
//! various fields.
//-----------------------------------------------------------------------
struct Oem4BinaryHeader
{
    uint8_t ucSync1{0};              //!< First sync byte of Header.
    uint8_t ucSync2{0};              //!< Second sync byte of Header.
    uint8_t ucSync3{0};              //!< Third sync byte of Header.
    uint8_t ucHeaderLength{0};       //!< Total Binary header length.
    uint16_t usMsgNumber{0};         //!< Binary log Message Number/ID.
    uint8_t ucMsgType{0};            //!< Binary log Message type response or data?.
    uint8_t ucPort{0};               //!< Receiver Port of logging.
    uint16_t usLength{0};            //!< Total length of binary log.
    uint16_t usSequenceNumber{0};    //!< Sequence number of Embedded message inside.
    uint8_t ucIdleTime{0};           //!< Receiver Idle time.
    uint8_t ucTimeStatus{0};         //!< GPS reference time status.
    uint16_t usWeekNo{0};            //!< GPS Week number.
    uint32_t uiWeekMSec{0};          //!< GPS week seconds.
    uint32_t uiStatus{0};            //!< Status of the log.
    uint16_t usMsgDefCrc{0};         //!< Message def CRC of binary log.
    uint16_t usReceiverSwVersion{0}; //!< Receiver Software version.

    Oem4BinaryHeader() = default;

    Oem4BinaryHeader(const IntermediateHeader& stInterHeader_)
    {
        ucSync1 = OEM4_BINARY_SYNC1;
        ucSync2 = OEM4_BINARY_SYNC2;
        ucSync3 = OEM4_BINARY_SYNC3;
        ucHeaderLength = OEM4_BINARY_HEADER_LENGTH;
        usMsgNumber = stInterHeader_.usMessageId;
        ucMsgType = stInterHeader_.ucMessageType & (~static_cast<uint32_t>(MESSAGE_TYPE_MASK::MSGFORMAT));
        ucPort = static_cast<uint8_t>(stInterHeader_.uiPortAddress);
        usLength = stInterHeader_.usLength;
        usSequenceNumber = stInterHeader_.usSequence;
        ucIdleTime = stInterHeader_.ucIdleTime;
        ucTimeStatus = static_cast<uint8_t>(stInterHeader_.uiTimeStatus & 0xFF);
        usWeekNo = stInterHeader_.usWeek;
        uiWeekMSec = static_cast<uint32_t>(stInterHeader_.dMilliseconds);
        uiStatus = stInterHeader_.uiReceiverStatus;
        usMsgDefCrc = static_cast<uint16_t>(stInterHeader_.uiMessageDefinitionCrc & 0xFFFF);
        usReceiverSwVersion = stInterHeader_.usReceiverSwVersion;
    }

    bool operator==(const Oem4BinaryHeader& other_) const { return memcmp(this, &other_, sizeof(*this)) == 0; }

    bool operator==(const Oem4BinaryShortHeader& stShortHeader_) const
    {
        return ucSync1 == stShortHeader_.ucSync1 && ucSync2 == stShortHeader_.ucSync2 && ucSync3 + 1 == stShortHeader_.ucSync3 &&
               usMsgNumber + 1 == stShortHeader_.usMessageId && usLength == stShortHeader_.ucLength && usWeekNo == stShortHeader_.usWeekNo &&
               uiWeekMSec == stShortHeader_.uiWeekMSec;
    }
};

#pragma pack(pop)

} // namespace novatel::edie::oem

#endif // NOVATEL_EDIE_DECODERS_COMMON_HPP
