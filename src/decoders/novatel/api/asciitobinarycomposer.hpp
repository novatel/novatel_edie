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

/*! \file   asciitobinarycomposer.hpp
 *  \brief  Class with methods to convert Ascii message to Binary message
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef ASCIITOBINARYCOMPOSER_H
#define ASCIITOBINARYCOMPOSER_H

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
#include "separator.hpp"
#include "decoders/jsoninterface/api/loaddatafromjson.hpp"
#include "decoders/jsoninterface/api/jsonfilereader.h"
#include "encodercommon.hpp"
#include <vector>
#include <iomanip>

/*! \class AsciiToBinaryComposer
 *  \brief Class has methods to convert messages from Ascii to BInary 
 */
class AsciiToBinaryComposer
{
public:
   /*! A defaulf constructor
    * \brief Gets the object for class AsciiToBinaryComposer.
    */
   AsciiToBinaryComposer();
   /*! A default destrutor*/
   ~AsciiToBinaryComposer();

   /*! \fn MsgConvertStatusEnum Convert(BaseMessageData *pclBaseData, CHAR* writeData_, BOOL bIsRxConfig = FALSE)
    *  \brief Method to convert Ascii message to binary.
    *
    *  \param [in] pclBaseData BasemessageData class pointer
    *  \param [in] writeData_ Buffer to hold converted log during conversion
    *  \param [in] bIsRxConfig Boolean variable to handle RXCONFIG log.
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    *
    *  \remark If input message format is binray and conversion type is binary, This method simply returns same Ascii data.
    *  No conversion required in this case.
    */
   MsgConvertStatusEnum Convert(BaseMessageData *pclBaseData, CHAR* writeData_,BOOL bIsRxConfig = FALSE);

