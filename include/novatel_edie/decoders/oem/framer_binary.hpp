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
// ! \file framer_binary.hpp
// ===============================================================================

#pragma once

#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class FramerBinary
//! \brief Search bytes for patterns that could be an OEM binary message.
//============================================================================
class FramerBinary : public FramerBase
{
  private:
    static constexpr size_t OEM4_BINARY_MESSAGE_LENGTH_INDEX{8U};

    //----------------------------------------------------------------------------
    //! \brief Frame an OEM message from bytes written to the Framer.
    //
    //! \param[out] pucFrameBuffer_ The buffer which the Framer should copy the
    //! framed OEM message to.
    //! \param[in] uiFrameBufferSize_ The length of pucFrameBuffer_.
    //! \param[out] stMetaData_ A MetaDataBase to contain some information
    //! about OEM message frame.
    //! \param [out] bMetadataOnly_ Only populate metadata and do not copy the message.
    //
    //! \return An error code describing the result of framing.
    //!   SUCCESS: An OEM message frame was found.
    //!   NULL_PROVIDED: pucFrameBuffer_ is a null pointer.
    //!   UNKNOWN: The bytes returned are unknown.
    //!   INCOMPLETE: The framer found what could be an OEM message, but there
    //! are no more bytes in the internal buffer.
    //!   BUFFER_EMPTY: There are no more bytes in the internal buffer.
    //!   BUFFER_FULL: pucFrameBuffer_ has no more room for added bytes, according
    //! to the size specified by uiFrameBufferSize_.
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataBase& stMetaData_,
                                  bool bMetadataOnly_ = false) override;

  public:
    //----------------------------------------------------------------------------
    //! \brief Reset the state of the Framer.
    //----------------------------------------------------------------------------
    void ResetState() override {}

    //----------------------------------------------------------------------------
    //! \brief Initialize the attributes of the Framer.
    //----------------------------------------------------------------------------
    void InitAttributes() override
    {
        uiMyByteCount = 0;
        uiMyCalculatedCrc32 = 0;
    };

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBinary class.
    //! \param [in] ringBuffer a shared pointer to the framer manager's fixed ring
    //! buffer.
    //----------------------------------------------------------------------------
    FramerBinary(std::shared_ptr<UCharFixedRingBuffer> ringBuffer);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBinary class.
    //----------------------------------------------------------------------------
    FramerBinary();

    //----------------------------------------------------------------------------
    //! \brief Public interface to frame an OEM message - enforces metadata type.
    //
    //! \see GetFrame(unsigned char*, uint32_t, MetaDataBase&, bool)
    //----------------------------------------------------------------------------
    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataStruct& stMetaData_)
    {
        return GetFrame(pucFrameBuffer_, uiFrameBufferSize_, stMetaData_, false);
    }
};

} // namespace novatel::edie::oem
