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

#include "novatel_edie/decoders/oem/framer_binary_base.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class FramerBinary
//! \brief Search bytes for patterns that could be an OEM binary message.
//============================================================================
class FramerBinary : public FramerBinaryBase<HEADER_FORMAT::BINARY, OEM4_BINARY_HEADER_LENGTH, 8U, uint16_t, MAX_BINARY_MESSAGE_LENGTH>
{
  protected:
    [[nodiscard]] std::array<unsigned char, 3> GetSyncByteArray() const noexcept override
    {
        return {OEM4_BINARY_SYNC1, OEM4_BINARY_SYNC2, OEM4_BINARY_SYNC3};
    }

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBinary class.
    //! \param [in] buffer a shared pointer to the framer manager's fixed buffer.
    //----------------------------------------------------------------------------
    FramerBinary(std::shared_ptr<UCharFixedBuffer> buffer);

    //----------------------------------------------------------------------------
    //! \brief A constructor for the FramerBinary class.
    //----------------------------------------------------------------------------
    FramerBinary();
};

} // namespace novatel::edie::oem
