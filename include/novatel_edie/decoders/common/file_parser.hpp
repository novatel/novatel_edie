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

#pragma once

#include <filesystem>
#include <iosfwd>
#include <memory>

#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

namespace novatel::edie {

//============================================================================
//! \class FileParserBase
//! \brief Frame, decode and re-encode logs from an InputFileStream.
//!
//! \tparam ParserT        Parser type.
//! \tparam MetaDataT      Metadata struct type, derived from MetaDataBase.
//! \tparam IntermediateHeaderT Intermediate header type produced by the header decoder.
//============================================================================
template <typename ParserT, typename FilterT, typename MetaDataT, typename IntermediateHeaderT> class FileParserBase
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger;
    ParserT clMyParser;
    std::shared_ptr<std::istream> pclMyInputStream{nullptr};

    [[nodiscard]] bool ReadStream()
    {
        std::array<char, MAX_ASCII_MESSAGE_LENGTH> cData{};
        pclMyInputStream->read(cData.data(), cData.size());
        size_t ullBytesRead = pclMyInputStream->gcount();
        const auto uiBytesRead = static_cast<uint32_t>(ullBytesRead);
        return ullBytesRead > 0 && clMyParser.Write(reinterpret_cast<unsigned char*>(cData.data()), uiBytesRead) == uiBytesRead;
    }

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
    template <typename ParserFunc, typename... Args> [[nodiscard]] STATUS HandleRead(ParserFunc parserFunc_, Args&&... args_)
    {
        while (true)
        {
            const STATUS eStatus = parserFunc_(std::forward<Args>(args_)...);
            switch (eStatus)
            {
            case STATUS::SUCCESS: return STATUS::SUCCESS;
            case STATUS::UNKNOWN: return STATUS::UNKNOWN;
            case STATUS::NO_DEFINITION: return STATUS::NO_DEFINITION;
            case STATUS::INCOMPLETE: [[fallthrough]];
            case STATUS::BUFFER_EMPTY: {
                if (ReadStream()) { continue; }
                return parserFunc_(std::forward<Args>(args_)..., true) == STATUS::SUCCESS ? STATUS::SUCCESS : STATUS::STREAM_EMPTY;
            }
            default: pclMyLogger->info("Encountered an error: {}\n", eStatus); return eStatus;
            }
        }
    }

    //----------------------------------------------------------------------------
    //! \brief Protected constructor for the FileParserBase class.
    //! Only derived classes may construct a FileParserBase.
    //
    //! \param[in] sLoggerName_ Name used to register the internal logger.
    //----------------------------------------------------------------------------
    explicit FileParserBase(std::string sLoggerName_) : pclMyLogger(GetBaseLoggerManager()->RegisterLogger(std::move(sLoggerName_))) {}

    explicit FileParserBase(std::string sLoggerName_, const std::filesystem::path& sDbPath_)
        : pclMyLogger(GetBaseLoggerManager()->RegisterLogger(std::move(sLoggerName_))), clMyParser(sDbPath_)
    {
    }

    explicit FileParserBase(std::string sLoggerName_, MessageDatabase::Ptr pclMessageDb_)
        : pclMyLogger(GetBaseLoggerManager()->RegisterLogger(std::move(sLoggerName_))), clMyParser(pclMessageDb_)
    {
    }

  public:
    //! NOTE: The following constructors prevent this class from ever being
    //! constructed from a copy, move or assignment.
    FileParserBase(const FileParserBase&) = delete;
    FileParserBase(FileParserBase&&) = delete;
    FileParserBase& operator=(const FileParserBase&) = delete;
    FileParserBase& operator=(FileParserBase&&) = delete;

    virtual ~FileParserBase() = default;

    //----------------------------------------------------------------------------
    //! \brief Load a MessageDatabase object.
    //
    //! \param[in] pclMessageDb_ A pointer to a MessageDatabase object.
    //----------------------------------------------------------------------------
    void LoadJsonDb(const MessageDatabase::Ptr& pclMessageDb_)
    {
        if (pclMessageDb_ != nullptr) { clMyParser.LoadJsonDb(pclMessageDb_); }
        else { pclMyLogger->debug("JSON DB is a NULL pointer."); }
    }

