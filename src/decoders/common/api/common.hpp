////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 NovAtel Inc.
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
////////////////////////////////////////////////////////////////////////////////

/*! \file   common.hpp
 *  \brief  Header file to define structures/Enums/Macros...
 *          which are used in entire library
 *
 *  \author akhan
 *  \date   FEB 2021
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef COMMON_HPP
#define COMMON_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include "env.hpp"
#include <cmath>
#include <iterator>

/*! user defined macro for a MT_MINUS_UTC is '10800'*/
#define MT_MINUS_UTC 10800
/*! user defined macro for a SECINDAY is '86400'*/
#define SECINDAY 86400
/*! user defined macro for a SECS_IN_WEEK is '604800'*/
#define SECS_IN_WEEK 604800
/*! user defined macro for a GPS_TO_UTC_SECS is '315964800'*/
#define GPS_TO_UTC_SECS 315964800
#ifndef INTEGRATION
/*! user defined macro for a PI is '3.141592653589793'*/
#define PI ((DOUBLE)3.141592653589793)
/*! user defined macro for a maximum ASCII message length is 64000'*/
#define MAX_ASCII_MESSAGE_LENGTH 64000
/*! user defined macro for a maximum ABB ASCII response length PI is 127*/
#define MAX_ABB_ASCII_RESPONSE_LENGTH 127
/*! user defined macro for a NMEA message is 127*/
#define MAX_NMEA_MESSAGE_LENGTH 127

#endif // INTEGRATION
class MessageDataFilter;

//-----------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------

/*! An unsigned char constant.
 *
 * First sync byte of Binary log header.
 */
const UCHAR  OEM4_BINARY_SYNC1               = 0xAA;
/*! An unsigned char constant
 *
 *  Second sync byte of Binary log header.
 */
const UCHAR  OEM4_BINARY_SYNC2               = 0x44;
/*! An unsigned char constant
 *
 * Third sync byte of Binary log header.
 */
const UCHAR  OEM4_BINARY_SYNC3               = 0x12;
/*! An unsigned char constant
 *
 * Third sync byte of Binary log header.
 */
const UCHAR  OEM4_SHORT_BINARY_SYNC3         = 0x13;
/*! A char constant
 *
 *  OEM4 Abbreviated Ascii Message identifier.
 */
const CHAR   OEM4_ABBASCII_SYNC              = '<';
/*! A char constant
 *
 * OEM4 Ascii Message identifier.
 */
const CHAR   OEM4_ASCII_SYNC                 = '#';
/*! A char constant
 *
 * OEM4 Short Ascii Message identifier.
 */
const CHAR   OEM4_SHORT_ASCII_SYNC           = '%';
/*! A char constant
 *
 * NMEA Message identifier.
 */
const CHAR   NMEA_MESSAGE_SYNC               = '$';
/*! An unsigned short constant
 *
 *  OEM4 binary message CRC Length.
 */
const USHORT OEM4_BINARY_CRC_LENGTH          = 4;
/*! An unsigned short constant
 *
 *  OEM4 binary message header Length.
 */
const USHORT OEM4_BINARY_HEADER_LENGTH       = 28;
/*! An unsigned short constant
 *
 *  OEM4 short binary message header Length.
 */
const USHORT OEM4_BINARY_SHORT_HEADER_LENGTH = 12;
/*! An unsigned short constant
 *
 *  OEM4 Ascii message CRC Length.
 */
const USHORT OEM4_ASCII_CRC_LENGTH           = 8;
/*! An unsigned short constant
 *
 *  NMEA message CRC Length.
 */
const USHORT NMEA_MESSAGE_CRC_LENGTH         = 2;
/*! An unsigned long constant
 *
 * Used to convert from seconds to milli seconds.
 */
const ULONG  SECONDS_TO_MILLISECONDS         = 1000;
/*! A char constant
 *
 *  OEM4 Ascii message field seperator identifier.
 */
const CHAR   ASCII_FIELD_SEPERATOR           = ',';
/*! A char constant
 *
 *  OEM4 Ascii message header terminator identifier.
 */
const CHAR   ASCII_HEADER_TERMINATOR         = ';';

#if !defined(LF)
/*! A Macro definition for Line Feed (LF).*/
#define LF  (0x0A)
#endif

#if !defined(CR)
/*! A Macro definition for Carriage Return (CR).*/
#define CR  (0x0D)
#endif

/*! A Macro definition for byte alignment value for compiler.*/
#define COMPILER_BYTE_ALIGNMENT ((INT)4)

/*! \brief Compare two double values.
 *
 *  \pre None
 *  \post None
 *
 *  \param[in] dVal1 Firat double type Value
 *  \param[in] dVal2 Second double type value
 *
 *  \return Boolean Value - Returns both values are equal or not?
 */
BOOL IsEqual(DOUBLE dVal1, DOUBLE dVal2);

/*! A Structure
 *
 * Hold/copy decoded log and size of it.
 */
struct ReadDataStructure
{
   UINT uiDataSize; /*!< Size of decoded log */
   CHAR* cData;     /*!< Memory pointer*/

   /*! Default Intializer */
   ReadDataStructure()
   {
      uiDataSize = 0;
      cData = NULL;
   }
};

/*! A Structure
 *
 *  Provides decoder read statistics.
 */
