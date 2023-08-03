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
//! \file message_decoder.cpp
//! \brief Decode OEM message bodies.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/message_decoder.hpp"
#include <bitset>
#include <sstream>

using namespace novatel::edie;
using namespace novatel::edie::oem;

// -------------------------------------------------------------------------------------------------------
MessageDecoder::MessageDecoder(JsonReader* pclJsonDb_)
{
   pclMyLogger = Logger().RegisterLogger("novatel_message_decoder");

   pclMyLogger->debug("MessageDecoder initializing...");
   if (pclJsonDb_ != nullptr)
   {
      LoadJsonDb(pclJsonDb_);
   }
   pclMyLogger->debug("MessageDecoder initialized");
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::LoadJsonDb(JsonReader* pclJsonDb_)
{
   pclMyMsgDb = pclJsonDb_;

   InitEnumDefns();
   CreateResponseMsgDefns();
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger>
MessageDecoder::GetLogger()
{
   return pclMyLogger;
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::SetLoggerLevel(spdlog::level::level_enum eLevel_)
{
   pclMyLogger->set_level(eLevel_);
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::ShutdownLogger()
{
   Logger::Shutdown();
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::InitEnumDefns()
{
   vMyRespDefns          = pclMyMsgDb->GetEnumDef("Responses");
   vMyCommandDefns       = pclMyMsgDb->GetEnumDef("Commands");
   vMyPortAddrDefns      = pclMyMsgDb->GetEnumDef("PortAddress");
   vMyGPSTimeStatusDefns = pclMyMsgDb->GetEnumDef("GPSTimeStatus");
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::CreateResponseMsgDefns()
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
   if (vMyRespDefns != nullptr)
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
   stMyRespDef.fields[0]; // responses don't have CRCs, hardcoding in 0 as the key to the fields map
   stMyRespDef.fields[0].push_back(stRespIdField.clone());
   stMyRespDef.fields[0].push_back(stRespStrField.clone());
}

// -------------------------------------------------------------------------------------------------------
uint32_t
MessageDecoder::MsgNameToMsgId(std::string sMsgName_)
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
      if (sTemp.back() == 'R')
      {
         uiResponse  = static_cast<uint32_t>(true);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
         sTemp.pop_back();
      }
      else if (sTemp.back() == 'A')
      {
         uiResponse  = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
         sTemp.pop_back();
      }
      else if (sTemp.back() == 'B')
      {
         uiResponse  = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::BINARY);
         sTemp.pop_back();
      }
      else
      {
         uiResponse  = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ABBREV);
      }

      pclMessageDef = pclMyMsgDb->GetMsgDef(sTemp);
      return CreateMsgID(pclMessageDef ? pclMessageDef->logID : GetEnumValue(vMyCommandDefns, sTemp), uiSiblingID, uiMsgFormat, uiResponse);
}

// -------------------------------------------------------------------------------------------------------
std::string
MessageDecoder::MsgIdToMsgName(const uint32_t uiMessageID_)
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
      : uiMessageFormat == static_cast<uint32_t>(MESSAGEFORMAT::ASCII ) ? "A"
      : ""; // default to abbreviated ascii format

   if (uiSiblingID)
      strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingID));

   return strMessageName.append(strMessageFormatSuffix);
}

