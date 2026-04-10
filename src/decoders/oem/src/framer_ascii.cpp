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
// ! \file framer_ascii.cpp
// ===============================================================================

#include "novatel_edie/decoders/oem/framer_ascii.hpp"

#include "novatel_edie/decoders/common/framer_registration.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// Register the OEM ASCII framer with the framer factory
REGISTER_FRAMER(OEM_ASCII, oem::FramerAscii, MetaDataStruct)

// -------------------------------------------------------------------------------------------------------
FramerAscii::FramerAscii() : FramerAsciiBase("novatel_framer_ascii") {}

// -------------------------------------------------------------------------------------------------------
FramerAscii::FramerAscii(std::shared_ptr<UCharFixedBuffer> buffer) : FramerAsciiBase(buffer, "novatel_framer_ascii")
{
    pclMyLogger->info("FramerAscii initialized");
}
