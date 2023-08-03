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
//! \file header_decoder.cpp
//! \brief Decoder OEM message headers.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/header_decoder.hpp"
#include <bitset>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
HeaderDecoder::HeaderDecoder(JsonReader* pclJsonDb_)
{
   pclMyLogger = Logger().RegisterLogger("novatel_header_decoder");

   pclMyLogger->debug("HeaderDecoder initializing...");

   if (pclJsonDb_ != nullptr)
   {
      LoadJsonDb(pclJsonDb_);
   }
   pclMyLogger->debug("HeaderDecoder initialized");
}

// -------------------------------------------------------------------------------------------------------
void
HeaderDecoder::LoadJsonDb(JsonReader* pclJsonDb_)
{
   pclMyMsgDb = pclJsonDb_;

   vMyCommandDefns       = pclMyMsgDb->GetEnumDef("Commands");
   vMyPortAddrDefns      = pclMyMsgDb->GetEnumDef("PortAddress");
   vMyGPSTimeStatusDefns = pclMyMsgDb->GetEnumDef("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void
HeaderDecoder::SetLoggerLevel(spdlog::level::level_enum eLevel_)
{
   pclMyLogger->set_level(eLevel_);
}

// -------------------------------------------------------------------------------------------------------
void
HeaderDecoder::ShutdownLogger()
{
   Logger::Shutdown();
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger>
HeaderDecoder::GetLogger()
{
   return pclMyLogger;
}

// -------------------------------------------------------------------------------------------------------
uint32_t
HeaderDecoder::MsgNameToMsgId(std::string sMsgName_)
{
      uint32_t uiSiblingID = 0;
      uint32_t uiMsgFormat = 0;
      uint32_t uiResponse  = 0; // 0 = original, 1 = response

      // Injest the sibling information, i.e. the _1 from LOGNAMEA_1
      if (sMsgName_.find_last_of('_') != std::string::npos
       && sMsgName_.find_last_of('_') == sMsgName_.size() - 2)
      {
         uiSiblingID = static_cast<uint32_t>(ToDigit(sMsgName_.back()));
         sMsgName_.resize(sMsgName_.size() - 2);
      }

      // If this is an abbrev msg (no format information), we will be able to find the MsgDef
      const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(sMsgName_);
      if (pclMessageDef)
      {
         uiResponse  = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ABBREV);

         return CreateMsgID(pclMessageDef->logID, uiSiblingID, uiMsgFormat, uiResponse);
      }

      std::string sTemp(sMsgName_);
      if (sTemp.back() == 'R') // Ascii Response
      {
         uiResponse  = static_cast<uint32_t>(true);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
         sTemp.pop_back();
      }
      else if (sTemp.back() == 'A') // Ascii
      {
         uiResponse  = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
         sTemp.pop_back();
      }
      else if (sTemp.back() == 'B') // Binary
      {
         uiResponse  = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::BINARY);
         sTemp.pop_back();
      }
      else // Abbrev
      {
         uiResponse  = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ABBREV);
      }

      pclMessageDef = pclMyMsgDb->GetMsgDef(sTemp);
      return pclMessageDef ? CreateMsgID(pclMessageDef->logID, uiSiblingID, uiMsgFormat, uiResponse) : 0;
}

// -------------------------------------------------------------------------------------------------------
std::string
HeaderDecoder::MsgIdToMsgName(const uint32_t uiMessageID_)
{
   uint16_t usLogID = 0;
   uint32_t uiSiblingID = 0;
   uint32_t uiMessageFormat = 0;
   uint32_t uiResponse = 0;

   UnpackMsgID(uiMessageID_, usLogID, uiSiblingID, uiMessageFormat, uiResponse);

   const MessageDefinition* pstMessageDefinition = pclMyMsgDb->GetMsgDef(usLogID);

   std::string strMessageName = pstMessageDefinition ? pstMessageDefinition->name : GetEnumString(vMyCommandDefns, usLogID);
   std::string strMessageFormatSuffix = uiResponse ? "R"
      : uiMessageFormat == static_cast<uint32_t>(MESSAGEFORMAT::BINARY) ? "B"
      : uiMessageFormat == static_cast<uint32_t>(MESSAGEFORMAT::ASCII)  ? "A"
      : ""; // default to abbreviated ascii format

   if (uiSiblingID)
      strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingID));
   return strMessageName.append(strMessageFormatSuffix);
}

