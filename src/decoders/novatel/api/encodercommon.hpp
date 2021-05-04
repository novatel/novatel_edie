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

/*! \file   encodercommon.hpp
 *  \brief  Header file with methods to use in entire conversion logic in encoder.
 *
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef ENCODERCOMMON_HPP
#define ENCODERCOMMON_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"
#include "messagesinfo.hpp"

/*! \class EncoderCommon
 *  \brief Common methods which will be used accross encoder
 *
 */
class EncoderCommon
{
public:
   /*! A satic inline method
	* \fn ULONG Memfunc_MemAlign(UCHAR** pszTargetAddress_, ULONG ulLength_)
	* \brief Returns computed memory alignment adjustment value
	*
	* \param [in] pszTargetAddress_
	* \param [in] ulLength_
	*
	* \return Compute adjustment value
	*/
   static inline ULONG Memfunc_MemAlign(UCHAR** pszTargetAddress_, ULONG ulLength_)
   {
      return Memfunc_MemAlign((ULONG*)pszTargetAddress_, ulLength_);
   }

   /*! A satic method
	* \fn ULONG Memfunc_MemAlign(ULONG* pulTargetAddress_, ULONG ulLength_)
	* \brief Aligining the buffres properly with the length
	*
	* \param [in] pulTargetAddress_
	* \param [in] ulLength_
	*
	* \return Compute adjustment value
	*/
   static ULONG Memfunc_MemAlign(ULONG* pulTargetAddress_, ULONG ulLength_)
   {
      INT iAlignment;
      if (ulLength_ > COMPILER_BYTE_ALIGNMENT)
         iAlignment = COMPILER_BYTE_ALIGNMENT;
      else
         iAlignment = ulLength_;

      //Compute Adjustment
      INT iAdjustment = ((*pulTargetAddress_) % iAlignment);
      if (iAdjustment != 0)
      {
         iAdjustment = iAlignment - iAdjustment;
         //Move the target address over that many bytes
         *pulTargetAddress_ += iAdjustment;
      }
      return(iAdjustment);
   }
   /*! A static method
    * \fn std::string getTimeStatus(UINT ulValue)
    * \brief Convert GPS time status integer value to string.
    * \param [in] ulValue GPS time status value
    * \return GPS time status as string
    */
   static std::string getTimeStatus(UINT ulValue)
   {
      if (ulValue == 20)       return "UNKNOWN";
      else if (ulValue == 60)  return  "APPROXIMATE";
      else if (ulValue == 80)  return "COARSEADJUSTING";
      else if (ulValue == 100) return "COARSE";
      else if (ulValue == 120) return "COARSESTEERING";
      else if (ulValue == 130) return "FREEWHEELING";
      else if (ulValue == 140) return "FINEADJUSTING";
      else if (ulValue == 160) return "FINE";
      else if (ulValue == 170) return "FINEBACKUPSTEERING";

      else if (ulValue == 180) return "FINESTEERING";
      else if (ulValue == 200) return "SATTIME";
      else return "ERROR";
   }
   /*! A static method
    * \fn MsgTypeFormat GetMsgFormat(ULONG ulMsgID)
    * \brief Returns Message type format from given message ID
    * \param [in] ulMsgID Message Id
    * \return MsgTypeFormat It could be Ascii/Binary...etc
    * \sa MsgTypeFormat
    */
   static MsgTypeFormat GetMsgFormat(ULONG ulMsgID)
   {
      return ((MsgTypeFormat)((ulMsgID & MSG_FORMAT_MASK) >> MSG_FORMAT_POSITION));
   }

   /*! A static method
    * \fn MsgConvertStatusEnum GetMessageNameForData(CHAR cMsgType, const ULONG ulMessageId_, CHAR* sMessageName_)
    * \brief Returns Message name ending with A or A_1 for Ascii and B for binary
    *
    * \param [in] cMsgType Message Type ex: '1'. Could be antenna source Primary or secondary
    * \param [in] ulMessageId_ Message ID
    * \param [in] sMessageName_ Actual message name
    * \return message name appended with type A/A_1 or B and Convert status enumaration
    * \sa MsgConvertStatusEnum
    */
   static MsgConvertStatusEnum GetMessageNameForData(CHAR cMsgType, const ULONG ulMessageId_, CHAR* sMessageName_)
   {
      std::string strMsgName = GetMessageNameByID(ulMessageId_ & 0xFFFF);

      switch (GetMsgFormat(ulMessageId_))
      {
      case MASCII:
      if (cMsgType == '1')
      {
         strMsgName.append("A_1");
      }
      else
      {
         strMsgName.append("A");
      }
      break;
      case MBINARY:
         strMsgName.append("B");
         break;
      default:
      break;
      }
      //strcpy(sMessageName_, strMsgName.c_str());
		sMessageName_ += sprintf(sMessageName_, "%s", strMsgName.c_str());
      return GOOD_MESSAGE;
   }

   /*! A static method
    * \fn BOOL isResponse(CHAR cMsgType)
    * \brief Returns message is response message or log data
    *
    * \param [in] cMsgType Message type variable
    * \return TRUE or FALSE
    * Returns TRUE if it is Response message
    * Returns FALSE if it is not reponse message
    */
   static BOOL isResponse(CHAR cMsgType)
   {
      if (cMsgType & MSG_RESPONSE_MASK)
         return (true);
      else
         return(false);
   }

   /*! A static method
    * \fn UINT getPort(UCHAR ucPortAddress)
    * \brief Extract and return port number from given port address value
    *
    * \param [in] ucPortAddress Port address from which the data has been logged
    * \return Port number
    */
   static UINT getPort(UCHAR ucPortAddress)
   {
      return((ucPortAddress & PORT_MASK) >> PORT_POSITION);
   }

   /*! A static method
    * \fn UINT getVirtualPortAddress(UCHAR ucPortAddress)
    * \brief Extract and return vitual prot number from given port address value
    *
    * \param [in] ucPortAddress Port address from which the data has been logged
    * \return Virtual Port number
    */
   static UINT getVirtualPortAddress(UCHAR ucPortAddress)
   {
      return((UINT)(ucPortAddress & VIRTUAL_ADDRESS_MASK));
   }

   /*! A static method
    * \fn void frameGlonassSatIdandSlotFreqValue(CHAR **psCurrentByte_, USHORT iValue, USHORT iValue1)
    * \brief Frame Glonass stellite ID and Slot frequency byte from given values
    *
    * \param [in] psCurrentByte_ Byte pointer to hold Glonass satellite id and slot frequncy
    * \param [in] iValue  satellite id value
    * \param [in] iValue1 frequency value
    *
    */
   static void frameGlonassSatIdandSlotFreqValue(CHAR **psCurrentByte_, USHORT iValue, USHORT iValue1)
   {
      *psCurrentByte_ += sprintf(*psCurrentByte_, "%d", iValue);
      std::string strValue = "+";
      if (iValue1 > 0)
      {
         strValue.append(std::to_string(iValue1));
         *psCurrentByte_ += sprintf(*psCurrentByte_, "%s", strValue.c_str());
      }
      else if (iValue1 != 0)
      {
         *psCurrentByte_ += sprintf(*psCurrentByte_, "%d", iValue1);
      }
   }
};
#endif
