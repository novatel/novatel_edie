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
#include "novatel_edie/common/circular_buffer.hpp"
#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/common/framer_manager.hpp"
#include "novatel_edie/decoders/oem/framer.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

// TODO Ideally each framer would register itself with the FramerManager, but the Linker doesn't include that code unless it's explicitly imported and
// used in whatever binary is being built. I've tried using "#pragma comment(linker", and a few other methods in MSVC attempt to force the Linker to
// include this code, but none were successful. It would improve the usability of the FramerManger if someone could figure out how to register the
// Framers without the user being required to import this file and call RegisterAllFramers()

//----------------------------------------------------------------------------
//! \brief Hold static function pointers for both framer and metadata factories.
//! This will contain a set of factories for each supported framer.
//----------------------------------------------------------------------------
inline void RegisterAllFramers()
{
    static const bool registerOEMFramer = ([]() {
        FramerManager::RegisterFramer(
            "OEM",
            [](std::shared_ptr<CircularBuffer> buffer) -> std::unique_ptr<novatel::edie::FramerBase> {
                auto framer = std::make_unique<novatel::edie::oem::Framer>(buffer);
                if (!framer) { throw std::runtime_error("Failed to create OEM Framer"); }
                return framer;
            },
            []() -> std::unique_ptr<MetaDataBase> {
                auto metaData = std::make_unique<MetaDataStruct>();
                if (!metaData) { throw std::runtime_error("Failed to create MetaDataStruct"); }
                return metaData;
            });
        return true;
    })();
}

#endif // FRAMER_REGISTRATION_HPP
