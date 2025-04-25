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
// ! \file framer.cpp
// ===============================================================================

#ifndef FRAMER_REGISTRATION_HPP
#define FRAMER_REGISTRATION_HPP

// Include all Framer headers
#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/oem/framer.hpp"
#include "novatel_edie/common/circular_buffer.hpp"
#include "novatel_edie/decoders/common/framer_manager.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// Explicitly register framers
inline void RegisterAllFramers()
{
    static const bool registerOEMFramer = ([]() {
        FramerManager::RegisterFramer(
            "OEM",
            [](std::shared_ptr<CircularBuffer> buffer) -> std::unique_ptr<novatel::edie::FramerBase> {
                return std::make_unique<novatel::edie::oem::Framer>(buffer);
            },
            []() -> std::unique_ptr<MetaDataBase> { return std::make_unique<MetaDataStruct>(); });
        return true;
    })();
}

#endif // FRAMER_REGISTRATION_HPP
