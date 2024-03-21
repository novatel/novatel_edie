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
//! \file parser.hpp
//! \brief Parse bytes for OEM logs.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef NOVATEL_PARSER_HPP
#define NOVATEL_PARSER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <exception>

#include "decoders/common/api/common.hpp"
#include "decoders/common/api/message_decoder.hpp"
#include "decoders/novatel/api/common.hpp"
#include "decoders/novatel/api/encoder.hpp"
#include "decoders/novatel/api/filter.hpp"
#include "decoders/novatel/api/framer.hpp"
#include "decoders/novatel/api/header_decoder.hpp"
#include "decoders/novatel/api/rangecmp/common.hpp"
#include "decoders/novatel/api/rangecmp/range_decompressor.hpp"
#include "decoders/novatel/api/rxconfig/rxconfig_handler.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class Parser
//! \brief Handle all the functionality related to decoding an OEM message.
//! This involves identifying the message sync framing the complete message
//! and validating the CRC before passing the message to the application.
//============================================================================
class Parser
{
    //! TODO: Manage copy/move/assignment constructors better.
    //! NOTE: The following constructors prevent this class from ever being
    //! constructed from a copy, move or assignment.
    Parser(const Parser&) = delete;
    Parser(const Parser&&) = delete;
    Parser& operator=(const Parser&) = delete;

  public:
    //! \brief uiPARSER_INTERNAL_BUFFER_SIZE: the size of the parser's internal buffer.
    static constexpr uint32_t uiPARSER_INTERNAL_BUFFER_SIZE = MESSAGE_SIZE_MAX;

  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{Logger::RegisterLogger("novatel_parser")};

    JsonReader clMyJsonReader;
    Filter* pclMyUserFilter{nullptr};
    Framer clMyFramer;
    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    // Niche components
    RangeDecompressor clMyRangeDecompressor;
    RxConfigHandler clMyRxConfigHandler;

    // Filters for specific components
    Filter clMyRangeCmpFilter;
    Filter clMyRxConfigFilter;

    unsigned char* const pcMyEncodeBuffer{nullptr};
    unsigned char* pucMyEncodeBufferPointer{nullptr};
    unsigned char* const pcMyFrameBuffer{nullptr};
    unsigned char* pucMyFrameBufferPointer{nullptr};

    // Configuration options
    bool bMyDecompressRangeCmp{true};
    bool bMyReturnUnknownBytes{true};
    bool bMyIgnoreAbbreviatedASCIIResponse{true};
    ENCODEFORMAT eMyEncodeFormat{ENCODEFORMAT::ASCII};

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Parser class.
    //
    //! \param[in] sDbPath_ Filepath to a JSON message DB.
    //----------------------------------------------------------------------------
    Parser(const std::string sDbPath_);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the Parser class.
    //
    //! \param[in] sDbPath_ Filepath to a JSON message DB.
    //----------------------------------------------------------------------------
    Parser(const std::u32string sDbPath_);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the Parser class.
    //
    //! \param[in] pclJsonDb_ A pointer to a JsonReader object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    Parser(JsonReader* pclJsonDb_ = nullptr);

