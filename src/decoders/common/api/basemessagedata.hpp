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

/*! \file basemessagedata.hpp
 *  \brief BaseMessageData is a class that will be used to communicate with the EDIE library.
 *  \author Gopi R
 *  \date   FEB 2021
 * Interface Object from application to library. Application can get everything from this class only.
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef BASEMESSAGEDATA_H
#define BASEMESSAGEDATA_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "common.hpp"
#include "stringtotypes.hpp"
#include "nexcept.h"

/*! \brief An interface class to the application and library.
 *
 * BaseMessageData is a class that will be used to communicate with the EDIE library.
 * For example, the Decoder will return a message to the application as a BaseMessageData
 * object and the Encoder will receive a BaseMessageData object from the application
 * and convert it from one format to another. Components will also use BaseMessageData
 * to pass information between them inside the library. For example, decoder will pass
 * the BaseMessageData object to a filter to check whether that message is allowed or
 * to be discard.
 */

class BaseMessageData
{
public:
   /*! \brief Default Class constructor. */
   BaseMessageData();

   /*! \brief Class constructor
    *
    *  \param [in] pstMessageHeader Message Header information pointer
    *  \param [in] pcDecodedMessage Decoded frame pointer
    */
   BaseMessageData(
      MessageHeader* pstMessageHeader,
      CHAR* pcDecodedMessage);

   /*! \brief Class destructor. */
    virtual ~BaseMessageData();

   /*! \brief Sets decoded log name
    *
    *  \param [in] sMessageName  Decoded log name
    *  \return "Does not return a value"
    */
   void setMessageName(const std::string& sMessageName);

   /*! \brief Returns decoded log name. */
   std::string getMessageName(void) const;

   /*! \brief Sets Message ID of the log
    *
    *  \param [in] uiMessageID Unsinged int value of ID
    *  \return "Does not return a value"
    */
   void setMessageID(UINT uiMessageID);

   /*! \brief Returns Message ID of the log.*/
   UINT getMessageID(void) const;

   /*! \brief Sets Message format to ASCII/BINARY/...
    *
    *  \param [in] eMessageFormat Type of Message enumaration
    *  \return "Does not return a value"
    */
   void setMessageFormat(MessageFormatEnum eMessageFormat);

   /*! \brief Returns Message Format could be ASCII/BINARY/...*/
   MessageFormatEnum getMessageFormat(void) const;

   /*! \brief Sets GPS Reference Time Status of the log
    *
    *  \param [in] eMessageTimeStatus GPS Reference Time Status enumaration
    *  \return "Does not return a value"
    */
   void setMessageTimeStatus(MessageTimeStatusEnum eMessageTimeStatus);

   /*! \brief Returns GPS Reference Time Status of the log.*/
   MessageTimeStatusEnum getMessageTimeStatus(void) const;

   /*! \brief Sets GPS Reference Week of the log
    *
    *  \param [in] ulGnssWeek ULONG value
    *  \return "Does not return a value"
    */
   void setMessageTimeWeek(ULONG ulGnssWeek);

   /*! \brief Returns GPS Reference Week of the log.*/
   ULONG getMessageTimeWeek(void) const;

   /*! \brief Sets Message GPS Reference Week seconds of the log
    *
    *  \param [in] ulMilliSeconds ULONG value
    *  \return "Does not return a value"
    */
   void setMessageTimeMilliSeconds(ULONG ulMilliSeconds);

   /*! \brief Returns Message GPS Reference Week seconds of the log. */
   ULONG getMessageTimeMilliSeconds(void) const;

   /*! \brief Sets Antenna Source of the log
    *
    *  \param [in] eMessageSource Antenna source enumaration
    *  \return "Does not return a value"
    */
   void setMessageSource(MessageAntennaSourceEnum eMessageSource);

   /*! \brief Returns Antenna Source of the log. */
   MessageAntennaSourceEnum getMessageSource(void) const;

   /*! \brief Sets Decoded message
    *
    *  \param [in] pcMessageData
    *  \return "Does not return a value"
    */
   void setMessageData(CHAR* pcMessageData);

   /*! \brief Sets the Flatten ASCII/BINARY Log
    *
    *  \param [in] pcDecodedMessage char pointer
    *  \return "Does not return a value"
    */
   void setFlattenMessageData(CHAR *pcDecodedMessage);

   /*! \brief Sets json message string
    *
    *  \param [in] sjasonMessage json message string
    *  \return "Does not return a value"
    */
   void  setMessagejsonstring(std::string sjasonMessage);

   /*! \brief Sets json header string
    *
    *  \param [in] sjsonheader json message header
    *  \return "Does not return a value"
    */
   void setHeaderjsonstring(std::string sjsonheader);

   /*! \brief Returns the json object. */
   std::string getjsonMessageData(void);

   /*! Gets json header string */
   std::string getjsonHeaderData(void);

   /*! \brief Returns the decoded log string */
   CHAR* getMessageData(void);

   /*! \brief Returns the Flatten ASCII/BINARY Log string. */
   CHAR* getFlattenMessageData(void);

   /*! \brief Sets the decoded log length
    *
    *  \param [in] uiMessageLength unsigned int value
    *  \return "Does not return a value"
    */
   void setMessageLength(UINT uiMessageLength);