// -------------------------------------------------------------------------------------------------------
template<ASCIIHEADER eField> bool
HeaderDecoder::DecodeAsciiHeaderField(IntermediateHeader& stIntermediateHeader_, char** ppcLogBuf_)
{
   constexpr bool bIsLeadingNumberField = eField == ASCIIHEADER::SEQUENCE
                                       || eField == ASCIIHEADER::IDLETIME
                                       || eField == ASCIIHEADER::WEEK
                                       || eField == ASCIIHEADER::SECONDS
                                       || eField == ASCIIHEADER::RECEIVER_SW_VERSION;

   constexpr bool bIsLeadingAlphaField = eField == ASCIIHEADER::MESSAGE_NAME
                                      || eField == ASCIIHEADER::PORT
                                      || eField == ASCIIHEADER::TIME_STATUS;

   constexpr bool bIsLeadingHexField = eField == ASCIIHEADER::RECEIVER_STATUS
                                    || eField == ASCIIHEADER::MSG_DEF_CRC;

   static_assert(bIsLeadingNumberField || bIsLeadingAlphaField || bIsLeadingHexField);

   // We check if the first character in the field is in the valid format
   if ((bIsLeadingNumberField && !isdigit (**ppcLogBuf_))
    || (bIsLeadingAlphaField  && !isalpha (**ppcLogBuf_))
    || (bIsLeadingHexField    && !isxdigit(**ppcLogBuf_)))
      return false;

   size_t ullTokenLength = strcspn(*ppcLogBuf_, " ,;\r");

   switch (eField)
   {
   case ASCIIHEADER::MESSAGE_NAME:
   {
      uint16_t usLogID = 0;
      uint32_t uiSiblingID = 0, uiMsgFormat = 0, uiResponse = 0;
      UnpackMsgID(MsgNameToMsgId(std::string(*ppcLogBuf_, ullTokenLength)), usLogID, uiSiblingID, uiMsgFormat, uiResponse);
      stIntermediateHeader_.usMessageID = usLogID;
      stIntermediateHeader_.ucMessageType = PackMsgType(uiSiblingID, uiMsgFormat, uiResponse);
      break;
   }
   case ASCIIHEADER::PORT:
      stIntermediateHeader_.uiPortAddress = static_cast<uint32_t>(GetEnumValue(vMyPortAddrDefns, std::string(*ppcLogBuf_, ullTokenLength)));
      break;
   case ASCIIHEADER::SEQUENCE:
      stIntermediateHeader_.usSequence = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10));
      break;
   case ASCIIHEADER::IDLETIME:
      stIntermediateHeader_.ucIdleTime = static_cast<uint8_t>(2.0 * strtof(*ppcLogBuf_, nullptr));
      break;
   case ASCIIHEADER::TIME_STATUS:
      stIntermediateHeader_.uiTimeStatus = GetEnumValue(vMyGPSTimeStatusDefns, std::string(*ppcLogBuf_, ullTokenLength));
      break;
   case ASCIIHEADER::WEEK:
      stIntermediateHeader_.usWeek = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10));
      break;
   case ASCIIHEADER::SECONDS:
      stIntermediateHeader_.dMilliseconds = 1000.0 * strtod(*ppcLogBuf_, nullptr);
      break;
   case ASCIIHEADER::RECEIVER_STATUS:
      stIntermediateHeader_.uiReceiverStatus = strtoul(*ppcLogBuf_, nullptr, 16);
      break;
   case ASCIIHEADER::MSG_DEF_CRC:
      stIntermediateHeader_.uiMessageDefinitionCRC = strtoul(*ppcLogBuf_, nullptr, 16);
      break;
   case ASCIIHEADER::RECEIVER_SW_VERSION:
      stIntermediateHeader_.usReceiverSwVersion = static_cast<uint16_t>(strtoul(*ppcLogBuf_, nullptr, 10));
      break;
   default:
      return false;
   }
   *ppcLogBuf_ += ullTokenLength + 1; // Consume the token and the trailing delimiter
   return true;
}

// -------------------------------------------------------------------------------------------------------
template <ASCIIHEADER... eFields>
bool HeaderDecoder::DecodeAsciiHeaderFields(IntermediateHeader& stIntermediateHeader_, char** ppcLogBuf) {
   return (DecodeAsciiHeaderField<eFields>(stIntermediateHeader_, ppcLogBuf) && ...);
}

