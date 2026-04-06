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
// ! \file framer.hpp
// ===============================================================================

#pragma once

#include "novatel_edie/common/fixed_buffer.hpp"
#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"

namespace novatel::edie {

//============================================================================
//! \class FramerBase
//! \brief Base class for all framers. Contains necessary buffers and member
//! variables, defining generic framer operations.
//============================================================================
class FramerBase
{
  protected:
    //----------------------------------------------------------------------------
    //! \struct FindFrameEndResult
    //! \brief A struct to hold the result of the FindFrameEnd method
    //! \see FramerBase::FindFrameEnd
    //----------------------------------------------------------------------------
    struct FindFrameEndResult
    {
        enum class Status
        {
            COMPLETE,   // A complete message frame was found
            RESPONSE,   // A complete response frame was found
            INCOMPLETE, // A potential frame was found, but the buffer ended before the end of the frame
            INVALID     // The buffer size exceeded the max message length before finding a complete frame
        };
        Status eStatus{Status::INVALID};
        HEADER_FORMAT eFormat{HEADER_FORMAT::UNKNOWN};

        // If eStatus is COMPLETE, RESPONSE, or INVALID, then uiIndex stores the index following the last byte
        // of the message, response, or unknown data, respectively. If eStatus is INCOMPLETE, then uiIndex stores
        // the index from which the search should be resumed when more data is available.
        size_t uiIndex{0};
    };

    // The following constant defines a number of bytes that the framer should search through when looking for
    // sync bytes. Custom framers may define their own max lookahead length, though empirical testing has
    // shown the following value gives good performance when using the framer manager.
    constexpr static size_t MAX_LOOKAHEAD_BYTES = 256;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    std::shared_ptr<UCharFixedBuffer> pclMyBuffer{std::make_shared<UCharFixedBuffer>()};

    uint32_t uiMyCalculatedCrc32{0U};
    uint32_t uiMyByteCount{0U};
    uint32_t uiMyExpectedPayloadLength{0U};
    uint32_t uiMyExpectedMessageLength{0U};

    bool bMyReportUnknownBytes{true};
    bool bMyPayloadOnly{false};
    bool bMyFrameJson{false};

    //----------------------------------------------------------------------------
    //! \brief Check if the provided position is a CRLF (carriage return and line feed).
    //!
    //! \param[in] uiPosition_ The position to check in the shared circular buffer.
    //
    //! \return true if the position is a CRLF, false otherwise.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool IsCrlf(const uint32_t uiPosition_) const
    {
        const auto& clFrameBuffer = *pclMyBuffer;
        return uiPosition_ + 1 < clFrameBuffer.size() && clFrameBuffer[uiPosition_] == '\r' && clFrameBuffer[uiPosition_ + 1] == '\n';
    }

    // The following three methods are the main extension points for custom framers.

    //----------------------------------------------------------------------------
    //! \brief Find the index of the first sync byte(s) in the internal buffer.
    //!
    //! \return The index of the first sync byte(s) if found, or UCharFixedBuffer::npos if not found.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual size_t FindSync() const { return UCharFixedBuffer::npos; };

    //----------------------------------------------------------------------------
    //! \brief Find the end of the candidate frame starting from the provided index.
    //!
    //! \param[in] start The index from which to start searching for the end of the frame.
    //! \return A FindFrameEndResult struct containing the status, format, and index of the end of the frame.
    //!
    //! \see FindFrameEndResult for details on the return struct.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual FindFrameEndResult FindFrameEnd([[maybe_unused]] size_t start) const { return FindFrameEndResult{}; };

    //----------------------------------------------------------------------------
    //! \brief Validate the candidate frame ending at the byte preceding the provided index.
    //!
    //! \param[in] frameEnd The index following the last byte of the candidate frame to validate.
    //! \return If the frame is valid, return 0. If the frame is invalid, return the number of
    //!     bytes to discard before the next potential frame.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual size_t Validate([[maybe_unused]] size_t frameEnd) const { return 0; };

    //----------------------------------------------------------------------------
    //! \brief Get a frame from the internal circular buffer if one exists
    //
    //! \param [out] pucFrameBuffer_ The buffer to which the Framer should copy the
    //! framed message.
    //! \param [in] uiFrameBufferSize_ The length of pucFrameBuffer_.
    //! \param [out] stMetaData_ A MetaDataBase to contain some information
    //! about the message frame.
    //! \param [out] bMetadataOnly_ Only populate metadata and do not copy the message.
    //
    //! \return An error code describing the result of framing.
    //!   SUCCESS: A message frame was found.
    //!   NULL_PROVIDED: pucFrameBuffer_ is a null pointer.
    //!   UNKNOWN: The bytes returned are unknown.
    //!   INCOMPLETE: The framer found what could be a message, but there
    //! are no more bytes in the internal buffer.
    //!   BUFFER_EMPTY: There are no more bytes in the internal buffer.
    //!   BUFFER_FULL: The frame found is larger than uiFrameBufferSize_ bytes.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_,
                                          bool bMetadataOnly_ = false);

  public:
    //----------------------------------------------------------------------------
    //! \brief Reset the state of the Framer.
    //----------------------------------------------------------------------------
    virtual void ResetState() = 0;