   /*! \brief Returns the decoded log length. */
   UINT getMessageLength(void) const;

   /*! \brief Sets Flatten message length
    *
    *  \param [in] uiMessageLength unsigned int value
    *  \return "Does not return a value"
    */
   void setFlattenMessageLength(UINT uiMessageLength);

   /*! \brief Returns flatten message length */
   UINT getFlattenMessageLength(void) const;

   /*! Sets the MessageDefCRC of the decoded Log. */
   void setMessageDefCrc(ULONG ulMessageDefCrc);

   /*! Returns the MessageDefCRC of the decoded log. */
   ULONG getMessageDefCrc(void) const;

   /*! \brief Returns the number of fields in NMEA message. */
   INT getNMEAMsgFieldCount(void);

   /*! Sets the Idle of the receiver. */
   void setIdleTime(DOUBLE dIdleTime);

   /*! Returns the receiver idle time. */
   DOUBLE getIdleTime(void) const;

   /*! Sets the receiver status value. */
   void setReceiverStatus(ULONG ulReceiverStatus);

   /*! Returns the receiver status value. */
   ULONG getReceiverStatus(void) const;

   /*! Sets receiver software version. */
   void setReceiverSWVersion(ULONG ulReceiverSWVersion);

   /*! Returns receiver software version. */
   ULONG getReceiverSWVersion(void) const;

   /*! Sets the recevier port from which data has been logged. */
   void setMessagePort(UCHAR ucPort);

   /*! Returns the receiver port from which data has been logged. */
   UCHAR getMessagePort(void) const;

   /*! Sets the value to TRUE if decoding data is response else False for Log data. */
   void setMessageType(BOOL bMessageType);

   /*! Check decoded data is log or response from the receiver. */
   BOOL getMessageType(void) const;

   /*! Sets the value if decoding data is response. */
   void setResponseID(INT iResponseID);

   /*! Returns the response id if decoding data is response. */
   INT getResponseID(void) const;

   /*! Set TRUE if Response is error? else False. */
   void setResponseType(BOOL bReponse);

   /*! Returns response data contains error message or not. True if error else False. */
   BOOL getResponseType(void) const;

   /*! Returns the log size. */
   virtual UINT GetMessageSize(void);

   /*! Returns the decoded json(?) obj size. */
   virtual UINT GetObjSize(void);

   /*! Copy Constructor of basemessagedata class. */
   BaseMessageData(const BaseMessageData& cBaseMessageData);

   /*! Assignment operator for basemessagedata class. */
   BaseMessageData& operator = (const BaseMessageData& cBaseMessageData);

private:
   std::string sMyMessageName;                  /**< Decoded Message Name */
   BOOL bIsMyMessageType;                       /**< Is it response or log */
   INT iMyResponseID;                           /**< Response id */
   BOOL bIsMyErrorResponse;                     /**< Is data is error message or not? */
   UINT uiMyMessageID;                          /**< Message ID */
   MessageFormatEnum eMyMessageFormat;          /**< Message Format */
   MessageTimeStatusEnum eMyMessageTimeStatus;  /**< Time Status */
   ULONG ulMyGnssWeek;                          /**< GNSS Time Week */
   ULONG ulMyMilliSeconds;                      /**< GNSS Time Milli Seconds */
   MessageAntennaSourceEnum eMyMessageSource;   /**< Message Source */
   DOUBLE dMyIdleTime;                          /**< Idle Time */
   ULONG ulMyReceiverStatus;                    /**< Receiver Status */
   ULONG ulMyReceiverSWVersion;                 /**< Receiver Software Version */
   UCHAR ucMyPortAddress;                       /**< Receiver port Address */
   UINT uiMyBinaryMessageLength;                /**< Binary Message Length */
   UINT uiMyFlattenBinaryMessageLength;         /**< Faltten Binary Message Length */
   UINT uiMyAsciiMessageLength;                 /**< Ascii Message Length */
   UINT uiMyAbbAsciiMessageLength;              /**< Abb Ascii Message Length */
   UINT uiMyUnknownMessageLength;               /**< Unknown Message Length */
   UINT uiMyRinexMessageLength;                 /**< RINEX Message Length */
   UINT uiMyNMEA2000MessageLength;              /**< NMEA2000 Message Length */
   ULONG ulMyMessageDefCrc;                     /**< Message Def CRC */
   CHAR* pcMyBinaryMessageData;                 /**< Binary Message Data */
   CHAR* pcMyAsciiMessageData;                  /**< Ascii Message Data */
   CHAR* pcMyAbbAsciiMessageData;               /**< Abb Ascii Message Data */
   CHAR* pcMyUnknownMessageData;                /**< Unknown Message Data */
   CHAR* pcMyRinexMessageData;                  /**< RINEX Message Data */
   CHAR* pcMyNMEA2000MessageData;               /**< NMEA2000 data */
   CHAR* pcMyFlattenMessageData;                /**< Binary flatten Message Data */
   std::string sMyjson;                         /**< Json string object */
   std::string sMyHeaderjson;                   /**< Json string header */
};

#endif