// -------------------------------------------------------------------------------------------------------
void
HeaderDecoder::DecodeJsonHeader(json clJsonHeader_, IntermediateHeader& stIntermediateHeader_)
{
   stIntermediateHeader_.usMessageID            = clJsonHeader_["id"].get<uint16_t>();
   stIntermediateHeader_.uiPortAddress          = static_cast<uint32_t>(GetEnumValue(vMyPortAddrDefns, clJsonHeader_["port"].get<std::string>()));
   stIntermediateHeader_.usSequence             = clJsonHeader_["sequence_num"].get<uint16_t>();
   stIntermediateHeader_.ucIdleTime             = static_cast<uint8_t>(clJsonHeader_["percent_idle_time"].get<float>() * 2.0);
   stIntermediateHeader_.uiTimeStatus           = static_cast<uint32_t>(GetEnumValue(vMyGPSTimeStatusDefns, clJsonHeader_["time_status"].get<std::string>()));
   stIntermediateHeader_.usWeek                 = clJsonHeader_["week"].get<uint16_t>();
   stIntermediateHeader_.dMilliseconds          = clJsonHeader_["seconds"].get<double>() * 1000.0;
   stIntermediateHeader_.uiReceiverStatus       = clJsonHeader_["receiver_status"].get<uint32_t>();
   stIntermediateHeader_.usReceiverSwVersion    = clJsonHeader_["receiver_sw_version"].get<uint16_t>();
   stIntermediateHeader_.uiMessageDefinitionCRC = clJsonHeader_["HEADER_reserved1"].get<uint32_t>();
}