   /*! \fn MsgConvertStatusEnum ExtractHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CommunicationPortEnum ePort_)
    *  \brief Convert Ascii header to Binary header
    *
    *  \param [in] psCurrentByte_ Current pointer in buffer
    *  \param [in] pcEnd_ End pointer of buffer
    *  \param [in] ePort_ Port of the receiver
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    *
    *  \remark If port in Binary header not valid returns INVALID_MESSAGE_FORMAT definition
    *  \sa INVALID_MESSAGE_FORMAT
    */
   MsgConvertStatusEnum ExtractHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CommunicationPortEnum ePort_ = MSGCVT_COMPORT1);

   /*! \fn MsgConvertStatusEnum GetMessageId(std::string sMessageName_, ULONG* pulMessageId_)
    *  \brief Gets the Message id from the name of the message
    *
    *  \param [in] sMessageName_ Message Name
    *  \param [in] pulMessageId_ Hold the essage id value(passby reference)
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum GetMessageId(std::string sMessageName_, ULONG* pulMessageId_);

   /*! \fn MsgConvertStatusEnum ExtractGeneralHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CHAR* sNextSeparator_, CommunicationPortEnum ePort_ = MSGCVT_COMPORT1)
    *  \brief Extract the general header from the buffer provided
    *
    *  \param [in] psCurrentByte_ Current pointer of the buffer
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] sNextSeparator_ seperator in the message
    *  \param [in] ePort_ Communication port of the receiver
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum ExtractGeneralHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CHAR* sNextSeparator_, CommunicationPortEnum ePort_ = MSGCVT_COMPORT1);

   /*! \fn MsgConvertStatusEnum ExtractCompressedHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CHAR* sNextSeparator_, CommunicationPortEnum ePort_ = MSGCVT_COMPORT1)
    *  \brief Extract the compressed log header from the buffer provided
    *
    *  \param [in] psCurrentByte_ Current pointer of the buffer
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] sNextSeparator_ seperator in the message
    *  \param [in] ePort_ Communication port of the receiver
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum ExtractCompressedHeader(CHAR** psCurrentByte_, const CHAR* pcEnd_, CHAR* sNextSeparator_, CommunicationPortEnum ePort_ = MSGCVT_COMPORT1);

   /*! \fn MsgConvertStatusEnum ASCIIParameterToBinary(const ULONG ulMessageId_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR** ppucIOMessageBody_, const std::vector<CMessage*>& messgeList, const std::vector<CMessageParams*>& messgeParamList, const std::vector<CTypes*>& typesList, const std::vector<CEnums*>& enumsList)
    *  \brief Convert Ascii paramer in a log to Binary
    *
    *  \param [in] ulMessageId_ Message ID
    *  \param [in] psCurrentByte_ Current pointer of the buffer
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] ppucIOMessageBody_ Buffer which contans header
    *  \param [in] messgeList vector of messages
    *  \param [in] messgeParamList vector of message parameters in a log
    *  \param [in] typesList vector of types in a log
    *  \param [in] enumsList vector of enums in a log
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum ASCIIParameterToBinary(const ULONG ulMessageId_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR** ppucIOMessageBody_, const std::vector<CMessage*>& messgeList, const std::vector<CMessageParams*>& messgeParamList, const std::vector<CTypes*>& typesList, const std::vector<CEnums*>& enumsList);

   /*! \fn MsgConvertStatusEnum ExtractArraySize(ElementTypeEnum eElementType_, CHAR** psCurrentByte_, const CHAR* pcEnd_, ULONG* pulArraySize, ULONG ulMaxArraySize_)
    *  \brief Extract the size of the array which contains elements
    *
    *  \param [in] eElementType_ The type of the element to be find in array
    *  \param [in] psCurrentByte_ Current pointer of buffer
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] pulArraySize Holds the size of the array
    *  \param [in] ulMaxArraySize_ Maximum size of the array
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum ExtractArraySize(ElementTypeEnum eElementType_, CHAR** psCurrentByte_, const CHAR* pcEnd_, ULONG* pulArraySize, ULONG ulMaxArraySize_);

   /*! \fn MsgConvertStatusEnum ExtractStringLength(ULONG ulMaxSize_, CHAR* pcStart_, const CHAR* pcEnd_, ULONG* ulArraySize_)
    *  \brief Extract the string length
    *
    *  \param [in] ulMaxSize_ Maximum size of the string can be check
    *  \param [in] pcStart_ Starting of the string
    *  \param [in] pcEnd_ End of the string
    *  \param [in] ulArraySize_ Holds the size of the string
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum ExtractStringLength(ULONG ulMaxSize_, CHAR* pcStart_, const CHAR* pcEnd_, ULONG* ulArraySize_);

   /*! \fn MsgConvertStatusEnum StringParametertoBinary(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_, BOOL bNullEnding_);
    *  \brief Method to convert string to binary
    *
    *  \param [in] ulArraySize_ Size of the string
    *  \param [in] psCurrentByte_ String of string
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] pucIOMessageBody_ Buffer contains the string
    *  \param [in] bNullEnding_ Boolean variable to check Null terminated string or not
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum StringParametertoBinary(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_, BOOL bNullEnding_);

   /*! \fn MsgConvertStatusEnum HexParametertoBinary(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_)
    *  \brief Method to convert HEX value to binary
    *
    *  \param [in] ulArraySize_ size of the buffer
    *  \param [in] psCurrentByte_ String of string
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] pucIOMessageBody_ Buffer contains the string
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum HexParametertoBinary(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_);

   /*! \fn MsgConvertStatusEnum ASCIItoPassthrough(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_)
    *  \brief Method to handle Ascii passthrough logs
    *
    *  \param [in] ulArraySize_ size of the buffer
    *  \param [in] psCurrentByte_ String of string
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] pucIOMessageBody_ Buffer contains the string
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum ASCIItoPassthrough(ULONG ulArraySize_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR* pucIOMessageBody_);

   /*! \fn MsgConvertStatusEnum EnumParameterToBinary(CHAR** psCurrentByte_, const CHAR* pcEnd_)
    *  \brief Method to convert enumaration value to binary
    *
    *  \param [in] psCurrentByte_ String of string
    *  \param [in] pcEnd_ End of the buffer
    *
    *  \return Conversion status enumaration
    *  \sa MsgConvertStatusEnum
    */
   MsgConvertStatusEnum EnumParameterToBinary(CHAR** psCurrentByte_, const CHAR* pcEnd_);

   /*! \fn MsgConvertStatusEnum SimpleParameterToBinary(ULONG ulElementLength_, const CHAR* sConvertString_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR** ppucIOMessageBody_, std::string szTypeName)
    *  \brief Method to convert parameter in buffer to binary
    *
    *  \param [in] ulElementLength_ Length of the parameter in buffer to be converted
    *  \param [in] sConvertString_ Holds the converted binary string
    *  \param [in] psCurrentByte_ String of string
    *  \param [in] pcEnd_ End of the buffer
    *  \param [in] ppucIOMessageBody_ Buffer contains the string
    *  \param [in] szTypeName Type of the element to be convert
    *  \return
    *
    *  \remark
    */
   MsgConvertStatusEnum SimpleParameterToBinary(ULONG ulElementLength_, const CHAR* sConvertString_, CHAR** psCurrentByte_, const CHAR* pcEnd_, UCHAR** ppucIOMessageBody_, std::string szTypeName);

   /*! \fn UCHAR setComPort(CommunicationPortEnum ePort_, UINT uiVirtualAddress_)
    *  \brief Set the receiver communication port
    *
    *  \param [in]  ePort_ Communiction port enumaration value
    *  \sa CommunicationPortEnum
    *  \param [in] uiVirtualAddress_ Virtual port address to be OR'ing with given port
    *
    *  \return Assigned Port number of the receiver
    */
   UCHAR setComPort(CommunicationPortEnum ePort_, UINT uiVirtualAddress_);

   /*! \fn BOOL IsResponseMessage(CHAR** psStart_)
    *  \brief Method to check data is response or normal log
    *
    *  \param [in] psStart_ Starting address of the buffer to be check
    *
    *  \return TRUE or FALSE
    *
    *  \remark Returns TRUE if data is response of the log, else returns FALSE
    */
   BOOL IsResponseMessage(CHAR** psStart_);

