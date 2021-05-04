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

/*! \file   binarytoasciicomposer.hpp
 *  \brief  Class with methods to convert Binary message to Ascii message
 */

#pragma once
//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef BINARYTOASCIICOMPOSER_H
#define BINARYTOASCIICOMPOSER_H

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"
#include "decoders/jsoninterface/api/message.hpp"
#include "decoders/jsoninterface/api/messageparams.hpp"
#include "decoders/jsoninterface/api/types.hpp"
#include "decoders/jsoninterface/api/enums.hpp"
#include "decoders/common/api/basemessagedata.hpp"
#include "messagesinfo.hpp"
#include "decoders/common/api/crc32.hpp"
#include "decoders/jsoninterface/api/loaddatafromjson.hpp"
#include "decoders/jsoninterface/api/jsonfilereader.h"
#include "encodercommon.hpp"
#include <vector>
#include <iomanip>
#include <algorithm>

/*! \class BinaryToAsciiComposer
 *  \brief Class has methods to convert messages from Binary to Ascii *
 */
class BinaryToAsciiComposer
{
public:
   /*! A constructor
    * \brief Gets the object for class BinaryToAsciiComposer.
    */
   BinaryToAsciiComposer();
   /*! A default destrutor*/
   ~BinaryToAsciiComposer();

   /*! \fn MsgConvertStatusEnum Convert(BaseMessageData *pclBaseData, CHAR* writeData_)
    *  \brief Method to convert message from binary to ascii.
    *  \param [in] pclBaseData BasemessageData class pointer
    *  \param [in] writeData_ Buffer to hold converted log during conversion
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    *  \remark If input message format is Ascii and conversion type is Ascii, This method simply returns same Ascii data.
    *  No conversion required in this case.
    */
   MsgConvertStatusEnum Convert(BaseMessageData *pclBaseData, CHAR* writeData_);

   /*! \fn MsgConvertStatusEnum FormatHeader(UCHAR *pucIOMessage, CHAR **pAsciiData)
    *  \brief Convert Binary header to Ascii header
    *
    *  \param [in] pucIOMessage Binary header pointer
    *  \param [in] pAsciiData Buffer to hold converted Ascii header
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    *
    *  \remark If port in Binary header not valid returns INVALID_MESSAGE_FORMAT definition
    *  \sa INVALID_MESSAGE_FORMAT
    */
   MsgConvertStatusEnum FormatHeader(UCHAR *pucIOMessage, CHAR **pAsciiData);

   /*! \fn MsgConvertStatusEnum BinaryParameterToASCII(const USHORT ulMessageId_, CHAR** ppcCurrentByte_, const std::vector<CMessage*>& messgeList, const std::vector<CMessageParams*>& messgeParamList, const std::vector<CTypes*>& typesList, const std::vector<CEnums*>& enumsList)
    *  \brief Method to convert Binary parameter to Ascii
    *
    *  \param [in] ulMessageId_ Message ID of the log
    *  \param [in] ppcCurrentByte_ Current position of the buffer
    *  \param [in] messgeList vector of messages
    *  \param [in] messgeParamList vector of message parameters inside it
    *  \param [in] typesList vector of message types list
    *  \param [in] enumsList vector of enums list of messgaes
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum BinaryParameterToASCII(const USHORT ulMessageId_, CHAR** ppcCurrentByte_, const std::vector<CMessage*>& messgeList, const std::vector<CMessageParams*>& messgeParamList, const std::vector<CTypes*>& typesList, const std::vector<CEnums*>& enumsList);

   /*! \fn MsgConvertStatusEnum SimpleParameterToASCII(std::string ulTypeName_, const CHAR*sConvertString_, CHAR** psCurrentByte_, ULONG uLength)
    *  \brief Method to convert parameter in a log to Ascii
    *
    *  \param [in] ulTypeName_ Message type
    *  \param [in] sConvertString_ string to hold converted value
    *  \param [in] psCurrentByte_ Current position of the buffer
    *  \param [in] uLength Length of the parameter from the current position
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum SimpleParameterToASCII(std::string ulTypeName_, const CHAR*sConvertString_, CHAR** psCurrentByte_, ULONG uLength);

   /*! \fn MsgConvertStatusEnum StringParametertoASCII(ULONG ulMaxLength_, CHAR** psCurrentByte_, ElementTypeEnum eEConvertType)
    *  \brief Method to convert string parameter to Ascii
    *
    *  \param [in] ulMaxLength_ Maximum length of the string
    *  \param [in] psCurrentByte_ Current position in the buffer
    *  \param [in] eEConvertType Convert type enumaration
    *  \sa ElementTypeEnum
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum StringParametertoASCII(ULONG ulMaxLength_, CHAR** psCurrentByte_, ElementTypeEnum eEConvertType);

   /*! \fn MsgConvertStatusEnum HexParametertoASCII(ULONG ulArraySize_, CHAR** psCurrentByte_)
    *  \brief Method to convert Hex value to Ascii
    *
    *  \param [in] ulArraySize_ Size of the parameter
    *  \param [in] psCurrentByte_ Current position in the buffer
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum HexParametertoASCII(ULONG ulArraySize_, CHAR** psCurrentByte_);

   /*! \fn MsgConvertStatusEnum PassThroughtoASCII(ULONG ulArraySize_, CHAR** psCurrentByte_)
    *  \brief Method to handle passthrough logs
    *
    *  \param [in] ulArraySize_ size of the log
    *  \param [in] psCurrentByte_ Current position in the buffer
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum PassThroughtoASCII(ULONG ulArraySize_, CHAR** psCurrentByte_);

   /*! \fn void frameGlonassSatIdandSlotFreq(CHAR **psCurrentByte_)
    *  \brief Method to frame the GLONASS slot id as it has a different signature in ASCII
    *
    *  \param [in] psCurrentByte_ Byte from which need to extract GLONASS slot id
    */
   void frameGlonassSatIdandSlotFreq(CHAR **psCurrentByte_);

private:
   /*! \var OEM4BinaryHeader *pstBinaryHeader
    * \brief structure of Binary header
    */
	OEM4BinaryHeader *pstBinaryHeader;