// -------------------------------------------------------------------------------------------------------
MsgFieldsVector*
MessageDecoder::GetMsgDefFromCRC(const MessageDefinition* pclMessageDef_, uint32_t& uiMsgDefCRC_)
{
   // If we can't find the correct CRC just default to the latest.
   if (pclMessageDef_->fields.count(uiMsgDefCRC_) == 0)
   {
      pclMyLogger->info("Log DB is missing the log definition {} - {}.  Defaulting to newest version fo the log definition.", pclMessageDef_->name, uiMsgDefCRC_);
      uiMsgDefCRC_ = pclMessageDef_->latestMessageCrc;
   }
   return &pclMessageDef_->fields.at(uiMsgDefCRC_);
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoder::DecodeBinary(const std::vector<BaseField*> MsgDefFields_, unsigned char** ppucLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_, uint32_t uiMessageLength_)
{
   STATUS eStatus = STATUS::SUCCESS;
   unsigned char* pucTempStart = *ppucLogBuf_;
   for (auto& field : MsgDefFields_)
   {
      // Realign to type byte boundry if needed
      uint8_t usTypeAlightment = field->dataType.length >= 4 ? 4 : field->dataType.length;
      if (reinterpret_cast<uint64_t>(*ppucLogBuf_) % usTypeAlightment != 0)
      {
         *ppucLogBuf_ += usTypeAlightment - (reinterpret_cast<uint64_t>(*ppucLogBuf_) % usTypeAlightment);
      }

      if (field->type == FIELD_TYPE::SIMPLE)
      {
         DecodeBinaryField(field, ppucLogBuf_, vIntermediateFormat_);
      }
      else if (field->type == FIELD_TYPE::ENUM)
      {
         vIntermediateFormat_.emplace_back(*reinterpret_cast<std::int32_t*>(*ppucLogBuf_), field);
         *ppucLogBuf_ += sizeof(int32_t);
      }
      else if (field->type == FIELD_TYPE::RESPONSE_ID)
      {
         vIntermediateFormat_.emplace_back(*reinterpret_cast<std::int32_t*>(*ppucLogBuf_), field);
         *ppucLogBuf_ += sizeof(int32_t);
      }
      else if (field->type == FIELD_TYPE::RESPONSE_STR)
      {
         std::string sTemp(reinterpret_cast<char*>(*ppucLogBuf_), uiMessageLength_ - sizeof(int32_t)); // Remove CRC
         vIntermediateFormat_.emplace_back(sTemp, field);
         // Binary response string is not null terminated or 4 byte aligned
         *ppucLogBuf_ += sTemp.size();
      }
      else if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
      {
         uint32_t uiArraySize = static_cast<const ArrayField*>(field)->arrayLength;
         vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);

         auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
         pvFC.reserve(uiArraySize);

         for (uint32_t i = 0; i < uiArraySize; ++i)
            DecodeBinaryField(field, ppucLogBuf_, pvFC);
      }
      else if (field->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
      {
         auto uiArraySize = *reinterpret_cast<std::uint32_t*>(*ppucLogBuf_);;
         *ppucLogBuf_ += sizeof(uint32_t);

         vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
         auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
         pvFC.reserve(uiArraySize);

         for (uint32_t i = 0; i < uiArraySize; ++i)
         {
            DecodeBinaryField(field, ppucLogBuf_, pvFC);
         }
      }
      else if (field->type == FIELD_TYPE::STRING)
      {
         // This version of a string is different. It is hopefully null terminated.
         std::string sTemp(reinterpret_cast<char*>(*ppucLogBuf_));
         vIntermediateFormat_.emplace_back(sTemp, field);
         *ppucLogBuf_ += sTemp.size() + 1; // + 1 to consume the NULL at the end of the string. This is to maintain byte alignment.

         if (reinterpret_cast<std::uint64_t>(*ppucLogBuf_) % 4 != 0)
         {
            *ppucLogBuf_ += 4 - reinterpret_cast<std::uint64_t>(*ppucLogBuf_) % 4;
         }
      }
      else if (field->type == FIELD_TYPE::FIELD_ARRAY)
      {
         auto* puiArraySize = reinterpret_cast<std::uint32_t*>(*ppucLogBuf_);
         *ppucLogBuf_ += sizeof(int32_t);

         auto* sub_field_defs = static_cast<FieldArrayField*>(field);

         vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
         auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
         pvFieldArrayContainer.reserve(*puiArraySize);

         for (uint32_t i = 0; i < *puiArraySize; ++i)
         {
            pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFC = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().field_value);
            pvFC.reserve((static_cast<const FieldArrayField*>(field))->fields.size());

            eStatus = DecodeBinary(sub_field_defs->fields, ppucLogBuf_, pvFC, uiMessageLength_ - static_cast<uint32_t>(*ppucLogBuf_ - pucTempStart));
         }
      }
      else
      {
         std::string sError = "DecodeBinary(): Unknown field type\n";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
      if (*ppucLogBuf_ - pucTempStart  >= static_cast<int32_t>(uiMessageLength_))
         break;
   }

   return eStatus;
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::DecodeBinaryField(const BaseField* MessageDataType_, unsigned char** ppucLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_)
{
   switch (MessageDataType_->conversionStripped)
   {
   case CONVERSION_STRING::m:  [[fallthrough]];
   // Time type. Float milliseconds time type thats had the decimal shifted to the right and then converted to a uint32_t
   case CONVERSION_STRING::T:  [[fallthrough]];
   case CONVERSION_STRING::id: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint32_t*>(*ppucLogBuf_), MessageDataType_); break;
   case CONVERSION_STRING::XB: [[fallthrough]];
   case CONVERSION_STRING::P:  [[fallthrough]];
   case CONVERSION_STRING::Z:  vIntermediateFormat_.emplace_back(*reinterpret_cast<uint8_t *>(*ppucLogBuf_), MessageDataType_); break;
   case CONVERSION_STRING::k:  vIntermediateFormat_.emplace_back(*reinterpret_cast<float   *>(*ppucLogBuf_), MessageDataType_); break;
   case CONVERSION_STRING::lk: vIntermediateFormat_.emplace_back(*reinterpret_cast<double  *>(*ppucLogBuf_), MessageDataType_); break;
   default:
      switch (MessageDataType_->dataType.name)
      {
      case DATA_TYPE_NAME::BOOL:      vIntermediateFormat_.emplace_back(*reinterpret_cast<bool    *>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::HEXBYTE:   [[fallthrough]];
      case DATA_TYPE_NAME::UCHAR:     vIntermediateFormat_.emplace_back(*reinterpret_cast<uint8_t *>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::CHAR:      vIntermediateFormat_.emplace_back(*reinterpret_cast<int8_t  *>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::USHORT:    vIntermediateFormat_.emplace_back(*reinterpret_cast<uint16_t*>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::SHORT:     vIntermediateFormat_.emplace_back(*reinterpret_cast<int16_t *>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::UINT:      [[fallthrough]];
      case DATA_TYPE_NAME::ULONG:     vIntermediateFormat_.emplace_back(*reinterpret_cast<uint32_t*>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::INT:       [[fallthrough]];
      case DATA_TYPE_NAME::LONG:      vIntermediateFormat_.emplace_back(*reinterpret_cast<int32_t *>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::ULONGLONG: vIntermediateFormat_.emplace_back(*reinterpret_cast<uint64_t*>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::LONGLONG:  vIntermediateFormat_.emplace_back(*reinterpret_cast<int64_t *>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::FLOAT:     vIntermediateFormat_.emplace_back(*reinterpret_cast<float   *>(*ppucLogBuf_), MessageDataType_); break;
      case DATA_TYPE_NAME::DOUBLE:    vIntermediateFormat_.emplace_back(*reinterpret_cast<double  *>(*ppucLogBuf_), MessageDataType_); break;
      default:
         std::string sError = "DecodeBinaryField(): Unknown field type\n";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }

   *ppucLogBuf_ += MessageDataType_->dataType.length;
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoder::DecodeAscii(const std::vector<BaseField*> MsgDefFields_, char** ppucLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_)
{
   STATUS eStatus = STATUS::SUCCESS;

   bool bEarlyEndOfMessage = false;
   for (auto& field : MsgDefFields_)
   {
      size_t tokenLength = strcspn(*ppucLogBuf_, ",*");
      if (static_cast<int8_t>(*(*ppucLogBuf_+tokenLength)) == '*')
         bEarlyEndOfMessage = true;

      if (field->type == FIELD_TYPE::SIMPLE)
      {
         DecodeAsciiField(field, ppucLogBuf_, tokenLength, vIntermediateFormat_);
         *ppucLogBuf_ += tokenLength + 1;
      }
      else if (field->type == FIELD_TYPE::ENUM)
      {
         std::string sEnum = std::string(*ppucLogBuf_, tokenLength);
         vIntermediateFormat_.emplace_back(GetEnumValue(static_cast<EnumField*>(field)->enumDef, sEnum), field);
         *ppucLogBuf_ += tokenLength + 1;
      }
      else if (field->type == FIELD_TYPE::STRING) // Handle string type directly
      {
         // It may be possible that there is a field delimiter character in the string, meaning the previous tokenLength value is invalid.
         tokenLength = strcspn(*ppucLogBuf_ + 1, "\"*"); // Look for LAST '\"' character, skipping past the first.
         std::string sTemp(*ppucLogBuf_ + 1, tokenLength); // +1 to traverse opening double-quote.
         vIntermediateFormat_.emplace_back(sTemp, field);
         // Skip past the first '\"', string token and the remaining characters ('\"' and ',').
         *ppucLogBuf_ += 1 + tokenLength + strcspn(*ppucLogBuf_ + tokenLength, ",*");
      }
      else if (field->type == FIELD_TYPE::RESPONSE_ID)
      {
         // Ensure we get the whole response (skip over ' ' delimiters in responses)
         tokenLength = strcspn(*ppucLogBuf_, "*");
         std::string sResponse(*ppucLogBuf_, tokenLength);
         if (sResponse == "OK")
         {
            vIntermediateFormat_.emplace_back(1, field);
         }
         else
         {
            // Remove the "ERROR:" prefix from the response
            std::string sRespDesc = sResponse.substr(OEM4_ERROR_PREFIX_LENGTH, sResponse.length() - OEM4_ERROR_PREFIX_LENGTH);
            // Note: This won't match responses with format specifiers in them (%d, %s, etc), they will be given id=0
            vIntermediateFormat_.emplace_back( GetResponseId(vMyRespDefns, sRespDesc), field);
         }
         // Do not advance buffer, need to reprocess this field for the following RESPONSE_STR.
         bEarlyEndOfMessage = false; // Need to reprocess this field
      }
      else if (field->type == FIELD_TYPE::RESPONSE_STR)
      {
         // Response strings aren't surrounded by double quotes
         // Ensure we get the whole response (skip over ' ' delimiters in responses)
         tokenLength = strcspn(*ppucLogBuf_, "*");
         std::string sTemp(*ppucLogBuf_, tokenLength);
         vIntermediateFormat_.emplace_back(sTemp, field);
         *ppucLogBuf_ += tokenLength + 1;
      }
      else if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY || field->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
      {
         uint32_t uiArraySize = 0;
         if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
         {
            uiArraySize = static_cast<const ArrayField*>(field)->arrayLength;
         }
         if (field->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
         {
            uiArraySize = static_cast<uint32_t>(strtoul(*ppucLogBuf_, nullptr, 10));
            *ppucLogBuf_ += tokenLength + 1;
            tokenLength = strcspn(*ppucLogBuf_, ",*");
         }

         vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
         auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
         pvFC.reserve(uiArraySize);

         bool bPrintAsString =
               field->type == FIELD_TYPE::STRING
            || field->conversionStripped == CONVERSION_STRING::s || field->conversionStripped == CONVERSION_STRING::S;

         bool bIsCommaSeperated =
            !( bPrintAsString
            || field->conversionStripped == CONVERSION_STRING::Z || field->conversionStripped == CONVERSION_STRING::P);

         char* pcPosition = *ppucLogBuf_;
         if (bPrintAsString)
         {
            // Ensure we grabbed the whole string, it might contain delimiters
            tokenLength = strcspn(*ppucLogBuf_ + 1, "\"*");
            tokenLength+=2; // Add the back in the quotes so we process them
            pcPosition++; // Start of string, skip first double-quote
         }

         for (uint32_t i = 0; i < uiArraySize; ++i)
         {
            if (field->conversionStripped == CONVERSION_STRING::Z)
            {
               uint32_t uiValueRead = 0;
               if (sscanf(pcPosition, "%02x", &uiValueRead) != 1)
               {
                  std::string sError = "DecodeAscii(): Error decoding %Z Array";
                  SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
                  throw std::runtime_error(sError);
               }
               pcPosition += 2;
               pvFC.emplace_back(static_cast<uint8_t>(uiValueRead), field);
            }
            // End of string, remove trailing double-quote
            else if (bPrintAsString && strncmp("\"", pcPosition, 1) == 0)
            {
               for (uint32_t j = 0; j < uiArraySize - i; j++)
               {
                  pvFC.emplace_back(static_cast<uint8_t>(0), field);
               }
               break;
            }
            // Escaped '\' character
            else if (strncmp("\\\\", pcPosition, 2) == 0)
            {
               pcPosition++; // Consume the escape char
               pvFC.emplace_back(static_cast<uint8_t>(*pcPosition), field);
               pcPosition++; // Consume 1 char
            }
            // Non-ascii char in hex e.g. \x0C
            else if (strncmp("\\x", pcPosition, 2) == 0)
            {
               pcPosition += 2; // Consume the '\x' that signifies hex without a char representation

               uint32_t uiValueRead = 0;
               if (sscanf(pcPosition, "%02x", &uiValueRead) != 1)
               {
                  std::string sError = "DecodeAscii(): Error decoding %s array";
                  SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
                  throw std::runtime_error(sError);
               }
               pcPosition += 2; // Consume the hex output that is always 2 chars

               pvFC.emplace_back(static_cast<uint8_t>(uiValueRead), field);
            }
            else
            {
               // Ascii character
               if (!bIsCommaSeperated)
               {
                  pvFC.emplace_back(static_cast<uint8_t>(*pcPosition), field);
                  pcPosition++; // Consume 1 char
               }
               // Simple type
               else
               {
                  tokenLength = strcspn(*ppucLogBuf_, ",*");
                  DecodeAsciiField(field, ppucLogBuf_, tokenLength, pvFC);
                  *ppucLogBuf_ += tokenLength + 1;
               }
            }
         }
         if (!bIsCommaSeperated)
            *ppucLogBuf_ += tokenLength + 1;
      }
      else if (field->type == FIELD_TYPE::FIELD_ARRAY)
      {
         auto uiArraySize = static_cast<uint32_t>(strtoul(*ppucLogBuf_, ppucLogBuf_, 10));
         *ppucLogBuf_ += 1;

         auto* sub_field_defs = static_cast<FieldArrayField*>(field);

         vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
         auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
         pvFieldArrayContainer.reserve(uiArraySize);

         for (uint32_t i = 0; i < uiArraySize; ++i)
         {
            pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvsubFC = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().field_value);
            pvsubFC.reserve((static_cast<const FieldArrayField*>(field))->fields.size());

            eStatus = DecodeAscii(sub_field_defs->fields, ppucLogBuf_, pvsubFC);
         }
      }
      else
      {
         std::string sError = "DecodeAscii(): Unknown field type";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }

      if (bEarlyEndOfMessage)
         break;
   }

   return eStatus;
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoder::DecodeAbbrevAscii(const std::vector<BaseField*> MsgDefFields_, char** ppucLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_)
{
   STATUS eStatus = STATUS::SUCCESS;

   try
   {
      for (auto& field : MsgDefFields_)
      {
         size_t tokenLength = strcspn(*ppucLogBuf_, " \r\n");
         if (ConsumeAbbrevFormatting(tokenLength, ppucLogBuf_))
            tokenLength = strcspn(*ppucLogBuf_, " \r\n");
         if (tokenLength == 0) // We encountered the end of the buffer unexpectedly
            return STATUS::MALFORMED_INPUT;

         if (field->type == FIELD_TYPE::SIMPLE)
         {
            DecodeAsciiField(field, ppucLogBuf_, tokenLength, vIntermediateFormat_);
            *ppucLogBuf_ += tokenLength + 1;
         }
         else if (field->type == FIELD_TYPE::ENUM)
         {
            std::string sEnum = std::string(*ppucLogBuf_, tokenLength);
            vIntermediateFormat_.emplace_back(GetEnumValue(static_cast<EnumField*>(field)->enumDef, sEnum), field);
            *ppucLogBuf_ += tokenLength + 1;
         }
         else if (field->type == FIELD_TYPE::STRING) // Handle string type directly
         {
            // It may be possible that there is a field delimiter character in the string, meaning the previous tokenLength value is invalid.
            tokenLength = strcspn(*ppucLogBuf_ + 1, "\"\r"); // Look for LAST '\"' character, skipping past the first.
            std::string sTemp(*ppucLogBuf_ + 1, tokenLength); // +1 to traverse opening double-quote.
            vIntermediateFormat_.emplace_back(sTemp, field);
            // Skip past the first '\"', string token and the remaining characters ('\"' and ',').
            *ppucLogBuf_ += 1 + tokenLength + strcspn(*ppucLogBuf_ + tokenLength, " \r");
         }
         else if (field->type == FIELD_TYPE::RESPONSE_ID)
         {
            // Ensure we get the whole response (skip over ',' and ' ' delimiters in responses)
            tokenLength = strcspn(*ppucLogBuf_, "\r");
            std::string sResponse(*ppucLogBuf_, tokenLength);
            if (sResponse == "OK")
            {
               vIntermediateFormat_.emplace_back(static_cast<int32_t>(1), field);
            }
            else
            {
               // Remove the "ERROR:" prefix from the response
               std::string sRespDesc = sResponse.substr(OEM4_ERROR_PREFIX_LENGTH, sResponse.length() - OEM4_ERROR_PREFIX_LENGTH);
               // Note: This won't match responses with format specifiers in them (%d, %s, etc), they will be given id=0
               vIntermediateFormat_.emplace_back( GetResponseId(vMyRespDefns, sRespDesc), field);
            }
            // Do not advance buffer, need to reprocess this field for the following RESPONSE_STR.
         }
         else if (field->type == FIELD_TYPE::RESPONSE_STR)
         {
            // Response strings aren't surrounded by double quotes
            // Ensure we get the whole response (skip over ' ' delimiters in responses)
            tokenLength = strcspn(*ppucLogBuf_, "\r");
            std::string sTemp(*ppucLogBuf_, tokenLength);
            vIntermediateFormat_.emplace_back(sTemp, field);
            *ppucLogBuf_ += tokenLength + 1;
         }
         else if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY || field->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
         {
            uint32_t uiArraySize = 0;
            if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY)
            {
               uiArraySize = static_cast<const ArrayField*>(field)->arrayLength;
            }
            if (field->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
            {
               uiArraySize = static_cast<uint32_t>(strtoul(*ppucLogBuf_, nullptr, 10));

               if (uiArraySize > static_cast<const ArrayField*>(field)->arrayLength)
               {
                  std::string sError = "DecodeAbbrevAscii(): Array size too large. Malformed Input\n";
                  SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
                  throw std::runtime_error(sError);
               }

               *ppucLogBuf_ += tokenLength + 1;
               tokenLength = strcspn(*ppucLogBuf_, " \r");
            }

            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFC.reserve(uiArraySize);

            bool bPrintAsString =
                  field->type == FIELD_TYPE::STRING
               || field->conversionStripped == CONVERSION_STRING::s || field->conversionStripped == CONVERSION_STRING::S;

            bool bIsCommaSeperated =
               !( bPrintAsString
               || field->conversionStripped == CONVERSION_STRING::Z || field->conversionStripped == CONVERSION_STRING::P);

            char* pcPosition = *ppucLogBuf_;
            if (bPrintAsString)
            {
               // Ensure we grabbed the whole string, it might contain delimiters
               tokenLength = strcspn(*ppucLogBuf_ + 1, "\"\r");
               tokenLength+=2; // Add the back in the quotes so we process them
               pcPosition++; // Start of string, skip first double-quote
            }

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
               if (field->conversionStripped == CONVERSION_STRING::Z)
               {
                  uint32_t uiValueRead = 0;
                  if (sscanf(pcPosition, "%02x", &uiValueRead) != 1)
                  {
                     std::string sError = "DecodeAscii(): Error decoding %Z Array";
                     SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
                     throw std::runtime_error(sError);
                  }
                  pcPosition += 2;
                  pvFC.emplace_back(static_cast<uint8_t>(uiValueRead), field);
               }
               // End of string, remove trailing double-quote
               else if (bPrintAsString && strncmp("\"", pcPosition, 1) == 0)
               {
                  for (uint32_t j = 0; j < (uiArraySize - i); j++)
                  {
                     pvFC.emplace_back(static_cast<uint8_t>(0), field);
                  }
                  break;
               }
               // Escaped '\' character
               else if (strncmp("\\\\", pcPosition, 2) == 0)
               {
                  pcPosition++; // Consume the escape char
                  pvFC.emplace_back(static_cast<uint8_t>(*pcPosition), field);
                  pcPosition++; // Consume 1 char
               }
               // Non-ascii char in hex e.g. \x0C
               else if (strncmp("\\x", pcPosition, 2) == 0)
               {
                  pcPosition += 2; // Consume the '\x' that signifies hex without a char representation

                  uint32_t uiValueRead = 0;
                  if (sscanf(pcPosition, "%02x", &uiValueRead) != 1)
                  {
                     std::string sError = "DecodeAscii(): Error decoding %s array";
                     SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
                     throw std::runtime_error(sError);
                  }
                  pcPosition += 2; // Consume the hex output that is always 2 chars

                  pvFC.emplace_back(static_cast<uint8_t>(uiValueRead), field);
               }
               else
               {
                  // Ascii character
                  if (!bIsCommaSeperated)
                  {
                     pvFC.emplace_back(static_cast<uint8_t>(*pcPosition), field);
                     pcPosition++; // Consume 1 char
                  }
                  // Simple type
                  else
                  {
                     tokenLength = strcspn(*ppucLogBuf_, " \r");
                     DecodeAsciiField(field, ppucLogBuf_, tokenLength, pvFC);
                     *ppucLogBuf_ += tokenLength + 1;
                  }
               }
            }
            if (!bIsCommaSeperated)
               *ppucLogBuf_ += tokenLength + 1;
         }
         else if (field->type == FIELD_TYPE::FIELD_ARRAY)
         {
            auto uiArraySize = static_cast<uint32_t>(strtoul(*ppucLogBuf_, ppucLogBuf_, 10));
            *ppucLogBuf_ += 1;

            auto* sub_field_defs = static_cast<FieldArrayField*>(field);
            if (uiArraySize > sub_field_defs->arrayLength)
            {
               std::string sError = "DecodeAbbrevAscii(): Array size too large. Malformed Input\n";
               SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
               throw std::runtime_error(sError);
            }

            vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
            pvFieldArrayContainer.reserve(uiArraySize);

            for (uint32_t i = 0; i < uiArraySize; ++i)
            {
               pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
               auto& pvsubFC = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().field_value);
               pvsubFC.reserve((static_cast<const FieldArrayField*>(field))->fields.size());

               eStatus = DecodeAbbrevAscii(sub_field_defs->fields, ppucLogBuf_, pvsubFC);
               if (eStatus != STATUS::SUCCESS)
                  break;
            }
         }
         else
         {
            std::string sError = "DecodeAscii(): Unknown field type";
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
            throw std::runtime_error(sError);
         }
      }
   }
   catch(const std::exception& e)
   {
      SPDLOG_LOGGER_DEBUG(pclMyLogger, e.what());
      eStatus = STATUS::MALFORMED_INPUT;
   }

   return eStatus;
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::DecodeAsciiField(const BaseField* MessageDataType_, char** ppcToken_, const size_t tokenLength_, std::vector<FieldContainer>& vIntermediateFormat_)
{
   // %d
   if (MessageDataType_->conversionStripped == CONVERSION_STRING::d
    || MessageDataType_->conversionStripped == CONVERSION_STRING::ld
    || MessageDataType_->conversionStripped == CONVERSION_STRING::hd
    || MessageDataType_->conversionStripped == CONVERSION_STRING::lld)
   {
      // bool is output using %d
      if (MessageDataType_->dataType.name == DATA_TYPE_NAME::BOOL)
      {
         std::string sToken = std::string(*ppcToken_, tokenLength_);

         if (sToken == "TRUE")
            vIntermediateFormat_.emplace_back(true, MessageDataType_);
         else if (sToken == "FALSE")
            vIntermediateFormat_.emplace_back(false, MessageDataType_);
         else
         {
            std::string sError = "DecodeAsciiField(): unknown bool conversion.";
            SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
            throw std::runtime_error(sError);
         }
      }
      else if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(static_cast<int8_t>(strtol(*ppcToken_, nullptr, 10)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 2)
         vIntermediateFormat_.emplace_back(static_cast<int16_t>(strtol(*ppcToken_, nullptr, 10)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(static_cast<int32_t>(strtol(*ppcToken_, nullptr, 10)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(static_cast<int64_t>(strtoll(*ppcToken_, nullptr, 10)), MessageDataType_);
      else
      {
         std::string sError = "DecodeAsciiField(): unknown %d conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %u
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::u
         || MessageDataType_->conversionStripped == CONVERSION_STRING::lu
         || MessageDataType_->conversionStripped == CONVERSION_STRING::hu
         || MessageDataType_->conversionStripped == CONVERSION_STRING::llu)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(static_cast<uint8_t >(strtoul (*ppcToken_, nullptr, 10)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 2)
         vIntermediateFormat_.emplace_back(static_cast<uint16_t>(strtoul (*ppcToken_, nullptr, 10)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtoul (*ppcToken_, nullptr, 10)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(static_cast<uint64_t>(strtoull(*ppcToken_, nullptr, 10)), MessageDataType_);
      else
      {
         std::string sError = "DecodeAsciiField(): unknown %u conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %x
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::x
         || MessageDataType_->conversionStripped == CONVERSION_STRING::X
         || MessageDataType_->conversionStripped == CONVERSION_STRING::lx)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(static_cast<uint8_t >(strtoul (*ppcToken_, nullptr, 16)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 2)
         vIntermediateFormat_.emplace_back(static_cast<uint16_t>(strtoul (*ppcToken_, nullptr, 16)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtoul (*ppcToken_, nullptr, 16)), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(static_cast<uint64_t>(strtoull(*ppcToken_, nullptr, 16)), MessageDataType_);
      else
      {
         std::string sError = "DecodeAsciiField(): unknown %x conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %c
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::c)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(static_cast<int8_t>(**ppcToken_), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4 && MessageDataType_->dataType.name == DATA_TYPE_NAME::ULONG)
         vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtoul(*ppcToken_, nullptr, 10)), MessageDataType_);
      else
      {
         std::string sError = "DecodeAsciiField(): unknown %c conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %uc
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::uc)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(static_cast<uint8_t>(**ppcToken_), MessageDataType_);
      else
      {
         std::string sError = "DecodeAsciiField(): unknown %uc conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %f
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::f || MessageDataType_->conversionStripped == CONVERSION_STRING::lf)
   {
      if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(strtof(*ppcToken_, nullptr), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(strtod(*ppcToken_, nullptr), MessageDataType_);
      else
      {
         std::string sError = "DecodeAsciiField(): unknown %f conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %k, %e, %g
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::k
         || MessageDataType_->conversionStripped == CONVERSION_STRING::lk
         || MessageDataType_->conversionStripped == CONVERSION_STRING::e
         || MessageDataType_->conversionStripped == CONVERSION_STRING::le
         || MessageDataType_->conversionStripped == CONVERSION_STRING::g)
   {
      if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(strtof(*ppcToken_, nullptr), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(strtod(*ppcToken_, nullptr), MessageDataType_);
      else
      {
         std::string sError = "DecodeAsciiField(): unknown floating point conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::B)
   {
      vIntermediateFormat_.emplace_back(static_cast<int8_t> (strtol (*ppcToken_, nullptr, 10)), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::UB)
   {
      vIntermediateFormat_.emplace_back(static_cast<uint8_t>(strtoul(*ppcToken_, nullptr, 10)), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::XB)
   {
      vIntermediateFormat_.emplace_back(static_cast<uint8_t>(strtoul(*ppcToken_, nullptr, 16)), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::id)
   {
      std::string sTemp(*ppcToken_, tokenLength_);

      uint16_t usSlot = 0;
      int16_t sFreq = 0;

      size_t sDelimiter = sTemp.find_last_of('+');
      if (sDelimiter == std::string::npos)
         sDelimiter = sTemp.find_last_of('-');

      if (sDelimiter != std::string::npos)
      {
         usSlot = static_cast<uint16_t>(strtoul(sTemp.substr(0, sDelimiter).c_str(), nullptr, 10));
         sFreq  = static_cast<int16_t>(strtol(sTemp.substr(sDelimiter, sTemp.length()).c_str(), nullptr, 10));
      }
      else
      {
         usSlot = static_cast<uint16_t>(strtoul(sTemp.c_str(), nullptr, 10));
         sFreq = 0;
      }

      uint32_t uiSatID = usSlot | (sFreq << 16);
      vIntermediateFormat_.emplace_back(uiSatID, MessageDataType_);
   }
   // %R
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::R)
   {
      // RXCONFIG in ASCII is always #COMMANDNAMEA
      char* pcStart = *ppcToken_ + 1; // Skip the '#'
      std::string sTemp(pcStart, tokenLength_ - 1);
      sTemp.pop_back();

      const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(sTemp);
      vIntermediateFormat_.emplace_back(pclMessageDef ? CreateMsgID(pclMessageDef->logID, 0, 1, 0) : 0, MessageDataType_);
   }
   // %ucb
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::ucb)
   {
      vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(*ppcToken_).to_ulong()), MessageDataType_);
   }
   // %m
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::m) // MsgID
   {
      std::string sMsgName(*ppcToken_, tokenLength_);
      vIntermediateFormat_.emplace_back( MsgNameToMsgId(sMsgName), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::T) // GPSTime msec field
   {
      vIntermediateFormat_.emplace_back(static_cast<uint32_t>(strtod(*ppcToken_, nullptr) * SEC_TO_MSEC), MessageDataType_);
   }
   else
   {
      char errorBuff[1024];
      snprintf(errorBuff, sizeof(errorBuff), "DecodeAsciiField() Message: %s, DataType: %d, Conversion: %s, Length %d",
         MessageDataType_->name.c_str(), static_cast<uint32_t>(MessageDataType_->dataType.name), MessageDataType_->conversion.c_str(), MessageDataType_->dataType.length);

      SPDLOG_LOGGER_CRITICAL(pclMyLogger, errorBuff);
      throw std::runtime_error(errorBuff);
   }
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoder::DecodeJson(const std::vector<BaseField*> MsgDefFields_, json clJsonFields_, std::vector<FieldContainer>& vIntermediateFormat_)
{
   STATUS eStatus = STATUS::SUCCESS;
   json clField;
   for (auto& field : MsgDefFields_)
   {
      clField = clJsonFields_[field->name];

      if (field->type == FIELD_TYPE::SIMPLE)
      {
         DecodeJsonField(field, clField, vIntermediateFormat_);
      }
      else if (field->type == FIELD_TYPE::ENUM)
      {
         vIntermediateFormat_.emplace_back(GetEnumValue(static_cast<EnumField*>(field)->enumDef, clField.get<std::string>()), field);
      }
      else if (field->type == FIELD_TYPE::STRING) // Handle string type directly
      {
         vIntermediateFormat_.emplace_back(clField.get<std::string>(), field);
      }
      else if (field->type == FIELD_TYPE::RESPONSE_ID)
      {
         std::string sResponse(clField.get<std::string>());
         if (sResponse == "OK")
         {
            vIntermediateFormat_.emplace_back(clField.get<std::string>(), field);
         }
         else
         {
            // Remove the "ERROR:" prefix from the response
            std::string sRespDesc = sResponse.substr(OEM4_ERROR_PREFIX_LENGTH, sResponse.length() - OEM4_ERROR_PREFIX_LENGTH);
            // Note: This won't match responses with format specifiers in them (%d, %s, etc), they will be given id=0
            vIntermediateFormat_.emplace_back( GetResponseId(vMyRespDefns, sRespDesc), field);
         }
      }
      else if (field->type == FIELD_TYPE::RESPONSE_STR)
      {
         vIntermediateFormat_.emplace_back(clField.get<std::string>(), field);
      }
      else if (field->type == FIELD_TYPE::FIXED_LENGTH_ARRAY || field->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY)
      {
         vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
         auto& pvFC = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);

         if (field->type == FIELD_TYPE::STRING
            || field->conversionStripped == CONVERSION_STRING::s
            || field->conversionStripped == CONVERSION_STRING::S)
         {
            pvFC.reserve(clField.get<std::string>().size());
            for(char& cValRead : clField.get<std::string>())
            {
               pvFC.emplace_back(static_cast<uint8_t>(cValRead), field);
            }
         }
         else
         {
            pvFC.reserve(clField.size());
            for (const auto& it : clField)
            {
               if (field->conversionStripped == CONVERSION_STRING::Z)
               {
                  pvFC.emplace_back(it.get<uint8_t>(), field);
               }
               else if (field->conversionStripped == CONVERSION_STRING::P)
               {
                  pvFC.emplace_back(it.get<int8_t>(), field);
               }
            }
         }
      }
      else if (field->type == FIELD_TYPE::FIELD_ARRAY)
      {
         auto uiArraySize = static_cast<uint32_t>(clField.size());

         auto* sub_field_defs = static_cast<FieldArrayField*>(field);

         vIntermediateFormat_.emplace_back(std::vector<FieldContainer>(), field);
         auto& pvFieldArrayContainer = std::get<std::vector<FieldContainer>>(vIntermediateFormat_.back().field_value);
         pvFieldArrayContainer.reserve(uiArraySize);

         for (uint32_t i = 0; i < uiArraySize; ++i)
         {
            pvFieldArrayContainer.emplace_back(std::vector<FieldContainer>(), field);
            auto& pvsubFC = std::get<std::vector<FieldContainer>>(pvFieldArrayContainer.back().field_value);
            pvsubFC.reserve((static_cast<const FieldArrayField*>(field))->fields.size());

            eStatus = DecodeJson(sub_field_defs->fields, clField[i], pvsubFC);
         }
      }
      else
      {
         std::string sError = "DecodeAscii(): Unknown field type";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }

   return eStatus;
}

// -------------------------------------------------------------------------------------------------------
void
MessageDecoder::DecodeJsonField(const BaseField* MessageDataType_, json clJsonField_, std::vector<FieldContainer>& vIntermediateFormat_)
{
   // %d
   if (MessageDataType_->conversionStripped == CONVERSION_STRING::d
    || MessageDataType_->conversionStripped == CONVERSION_STRING::ld
    || MessageDataType_->conversionStripped == CONVERSION_STRING::hd
    || MessageDataType_->conversionStripped == CONVERSION_STRING::lld)
   {
      // bool is output using %d
      if (MessageDataType_->dataType.name == DATA_TYPE_NAME::BOOL)
         vIntermediateFormat_.emplace_back(clJsonField_.get<bool>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(clJsonField_.get<int8_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 2)
         vIntermediateFormat_.emplace_back(clJsonField_.get<int16_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(clJsonField_.get<int32_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(clJsonField_.get<int64_t>(), MessageDataType_);
      else
      {
         std::string sError = "DecodeJsonField(): unknown %d conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %u
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::u
         || MessageDataType_->conversionStripped == CONVERSION_STRING::lu
         || MessageDataType_->conversionStripped == CONVERSION_STRING::hu
         || MessageDataType_->conversionStripped == CONVERSION_STRING::llu)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 2)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint16_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint32_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint64_t>(), MessageDataType_);
      else
      {
         std::string sError = "DecodeJsonField(): unknown %u conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %x
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::x
         || MessageDataType_->conversionStripped == CONVERSION_STRING::X
         || MessageDataType_->conversionStripped == CONVERSION_STRING::lx)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 2)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint16_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint32_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint64_t>(), MessageDataType_);
      else
      {
         std::string sError = "DecodeJsonField(): unknown %x conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %c
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::c)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(clJsonField_.get<int8_t>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 4 && MessageDataType_->dataType.name == DATA_TYPE_NAME::ULONG)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint32_t>(), MessageDataType_);
      else
      {
         std::string sError = "DecodeJsonField(): unknown %c conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %uc
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::uc)
   {
      if (MessageDataType_->dataType.length == 1)
         vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), MessageDataType_);
      else
      {
         std::string sError = "DecodeJsonField(): unknown %uc conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %f
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::f
         || MessageDataType_->conversionStripped == CONVERSION_STRING::lf)
   {
      if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(clJsonField_.get<float>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(clJsonField_.get<double>(), MessageDataType_);
      else
      {
         std::string sError = "DecodeJsonField(): unknown %f conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   // %k, %e, %g
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::k
         || MessageDataType_->conversionStripped == CONVERSION_STRING::lk
         || MessageDataType_->conversionStripped == CONVERSION_STRING::e
         || MessageDataType_->conversionStripped == CONVERSION_STRING::le
         || MessageDataType_->conversionStripped == CONVERSION_STRING::g)
   {
      if (MessageDataType_->dataType.length == 4)
         vIntermediateFormat_.emplace_back(clJsonField_.get<float>(), MessageDataType_);
      else if (MessageDataType_->dataType.length == 8)
         vIntermediateFormat_.emplace_back(clJsonField_.get<double>(), MessageDataType_);
      else
      {
         std::string sError = "DecodeJsonField(): unknown floating point conversion.";
         SPDLOG_LOGGER_CRITICAL(pclMyLogger, sError);
         throw std::runtime_error(sError);
      }
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::B)
   {
      vIntermediateFormat_.emplace_back(clJsonField_.get<int8_t>(), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::UB)
   {
      vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::XB)
   {
      vIntermediateFormat_.emplace_back(clJsonField_.get<uint8_t>(), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::id)
   {
      std::string sTemp(clJsonField_.get<std::string>());

      uint16_t usSlot = 0;
      int16_t sFreq = 0;

      size_t sDelimiter = sTemp.find_last_of('+');
      if (sDelimiter == std::string::npos)
         sDelimiter = sTemp.find_last_of('-');

      if (sDelimiter != std::string::npos)
      {
         usSlot = static_cast<uint16_t>(strtoul(sTemp.substr(0, sDelimiter).c_str(), nullptr, 10));
         sFreq = static_cast<int16_t>(strtol(sTemp.substr(sDelimiter, sTemp.length()).c_str(), nullptr, 10));
      }
      else
      {
         usSlot = static_cast<uint16_t>(strtoul(sTemp.c_str(), nullptr, 10));
         sFreq = 0;
      }

      uint32_t uiSatID = (usSlot | (sFreq << 16));
      vIntermediateFormat_.emplace_back(uiSatID, MessageDataType_);
   }
   // %R
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::R)
   {
      const MessageDefinition* pclMessageDef = pclMyMsgDb->GetMsgDef(clJsonField_.get<std::string>());
      vIntermediateFormat_.emplace_back(pclMessageDef ? CreateMsgID(pclMessageDef->logID, 0, 1, 0) : 0, MessageDataType_);
   }
   // %ucb
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::ucb)
   {
      // TODO: Validate this with a log conversion.
      vIntermediateFormat_.emplace_back(static_cast<uint32_t>(std::bitset<8>(clJsonField_.get<std::string>().c_str()).to_ulong()), MessageDataType_);
   }
   // %m
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::m) // MsgID
   {
      vIntermediateFormat_.emplace_back(MsgNameToMsgId(clJsonField_.get<std::string>()), MessageDataType_);
   }
   else if (MessageDataType_->conversionStripped == CONVERSION_STRING::T) // GPSTime msec field
   {
      vIntermediateFormat_.emplace_back(static_cast<uint32_t>(clJsonField_.get<double>() * SEC_TO_MSEC), MessageDataType_);
   }
   else
   {
      char errorBuff[1024];
      snprintf(errorBuff, sizeof(errorBuff), "DecodeJsonField() Message: %s, DataType: %d, Conversion: %s, Length %d",
         MessageDataType_->name.c_str(), static_cast<uint32_t>(MessageDataType_->dataType.name), MessageDataType_->conversion.c_str(), MessageDataType_->dataType.length);

      SPDLOG_LOGGER_CRITICAL(pclMyLogger, errorBuff);
      throw std::runtime_error(errorBuff);
   }
}

// -------------------------------------------------------------------------------------------------------
STATUS
MessageDecoder::Decode(unsigned char* pucInData_, IntermediateMessage& stIntermediateMessage_, MetaDataStruct& stMetaData_)
{
   if (pucInData_ == nullptr)
   {
      return STATUS::NULL_PROVIDED;
   }

   unsigned char* pucTempInData = pucInData_;

   const MessageDefinition* vMsgDef;
   MsgFieldsVector* pvCurrentMsgFields;

   if (stMetaData_.bResponse)
   {
      if (stMetaData_.eFormat != HEADERFORMAT::BINARY
       && stMetaData_.eFormat != HEADERFORMAT::SHORT_BINARY
       && stMetaData_.eFormat != HEADERFORMAT::ASCII
       && stMetaData_.eFormat != HEADERFORMAT::SHORT_ASCII
       && stMetaData_.eFormat != HEADERFORMAT::SHORT_ABB_ASCII)
      {
         return STATUS::NO_DEFINITION;
      }
      vMsgDef = &stMyRespDef;
      pvCurrentMsgFields = &vMsgDef->fields.at(0);
   }
   else
   {
      if (!pclMyMsgDb)
      {
         return STATUS::NO_DATABASE;
      }

      vMsgDef = pclMyMsgDb->GetMsgDef(stMetaData_.usMessageID);

      if (!vMsgDef)
      {
         pclMyLogger->warn("No log definition for ID {}", stMetaData_.usMessageID);
         return STATUS::NO_DEFINITION;
      }

      pvCurrentMsgFields = GetMsgDefFromCRC(vMsgDef, stMetaData_.uiMessageCRC);
   }

   // Get the current msg fields and expand the intermediate format vector to prevent the copy constructor from being called
   // when the vector grows in size.
   stIntermediateMessage_.clear();
   stIntermediateMessage_.reserve(pvCurrentMsgFields->size());

   // Decode the detected format.
  return stMetaData_.eFormat == HEADERFORMAT::ASCII || stMetaData_.eFormat == HEADERFORMAT::SHORT_ASCII
       ? DecodeAscii(*pvCurrentMsgFields, reinterpret_cast<char**>(&pucTempInData), stIntermediateMessage_)
       : stMetaData_.eFormat == HEADERFORMAT::ABB_ASCII || stMetaData_.eFormat == HEADERFORMAT::SHORT_ABB_ASCII
       ? DecodeAbbrevAscii(*pvCurrentMsgFields, reinterpret_cast<char**>(&pucTempInData), stIntermediateMessage_)
       : stMetaData_.eFormat == HEADERFORMAT::BINARY || stMetaData_.eFormat == HEADERFORMAT::SHORT_BINARY
       ? DecodeBinary(*pvCurrentMsgFields, &pucTempInData, stIntermediateMessage_, stMetaData_.uiBinaryMsgLength)
       : stMetaData_.eFormat == HEADERFORMAT::JSON
       ? DecodeJson(*pvCurrentMsgFields, json::parse(pucTempInData)["body"], stIntermediateMessage_)
       : STATUS::UNKNOWN;
}
