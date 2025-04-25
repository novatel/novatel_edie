// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file rxconfig_handler.hpp
// ===============================================================================

#ifndef RXCONFIG_HANDLER_HPP
#define RXCONFIG_HANDLER_HPP

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/oem/common.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/framer.hpp"
#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace novatel::edie::oem {

constexpr uint16_t US_RX_CONFIG_MSG_ID = 128;
constexpr uint16_t US_RX_CONFIG_USER_MSG_ID = 2474;

//============================================================================
//! \class RxConfigHandler
//! \brief Class to handle RXCONFIGA and RXCONFIGB messages, which are
//! outliers with regard to OEM message protocol. This class should have
//! bytes containing RXCONFIG messages written to it with calls to Write()
//! and the converted data should be retrieved with calls to Convert().
//============================================================================
class RxConfigHandler
{
  private:
    static constexpr std::string_view szAbbrevAsciiEmbeddedHeaderPrefix = "<     ";
    static constexpr uint32_t uiInternalBufferSize = MESSAGE_SIZE_MAX;

    static std::unique_ptr<Framer> pclMyFramer;
    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    std::shared_ptr<spdlog::logger> pclMyLogger{nullptr};
    MessageDatabase::Ptr pclMyMsgDb{nullptr};
    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};

    std::unique_ptr<unsigned char[]> pcMyFrameBuffer{nullptr};
    std::unique_ptr<unsigned char[]> pcMyEncodeBuffer{nullptr};

    BaseField::ConstPtr pclRxConfigFieldDef{nullptr};
    BaseField::ConstPtr pclRxConfigUserFieldDef{nullptr};

    //----------------------------------------------------------------------------
    //! \brief Validate that a message definition contains valid fields to be decoded as RXConfig
    //!
    //! \param pclMsgDef_ The message definition to validate.
    //!
    //! \throws std::invalid_argument if the message definition is invalid.
    //------------------------------------------------------------------------------
    static void ValidateMsgDef(const MessageDefinition::ConstPtr& pclMsgDef_);

    //----------------------------------------------------------------------------
    //! \brief Retrieve the only field definition from a valid RXConfig message definition.
    //!
    //! \param pclMsgDef_ The message definition retrieve the field definition from.
    //!
    //! \returns A shared pointer to the field definition.
    //------------------------------------------------------------------------------
    static BaseField::ConstPtr GetFieldDefFromMsgDef(const MessageDefinition::ConstPtr& pclMsgDef_);

    //----------------------------------------------------------------------------
    //! \brief Encode an RXConfig message from the provided intermediate structures.
    //
    // This function has additional output parameters than the public version and
    // exists primarily for compatiblity with the original conversion based class interface.
    //
    //! \param[out] ppucBuffer_ A pointer to the buffer to return the encoded
    //! message to.
    //! \param[in] uiBufferSize_ The length of ppcBuffer_.
    //! \param[in] stHeader_ A reference to the decoded header intermediate.
    //! This must be populated by the HeaderDecoder.
    //! \param[in] stMessage_ A reference to the decoded message intermediate.
    //! This must be populated by the MessageDecoder.
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated with the message data of the RXCONFIG message.
    //! \param[out] stEmbeddedMessageData_ A reference to a MessageDataStruct to be
    //! populated with the message data of the embedded message.
    //! \parm[out ] stEmbeddedMetaData_ A reference to a MetaDataStruct to be
    //! populated with the MetaData of the embedded message.
    //! \param[in] eFormat_ The format to encode the message to.
    //
    //! \return An error code describing the result of encoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: ppucBuffer_ either points to a null pointer or is
    //! a null pointer itself.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //!   FAILURE: stMessageData_.pucMessageHeader was not correctly set inside
    //! this function. This should not happen.
    //!   UNSUPPORTED: eFormat_ contains a format that is not supported for
    //! encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                const std::vector<FieldContainer>& stMessage_, MessageDataStruct& stMessageData_,
                                MessageDataStruct& stEmbeddedMessageData_, MetaDataStruct& stEmbeddedMetaData_, ENCODE_FORMAT eFormat_) const;

    //----------------------------------------------------------------------------
    //! \brief Encodes an RXCONFIG message into Binary format.
    //!
    //! \param[out] ppucBuffer_ A pointer to the buffer where the encoded JSON message will be written.
    //! \param[in] uiBufferSize_ The size of the buffer provided.
    //! \param[in] stHeader_ A reference to the intermediate header structure containing header information.
    //! \param[in] stMessageData_ A reference to the structure containing the main message data.
    //! \param[in] stEmbeddedMessageData_ A reference to the structure containing the embedded message data.
    //! \param[in] stEmbeddedMetaData_ A reference to the metadata structure for the embedded message.
    //! \param[in] stEmbeddedHeader_ A reference to the intermediate header for the embedded message.
    //! \param[in] stEmbeddedMessage_ A vector of field containers representing the embedded message fields.
    //!
    //! \return A STATUS code indicating the result of the encoding operation.
    //! Possible values include:
    //! - SUCCESS: The operation was successful.
    //! - BUFFER_FULL: The provided buffer was not large enough to hold the encoded message.
    //! - FAILURE: An error occurred during encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS EncodeBinary(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                      MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_,
                                      MetaDataStruct& stEmbeddedMetaData_, IntermediateHeader& stEmbeddedHeader_,
                                      std::vector<FieldContainer>& stEmbeddedMessage_) const;

    //----------------------------------------------------------------------------
    //! \brief Encodes an RXCONFIG message into ASCII format.
    //!
    //! \param[out] ppucBuffer_ A pointer to the buffer where the encoded JSON message will be written.
    //! \param[in] uiBufferSize_ The size of the buffer provided.
    //! \param[in] stHeader_ A reference to the intermediate header structure containing header information.
    //! \param[in] stMessageData_ A reference to the structure containing the main message data.
    //! \param[in] stEmbeddedMessageData_ A reference to the structure containing the embedded message data.
    //! \param[in] stEmbeddedMetaData_ A reference to the metadata structure for the embedded message.
    //! \param[in] stEmbeddedHeader_ A reference to the intermediate header for the embedded message.
    //! \param[in] stEmbeddedMessage_ A vector of field containers representing the embedded message fields.
    //!
    //! \return A STATUS code indicating the result of the encoding operation.
    //! Possible values include:
    //! - SUCCESS: The operation was successful.
    //! - BUFFER_FULL: The provided buffer was not large enough to hold the encoded message.
    //! - FAILURE: An error occurred during encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS EncodeAscii(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                     MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_,
                                     MetaDataStruct& stEmbeddedMetaData_, IntermediateHeader& stEmbeddedHeader_,
                                     std::vector<FieldContainer>& stEmbeddedMessage_) const;

    //----------------------------------------------------------------------------
    //! \brief Encodes an RXCONFIG message into Abbreviated Ascii format.
    //!
    //! \param[out] ppucBuffer_ A pointer to the buffer where the encoded JSON message will be written.
    //! \param[in] uiBufferSize_ The size of the buffer provided.
    //! \param[in] stHeader_ A reference to the intermediate header structure containing header information.
    //! \param[in] stMessageData_ A reference to the structure containing the main message data.
    //! \param[in] stEmbeddedMessageData_ A reference to the structure containing the embedded message data.
    //! \param[in] stEmbeddedMetaData_ A reference to the metadata structure for the embedded message.
    //! \param[in] stEmbeddedHeader_ A reference to the intermediate header for the embedded message.
    //! \param[in] stEmbeddedMessage_ A vector of field containers representing the embedded message fields.
    //!
    //! \return A STATUS code indicating the result of the encoding operation.
    //! Possible values include:
    //! - SUCCESS: The operation was successful.
    //! - BUFFER_FULL: The provided buffer was not large enough to hold the encoded message.
    //! - FAILURE: An error occurred during encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS EncodeAbbrevAscii(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                           MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_,
                                           MetaDataStruct& stEmbeddedMetaData_, IntermediateHeader& stEmbeddedHeader_,
                                           std::vector<FieldContainer>& stEmbeddedMessage_) const;

    //----------------------------------------------------------------------------
    //! \brief Encodes an RXCONFIG message into JSON format.
    //!
    //! \param[out] ppucBuffer_ A pointer to the buffer where the encoded JSON message will be written.
    //! \param[in] uiBufferSize_ The size of the buffer provided.
    //! \param[in] stHeader_ A reference to the intermediate header structure containing header information.
    //! \param[in] stMessageData_ A reference to the structure containing the main message data.
    //! \param[in] stEmbeddedMessageData_ A reference to the structure containing the embedded message data.
    //! \param[in] stEmbeddedMetaData_ A reference to the metadata structure for the embedded message.
    //! \param[in] stEmbeddedHeader_ A reference to the intermediate header for the embedded message.
    //! \param[in] stEmbeddedMessage_ A vector of field containers representing the embedded message fields.
    //!
    //! \return A STATUS code indicating the result of the encoding operation.
    //! Possible values include:
    //! - SUCCESS: The operation was successful.
    //! - BUFFER_FULL: The provided buffer was not large enough to hold the encoded message.
    //! - FAILURE: An error occurred during encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS EncodeJSON(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                    MessageDataStruct& stMessageData_, MessageDataStruct& stEmbeddedMessageData_, MetaDataStruct& stEmbeddedMetaData_,
                                    IntermediateHeader& stEmbeddedHeader_, std::vector<FieldContainer>& stEmbeddedMessage_) const;

  public:
    //! NOTE: The following constructors prevent this class from ever being
    //! constructed from a copy, move or assignment.
    RxConfigHandler(const RxConfigHandler&) = delete;
    RxConfigHandler(RxConfigHandler&&) = delete;
    RxConfigHandler& operator=(const RxConfigHandler&) = delete;
    RxConfigHandler& operator=(RxConfigHandler&&) = delete;

    //----------------------------------------------------------------------------
    //! \brief A constructor for the RxConfigHandler class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    RxConfigHandler(const MessageDatabase::Ptr& pclMessageDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief Returns whether a message ID corresponds to an RXCONFIG message.
    //!
    //! \param[in] usMessageId_ The message ID to check.
    //!
    //! \return True if the message ID corresponds to an RXCONFIG message, false otherwise.
    //------------------------------------------------------------------------------
    static bool IsRxConfigTypeMsg(uint16_t usMessageId_);

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(const MessageDatabase::Ptr& pclMessageDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_  The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Write bytes to the RxConfigHandler to be converted.
    //
    //! \param[in] pucData_ Buffer containing data to be written.
    //! \param[in] uiDataSize_ Size of data to be written.
    //
    //! \return The number of bytes successfully written to the RxConfigHandler.
    //----------------------------------------------------------------------------
    size_t Write(const unsigned char* pucData_, uint32_t uiDataSize_);

    //----------------------------------------------------------------------------
    //! \brief Read and convert an RXCONFIG message from the handler.
    //
    //! \param[out] stRxConfigMessageData_ A reference to a MessageDataStruct to be
    //! populated by the handler, referring to the RXCONFIG message.
    //! \param[out] stRxConfigMetaData_ A reference to a MetaDataStruct to be populated
    //! by the handler, referring to the RXCONFIG message.
    //! \param[out] stEmbeddedMessageData_ A reference to a MessageDataStruct to be
    //! populated by the handler, referring to the embedded message in RXCONFIG.
    //! \param[out] stEmbeddedMetaData_ A reference to a MetaDataStruct to be populated
    //! by the handler, referring to the embedded message in RXCONFIG.
    //! \param[in] eEncodeFormat_ An enum describing the format to encode the message to.
    //
    //! \return An error code describing the result of decoding and converting.
    //! See novatel::edie::oem::STATUS.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Convert(MessageDataStruct& stRxConfigMessageData_, MetaDataStruct& stRxConfigMetaData_,
                                 MessageDataStruct& stEmbeddedMessageData_, MetaDataStruct& stEmbeddedMetaData_, ENCODE_FORMAT eEncodeFormat_);

    //----------------------------------------------------------------------------
    //! \brief Decode the payload of an RXCONFIG message.
    //
    //! \param[in] pucMessage_ A pointer to a message payload.
    //! \param[out] stInterMessage_ The intermediate data structure to be populated.
    //! \param[in, out] stMetaData_ MetaDataStruct to provide information about
    //! the frame and be fully populated to help describe the decoded log.
    //
    //! \return A STATUS code describing the result of decoding.
    //!    SUCCESS: The operation was successful.
    //!    NO_DEFINITION: The message ID was not found in the database.
    //!    UNSUPPORTED: The message ID is not for an RXCONFIG type message.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Decode(const unsigned char* pucMessage_, std::vector<FieldContainer>& stInterMessage_,
                                MetaDataStruct& stRxConfigMetaData_) const;

    //----------------------------------------------------------------------------
    //! \brief Encode an RXConfig message from the provided intermediate structures.
    //
    //! \param[out] ppucBuffer_ A pointer to the buffer to return the encoded
    //! message to.
    //! \param[in] uiBufferSize_ The length of ppcBuffer_.
    //! \param[in] stHeader_ A reference to the decoded header intermediate.
    //! This must be populated by the HeaderDecoder.
    //! \param[in] stMessage_ A reference to the decoded message intermediate.
    //! This must be populated by the MessageDecoder.
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the encoder.
    //! \param[in] eFormat_ The format to encode the message to.
    //
    //! \return An error code describing the result of encoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: ppucBuffer_ either points to a null pointer or is
    //! a null pointer itself.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //!   FAILURE: stMessageData_.pucMessageHeader was not correctly set inside
    //! this function. This should not happen.
    //!   UNSUPPORTED: eFormat_ contains a format that is not supported for
    //! encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                const std::vector<FieldContainer>& stMessage_, MessageDataStruct& stMessageData_, ENCODE_FORMAT eFormat_) const;

    //----------------------------------------------------------------------------
    //! \brief Flush all bytes from the internal Framer.
    //
    //! \param[in] pucBuffer_ A buffer to contain flushed bytes, if desired.
    //! Defaults to NULL.
    //! \param[in] uiBufferSize_ The length of ulBufferSize_, if provided.
    //! Defaults to the size of the internal buffer (64kB).
    //
    //! \return The number of bytes flushed from the internal Framer.
    //----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_ = nullptr, uint32_t uiBufferSize_ = uiInternalBufferSize)
    {
        return clMyFramer.Flush(pucBuffer_, uiBufferSize_);
    }
};

} // namespace novatel::edie::oem

#endif // RXCONFIG_HANDLER_HPP