   /*! \var OEM4BinaryShortHeader *pstBinaryShortHeader
    * \brief structure of Binary short header
    */
	OEM4BinaryShortHeader *pstBinaryShortHeader;

   /*! \var MessageStyleEnum msgStyle
    * \brief enum of message style
    */
	MessageStyleEnum msgStyle;

   /*! \var JSONFileReader *dbData
    * \brief Pointer to hold Json reader class object
    */
	JSONFileReader *dbData;

   /*! \var UCHAR* pucMyInputData
    * \brief char pointer to hold address input data
    */
	UCHAR* pucMyInputData;

   /*! \var UCHAR* pucMyIOStart
    * \brief char pointer to hold address input data
    */
	UCHAR* pucMyIOStart;

   /*! \var CHAR cMySeparator
    * \brief char valriable to hold next separator in Ascii string
    */
	CHAR cMySeparator;

   /*! \var string m_strEnum
    * \brief string valriable to hold enum value in string
    */
	std::string m_strEnum;

   /*! \var string m_strEnum
    * \brief string valriable to hold message name
    */
	std::string strMsgName;

   /*! \var ULONG ulMaxNoOfParameters
    * \brief valriable to hold max no of parameters in a mesage definition.
    */
	ULONG ulMaxNoOfParameters;

   /*! \var ULONG ulMyParameterNumber
    * \brief valriable to hold current parameter value.
    */
	ULONG ulMyParameterNumber;

   /*! \var ULONG ulSize
    * \brief valriable to hold size of message parameter list.
    */
	ULONG ulSize;

   /*! \var ULONG ulMaxParameterNumberBin
    * \brief valriable to hold max no of parameters in a mesage definition.
    */
	ULONG ulMaxParameterNumberBin;

   /*! \var UINT uiMsgId
    * \brief valriable to hold message id.
    */
	UINT uiMsgId;

   /*! \var ULONG ulCopyParam
    * \brief valriable to hold no of parameters copied or coverted.
    */
	ULONG ulCopyParam;

   /*! \var BOOL bIsBinaryRxconfig
    * \brief Flag to check if the input is Rxconfig.
    */
	BOOL bIsBinaryRxconfig;

   /*! \var MsgConvertStatusEnum MessageStatus
    * \brief enum variable to hold convertion staus.
    */
	MsgConvertStatusEnum MessageStatus;

   /*! \var ULONG ulMyTotalClassArrayParameterNumber
    * \brief variable to hold size of class array parameters.
    */
	ULONG ulMyTotalClassArrayParameterNumber;

   /*! \var ULONG ulMyCopiedClassArrayParameterNumber
    * \brief variable to hold no of class array parameters copied.
    */
	ULONG ulMyCopiedClassArrayParameterNumber;

   /*! \var BaseMessageData *pBaseData
    * \brief Pointer to hold Base Message Data class object.
    */
	BaseMessageData *pBaseData;


	CHAR* writeData;
	BOOL bMyFlipCRC;
	INT iMyMessageLength;
	CHAR* sCurrentByte_;
	std::list<USHORT> listOfShortHeaderLogs;
	USHORT usBodyLength;
	USHORT usMsgNumber;
};


#endif // !BINARYTOASCIICOMPOSER_H

