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
//! \file rxconfig_handler.cpp
//! \brief Class implementation for the RxConfigHandler.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/novatel/api/rxconfig/rxconfig_handler.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

const char* RxConfigHandler::szAbbrevASCIIEmbeddedHeaderPrefix = "<     ";

// -------------------------------------------------------------------------------------------------------
RxConfigHandler::RxConfigHandler(JsonReader* pclJsonDB_) :
   clMyHeaderDecoder(pclJsonDB_),
   clMyMessageDecoder(pclJsonDB_),
   clMyEncoder(pclJsonDB_),
   pcMyFrameBuffer(new unsigned char[uiINTERNAL_BUFFER_SIZE]),
   pcMyEncodeBuffer(new unsigned char[uiINTERNAL_BUFFER_SIZE])
{
   pclMyLogger = Logger().RegisterLogger("rxconfig_handler");

   pclMyLogger->debug("RxConfigHandler initializing...");

   if (pclJsonDB_ != NULL)
   {
      LoadJsonDb(pclJsonDB_);
   }

   pclMyLogger->debug("RxConfigHandler initialized");
}

// -------------------------------------------------------------------------------------------------------
RxConfigHandler::~RxConfigHandler()
{
   if(pcMyFrameBuffer)
   {
      delete[] pcMyFrameBuffer;
   }

   if(pcMyEncodeBuffer)
   {
      delete[] pcMyEncodeBuffer;
   }
}

// -------------------------------------------------------------------------------------------------------
void
RxConfigHandler::LoadJsonDb(JsonReader* pclJsonDB_)
{
   pclMyMsgDB = pclJsonDB_;
   clMyHeaderDecoder.LoadJsonDb(pclJsonDB_);
   clMyMessageDecoder.LoadJsonDb(pclJsonDB_);
   clMyEncoder.LoadJsonDb(pclJsonDB_);

   vMyCommandDefns = pclMyMsgDB->GetEnumDef("Commands");
   vMyPortAddrDefns = pclMyMsgDB->GetEnumDef("PortAddress");
   vMyGPSTimeStatusDefns = pclMyMsgDB->GetEnumDef("GPSTimeStatus");
   CreateRXConfigMsgDefn();
}

// -------------------------------------------------------------------------------------------------------
void
RxConfigHandler::SetLoggerLevel(spdlog::level::level_enum eLevel_)
{
   pclMyLogger->set_level(eLevel_);
}

// -------------------------------------------------------------------------------------------------------
void
RxConfigHandler::ShutdownLogger()
{
   Logger::Shutdown();
}

// -------------------------------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger>
RxConfigHandler::GetLogger()
{
   return pclMyLogger;
}

//-----------------------------------------------------------------------
bool
RxConfigHandler::IsRXConfigTypeMsg(uint16_t usMessageID_)
{
   return (usMessageID_ == usRXConfigMsgID || usMessageID_ == usRXConfigUserMsgID);
}

// -------------------------------------------------------------------------------------------------------
uint32_t
RxConfigHandler::MsgNameToMsgId(std::string sMsgName_)
{
      uint32_t uiSiblingID = 0;
      uint32_t uiMsgFormat = 0;
      uint32_t uiResponse  = 0; // 0 = original, 1 = response

      // Injest the sibling information, i.e. the _1 from LOGNAMEA_1
      if ((sMsgName_.find_last_of('_') != std::string::npos) &&
         (sMsgName_.find_last_of('_') == sMsgName_.size() - 2))
      {
         uiSiblingID = static_cast<uint32_t>(ToDigit(sMsgName_.back()));
         sMsgName_.resize(sMsgName_.size()-2);
      }

      // If this is an abbrev msg (no format information), we will be able to find the MsgDef
      const MessageDefinition* pclMessageDef = pclMyMsgDB->GetMsgDef(sMsgName_);
      if(pclMessageDef)
      {
         uiResponse = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ABBREV);

         uint32_t uiMessageID = CreateMsgID(pclMessageDef->logID, uiSiblingID, uiMsgFormat, uiResponse);
         return uiMessageID;
      }

      std::string sTemp(sMsgName_);
      if(sTemp.back() == 'R') // Ascii Response
      {
         uiResponse = static_cast<uint32_t>(true);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
         sTemp.pop_back();
      }
      else if(sTemp.back() == 'A') // Ascii
      {
         uiResponse = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::ASCII);
         sTemp.pop_back();
      }
      else if(sTemp.back() == 'B') // Binary
      {
         uiResponse = static_cast<uint32_t>(false);
         uiMsgFormat = static_cast<uint32_t>(MESSAGEFORMAT::BINARY);
         sTemp.pop_back();
      }

      pclMessageDef = pclMyMsgDB->GetMsgDef(sTemp);
      if(pclMessageDef)
      {
         uint32_t uiMessageID = CreateMsgID(pclMessageDef ? pclMessageDef->logID : GetEnumValue(vMyCommandDefns, sTemp), uiSiblingID, uiMsgFormat, uiResponse);
         return uiMessageID;
      }

      return 0;
}