    // ---------------------------------------------------------------------------
    //! \brief Get the MessageDatabase object.
    //
    //! \return A shared pointer to the MessageDatabase object.
    // ---------------------------------------------------------------------------
    MessageDatabase::ConstPtr MessageDb() const { return this->clMyParser.MessageDb(); }

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
    //! \brief Set the FilterT for the FileParserBase.
    //
    //! \param[in] pclFilter_ A pointer to an OEM message FilterT object.
    //----------------------------------------------------------------------------
    void SetFilter(const std::shared_ptr<FilterT>& pclFilter_) { clMyParser.SetFilter(pclFilter_); }

    //----------------------------------------------------------------------------
    //! \brief Get the config for the FileParserBase.
    //
    //! \return A pointer to the FileParser's OEM message FilterT object.
    //----------------------------------------------------------------------------
    [[nodiscard]] const std::shared_ptr<FilterT>& GetFilter() const { return clMyParser.GetFilter(); }

    //----------------------------------------------------------------------------
    //! \brief Set the InputFileStream for the FileParserBase.
    //
    //! \param[in] pclInputStream_ A pointer to the input stream.
    //
    //! \return A boolean describing if the operation was successful
    //----------------------------------------------------------------------------
    [[nodiscard]] bool SetStream(std::shared_ptr<std::istream> pclInputStream_)
    {
        if (pclInputStream_ == nullptr || pclInputStream_->eof()) { return false; }
        pclMyInputStream = pclInputStream_;
        Reset();
        return true;
    }

    //----------------------------------------------------------------------------
    //! \brief Read a log from the FileParser.
    //
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the FileParser.
    //! \param[out] stMetaData_ A reference to a MetaDataT to be populated
    //! by the FileParser.
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   STREAM_EMPTY: There are no more bytes to parse in the file provided.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS Read(MessageDataStruct& stMessageData_, MetaDataT& stMetaData_)
    {
        return HandleRead([this](auto&&... params) { return clMyParser.Read(std::forward<decltype(params)>(params)...); }, stMessageData_,
                          stMetaData_);
    }

    //----------------------------------------------------------------------------
    //! \brief Read a log from the FileParser.
    //
    //! \param[out] stMessageData_ A reference to a MessageDataStruct to be
    //! populated by the FileParser.
    //! \param[out] header_ A reference to a IntermediateHeaderT to be
    //! populated by the FileParser.
    //! \param[out] stMessage_ A reference to a vector of FieldContainers to be
    //! populated by the FileParser.
    //! \param[out] stMetaData_ A reference to a MetaDataT to be populated
    //! by the FileParser.
    //
    //! \return An error code describing the result of parsing.
    //!   SUCCESS: A message was framed, decoded, filtered, encoded and stored
    //! internally, pointed to by stMessageData_.
    //!   UNKNOWN: A message could not be found and unknown bytes were returned
    //! if requested in the ParserConfigStruct given to SetConfig().
    //!   STREAM_EMPTY: There are no more bytes to parse in the file provided.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS ReadIntermediate(MessageDataStruct& stMessageData_, IntermediateHeaderT& header_, std::vector<FieldContainer>& stMessage_,
                                          MetaDataT& stMetaData_)
    {
        return HandleRead([this](auto&&... params) { return clMyParser.ReadIntermediate(std::forward<decltype(params)>(params)...); }, stMessageData_,
                          header_, stMessage_, stMetaData_);
    }

    //----------------------------------------------------------------------------
    //! \brief Reset the InputFileStream, and flush all bytes from the internal
    //! FileParser.
    //
    //! \return A boolean describing if the operation was successful.
    //----------------------------------------------------------------------------
    bool Reset()
    {
        Flush();
        if (pclMyInputStream != nullptr)
        {
            pclMyInputStream->clear();
            pclMyInputStream->seekg(0, std::ios::beg);
        }
        return true;
    }

    //----------------------------------------------------------------------------
    //! \brief Flush all bytes from the internal Parser.
    //
    //! \param[in] pucBuffer_ A buffer to contain flushed bytes, if desired.
    //! Defaults to nullptr.
    //! \param[in] uiBufferSize_ The length of ulBufferSize_, if provided.
    //
    //! \return The number of bytes flushed from the internal Parser.
    //----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_ = nullptr, uint32_t uiBufferSize_ = ParserT::uiParserInternalBufferSize)
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

} // namespace novatel::edie
