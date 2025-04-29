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

#include "novatel_edie/common/logger.hpp"

template <typename T, size_t N> class FixedRingBuffer
{
    static_assert(N > 0, "Buffer size must be greater than zero");

    std::array<T, N> buffer_;
    size_t head_ = 0;
    size_t size_ = 0;

  public:
    void push_back(const T& value) { buffer_[(head_ + size_++) % N] = value; }

    const T& operator[](size_t i) const { return buffer_[(head_ + i) % N]; }

    void erase_begin(size_t count)
    {
        const size_t to_erase = std::min(count, size_);
        head_ = (head_ + to_erase) % N;
        size_ -= to_erase;
    }

    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] size_t capacity() const { return N; }
    [[nodiscard]] size_t reserve() const { return N - size_; }
    [[nodiscard]] bool empty() const { return size_ == 0; }
};

//============================================================================
//! \class FramerBase
//! \brief Base class for all framers. Contains necessary buffers and member
//! variables, defining generic framer operations.
//============================================================================
class FramerBase
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger;
    FixedRingBuffer<unsigned char, 1 << 18> clMyBuffer;

    uint32_t uiMyCalculatedCrc32{0U};
    uint32_t uiMyByteCount{0U};
    uint32_t uiMyExpectedPayloadLength{0U};
    uint32_t uiMyExpectedMessageLength{0U};

    bool bMyReportUnknownBytes{true};
    bool bMyPayloadOnly{false};
    // Ensure we don't exceed the number of available bytes
    bool bMyFrameJson{false};

    virtual void ResetState() = 0;

    [[nodiscard]] bool IsCrlf(const uint32_t uiPosition_) const
    {
        return uiPosition_ + 1 < clMyBuffer.size() && clMyBuffer[uiPosition_] == '\r' && clMyBuffer[uiPosition_ + 1] == '\n';
    }

    void CopyFromBuffer(unsigned char* destination, size_t count)
    {
        if (count == 0) { return; }

        // Ensure we don't exceed the number of available bytes
        const size_t available = clMyBuffer.size();
        size_t to_copy = std::min(count, available);

        size_t bytes_copied = 0;

        // First chunk (from head to the end of the buffer)
        while (bytes_copied < to_copy && bytes_copied < clMyBuffer.size())
        {
            destination[bytes_copied] = clMyBuffer[bytes_copied];
            ++bytes_copied;
        }
    }

    void HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t uiUnknownBytes_)
    {
        if (uiUnknownBytes_ == 0) { return; }

        const size_t bytes_to_handle = std::min(static_cast<size_t>(uiUnknownBytes_), clMyBuffer.size());

        if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { CopyFromBuffer(pucBuffer_, bytes_to_handle); }

        clMyBuffer.erase_begin(bytes_to_handle);

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
    //! \brief Write new bytes to the internal circular buffer.
    //
    //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
    //! written into the framer buffer.
    //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
    //
    //! \return The number of bytes written to the internal circular buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool Write(const unsigned char* pucDataBuffer_, size_t uiDataBytes_)
    {
        if (pucDataBuffer_ == nullptr || uiDataBytes_ == 0) { return true; }

        if (uiDataBytes_ > clMyBuffer.reserve()) { return false; }

        for (size_t i = 0; i < uiDataBytes_; ++i) { clMyBuffer.push_back(pucDataBuffer_[i]); }

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
        const uint32_t uiBytesToFlush = std::min(static_cast<uint32_t>(clMyBuffer.size()), uiBufferSize_);
        HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
        return uiBytesToFlush;
    }
};

#endif // FRAMER_HPP