// -------------------------------------------------------------------------------------------------------
std::string
RxConfigHandler::MsgIdToMsgName(const uint32_t uiMessageID_)
{
   uint16_t usLogID = 0;
   uint32_t uiSiblingID = 0;
   uint32_t uiMessageFormat = 0;
   uint32_t uiResponse = 0;

   UnpackMsgID(uiMessageID_, usLogID, uiSiblingID, uiMessageFormat, uiResponse);

   std::string strMessageName;
   const MessageDefinition* pstMessageDefinition = pclMyMsgDB->GetMsgDef(usLogID);

   if(pstMessageDefinition)
   {
      strMessageName = pstMessageDefinition->name;
   }
   else
   {
      strMessageName = GetEnumString(vMyCommandDefns, usLogID);
   }

   std::string strMessageFormatSuffix = uiResponse ? "R"
      : uiMessageFormat == static_cast<uint32_t>(MESSAGEFORMAT::BINARY) ? "B"
      : uiMessageFormat == static_cast<uint32_t>(MESSAGEFORMAT::ASCII)  ? "A"
      : ""; // default to abbreviated ascii format

   if(uiSiblingID)
      strMessageFormatSuffix.append("_").append(std::to_string(uiSiblingID));
   return strMessageName.append(strMessageFormatSuffix);
}

// -------------------------------------------------------------------------------------------------------
void
RxConfigHandler::CreateRXConfigMsgDefn()
{
   // Embedded Header
   SimpleDataType stEmbeddedHeaderDataType = SimpleDataType();
   stEmbeddedHeaderDataType.description = "Embedded Header";
   stEmbeddedHeaderDataType.length = sizeof(IntermediateHeader);
   stEmbeddedHeaderDataType.name = DATA_TYPE_NAME::EMBEDDED_HEADER;

   BaseField stEmbeddedHeader = BaseField();
   stEmbeddedHeader.name = "embedded_header";
   stEmbeddedHeader.type = FIELD_TYPE::RXCONFIG_HEADER;
   stEmbeddedHeader.dataType = stEmbeddedHeaderDataType;

   // Embedded body
   SimpleDataType stEmbeddedBodyDataType = SimpleDataType();
   stEmbeddedBodyDataType.description = "Embedded Body";
   stEmbeddedBodyDataType.name = DATA_TYPE_NAME::EMBEDDED_BODY;
   stEmbeddedBodyDataType.length = 1;

   BaseField stEmbeddedBody = BaseField();
   stEmbeddedBody.name = "embedded_body";
   stEmbeddedBody.type = FIELD_TYPE::RXCONFIG_BODY;
   stEmbeddedBody.dataType = stEmbeddedBodyDataType;

   // Message Definition
   stMyRXConfigMsgDef = MessageDefinition();
   stMyRXConfigMsgDef.name = std::string("rxconfig");
   stMyRXConfigMsgDef.fields[0];
   stMyRXConfigMsgDef.fields[0].push_back(stEmbeddedHeader.clone());
   stMyRXConfigMsgDef.fields[0].push_back(stEmbeddedBody.clone());
}

// -------------------------------------------------------------------------------------------------------
uint32_t
RxConfigHandler::Write(unsigned char* pucData_, uint32_t uiDataSize_)
{
   return clMyFramer.Write(pucData_, uiDataSize_);
}

