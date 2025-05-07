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
// ! \file parser.hpp
// ===============================================================================

#ifndef NOVATEL_PARSER_HPP
#define NOVATEL_PARSER_HPP

#include <memory>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/oem/common.hpp"
#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/filter.hpp"
#include "novatel_edie/decoders/oem/framer.hpp"
#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/rangecmp/range_decompressor.hpp"
#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class Parser
//! \brief Handle all the functionality related to decoding an OEM message.
//! This involves identifying the message sync framing the complete message
//! and validating the CRC before passing the message to the application.
//============================================================================
class Parser
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger{GetBaseLoggerManager()->RegisterLogger("novatel_parser")};

    MessageDatabase::Ptr pclMyMessageDb;
    Filter::Ptr pclMyUserFilter;
    std::unique_ptr<oem::Framer> pclMyFramer;
    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    // Niche components
    RangeDecompressor clMyRangeDecompressor;
    RxConfigHandler clMyRxConfigHandler;

    // Filters for specific components
    Filter clMyRangeCmpFilter;

    std::unique_ptr<unsigned char[]> pcMyEncodeBuffer{std::make_unique<unsigned char[]>(uiParserInternalBufferSize)};
    unsigned char* pucMyEncodeBufferPointer{nullptr};
    std::unique_ptr<unsigned char[]> pcMyFrameBuffer{std::make_unique<unsigned char[]>(uiParserInternalBufferSize)};
    unsigned char* pucMyFrameBufferPointer{nullptr};

    // Configuration options
    bool bMyDecompressRangeCmp{true};
    bool bMyReturnUnknownBytes{true};
    bool bMyIgnoreAbbreviatedAsciiResponse{true};
    ENCODE_FORMAT eMyEncodeFormat{ENCODE_FORMAT::ASCII};

  public:
    //! \brief uiParserInternalBufferSize: the size of the parser's internal buffer.
    static constexpr uint32_t uiParserInternalBufferSize = MESSAGE_SIZE_MAX;

    //! NOTE: The following constructors prevent this class from ever being
    //! constructed from a copy, move or assignment.
    Parser(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser& operator=(Parser&&) = delete;

    //----------------------------------------------------------------------------
    //! \brief A constructor for the Parser class.
    //
    //! \param[in] sDbPath_ Filepath to a JSON message DB.
    //----------------------------------------------------------------------------
    Parser(const std::filesystem::path& sDbPath_);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the Parser class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    Parser(MessageDatabase::Ptr pclMessageDb_ = nullptr);

    // ---------------------------------------------------------------------------
    //! \brief Get the MessageDatabase object.
    //
    //! \return A shared pointer to the MessageDatabase object.
    // ---------------------------------------------------------------------------
    MessageDatabase::ConstPtr MessageDb() const;

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
    std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal components'
    //! internal loggers
    //
    //! \param[in] eLevel_ The logging level to enable.
    //! \param[in] sFileName_ The logging level to enable.
    //----------------------------------------------------------------------------
    void EnableFramerDecoderLogging(spdlog::level::level_enum eLevel_ = spdlog::level::debug, const std::string& sFileName_ = "edie.log");

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Set the abbreviated ASCII response option.
    //
    //! \param[in] bIgnoreAbbreviatedAsciiResponses_ true to ignore abbreviated
    //! ASCII responses.
    //----------------------------------------------------------------------------
    void SetIgnoreAbbreviatedAsciiResponses(bool bIgnoreAbbreviatedAsciiResponses_)
    {
        bMyIgnoreAbbreviatedAsciiResponse = bIgnoreAbbreviatedAsciiResponses_;
    }

    //----------------------------------------------------------------------------
    //! \brief Get the abbreviated ASCII response option.
    //
    //! \return The current option for ignoring abbreviated ASCII responses.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool GetIgnoreAbbreviatedAsciiResponses() const { return bMyIgnoreAbbreviatedAsciiResponse; }

    //----------------------------------------------------------------------------
    //! \brief Set the decompression option for RANGECMP messages.
    //
    //! \param[in] bDecompressRangeCmp_ true to decompress RANGECMP messages.
    //----------------------------------------------------------------------------
    void SetDecompressRangeCmp(bool bDecompressRangeCmp_) { bMyDecompressRangeCmp = bDecompressRangeCmp_; }

    //----------------------------------------------------------------------------
    //! \brief Get the decompression option for RANGECMP messages.
    //
    //! \return The current option for decompressing RANGECMP messages.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool GetDecompressRangeCmp() const { return bMyDecompressRangeCmp; }

    //----------------------------------------------------------------------------
    //! \brief Set the return option for unknown bytes.
    //
    //! \param[in] bReturnUnknownBytes_ true to return unknown bytes.
    //----------------------------------------------------------------------------
    void SetReturnUnknownBytes(bool bReturnUnknownBytes_) { bMyReturnUnknownBytes = bReturnUnknownBytes_; }

    //----------------------------------------------------------------------------
    //! \brief Get the return option for unknown bytes.
    //
    //! \return The current option for returning unknown bytes.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool GetReturnUnknownBytes() const { return bMyReturnUnknownBytes; }

    //----------------------------------------------------------------------------
    //! \brief Set the encode format for messages.
    //
    //! \param[in] eFormat_ the encode format for future messages.
    //----------------------------------------------------------------------------
    void SetEncodeFormat(ENCODE_FORMAT eFormat_) { eMyEncodeFormat = eFormat_; }

    //----------------------------------------------------------------------------
    //! \brief Get the encode format for messages.
    //
    //! \return The current encode format for messages.
    //----------------------------------------------------------------------------
    [[nodiscard]] ENCODE_FORMAT GetEncodeFormat() const { return eMyEncodeFormat; }

    //----------------------------------------------------------------------------
    //! \brief Set the Filter for the Parser.
    //
    //! \param[in] pclFilter_ A pointer to an OEM message Filter object.
    //----------------------------------------------------------------------------
    void SetFilter(const Filter::Ptr& pclFilter_) { pclMyUserFilter = pclFilter_; }

    //----------------------------------------------------------------------------
    //! \brief Get the config for the Parser.
    //
    //! \return A pointer to the Parser's OEM message Filter object.
    //----------------------------------------------------------------------------
    const Filter::Ptr& GetFilter() const { return pclMyUserFilter; }

    //----------------------------------------------------------------------------
    //! \brief Get a pointer to the current framed log raw data.
    //
    //! \return A pointer to the Parser's internal encode buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] unsigned char* GetInternalBuffer() const { return pucMyEncodeBufferPointer; }

    //----------------------------------------------------------------------------
    //! \brief Write bytes to the Parser to be parsed.
    //
    //! \param[in] pucData_ Buffer containing data to be written.
    //! \param[in] uiDataSize_ Size of data to be written.
    //
    //! \return The number of bytes successfully written to the Parser.
    //----------------------------------------------------------------------------
    [[nodiscard]] size_t Write(const unsigned char* pucData_, size_t uiDataSize_) { return clMyFramer.Write(pucData_, uiDataSize_); }

    //----------------------------------------------------------------------------
    //! \brief Get the number of bytes available in the Parser's internal buffer.
    //!
    //! \return The number of bytes available in the Parser's internal buffer for writing new data.
    //---------------------------------------------------------------------------
    [[nodiscard]] size_t GetAvailableSpace() const { return clMyFramer.GetAvailableSpace(); }

    //----------------------------------------------------------------------------
    //! \brief Read a log from the Parser.
    //
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the Parser.
    //! \param[out] stMetaData_ A reference to a MetaDataStruct to be populated
    //! by the Parser.
    //! \param[in] bDecodeIncompleteAbbreviated_ If at the end of the data stream decode
    //! last Abbreviated Ascii message if it's incomplete. Set to true when the data
    //! stream is empty and the parser has returned BUFFER_EMPTY
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   BUFFER_EMPTY: There are no more bytes to parse in the Parser.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_, bool bDecodeIncompleteAbbreviated_ = false);

    //----------------------------------------------------------------------------
    //! \brief Retrive the intermediate representations of a message from a parser.
    //
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the Parser.
    //! \param[out] header_ A reference to a IntermediateHeader to be
    //! populated by the Parser.
    //! \param[out] stMessage_ A reference to a FieldContainer to be populated
    //! by the Parser.
    //! \param[out] stMetaData_ A reference to a MetaDataStruct to be populated
    //! by the Parser.
    //! \param[out] bDecodeIncompleteAbbreviated_ If at the end of the data stream decode
    //! last Abbreviated Ascii message if it's incomplete. Set to true when the data
    //! stream is empty and the parser has returned BUFFER_EMPTY
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   BUFFER_EMPTY: There are no more bytes to parse in the Parser.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS ReadIntermediate(MessageDataStruct& stMessageData_, IntermediateHeader& header_, std::vector<FieldContainer>& stMessage_,
                                          MetaDataStruct& stMetaData_, bool bDecodeIncompleteAbbreviated_ = false);

    //----------------------------------------------------------------------------
    //! \brief Flush all bytes from the internal Parser.
    //
    //! \param[in] pucBuffer_ A buffer to contain flushed bytes, if desired.
    //! Defaults to NULL.
    //! \param[in] uiBufferSize_ The length of ulBufferSize_, if provided.
    //
    //! \return The number of bytes flushed from the internal Parser.
    //----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_ = nullptr, uint32_t uiBufferSize_ = uiParserInternalBufferSize);
};

} // namespace novatel::edie::oem

#endif // PARSER_H
