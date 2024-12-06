#ifndef FRAMER_HPP
#define FRAMER_HPP

#include "circular_buffer.hpp"
#include "decoders/common/api/common.hpp"
#include "decoders/common/api/framer.hpp"
#include "logger.hpp"

namespace novatel::edie {
// Base Class Template | declaration
class FramerBase
{
  protected:
    std::shared_ptr<spdlog::logger> pclMyLogger;
    std::shared_ptr<CircularBuffer> pclMyCircularDataBuffer;

    uint32_t uiMyCalculatedCrc32{0U};
    uint32_t uiMyByteCount{0U};
    uint32_t uiMyExpectedPayloadLength{0U};
    uint32_t uiMyExpectedMessageLength{0U};

    bool bMyPayloadOnly{false};
    bool bMyFrameJson{false};

    //============================================================================
    //! \class FramerBase
    //! \brief Base class for all framers. Contains necessary buffers and member
    //! variables, defining generic framer operations.
    //============================================================================
    [[nodiscard]] bool IsCrlf(const uint32_t uiPosition_) const
    {
        return uiPosition_ + 1 < pclMyCircularDataBuffer->GetLength() && (*pclMyCircularDataBuffer)[uiPosition_] == '\r' &&
               (*pclMyCircularDataBuffer)[uiPosition_ + 1] == '\n';
    }

  public:
    STATUS eMyCurrentFramerStatus{STATUS::UNKNOWN};
    
    // TODO delete this
    //uint32_t uiMySyncByteOffset{0U};

    virtual void ResetState() { eMyCurrentFramerStatus = STATUS::UNKNOWN; }
    virtual void ResetStateAndByteCount() = 0;

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //----------------------------------------------------------------------------
    FramerBase(const std::string& strLoggerName_, std::shared_ptr<CircularBuffer> circularBuffer_)
        : pclMyLogger(Logger::RegisterLogger(strLoggerName_)), pclMyCircularDataBuffer(circularBuffer_)
    {
        pclMyCircularDataBuffer->Clear();
        pclMyLogger->debug("Framer initialized");
    }

    FramerBase(const std::string& strLoggerName_) : pclMyLogger(Logger::RegisterLogger(strLoggerName_))
    {
        pclMyCircularDataBuffer->Clear();
        pclMyLogger->debug("Framer initialized without Circular Buffer");
    }

    virtual int32_t FindNextSyncByte(unsigned char* pucFrameBuffer_, const uint32_t uiFrameBufferSize_) = 0;
    //[[nodiscard]] virtual novatel::edie::STATUS GetFrame(unsigned char* pucFrameBuffer_, const uint32_t uiFrameBufferSize_, MetaDataBase&
    //stMetaData_,
    //                                                     uint32_t& uiFrameBufferOffset_);

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
    //! \brief Shutdown the internal logger.
    //----------------------------------------------------------------------------
    static void ShutdownLogger() { Logger::Shutdown(); }

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
    //! \brief Get the number of bytes available in the internal circular buffer.
    //
    //! \return The number of bytes available in the internal circular buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual uint32_t GetBytesAvailableInBuffer() const
    {
        return pclMyCircularDataBuffer->GetCapacity() - pclMyCircularDataBuffer->GetLength();
    }

    //----------------------------------------------------------------------------
    //! \brief Write new bytes to the internal circular buffer.
    //
    //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
    //! written into the framer buffer.
    //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
    //
    //! \return The number of bytes written to the internal circular buffer.
    //----------------------------------------------------------------------------
    virtual uint32_t Write(const unsigned char* pucDataBuffer_, uint32_t uiDataBytes_)
    {
        return pclMyCircularDataBuffer->Append(pucDataBuffer_, uiDataBytes_);
    }

    ////----------------------------------------------------------------------------
    ////! \brief Flush bytes from the internal circular buffer.
    ////
    ////! \param[in] pucBuffer_ The buffer to contain the flushed bytes.
    ////! \param[in] uiBufferSize_ The size of the provided buffer.
    ////
    ////! \return The number of bytes flushed from the internal circular buffer.
    ////----------------------------------------------------------------------------
    virtual uint32_t Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
    {
        const uint32_t uiBytesToFlush = std::min(pclMyCircularDataBuffer->GetLength(), uiBufferSize_);
        // HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
        return uiBytesToFlush;
    }

    [[nodiscard]] virtual STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_) = 0;

    //----------------------------------------------------------------------------
    //! \brief A destructor for the FramerBase class.
    //----------------------------------------------------------------------------
    virtual ~FramerBase() = default;
};
} // namespace novatel::edie
#endif // FRAMER_HPP
