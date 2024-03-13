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
//! \file decoders_export.h
//! \brief Utilities for DLL export.
////////////////////////////////////////////////////////////////////////
#ifndef DECODERS_EXPORT_H
#define DECODERS_EXPORT_H

#if defined(_WIN32) || defined(_WIN64)
// For Windows platforms
#ifdef DECODERS_STATIC_DEFINE
#define DECODERS_EXPORT
#define DECODERS_NO_EXPORT
#else
#ifndef DECODERS_EXPORT
#ifdef DECODERS_EXPORTS
// We are building this library.
#define DECODERS_EXPORT __declspec(dllexport)
#else
// We are using this library.
#define DECODERS_EXPORT __declspec(dllimport)
#endif
#endif

#ifndef DECODERS_NO_EXPORT
#define DECODERS_NO_EXPORT
#endif
#endif

#ifndef DECODERS_DEPRECATED
#define DECODERS_DEPRECATED __declspec(deprecated)
#endif

#ifndef DECODERS_DEPRECATED_EXPORT
#define DECODERS_DEPRECATED_EXPORT DECODERS_EXPORT DECODERS_DEPRECATED
#endif

#ifndef DECODERS_DEPRECATED_NO_EXPORT
#define DECODERS_DEPRECATED_NO_EXPORT DECODERS_NO_EXPORT DECODERS_DEPRECATED
#endif
#else
// For Linux platforms
#define DECODERS_EXPORT
#define DECODERS_NO_EXPORT
#ifndef DECODERS_DEPRECATED
#define DECODERS_DEPRECATED __attribute__((deprecated))
#endif

#ifndef DECODERS_DEPRECATED_EXPORT
#define DECODERS_DEPRECATED_EXPORT DECODERS_EXPORT DECODERS_DEPRECATED
#endif

#ifndef DECODERS_DEPRECATED_NO_EXPORT
#define DECODERS_DEPRECATED_NO_EXPORT DECODERS_NO_EXPORT DECODERS_DEPRECATED
#endif
#endif

#if 0 // DEFINE_NO_DEPRECATED
#ifndef DECODERS_NO_DEPRECATED
#define DECODERS_NO_DEPRECATED
#endif
#endif

#endif // DECODERS_EXPORT_H
