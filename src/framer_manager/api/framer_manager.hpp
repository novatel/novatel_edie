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
// ! \file framer_manager.hpp
// ===============================================================================

#ifndef FRAMER_MANAGER_HPP
#define FRAMER_MANAGER_HPP

#include <memory>

#include "novatel-edie/src/decoders/common/api/circular_buffer.hpp"
#include "novatel-edie/src/decoders/common/api/crc32.hpp"
#include "novatel-edie/src/decoders/common/api/logger.hpp"

// Type-Specific Framers
// #include "src/decoders/automotive/api/common.hpp"
// #include "src/decoders/automotive/api/framer.hpp"
//
// #include "src/decoders/nmea/api/common.hpp"
// #include "src/decoders/nmea/api/framer.hpp"
//
#include "novatel-edie/src/decoders/novatel/api/common.hpp"
#include "novatel-edie/src/decoders/novatel/api/framer.hpp"
//
// #include "src/decoders/pimtp/api/common.hpp"
// #include "src/decoders/pimtp/api/framer.hpp"
//
// #include "src/decoders/waas/api/common.hpp"
// #include "src/decoders/waas/api/framer.hpp"

namespace novatel::edie {

//-----------------------------------------------------------------------
//! \enum FRAMER_MANAGER_FRAME_STATE
//! \brief Enumeration for state machine used while framing the log.
//-----------------------------------------------------------------------
enum class FRAMER_MANAGER_FRAME_STATE
{
    WAITING_FOR_SYNC,
    WAITING_FOR_HEADER,
    WAITING_FOR_BODY,
    COMPLETE_MESSAGE
};

class FramerManager
{
  public:
    // FRAMER_MANAGER_FRAME_STATE eMyFrameState{FRAMER_MANAGER_FRAME_STATE::WAITING_FOR_SYNC};

    std::shared_ptr<spdlog::logger> pclMyLogger;
    std::shared_ptr<CircularBuffer> pclMyCircularDataBuffer;

    // Framers and Dependencies
    // automotive::Framer automotiveFramer;
    // automotive::MetaDataStruct automotiveMetaDataStruct;

    // nmea::NmeaFramer nmeaFramer;
    // nmea::MetaDataStruct nmeaMetaDataStruct;

    std::unique_ptr<oem::Framer> novatelFramer;

    // pim::Framer pimtpFramer;
    // pim::MetaDataStruct pimtpMetaDataStruct;

    // waas::Framer giiiFramer;
    // waas::MetaDataStruct giiiMetaDataStruct;

    uint32_t uiMyCalculatedCrc32{0U};
    uint32_t uiMyByteCount{0U};
    uint32_t uiMyExpectedPayloadLength{0U};
    uint32_t uiMyExpectedMessageLength{0U};

    bool bMyReportUnknownBytes{true};
    bool bMyPayloadOnly{false};
    bool bMyFrameJson{false};

    [[nodiscard]] bool IsCrlf(const uint32_t uiPosition_) const
    {
        return uiPosition_ + 1 < pclMyCircularDataBuffer->GetLength() && (*pclMyCircularDataBuffer)[uiPosition_] == '\r' &&
               (*pclMyCircularDataBuffer)[uiPosition_ + 1] == '\n';
    }

    // TODO: reorder the functions below this
    // Repurposed Framer Functions
    void HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t uiUnknownBytes_);


  public:
    oem::MetaDataStruct novatelMetaDataStruct;
    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //----------------------------------------------------------------------------
    FramerManager(const std::string& strLoggerName_);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBase class that is called with a shared circular buffer.
    //
    //! \param[in] strLoggerName_ String to name the internal logger.
    //----------------------------------------------------------------------------
    FramerManager(const std::string& strLoggerName_, std::shared_ptr<CircularBuffer> pclMyCircularDataBuffer_);

    //----------------------------------------------------------------------------
    //! \brief A destructor for the FramerBase class.
    //----------------------------------------------------------------------------
    ~FramerManager() = default;

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

    virtual [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_);

};
} // namespace novatel::edie

#endif // FRAMER_MANAGER_HPP
