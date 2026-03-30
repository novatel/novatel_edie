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
// ! \file framer_ascii.hpp
// ===============================================================================

#pragma once

#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class FramerAscii
//! \brief Search bytes for patterns that could be an OEM ASCII message.
//============================================================================
class FramerAscii : public FramerBase
{
  private:
    size_t FindSync() const override;
    FindFrameEndResult FindFrameEnd(size_t start) const override;
    size_t Validate(size_t frameEnd) const override;

    //----------------------------------------------------------------------------
    //! \brief Check if the characters following an '*' fit the CRC format.
    //! \param[in] uiDelimiterPosition_ Position of the CRC delimiter '*'.
    //! \return If a CRLF appears 8 characters after uiDelimiterPosition_.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool IsAsciiCrc(const uint32_t uiDelimiterPosition_) const { return IsCrlf(uiDelimiterPosition_ + OEM4_ASCII_CRC_LENGTH); }

  public:
    //----------------------------------------------------------------------------
    //! \brief Reset the state of the Framer.
    //----------------------------------------------------------------------------
    void ResetState() override {}

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerAscii class.
    //! \param [in] buffer a shared pointer to the framer manager's fixed buffer.
    //----------------------------------------------------------------------------
    FramerAscii(std::shared_ptr<UCharFixedBuffer> buffer);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerAscii class.
    //----------------------------------------------------------------------------
    FramerAscii();

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
