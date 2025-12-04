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
#include <string>
#include <unordered_map>
#include <vector>

#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/common/fixed_ring_buffer.hpp"
#include "novatel_edie/common/logger.hpp"
#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/framer.hpp"

namespace novatel::edie {

struct FramerEntry
{
    std::string framerName;
    std::unique_ptr<FramerBase> framerInstance;
    std::unique_ptr<MetaDataBase> metadataInstance;
    int32_t offset;

    FramerEntry(std::string framerName_, std::unique_ptr<FramerBase> framerInstance_, std::unique_ptr<MetaDataBase> metadataInstance_)
        : framerName(framerName_), framerInstance(std::move(framerInstance_)), metadataInstance(std::move(metadataInstance_)), offset(0)
    {
    }
};

//================================================================================
//! \class FramerManager
//! \brief Provides an interface to operate multiple framers on the same data buffer.
//================================================================================
class FramerManager
{
  public:
    using FramerFactory = std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>, MetaDataBase&)>;

    //----------------------------------------------------------------------------
    //! \brief Construct a FramerManager with specified framers.
    //!
    //! Creates a FramerManager instance and initializes it with the specified
    //! framers from the registered framer factories.
    //!
    //! \param[in] selectedFramers A list of framer names to initialize. Each name
    //! must match a framer that has been registered via RegisterFramer().
    //!
    //! \sa RegisterFramer(), ListAvailableFramers()
    //----------------------------------------------------------------------------
    explicit FramerManager(const std::vector<std::string>& selectedFramers = {});

    //----------------------------------------------------------------------------
    //! \brief Add a framer type to the internal registry.
    //
    //! \param[in] framerName_ A name that uniquely identifies the framer type.
    //! \param[in] framerFactory_ A factory function that creates instances of the framer type.
    //! \param[in] metadataConstructor_ A factory function that creates instances of the
    //! metadata for the specified framer type.
    //----------------------------------------------------------------------------
    static void RegisterFramer(const std::string& framerName_,
                               std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)> framerFactory_,
                               std::function<std::unique_ptr<MetaDataBase>()> metadataConstructor_);

    //----------------------------------------------------------------------------
    //! \brief Get the MetaData for a specific framer.
    //
    //! \param[in] framerId_ The ID of the framer to get the MetaData for.
    //! \return A pointer to the MetaData for the specified framer.
    //! \return nullptr if the framer is not found.
    //----------------------------------------------------------------------------
    MetaDataBase* GetMetaData(std::string framerName_);

    //----------------------------------------------------------------------------
    //! \brief List all available framers that have been registered.
    //----------------------------------------------------------------------------
    static void ListAvailableFramers()
    {
        std::cout << "Available Framers:" << std::endl;
        for (const auto& pair : GetFramerFactories()) { std::cout << "- " << pair.first << std::endl; }
    }

    //----------------------------------------------------------------------------
    //! \brief Reset the state of all framers in the framer registry.
    //----------------------------------------------------------------------------
    void ResetAllFramerStates() const;

    //----------------------------------------------------------------------------
    //! \brief Configure the framer manager to return unknown bytes in the provided
    //! buffer.
    //
    //! \param[in] bReportUnknownBytes_ Set to true to return unknown bytes.
    //----------------------------------------------------------------------------
    void SetReportUnknownBytes(const bool bReportUnknownBytes_) { bMyReportUnknownBytes = bReportUnknownBytes_; }

    //----------------------------------------------------------------------------
    //! \brief Get the internal fixed ring buffer.
    //
    //! \return Shared pointer to the internal fixed ring buffer object.
    //---------------------------------------------------------------------------
    [[nodiscard]] std::shared_ptr<UCharFixedRingBuffer> GetFixedRingBuffer() const { return pclMyFixedRingBuffer; }

    //----------------------------------------------------------------------------
    //! \brief A destructor for the FramerManager class.
    //----------------------------------------------------------------------------
    ~FramerManager() = default;

    //----------------------------------------------------------------------------
    //! \brief Get the pointer to a FramerElement Structure for specified framer.
    //
    //! \param[in] framerId_ The ID of the framer to get the FramerElement for.
    //
    //! \return A pointer to the FramerElement for the specified framer.
    //----------------------------------------------------------------------------
    FramerEntry* GetFramerElement(const std::string framerName_);

    //----------------------------------------------------------------------------
    //! \brief Get the name of the active framer.
    //
    //! \return The name of the currently active FramerElement.
    //----------------------------------------------------------------------------
    std::string GetActiveFramerName() const { return framerRegistry.front().framerName; }

    //----------------------------------------------------------------------------
    //! \brief Write new bytes to the internal circular buffer.
    //
    //! \param[in] pucDataBuffer_ The data buffer containing the bytes to be
    //! written into the framer buffer.
    //! \param[in] uiDataBytes_ The number of bytes contained in pucDataBuffer_.
    //
    //! \return The number of bytes written to the internal circular buffer.
    //----------------------------------------------------------------------------
    size_t Write(const unsigned char* pucDataBuffer_, size_t uiDataBytes_) { return pclMyFixedRingBuffer->Write(pucDataBuffer_, uiDataBytes_); }

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
        const uint32_t uiBytesToFlush = std::min(static_cast<uint32_t>(pclMyFixedRingBuffer->size()), uiBufferSize_);
        HandleUnknownBytes(pucBuffer_, uiBytesToFlush);
        return uiBytesToFlush;
    }

    //----------------------------------------------------------------------------
    //! \brief Get the number of bytes currently available in the internal buffer.
    //!
    //! \return The number of bytes available in the internal circular buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] size_t GetAvailableSpace() const { return pclMyFixedRingBuffer->available_space(); }

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
    //! \brief Attempt to discover a frame from the internal circular buffer if one exists
    //
    //! \param[in] pucFrameBuffer_ The buffer to contain the discovered frame.
    //! \param[in] uiFrameBufferSize_ The size of the provided buffer.
    //
    //! \return The status of the frame discovery.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase*& stMetaData_);

    //----------------------------------------------------------------------------
    //! \brief Get the registered framer factories.
    //
    //! \return A map from framer names to their associated factory functions.
    //----------------------------------------------------------------------------
    static auto GetFramerFactories()
        -> std::unordered_map<std::string, std::pair<std::function<std::unique_ptr<FramerBase>(std::shared_ptr<UCharFixedRingBuffer>)>,
                                                     std::function<std::unique_ptr<MetaDataBase>()>>>&;

  protected:
    bool bMyReportUnknownBytes{true};

  private:
    //! NOTE: disable copies and moves.
    FramerManager(const FramerManager&) = delete;
    FramerManager(const FramerManager&&) = delete;
    FramerManager& operator=(const FramerManager&) = delete;
    FramerManager& operator=(const FramerManager&&) = delete;

    MetaDataBase stMyMetaData;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    std::shared_ptr<UCharFixedRingBuffer> pclMyFixedRingBuffer;

    void HandleUnknownBytes(unsigned char* pucBuffer_, size_t uiUnknownBytes_) const;

    std::deque<FramerEntry> framerRegistry;
};

} // namespace novatel::edie

#endif // FRAMER_MANAGER_HPP
