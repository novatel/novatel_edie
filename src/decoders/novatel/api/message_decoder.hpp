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
//! \file message_decoder.hpp
//! \brief Decode OEM message bodies.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef NOVATEL_MESSAGE_DECODER_HPP
#define NOVATEL_MESSAGE_DECODER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"
#include "decoders/common/api/crc32.hpp"
#include "decoders/common/api/jsonreader.hpp"
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/header_decoder.hpp"

#include <nlohmann/json.hpp>
#include <logger/logger.hpp>
#include <variant>
#include <string>
#include <sstream>
#include <cassert>
#include <iostream>
#include <fstream>
#include <stdarg.h>

namespace novatel::edie::oem {

using nlohmann::json;

struct FieldContainer;

#define novatel_types bool, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double, std::string
#define container_types novatel_types, std::vector<std::variant<novatel_types> >, std::vector<FieldContainer>, novatel::edie::oem::IntermediateHeader

//-----------------------------------------------------------------------
//! \struct FieldContainer
//! \brief A struct to contain different fields from OEM messages.
//-----------------------------------------------------------------------
struct FieldContainer
{
   std::variant<container_types> field_value;
   const novatel::edie::BaseField* field_def;

   template <class T>
   FieldContainer(T field_value_, const novatel::edie::BaseField* field_def_)
      : field_value(field_value_), field_def(field_def_)
   {}

   FieldContainer([[maybe_unused]] const FieldContainer& obj)
   {
      throw std::runtime_error("FieldContainer: I'm being copied. Implement a proper copy constructor.");
   }
};

typedef std::vector<FieldContainer> IntermediateMessage;
typedef const std::vector<novatel::edie::BaseField*> MsgFieldsVector;

//============================================================================
//! \class MessageDecoder
//! \brief Decode OEM message bodies.
//============================================================================
class MessageDecoder
{
private:
   std::shared_ptr<spdlog::logger> pclMyLogger;
   JsonReader* pclMyMsgDb{ nullptr };
   EnumDefinition* vMyRespDefns{ nullptr };
   EnumDefinition* vMyCommandDefns{ nullptr };
   EnumDefinition* vMyPortAddrDefns{ nullptr };
   EnumDefinition* vMyGPSTimeStatusDefns{ nullptr };

   unsigned char* pucTempBufConvert{ nullptr };
   unsigned char* pucBeginningConvert{ nullptr };
   uint32_t uiMyBufferBytesRemaining;
   uint32_t uiMyAbbrevAsciiIndentationLevel;
   MessageDefinition stMyRespDef;

   // Inline buffer functions
   [[nodiscard]] bool PrintToBuffer(char** ppcBuffer_, char* szFormat_, ...)
   {
      va_list args;
      va_start(args, szFormat_);
      uint32_t uiBytesBuffered = vsnprintf(*ppcBuffer_, uiMyBufferBytesRemaining, szFormat_, args);
      va_end(args);
      if (uiMyBufferBytesRemaining < uiBytesBuffered)
      {
         return false;
      }
      *ppcBuffer_ += uiBytesBuffered;
      uiMyBufferBytesRemaining -= uiBytesBuffered;
      return true;
   }

   template <typename T>
   [[nodiscard]] inline bool CopyToBuffer(unsigned char** ppcBuffer_, T* ptItem_, uint32_t uiItemSize_)
   {
      if (uiMyBufferBytesRemaining < uiItemSize_)
      {
         return false;
      }
      memcpy(*ppcBuffer_, ptItem_, uiItemSize_);
      *ppcBuffer_ += uiItemSize_;
      uiMyBufferBytesRemaining -= uiItemSize_;
      return true;
   }

   [[nodiscard]] inline bool SetInBuffer(unsigned char** ppcBuffer_, int32_t iItem_, uint32_t uiItemSize_)
   {
      if (uiMyBufferBytesRemaining < uiItemSize_)
      {
         return false;
      }
      memset(*ppcBuffer_, iItem_, uiItemSize_);
      *ppcBuffer_ += uiItemSize_;
      uiMyBufferBytesRemaining -= uiItemSize_;
      return true;
   }

