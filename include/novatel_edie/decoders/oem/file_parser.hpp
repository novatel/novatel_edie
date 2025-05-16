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
// ! \file file_parser.hpp
// ===============================================================================

#ifndef NOVATEL_FILE_PARSER_HPP
#define NOVATEL_FILE_PARSER_HPP

#include <filesystem>
#include <iosfwd>
#include <memory>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/oem/parser.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class FileParser
//! \brief Frame, decode and re-encode OEM logs from an InputFileStream.
//============================================================================
class FileParser
{
  private:
    std::shared_ptr<spdlog::logger> pclMyLogger{pclLoggerManager->RegisterLogger("novatel_file_parser")};
    Parser clMyParser;
    std::shared_ptr<std::istream> pclMyInputStream{nullptr};

    [[nodiscard]] bool ReadStream();

    //----------------------------------------------------------------------------
    //! \brief Read a message from the FileParser.
    //
    //! \param[in] parserFunc_ the function used to read the message from the Parser.
    //! \param[in] args_ A forwarding reference to the arguments of the ParserFunc.
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   STREAM_EMPTY: There are no more bytes to parse in the file provided.
    //----------------------------------------------------------------------------
    template <typename ParserFunc, typename... Args> [[nodiscard]] STATUS HandleRead(ParserFunc parserFunc_, Args&&... args_);

  public:
    //! NOTE: The following constructors prevent this class from ever being
    //! constructed from a copy, move or assignment.
    FileParser(const FileParser&) = delete;
    FileParser(FileParser&&) = delete;
    FileParser& operator=(const FileParser&) = delete;
    FileParser& operator=(FileParser&&) = delete;

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FileParser class.
    //
    //! \param[in] sDbPath_ Filepath to a JSON message DB.
    //----------------------------------------------------------------------------
    FileParser(const std::filesystem::path& sDbPath_);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FileParser class.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object. Defaults to nullptr.
    //----------------------------------------------------------------------------
    FileParser(const MessageDatabase::Ptr& pclMessageDb_ = {nullptr});

    ~FileParser() = default; 

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(const MessageDatabase::Ptr& pclMessageDb_);

