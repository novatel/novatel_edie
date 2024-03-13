////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file Framer.hpp
//! \brief Frame OEM messages.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef NOVATEL_FRAMER_HPP
#define NOVATEL_FRAMER_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/framerinterface.hpp"
#include "decoders/novatel/api/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class Framer
//! \brief Search bytes for patterns that could be OEM message.
//============================================================================
class Framer : public FramerInterface
{
  private:
    // -------------------------------------------------------------------------------------------------------
    NovAtelFrameStateEnum eMyFrameState{NovAtelFrameStateEnum::WAITING_FOR_SYNC};

    bool bMyPayloadOnly{false};
    bool bMyFrameJson{false};

    uint32_t uiMyJsonObjectOpenBraces{0};
    uint32_t uiMyAbbrevAsciiHeaderPosition{0};

    virtual void HandleUnknownBytes(unsigned char* pucBuffer_, uint32_t uiUnknownBytes_);

    //----------------------------------------------------------------------------
    //! \brief Check if the characters following an '*' fit the CRC format.
    //! \param [in] uiDelimiterPosition_ Position of the CRC delimiter '*'.
    //! \return If a CRLF appears 8 characters after uiDelimiterPosition_.
    //----------------------------------------------------------------------------
    bool IsAsciiCRC(uint32_t uiDelimiterPosition_) const;
    bool IsCRLF(uint32_t uiCircularBufferPosition_) const;
    bool IsSpaceCRLF(uint32_t uiCircularBufferPosition_) const;
    bool IsEmptyLine(uint32_t uiCircularBufferPosition_) const;
    bool IsAbbrevAsciiResponse() const;

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the Framer class.
    //----------------------------------------------------------------------------
    Framer();

    //----------------------------------------------------------------------------
    //! \brief A destructor for the Framer class.
    //----------------------------------------------------------------------------
    virtual ~Framer() = default;

    //----------------------------------------------------------------------------
    //! \brief Should the Framer look for JSON objects in provided bytes?
    //! This setting can be extremely error prone, as there is no CRC search or
    //! sync bytes, just direct comparisons to '{' and '}' characters.
    //
    //! \param[in] bFrameJson_ true if the Framer should search for JSON objects.
    //----------------------------------------------------------------------------
    void SetFrameJson(bool bFrameJson_);

    //----------------------------------------------------------------------------
    //! \brief Should the Framer return only the message body of OEM messages and
    //! discard the header?
    //
    //! \param[in] bPayloadOnly_ true if the Framer should discard OEM message
    //! headers.
    //----------------------------------------------------------------------------
    void SetPayloadOnly(bool bPayloadOnly_);

    //----------------------------------------------------------------------------
    //! \brief Frame an OEM message from bytes written to the Framer.
    //
    //! \param [out] pucFrameBuffer_ The buffer which the Framer should copy the
    //! framed OEM message to.
    //! \param [in] uiFrameBufferSize_ The length of pucFrameBuffer_.
    //! \param [out] stMetaData_ A MetaDataStruct to contain some information
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
    [[nodiscard]] STATUS GetFrame(unsigned char* pucFrameBuffer_, uint32_t uiFrameBufferSize_, MetaDataStruct& stMetaData_);
};
} // namespace novatel::edie::oem
#endif // FRAMER_H