    //----------------------------------------------------------------------------
    //! \brief A destructor for the Parser class.
    //----------------------------------------------------------------------------
    ~Parser();

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
    std::shared_ptr<spdlog::logger> GetLogger();

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal components'
    //! internal loggers
    //
    //! \param [in] eLevel_ The logging level to enable.
    //! \param [in] sFileName_ The logging level to enable.
    //----------------------------------------------------------------------------
    void EnableFramerDecoderLogging(spdlog::level::level_enum eLevel_ = spdlog::level::debug, std::string sFileName_ = "edie.log");

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_ The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_);

    //----------------------------------------------------------------------------
    //! \brief Shutdown the internal logger.
    //----------------------------------------------------------------------------
    void ShutdownLogger();

    //----------------------------------------------------------------------------
    //! \brief Set the abbreviated ASCII response option.
    //
    //! \param [in] bIgnoreAbbreivatedAsciiResponses_ true to ignore abbreivated
    //! ASCII responses.
    //----------------------------------------------------------------------------
    void SetIgnoreAbbreviatedAsciiResponses(bool bIgnoreAbbreivatedAsciiResponses_);

    //----------------------------------------------------------------------------
    //! \brief Get the abbreviated ASCII response option.
    //
    //! \return The current option for ignoring abbreviated ASCII responses.
    //----------------------------------------------------------------------------
    bool GetIgnoreAbbreviatedAsciiResponses(void);

    //----------------------------------------------------------------------------
    //! \brief Set the decompression option for RANGECMP messages.
    //
    //! \param [in] bDecompressRangeCmp_ true to decompress RANGECMP messages.
    //----------------------------------------------------------------------------
    void SetDecompressRangeCmp(bool bDecompressRangeCmp_);

    //----------------------------------------------------------------------------
    //! \brief Get the decompression option for RANGECMP messages.
    //
    //! \return The current option for decompressing RANGECMP messages.
    //----------------------------------------------------------------------------
    bool GetDecompressRangeCmp();

    //----------------------------------------------------------------------------
    //! \brief Set the return option for unknown bytes.
    //
    //! \param [in] bReturnUnknownBytes_ true to return unknown bytes.
    //----------------------------------------------------------------------------
    void SetReturnUnknownBytes(bool bReturnUnknownBytes_);

    //----------------------------------------------------------------------------
    //! \brief Get the return option for unknown bytes.
    //
    //! \return The current option for returning unknown bytes.
    //----------------------------------------------------------------------------
    bool GetReturnUnknownBytes();

    //----------------------------------------------------------------------------
    //! \brief Set the encode format for messages.
    //
    //! \param [in] eFormat_ the encode format for future messages.
    //----------------------------------------------------------------------------
    void SetEncodeFormat(ENCODEFORMAT eFormat_);

    //----------------------------------------------------------------------------
    //! \brief Get the encode format for messages.
    //
    //! \return The current encode format for messages.
    //----------------------------------------------------------------------------
    ENCODEFORMAT
    GetEncodeFormat();

    //----------------------------------------------------------------------------
    //! \brief Set the Filter for the FileParser.
    //
    //! \param [in] pclFilter_ A pointer to an OEM message Filter object.
    //----------------------------------------------------------------------------
    void SetFilter(Filter* pclFilter_);

    //----------------------------------------------------------------------------
    //! \brief Get the config for the FileParser.
    //
    //! \return A pointer to the FileParser's OEM message Filter object.
    //----------------------------------------------------------------------------
    Filter* GetFilter();

    //----------------------------------------------------------------------------
    //! \brief Get a pointer to the current framed log raw data.
    //
    //! \return A pointer to the Parser's internal encode buffer.
    //----------------------------------------------------------------------------
    unsigned char* GetInternalBuffer();

    //----------------------------------------------------------------------------
    //! \brief Write bytes to the Parser to be parsed.
    //
    //! \param [in] pucData_ Buffer containing data to be written.
    //! \param [in] uiDataSize_ Size of data to be written.
    //
    //! \return The number of bytes successfully written to the Parser.
    //----------------------------------------------------------------------------
    uint32_t Write(unsigned char* pucData_, uint32_t uiDataSize_);

    //----------------------------------------------------------------------------
    //! \brief Read a log from the Parser.
    //
    //! \param [out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the FileParser.
    //! \param [out] stMetaData_ A reference to a MetaDataStruct to be populated
    //! by the FileParser.
    //! \param [out] bDecodeIncompleteAbbv If at the end of the datastream decode
    //! last Abbv Ascii message if it's incomplete. Set to true when the data
    //! stream is empty and the parser has returned BUFFER_EMPTY
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   BUFFER_EMPTY: There are no more bytes to parse in the Parser.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_, bool bDecodeIncompleteAbbv = false);

    //----------------------------------------------------------------------------
    //! \brief Flush all bytes from the internal Parser.
    //
    //! \param [in] pucBuffer_ A buffer to contain flushed bytes, if desired.
    //! Defaults to NULL.
    //! \param [in] uiBufferSize_ The length of ulBufferSize_, if provided.
    //
    //! \return The number of bytes flushed from the internal Parser.
    //----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_ = nullptr, uint32_t uiBufferSize_ = uiPARSER_INTERNAL_BUFFER_SIZE);
};
} // namespace novatel::edie::oem
#endif // PARSER_H