struct StreamReadStatus
{
   UINT uiPercentStreamRead;  /*!< % of amount of data read from file */
   UINT uiCurrentStreamRead;  /*!< Number of bytes read for one Read */
   ULONGLONG ullStreamLength; /*!< Length of the bytes of decoded log */
   BOOL bEOS;                 /*!< IS EOF of file reached or not? */

   /*! Default Intializer */
   StreamReadStatus()
   {
      uiPercentStreamRead = 0;
      uiCurrentStreamRead = 0;
      ullStreamLength = 0;
      bEOS = FALSE;
   }
};

#pragma pack(push, 1)
/*! A Structure
 *
 *  OEM4 Binary log structure can be decoded.
 */
struct OEM4BinaryHeader
{
   UCHAR ucSync1;               /*!< First sync byte of Header */
   UCHAR ucSync2;               /*!< Second sync byte of Header */
   UCHAR ucSync3;               /*!< Third sync byte of Header */
   UCHAR ucHeaderLength;        /*!< Total Binary header length */
   USHORT usMsgNumber;          /*!< Binary log Message Number/ID */
   UCHAR ucMsgType;             /*!< Binary log Message type response or data? */
   UCHAR ucPort;                /*!< Reciever Port of logging */
   USHORT usLength;             /*!< Total length of binray log */
   USHORT usSequenceNumber;     /*!< Sequence number of Embedded message inside */
   UCHAR ucIdleTime;            /*!< Receiver Idle time  */
   UCHAR ucTimeStatus;          /*!< GPS reference time status */
   USHORT usWeekNo;             /*!< GPS Week number */
   ULONG ulWeekMSec;            /*!< GPS week seconds */
   ULONG ulStatus;              /*!< Status of the log */
   USHORT usMsgDefCRC;          /*!< Message def CRC of binary log */
   USHORT usReceiverSWVersion;  /*!< Receiver Software version */

   /*! Default Intializer */
   OEM4BinaryHeader()
   {
	   ucSync1 = 0;
	   ucSync2 = 0;
	   ucSync3 = 0;
	   ucHeaderLength = 0;
	   usMsgNumber = 0;
	   ucMsgType = 0;
	   ucPort = 0;
	   usLength = 0;
	   usSequenceNumber = 0;
	   ucIdleTime = 0;
	   ucTimeStatus = 0;
	   usWeekNo = 0;
	   ulWeekMSec = 0;
	   ulStatus = 0;
	   usMsgDefCRC = 0;
	   usReceiverSWVersion = 0;

   }

};

/*! A Structure
 *
 *  OEM4 Short Binary log structure can be decoded.
 */
struct OEM4BinaryShortHeader
{
   UCHAR  ucSync1;      /*!< First sync byte of Header */
   UCHAR  ucSync2;      /*!< Second sync byte of Header */
   UCHAR  ucSync3;      /*!< Third sync byte of Header */
   UCHAR  ucLength;     /*!< Message body length */
   USHORT usMessageId;  /*!< Message ID of the log */
   USHORT usWeekNo;     /*!< GPS Week number */
   ULONG  ulWeekMSec;   /*!< GPS Week seconds */

   OEM4BinaryShortHeader()
   {
	   ucSync1 = 0;
	   ucSync2 = 0;
	   ucSync3 = 0;
	   ucLength = 0;
	   usMessageId = 0;
	   usWeekNo = 0;
	   ulWeekMSec = 0;
   }
};
#pragma pack(pop)

/*! An enum.
 *
 *  Enumaration for state machine used while parsing the log.
 */
typedef enum
{
   WAITING_FOR_SYNC,                      /*!< First sync byte of Header */
   WAITING_FOR_BINARY_SYNC2,              /*!< Second sync byte of Header */
   WAITING_FOR_BINARY_SYNC3,              /*!< Tird sync byte of Header */
   WAITING_FOR_BINARY_HEADER,             /*!< Read complete Binary header*/
   WAITING_FOR_SHORT_BINARY_HEADER,       /*!< Read complete short Binary header*/
   WAITING_FOR_BINARY_BODY_AND_CRC,       /*!< Read complete Binary log including CRC*/
   WAITING_FOR_SHORT_BINARY_BODY_AND_CRC, /*!< Read complete short Binary log including CRC*/
   WAITING_FOR_ASCII_BODY,                /*!< Read complete ASCII log including CRC*/
   WAITING_FOR_SHORT_ASCII_BODY,          /*!< Read complete short ASCII log including CRC*/
   WAITING_FOR_ABB_ASCII_BODY,            /*!< Read complete Abbreviated ASCII log */
   WAITING_FOR_NMEA_BODY,                 /*!< Read complete NMEA log */
   COMPLETE_MESSAGE                       /*!< Completed decoding of one log */
} MessageParseStatusEnum;

/*! An enum.
 *
 *  Ascii Message header format sequence.
 */
typedef enum
{
   ASCII_MESSAGE_NAME,            /*!< Ascii log Name*/
   ASCII_PORT,                    /*!< Receiver logging port */
   ASCII_SEQUENCE,                /*!< Embedded log sequence number */
   ASCII_IDLETIME,                /*!< Receiver Idle time */
   ASCII_TIME_STATUS,             /*!< GPS reference time status */
   ASCII_WEEK,                    /*!< GPS Week number */
   ASCII_SECONDS,                 /*!< GPS week seconds */
   ASCII_RECEIVER_STATUS,         /*!< Receiver status*/
   ASCII_RESERVED,                /*!< Reserved Field*/
   ASCII_RECEIVER_SW_VERSION,     /*!< Receiver software versio */
   NUMBER_OF_ASCII_HEADER_ELEMENTS/*!< Number of elements in Ascii header */
} ASCIIHeaderEnum;

