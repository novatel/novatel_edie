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
//! \file encoder.cpp
//! \brief Encode OEM messages.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/encoder.hpp"
#include <bitset>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
constexpr bool
PrintAsString(const BaseField* field_def)
{
   // Printing as a string means two things:
   // 1. The field will be surrounded by quotes
   // 2. The field will not contain null-termination or padding
   return field_def->type == FIELD_TYPE::STRING
       || field_def->conversionStripped == CONVERSION_STRING::s
       || field_def->conversionStripped == CONVERSION_STRING::S;
}

// -------------------------------------------------------------------------------------------------------
constexpr bool
IsCommaSeparated(const BaseField* field_def)
{
   // In certain cases there are no separators printed between array elements
   return !PrintAsString(field_def)
      && field_def->conversionStripped != CONVERSION_STRING::Z
      && field_def->conversionStripped != CONVERSION_STRING::P;
}

// -------------------------------------------------------------------------------------------------------
Encoder::Encoder(JsonReader* pclJsonDb_) :
   uiMyAbbrevAsciiIndentationLevel(1)
{
   pclMyLogger = Logger().RegisterLogger("novatel_encoder");

   pclMyLogger->debug("Encoder initializing...");

   if (pclJsonDb_ != nullptr)
   {
      LoadJsonDb(pclJsonDb_);
   }
   pclMyLogger->debug("Encoder initialized");
}

// -------------------------------------------------------------------------------------------------------
void
Encoder::LoadJsonDb(JsonReader* pclJsonDb_)
{
   pclMyMsgDb = pclJsonDb_;

   InitEnumDefns();
   CreateResponseMsgDefns();
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger>
Encoder::GetLogger() const
{
   return pclMyLogger;
}

// -------------------------------------------------------------------------------------------------------
void
Encoder::SetLoggerLevel(spdlog::level::level_enum eLevel_)
{
   pclMyLogger->set_level(eLevel_);
}

// -------------------------------------------------------------------------------------------------------
void
Encoder::ShutdownLogger()
{
   Logger::Shutdown();
}

// -------------------------------------------------------------------------------------------------------
void
Encoder::InitEnumDefns()
{
   vMyRespDefns          = pclMyMsgDb->GetEnumDef("Responses");
   vMyCommandDefns       = pclMyMsgDb->GetEnumDef("Commands");
   vMyPortAddrDefns      = pclMyMsgDb->GetEnumDef("PortAddress");
   vMyGPSTimeStatusDefns = pclMyMsgDb->GetEnumDef("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void
Encoder::CreateResponseMsgDefns()
{
   // Numerical response ID
   SimpleDataType stRespIdDataType;
   stRespIdDataType.description = "Response as numerical id";
   stRespIdDataType.length = 4;
   stRespIdDataType.name = DATA_TYPE_NAME::UINT;

   EnumField stRespIdField;
   stRespIdField.name = "response_id";
   stRespIdField.type = FIELD_TYPE::RESPONSE_ID;
   stRespIdField.dataType = stRespIdDataType;
   stRespIdField.enumID = vMyRespDefns->_id;
   stRespIdField.enumDef = vMyRespDefns;

   // String response ID
   SimpleDataType stRespStrDataType;
   stRespStrDataType.description = "Response as a string";
   stRespStrDataType.length = 1;
   stRespStrDataType.name = DATA_TYPE_NAME::CHAR;

   BaseField stRespStrField;
   stRespStrField.name = "response_str";
   stRespStrField.type = FIELD_TYPE::RESPONSE_STR;
   stRespStrField.dataType = stRespStrDataType;

   // Message Definition
   stMyRespDef = MessageDefinition();
   stMyRespDef.name = std::string("response");
   stMyRespDef.fields[0].push_back(stRespIdField.clone());
   stMyRespDef.fields[0].push_back(stRespStrField.clone());
}

// -------------------------------------------------------------------------------------------------------
uint32_t
Encoder::MsgNameToMsgId(std::string sMsgName_) const
{
   uint32_t uiSiblingID = 0;
   uint32_t uiMsgFormat;
   uint32_t uiResponse;

   // Ingest the sibling information, i.e. the _1 from LOGNAMEA_1
   if (sMsgName_.find_last_of('_') != std::string::npos &&
       sMsgName_.find_last_of('_') == sMsgName_.size() - 2)
   {
      uiSiblingID = static_cast<uint32_t>(ToDigit(sMsgName_.back()));
      sMsgName_.resize(sMsgName_.size() - 2);
   }

   // If this is an abbrev msg (no format information), we will be able to find the MsgDef
   const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(sMsgName_);
   if (pclMessageDef)
   {
      uiResponse = static_cast<uint32_t>(false);
      uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ABBREV);

      return CreateMsgID(pclMessageDef->logID, uiSiblingID, uiMsgFormat, uiResponse);
   }
   
   if (sMsgName_.back() == 'R') // Ascii Response
   {
      uiResponse = static_cast<uint32_t>(true);
      uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
      sMsgName_.pop_back();
   }
   else if (sMsgName_.back() == 'A') // Ascii
   {
      uiResponse = static_cast<uint32_t>(false);
      uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
      sMsgName_.pop_back();
   }
   else if (sMsgName_.back() == 'B') // Binary
   {
      uiResponse = static_cast<uint32_t>(false);
      uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::BINARY);
      sMsgName_.pop_back();
   }
   else // Abbrev
   {
      uiResponse = static_cast<uint32_t>(false);
      uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ABBREV);
   }

   pclMessageDef = pclMyMsgDb->GetMsgDef(sMsgName_);

   return pclMessageDef ? CreateMsgID(pclMessageDef ? pclMessageDef->logID : GetEnumValue(vMyCommandDefns, sMsgName_), uiSiblingID, uiMsgFormat, uiResponse) : 0;
}

// -------------------------------------------------------------------------------------------------------
std::string
Encoder::MsgIdToMsgName(const uint32_t uiMessageID_) const
{
   uint16_t usLogID = 0;
   uint32_t uiSiblingID = 0;
   uint32_t uiMessageFormat = 0;
   uint32_t uiResponse = 0;

   UnpackMsgID(uiMessageID_, usLogID, uiSiblingID, uiMessageFormat, uiResponse);

   const MessageDefinition* pstMessageDefinition = pclMyMsgDb->GetMsgDef(usLogID);
   std::string strMessageName = pstMessageDefinition ? pstMessageDefinition->name : GetEnumString(vMyCommandDefns, usLogID);

   std::string strMessageFormatSuffix =
      uiResponse ? "R"
    : uiMessageFormat == static_cast<uint32_t>(MESSAGEFORMAT::BINARY) ? "B"
    : uiMessageFormat == static_cast<uint32_t>(MESSAGEFORMAT::ASCII)  ? "A" : "";

   if (uiSiblingID)
      strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingID));

   return strMessageName.append(strMessageFormatSuffix);
}

