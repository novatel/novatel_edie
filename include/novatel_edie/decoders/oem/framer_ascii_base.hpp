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
// ! \file framer_ascii_base.hpp
// ===============================================================================

#pragma once

#include <charconv>

#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class FramerAsciiBase
//! \brief Search bytes for patterns that could be an OEM ASCII message.
//============================================================================
template <HEADER_FORMAT HeaderFormat, unsigned char SyncByte, size_t MaxMessageLength> class FramerAsciiBase : public FramerBase
{
  protected:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerAsciiBase class.
    //! \param [in] buffer a shared pointer to the framer manager's fixed buffer.
    //----------------------------------------------------------------------------
    FramerAsciiBase(std::shared_ptr<UCharFixedBuffer> buffer, const std::string& loggerName) : FramerBase(loggerName, buffer) {}

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerAsciiBase class.
    //----------------------------------------------------------------------------
    FramerAsciiBase(const std::string& loggerName) : FramerBase(loggerName) {}

    //----------------------------------------------------------------------------
    //! \brief Find the index of the first sync byte in the internal buffer.
    //!
    //! \return The index of the first sync byte if found, or UCharFixedBuffer::npos if not found.
    //----------------------------------------------------------------------------
    [[nodiscard]] size_t FindSync() const override { return pclMyBuffer->search_char(SyncByte, 0, MaxMessageLength); }

    //----------------------------------------------------------------------------
    //! \brief Find the end of the candidate frame starting from the provided index.
    //!
    //! \param[in] start The index from which to start searching for the CRC delimiter/
    //!    end of frame.
    //! \return A FindFrameEndResult struct containing the status, format, and index of the end of the frame.
    //!
    //! \see FramerBase::FindFrameEndResult for details on the return struct.
    //----------------------------------------------------------------------------
    [[nodiscard]] FramerBase::FindFrameEndResult FindFrameEnd(size_t start) const override
    {
        using Result = FramerBase::FindFrameEndResult;
        const auto& clFrameBuffer = *pclMyBuffer;
        const auto uiBufferSize = clFrameBuffer.size();
        constexpr size_t uiCrcTrailerLength = OEM4_ASCII_CRC_LENGTH + 3; // Delimiter + CRC + CRLF
        auto uiCrcDelimIndex = clFrameBuffer.search_char(OEM4_ASCII_CRC_DELIMITER, start, MaxMessageLength);
        while (uiCrcDelimIndex != UCharFixedBuffer::npos && uiBufferSize - uiCrcDelimIndex >= uiCrcTrailerLength && !IsAsciiCrc(uiCrcDelimIndex + 1))
        {
            uiCrcDelimIndex = clFrameBuffer.search_char(OEM4_ASCII_CRC_DELIMITER, uiCrcDelimIndex + 1, MaxMessageLength);
        }

        if (uiCrcDelimIndex == UCharFixedBuffer::npos)
        {
            return uiBufferSize >= MaxMessageLength ? Result{Result::Status::INVALID, HEADER_FORMAT::UNKNOWN, OEM4_ASCII_SYNC_LENGTH}
                                                    : Result{Result::Status::INCOMPLETE, HeaderFormat, uiBufferSize};
        }
        if (uiBufferSize - uiCrcDelimIndex < uiCrcTrailerLength) { return {Result::Status::INCOMPLETE, HeaderFormat, uiCrcDelimIndex}; }
        return {Result::Status::COMPLETE, HeaderFormat, uiCrcDelimIndex + uiCrcTrailerLength};
    }

    //----------------------------------------------------------------------------
    //! \brief Validate the candidate frame ending at the byte preceding the provided index.
    //!
    //! \param[in] frameEnd The index following the last byte of the candidate frame to validate.
    //! \return If the frame is valid, return 0. If the frame is invalid, return the number of
    //!     bytes to discard before the next potential frame.
    //----------------------------------------------------------------------------
    [[nodiscard]] size_t Validate(size_t frameEnd) const override
    {
        const auto& clFrameBuffer = *pclMyBuffer;
        char acCrc[OEM4_ASCII_CRC_LENGTH + 1];
        std::memcpy(acCrc, clFrameBuffer.data() + frameEnd - OEM4_ASCII_CRC_LENGTH - 2, OEM4_ASCII_CRC_LENGTH);

        uint32_t uiMessageCrc = 0;
        auto result = std::from_chars(acCrc, acCrc + OEM4_ASCII_CRC_LENGTH, uiMessageCrc, 16);
        auto uiCalcCrc = CalculateBlockCrc32(clFrameBuffer.data() + OEM4_ASCII_SYNC_LENGTH,
                                             static_cast<uint32_t>(frameEnd - OEM4_ASCII_SYNC_LENGTH - OEM4_ASCII_CRC_LENGTH - 3));
        return result.ec == std::errc() && uiCalcCrc == uiMessageCrc ? 0 : OEM4_ASCII_SYNC_LENGTH;
    }

    //----------------------------------------------------------------------------
    //! \brief Check if the characters following an '*' fit the CRC format.
    //! \param[in] uiDelimiterPosition_ Position of the CRC delimiter '*'.
    //! \return If a CRLF appears 8 characters after uiDelimiterPosition_.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool IsAsciiCrc(const size_t uiDelimiterPosition_) const
    {
        return IsCrlf(static_cast<uint32_t>(uiDelimiterPosition_ + OEM4_ASCII_CRC_LENGTH));
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief Reset the state of the Framer.
    //----------------------------------------------------------------------------
    void ResetState() override {}

    using FramerBase::GetFrame;

    //----------------------------------------------------------------------------
    //! \brief Public interface to frame an OEM ASCII message - enforces metadata type.
    //
    //! \see GetFrame(unsigned char*, uint32_t, MetaDataBase&, bool)
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataStruct& stMetaData_)
    {
        return FramerBase::GetFrame(pucFrameBuffer_, uiFrameBufferSize_, stMetaData_, false);
    }
};

} // namespace novatel::edie::oem