/*! An enum.
 *
 *  Short Ascii Message header format sequence.
 */
typedef enum
{
   SHORT_ASCII_MESSAGE_NAME,              /*!< Ascii log Name*/
   SHORT_ASCII_WEEK,                      /*!< GPS Week number */
   SHORT_ASCII_SECONDS,                   /*!< GPS week seconds */
   NUMBER_OF_SHORT_ASCII_HEADER_ELEMENTS  /*!< Number of elements in short Ascii header */
} ShortASCIIHeaderEnum;

/*! An enum.
 *
 * Message format enumaration to reprent type of the message.
 * Format Could be Ascii/Binary/Abb. Ascii/Rinex/NMEA2000 or Unknown.
 */
typedef enum
{
   MESSAGE_BINARY,     /*!< Decoded Log is Binary */
   MESSAGE_ASCII,      /*!< Decoded Log is Ascii */
   MESSAGE_SHORT_HEADER_BINARY,     /*!< Decoded Log is Binary */
   MESSAGE_SHORT_HEADER_ASCII,      /*!< Decoded Log is Ascii */
   MESSAGE_ABB_ASCII,  /*!< Decoded Log is Abbreviated Ascii */
   MESSAGE_UNKNOWN,    /*!< Decoded Log is unknown */
   MESSAGE_RINEX,      /*!< Decoded Log is Rinex Data */
   MESSAGE_NMEA2000    /*!< Decoded Log is NMEA2000 */
} MessageFormatEnum;

/*! An enum.
 *
 *  Recevier antenna source enumaration.
 */
typedef enum
{
   PRIMARY_ANTENNA,   /*!< Antenna is connected to primary antenna connector */
   SECONDARY_ANTENNA, /*!< Antenna is connected to seconday antenna connector */
   BOTH_ANTENNA       /*!< Antenna is connected to both */
} MessageAntennaSourceEnum;

/*! An enum.
 *
 * GPS Reference Time Status. The status indicates how well a time is known.
 */
typedef enum
{
   TIME_UNKNOWN = 20,              /*!< Time validity is unknown */
   TIME_APPROXIMATE = 60,          /*!< Time is set approximately */
   TIME_COARSEADJUSTING = 80,      /*!< Time is approaching coarse precision */
   TIME_COARSE = 100,              /*!< This time is valid to coarse precision */
   TIME_COARSESTEERING = 120,      /*!< Time is coarse set and is being steered */
   TIME_FREEWHEELING = 130,        /*!< Position is lost and the range bias cannot be calculated */
   TIME_FINEADJUSTING = 140,       /*!< Time is adjusting to fine precision */
   TIME_FINE = 160,                /*!< Time has fine precision */
   TIME_FINEBACKUPSTEERING = 170,  /*!< Time is fine set and is being steered by the backup system */
   TIME_FINESTEERING = 180,        /*!< Time is fine set and is being steered */
   TIME_SATTIME = 200              /*!< Time from satellite. Only used in logs containing satellite data such as ephemeris and almanac */
} MessageTimeStatusEnum;

/*! An enum.
 *
 *  BMD Output Formate
 */
typedef enum
{
	JSON,         /*!< BMD filled with decoded json string */
	FLATTEN,      /*!< BMD filled with decoded flatten message data */
	BOTH          /*!< BMD filled with both decoded message formates*/
} BMDOutputFormat;


/*! A Structure.
 *
 *  All the message information will be captured here.
 */
struct MessageHeader
{
   std::string szMessageName;                 /*!< Message name */
   UINT uiMessageID;                          /*!< Message ID */
   UINT uiMessageLength;                      /*!< Message Length */
   ULONG ulMessageWeek;                       /*!< GPS week number */
   ULONG ulMessageTime;                       /*!< GPS reference time seconds*/
   ULONG ulMessageDefCrc;                     /*!< Message Def CRC of the log */
   ULONG ulReceiverStatus;                    /*!< Recevier status */
   ULONG ulReceiverSWVersion;                 /*!< Recevier software version */
   ULONG ulSequenceNumber;                    /*!< Log sequence number */
   DOUBLE dIdleTime;                          /*!< Recevier Idle time */
   MessageAntennaSourceEnum eMessageSource;   /*!< Primary or secondary Antenna or both */
   MessageFormatEnum eMessageFormat;          /*!< Ascii or Binary or Abbreviated Ascii */
   MessageTimeStatusEnum eMessageTimeStatus;  /*!< GPS Reference Time Status */
   UCHAR ucPortAddress;                       /*!< Receiver Port address */
   BOOL bIsNMEAMessage;                       /*!< Is it NMEA message? */
   BOOL bErrorResponse;                       /*!< Data is Error response? */
   BOOL bMessageType;                         /*!< Is it log data or response data?*/
   INT iReponseID;                            /*!< Response ID */

   /*! Default Intializer */
   MessageHeader()
      :szMessageName(""),
      uiMessageID(0),
      uiMessageLength(0),
      ulMessageWeek(0),
      ulMessageTime(0),
      ulMessageDefCrc(0),
      ulReceiverStatus(0),
      ulReceiverSWVersion(0),
	  ulSequenceNumber(0),
      dIdleTime(0),
      eMessageSource(MessageAntennaSourceEnum::PRIMARY_ANTENNA),
      eMessageFormat(MessageFormatEnum::MESSAGE_UNKNOWN),
      eMessageTimeStatus(MessageTimeStatusEnum::TIME_UNKNOWN),
      ucPortAddress(0),
      bIsNMEAMessage(FALSE),
      bErrorResponse(FALSE),
      bMessageType(FALSE),
      iReponseID(0)
   {
   }
};