// -------------------------------------------------------------------------------------------------------
MsgFieldsVector*
Encoder::GetMsgDefFromCRC(const MessageDefinition* pclMessageDef_, uint32_t &uiMsgDefCRC_) const
{
   // If we can't find the correct CRC just default to the latest.
   MsgFieldsVector* pvMsgFields;
   if (pclMessageDef_->fields.count(uiMsgDefCRC_) == 0)
   {
      pclMyLogger->info("Log DB is missing the log definition {} - {}.  Defaulting to newest version of the log definition.", pclMessageDef_->name, uiMsgDefCRC_);
      pvMsgFields = &pclMessageDef_->fields.at(pclMessageDef_->latestMessageCrc);
      uiMsgDefCRC_ = pclMessageDef_->latestMessageCrc;
   }
   else
   {
      pvMsgFields = &pclMessageDef_->fields.at(uiMsgDefCRC_);
   }
   return pvMsgFields;
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeBinaryHeader(const IntermediateHeader& stIntermediateHeader_, unsigned char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   OEM4BinaryHeader stBinaryHeader;

   stBinaryHeader.ucSync1 = OEM4_BINARY_SYNC1;
   stBinaryHeader.ucSync2 = OEM4_BINARY_SYNC2;
   stBinaryHeader.ucSync3 = OEM4_BINARY_SYNC3;
   stBinaryHeader.ucHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stBinaryHeader.usMsgNumber = stIntermediateHeader_.usMessageID;
   stBinaryHeader.ucMsgType = stIntermediateHeader_.ucMessageType & (~static_cast<uint32_t>(MESSAGETYPEMASK::MSGFORMAT));
   stBinaryHeader.ucPort = static_cast<uint8_t>(stIntermediateHeader_.uiPortAddress);
   stBinaryHeader.usLength = stIntermediateHeader_.usLength;
   stBinaryHeader.usSequenceNumber = stIntermediateHeader_.usSequence;
   stBinaryHeader.ucIdleTime = stIntermediateHeader_.ucIdleTime;
   stBinaryHeader.ucTimeStatus = static_cast<uint8_t>(stIntermediateHeader_.uiTimeStatus & 0xFF);
   stBinaryHeader.usWeekNo = stIntermediateHeader_.usWeek;
   stBinaryHeader.uiWeekMSec = static_cast<uint32_t>(stIntermediateHeader_.dMilliseconds);
   stBinaryHeader.uiStatus = stIntermediateHeader_.uiReceiverStatus;
   stBinaryHeader.usMsgDefCRC = static_cast<uint16_t>(stIntermediateHeader_.uiMessageDefinitionCRC & 0xFFFF);
   stBinaryHeader.usReceiverSWVersion = stIntermediateHeader_.usReceiverSwVersion;

   return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &stBinaryHeader);
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeBinaryShortHeader(const IntermediateHeader& stIntermediateHeader_, unsigned char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   OEM4BinaryShortHeader stBinaryHeader; 

   stBinaryHeader.ucSync1 = OEM4_BINARY_SYNC1;
   stBinaryHeader.ucSync2 = OEM4_BINARY_SYNC2;
   stBinaryHeader.ucSync3 = OEM4_SHORT_BINARY_SYNC3;
   stBinaryHeader.ucLength = 0; // Will be filled in following the body encoding
   stBinaryHeader.usMessageId = stIntermediateHeader_.usMessageID;
   stBinaryHeader.usWeekNo = stIntermediateHeader_.usWeek;
   stBinaryHeader.uiWeekMSec = static_cast<uint32_t>(stIntermediateHeader_.dMilliseconds);

   return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &stBinaryHeader);
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeBinaryBody(const IntermediateMessage& stIntermediateMessage_, unsigned char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_, bool bFlatten_)
{
   unsigned char* pucTempStart;

   for (const auto& field : stIntermediateMessage_)
   {
      // Realign to type byte boundary if needed
      const uint16_t usTypeAlignment = field.field_def->dataType.length >= 4 ? 4 : field.field_def->dataType.length;
      if (reinterpret_cast<uint64_t>(*ppcOutBuf_) % usTypeAlignment != 0
         && !SetInBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, 0, usTypeAlignment - (reinterpret_cast<uint64_t>(*ppcOutBuf_) % usTypeAlignment)))
      {
         return false;
      }

      if (std::holds_alternative<std::vector<FieldContainer>>(field.field_value))
      {
         const auto& vFCCurrentVectorField = std::get<std::vector<FieldContainer>>(field.field_value);

         // FIELD_ARRAY types contain several classes and so will use a recursive call
         if (field.field_def->type == FIELD_TYPE::FIELD_ARRAY)
         {
            const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.field_value);

            uint32_t uiFieldCount = static_cast<uint32_t>(vCurrentFieldArrayField.size());
            if (!CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &uiFieldCount))
            {
               return false;
            }

            pucTempStart = *ppcOutBuf_; // Move the start placeholder to the front of the array start

            for (const auto& clFieldArray : vCurrentFieldArrayField)
            {
               if (!EncodeBinaryBody(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiMyBufferBytesRemaining_, bFlatten_))
               {
                  return false;
               }
            }

            // If the user wishes to receive a flattened version of the log, fill in the remaining fields that are not used with 0x00.
            if (bFlatten_ && (static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart) < dynamic_cast<const FieldArrayField*>(field.field_def)->fieldSize))
            {
               if (!SetInBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, '\0', dynamic_cast<const FieldArrayField*>(field.field_def)->fieldSize - static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart)))
               {
                  return false;
               }
            }
         }
         else
         {
            if (field.field_def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
               // if the field is a variable array, print the size first
               const uint32_t uiVarArraySize = static_cast<uint32_t>(vFCCurrentVectorField.size());
               if (!CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &uiVarArraySize))
               {
                  return false;
               }
            }

            pucTempStart = *ppcOutBuf_; // Move the start placeholder to the front of the array start

            // This is an array of simple elements
            for (const auto& arrayField : vFCCurrentVectorField)
            {
               if (!FieldToBinary(arrayField, ppcOutBuf_, uiMyBufferBytesRemaining_))
               {
                  return false;
               }
            }

            // If the user wishes to receive a flattened version of the log, fill in the remaining fields that are not used with 0x00.
            if (bFlatten_)
            {
               const uint32_t uiMaxArraySize = (dynamic_cast<const ArrayField*>(field.field_def)->arrayLength) * (field.field_def->dataType.length);
               if (static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart) < uiMaxArraySize)
               {
                  if (!SetInBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, '\0', uiMaxArraySize - static_cast<uint32_t>(*ppcOutBuf_ - pucTempStart)))
                  {
                     return false;
                  }
               }
            }
         }
      }
      else if (field.field_def->type == FIELD_TYPE::STRING)
      {
         // Handle the entire string as we know it will be null-terminated
         const char* szString = std::get<std::string>(field.field_value).c_str();
         if (!CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, szString))
         {
            return false;
         }

         // If the user wishes to receive a flattened version of the log, fill in the remaining characters that are not used with 0x00.
         if (bFlatten_)
         {
            const auto uiStringLength = static_cast<uint32_t>(strlen(szString));
            const uint32_t uiMaxArraySize = dynamic_cast<const ArrayField*>(field.field_def)->arrayLength * field.field_def->dataType.length;
            if (uiStringLength < uiMaxArraySize && !SetInBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, '\0', uiMaxArraySize - uiStringLength))
            {
               return false;
            }
         }
         else if ((4 - reinterpret_cast<uint64_t>(*ppcOutBuf_) % 4) != 0 &&
            !SetInBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, '\0', 4 - reinterpret_cast<uint64_t>(*ppcOutBuf_) % 4))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::ENUM)
      {
         if (!CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<int32_t>(field.field_value)))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_ID)
      {
         if (!CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<int32_t>(field.field_value)))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_STR)
      {
         if (!CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, std::get<std::string>(field.field_value).c_str()))
         {
            return false;
         }
      }
      // Handle individual simple element
      else if (!FieldToBinary(field, ppcOutBuf_, uiMyBufferBytesRemaining_))
      {
         return false;
      }
   }
   return true;
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   switch (fc_.field_def->conversionStripped)
   {
   case CONVERSION_STRING::m:  [[fallthrough]];
   case CONVERSION_STRING::T:  [[fallthrough]];
   case CONVERSION_STRING::id: return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint32_t>(fc_.field_value));
   case CONVERSION_STRING::UB: [[fallthrough]];
   case CONVERSION_STRING::P:  [[fallthrough]];
   case CONVERSION_STRING::XB: return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint8_t> (fc_.field_value));
   case CONVERSION_STRING::B:  return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<int8_t>  (fc_.field_value));
   case CONVERSION_STRING::k:  return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<float>   (fc_.field_value));
   case CONVERSION_STRING::lk: return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<double>  (fc_.field_value));
   case CONVERSION_STRING::c: 
	  return (fc_.field_def->dataType.length == 1)
         ? CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint8_t>(fc_.field_value))
         : (fc_.field_def->dataType.length == 4 && fc_.field_def->dataType.name == DATA_TYPE_NAME::ULONG)
         ? CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint32_t>(fc_.field_value))
         : true; // is this a bug?
   default:
      switch (fc_.field_def->dataType.name)
      {
      case DATA_TYPE_NAME::BOOL:
      {
         int32_t i4ByteBoolRep = static_cast<int32_t>(std::get<bool>(fc_.field_value));
         return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &i4ByteBoolRep);
      }
      case DATA_TYPE_NAME::HEXBYTE:   [[fallthrough]];
      case DATA_TYPE_NAME::UCHAR:     return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint8_t> (fc_.field_value));
      case DATA_TYPE_NAME::CHAR:      return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<int8_t>  (fc_.field_value));
      case DATA_TYPE_NAME::USHORT:    return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint16_t>(fc_.field_value));
      case DATA_TYPE_NAME::SHORT:     return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<int16_t> (fc_.field_value));
      case DATA_TYPE_NAME::UINT:      [[fallthrough]];
      case DATA_TYPE_NAME::ULONG:     return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint32_t>(fc_.field_value));
      case DATA_TYPE_NAME::INT:       [[fallthrough]];
      case DATA_TYPE_NAME::LONG:      return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<int32_t> (fc_.field_value));
      case DATA_TYPE_NAME::ULONGLONG: return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<uint64_t>(fc_.field_value));
      case DATA_TYPE_NAME::LONGLONG:  return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<int64_t> (fc_.field_value));
      case DATA_TYPE_NAME::FLOAT:     return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<float>   (fc_.field_value));
      case DATA_TYPE_NAME::DOUBLE:    return CopyToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, &std::get<double>  (fc_.field_value));
      default:
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToBinary(): unknown type.");
         throw std::runtime_error("FieldToBinary(): unknown type.");
      }
   }
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeAsciiHeader(const IntermediateHeader& stIntermediateHeader_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   // Sync byte
   if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ASCII_SYNC))
   {
      return false;
   }

   // Message name
   const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(stIntermediateHeader_.usMessageID);
   std::string sMsgName(pclMessageDef ? pclMessageDef->name : GetEnumString(vMyCommandDefns, stIntermediateHeader_.usMessageID));
   const uint32_t uiSiblingID = stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
   const uint32_t uiResponse = (stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE)) >> 7;
   sMsgName.append(uiResponse ? "R" : "A"); // Append 'A' for ascii, or 'R' for ascii response
   if (uiSiblingID) // Append sibling i.e. the _1 of RANGEA_1
      sMsgName.append("_").append(std::to_string(uiSiblingID));

   return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c",    sMsgName.c_str(),                             OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c",    GetEnumString(vMyPortAddrDefns, stIntermediateHeader_.uiPortAddress).c_str(), OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu%c",   stIntermediateHeader_.usSequence,             OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.1f%c",  stIntermediateHeader_.ucIdleTime * 0.500,     OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c",    GetEnumString(vMyGPSTimeStatusDefns, stIntermediateHeader_.uiTimeStatus).c_str(), OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu%c",   stIntermediateHeader_.usWeek,                 OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.3f%c",  stIntermediateHeader_.dMilliseconds / 1000.0, OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%08lx%c", stIntermediateHeader_.uiReceiverStatus,       OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%04x%c",  stIntermediateHeader_.uiMessageDefinitionCRC, OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu%c",   stIntermediateHeader_.usReceiverSwVersion,    OEM4_ASCII_HEADER_TERMINATOR);
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeAbbrevAsciiHeader(const IntermediateHeader& stIntermediateHeader_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_, bool bIsEmbeddedHeader_)
{
   // Sync byte
   if (!bIsEmbeddedHeader_
      && !PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ABBREV_ASCII_SYNC))
   {
      return false;
   }

   // Message name
   const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(stIntermediateHeader_.usMessageID);
   std::string sMsgName(pclMessageDef ? pclMessageDef->name : GetEnumString(vMyCommandDefns, stIntermediateHeader_.usMessageID));
   const uint32_t uiSiblingID = stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
   if (uiSiblingID) // Append sibling i.e. the _1 of RANGEA_1
      sMsgName.append("_").append(std::to_string(uiSiblingID));

   return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c",    sMsgName.c_str(),                             OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c",    GetEnumString(vMyPortAddrDefns,      stIntermediateHeader_.uiPortAddress).c_str(), OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu%c",   stIntermediateHeader_.usSequence,             OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.1f%c",  stIntermediateHeader_.ucIdleTime * 0.500,     OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c",    GetEnumString(vMyGPSTimeStatusDefns, stIntermediateHeader_.uiTimeStatus).c_str(),  OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu%c",   stIntermediateHeader_.usWeek,                 OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.3f%c",  stIntermediateHeader_.dMilliseconds / 1000.0, OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%08lx%c", stIntermediateHeader_.uiReceiverStatus,       OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%04x%c",  stIntermediateHeader_.uiMessageDefinitionCRC, OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu",     stIntermediateHeader_.usReceiverSwVersion)
       ? bIsEmbeddedHeader_
       ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ABBREV_ASCII_SEPARATOR)
       : PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\r\n")
       : false;
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeAsciiShortHeader(const IntermediateHeader& stIntermediateHeader_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   // Sync byte
   if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_SHORT_ASCII_SYNC))
   {
      return false;
   }

   // Message name
   std::string sMsgName(pclMyMsgDb->GetMsgDef(stIntermediateHeader_.usMessageID)->name);
   const uint32_t uiSiblingID = stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
   const uint32_t uiResponse = (stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::RESPONSE)) >> 7;
   sMsgName.append(uiResponse ? "R" : "A"); // Append 'A' for ascii, or 'R' for ascii response
   if (uiSiblingID) // Append sibling i.e. the _1 of RANGEA_1
      sMsgName.append("_").append(std::to_string(uiSiblingID));

   return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c", sMsgName.c_str(), OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu%c", stIntermediateHeader_.usWeek, OEM4_ASCII_FIELD_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.3f%c", stIntermediateHeader_.dMilliseconds / 1000.0, OEM4_ASCII_HEADER_TERMINATOR);
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeAbbrevAsciiShortHeader(const IntermediateHeader& stIntermediateHeader_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   // Sync byte
   if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ABBREV_ASCII_SYNC))
   {
      return false;
   }

   // Message name
   std::string sMsgName(pclMyMsgDb->GetMsgDef(stIntermediateHeader_.usMessageID)->name);
   uint32_t uiSiblingID = stIntermediateHeader_.ucMessageType & static_cast<uint32_t>(MESSAGETYPEMASK::MEASSRC);
   if (uiSiblingID) // Append sibling i.e. the _1 of RANGEA_1
      sMsgName.append("_").append(std::to_string(uiSiblingID));

   return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c", sMsgName.c_str(), OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu%c", stIntermediateHeader_.usWeek, OEM4_ABBREV_ASCII_SEPARATOR)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.3f", stIntermediateHeader_.dMilliseconds / 1000.0)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\r\n");
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeAsciiBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   for (const auto& field : vIntermediateFormat_)
   {
      if (std::holds_alternative<std::vector<FieldContainer>>(field.field_value))
      {
         const auto& vFCCurrentVectorField = std::get<std::vector<FieldContainer>>(field.field_value);

         // FIELD_ARRAY types contain several classes and so will use a recursive call
         if (field.field_def->type == FIELD_TYPE::FIELD_ARRAY)
         {
            if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d%c", vFCCurrentVectorField.size(), OEM4_ASCII_FIELD_SEPARATOR))
            {
               return false;
            }

            const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.field_value);
            for (const auto& clFieldArray : vCurrentFieldArrayField)
            {
               if (!EncodeAsciiBody(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiMyBufferBytesRemaining_))
               {
                  return false;
               }
            }
         }
         else
         {
            const bool bPrintAsString = PrintAsString(field.field_def);
            const bool bIsCommaSeparated = IsCommaSeparated(field.field_def);

            // if the field is a variable array, print the size first
            if ((field.field_def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY
               && !PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d%c", vFCCurrentVectorField.size(), OEM4_ASCII_FIELD_SEPARATOR))
               || (bPrintAsString && !PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\"")))
            {
               return false;
            }

            // This is an array of simple elements
            for (const auto& arrayField : vFCCurrentVectorField)
            {
               // If we are printing a string, don't print the null terminator or any padding bytes
               if (bPrintAsString)
               {
                  if ((std::holds_alternative<int8_t>(arrayField.field_value)
                     && std::get<int8_t>(arrayField.field_value) == '\0')
                     || (std::holds_alternative<uint8_t>(arrayField.field_value)
                     && std::get<uint8_t>(arrayField.field_value) == '\0'))
                  {
                     break;
                  }
               }

               if (!FieldToAscii(arrayField, ppcOutBuf_, uiMyBufferBytesRemaining_))
               {
                  return false;
               }

               if (bIsCommaSeparated)
               {
                  if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ASCII_FIELD_SEPARATOR))
                  {
                     return false;
                  }
               }
            }
            // Quoted elements need a trailing comma
            if (bPrintAsString)
            {
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\"%c", OEM4_ASCII_FIELD_SEPARATOR))
               {
                  return false;
               }
            }
            // Non-quoted, non-internally-separated elements also need a trailing comma
            else if (!bIsCommaSeparated)
            {
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ASCII_FIELD_SEPARATOR))
               {
                  return false;
               }
            }
         }
      }
      else if (field.field_def->type == FIELD_TYPE::STRING)
      {
         // STRING types can be handled all at once because they show up as a single element and
         // are guaranteed to have a null terminator
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\"%s\"%c", std::get<std::string>(field.field_value).c_str(), OEM4_ASCII_FIELD_SEPARATOR))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::ENUM)
      {
         const auto enumField = dynamic_cast<const EnumField*>(field.field_def);
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c", GetEnumString(enumField->enumDef, std::get<int32_t>(field.field_value)).c_str(), OEM4_ASCII_FIELD_SEPARATOR))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_ID)
      {
         // Do nothing, ascii logs don't output this field
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_STR)
      {
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c", std::get<std::string>(field.field_value).c_str(), OEM4_ASCII_FIELD_SEPARATOR))
         {
            return false;
         }
      }
      // Handle individual simple element
      else if (!FieldToAscii(field, ppcOutBuf_, uiMyBufferBytesRemaining_) || !PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ASCII_FIELD_SEPARATOR))
      {
         return false;
      }
   }
   return true;
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeAbbrevAsciiBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   bool new_indent_line = false;

   std::string strIndentation(static_cast<uint64_t>(uiMyAbbrevAsciiIndentationLevel) * OEM4_ABBREV_ASCII_INDENTATION_LENGTH, ' ');
   if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "<%s", strIndentation.c_str()))
      return false;

   for (const auto& field : vIntermediateFormat_)
   {
      if (new_indent_line)
      {
         strIndentation = std::string(static_cast<uint64_t>(uiMyAbbrevAsciiIndentationLevel) * OEM4_ABBREV_ASCII_INDENTATION_LENGTH, ' ');
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\r\n<%s", strIndentation.c_str()))
            return false;
         new_indent_line = false;
      }
      if (std::holds_alternative<std::vector<FieldContainer>>(field.field_value))
      {
         const auto& vFCCurrentVectorField = std::get<std::vector<FieldContainer>>(field.field_value);

         // FIELD_ARRAY types contain several classes and so will use a recursive call
         if (field.field_def->type == FIELD_TYPE::FIELD_ARRAY)
         {
            if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d%c", vFCCurrentVectorField.size(), OEM4_ABBREV_ASCII_SEPARATOR))
            {
               return false;
            }

            const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.field_value);
            // Abbrev ascii will output a blank line for a 0 length array, nice
            if (vCurrentFieldArrayField.empty())
            {
               uiMyAbbrevAsciiIndentationLevel++;
               strIndentation = std::string(static_cast<uint64_t>(uiMyAbbrevAsciiIndentationLevel) * OEM4_ABBREV_ASCII_INDENTATION_LENGTH, ' ');
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\r\n<%s", strIndentation.c_str()))
                  return false;
               uiMyAbbrevAsciiIndentationLevel--;
            }
            // Data was printed so a new line is required at the end of the array if there are more fields in the log
            else
            {
               for (const auto& clFieldArray : vCurrentFieldArrayField)
               {
                  if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\r\n"))
                  {
                     return false;
                  }

                  uiMyAbbrevAsciiIndentationLevel++;
                  if (!EncodeAbbrevAsciiBody(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiMyBufferBytesRemaining_))
                  {
                     return false;
                  }
                  uiMyAbbrevAsciiIndentationLevel--;
               }
               new_indent_line = true;
            }
         }
         else
         {
            const bool bPrintAsString = PrintAsString(field.field_def);
            const bool bIsCommaSeparated = IsCommaSeparated(field.field_def);

            // if the field is a variable array, print the size first
            if (field.field_def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d%c", vFCCurrentVectorField.size(), OEM4_ABBREV_ASCII_SEPARATOR))
               {
                  return false;
               }
            }

            if (bPrintAsString && !PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\""))
            {
               return false;
            }
            // This is an array of simple elements
            for (const auto& arrayField : vFCCurrentVectorField)
            {
               // If we are printing a string, don't print the null terminator or any padding bytes
               if (bPrintAsString)
               {
                  if ((std::holds_alternative<int8_t>(arrayField.field_value)
                     && std::get<int8_t>(arrayField.field_value) == '\0')
                     || (std::holds_alternative<uint8_t>(arrayField.field_value)
                     && std::get<uint8_t>(arrayField.field_value) == '\0'))
                  {
                     break;
                  }
               }

               if (!FieldToAscii(arrayField, ppcOutBuf_, uiMyBufferBytesRemaining_))
               {
                  return false;
               }

               if (bIsCommaSeparated)
               {
                  if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ABBREV_ASCII_SEPARATOR))
                  {
                     return false;
                  }
               }
            }
            // Quoted elements need a trailing comma
            if (bPrintAsString)
            {
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\"%c", OEM4_ABBREV_ASCII_SEPARATOR))
               {
                  return false;
               }
            }
            // Non-quoted, non-internally-separated elements also need a trailing comma
            else if (!bIsCommaSeparated)
            {
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ABBREV_ASCII_SEPARATOR))
               {
                  return false;
               }
            }
         }
      }
      else if (field.field_def->type == FIELD_TYPE::STRING)
      {
         // STRING types can be handled all at once because they show up as a single element and
         // are guaranteed to have a null terminator
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\"%s\"%c", std::get<std::string>(field.field_value).c_str(), OEM4_ABBREV_ASCII_SEPARATOR))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::ENUM)
      {
         const auto enumField = dynamic_cast<const EnumField*>(field.field_def);
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c", GetEnumString(enumField->enumDef, std::get<int32_t>(field.field_value)).c_str(), OEM4_ABBREV_ASCII_SEPARATOR))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_ID)
      {
         // Do nothing, ascii logs don't output this field
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_STR)
      {
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s%c", std::get<std::string>(field.field_value).c_str(), OEM4_ABBREV_ASCII_SEPARATOR))
         {
            return false;
         }
      }
      // Handle individual simple element
      else if (!FieldToAscii(field, ppcOutBuf_, uiMyBufferBytesRemaining_) || !PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", OEM4_ABBREV_ASCII_SEPARATOR))
      {
         return false;
      }
   }
   return true;
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::FieldToAscii(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   const char* pcConvertString = fc_.field_def->conversion.c_str();
   char acNewConvertString[7];

   switch (fc_.field_def->conversionStripped)
   {
   // Conversion string says it's a string, but it may not be null terminated so it's being stored character by character for now.
   // Modify conversion string to %c
   case CONVERSION_STRING::s:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c",    fc_.field_def->dataType.name == DATA_TYPE_NAME::UCHAR ? std::get<uint8_t>(fc_.field_value) : std::get<int8_t>(fc_.field_value));
   case CONVERSION_STRING::m:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%s",    MsgIdToMsgName(std::get<uint32_t>(fc_.field_value)).c_str());
   case CONVERSION_STRING::T:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.3lf", std::get<uint32_t>(fc_.field_value) / 1000.0);
   case CONVERSION_STRING::UB: return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%u",    std::get<uint8_t >(fc_.field_value));
   case CONVERSION_STRING::B:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d",    std::get<int8_t  >(fc_.field_value));
   case CONVERSION_STRING::XB: return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%02x",  std::get<uint8_t >(fc_.field_value));
   case CONVERSION_STRING::id:
   {
      const uint32_t uiTempID = std::get<uint32_t>(fc_.field_value);
      const uint16_t usSV = uiTempID & 0x0000FFFF;
      const int16_t sGloChan = (uiTempID & 0xFFFF0000) >> 16;

      return !PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%u", usSV)
         ? false
         : (sGloChan != 0)
         ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%+d", sGloChan)
         : true; // is this a bug?
   }
   case CONVERSION_STRING::P:
   {
      const uint8_t uiValue = std::get<uint8_t>(fc_.field_value);
      return uiValue == '\\'
         ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\\\\")
         : ((uiValue > 32 && uiValue < 127) || uiValue == ' ')
         // just print out the character
         ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", uiValue)
         // print it out as a hex character with a () surrounding
         : PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\\x%02x", uiValue);
   }
   // is it a SUPER Float
   case CONVERSION_STRING::k:
      MakeConversionString<float>(fc_, acNewConvertString);
      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, acNewConvertString, std::get<float>(fc_.field_value));
   // is it a SUPER LONG Float
   case CONVERSION_STRING::lk:
      MakeConversionString<double>(fc_, acNewConvertString);
      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, acNewConvertString, std::get<double>(fc_.field_value));
   case CONVERSION_STRING::c:
      return fc_.field_def->dataType.length == 1
         ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", std::get<uint8_t>(fc_.field_value))
         : (fc_.field_def->dataType.length == 4 && fc_.field_def->dataType.name == DATA_TYPE_NAME::ULONG)
         ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", std::get<uint32_t>(fc_.field_value))
         : true; // is this a bug?
   default:
      switch (fc_.field_def->dataType.name)
      {
      case DATA_TYPE_NAME::BOOL:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, std::get<bool>(fc_.field_value) ? "TRUE" : "FALSE");
      case DATA_TYPE_NAME::HEXBYTE:   return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%02x",          std::get<uint8_t >(fc_.field_value));
      case DATA_TYPE_NAME::UCHAR:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<uint8_t >(fc_.field_value));
      case DATA_TYPE_NAME::CHAR:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<int8_t  >(fc_.field_value));
      case DATA_TYPE_NAME::USHORT:    return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<uint16_t>(fc_.field_value));
      case DATA_TYPE_NAME::SHORT:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<int16_t >(fc_.field_value));
      case DATA_TYPE_NAME::UINT:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<uint32_t>(fc_.field_value));
      case DATA_TYPE_NAME::INT:       return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<int32_t >(fc_.field_value));
      case DATA_TYPE_NAME::ULONG:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<uint32_t>(fc_.field_value));
      case DATA_TYPE_NAME::ULONGLONG: return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<uint64_t>(fc_.field_value));
      case DATA_TYPE_NAME::LONG:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<int32_t >(fc_.field_value));
      case DATA_TYPE_NAME::LONGLONG:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<int64_t >(fc_.field_value));
      case DATA_TYPE_NAME::FLOAT:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<float   >(fc_.field_value));
      case DATA_TYPE_NAME::DOUBLE:    return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<double  >(fc_.field_value));
      default:
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToAscii(): unknown type.");
         throw std::runtime_error("FieldToAscii(): unknown type.");
      }
   }
}

