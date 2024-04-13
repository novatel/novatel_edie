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

#ifndef NOVATEL_ENCODER_HPP
#define NOVATEL_ENCODER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <cassert>
#include <logger/logger.hpp>
#include <nlohmann/json.hpp>

#include "decoders/common/api/common.hpp"
#include "decoders/common/api/encoder.hpp"
#include "decoders/common/api/jsonreader.hpp"
#include "decoders/common/api/message_decoder.hpp"
#include "decoders/novatel/api/common.hpp"

using nlohmann::json;

namespace novatel::edie::oem {

//============================================================================
//! \class Encoder
//! \brief Class to encode OEM messages.
//============================================================================
class Encoder : public EncoderBase
{
  private:
    // Enum util functions
    void InitEnumDefns();
    static void InitFieldMaps();
    std::string JsonHeaderToMsgName(const IntermediateHeader& stInterHeader_) const;

  protected:
    char separatorASCII() const override { return OEM4_ASCII_FIELD_SEPARATOR; };
    char separatorAbbASCII() const override { return OEM4_ABBREV_ASCII_SEPARATOR; };
    uint32_t indentationLengthAbbASCII() const override { return OEM4_ABBREV_ASCII_INDENTATION_LENGTH; };

    // Encode binary
    [[nodiscard]] bool EncodeBinaryHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_);
    [[nodiscard]] bool EncodeBinaryShortHeader(const IntermediateHeader& stInterHeader_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_);
    [[nodiscard]] bool FieldToBinary(const FieldContainer& fc_, unsigned char** ppcOutBuf_, uint32_t& uiBytesLeft_);

    // Encode ascii
    [[nodiscard]] bool EncodeAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_);
    [[nodiscard]] bool EncodeAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_);
    [[nodiscard]] bool EncodeAbbrevAsciiHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_,
                                               bool bIsEmbeddedHeader_ = false);
    [[nodiscard]] bool EncodeAbbrevAsciiShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_);

    // Encode JSON
    [[nodiscard]] bool EncodeJsonHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_);
    [[nodiscard]] bool EncodeJsonShortHeader(const IntermediateHeader& stInterHeader_, char** ppcOutBuf_, uint32_t& uiBytesLeft_);

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
    //! \param[out] ppucEncodeBuffer_ A pointer to the buffer to return the encoded
    //! message to.
    //! \param[in] uiEncodeBufferSize_ The length of ppcEncodeBuffer_.
    //! \param[in] stHeader_ A reference to the decoded header intermediate.
    //! This must be populated by the HeaderDecoder.
    //! \param[in] stMessage_ A reference to the decoded message intermediate.
    //! This must be populated by the MessageDecoder.
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the encoder.
    //! \param[in] stMetaData_ A reference to a populated MetaDataStruct
    //! containing relevant information about the decoded log.
    //! This must be populated by the Framer and HeaderDecoder.
    //! \param[in] eEncodeFormat_ The format to encode the message to.
    //
    //! \return An error code describing the result of encoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: ppucEncodeBuffer_ either points to a null pointer or is
    //! a null pointer itself.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   BUFFER_FULL: An attempt was made to write bytes to the provided buffer,
    //! but the buffer is already full or could not write the bytes without
    //! over-running.
    //!   FAILURE: stMessageData_.pucMessageHeader was not correctly set inside
    //! this function.  This should not happen.
    //!   UNSUPPORTED: eEncodeFormat_ contains a format that is not supported for
    //! encoding.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Encode(unsigned char** ppucEncodeBuffer_, uint32_t uiEncodeBufferSize_, IntermediateHeader& stHeader_,
                                IntermediateMessage& stMessage_, MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_,
                                ENCODEFORMAT eEncodeFormat_);

    //----------------------------------------------------------------------------
    //! \brief Encode an OEM message header from the provided intermediate header.
    //
    //! \param[out] ppucEncodeBuffer_ A pointer to the buffer to return the encoded
    //! message to.
    //! \param[in] uiEncodeBufferSize_ The length of ppcEncodeBuffer_.
    //! \param[in] stHeader_ A reference to the decoded header intermediate.
    //! This must be populated by the HeaderDecoder.
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the encoder.
    //! \param[in] stMetaData_ A reference to a populated MetaDataStruct
    //! containing relevant information about the decoded log.
    //! This must be populated by the Framer and HeaderDecoder.
    //! \param[in] eEncodeFormat_ The format to encode the message to.
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
    [[nodiscard]] STATUS EncodeHeader(unsigned char** ppucEncodeBuffer_, uint32_t uiEncodeBufferSize_, IntermediateHeader& stHeader_,
                                      MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_, ENCODEFORMAT eEncodeFormat_,
                                      bool bIsEmbeddedHeader_ = false);

    //----------------------------------------------------------------------------
    //! \brief Encode an OEM message body from the provided intermediate message.
    //
    //! \param[out] ppucEncodeBuffer_ A pointer to the buffer to return the encoded
    //! message to.
    //! \param[in] uiEncodeBufferSize_ The length of ppcEncodeBuffer_.
    //! \param[in] stMessage_ A reference to the decoded message intermediate.
    //! This must be populated by the MessageDecoder.
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the encoder.
    //! \param[in] stMetaData_ A reference to a populated MetaDataStruct
    //! containing relevant information about the decoded log.
    //! This must be populated by the Framer and HeaderDecoder.
    //! \param[in] eEncodeFormat_ The format to encode the message to.
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
    [[nodiscard]] STATUS EncodeBody(unsigned char** ppucEncodeBuffer_, uint32_t uiEncodeBufferSize_, IntermediateMessage& stMessage_,
                                    MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_, ENCODEFORMAT eEncodeFormat_);
};
} // namespace novatel::edie::oem
#endif // NOVATEL_ENCODER_HPP