// -------------------------------------------------------------------------------------------------------
STATUS
HeaderDecoder::Decode(unsigned char* pucLogBuf_, IntermediateHeader& stIntermediateHeader_, MetaDataStruct& stMetaData_)
{
   if (!pucLogBuf_)
   {
      return STATUS::NULL_PROVIDED;
   }

   if (!pclMyMsgDb)
   {
      return STATUS::NO_DATABASE;
   }

   auto* pcTempBuf = reinterpret_cast<char*>(pucLogBuf_);
   auto* pstBinaryHeader = reinterpret_cast<OEM4BinaryHeader*>(pucLogBuf_);

   stMetaData_.eFormat = pstBinaryHeader->ucSync1 == OEM4_ASCII_SYNC         ? HEADERFORMAT::ASCII
                       : pstBinaryHeader->ucSync1 == OEM4_SHORT_ASCII_SYNC   ? HEADERFORMAT::SHORT_ASCII
                       : pstBinaryHeader->ucSync1 == OEM4_ABBREV_ASCII_SYNC  ? HEADERFORMAT::ABB_ASCII
                       : pstBinaryHeader->ucSync1 == NMEA_SYNC               ? HEADERFORMAT::NMEA
                       : pstBinaryHeader->ucSync1 == '{'                     ? HEADERFORMAT::JSON
                       : pstBinaryHeader->ucSync1 == OEM4_BINARY_SYNC1
                      && pstBinaryHeader->ucSync3 == OEM4_BINARY_SYNC3       ? HEADERFORMAT::BINARY
                       : pstBinaryHeader->ucSync1 == OEM4_BINARY_SYNC1
                      && pstBinaryHeader->ucSync3 == OEM4_SHORT_BINARY_SYNC3 ? HEADERFORMAT::SHORT_BINARY
                       : HEADERFORMAT::UNKNOWN;

   switch(stMetaData_.eFormat)
   {
   case HEADERFORMAT::ASCII:
      ++pcTempBuf; // Move the input buffer past the sync char '#'
      if (!DecodeAsciiHeaderFields<
         ASCIIHEADER::MESSAGE_NAME,
         ASCIIHEADER::PORT,
         ASCIIHEADER::SEQUENCE,
         ASCIIHEADER::IDLETIME,
         ASCIIHEADER::TIME_STATUS,
         ASCIIHEADER::WEEK,
         ASCIIHEADER::SECONDS,
         ASCIIHEADER::RECEIVER_STATUS,
         ASCIIHEADER::MSG_DEF_CRC,
         ASCIIHEADER::RECEIVER_SW_VERSION
      >(stIntermediateHeader_, &pcTempBuf))
      {
         return STATUS::FAILURE;
      }
      break;

   case HEADERFORMAT::ABB_ASCII:
      ++pcTempBuf; // Move the input buffer past the sync char '<'
      // At this point, we do not know if the format is short or not, but both have a message field
      if (!DecodeAsciiHeaderFields<ASCIIHEADER::MESSAGE_NAME>(stIntermediateHeader_, &pcTempBuf))
      {
         return STATUS::FAILURE;
      }
      if (DecodeAsciiHeaderFields<ASCIIHEADER::PORT>(stIntermediateHeader_, &pcTempBuf))
      {
         // Port field succeeded, so this is not short format
         if (!DecodeAsciiHeaderFields<
            ASCIIHEADER::SEQUENCE,
            ASCIIHEADER::IDLETIME,
            ASCIIHEADER::TIME_STATUS,
            ASCIIHEADER::WEEK,
            ASCIIHEADER::SECONDS,
            ASCIIHEADER::RECEIVER_STATUS,
            ASCIIHEADER::MSG_DEF_CRC,
            ASCIIHEADER::RECEIVER_SW_VERSION
         >(stIntermediateHeader_, &pcTempBuf))
         {
            return STATUS::FAILURE;
         };
      }
      else
      {
         // Port field failed, so we (unsafely) assume this is short
         stMetaData_.eFormat = HEADERFORMAT::SHORT_ABB_ASCII;
         if (!DecodeAsciiHeaderFields<
            ASCIIHEADER::WEEK,
            ASCIIHEADER::SECONDS
         >(stIntermediateHeader_, &pcTempBuf))
         {
            return STATUS::FAILURE;
         }
      }
      ++pcTempBuf; // Move the input buffer past the trailing delimiter '\n'
      break;

   case HEADERFORMAT::SHORT_ASCII:
      ++pcTempBuf; // Move the input buffer past the sync char '%'
      if (!DecodeAsciiHeaderFields<
         ASCIIHEADER::MESSAGE_NAME,
         ASCIIHEADER::WEEK,
         ASCIIHEADER::SECONDS
      >(stIntermediateHeader_, &pcTempBuf))
      {
         return STATUS::FAILURE;
      };
      break;

   case HEADERFORMAT::BINARY:
      stIntermediateHeader_.usMessageID            = pstBinaryHeader->usMsgNumber;
      stIntermediateHeader_.ucMessageType          = pstBinaryHeader->ucMsgType;
      stIntermediateHeader_.uiPortAddress          = pstBinaryHeader->ucPort;
      stIntermediateHeader_.usLength               = pstBinaryHeader->usLength;
      stIntermediateHeader_.usSequence             = pstBinaryHeader->usSequenceNumber;
      stIntermediateHeader_.ucIdleTime             = pstBinaryHeader->ucIdleTime;
      stIntermediateHeader_.uiTimeStatus           = pstBinaryHeader->ucTimeStatus;
      stIntermediateHeader_.usWeek                 = pstBinaryHeader->usWeekNo;
      stIntermediateHeader_.dMilliseconds          = pstBinaryHeader->uiWeekMSec;
      stIntermediateHeader_.uiReceiverStatus       = pstBinaryHeader->uiStatus;
      stIntermediateHeader_.usReceiverSwVersion    = pstBinaryHeader->usReceiverSWVersion;
      stIntermediateHeader_.uiMessageDefinitionCRC = pstBinaryHeader->usMsgDefCRC;
      pcTempBuf += sizeof(OEM4BinaryHeader);
      break;

   case HEADERFORMAT::SHORT_BINARY:
   {
      auto* pstBinaryShortHeader = reinterpret_cast<OEM4BinaryShortHeader*>(pucLogBuf_);
      stIntermediateHeader_.usLength      = pstBinaryShortHeader->ucLength;
      stIntermediateHeader_.usMessageID   = pstBinaryShortHeader->usMessageId;
      stIntermediateHeader_.usWeek        = pstBinaryShortHeader->usWeekNo;
      stIntermediateHeader_.dMilliseconds = pstBinaryShortHeader->uiWeekMSec;
      pcTempBuf += sizeof(OEM4BinaryShortHeader);
      break;
   }
   case HEADERFORMAT::JSON:
      try
      {
         DecodeJsonHeader(json::parse(pcTempBuf)["header"], stIntermediateHeader_);
      }
      catch (std::exception& e)
      {
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, e.what());
         return STATUS::FAILURE;
      }
      break;

   case HEADERFORMAT::NMEA:
      return STATUS::UNSUPPORTED;

   default:
      return STATUS::UNKNOWN;
   }

   stMetaData_.eMeasurementSource = static_cast<MEASUREMENT_SOURCE>(stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC));
   stMetaData_.eTimeStatus        = static_cast<TIME_STATUS>(stIntermediateHeader_.uiTimeStatus);
   stMetaData_.bResponse          = static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE) == (stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE));
   stMetaData_.usWeek             = static_cast<uint32_t>(stIntermediateHeader_.usWeek);
   stMetaData_.dMilliseconds      = static_cast<uint32_t>(stIntermediateHeader_.dMilliseconds);
   stMetaData_.usMessageID        = static_cast<uint32_t>(stIntermediateHeader_.usMessageID);
   stMetaData_.uiMessageCRC       = static_cast<uint32_t>(stIntermediateHeader_.uiMessageDefinitionCRC);
   stMetaData_.uiHeaderLength     = static_cast<uint32_t>(pcTempBuf - reinterpret_cast<char*>(pucLogBuf_));
   stMetaData_.uiBinaryMsgLength  = static_cast<uint32_t>(stIntermediateHeader_.usLength);

   // Reconstruct a message name that won't have a suffix of any kind.
   stMetaData_.MessageName(MsgIdToMsgName(CreateMsgID(static_cast<uint32_t>(stIntermediateHeader_.usMessageID), static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY), static_cast<uint32_t>(MESSAGEFORMAT::ABBREV), 0U)));

   return STATUS::SUCCESS;
}