// -------------------------------------------------------------------------------------------------------
std::string Encoder::JsonHeaderToMsgName(const IntermediateHeader& stIntermediateHeader_) const
{
   // Message name
   const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(stIntermediateHeader_.usMessageID);
   std::string sMsgName(pclMessageDef ? pclMessageDef->name : GetEnumString(vMyCommandDefns, stIntermediateHeader_.usMessageID));
   uint32_t uiSiblingID = stIntermediateHeader_.ucMessageType & 0b00011111;
   if (uiSiblingID) // Append sibling i.e. the _1 of RANGEA_1
      sMsgName.append("_").append(std::to_string(uiSiblingID));

   return sMsgName;
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeJsonHeader(const IntermediateHeader& stIntermediateHeader_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   return CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "{")
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("message": "%s",)", JsonHeaderToMsgName(stIntermediateHeader_).c_str())
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("id": %hu,)", stIntermediateHeader_.usMessageID)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("port": "%s",)", GetEnumString(vMyPortAddrDefns, stIntermediateHeader_.uiPortAddress).c_str())
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("sequence_num": %hu,)", stIntermediateHeader_.usSequence)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("percent_idle_time": %.1f,)", static_cast<float>(stIntermediateHeader_.ucIdleTime) * 0.500)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("time_status": "%s",)", GetEnumString(vMyGPSTimeStatusDefns, stIntermediateHeader_.uiTimeStatus).c_str())
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("week": %hu,)", stIntermediateHeader_.usWeek)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("seconds": %.3f,)", (stIntermediateHeader_.dMilliseconds / 1000.0))
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("receiver_status": %ld,)", stIntermediateHeader_.uiReceiverStatus)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("HEADER_reserved1": %d,)", stIntermediateHeader_.uiMessageDefinitionCRC)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("receiver_sw_version": %hu)", stIntermediateHeader_.usReceiverSwVersion)
       && CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "}");
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeJsonShortHeader(const IntermediateHeader& stIntermediateHeader_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   return CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "{")
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("message": "%s",)", JsonHeaderToMsgName(stIntermediateHeader_).c_str())
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("id": %hu,)", stIntermediateHeader_.usMessageID)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("week": %hu,)", stIntermediateHeader_.usWeek)
       && PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("seconds": %.3f)", (stIntermediateHeader_.dMilliseconds / 1000.0))
       && CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "}");
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::EncodeJsonBody(const std::vector<FieldContainer>& vIntermediateFormat_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "{")) { return false; }

   for (const auto& field : vIntermediateFormat_)
   {
      if (std::holds_alternative<std::vector<FieldContainer>>(field.field_value))
      {
         const auto& vFCCurrentVectorField = std::get<std::vector<FieldContainer>>(field.field_value);

         // FIELD_ARRAY types contain several classes and so will use a recursive call
         if (field.field_def->type == FIELD_TYPE::FIELD_ARRAY)
         {
            if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": [)", field.field_def->name.c_str())) { return false; }
            const auto& vCurrentFieldArrayField = std::get<std::vector<FieldContainer>>(field.field_value);
            if (vCurrentFieldArrayField.empty())
            {
               if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "],")) { return false; }
            }
            else
            {
               for (const auto& clFieldArray : vCurrentFieldArrayField)
               {
                  if (!EncodeJsonBody(std::get<std::vector<FieldContainer>>(clFieldArray.field_value), ppcOutBuf_, uiMyBufferBytesRemaining_)) { return false; }
                  if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, ",")) { return false; }
               }
               *(*ppcOutBuf_ - 1) = ']';
               if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, ",")) { return false; }
            }
         }
         else
         {
            const bool bPrintAsString = PrintAsString(field.field_def);

            if (bPrintAsString)
            {
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": ")", field.field_def->name.c_str())) { return false; }
            }
            else
            {
               // This is an array of simple elements
               if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": [)", field.field_def->name.c_str())) { return false; }
               if (vFCCurrentVectorField.empty())
               {
                  if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "]")) { return false; }
               }
            }

            for (const auto& arrayField : vFCCurrentVectorField)
            {
               // If we are printing a string, don't print the null terminator or any padding bytes
               if (bPrintAsString)
               {
                  if (std::holds_alternative<int8_t>(arrayField.field_value)
                     && std::get<int8_t>(arrayField.field_value) == '\0')
                  {
                     break;
                  }
                  if (std::holds_alternative<uint8_t>(arrayField.field_value)
                     && std::get<uint8_t>(arrayField.field_value) == '\0')
                  {
                     break;
                  }
               }

               if (!FieldToJson(arrayField, ppcOutBuf_, uiMyBufferBytesRemaining_)) { return false; }
               if (!bPrintAsString && !CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, ",")) { return false; }
            }

            // Quoted elements need a trailing comma
            if (bPrintAsString)
            {
               if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, "\",")) { return false; }
            }
            // Non-quoted, non-internally-separated elements also need a trailing comma
            else
            {
               *(*ppcOutBuf_ - 1) = ']';
               if (!CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, ",")) { return false; }
            }
         }
      }
      else if (field.field_def->type == FIELD_TYPE::STRING)
      {
         // STRING types can be handled all at once because they show up as a single element and
         // are guaranteed to have a null terminator
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": "%s",)", field.field_def->name.c_str(), std::get<std::string>(field.field_value).c_str())) { return false; }
      }
      else if (field.field_def->type == FIELD_TYPE::ENUM)
      {
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": "%s",)", field.field_def->name.c_str(), GetEnumString(dynamic_cast<const EnumField*>(field.field_def)->enumDef, std::get<int32_t>(field.field_value)).c_str())) { return false; }
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_ID)
      {
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": %d,)", field.field_def->name.c_str(), std::get<int32_t>(field.field_value)))
         {
            return false;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::RESPONSE_STR)
      {
         if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": "%s",)", field.field_def->name.c_str(), std::get<std::string>(field.field_value).c_str()))
         {
            return false;
         }
      }
      // Handle individual simple element
      else if (!PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s": )", field.field_def->name.c_str())
            || !FieldToJson(field, ppcOutBuf_, uiMyBufferBytesRemaining_)
            || !CopyToBuffer(reinterpret_cast<unsigned char**>(ppcOutBuf_), uiMyBufferBytesRemaining_, ","))
      {
         return false;
      }
   }
   *(*ppcOutBuf_ - 1) = '}';
   return true;
}

