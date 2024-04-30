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
// ! \file hwinterface_export.h
// ===============================================================================

#ifndef HWINTERFACE_EXPORT_H
#define HWINTERFACE_EXPORT_H

#ifdef HWINTERFACE_STATIC_DEFINE
#define HWINTERFACE_EXPORT
#define HWINTERFACE_NO_EXPORT
#else
#ifndef HWINTERFACE_EXPORT
#ifdef HWINTERFACE_EXPORTS
/* We are building this library */
#define HWINTERFACE_EXPORT __declspec(dllexport)
#else
/* We are using this library */
#define HWINTERFACE_EXPORT __declspec(dllimport)
#endif
#endif

#ifndef HWINTERFACE_NO_EXPORT
#define HWINTERFACE_NO_EXPORT
#endif
#endif

#ifndef HWINTERFACE_DEPRECATED
#define HWINTERFACE_DEPRECATED __declspec(deprecated)
#endif

#ifndef HWINTERFACE_DEPRECATED_EXPORT
#define HWINTERFACE_DEPRECATED_EXPORT HWINTERFACE_EXPORT HWINTERFACE_DEPRECATED
#endif

#ifndef HWINTERFACE_DEPRECATED_NO_EXPORT
#define HWINTERFACE_DEPRECATED_NO_EXPORT HWINTERFACE_NO_EXPORT HWINTERFACE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#ifndef HWINTERFACE_NO_DEPRECATED
#define HWINTERFACE_NO_DEPRECATED
#endif
#endif

#endif /* HWINTERFACE_EXPORT_H */
