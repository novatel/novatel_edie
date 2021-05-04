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
#ifndef NOVATELPARSER_H
#define NOVATELPARSER_H

/*! \file novatelparser.hpp
 *  \brief Class to read from port/file or memory buffer and parse the novatel message data
 *
 *  Class will start parsing the data in the buffer by looking for valid sync bytes and
 *  then determining the format of the message such as ASCII, Binary etc.
 *
 *  For ASCII messages the end of the message is delimited by a Carriage return [CR] and line feed [LF].
 *  After reading the complete ASCII message a 32-bit CRC will be calculated and compared against
 *  the one embedded in the message. If the CRC is valid the decoder will pass the message to the application.
 *
 *  For Binary messages the header will be parsed first to get the message length.
 *  The CRC will then be calculated on the buffer of message length bytes and validated against the provided CRC.
 *  If the CRC is valid the decoder will pass the message to the application.
 *
 *  For NMEA messages the end of the message is delimited by a line feed. After reading the complete message checksum will be calculated and compared against the one embedded in the message.
 *  If the checksum matches the decoder will pass the message to the application.
 *
 *  For now, decoder only supports ASCII, Binary NovAtel messages and NMEA messages.
 *
 */

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/circularbuffer.hpp"
// Open Source Decoder Changes - Need to replace the methods from JSON interface
//#include "messagesinfo.hpp"
#include "hw_interface/stream_interface/api/inputstreaminterface.hpp"
#include "decoders/common/api/crc32.hpp"
// Open Source Decoder Changes - Need to replace the methods from JSON interface
//#include "enumstable.hpp"
#include "decoders/common/api/stringtotypes.hpp"
#include <cstring>
#include <sstream>
#include <vector>

/*! \def READ_BUFFER_SIZE
    \brief A macro that returns the maximum of internal buffer size from port/file or memory buffer
*/
#define READ_BUFFER_SIZE 10240

/*! \class NovatelParser
 *  \brief Provides API's to Parse the data read from port/file or memory buffer.
 *
 */
class NovatelParser
{
private:
   /*! \var InputStreamInterface* pclMyInputStreamInterface
    *  \brief InputStreamInterace object pointer
    *  \sa InputStremInterface
    */
   InputStreamInterface* pclMyInputStreamInterface;
   /*! \var CircularBuffer cMyCircularDataBuffer
    *  \brief Circular Buffer to hold data read from stream
    *  \sa CircularBuffer
    */
   CircularBuffer cMyCircularDataBuffer;
   /*! \var MessageParseStatusEnum eMyParseStatus
    *  \brief Variable to use for reperenting state in parsing stae machine
    *  \sa MessageParseStatusEnum
    */
   MessageParseStatusEnum eMyParseStatus;
   /*! \var StreamReadStatus eMyStreamReadStatus
    *  \brief Statistics after reading data from port/file or memory buffer
    *  \sa StreamReadStatus
    */
   StreamReadStatus eMyStreamReadStatus;
   /*! \var BOOL bIsInputStreamEOF
    *  \brief Boolean variable will check for reading data reached END or not.
    *  If FALSE, user can continue to read else can terminate decoding.
    */
   BOOL bIsInputStreamEOF;
   /*! \var UINT uiMyByteCount
    *  \brief Byte count using during parsing.
    *  Will be use full to check reading of complet ASCII/Binary messgae
    */
   UINT uiMyByteCount;
   /*! \var BOOL bMySkipCrcCheck
    *  \brief Boolen variable to skip CRC validation on ASCII/Binary messages.
    *  disabled by default.
    */
   BOOL bMySkipCrcCheck;
   /*! \var std::vector<CHAR> vcMyUnknownBytes
    *  \brief vector to hold unknown bytes while parsing data.
    */
   std::vector<CHAR> vcMyUnknownBytes;
   /*! \var std::map<std::string, MessageTimeStatusEnum> mTimeStatusMap
    *  \brief Map to convert Time status string to enum. Contians map of GPS time status string and associated ENUM as key value par.
    */
   std::map<std::string, MessageTimeStatusEnum> mTimeStatusMap;
   /*! \fn BOOL ExtractAsciiHeader(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader)
    *  \brief Method to extract ascii header parameters from ascii data frame
    *  \param [in] pcMessageBuffer Ascii message data pointer
    *  \param [in] stMessageHeader Generic message header to fill after decoding ascii message
    *  \sa MessageHeader
    *
    *  \return TRUE or FALSE. Returns TRUE , If we received all header parameters (excluding sync byte) else FALSE.
    *  Returns TRUE if message name is valid, else returns FALSE.
    */
   BOOL ExtractAsciiHeader(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader);
   /*! \fn BOOL ExtractShortAsciiHeader(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader)
    *  \brief Method to extract ascii header parameters from ascii data frame
    *  \param [in] pcMessageBuffer Ascii message data pointer
    *  \param [in] stMessageHeader Generic message header to fill after decoding ascii message
    *  \sa MessageHeader
    *
    *  \return TRUE or FALSE.
    *  Returns TRUE , If we received all header parameters (excluding sync byte) else FALSE.
    *  Returns TRUE if message name is valid, else returns FALSE.
    */
   BOOL ExtractShortAsciiHeader(CHAR* pcMessageBuffer, MessageHeader* stMessageHeader);