// -------------------------------------------------------------------------------------------------------
bool
Encoder::FieldToJson(const FieldContainer& fc_, char** ppcOutBuf_, uint32_t& uiMyBufferBytesRemaining_)
{
   const char* pcConvertString = fc_.field_def->conversion.c_str();
   char acNewConvertString[7];

   switch (fc_.field_def->conversionStripped)
   {
   //IS it a char field that we want to output as a hex byte?
   case CONVERSION_STRING::XB: return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hhu",  std::get<uint8_t >(fc_.field_value));
   case CONVERSION_STRING::T:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%.3lf", std::get<uint32_t>(fc_.field_value) / 1000.0);
   case CONVERSION_STRING::UB: return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%u",    std::get<uint8_t >(fc_.field_value));
   case CONVERSION_STRING::B:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d",    std::get<int8_t  >(fc_.field_value));
   case CONVERSION_STRING::P:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hhu",  std::get<uint8_t >(fc_.field_value));
   case CONVERSION_STRING::m:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%s")", MsgIdToMsgName(std::get<uint32_t>(fc_.field_value)).c_str());
   case CONVERSION_STRING::id:
   {
      const auto uiTempID = std::get<uint32_t>(fc_.field_value);
      const uint16_t usSV = uiTempID & 0x0000FFFF;
      const int16_t sGloChan = (uiTempID & 0xFFFF0000) >> 16;

      if (sGloChan)
      {
         // The sign will only be applied to the string in the case that sGloChan is negative.
         if (sGloChan < 0)
         {
            return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%u%d")", usSV, sGloChan);
         }
         return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%u+%d")", usSV, sGloChan);
      }
      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, R"("%u")", usSV);
   }
   case CONVERSION_STRING::k: //IS it a SUPER Float
      MakeConversionString<float>(fc_, acNewConvertString);
      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, acNewConvertString, std::get<float>(fc_.field_value));
   case CONVERSION_STRING::lk: //IS it a SUPER LONG Float
      MakeConversionString<double>(fc_, acNewConvertString);
      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, acNewConvertString, std::get<double>(fc_.field_value));
   // Conversion string says it's a string, but it may not be null terminated so it's being stored character by character for now.
   // Modify conversion string to %c
   case CONVERSION_STRING::s: return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%c", (fc_.field_def->dataType.name == DATA_TYPE_NAME::UCHAR) ? std::get<uint8_t>(fc_.field_value) : std::get<int8_t>(fc_.field_value));
   case CONVERSION_STRING::c:
      return (fc_.field_def->dataType.length == 1)
         ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\"%c\"", std::get<uint8_t>(fc_.field_value))
         : (fc_.field_def->dataType.length == 4 && fc_.field_def->dataType.name == DATA_TYPE_NAME::ULONG)
         ? PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "\"%c\"", std::get<uint32_t>(fc_.field_value))
         : true; // is this a bug?
   default:
      switch (fc_.field_def->dataType.name)
      {
      case DATA_TYPE_NAME::BOOL:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, std::get<bool>(fc_.field_value) ? "true" : "false");
      case DATA_TYPE_NAME::HEXBYTE:   return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hhu",          std::get<uint8_t >(fc_.field_value));
      case DATA_TYPE_NAME::UCHAR:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hhu",          std::get<uint8_t >(fc_.field_value));
      case DATA_TYPE_NAME::CHAR:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hhd",          std::get<int8_t  >(fc_.field_value));
      case DATA_TYPE_NAME::USHORT:    return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hu",           std::get<uint16_t>(fc_.field_value));
      case DATA_TYPE_NAME::SHORT:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%hd",           std::get<int16_t >(fc_.field_value));
      case DATA_TYPE_NAME::UINT:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%u",            std::get<uint32_t>(fc_.field_value));
      case DATA_TYPE_NAME::INT:       return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d",            std::get<int32_t >(fc_.field_value));
      case DATA_TYPE_NAME::ULONG:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%u",            std::get<uint32_t>(fc_.field_value));
      case DATA_TYPE_NAME::LONG:      return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%d",            std::get<int32_t >(fc_.field_value));
      case DATA_TYPE_NAME::ULONGLONG: return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%llu",          std::get<uint64_t>(fc_.field_value));
      case DATA_TYPE_NAME::LONGLONG:  return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, "%lld",          std::get<int64_t >(fc_.field_value));
      case DATA_TYPE_NAME::FLOAT:     return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<float   >(fc_.field_value));
      case DATA_TYPE_NAME::DOUBLE:    return PrintToBuffer(ppcOutBuf_, uiMyBufferBytesRemaining_, pcConvertString, std::get<double  >(fc_.field_value));
      default:
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, "FieldToJson(): unknown type.");
         throw std::runtime_error("FieldToJson(): unknown type.");
      }
   }
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::Encode(
   unsigned char** ppucEncodeBuffer_,
   uint32_t uiEncodeBufferSize_,
   IntermediateHeader& stHeader_,
   IntermediateMessage& stMessage_,
   MessageDataStruct& stMessageData_,
   MetaDataStruct& stMetaData_,
   ENCODEFORMAT eEncodeFormat_)
{
   if (ppucEncodeBuffer_ == nullptr || *ppucEncodeBuffer_ == nullptr)
   {
      return STATUS::NULL_PROVIDED;
   }

   if (!pclMyMsgDb)
   {
      return STATUS::NO_DATABASE;
   }

   STATUS eStatus = STATUS::UNKNOWN;
   unsigned char* pucTempEncodeBuffer = *ppucEncodeBuffer_;

   if (eEncodeFormat_ == ENCODEFORMAT::JSON)
   {
      if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_, R"({"header": )"))
      {
         return STATUS::BUFFER_FULL;
      }
   }

   eStatus = EncodeHeader(&pucTempEncodeBuffer, uiEncodeBufferSize_, stHeader_, stMessageData_, stMetaData_, eEncodeFormat_);
   if (eStatus != STATUS::SUCCESS)
   {
      return eStatus;
   }

   pucTempEncodeBuffer += stMessageData_.uiMessageHeaderLength;

   if (eEncodeFormat_ == ENCODEFORMAT::JSON)
   {
      if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_, R"(,"body": )"))
      {
         return STATUS::BUFFER_FULL;
      }
   }

   eStatus = EncodeBody(&pucTempEncodeBuffer, uiEncodeBufferSize_, stMessage_, stMessageData_, stMetaData_, eEncodeFormat_);
   if (eStatus != STATUS::SUCCESS)
   {
      return eStatus;
   }

   pucTempEncodeBuffer += stMessageData_.uiMessageBodyLength;

   if (eEncodeFormat_ == ENCODEFORMAT::JSON)
   {
      if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_, R"(})"))
      {
         return STATUS::BUFFER_FULL;
      }
   }

   stMessageData_.pucMessage = *ppucEncodeBuffer_;
   stMessageData_.uiMessageLength = static_cast<uint32_t>(pucTempEncodeBuffer - *ppucEncodeBuffer_);

   return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::EncodeHeader(
   unsigned char** ppucEncodeBuffer_,
   uint32_t uiEncodeBufferSize_,
   IntermediateHeader& stHeader_,
   MessageDataStruct& stMessageData_,
   MetaDataStruct& stMetaData_,
   ENCODEFORMAT eEncodeFormat_,
   bool bIsEmbeddedHeader_)
{
   if (ppucEncodeBuffer_ == nullptr || *ppucEncodeBuffer_ == nullptr)
   {
      return STATUS::NULL_PROVIDED;
   }

   if (!pclMyMsgDb)
   {
      return STATUS::NO_DATABASE;
   }

   unsigned char* pucTempEncodeBuffer = *ppucEncodeBuffer_;

   switch (eEncodeFormat_)
   {
      case ENCODEFORMAT::ASCII:
         if ((stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_BINARY
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_ABB_ASCII)
            ? !EncodeAsciiShortHeader(stHeader_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_)
            : !EncodeAsciiHeader     (stHeader_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_))
         {
            return STATUS::BUFFER_FULL;
         }
         break;
      case ENCODEFORMAT::ABBREV_ASCII:
         if ((stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_BINARY
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_ABB_ASCII)
            ? !EncodeAbbrevAsciiShortHeader(stHeader_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_)
            : !EncodeAbbrevAsciiHeader     (stHeader_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_, bIsEmbeddedHeader_))
         {
            return STATUS::BUFFER_FULL;
         }
         break;
      case ENCODEFORMAT::FLATTENED_BINARY:
         [[fallthrough]];
      case ENCODEFORMAT::BINARY:
         if ((stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_BINARY
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_ABB_ASCII)
            ? !EncodeBinaryShortHeader(stHeader_, &pucTempEncodeBuffer, uiEncodeBufferSize_)
            : !EncodeBinaryHeader     (stHeader_, &pucTempEncodeBuffer, uiEncodeBufferSize_))
         {
            return STATUS::BUFFER_FULL;
         }
         break;
      case ENCODEFORMAT::JSON:
         if ((stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_BINARY
           || stMetaData_.eFormat == HEADERFORMAT::SHORT_ABB_ASCII)
            ? !EncodeJsonShortHeader(stHeader_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_)
            : !EncodeJsonHeader     (stHeader_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_))
         {
            return STATUS::BUFFER_FULL;
         }
         break;
      default:
         return STATUS::UNSUPPORTED;
   }

   // Record the length of the encoded message header.
   stMessageData_.pucMessageHeader = *ppucEncodeBuffer_;
   stMessageData_.uiMessageHeaderLength = static_cast<uint32_t>(pucTempEncodeBuffer - stMessageData_.pucMessageHeader);
   return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
STATUS
Encoder::EncodeBody(
   unsigned char** ppucEncodeBuffer_,
   uint32_t uiEncodeBufferSize_,
   IntermediateMessage& stMessage_,
   MessageDataStruct& stMessageData_,
   MetaDataStruct& stMetaData_,
   ENCODEFORMAT eEncodeFormat_)
{
   if (ppucEncodeBuffer_ == nullptr || *ppucEncodeBuffer_ == nullptr)
   {
      return STATUS::NULL_PROVIDED;
   }

   if (!pclMyMsgDb)
   {
      return STATUS::NO_DATABASE;
   }

   uint32_t uiCRC;
   unsigned char* pucTempEncodeBuffer = *ppucEncodeBuffer_;

   OEM4BinaryHeader* pstTempBinaryHeader;
   OEM4BinaryShortHeader* pstTempBinaryShortHeader;

   switch (eEncodeFormat_)
   {
      case ENCODEFORMAT::ASCII:
         if (!EncodeAsciiBody(stMessage_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_))
         {
            return STATUS::BUFFER_FULL;
         }

         pucTempEncodeBuffer--; // Remove last delimiter ','
         uiCRC = CalculateBlockCRC32(
            static_cast<uint32_t>(pucTempEncodeBuffer - stMessageData_.pucMessageHeader + 1 - 2), // + 1 to avoid sync character
            0,
            stMessageData_.pucMessageHeader + 1); // + 1 to avoid sync '%' or '#'.

         if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_, "*%08x\r\n", uiCRC))
         {
            return STATUS::BUFFER_FULL;
         }
         break;

      case ENCODEFORMAT::ABBREV_ASCII:
         if (!EncodeAbbrevAsciiBody(stMessage_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_))
         {
            return STATUS::BUFFER_FULL;
         }

         pucTempEncodeBuffer--; // Remove last delimiter ' '
         if (!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_, "\r\n"))
         {
            return STATUS::BUFFER_FULL;
         }
         break;

      case ENCODEFORMAT::FLATTENED_BINARY:
         [[fallthrough]];
      case ENCODEFORMAT::BINARY:
         if (!EncodeBinaryBody(stMessage_, &pucTempEncodeBuffer, uiEncodeBufferSize_, eEncodeFormat_ == ENCODEFORMAT::FLATTENED_BINARY))
         {
            return STATUS::BUFFER_FULL;
         }

         // MessageData must have a valid MessageHeader pointer to populate the length field.
         if (!stMessageData_.pucMessageHeader)
         {
            return STATUS::FAILURE;
         }

         // Go back and set the length field in the header.
         if (stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII
          || stMetaData_.eFormat == HEADERFORMAT::SHORT_BINARY
          || stMetaData_.eFormat == HEADERFORMAT::SHORT_ABB_ASCII)
         {
            pstTempBinaryShortHeader = reinterpret_cast<OEM4BinaryShortHeader*>(stMessageData_.pucMessageHeader);
            pstTempBinaryShortHeader->ucLength = static_cast<uint8_t>(pucTempEncodeBuffer - (*ppucEncodeBuffer_));
         }
         else
         {
            pstTempBinaryHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData_.pucMessageHeader);
            pstTempBinaryHeader->usLength = static_cast<uint16_t>(pucTempEncodeBuffer - (*ppucEncodeBuffer_));
         }

         uiCRC = CalculateBlockCRC32(static_cast<uint32_t>(pucTempEncodeBuffer - stMessageData_.pucMessageHeader), 0, (stMessageData_.pucMessageHeader));
         if (!CopyToBuffer(&pucTempEncodeBuffer, uiEncodeBufferSize_, &uiCRC))
         {
            return STATUS::BUFFER_FULL;
         }
         break;

      case ENCODEFORMAT::JSON:
         if (!EncodeJsonBody(stMessage_, reinterpret_cast<char**>(&pucTempEncodeBuffer), uiEncodeBufferSize_))
         {
            return STATUS::BUFFER_FULL;
         }
         break;

      default:
         return STATUS::UNSUPPORTED;
   }

   // Record the length of the encoded message body.
   stMessageData_.pucMessageBody = *ppucEncodeBuffer_;
   stMessageData_.uiMessageBodyLength = static_cast<uint32_t>(pucTempEncodeBuffer - stMessageData_.pucMessageBody);
   return STATUS::SUCCESS;
}
