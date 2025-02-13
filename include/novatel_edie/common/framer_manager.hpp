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
#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

#include "novatel_edie/common/circular_buffer.hpp"
#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/framer.hpp"

//----------------------------------------------------------------------------
//! \brief A constructor for the FramerBase class.
//
//! \param[in] strLoggerName_ String to name the internal logger.
//----------------------------------------------------------------------------

namespace novatel::edie {

//enum class FRAMER_ID
//{
//    AUTOMOTIVE,
//    NOVATEL,
//    NMEA,
//    UNKNOWN
//};

struct FramerElement
{
    int framerId;
    std::unique_ptr<FramerBase> framer;
    std::unique_ptr<MetaDataBase> metadata;
    int32_t offset;

    FramerElement(int framerId_, std::unique_ptr<FramerBase> framer_, std::unique_ptr<MetaDataBase> metadata_, int32_t offset_)
        : framerId(framerId_), framer(std::move(framer_)), metadata(std::move(metadata_)), offset(offset_)
    {
    }
};

// Forward Declarations of Framers
// class novatel::edie::oem::Framer;

class FramerManager
{
  private:
    FramerManager();

    //! NOTE: disable copies and moves.
    FramerManager(const FramerManager&) = delete;
    FramerManager(const FramerManager&&) = delete;
    FramerManager& operator=(const FramerManager&) = delete;
    FramerManager& operator=(const FramerManager&&) = delete;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    std::shared_ptr<CircularBuffer> pclMyCircularDataBuffer;
    

    bool bMyReportUnknownBytes{true};

    void HandleUnknownBytes(unsigned char* pucBuffer_, const uint32_t& uiUnknownBytes_) const;

    void ResetInactiveFramerStates(const int& activeFramer_);

    void ResetInactiveMetaDataStates(const int& activeFramer_);

  protected:
    // TODO delete this
    void DisplayFramerStack();

  public:
    std::unordered_map<std::string, int> idMap;
    std::deque<FramerElement> framerRegistry;
    void RegisterFramer(std::string framerName_, std::unique_ptr<FramerBase>, std::unique_ptr<MetaDataBase>);
    //---------------------------------------------------------------------------
    //! \brief Get the MetaData for a specific framer.
    //
    //! \param[in] framerId_ The ID of the framer to get the MetaData for.
    //! \return A pointer to the MetaData for the specified framer.
    //! \return nullptr if the framer is not found.
    //---------------------------------------------------------------------------
    MetaDataBase* GetMetaData(int framerId_);

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

    //----------------------------------------------------------------------------
    //! \brief Get the internal circular buffer.
    //
    //! \return Shared pointer to the internal circular buffer object.
    //---------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<CircularBuffer> GetCircularBuffer() const { return pclMyCircularDataBuffer; }

    //----------------------------------------------------------------------------
    //! \brief A destructor for the FramerBase class.
    //----------------------------------------------------------------------------
    ~FramerManager() = default;

    //----------------------------------------------------------------------------
    //! \brief Get the pointer to a FramerElement Structure for specified framer.
    //
    //! \param[in] framerId_ The ID of the framer to get the FramerElement for.
    //
    //! \return A pointer to the FramerElement for the specified framer.
    //---------------------------------------------------------------------------
    FramerElement* GetFramerElement(int framerId_);

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
    //! \brief Reorder the framers in the framer registry.
    //----------------------------------------------------------------------------
    void SortFramers();

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
    void ShutdownLogger() { ShutdownLogger(); }

    //----------------------------------------------------------------------------
    //! \brief Attempt to discover a frame from the internal circular buffer if one exists
    //
    //! \param[in] pucFrameBuffer_ The buffer to contain the discovered frame.
    //! \param[in] uiFrameBufferSize_ The size of the provided buffer.
    //! \param[out] eActiveFramerId_ The ID of the framer that discovered the frame.
    //
    //! \return The status of the frame discovery.
    //---------------------------------------------------------------------------
    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, int& eActiveFramerId_);
};
} // namespace novatel::edie

#endif // FRAMER_MANAGER_HPP
