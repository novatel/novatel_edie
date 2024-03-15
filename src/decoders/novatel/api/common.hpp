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
//! used across the NovAtel framer, decoder, parser, unit tests and
//! example program source code.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef NOVATEL_COMMON_HPP
#define NOVATEL_COMMON_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <stdint.h>
#include <string.h>

#include "decoders/common/api/common.hpp"

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
//! \enum NovAtelFrameStateEnum
//! \brief Enumeration for state machine used while framing the log.
//-----------------------------------------------------------------------
enum class NovAtelFrameStateEnum
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
//! \enum ASCIIHEADER
//! \brief Ascii Message header format sequence.
//-----------------------------------------------------------------------
enum class ASCIIHEADER
{
    MESSAGE_NAME,        //!< Ascii log Name.
    PORT,                //!< Receiver logging port.
    SEQUENCE,            //!< Embedded log sequence number.
    IDLETIME,            //!< Receiver Idle time.
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

    MetaDataStruct(HEADERFORMAT eFormat_) { eFormat = eFormat_; }

    MetaDataStruct(HEADERFORMAT eFormat_, uint32_t uiLength_)
    {
        eFormat = eFormat_;
        uiLength = uiLength_;
    }

    ~MetaDataStruct() override = default;

    bool operator==(const MetaDataStruct& other) const
    {
        return eFormat == other.eFormat && eMeasurementSource == other.eMeasurementSource && eTimeStatus == other.eTimeStatus &&
               bResponse == other.bResponse && usWeek == other.usWeek && dMilliseconds == other.dMilliseconds && uiLength == other.uiLength &&
               uiHeaderLength == other.uiHeaderLength && usMessageID == other.usMessageID && uiMessageCRC == other.uiMessageCRC &&
               MessageName() == other.MessageName();
    }
};

//-----------------------------------------------------------------------
//! \struct IntermediateHeader
//! \brief Structure containing raw information about an OEM header.
//-----------------------------------------------------------------------
struct IntermediateHeader
{
    uint16_t usMessageID{0};
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
    uint32_t uiMessageDefinitionCRC{0};
    uint16_t usReceiverSwVersion{0};

    constexpr IntermediateHeader() = default;
};

#pragma pack(push, 1)

//-----------------------------------------------------------------------
//! \struct OEM4BinaryShortHeader
//! \brief Structure that represents an OEM Short Binary header and its
//! various fields.
//-----------------------------------------------------------------------
struct OEM4BinaryShortHeader
{
    uint8_t ucSync1{0};      //!< First sync byte of Header.
    uint8_t ucSync2{0};      //!< Second sync byte of Header.
    uint8_t ucSync3{0};      //!< Third sync byte of Header.
    uint8_t ucLength{0};     //!< Message body length.
    uint16_t usMessageId{0}; //!< Message ID of the log.
    uint16_t usWeekNo{0};    //!< GPS Week number.
    uint32_t uiWeekMSec{0};  //!< GPS Week seconds.

    constexpr OEM4BinaryShortHeader() = default;
};

//-----------------------------------------------------------------------
//! \struct OEM4BinaryHeader
//! \brief Structure that represents an OEM Binary header and its
//! various fields.
//-----------------------------------------------------------------------
struct OEM4BinaryHeader
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
    uint16_t usMsgDefCRC{0};         //!< Message def CRC of binary log.
    uint16_t usReceiverSWVersion{0}; //!< Receiver Software version.

    constexpr OEM4BinaryHeader() = default;

    bool operator==(const OEM4BinaryHeader& other) const { return memcmp(this, &other, sizeof(*this)) == 0; }

    bool operator==(const OEM4BinaryShortHeader& stShortHeader) const
    {
        return ucSync1 == stShortHeader.ucSync1 && ucSync2 == stShortHeader.ucSync2 && ucSync3 + 1 == stShortHeader.ucSync3 &&
               usMsgNumber + 1 == stShortHeader.usMessageId && usLength == stShortHeader.ucLength && usWeekNo == stShortHeader.usWeekNo &&
               uiWeekMSec == stShortHeader.uiWeekMSec;
    }
};

#pragma pack(pop)

} // namespace novatel::edie::oem
#endif // NOVATEL_COMMON_HPP
