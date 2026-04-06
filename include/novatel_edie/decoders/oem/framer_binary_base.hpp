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
// ! \file framer_binary_base.hpp
// ===============================================================================

#pragma once

#include "novatel_edie/common/crc32.hpp"
#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class FramerBinaryBase
//! \brief Search bytes for patterns that could be an OEM binary message.
//============================================================================
template <HEADER_FORMAT HeaderFormat, size_t HeaderLength, size_t MessageLengthIndex, typename MessageLengthT, size_t MaxMessageLength>
class FramerBinaryBase : public FramerBase
{
  protected:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBinaryBase class.
    //! \param [in] buffer a shared pointer to the framer manager's fixed buffer.
    //----------------------------------------------------------------------------
    FramerBinaryBase(std::shared_ptr<UCharFixedBuffer> buffer, const std::string& loggerName) : FramerBase(loggerName, buffer) {}

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBinaryBase class.
    //----------------------------------------------------------------------------
    FramerBinaryBase(const std::string& loggerName) : FramerBase(loggerName) {}

    //----------------------------------------------------------------------------
    //! \brief Get the sync bytes that identify this binary message format.
    //----------------------------------------------------------------------------
    [[nodiscard]] virtual std::array<unsigned char, 3> GetSyncByteArray() const noexcept = 0;

    //----------------------------------------------------------------------------
    //! \brief Find the index of the first sync byte sequence in the internal buffer.
    //!
    //! \return The index of the first sync byte sequence if found, or the minimum of the
    //!     buffer size and the max lookahead if not found.
    //----------------------------------------------------------------------------
    size_t FindSync() const override
    {
        const auto syncIndex = pclMyBuffer->search_chars(GetSyncByteArray(), 0, MAX_LOOKAHEAD_BYTES);
        return syncIndex == UCharFixedBuffer::npos ? std::min(pclMyBuffer->size(), MAX_LOOKAHEAD_BYTES) : syncIndex;
    }

    //----------------------------------------------------------------------------
    //! \brief Find the end of the candidate frame starting from the provided index.
    //!
    //! \param[in] start The index from which to start searching for the end of frame.
    //! \return A FindFrameEndResult struct containing the status, format, and index of the end of the frame.
    //!
    //! \see FramerBase::FindFrameEndResult for details on the return struct.
    //----------------------------------------------------------------------------
    FindFrameEndResult FindFrameEnd([[maybe_unused]] size_t start) const override
    {
        using Result = FindFrameEndResult;
        const auto& clFrameBuffer = *pclMyBuffer;
        const auto uiBufferSize = clFrameBuffer.size();
        if (uiBufferSize < HeaderLength)
        {
            return uiBufferSize < OEM4_BINARY_SYNC_LENGTH ? Result{Result::Status::INCOMPLETE, HEADER_FORMAT::UNKNOWN, start}
                                                          : Result{Result::Status::INCOMPLETE, HeaderFormat, uiBufferSize};
        }
        const size_t uiTotalMessageLength =
            HeaderLength + clFrameBuffer.template read_value<MessageLengthT>(MessageLengthIndex) + OEM4_BINARY_CRC_LENGTH;
        if (uiTotalMessageLength > MaxMessageLength) { return {Result::Status::INVALID, HEADER_FORMAT::UNKNOWN, OEM4_BINARY_SYNC_LENGTH}; }
        return uiTotalMessageLength > uiBufferSize ? Result{Result::Status::INCOMPLETE, HeaderFormat, uiBufferSize}
                                                   : Result{Result::Status::COMPLETE, HeaderFormat, uiTotalMessageLength};
    }

    //----------------------------------------------------------------------------
    //! \brief Validate the candidate frame ending at the byte preceding the provided index.
    //!
    //! \param[in] frameEnd The index following the last byte of the candidate frame to validate.
    //! \return If the frame is valid, return 0. If the frame is invalid, return the number of
    //!     bytes to discard before the next potential frame.
    //----------------------------------------------------------------------------
    size_t Validate(size_t frameEnd) const override
    {
        const auto& clFrameBuffer = *pclMyBuffer;
        uint32_t uiMessageCrc = clFrameBuffer.template read_value<uint32_t>(frameEnd - sizeof(uint32_t));
        uint32_t uiCalcCrc = CalculateBlockCrc32(clFrameBuffer.data(), static_cast<uint32_t>(frameEnd - sizeof(uint32_t)));
        return uiCalcCrc == uiMessageCrc ? 0 : OEM4_BINARY_SYNC_LENGTH;
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief Reset the state of the Framer.
    //----------------------------------------------------------------------------
    void ResetState() override {}

    using FramerBase::GetFrame;

    //----------------------------------------------------------------------------
    //! \brief Public interface to frame an OEM binary message - enforces metadata type.
    //
    //! \see GetFrame(unsigned char*, uint32_t, MetaDataBase&, bool)
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataStruct& stMetaData_)
    {
        return FramerBase::GetFrame(pucFrameBuffer_, uiFrameBufferSize_, stMetaData_, false);
    }
};

} // namespace novatel::edie::oem