/*! A Structure.
 *
 * OEM4 Binary response data.
 */
struct OEM4BinaryReponseData
{
   OEM4BinaryHeader   stOem4BinaryHeader;  /*!< OEM4 Binary message header structure */
   INT                iResponseID;         /*!< OEM4 binary Response ID */
   CHAR               szResponsStr[40];    /*!< Dummy data only used to get response id from structure */
};

/*! A Structure.
 *
 *  Provides Message statistics like name, ID, Nuber of ASCII/Binary messages...
 */
struct MessageInfo
{
   std::string szMessageName;   /*!< Log Message Name, Default value: "" */
   UINT uiMessageID;            /*!< Log Message ID, Default value: 0 */
   UINT uiBinaryMessages;       /*!< Number of Binary messgaes, Default value: 0 */
   UINT uiAsciiMessages;        /*!< Number of Ascii messages, Default value: 0 */
   UINT uiShortBinaryMessages;  /*!< Number of Short Binary, Default value: 0 */
   UINT uiShortAsciiMessages;   /*!< Number of Short Ascii messages, Default value: 0 */

   /*! Default Intializer */
   MessageInfo()
      : szMessageName(""),
      uiMessageID(0),
      uiBinaryMessages(0),
      uiAsciiMessages(0),
      uiShortBinaryMessages(0),
      uiShortAsciiMessages(0)
   {
   }
};

/*! A Structure.
 *
 *  Provides Unknown message statistics, Default value for all members are '0'.
 */
struct UnknownDataStatistics
{
   ULONG ulLineFeeds;           /*!< Number of Line Feed's */
   ULONG ulCarriageReturns;     /*!< Number of Carriage Return's */
   ULONG ulInvalidBinaryMsgs;   /*!< Number of Invalid Binary Messages */
   ULONG ulInvalidAsciiMsgs;    /*!< Number of Invalid Ascii Messages */
   ULONG ulCOMPorts;            /*!< Number of [COMx] ports */
   ULONG ulOKPrompts;           /*!< Number of <OK prompts */
   ULONG ulUnknownAsciiBytes;   /*!< Number of unknown Ascii Bytes */
   ULONG ulUnknownBinaryBytes;  /*!< Number of unknown Binary Bytes */
   ULONG ulInvalidCOMPorts;     /*!< Number of Invalid [COMx] ports */
   ULONG ulValidCOMPortBytes;   /*!< Number of Valid [COMx] port bytes */

   /*! Default Intializer */
   UnknownDataStatistics()
   {
      ulLineFeeds = 0;
      ulCarriageReturns = 0;
      ulInvalidBinaryMsgs = 0;
      ulInvalidAsciiMsgs = 0;
      ulCOMPorts = 0;
      ulOKPrompts = 0;
      ulUnknownAsciiBytes = 0;
      ulUnknownBinaryBytes = 0;
      ulInvalidCOMPorts = 0;
      ulValidCOMPortBytes = 0;
   }
};

/*! A Structure.
 *
 *  Statistics provided by decoder, All members default values are '0'.
 */
struct DecoderStatistics
{
   ULONG ulTotalUniqueMessages; /*!< Total number of unique messages decoded successfully */
   ULONG ulStartWeek;           /*!< Message start week (when looging stated)*/
   ULONG ulStartTimeMSec;       /*!< Message start week seconds (when looging stated) */
   ULONG ulEndWeek;             /*!< Message end week (when looging stopped)*/
   ULONG ulEndTimeMSec;         /*!< Message end week seconds (when looging stopped)*/

   /*! Default Intializer */
   DecoderStatistics()
   {
      ulTotalUniqueMessages = 0;
      ulStartWeek = 0;
      ulStartTimeMSec = 0;
      ulEndWeek = 0;
      ulEndTimeMSec = 0;
   }
};

/*! A Structure.
 *
 * Provieds way to configure different filters through this structure while decoding the messages.
 */
struct FilterConfig
{
   /*! vector of Message ID and Message Format enumaration pair to use for filter */
   std::vector < std::pair <UINT, MessageFormatEnum> > pIDFormatPair;
   /*! vector of Message ID and Antenna Source enumaration pair to use for filter */
   std::vector < std::pair <UINT, MessageAntennaSourceEnum> > pIDSourcePair;
   /*! vector of Start Time (GPR Week seconds and GPS week number) pair to use for filter */
   std::vector < std::pair <ULONG, ULONG> > ulStartTimePair;
   /*! vector of End Time (GPR Week seconds and GPS week number) pair to use for filter */
   std::vector < std::pair <ULONG, ULONG> > ulEndTimePair;
   /*! vector of MessageDataFilter Class for filter */
   std::vector < MessageDataFilter* > pMessageDataFilterVec;
   /*! \var bIsNegFilter
       \brief If True, Messages will be filtered with in limits.
      if False, Messages will be filtered out of limits.
   */
   BOOL bIsNegFilter;

   /*! \var DOUBLE dSamplePeriod
       \brief Variable used for Filter with Sample period time
   */
   DOUBLE dSamplePeriod;