   // Enum util functions
   void InitEnumDefns();
   void CreateResponseMsgDefns();
   uint32_t MsgNameToMsgId(std::string sMsgName_);
   std::string MsgIdToMsgName(const uint32_t uiMessageID_);

   MsgFieldsVector* GetMsgDefFromCRC(const MessageDefinition* pclMessageDef_, uint32_t& uiMsgDefCRC_);

   // Decode binary body
   void DecodeBinaryField(const BaseField* MessageDataType_, unsigned char** ppcLogBuf_, std::vector<FieldContainer>& vIntermediateFormat);

   // Decode ascii body
   [[nodiscard]] STATUS DecodeAbbrevAscii(const std::vector<BaseField*> MsgDefFields_, char** ppcLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_);
   void DecodeAsciiField(const BaseField* MessageDataType_, char** ppcToken_, const size_t tokenLength_, std::vector<FieldContainer>& vIntermediateFormat);

   // Decode json body
   [[nodiscard]] STATUS DecodeJson(const std::vector<BaseField*> MsgDefFields_, json clJsonFields_, std::vector<FieldContainer>& vIntermediateFormat_);
   void DecodeJsonField(const BaseField* MessageDataType_, json clJsonField_, std::vector<FieldContainer>& vIntermediateFormat);

protected:
   [[nodiscard]] STATUS DecodeBinary(const std::vector<BaseField*> MsgDefFields_, unsigned char** ppucLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_, uint32_t uiMessageLength_);
   [[nodiscard]] STATUS DecodeAscii(const std::vector<BaseField*> MsgDefFields_, char** ppcLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_);

public:
   //----------------------------------------------------------------------------
   //! \brief A constructor for the MessageDecoder class.
   //
   //! \param[in] pclJsonDb_ A pointer to a JsonReader object.  Defaults to nullptr.
   //----------------------------------------------------------------------------
   MessageDecoder(JsonReader* pclJsonDb_ = nullptr);

   //----------------------------------------------------------------------------
   //! \brief Load a JsonReader object.
   //
   //! \param[in] pclJsonDb_ A pointer to a JsonReader object.
   //----------------------------------------------------------------------------
   void
   LoadJsonDb(JsonReader* pclJsonDb_);

   //----------------------------------------------------------------------------
   //! \brief Get the internal logger.
   //
   //! \return A shared_ptr to the spdlog::logger.
   //----------------------------------------------------------------------------
   std::shared_ptr<spdlog::logger>
   GetLogger();

   //----------------------------------------------------------------------------
   //! \brief Set the level of detail produced by the internal logger.
   //
   //! \param[in] eLevel_ The logging level to enable.
   //----------------------------------------------------------------------------
   void
   SetLoggerLevel(spdlog::level::level_enum eLevel_);

   //----------------------------------------------------------------------------
   //! \brief Shutdown the internal logger.
   //----------------------------------------------------------------------------
   void
   ShutdownLogger();

   //----------------------------------------------------------------------------
   //! \brief Decode an OEM message body from the provided frame.
   //
   //! \param[in] pucMessage_ A pointer to an OEM message body.
   //! \param[out] stIntermediateLog_ The IntermediateLog to be populated.
   //! \param[in, out] stMetaData_ MetaDataStruct to provide information about
   //! the frame and be fully populated to help describe the decoded log.
   //
   //! \remark Note, that pucMessage_ must not point to the OEM message header,
   //! rather the OEM message body.  This can be done by advancing the pointer
   //! of an OEM message frame by stMetaData.uiHeaderLength.
   //! \return An error code describing the result of decoding.
   //!   SUCCESS: The operation was successful.
   //!   NULL_PROVIDED: pucMessage_ is a null pointer.
   //!   NO_DATABASE: No database was ever loaded into this component.
   //!   NO_DEFINITION: No definition for the current log was found in the
   //! provided database.
   //!   FAILURE: Failed to decode a header field.
   //!   UNSUPPORTED: Attempted to decode an unsupported format.
   //!   UNKNOWN: The header format provided is not known.
   //----------------------------------------------------------------------------
   [[nodiscard]] STATUS
   Decode(unsigned char* pucMessage_, IntermediateMessage& stIntermediateMessage_, MetaDataStruct& stMetaData_);
};
}

#endif // NOVATEL_MESSAGE_DECODER_HPP