// -------------------------------------------------------------------------------------------------------
STATUS
RxConfigHandler::Convert(
   MessageDataStruct& stRxConfigMessageData_, MetaDataStruct& stRxConfigMetaData_,
   MessageDataStruct& stEmbeddedMessageData_, MetaDataStruct& stEmbeddedMetaData_,
   ENCODEFORMAT eEncodeFormat_)
{
   STATUS eStatus = STATUS::SUCCESS;
   IntermediateHeader stRxConfigHeader;
   IntermediateHeader stEmbeddedHeader;
   IntermediateMessage stRxConfigMessage;
   IntermediateMessage stEmbeddedMessage;
   pucMyFrameBufferPointer = pcMyFrameBuffer;
   unsigned char* pucTempMessagePointer = pucMyFrameBufferPointer;

   // Get an RXCONFIG log.
   eStatus = clMyFramer.GetFrame(pucMyFrameBufferPointer, uiINTERNAL_BUFFER_SIZE, stRxConfigMetaData_);
   if (eStatus == STATUS::BUFFER_EMPTY || eStatus == STATUS::INCOMPLETE)
   {
      return STATUS::BUFFER_EMPTY;
   }
   if(eStatus != STATUS::SUCCESS)
   {
      return eStatus;
   }

   // Decode the RXCONFIG log.
   eStatus = clMyHeaderDecoder.Decode(pucMyFrameBufferPointer, stRxConfigHeader, stRxConfigMetaData_);
   if(eStatus != STATUS::SUCCESS)
   {
      return eStatus;
   }
   pucTempMessagePointer += stRxConfigMetaData_.uiHeaderLength;

   // If we have something that isn't RXCONFIG, get rid of it.
   if(!IsRXConfigTypeMsg(stRxConfigMetaData_.usMessageID))
   {
      return STATUS::UNKNOWN;
   }

   // Decode the RXCONFIG message body, which is a regular OEM header and body.
   novatel::edie::oem::MsgFieldsVector vRxConfigMessageFields = stMyRXConfigMsgDef.fields.at(0);
   stRxConfigMessage.reserve(vRxConfigMessageFields.size());
   for(auto& field : vRxConfigMessageFields)
   {
      if(field->type == FIELD_TYPE::RXCONFIG_HEADER)
      {
         if(stRxConfigMetaData_.eFormat == HEADERFORMAT::ABB_ASCII)
         {
            // Abbreviated ASCII RXCONFIG logs have indentations on the embedded header.  The HeaderDecoder
            // does not expect this and the spaces must be removed.  Remove "<     ", then put '<' back at
            // the beginning so the header is treated correctly.
            ConsumeAbbrevFormatting(0, reinterpret_cast<char**>(&pucTempMessagePointer));
            pucTempMessagePointer -= OEM4_ASCII_SYNC_LENGTH;
            *pucTempMessagePointer = OEM4_ABBREV_ASCII_SYNC;
         }

         eStatus = clMyHeaderDecoder.Decode(pucTempMessagePointer, stEmbeddedHeader, stEmbeddedMetaData_);
         if(eStatus == STATUS::NO_DEFINITION)
         {
            return STATUS::NO_DEFINITION_EMBEDDED;
         }
         if(eStatus != STATUS::SUCCESS)
         {
            return eStatus;
         }

         stRxConfigMessage.emplace_back(stEmbeddedHeader, field);
      }
      else if(field->type == FIELD_TYPE::RXCONFIG_BODY)
      {
         // Put an IntermediateMessage struct into the RXCONFIG IntermediateMessage, then
         // pass it to the message decoder as a destination for the embedded message.
         stRxConfigMessage.emplace_back(IntermediateMessage(), field);
         eStatus = clMyMessageDecoder.Decode((pucTempMessagePointer + stEmbeddedMetaData_.uiHeaderLength), std::get<IntermediateMessage>(stRxConfigMessage.back().field_value), stEmbeddedMetaData_);
         if(eStatus == STATUS::NO_DEFINITION)
         {
            return STATUS::NO_DEFINITION_EMBEDDED;
         }
         if(eStatus != STATUS::SUCCESS)
         {
            return eStatus;
         }
      }
      else
      {
         return STATUS::FAILURE;
      }
   }

   // Encode the RXCONFIG log.
   pucMyEncodeBufferPointer = pcMyEncodeBuffer;
   unsigned char* pucTempEncodeBuffer = pucMyEncodeBufferPointer;
   uiMyBufferBytesRemaining = uiINTERNAL_BUFFER_SIZE;

   if(eEncodeFormat_ == ENCODEFORMAT::JSON)
   {
      if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), R"({"header":)"))
      {
         return STATUS::BUFFER_FULL;
      }
   }

   eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, stRxConfigHeader, stRxConfigMessageData_, stRxConfigMetaData_, eEncodeFormat_);
   if(eStatus == STATUS::NO_DEFINITION)
   {
      return STATUS::NO_DEFINITION_EMBEDDED;
   }
   if(eStatus != STATUS::SUCCESS)
   {
      return eStatus;
   }

   pucTempEncodeBuffer += stRxConfigMessageData_.uiMessageHeaderLength;

   if(eEncodeFormat_ == ENCODEFORMAT::JSON)
   {
      if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), R"(,"body":)"))
      {
         return STATUS::BUFFER_FULL;
      }
   }

   stEmbeddedMessageData_.pucMessage = pucTempEncodeBuffer;
   stRxConfigMessageData_.pucMessageBody = pucTempEncodeBuffer;

   // This is just dummy args that we must pass to the encoder.  They will not be used.
   uint32_t uiCRC = 0;
   for (FieldContainer& field : stRxConfigMessage)
   {
      if (field.field_def->type == FIELD_TYPE::RXCONFIG_HEADER)
      {
         switch(eEncodeFormat_)
         {
            case ENCODEFORMAT::JSON:
               if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), R"({"embedded_header":)"))
               {
                  return STATUS::BUFFER_FULL;
               }
               break;

            case ENCODEFORMAT::ABBREV_ASCII:
               if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), szAbbrevASCIIEmbeddedHeaderPrefix))
               {
                  return STATUS::BUFFER_FULL;
               }
               break;

            default:
               break;
         }

         eStatus = clMyEncoder.EncodeHeader(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, std::get<IntermediateHeader>(field.field_value), stEmbeddedMessageData_, stEmbeddedMetaData_, eEncodeFormat_, true);
         if(eStatus != STATUS::SUCCESS)
         {
            return eStatus;
         }

         pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageHeaderLength;
         uiMyBufferBytesRemaining -= stEmbeddedMessageData_.uiMessageHeaderLength;

         switch(eEncodeFormat_)
         {
            case ENCODEFORMAT::JSON:
               if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), R"(,"embedded_body":)"))
               {
                  return STATUS::BUFFER_FULL;
               }
               break;

            case ENCODEFORMAT::ABBREV_ASCII:
               // The header is going to be pointing to the wrong location, so reverse it back to before the "<     " characters.
               stEmbeddedMessageData_.pucMessageHeader -= strlen(szAbbrevASCIIEmbeddedHeaderPrefix);
               stEmbeddedMessageData_.uiMessageHeaderLength += static_cast<uint32_t>(strlen(szAbbrevASCIIEmbeddedHeaderPrefix));
               // A normal abbreviated ASCII log would remove the final ' ' delimiter, however since this is part of a message
               // body, we should encode it to follow the standard of trailing spaces in the message body.  EncodeHeader()
               // would have removed this, so add it back in the message MessageHeaderLength count.
               stEmbeddedMessageData_.uiMessageHeaderLength++;

               if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), "\r\n"))
               {
                  return STATUS::BUFFER_FULL;
               }

               stEmbeddedMessageData_.uiMessageHeaderLength++;
               break;

            default:
               break;
         }
      }
      else if (field.field_def->type == FIELD_TYPE::RXCONFIG_BODY)
      {
         eStatus = clMyEncoder.EncodeBody(&pucTempEncodeBuffer, uiMyBufferBytesRemaining, std::get<IntermediateMessage>(field.field_value), stEmbeddedMessageData_, stEmbeddedMetaData_, eEncodeFormat_);
         if(eStatus != STATUS::SUCCESS)
         {
            return eStatus;
         }

         uiMyBufferBytesRemaining -= stEmbeddedMessageData_.uiMessageBodyLength;
         pucTempEncodeBuffer += stEmbeddedMessageData_.uiMessageBodyLength;

         // The last CRC would have been written correctly.  Pull it out, flip it, put it back in.
         // This will be done differently depending on how we encoded the message.
         switch(eEncodeFormat_)
         {
            case ENCODEFORMAT::ASCII:
               // Move back over CRLF.
               pucTempEncodeBuffer -= 2;
               uiMyBufferBytesRemaining += 2;
               stEmbeddedMessageData_.uiMessageBodyLength -= 2;
               // Move back over the CRC.
               pucTempEncodeBuffer -= OEM4_ASCII_CRC_LENGTH;
               uiMyBufferBytesRemaining += OEM4_ASCII_CRC_LENGTH;
               // Grab the CRC from the encode buffer and invert it.
               uiCRC = strtoul(reinterpret_cast<char*>(pucTempEncodeBuffer), NULL, 16);
               uiCRC = uiCRC ^ 0xFFFFFFFF;
               if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), "%08x", uiCRC))
               {
                  return STATUS::BUFFER_FULL;
               }
               break;

            case ENCODEFORMAT::BINARY:
               // Move back over the CRC.
               pucTempEncodeBuffer -= OEM4_BINARY_CRC_LENGTH;
               uiMyBufferBytesRemaining += OEM4_BINARY_CRC_LENGTH;
               // Grab the CRC from the encode buffer and invert it.
               uiCRC = *(reinterpret_cast<uint32_t*>(pucTempEncodeBuffer));
               uiCRC = uiCRC ^ 0xFFFFFFFF;
               if(!CopyToBuffer(&pucTempEncodeBuffer, &uiCRC, sizeof(uint32_t)))
               {
                  return STATUS::BUFFER_FULL;
               }
               break;

            case ENCODEFORMAT::JSON:
               if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), R"(})"))
               {
                  return STATUS::BUFFER_FULL;
               }
               break;

            default:
               break;
         }
      }
      else
      {
         return STATUS::FAILURE;
      }
   }

   stEmbeddedMessageData_.uiMessageLength = static_cast<uint32_t>(pucTempEncodeBuffer - stRxConfigMessageData_.pucMessageBody);

   // Put the final CRC at the end.
   switch(eEncodeFormat_)
   {
      case ENCODEFORMAT::ASCII:
         uiCRC = CalculateBlockCRC32(static_cast<uint32_t>(pucTempEncodeBuffer - (pucMyEncodeBufferPointer+1)), 0, pucMyEncodeBufferPointer+1);
         if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), "*%08x\r\n", uiCRC))
         {
            return STATUS::BUFFER_FULL;
         }
         break;

      case ENCODEFORMAT::BINARY:
         uiCRC = CalculateBlockCRC32(static_cast<uint32_t>(pucTempEncodeBuffer - pucMyEncodeBufferPointer), 0, pucMyEncodeBufferPointer);
         if(!CopyToBuffer(&pucTempEncodeBuffer, &uiCRC, sizeof(uint32_t)))
         {
            return STATUS::BUFFER_FULL;
         }
         break;

      default:
         break;
   }

   stRxConfigMessageData_.uiMessageBodyLength = static_cast<uint32_t>(pucTempEncodeBuffer - stRxConfigMessageData_.pucMessageBody);

   // Add the closing '}' character, but don't count it as part of the messasge body length.
   if(eEncodeFormat_ == ENCODEFORMAT::JSON)
   {
      if(!PrintToBuffer(reinterpret_cast<char**>(&pucTempEncodeBuffer), R"(})"))
      {
         return STATUS::BUFFER_FULL;
      }
   }

   stRxConfigMessageData_.pucMessage = pucMyEncodeBufferPointer;
   stRxConfigMessageData_.uiMessageLength = static_cast<uint32_t>(pucTempEncodeBuffer - pucMyEncodeBufferPointer);

   return STATUS::SUCCESS;
}

// -------------------------------------------------------------------------------------------------------
uint32_t
RxConfigHandler::Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
{
   return clMyFramer.Flush(pucBuffer_, uiBufferSize_);
}