    // ---------------------------------------------------------------------------
    //! \brief Get the MessageDatabase object.
    //
    //! \return A shared pointer to the MessageDatabase object.
    // ---------------------------------------------------------------------------
    MessageDatabase::ConstPtr MessageDb() const;

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal components'
    //! internal loggers
    //
    //! \param[in] eLevel_ The logging level to enable.
    //! \param[in] sFileName_ The logging level to enable.
    //----------------------------------------------------------------------------
    void EnableFramerDecoderLogging(spdlog::level::level_enum eLevel_ = spdlog::level::debug, const std::string& sFileName_ = "edie.log")
    {
        clMyParser.EnableFramerDecoderLogging(eLevel_, sFileName_);
    }

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
        clMyParser.SetIgnoreAbbreviatedAsciiResponses(bIgnoreAbbreviatedAsciiResponses_);
    }

    //----------------------------------------------------------------------------
    //! \brief Get the abbreviated ASCII response option.
    //
    //! \return The current option for ignoring abbreviated ASCII responses.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool GetIgnoreAbbreviatedAsciiResponses() const { return clMyParser.GetIgnoreAbbreviatedAsciiResponses(); }

    //----------------------------------------------------------------------------
    //! \brief Set the decompression option for RANGECMP messages.
    //
    //! \param[in] bDecompressRangeCmp_ true to decompress RANGECMP messages.
    //----------------------------------------------------------------------------
    void SetDecompressRangeCmp(bool bDecompressRangeCmp_) { clMyParser.SetDecompressRangeCmp(bDecompressRangeCmp_); }

    //----------------------------------------------------------------------------
    //! \brief Get the decompression option for RANGECMP messages.
    //
    //! \return The current option for decompressing RANGECMP messages.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool GetDecompressRangeCmp() const { return clMyParser.GetDecompressRangeCmp(); }

    //----------------------------------------------------------------------------
    //! \brief Set the return option for unknown bytes.
    //
    //! \param[in] bReturnUnknownBytes_ true to return unknown bytes.
    //----------------------------------------------------------------------------
    void SetReturnUnknownBytes(bool bReturnUnknownBytes_) { clMyParser.SetReturnUnknownBytes(bReturnUnknownBytes_); }

    //----------------------------------------------------------------------------
    //! \brief Get the return option for unknown bytes.
    //
    //! \return The current option for returning unknown bytes.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool GetReturnUnknownBytes() const { return clMyParser.GetReturnUnknownBytes(); }

    //----------------------------------------------------------------------------
    //! \brief Set the encode format for messages.
    //
    //! \param[in] eFormat_ the encode format for future messages.
    //----------------------------------------------------------------------------
    void SetEncodeFormat(ENCODE_FORMAT eFormat_) { clMyParser.SetEncodeFormat(eFormat_); }

    //----------------------------------------------------------------------------
    //! \brief Get the encode format for messages.
    //
    //! \return The current encode format for messages.
    //----------------------------------------------------------------------------
    [[nodiscard]] ENCODE_FORMAT GetEncodeFormat() const { return clMyParser.GetEncodeFormat(); }

    //----------------------------------------------------------------------------
    //! \brief Set the Filter for the FileParser.
    //
    //! \param[in] pclFilter_ A pointer to an OEM message Filter object.
    //----------------------------------------------------------------------------
    void SetFilter(const Filter::Ptr& pclFilter_) { clMyParser.SetFilter(pclFilter_); }

    //----------------------------------------------------------------------------
    //! \brief Get the config for the FileParser.
    //
    //! \return A pointer to the FileParser's OEM message Filter object.
    //----------------------------------------------------------------------------
    [[nodiscard]] const Filter::Ptr& GetFilter() const { return clMyParser.GetFilter(); }

    //----------------------------------------------------------------------------
    //! \brief Set the InputFileStream for the FileParser.
    //
    //! \param[in] pclInputStream_ A pointer to the input stream.
    //
    //! \return A boolean describing if the operation was successful
    //----------------------------------------------------------------------------
    [[nodiscard]] bool SetStream(std::shared_ptr<std::istream> pclInputStream_);

    //----------------------------------------------------------------------------
    //! \brief Read a log from the FileParser.
    //
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the FileParser.
    //! \param[out] stMetaData_ A reference to a MetaDataStruct to be populated
    //! by the FileParser.
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   STREAM_EMPTY: There are no more bytes to parse in the file provided.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Read(MessageDataStruct& stMessageData_, MetaDataStruct& stMetaData_);

    //----------------------------------------------------------------------------
    //! \brief Read a log from the FileParser.
    //
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the FileParser.
    //! \param[out] stMetaData_ A reference to a MetaDataStruct to be populated
    //! by the FileParser.
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   STREAM_EMPTY: There are no more bytes to parse in the file provided.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS ReadIntermediate(MessageDataStruct& stMessageData_, IntermediateHeader& header_, std::vector<FieldContainer>& stMessage_,
                                          MetaDataStruct& stMetaData_);

    //----------------------------------------------------------------------------
    //! \brief Reset the InputFileStream, and flush all bytes from the internal
    //! FileParser.
    //
    //! \return A boolean describing if the operation was successful.
    //----------------------------------------------------------------------------
    bool Reset();

    //----------------------------------------------------------------------------
    //! \brief Flush all bytes from the internal Parser.
    //
    //! \param[in] pucBuffer_ A buffer to contain flushed bytes, if desired.
    //! Defaults to nullptr.
    //! \param[in] uiBufferSize_ The length of ulBufferSize_, if provided.
    //
    //! \return The number of bytes flushed from the internal Parser.
    //----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_ = nullptr, uint32_t uiBufferSize_ = Parser::uiParserInternalBufferSize)
    {
        return clMyParser.Flush(pucBuffer_, uiBufferSize_);
    }

    //----------------------------------------------------------------------------
    //! \brief Get a pointer to the current framed log raw data.
    //
    //! \return A pointer to the internal Parser's internal encode buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] unsigned char* GetInternalBuffer() const { return clMyParser.GetInternalBuffer(); }
};

} // namespace novatel::edie::oem

#endif // NOVATEL_FILE_PARSER_HPP
