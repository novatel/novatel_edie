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

#ifndef FRAMER_HPP
#define FRAMER_HPP

#include <memory>

#include <boost/circular_buffer.hpp>

#include "novatel_edie/common/logger.hpp"

//============================================================================
//! \class FramerBase
//! \brief Base class for all framers. Contains necessary buffers and member
//! variables, defining generic framer operations.
//============================================================================
class FramerBase
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger;
    boost::circular_buffer<unsigned char> clMyCircularDataBuffer;

    uint32_t uiMyCalculatedCrc32{0U};
    uint32_t uiMyByteCount{0U};
    uint32_t uiMyExpectedPayloadLength{0U};
    uint32_t uiMyExpectedMessageLength{0U};

    bool bMyReportUnknownBytes{true};
    bool bMyPayloadOnly{false};
    bool bMyFrameJson{false};

    virtual void ResetState() = 0;

    [[nodiscard]] bool IsCrlf(const uint32_t uiPosition_) const
    {
        return uiPosition_ + 1 < clMyCircularDataBuffer.size() && clMyCircularDataBuffer[uiPosition_] == '\r' &&
               clMyCircularDataBuffer[uiPosition_ + 1] == '\n';
    }

    void CopyFromBuffer(unsigned char* destination, size_t count)
    {
        if (count == 0) { return; }

        auto ranges = clMyCircularDataBuffer.array_one();
        size_t first_chunk = std::min(count, ranges.second);
        memcpy(destination, ranges.first, first_chunk);

        if (count > first_chunk)
        {
            ranges = clMyCircularDataBuffer.array_two();
            size_t second_chunk = std::min(count - first_chunk, ranges.second);
            memcpy(destination + first_chunk, ranges.first, second_chunk);
        }
    }

    void HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t uiUnknownBytes_)
    {
        if (uiUnknownBytes_ == 0) { return; }

        const size_t bytes_to_handle = std::min(static_cast<size_t>(uiUnknownBytes_), clMyCircularDataBuffer.size());

        if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { CopyFromBuffer(pucBuffer_, bytes_to_handle); }

        clMyCircularDataBuffer.erase_begin(bytes_to_handle);

        uiMyByteCount = 0;
        uiMyExpectedMessageLength = 0;
        uiMyExpectedPayloadLength = 0;

        ResetState();
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //----------------------------------------------------------------------------
    FramerBase(const std::string& strLoggerName_) : pclMyLogger(pclLoggerManager->RegisterLogger(strLoggerName_))
    {
        clMyCircularDataBuffer.set_capacity(1 << 18);
        pclMyLogger->debug("Framer initialized");
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
    //! \brief Should the Framer return only the message body of messages and
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
    //! \brief Get the number of bytes available in the internal circular buffer.
    //
    //! \return The number of bytes available in the internal circular buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] uint32_t GetBytesAvailableInBuffer() const { return clMyCircularDataBuffer.reserve(); }

    //----------------------------------------------------------------------------
    //! \brief Write new bytes to the internal circular buffer.
    //
    //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
    //! written into the framer buffer.
    //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
    //
    //! \return The number of bytes written to the internal circular buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool Write(const unsigned char* pucDataBuffer_, uint32_t uiDataBytes_)
    {
        if (pucDataBuffer_ == nullptr || uiDataBytes_ == 0) { return true; }

        if (uiDataBytes_ > GetBytesAvailableInBuffer()) { return false; }

        clMyCircularDataBuffer.insert(clMyCircularDataBuffer.end(), pucDataBuffer_, pucDataBuffer_ + uiDataBytes_);
        return true;
    }

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
        const uint32_t uiBytesToFlush = std::min(static_cast<uint32_t>(clMyCircularDataBuffer.size()), uiBufferSize_);
        HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
        return uiBytesToFlush;
    }
};

#endif // FRAMER_HPP