   /*! \var std::vector < std::pair <MessageAntennaSourceEnum, MessageFormatEnum> > pSourceFormatPair
       \brief Vector of Antenna source and Message Format enumaration pair
   */
   std::vector < std::pair <MessageAntennaSourceEnum, MessageFormatEnum> > pSourceFormatPair;

   /*! \var std::vector < std::string > szMessageName
       \brief Vector contains Message name strings
   */
   std::vector < std::string > szMessageName;

   /*! \var bIsNegTimeFilter
       \brief If True, Messages will be filtered with in time(Week & Week seconds) limits.
      if False, Messages will be filtered out of time(Week & Week seconds) limits.
   */
   BOOL bIsNegTimeFilter;
   /*!< Enable or Diable SATTIME filter */
   BOOL bSatTimeFilter;

   /*!< Default Constructor, All defaults are '0' ans clear of all maps*/
   FilterConfig()
      :  bIsNegFilter(FALSE),
      dSamplePeriod(0.0),
      bIsNegTimeFilter(FALSE),
      bSatTimeFilter(TRUE)
   {
      szMessageName.clear();
      pIDFormatPair.clear();
      pIDSourcePair.clear();
      ulStartTimePair.clear();
      ulEndTimePair.clear();
      pSourceFormatPair.clear();
      pMessageDataFilterVec.clear();
   }

   /*!< Default destructor */
   ~FilterConfig() {}

   /*! \brief Emplace back Message name and Format enumaration pair
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] uiMessage Message ID.
    *  \param[in] eMessageFormatEnum Message Format Enumaration.
    *  \param[in] eMessageAntennaSource optional Antenna source enumaration.
    *
    *  \return Does not return value.
    *  \remark None.
    */
   void AddIDFormatPair(UINT uiMessage, MessageFormatEnum eMessageFormatEnum, MessageAntennaSourceEnum eMessageAntennaSource = BOTH_ANTENNA)
   {
      pIDFormatPair.reserve(1);
      pIDFormatPair.emplace_back( std::make_pair(uiMessage, eMessageFormatEnum) );
      if (std::find(pIDSourcePair.begin(), pIDSourcePair.end(), std::make_pair(uiMessage, eMessageAntennaSource)) == pIDSourcePair.end())
      {
         pIDSourcePair.reserve(1);
         pIDSourcePair.emplace_back(std::make_pair(uiMessage, eMessageAntennaSource));
      }
   }

   /*! \brief Emplace back Start GPS week seconds and GPS Week number pair
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] ulMilliSeconds GPS Week Seconds.
    *  \param[in] ulGnssWeek optional GPS Week Number.
    *
    *  \return Does not return value.
    *  \remark Start week is optional here, whereas start time is mandatory.
    *          If start week value will not be passed/assigned, then the default start week value will be assigned as 0.
    */
   void AddStartTimePair(ULONG ulMilliSeconds, ULONG ulGnssWeek = 0)
   {
      ulStartTimePair.reserve(1);
      ulStartTimePair.emplace_back( std::make_pair(ulMilliSeconds, ulGnssWeek) );
   }

   /*! \brief Emplace back End GPS week seconds and GPS Week number pair
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] ulMilliSeconds GPS Week Seconds.
    *  \param[in] ulGnssWeek optional GPS Week Number.
    *
    *  \return Does not return value.
    *  \remark End week is optional here, whereas start time is mandatory.
    *          If end week value will not be passed/assigned, then the default start week value will be assigned as 0.
    */
    void AddEndTimePair(ULONG ulMilliSeconds, ULONG ulGnssWeek = 0)
   {
      ulEndTimePair.reserve(1);
      ulEndTimePair.emplace_back( std::make_pair(ulMilliSeconds, ulGnssWeek) );
   }

   /*! \brief Emplace back Filter container class
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] pMessageDataFilter Filter container class.
    *
    *  \return Does not return value.
    *  \remark None.
    */
   void AddFilterToContainer(MessageDataFilter* pMessageDataFilter)
   {
      pMessageDataFilterVec.reserve(1);
      pMessageDataFilterVec.emplace_back(pMessageDataFilter);
   }

   /*! \brief Enable/Disable Negativ Filter
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] bNegFilter Boolean variable.
    *
    *  \return Does not return value.
    *
    *  \remark If Enable Filter out messages in configured limits
    *          If Disable Filter out messages out of configured limits
    */
   void IsNegativeFilter(BOOL bNegFilter)
   {
      bIsNegFilter = bNegFilter;
   }

   /*! \brief Sets Sample time period
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] dPeriod Double variable.
    *
    *  \return Does not return value.
    *  \remark None
    */
   void AddSamplePeriod(DOUBLE dPeriod)
   {
      dSamplePeriod = dPeriod;
   }

   /*! \brief Adds Antenna Source and Message Format enumaration pair
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] eMessageAntennaSourceEnum Antenna source enumaration value.
    *  \param[in] eMessageFormatEnum Message Format enumaration value.
    *
    *  \return Does not return value.
    *  \remark None
    */
   void AddSourceFormatPair(MessageAntennaSourceEnum eMessageAntennaSourceEnum, MessageFormatEnum eMessageFormatEnum)
   {
      pSourceFormatPair.reserve(1);
      pSourceFormatPair.emplace_back( std::make_pair(eMessageAntennaSourceEnum, eMessageFormatEnum) );
   }

