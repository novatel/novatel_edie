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

#include "novatel_edie/common/fixed_ring_buffer.hpp"
#include "novatel_edie/decoders/common/framer.hpp"
#include "novatel_edie/decoders/common/framer_manager.hpp"

using namespace novatel::edie;

// Macro to register a framer with the FramerManager's factory
#define REGISTER_FRAMER(FramerName, FramerClass, MetaDataClass)                                                                                      \
    namespace {                                                                                                                                      \
    struct FramerName##_Registrar                                                                                                                    \
    {                                                                                                                                                \
        FramerName##_Registrar();                                                                                                                    \
    };                                                                                                                                               \
    static FramerName##_Registrar s_##FramerName##_registrar;                                                                                        \
    FramerName##_Registrar::FramerName##_Registrar()                                                                                                 \
    {                                                                                                                                                \
        novatel::edie::FramerManager::RegisterFramer(                                                                                                \
            #FramerName,                                                                                                                             \
            [](std::shared_ptr<novatel::edie::UCharFixedRingBuffer> buffer) -> std::unique_ptr<novatel::edie::FramerBase> {                          \
                return std::make_unique<FramerClass>(buffer);                                                                                        \
            },                                                                                                                                       \
            []() -> std::unique_ptr<novatel::edie::MetaDataBase> { return std::make_unique<MetaDataClass>(); });                                     \
    }                                                                                                                                                \
    }

#endif // FRAMER_REGISTRATION_HPP
