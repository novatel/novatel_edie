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
// ! \file header_decoder.hpp
// ===============================================================================

#ifndef NOVATEL_HEADER_DECODER_HPP
#define NOVATEL_HEADER_DECODER_HPP

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class HeaderDecoder
//! \brief Decode framed OEM message headers.
//============================================================================
class HeaderDecoder
{
  private:
    //! \brief Type alias for message counts key.
    //! \details Tuple containing: format (ASCII, binary, etc.), message ID (uint16_t), sibling ID (uint8_t).
    using MessageCountsKey = std::tuple<HEADER_FORMAT, uint16_t, uint8_t>;

    struct MessageCountsKeyHash {
        std::size_t operator()(const MessageCountsKey& key) const {
            auto h1 = std::hash<int>{}(static_cast<int>(std::get<0>(key)));
            auto h2 = std::hash<uint16_t>{}(std::get<1>(key));
            auto h3 = std::hash<uint8_t>{}(std::get<2>(key));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("novatel_header_decoder")};
    MessageDatabase::Ptr pclMyMsgDb{nullptr};
    EnumDefinition::ConstPtr vMyCommandDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyPortAddressDefinitions{nullptr};
    EnumDefinition::ConstPtr vMyGpsTimeStatusDefinitions{nullptr};
    MessageDefinition stMyResponseDefinition;
    mutable std::unordered_map<MessageCountsKey, uint64_t, MessageCountsKeyHash> mapMyMessageCounts{};

    // Decode novatel headers
    template <const char pcDelimiter[], ASCII_HEADER eField>
    [[nodiscard]] bool DecodeAsciiHeaderField(IntermediateHeader& stInterHeader_, const char** ppcLogBuf_) const;
    template <const char pcDelimiter[], ASCII_HEADER... eFields>
    [[nodiscard]] bool DecodeAsciiHeaderFields(IntermediateHeader& stInterHeader_, const char** ppcLogBuf_) const;
    void DecodeJsonHeader(std::string_view pcTempBuf_, IntermediateHeader& stInterHeader_) const;

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the HeaderDecoder class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    HeaderDecoder(MessageDatabase::Ptr pclMessageDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(MessageDatabase::Ptr pclMessageDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger() { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Decode an OEM message header from the provided frame.
    //
    //! \param[in] pucLogBuf_ A pointer to an OEM message header.
    //! \param[out] stInterHeader_ The IntermediateHeader to be populated.
    //! \param[in, out] stMetaData_ MetaDataStruct to provide information about
    //! the frame and be fully populated to help describe the decoded log.
    //
    //! \return An error code describing the result of decoding.
    //!   SUCCESS: The operation was successful.
    //!   NULL_PROVIDED: pucHeader_ is a null pointer.
    //!   NO_DATABASE: No database was ever loaded into this component.
    //!   FAILURE: Failed to decode a header field.
    //!   UNSUPPORTED: Attempted to decode an unsupported format.
    //!   UNKNOWN: The header format provided is not known.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Decode(const unsigned char* pucLogBuf_, IntermediateHeader& stInterHeader_, MetaDataStruct& stMetaData_) const;
};

} // namespace novatel::edie::oem

#endif