   /*! \brief Adds Message Name to vector
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] szMsgName Message Name to be filer.
    *
    *  \return Does not return value.
    *  \remark None
    */
   void AddMessageName(std::string szMsgName)
   {
      szMessageName.reserve(1);
      szMessageName.emplace_back(szMsgName);
   }

   /*! \brief Enable/Disable Negative Time Filter
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] bNegTimeFilter Boolean value.
    *
    *  \return Does not return value.
    *
    *  \remark If Enable, Filter out messages in between configured GPS Week and Seconds limits
    *          If Disable, Filter out messages out of configured GPS Week and Seconds limits
    */
   void IsNegativeTimeFilter(BOOL bNegTimeFilter)
   {
      bIsNegTimeFilter = bNegTimeFilter;
   }

   /*! \brief Enable/Disable SATTIME Filter
    *
    *  \pre None
    *  \post None
    *
    *  \param[in] bEnableFilter Boolean value.
    *
    *  \return Does not return value.
    *
    *  \remark If Enable, Filter out messages with Time from satellite.
    *          If Disable, Filter out messages which does not had Time from satellite.
    *          Only used in logs containing satellite data such as ephemeris and almanac.
    */
   void EnableSatTimeFilter(BOOL bEnableFilter)
   {
      bSatTimeFilter = bEnableFilter;
   }
};

/*! An enum.
 *
 *  Filter type enumaration.
 */
typedef enum
{
   MESSAGE_FILTER,       /*!< Filter with Message name */
   TIME_FILTER,          /*!< Filter with Time Week and Week Seconds pair */
   DECIMATION_FILTER,    /*!< Filter with Sample period time */
   SOURCE_FILTER,        /*!< Filter with Antenna source and MEssage format pair*/
   SATTIME_FILTER,       /*!< Filter with GPS reference time status */
   CONSTELLATION_FILTER, /*!< Filter with Constellation */
   CONTAINER_FILTER,     /*!< Filter with Combination of come filters */
} FILTERTYPE;

/*! A Structure.
 *
 * Provides Satellite number and frequency channel, Default values are '0'.
 */
struct SATELLITEID
{
   USHORT usPrnOrSlot;       /*!< PRN/Slot number of satellite*/
   SHORT  sFrequencyChannel; /*!< Frequency channel number*/
   /*! Default Intializer */
   SATELLITEID()
   {
      usPrnOrSlot = 0;
      sFrequencyChannel = 0;
   }
};

/*! An Enum.
 *
 * Splitting type of log enumaration while writing to different output files.
 */
typedef enum
{
   SPLIT_SIZE,   /*!< Split based on Size of log */
   SPLIT_LOG,    /*!< Split based on Log name */
   SPLIT_TIME,   /*!< Split based on time of log */
   SPLIT_NONE    /*!< Do not split */
} FileSplitMethodEnum;

/*! A macro for Maximum of circullar buffer size to read data from input stream */
#define BUFFER_SIZE 10240


/*! A constant integer value for Maximum Ascii message name length */
const INT MAX_ASCII_MESSAGE_NAME             = 40;
/*! A constant integer value for Maximum virtual address length */
const INT MAX_VIRTUAL_ADDRESS                = 31;
/*! A constant unsigned character value for virtual address mask */
const UCHAR VIRTUAL_ADDRESS_MASK             = 0x1f;
/*! A constant integer value for position of port value in message */
const INT PORT_POSITION                      = 5;
/*! A constant integer value for port mask to extract port */
const INT PORT_MASK                          = 0xe0;
/*! A constant integer value for MAximum AScii buffer size */
const INT MAX_ASCII_BUFFER_SIZE              = 20480;
/*! A const integer value for response mask in message */
const INT MSG_RESPONSE_MASK                  = 0x80;
/*! A constant unsigned long value for Message format mask */
const ULONG MSG_FORMAT_MASK                  = 0x600000;
/*! A const integer value for positon of message format in a message */
const INT MSG_FORMAT_POSITION                = 21;
/*! A const character value for quote value */
const CHAR QUOTE                             = '"';
/*! A const character value for Ascii message body and CRC seperator */
const CHAR BODY_CRC_SEPARATOR                = '*';
/*! A constant integer value for Binary message CRC size */
const INT BINARY_CRC_SIZE                    = sizeof(INT);
/*! A constant integer value for Ascii message CRC size */
const INT ASCII_CRC_SIZE                     = BINARY_CRC_SIZE * 2;
/*! A constant integer value for number of ascii header fields */
const INT HEADER_NUM_ASCII_FIELDS            = 10;
/*! A constant integer value for field position of Ascii message name in the header */
const INT HEADER_NAME_FIELD                  = 1;
/*! A constant integer value for field position of port address in the header */
const INT HEADER_PORT_ADDRESS_FIELD          = 2;
/*! A constant integer value for field position of sequence number in the header */
const INT HEADER_SEQ_NUM                     = 3;
/*! A constant integer value for field position of Receiver Idle time in the header */
const INT HEADER_IDLE_TIME                   = 4;
/*! A constant integer value for field position of time status in the header */
const INT HEADER_TIME_STATUS_FIELD           = 5;
/*! A constant integer value for field position of GNSS week number in the header */
const INT HEADER_WEEK_NUMBER_FIELD           = 6;
/*! A constant integer value for field position of GNSS week seconds in the header */
const INT HEADER_MILLISECONDS_FIELD          = 7;
/*! A constant integer value for field position of Receiver status in the header */
const INT HEADER_RECEIVER_STATUS_FIELD       = 8;
/*! A constant integer value for field position of message def CRC in the header */
const INT HEADER_MESSAGE_DEF_CRC_FIELD       = 9;
/*! A constant integer value for field position of Receiver software versio in the header */
const INT HEADER_RECEIVER_SW_VERSION_FIELD   = 10;
/*! A constant integer value for number of fields in compressed ascii message header */
const INT HEADER_NUM_COMP_ASCII_FIELDS       = 3;
/*! A constant integer value for field position of message name in the compressed ascii message header */
const INT HEADER_COMP_NAME_FIELD             = 1;
/*! A constant integer value for  field position of GNSS week number in the compressed ascii message header */
const INT HEADER_COMP_WEEK_NUMBER_FIELD      = 2;
/*! A constant integer value for  field position of GNSS week seconds in the compressed ascii message header */
const INT HEADER_COMP_MILLISECONDS_FIELD     = 3;
/*! A constant character value for Abbreviated Ascii sync byte */
const CHAR ABB_ASCII_SYNC                    = '<';
/*! A constant character value for Abbreviated Ascii message fields seperator */
const CHAR ABB_ASCII_FIELD_SEPERATOR         = ' ';
/*! A constant integer value for maximum enumaration value length */
const INT MAX_ENUM_VALUE_LENGTH              = 100;
/*! A constant integer value for length offset in Binary message header */
const INT LENGTH_OFFSET                      = 8;