   /*! \fn BOOL CalculateBlockCrc(UINT uiNumberOfBytes)
    *  \brief Method to calculate CRC on block
    *  \param [in] uiNumberOfBytes Number of bytes used in the buffer to calculate CRC
    *
    *  \return TRUE or FALSE.
    *          Retursn TRUE, If CRC Check Skip enabled.
    *          Returns FALSE, If number of bytes u the buffer not match with OEM4_ASCII_CRC_LENGTH + 3(#, CR & LF)
    *          Returns TRUE, If CRC in message matches with computed CRC on data. esle retursn FALSE.
    *  \sa OEM4_ASCII_CRC_LENGTH
    */
   BOOL CalculateBlockCrc(UINT uiNumberOfBytes);

   /*! \fn UINT CalculateCharacterCrc(UCHAR uCharacter, UINT uiCrcCalculated)
    *  \brief Method to calculate CRC character by character
    *  \param [in] uCharacter Character to compute CRC
    *  \param [in] uiCrcCalculated CRC computed so far
    *
    *  \return Retursn 0, If CRC Check Skip enabled
    *          Else returns calculates the CRC-32 of a block of data one character for each call
    */
   UINT CalculateCharacterCrc(UCHAR uCharacter, UINT uiCrcCalculated);

   /*! \fn BOOL ValidateNMEAChecksum( UINT uiNumberOfBytes)
    *  \brief Method to calculate and validate NMEA Message checksum
    *  \param [in] uiNumberOfBytes Number of bytes used in the buffer to calculate CRC
    *
    *  \return TRUE or FALSE.
    *          Returns FALSE, If number of bytes u the buffer not match with NMEA_MESSAGE_CRC_LENGTH + 3(#, CR & LF  )
    *          Returns TRUE, If CRC in message matches with computed CRC on data. esle retursn FALSE.
    */
   BOOL ValidateNMEAChecksum( UINT uiNumberOfBytes);

   /*! \fn  INT HexToInt(CHAR cValue)
    *  \brief Method to convert hex number to integer number
    *  \param [in] cValue HEX value to be converted to Integer
    *
    *  \return Converted Integer value from Hex value
    *
    */
   INT HexToInt(CHAR cValue);

   /*! \fn  UINT ReadInputStream(void)
    *  \brief Method to read data from input stream(port/file or memory buffer)
    *
    *  \details Read data from input stream and append the data read from file to praser buffer.
    *  If Input file size is less then READ_BUFFER_SIZE, then process only those bytes.
    *  And append, only non callback data in case of port. Converted Integer value from Hex value 
    *
    *  \sa READ_BUFFER_SIZE
    */
   UINT ReadInputStream(void);

   /*! \fn void HandleInvalidData(void)
    *  \brief Handles invalid data in the buffer.
    *
    *  \details Sets parsing state machine to WAITING_FOR_SYNC. Push back invalid byte to unknown data vector.
    *  Discard one byte from buffer. Set number of bytes decoded to 0.
    *  \sa MessageParseStatusEnum
    */
   void HandleInvalidData(void);

public:
   /*! A constructor
    *
    * \param [in] pclInputStreamInterface Input Stream pointer, Could be File/Port or buffer.
    * \details throw exception if input is not valid.
    *  Default skipping CRC validation on messages to FALSE. Clear the unknown bytes vector.
    *  Prepare GPS Time Status string to enum map
    */
   NovatelParser(InputStreamInterface* pclInputStreamInterface);

   /*! \fn StreamReadStatus  ParseData(CHAR** pcMessageBuffer, MessageHeader* stMessageHeader)
    * \brief Method to parse data
    * \param [in] pcMessageBuffer Message pointer to parse a frame and store
    * \param [in] stMessageHeader Header structure to fill for above decoded buffer.
    *
    * \details  Store unknwon bytes till next valid sync or 10kb whichever is earlier. And handle uknown byes as well.
    * Set the default parsing status to WAITING_FOR_SYNC. Read data from file or port when circular buffer reach end
    * or we didn't find complete frame in current data buffer.
    *
    * Extract valid ASCII/Binary and NMEA messages from the input.
    */
   StreamReadStatus ParseData(CHAR** pcMessageBuffer, MessageHeader* stMessageHeader);

   /*! Default destructor. */
   ~NovatelParser();

   /*! \fn  CircularBuffer* getCircularBuffer(void)
    * \brief Method to get Circular Buffer which contains the data read from file/port or memory input
    * \return CircularBuffer object pointer
    */
   CircularBuffer* getCircularBuffer(void);

   /*! \fn void SetCrcCheckFlag(BOOL bEnable)
    * \brief Method to set CRC Check Flag to enable or disable CRC validation on messages.
    * \param [in] bEnable Boolean variable to enable or disable. If TRUE CRC vaidation can be skip.
    */
   void SetCrcCheckFlag(BOOL bEnable);

   /*! \fn StreamReadStatus* getStreamReadStatus(void)
    * \brief Returns the status of data reading.
    * \sa StreamReadStatus
    * \return StreamReadStatus pointer with read data statistics.
    */
   StreamReadStatus* getStreamReadStatus(void);

   /*! \fn void Reset()
    * \brief Clears Circullar buffer which will be used for storing read data from input.
    * EOS(End Of Stream) to FALS.
    */
   void Reset();
};
#endif // NOVATELPARSER_H
