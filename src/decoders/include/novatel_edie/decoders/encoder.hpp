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
// ! \file encoder.hpp
// ===============================================================================

#ifndef NOVATEL_EDIE_DECODERS_ENCODER_HPP
#define NOVATEL_EDIE_DECODERS_ENCODER_HPP

#include <nlohmann/json.hpp>

#include "novatel_edie/common/common.hpp"
#include "novatel_edie/common/encoder.hpp"
#include "novatel_edie/common/json_reader.hpp"
#include "novatel_edie/common/message_decoder.hpp"
#include "novatel_edie/decoders/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class Encoder
//! \brief Class to encode OEM messages.
//============================================================================
class Encoder : public EncoderBase
{
  private:
    // Enum util functions
    void InitEnumDefinitions() override;
    static void InitFieldMaps();
    [[nodiscard]] std::string JsonHeaderToMsgName(const IntermediateHeader& stInterHeader_) const;

  protected:
    [[nodiscard]] char SeparatorAscii() const override { return OEM4_ASCII_FIELD_SEPARATOR; }
    [[nodiscard]] char SeparatorAbbAscii() const override { return OEM4_ABBREV_ASCII_SEPARATOR; }
    [[nodiscard]] uint32_t IndentationLengthAbbAscii() const override { return OEM4_ABBREV_ASCII_INDENTATION_LENGTH; }

    // Encode binary
    [[nodiscard]] bool EncodeBinaryHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_);
    [[nodiscard]] bool EncodeBinaryShortHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_);
    [[nodiscard]] bool FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_) override;

    // Encode ascii
    [[nodiscard]] bool EncodeAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;
    [[nodiscard]] bool EncodeAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;
    [[nodiscard]] bool EncodeAbbrevAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                               bool bIsEmbeddedHeader_ = false) const;
    [[nodiscard]] bool EncodeAbbrevAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;

    // Encode JSON
    [[nodiscard]] bool EncodeJsonHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;
    [[nodiscard]] bool EncodeJsonShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_) const;

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Encoder class.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object.  Defaults to nullptr.
    //----------------------------------------------------------------------------
    Encoder(JsonReader* pclJsonDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief Encode an OEM message from the provided intermediate structures.
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
    //! \param[in] stMetaData_ A reference to a populated MetaDataStruct
    //! containing relevant information about the decoded log.
    //! This must be populated by the Framer and HeaderDecoder.
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
    //! this function.  This should not happen.
    //!   UNSUPPORTED: eFormat_ contains a format that is not supported for
    //! encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Encode(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                const std::vector<FieldContainer>& stMessage_, MessageDataStruct& stMessageData_, const MetaDataStruct& stMetaData_,
                                ENCODE_FORMAT eFormat_);

    //----------------------------------------------------------------------------
    //! \brief Encode an OEM message header from the provided intermediate header.
    //
    //! \param[out] ppucBuffer_ A pointer to the buffer to return the encoded
    //! message to.
    //! \param[in] uiBufferSize_ The length of ppcEncodeBuffer_.
    //! \param[in] stHeader_ A reference to the decoded header intermediate.
    //! This must be populated by the HeaderDecoder.
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the encoder.
    //! \param[in] stMetaData_ A reference to a populated MetaDataStruct
    //! containing relevant information about the decoded log.
    //! This must be populated by the Framer and HeaderDecoder.
    //! \param[in] eFormat_ The format to encode the message to.
    //! \param[in] bIsEmbeddedHeader_ This header is embedded in an RXCONFIG
    //! message, and should be treated as such.  Is a default argument and is
    //! defaulted as false.
    //
    //! \return An error code describing the result of encoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: ppucEncodeBuffer_ either points to a null pointer or is
    //! a null pointer itself.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //!   UNSUPPORTED: eEncodeFormat_ contains a format that is not supported for
    //! encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS EncodeHeader(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const IntermediateHeader& stHeader_,
                                      MessageDataStruct& stMessageData_, const MetaDataStruct& stMetaData_, ENCODE_FORMAT eFormat_,
                                      bool bIsEmbeddedHeader_ = false);

    //----------------------------------------------------------------------------
    //! \brief Encode an OEM message body from the provided intermediate message.
    //
    //! \param[out] ppucBuffer_ A pointer to the buffer to return the encoded
    //! message to.
    //! \param[in] uiBufferSize_ The length of ppcEncodeBuffer_.
    //! \param[in] stMessage_ A reference to the decoded message intermediate.
    //! This must be populated by the MessageDecoder.
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the encoder.
    //! \param[in] stMetaData_ A reference to a populated MetaDataStruct
    //! containing relevant information about the decoded log.
    //! This must be populated by the Framer and HeaderDecoder.
    //! \param[in] eFormat_ The format to encode the message to.
    //
    //! \return An error code describing the result of encoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: ppucEncodeBuffer_ either points to a null pointer or is
    //! a null pointer itself.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //!   FAILURE: stMessageData_.pucMessageHeader is a null pointer.
    //!   UNSUPPORTED: eEncodeFormat_ contains a format that is not supported for
    //! encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS EncodeBody(unsigned char** ppucBuffer_, uint32_t uiBufferSize_, const std::vector<FieldContainer>& stMessage_,
                                    MessageDataStruct& stMessageData_, const MetaDataStruct& stMetaData_, ENCODE_FORMAT eFormat_);
};

} // namespace novatel::edie::oem

#endif // NOVATEL_EDIE_DECODERS_ENCODER_HPP