/*! A Macro definition for Abbreviated Ascii message header terminator string */
#define ABB_HEADER_TERMINATOR "\n<     "
/*! A Macro definition for class array terminator string  in Abbreviated Ascii message */
#define ABB_CLASSARRAY_TERMINATOR "\n<          "
/*! A Macro definition for Maximum novatel message size buffer to be used in code */
#define MESSAGE_SIZE_MAX    ((INT)20480)
/*! A Macro definition for code compiler byte alignment */
#define COMPILER_BYTE_ALIGNMENT ((INT)4)
/*! A Macro definition for number of milliseconds in a second */
#define SEC_TO_MSEC ((ULONG)1000)

/*! An enum.
 *
 *  Enumaration for message convert status.
 */
typedef enum
{
   INVALID_MESSAGE_ID,         /*!< Message ID is invalid */
   INVALID_MESSAGE_FORMAT,     /*!< Message format is invalid*/
   INVALID_CRC,                /*!< Message CRC is invalid */
   EMPTY_MESSAGE,              /*!< No data in message */
   GOOD_MESSAGE,               /*!< Message converted successfull */
   GOOD_MESSAGE_NO_TIME,       /*!< Message Converted successfull but with no time in it */
   RESPONSE_MESSAGE,           /*!< It is a response message */
   UNEXPECTED_END_OF_MESSAGE,  /*!< Got unexpected end for message */
   INVALID_MODEL,              /*!< Receiver model is invalid */
   BLANK_MESSAGE               /*!< Blank message received */
} MsgConvertStatusEnum;

/*! An enum.
 *
 *  Enumaration for novatel message style.
 */
typedef enum
{
   OEM4_MESSAGE_STYLE,          /*!< Message is OEM4 type */
   OEM3_NOTIME_MESSAGE_STYLE,   /*!< Message is OEM3 type with no time */
   OEM3_TIME_MESSAGE_STYLE,     /*!< Message is OEM3 type */
   RTCA_STYLE,                  /*!< Message is RTCA type */
   RTCM_STYLE,                  /*!< Message is RTCM type */
   OEM4_COMPRESSED_STYLE,       /*!< Message is OEM4 compressed type */
   MSG_STYLE_FILLER = 320000    /*!< Message is Filter type */
} MessageStyleEnum;

/*! An enum.
 *
 *  Enumaration for novatel message format.
 */
typedef enum
{
   ASCII,      /*!< Novatel Ascii Message */
   BINARY,     /*!< Novatel Binary Message */
   AASCII,     /*!< Novatel Abbreviated Ascii Message */
   RINEX2_1,   /*!< Rinex 2.1 Message */
   RINEX3_01,  /*!< Rinex 3.01 Message */
   RINEX3_02,  /*!< Rinex 3.02 Message */
   RINEX3_03,  /*!< Rinex 3.03 Message */
   RINEX3_04   /*!< Rinex 3.04 Message */
}Format;

/*! An enum.
 *
 *  Enumaration for novatel message type format.
 */
typedef enum
{
   MBINARY,      /*!< Binary format message */
   MASCII,       /*!< Ascii format message */
   MAASCII       /*!< Abbreviated Ascii message */
}MsgTypeFormat;

/*! An enum.
 *
 *  Enumaration for Novatel Receiver communication port.
 */
typedef enum
{
   MSGCVT_SPECIAL = 0,          /*!< Special designation */
   MSGCVT_COMPORT1 = 1,         /*!< COM1 communication port */
   MSGCVT_COMPORT2 = 2,         /*!< COM2 communication port */
   MSGCVT_COMPORT3 = 3,         /*!< COM3 communication port */
   MSGCVT_COMPORT_UNKNOWN = 5,  /*!< USB or XCOM communication port */
   MSGCVT_COMPORT_THISPORT = 6, /*!< The incoming port is the port used */
   MSGCVT_COMPORT_FILE = 7,     /*!< FILE communications port */
   MSGCVT_COMPORTX1 = 9,        /*!< Internal virutal port XCOM1 */
   MSGCVT_COMPORTX2 = 10,       /*!< Internal virutal port XCOM2 */
   MSGCVT_COMPORT_USB1 = 13,    /*!< USB communication port 1 */
   MSGCVT_COMPORT_USB2 = 14,    /*!< USB communication port 2 */
   MSGCVT_COMPORT_USB3 = 15,    /*!< USB communication port 3 */
   MSGCVT_COMPORT_AUX = 16,     /*!< AUX communication port */
   MSGCVT_COMPORTX3 = 17,       /*!< Internal virtual port XCOM3 */
   MSGCVT_COMPORT_MAX           /*!< THIS ALWAYS THE LAST ENUM!!!! */
} CommunicationPortEnum;

