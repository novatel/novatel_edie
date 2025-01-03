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

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef FRAMER_H
#define FRAMER_H

/*! \file framer.hpp
 *  \brief Class to handle all the functionality related to decoding a NovAtel message.
 *  This involves identifying the message sync framing the complete message and validating
 *  the CRC before passing the message to the application.
 *
 */

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <memory>
#include "novatelparser.hpp"
#include "decoders/common/api/messagecounter.hpp"
#include "decoders/common/api/basemessagedata.hpp"
#include "filters/messagedatafilter.hpp"
#include "decoders/common/api/unknowndatahandler.hpp"
#include "lib/nlohmann/json.hpp"

/*! Json library to be used to decode json file*/
using json = nlohmann::json;

/*! \class Framer
 *  \brief Framer will handle all the functionality related to decoding a NovAtel message.
 *  This involves identifying the message sync framing the complete message and validating
 *  the CRC before passing the message to the application.
 *
 */
class Framer
{
private:
   /*! \var  std::unique_ptr<NovatelParser> pclMyNovatelParser
    * \brief NovatelParser Class unique Pointer 
    */
   std::unique_ptr<NovatelParser> pclMyNovatelParser;
   /*! \var std::unique_ptr<MessageCounter> pclMyMessageCounter
    * \brief MessageCounter Class unique Pointer 
    */
   std::unique_ptr<MessageCounter> pclMyMessageCounter;
   /*! \var MessageDataFilter* pclMyMessageDataFilter
    * \brief MessageDataFilter Class Pointer 
    */
   MessageDataFilter* pclMyMessageDataFilter;
   /*! \var std::unique_ptr<UnknownDataHandler> pclMyUnknownDataHandler
    * \brief UnknownDataHandler Class unique Pointer 
   */
   std::unique_ptr<UnknownDataHandler> pclMyUnknownDataHandler;
   /*! \var InputStreamInterface* pclMyInputStreamInterface 
    * \brief InputStreamInterface Class Pointer 
    */
   InputStreamInterface* pclMyInputStreamInterface;
   /*! \var bMyEnableUnknownData
    *  \brief Boolean variable for enable or disable unknown data parsing/handling.
    *  By default this was Enable.
    */
   BOOL bMyEnableUnknownData;
   /*! \var bMyNonBlockingMode
    *  \brief Boolean variable for enable or disable non blocking mode when data reading from port.
    *  By default this was Disable.
    */
   // Disabled by default
   BOOL bMyNonBlockingMode;
   /*! \fn void Header_json(json& jHeader, MessageHeader* pstMessageHeader)
    *  \brief Frame Json header from given MessageHeader structure
    *  \param [in] jHeader Json header to be filled.
    *  \sa json
    *  \param [in] pstMessageHeader MessageHeader structure pointer
    *  \sa MessageHeader
    *
    */
   void Header_json(json& jHeader, MessageHeader* pstMessageHeader);

   /*! \var eMyBMDOutputFormate
	*  \brief ENUM variable to fill BMD with requred output.
	*  By default this was BOTH.
	*/
   BMDOutputFormat eMyBMDOutputFormate;
public:
   /*! A constructor with out filter
    * \brief Instantiate NovatelParser, MessageCounter and UnknownDataHandler classes.
    * Enable UnknownDataHandler
    * Disable Non Blocking mode of port reading.
    * Initialize MessageDataFilter object to NULL.
    * And save InputStreamInterface class object provided as input.
    *
    * \param [in] pclInputStreamInterface Input Stream interface (port/file or buffer)
    */
   Framer(InputStreamInterface* pclInputStreamInterface);
   /*! A constructor with filter
    * \brief Instantiate NovatelParser, MessageCounter and UnknownDataHandler classes.
    * Enable UnknownDataHandler
    * Disable Non Blocking mode of port reading.
    * Save MessageDataFilter and  InputStreamInterface class objects with input provided accordingly.
    *
    * \param [in] pclInputStreamInterface Input Stream interface (port/file or buffer)
    * \param [in] rMessageDataFilter MessageDataFilter object
    */
   Framer(InputStreamInterface* pclInputStreamInterface, MessageDataFilter& rMessageDataFilter);
   /*! Default constructor */
   Framer();

   /*! \fn void EnableUnknownData(BOOL bEnable)
    * \brief Method to enable unknown messages
    * \param [in] bEnable (TRUE/FALSE)
    * \details If Enable Unknown data will be parsed by decoder else not.
    */
   void EnableUnknownData(BOOL bEnable);

   /*! \fn SetBMDOutput(BMDOutputFormate eMyBMDoutput)
	* \brief Method to set BMD output format
	* \param [in] BMDOutputFormate
	* \.
	*/
   void SetBMDOutput(BMDOutputFormat eMyBMDoutput);

   /*! \fn GetBMDOutput(BMDOutputFormat eMyBMDoutput)
	* \brief Method to get BMD output format
	* \
	* \.
	*/
   BMDOutputFormat GetBMDOutput(void);

   /*! \fn void SkipCRCValidation(BOOL bEnable)
    * \brief Method to enable CRC Check Skip
    * \param [in] bEnable (TRUE/FALSE)
    * \details If Enable CRC validation will not be considered by parser.
    * If Disable CRC validation will consider.
    */
   void SkipCRCValidation(BOOL bEnable);

