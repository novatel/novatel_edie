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

#include "novatel_edie/common/framer_manager.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/json_reader.hpp"
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
    static constexpr auto szAbbrevAsciiEmbeddedHeaderPrefix = "<     ";
    static constexpr uint32_t uiInternalBufferSize = MESSAGE_SIZE_MAX;

    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    JsonReader* pclMyMsgDb{};
    EnumDefinition* vMyCommandDefinitions{};
    EnumDefinition* vMyPortAddressDefinitions{};
    EnumDefinition* vMyGpsTimeStatusDefinitions{};

    std::unique_ptr<unsigned char[]> pcMyFrameBuffer;
    std::unique_ptr<unsigned char[]> pcMyEncodeBuffer;

    static bool IsRxConfigTypeMsg(uint16_t usMessageId_);

  public:
    //! NOTE: The following constructors prevent this class from ever being
    //! constructed from a copy, move or assignment.
    RxConfigHandler(const RxConfigHandler&) = delete;
    RxConfigHandler(const RxConfigHandler&&) = delete;
    RxConfigHandler& operator=(const RxConfigHandler&) = delete;

    //----------------------------------------------------------------------------
    //! \brief A constructor for the RxConfigHandler class.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    RxConfigHandler(JsonReader* pclJsonDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief Load a JsonReader object.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(JsonReader* pclJsonDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const;

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_  The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const;

    //----------------------------------------------------------------------------
    //! \brief Write bytes to the RxConfigHandler to be converted.
    //
    //! \param[in] pucData_ Buffer containing data to be written.
    //! \param[in] uiDataSize_ Size of data to be written.
    //
    //! \return The number of bytes successfully written to the RxConfigHandler.
    //----------------------------------------------------------------------------
    uint32_t Write(unsigned char* pucData_, uint32_t uiDataSize_);

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
    //! \brief Flush all bytes from the internal Framer.
    //
    //! \param[in] pucBuffer_ A buffer to contain flushed bytes, if desired.
    //! Defaults to NULL.
    //! \param[in] uiBufferSize_ The length of ulBufferSize_, if provided.
    //! Defaults to the size of the internal buffer (64kB).
    //
    //! \return The number of bytes flushed from the internal Framer.
    //----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_ = nullptr, uint32_t uiBufferSize_ = uiInternalBufferSize);
};

} // namespace novatel::edie::oem

#endif // EXTENSION_RXCONFIG_HANDLER_HPP
