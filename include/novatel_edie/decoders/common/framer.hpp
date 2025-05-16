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

#include "novatel_edie/common/circular_buffer.hpp"
#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"

using namespace novatel::edie;

namespace novatel::edie {

//============================================================================
//! \class FramerBase
//! \brief Base class for all framers. Contains necessary buffers and member
//! variables, defining generic framer operations.
//============================================================================
class FramerBase
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger;
    std::shared_ptr<CircularBuffer> pclMyCircularDataBuffer;

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
        return uiPosition_ + 1 < pclMyCircularDataBuffer->GetLength() && (*pclMyCircularDataBuffer)[uiPosition_] == '\r' &&
               (*pclMyCircularDataBuffer)[uiPosition_ + 1] == '\n';
    }

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
    };

  protected:
    void HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t uiUnknownBytes_)
    {
        if (bMyReportUnknownBytes && pucBuffer_ != nullptr) { pclMyCircularDataBuffer->Copy(pucBuffer_, uiUnknownBytes_); }
        pclMyCircularDataBuffer->Discard(uiUnknownBytes_);

        uiMyByteCount = 0;
        uiMyExpectedMessageLength = 0;
        uiMyExpectedPayloadLength = 0;

        ResetState();
    }

  public:
    uint32_t GetMyByteCount() { return uiMyByteCount; };

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //----------------------------------------------------------------------------
    FramerBase(const std::string& strLoggerName_) : pclMyLogger(pclLoggerManager->RegisterLogger(strLoggerName_))
    {
        if (pclMyCircularDataBuffer == nullptr) { pclMyCircularDataBuffer = std::make_shared<CircularBuffer>(); }
        pclMyCircularDataBuffer->Clear();
        pclMyLogger->debug("Framer initialized");
    }

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //----------------------------------------------------------------------------
    FramerBase(const std::string& strLoggerName_, const std::shared_ptr<CircularBuffer> circularBuffer_)
        : pclMyLogger(Logger::RegisterLogger(strLoggerName_)), pclMyCircularDataBuffer(circularBuffer_)
    {
        pclMyLogger->debug("FramerBase initializing...");
        pclMyCircularDataBuffer->Clear();
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
    [[nodiscard]] uint32_t GetBytesAvailableInBuffer() const { return pclMyCircularDataBuffer->GetCapacity() - pclMyCircularDataBuffer->GetLength(); }

    //----------------------------------------------------------------------------
    //! \brief Write new bytes to the internal circular buffer.
    //
    //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
    //! written into the framer buffer.
    //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
    //
    //! \return The number of bytes written to the internal circular buffer.
    //----------------------------------------------------------------------------
    uint32_t Write(const unsigned char* pucDataBuffer_, uint32_t uiDataBytes_)
    {
        return pclMyCircularDataBuffer->Append(pucDataBuffer_, uiDataBytes_);
    }

    //----------------------------------------------------------------------------
    //! \brief Flush bytes from the internal circular buffer.
    //
    //! \param[in] pucBuffer_ The buffer to contain the flushed bytes.
    //! \param[in] uiBufferSize_ The size of the provided buffer.
    //
    //! \return The number of bytes flushed from the internal circular buffer.
    //----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
    {
        const uint32_t uiBytesToFlush = std::min(pclMyCircularDataBuffer->GetLength(), uiBufferSize_);
        HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
        return uiBytesToFlush;
    }

    ////----------------------------------------------------------------------------
    ////! \brief Flush bytes from the internal circular buffer. IMPORTANT: THIS DOES NOT HANDLE UNKNOWN BYTES
    ////
    ////! \param[in] uiBufferSize_ The size of the provided buffer.
    ////
    ////! \return The number of bytes flushed from the internal circular buffer.
    ////----------------------------------------------------------------------------
    virtual uint32_t Flush(uint32_t uiBufferSize_)
    {
        const uint32_t uiBytesToFlush = std::min(pclMyCircularDataBuffer->GetLength(), uiBufferSize_);
        return uiBytesToFlush;
    }

    //----------------------------------------------------------------------------
    //! \brief Find the next sync byte in the circular buffer.
    //! \param[in] pucFrameBuffer_ The buffer to search for the next sync byte.
    //! \param[in] uiFrameBufferSize_ The length of pucFrameBuffer_.
    //! \return The offset of the next sync byte. | -1 if no sync byte is found within the buffer.
    //---------------------------------------------------------------------------
    virtual uint32_t FindSyncOffset(uint32_t uiFrameBufferSize_, STATUS& offsetStatus) = 0;

    //----------------------------------------------------------------------------
    //! \brief virtual function to be overridden with casting MetaDataBase to type-specific MetaDataStruct
    //
    //! \param [out] pucFrameBuffer_ The buffer which the Framer should copy the
    //! framed message to.
    //! \param [in] uiFrameBufferSize_ The length of pcFrameBuffer_.
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
    //!   BUFFER_FULL: pucFrameBuffer_ has no more room for added bytes, according
    //! to the size specified by uiFrameBufferSize_.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_,
                                          bool bMetadataOnly_ = false) = 0;
};
} // namespace novatel::edie

#endif // FRAMER_HPP
