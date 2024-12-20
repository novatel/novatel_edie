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

#include <algorithm>
#include <deque>
#include <memory>
#include <vector>


#include "novatel_edie/common/circular_buffer.hpp"
#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/common/logger.hpp"

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/framer.hpp"



// Type-Specific Framers
// #include "src/decoders/automotive/api/common.hpp"
// #include "src/decoders/automotive/api/framer.hpp"
//
// #include "src/decoders/nmea/api/common.hpp"
// #include "src/decoders/nmea/api/framer.hpp"
//
// #include "novatel-edie/src/decoders/novatel/api/common.hpp"
// #include "novatel-edie/src/decoders/novatel/api/framer.hpp"
//
// #include "src/decoders/pimtp/api/common.hpp"
// #include "src/decoders/pimtp/api/framer.hpp"
//
// #include "src/decoders/waas/api/common.hpp"
// #include "src/decoders/waas/api/framer.hpp"

//----------------------------------------------------------------------------
//! \brief A constructor for the FramerBase class.
//
//! \param[in] strLoggerName_ String to name the internal logger.
//----------------------------------------------------------------------------
#include <iostream>

namespace novatel::edie {

enum class FRAMER_ID
{
    AUTOMOTIVE,
    NOVATEL,
    NMEA,
    UNKNOWN
};

struct FramerElement
{
    FRAMER_ID framerId;
    std::unique_ptr<FramerBase> framer;
    std::unique_ptr<MetaDataBase> metadata;
    int32_t offset;

    FramerElement(FRAMER_ID framerId_, std::unique_ptr<FramerBase> framer_, std::unique_ptr<MetaDataBase> metadata_, int32_t offset_)
        : framerId(framerId_), framer(std::move(framer_)), metadata(std::move(metadata_)), offset(offset_)
    {
    }
};

// Forward Declarations of Framers
// class novatel::edie::oem::Framer;

class FramerManager
{
  private:
    std::list<FRAMER_ID> userFramers;
    FramerManager();
    FramerManager(const FramerManager&) = delete;
    FramerManager& operator=(const FramerManager&) = delete;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    std::shared_ptr<CircularBuffer> pclMyCircularDataBuffer;

    bool bMyReportUnknownBytes{true};

    void HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t& uiUnknownBytes_);

    void ResetInactiveFramerStates(const FRAMER_ID& activeFramer_);

    void ResetInactiveMetaDataStates(const FRAMER_ID& activeFramer_);

  protected:
    // TODO delete this
    void DisplayFramerStack();

  public:
    std::deque<FramerElement> framerRegistry;
    void RegisterFramer(const FRAMER_ID framerId_, std::unique_ptr<FramerBase>, std::unique_ptr<MetaDataBase>);
    //---------------------------------------------------------------------------
    //! \brief Get the MetaData for a specific framer.
    //! \param[in] framerId_ The ID of the framer to get the MetaData for.
    //! \return A pointer to the MetaData for the specified framer.
    //! \return nullptr if the framer is not found.
    //---------------------------------------------------------------------------
    MetaDataBase* GetMetaData(const FRAMER_ID framerId_);

    //----------------------------------------------------------------------------
    //! \brief Reset the state of all framers in the framer registry.
    //---------------------------------------------------------------------------
    void ResetAllFramerStates();

    //----------------------------------------------------------------------------
    //! \brief Reset the state of all metadata in the framer registry.
    //---------------------------------------------------------------------------
    void ResetAllMetaDataStates();

    //----------------------------------------------------------------------------
    //! \brief Configure the framer manager to return unknown bytes in the provided
    //! buffer.
    //
    //! \param[in] bReportUnknownBytes_ Set to true to return unknown bytes.
    //----------------------------------------------------------------------------
    void SetReportUnknownBytes(const bool bReportUnknownBytes_) { bMyReportUnknownBytes = bReportUnknownBytes_; }

    //----------------------------------------------------------------------------
    //! \brief Get the FramerManager instance.
    //! \return The FramerManager instance.
    //----------------------------------------------------------------------------
    static FramerManager& GetInstance()
    {
        static FramerManager instance; // Singleton
        return instance;
    }

    std::shared_ptr<CircularBuffer> GetCircularBuffer() const;

    void AddFramer(const FRAMER_ID framerId_);

    // nmea::MetaDataStruct nmeaMetaDataStruct;
    ////----------------------------------------------------------------------------
    ////! \brief A constructor for the FramerBase class.
    ////
    ////! \param[in] strLoggerName_ String to name the internal logger.
    ////----------------------------------------------------------------------------
    // FramerManager(const std::string& strLoggerName_);

    //----------------------------------------------------------------------------
    //! \brief A destructor for the FramerBase class.
    //----------------------------------------------------------------------------
    ~FramerManager() = default;

    FramerElement* GetFramerElement(const FRAMER_ID framerId_);

    //----------------------------------------------------------------------------
    //! \brief Write new bytes to the internal circular buffer.
    //
    //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
    //! written into the framer buffer.
    //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
    //
    //! \return The number of bytes written to the internal circular buffer.
    //----------------------------------------------------------------------------
    uint32_t Write(const unsigned char* pucDataBuffer_, uint32_t uiDataBytes_);

    //----------------------------------------------------------------------------
    //! \brief Reorder the framers in the framer registry.
    //----------------------------------------------------------------------------
    void ReorderFramers();

    ////----------------------------------------------------------------------------
    ////! \brief Flush bytes from the internal circular buffer.
    ////
    ////! \param[in] pucBuffer_ The buffer to contain the flushed bytes.
    ////! \param[in] uiBufferSize_ The size of the provided buffer.
    ////
    ////! \return The number of bytes flushed from the internal circular buffer.
    ////----------------------------------------------------------------------------
    uint32_t Flush(unsigned char* pucBuffer_, uint32_t uiBufferSize_)
    {
        const uint32_t uiBytesToFlush = std::min(pclMyCircularDataBuffer->GetLength(), uiBufferSize_);
        HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
        return uiBytesToFlush;
    }

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return Shared pointer to the internal logger object.
    //----------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<spdlog::logger> GetLogger() const;

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \param[in] eLevel_ Shared pointer to the internal logger object.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(const spdlog::level::level_enum eLevel_) const;

    //----------------------------------------------------------------------------
    //! \brief Shutdown the internal logger.
    //----------------------------------------------------------------------------
    void ShutdownLogger();

    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, FRAMER_ID& eActiveFramerId_);
};
} // namespace novatel::edie

#endif // FRAMER_MANAGER_HPP
