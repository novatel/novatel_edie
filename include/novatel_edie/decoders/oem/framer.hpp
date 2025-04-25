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

#ifndef NOVATEL_FRAMER_HPP
#define NOVATEL_FRAMER_HPP

#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/oem/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class Framer
//! \brief Search bytes for patterns that could be OEM message.
//============================================================================
class Framer : public FramerBase
{
  private:
    NovAtelFrameState eMyFrameState{NovAtelFrameState::WAITING_FOR_SYNC};

    uint32_t uiMyJsonObjectOpenBraces{0};
    uint32_t uiMyAbbrevAsciiHeaderPosition{0};

    //----------------------------------------------------------------------------
    //! \brief Check if the characters following an '*' fit the CRC format.
    //! \param[in] uiDelimiterPosition_ Position of the CRC delimiter '*'.
    //! \return If a CRLF appears 8 characters after uiDelimiterPosition_.
    //----------------------------------------------------------------------------
    [[nodiscard]] bool IsAsciiCrc(uint32_t uiDelimiterPosition_) const;
    [[nodiscard]] bool IsAbbrevSeparatorCrlf(uint32_t uiCircularBufferPosition_) const;
    [[nodiscard]] bool IsEmptyAbbrevLine(uint32_t uiCircularBufferPosition_) const;
    [[nodiscard]] bool IsAbbrevAsciiResponse() const;

  public:
    //----------------------------------------------------------------------------
    //! \brief Reset the state of the Framer.
    //----------------------------------------------------------------------------
    void ResetState() override { eMyFrameState = NovAtelFrameState::WAITING_FOR_SYNC; };

    //----------------------------------------------------------------------------
    //! \brief A constructor for the Framer class.
    //! \param [in] circularBuffer a shared pointer to the framer manager's circular buffer.
    //----------------------------------------------------------------------------
    Framer(std::shared_ptr<CircularBuffer> circularBuffer);


    //----------------------------------------------------------------------------
    //! \brief A constructor for the Framer class.
    //----------------------------------------------------------------------------
    Framer();

    //----------------------------------------------------------------------------
    //! \brief Find the next sync byte in the circular buffer.
    //! \param[in] pucFrameBuffer_ The buffer to search for the next sync byte.
    //! \param[in] uiFrameBufferSize_ The length of pucFrameBuffer_.
    //! \return The offset of the next sync byte. | -1 if no sync byte is found within the buffer.
    //---------------------------------------------------------------------------
    uint32_t FindSyncOffset(uint32_t uiFrameBufferSize_, STATUS& offsetStatus) override;

    //----------------------------------------------------------------------------
    //! \brief Frame an OEM message from bytes written to the Framer.
    //
    //! \param[out] pucFrameBuffer_ The buffer which the Framer should copy the
    //! framed OEM message to.
    //! \param[in] uiFrameBufferSize_ The length of pucFrameBuffer_.
    //! \param[out] stMetaData_ A MetaDataBase to contain some information
    //! about OEM message frame.
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

};

} // namespace novatel::edie::oem

#endif // FRAMER_H