    //----------------------------------------------------------------------------
    //! \brief Initialize the attributes of the Framer.
    //----------------------------------------------------------------------------
    virtual void InitAttributes()
    {
        uiMyExpectedMessageLength = 0;
        uiMyExpectedPayloadLength = 0;
        uiMyByteCount = 0;
        uiMyCalculatedCrc32 = 0;
    };

    void HandleUnknownBytes(unsigned char* pucBuffer_, size_t count_)
    {
        count_ = std::min(count_, pclMyBuffer->size());

        if (count_ == 0) { return; }

        if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyBuffer->copy_out(pucBuffer_, count_); }

        pclMyBuffer->erase_begin(count_);
        InitAttributes();
        ResetState();
    }

    uint32_t GetMyByteCount() { return uiMyByteCount; };

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //----------------------------------------------------------------------------
    FramerBase(const std::string& strLoggerName_) : pclMyLogger(GetBaseLoggerManager()->RegisterLogger(strLoggerName_))
    {
        pclMyLogger->debug("FramerBase initializing...");
        if (pclMyBuffer == nullptr) { pclMyBuffer = std::make_shared<UCharFixedBuffer>(); }
        pclMyLogger->debug("FramerBase initialized");
    }

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //! \param[in] buffer pointer to an already created sharable fixed buffer.
    //----------------------------------------------------------------------------
    FramerBase(const std::string& strLoggerName_, const std::shared_ptr<UCharFixedBuffer> buffer_)
        : pclMyLogger(GetBaseLoggerManager()->RegisterLogger(strLoggerName_)), pclMyBuffer(buffer_)
    {
        pclMyLogger->debug("FramerBase initialized");
    }

    //----------------------------------------------------------------------------
    //! \brief A destructor for the FramerBase class.
    //----------------------------------------------------------------------------
    virtual ~FramerBase() = default;

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return Shared pointer to the internal logger object.
    //----------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \param[in] eLevel_ Shared pointer to the internal logger object.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(const spdlog::level::level_enum eLevel_) const { pclMyLogger->set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Should the Framer look for JSON objects in provided bytes?
    //! This setting can be extremely error-prone, as there is no CRC search or
    //! sync bytes, just direct comparisons to '{' and '}' characters.
    //
    //! \param[in] bFrameJson_ true if the Framer should search for JSON objects.
    //----------------------------------------------------------------------------
    void SetFrameJson(const bool bFrameJson_) { bMyFrameJson = bFrameJson_; }

    //----------------------------------------------------------------------------
    //! \brief Should the Framer return only the message payload of messages and
    //! discard the header?
    //
    //! \param[in] bPayloadOnly_ true if the Framer should discard message
    //! headers.
    //----------------------------------------------------------------------------
    void SetPayloadOnly(const bool bPayloadOnly_) { bMyPayloadOnly = bPayloadOnly_; }

    //----------------------------------------------------------------------------
    //! \brief Configure the framer to return unknown bytes in the provided
    //! buffer.
    //
    //! \param[in] bReportUnknownBytes_ Set to true to return unknown bytes.
    //----------------------------------------------------------------------------
    void SetReportUnknownBytes(const bool bReportUnknownBytes_) { bMyReportUnknownBytes = bReportUnknownBytes_; }

    //----------------------------------------------------------------------------
    //! \brief Write new bytes to the internal circular buffer.
    //
    //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
    //! written into the framer buffer.
    //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
    //
    //! \return The number of bytes written to the internal circular buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] size_t Write(const unsigned char* pucDataBuffer_, size_t uiDataBytes_) { return pclMyBuffer->write(pucDataBuffer_, uiDataBytes_); }

    //----------------------------------------------------------------------------
    //! \brief Flush bytes from the internal circular buffer.
    //
    //! \param[in] pucBuffer_ The buffer to contain the flushed bytes.
    //! \param[in] uiBufferSize_ The size of the provided buffer.
    //
    //! \return The number of bytes flushed from the internal circular buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] uint32_t Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
    {
        const uint32_t uiBytesToFlush = std::min(static_cast<uint32_t>(pclMyBuffer->size()), uiBufferSize_);
        HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
        return uiBytesToFlush;
    }

    //----------------------------------------------------------------------------
    //! \brief Get the number of bytes currently available in the internal buffer.
    //!
    //! \return The number of bytes available in the internal circular buffer.
    //------------------------------------------------------------------------------
    [[nodiscard]] size_t GetAvailableSpace() const { return pclMyBuffer->available_space(); }

    //----------------------------------------------------------------------------
    //! \brief Get the maximum number of bytes the framer will search through when looking for sync bytes.
    //!
    //! \return The maximum number of bytes the framer will search through when looking for sync bytes.
    //----------------------------------------------------------------------------
    [[nodiscard]] static constexpr size_t GetMaxLookaheadBytes() noexcept { return MAX_LOOKAHEAD_BYTES; }

    //----------------------------------------------------------------------------
    //! \brief Allow FramerManager to access protected GetFrame method.
    //----------------------------------------------------------------------------
    friend class FramerManager;
};
} // namespace novatel::edie