   /*! \fn void EnableNonBlockingMode(BOOL bEnable)
    * \brief Method to make decoder non-blocking
    * \param [in] bEnable (TRUE/FALSE)
    * \details If TRUE, non blocking mode will be enabled while reading data from port input.
    * If FALSE, non blocking mode will be disabled for port input.
    */
   //
   void EnableNonBlockingMode(BOOL bEnable);

   /*! \fn void EnableTimeIssueFix(BOOL bEnable);
    * \brief Method to apply time issue fix with IONUTC and QZSSIONUTC logs
    * \param [in] bEnable (TRUE/FALSE)
    * \details If True, Time issue fix will be applied for IONUTC and QZSSIONUTC logs
    */
   void EnableTimeIssueFix(BOOL bEnable);

   /*! \fn StreamReadStatus ReadMessage(BaseMessageData** pclBaseMessageData)
    * \brief Method to read the next message from file/port or buffer.
    * \param [in] pclBaseMessageData
    * \sa BaseMessageData
    * \return Read statistics (StreamReadStatus)
    * \sa StreamReadStatus
    * \details Reading data from buffer and identifying the message sync framing
    *  with complete message and validating the CRC before passing the message to the application
    *  as BaseMessageData object.
    */
   StreamReadStatus ReadMessage(BaseMessageData** pclBaseMessageData);

   /*! A virtual method
    * \fn void GenerateBaseMessageData(BaseMessageData** pclBaseMessageData,
    *                                  MessageHeader* pstMessageHeader,
    *                                  CHAR* pcData)
    * \brief Method to generate BaseMessageData object with decoded log.
    * \param [in] pclBaseMessageData BaseMessageData object contains decoded log
    * \param [in] pstMessageHeader Decoded log details
    * \param [in] pcData Decoded data
    *
    */
   // This function will generate basemessagedata object
   virtual void GenerateBaseMessageData(
      BaseMessageData** pclBaseMessageData, 
      MessageHeader* pstMessageHeader, 
      CHAR* pcData);

   /*! \fn std::map<std::string, MessageInfo> GetAsciiMessageStatistics() const
    * \brief  This function will return ascii message statistics map
    *
    * \return Map with message name and MessageInfo structure as key-value pair.
    * \sa MessageInfo
    *
    */
   std::map<std::string, MessageInfo> GetAsciiMessageStatistics() const;

   /*! \fn std::map<USHORT, MessageInfo> GetBinaryMessageStatistics() const
    * \brief  This function will return binary message statistics map
    *
    * \return Map with message id and MessageInfo structure as key-value pair.
    * \sa MessageInfo
    *
    */
   std::map<UINT, MessageInfo> GetBinaryMessageStatistics() const;

   /*! \fn std::map<std::string, MessageInfo> GetAsciiMessageStatisticsWithoutFilter() const
    * \brief  This function will return ascii message statistics map with out filtering
    *
    * \return Map with message name and MessageInfo structure as key-value pair.
    * \sa MessageInfo
    *
    */
   // This function will return ascii message statistics map
   std::map<std::string, MessageInfo> GetAsciiMessageStatisticsWithoutFilter() const;

   /*! \fn std::map<USHORT, MessageInfo> GetBinaryMessageStatisticsWithoutFilter() const
    * \brief  This function will return binary message statistics map with out filtering
    *
    * \return Map with message id and MessageInfo structure as key-value pair.
    * \sa MessageInfo
    *
    */
   std::map<UINT, MessageInfo>  GetBinaryMessageStatisticsWithoutFilter() const;

   /*! \fn DecoderStatistics GetDecoderStatistics(void) const
    * \brief This function will return decoder statistics
    *
    * \details This method will provide decoder statistics at any time while decoding or after.
    * \sa DecoderStatistics
    */
   DecoderStatistics GetDecoderStatistics(void) const;

   /*! \fn UnknownDataStatistics GetUnknownDataStatistics(void) const
    * \brief This function will return unknown bytes statistics
    *
    * \details This method will provide unknwon bytes statistics if any in the data from port/file or buffer.
    * \sa UnknownDataStatistics
    */
   UnknownDataStatistics GetUnknownDataStatistics(void) const;

   /*! \fn void Reset(std::streamoff = 0, std::ios_base::seekdir = std::ios::beg)
    * \brief This method will reset NovatelParser, MessageDataFilter, MessageCounter and UnknownDataHandler.
    * Also seeks the file pointer with provided offset and direction.
    *
    * \param [in] offset File offset to which need to set for next read
    * \param [in] dir Direction from starting to End or End to starting.
    */
   void Reset(std::streamoff offset = 0, std::ios_base::seekdir dir = std::ios::beg);

   //virtual void
   //EnableForRinexConversionOnly(BOOL bFlag){};

   //virtual void
   //EnableForRinexConversionOnly(BOOL){};

   /*! \fn ULONGLONG GetCurrentFileOffset(void) const
    * \brief Returns the file offset from which next read will happen.
    * \return Current File pointer
    */
   ULONGLONG GetCurrentFileOffset(void) const;
   
   /*! Default destructor */
   ~Framer();
};
#endif // FRAMER_H