/*! An enum.
 *
 *  Enumaration for GPS reference time status. The status indicates how well a time is known.
 */
typedef enum
{
   MSG_GPSTIME_UNKNOWN = 20,         /*!< Time validity is unknown */
   MSG_GPSTIME_USER_ADJUSTING = 40,  /*!< Time is adjested by user */
   MSG_GPSTIME_USER = 60,            /*!< Time is set approximately */
   MSG_GPSTIME_COARSEADJUSTING = 80, /*!< Time is approaching coarse precision */
   MSG_GPSTIME_COARSE = 100,         /*!< This time is valid to coarse precision */
   MSG_GPSTIME_COARSESTEERING = 120, /*!< Time is coarse set and is being steered */
   MSG_GPSTIME_FREEWHEELING = 130,   /*!< Position is lost and the range bias cannot be calculated */
   MSG_GPSTIME_FINEADJUSTING = 140,  /*!< Time is adjusting to fine precision */
   MSG_GPSTIME_FINE = 160,           /*!< Time has fine precision */
   MSG_GPSTIME_FINESTEERING = 180,   /*!< Time is fine set and is being steered */
   MSG_GPSTIME_SATTIME = 200,        /*!< Time from satellite. Only used in logs containing satellite data such as ephemeris and almanac */
   MSG_GPSTIME_EXTERN = 220,         /*!< Time source is external to the Reciever */
   MSG_GPSTIME_EXACT = 240           /*!< Exact Time value */
} MsgConvertTimeStatusEnum;

/*! An enum.
 *
 *  Enumaration for Type of a satellite has been tracked.
 */
typedef enum{
   GPS_SAT = 0,       /*!< Tracked GPS satellite */
   GLONASS_SAT = 1,   /*!< Tracked GLONASS satellite */
   SBAS_SAT = 2,      /*!< Tracked SBAS satellite */
   GALILEO_SAT = 3,   /*!< Tracked GALILEO satellite */
   BEIDOU_SAT = 4,    /*!< Tracked BEIDOU satellite */
   QZSS_SAT = 5,      /*!< Tracked QZSS satellite */
   NAVIC_SAT = 6,     /*!< Tracked NAVIC satellite */
   OTHER_SAT = 7      /*!< Tracked other than GPS, GLONASS, SBAS, GALILEO, BEIDOU, QZSS and NAVIC satellites */
}SatelliteTypeEnum;

/*! An enum.
 *
 *  Enumaration for Type of a filter is being used to filter Constellation.
 */
typedef enum{
   GPS_CONSTELLATION_FILTER = 0,     /*!< Filter out GPS satellites */
   GLONASS_CONSTELLATION_FILTER = 1, /*!< Filter out GLONASS satellites */
   SBAS_CONSTELLATION_FILTER = 2,    /*!< Filter out SBAS satellites */
   GALILEO_CONSTELLATION_FILTER = 3, /*!< Filter out GALILEO satellites */
   BDS_CONSTELLATION_FILTER = 4,     /*!< Filter out BEIDOU satellites */
   QZSS_CONSTELLATION_FILTER = 5,    /*!< Filter out QZSS satellites */
   NAVIC_CONSTELATION_FILTER = 6     /*!< Filter out NAVIC satellites */
}ConstellationTypeEnum;

/*! An enum.
 *
 *  Enumaration for filter Constellation in Compressed logs type.
 */
typedef enum{
   GPS_COMPRESSED_CONSTELLATION_FILTER = 0,     /*!< Filter out GPS satellites */
   GLONASS_COMPRESSED_CONSTELLATION_FILTER = 1, /*!< Filter out GLONASS satellites */
   SBAS_COMPRESSED_CONSTELLATION_FILTER = 2,    /*!< Filter out SBAS satellites */
   GALILEO_COMPRESSED_CONSTELLATION_FILTER = 5, /*!< Filter out GALILEO satellites */
   BDS_COMPRESSED_CONSTELLATION_FILTER = 6,     /*!< Filter out BEIDOU satellites */
   QZSS_COMPRESSED_CONSTELLATION_FILTER = 7,    /*!< Filter out QZSS satellites */
   NAVIC_COMPRESSED_CONSTELATION_FILTER = 9     /*!< Filter out NAVIC satellites */
}ConstellationCompressedLogTypeEnum;

/*! An enum.
 *
 *  Enumaration to find seperator in a message.
 */
typedef enum
{
   PAST_END_OF_BUFFER,      /*!< Reached end of the buffer */
   SEPARATOR_FOUND,         /*!< Found Seperator */
   HEADER_TERMINATOR_FOUND, /*!< Found Message header terminator */
   INVALID_SEPARATOR        /*!< Found Invalid seperator */
} SeparatorEnum;


#endif