private:
   /*! \var MsgConvertStatusEnum MessageStatus
    * \brief enum variable to hold convertion staus.
    */
   MsgConvertStatusEnum MessageStatus;

   /*! \var BaseMessageData *pBaseData
    * \brief Pointer to hold Base Message Data class object.
    */
   BaseMessageData *pBaseData;

   /*! \var CommunicationPortEnum eMyPort
    * \brief enum variable to hold Communication Port.
    */
   CommunicationPortEnum eMyPort;

   /*! \var Format eMyFormat
    * \brief enum variable to hold mesage formate type.
    */
   Format eMyFormat;

   /*! \var Separator clMySeparator
    * \brief Separator class object.
    */
   Separator clMySeparator;

   /*! \var SeparatorEnum eMySeparatorStatus
    * \brief enum to hold Separator type.
    */
   SeparatorEnum eMySeparatorStatus;

   /*! \var OEM4BinaryHeader *pstBinaryHeader
    * \brief structure of Binary header
    */
   OEM4BinaryHeader pstBinaryHeader;

   /*! \var OEM4BinaryShortHeader *pstBinaryShortHeader
    * \brief structure of Binary short header
    */
   OEM4BinaryShortHeader pstBinaryShortHeader;

   /*! \var MessageStyleEnum eMyMessageStyle
    * \brief enum of message style
    */
   MessageStyleEnum eMyMessageStyle;

   /*! \var JSONFileReader *dbData
    * \brief Pointer to hold Json reader class object
    */
   JSONFileReader *dbData;

   /*! \var map<std::string, INT> timeStatus
    * \brief map of time status name-value as key-value pair
    */
   std::map<std::string, INT> timeStatus;

   CHAR* writeData;
   CHAR* pucMyInputData;
   CHAR* szMyLastParameter;
   CHAR* sNextSeparator;
   UCHAR *pucMyBufferEnd;
   UCHAR* pucIOMessageBody;
   BOOL bMyFlipCRC;
   BOOL bStatus;
   BOOL bLength;
   ULONG ulMyParameterNumber;
   ULONG ulMyTotalClassArrayParameterNumber;
   ULONG ulMyCopiedClassArrayParameterNumber;
   ULONG ulMyFieldNumber;
   ULONG ulSize;
   INT iMessageId;
   ULONG ulCopyParam;
   std::string strEnumName;
};

#endif // ASCIITOBINARYCOMPOSER_H
