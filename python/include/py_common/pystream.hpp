// Based on https://github.com/cctbx/cctbx_project/blob/master/boost_adaptbx/python_streambuf.h
// which has been adapted to nanobind (by Martin Valgur, released under Unlicense) at
// https://github.com/valgur/ncompress/blob/main/src/pystreambuf.h

// SPDX-License-Identifier: BSD-3-Clause-LBNL
//
// *** License agreement ***
//
// cctbx Copyright (c) 2006, The Regents of the University of
// California, through Lawrence Berkeley National Laboratory (subject to
// receipt of any required approvals from the U.S. Dept. of Energy).  All
// rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley
// National Laboratory, U.S. Dept. of Energy nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// You are under no obligation whatsoever to provide any bug fixes,
// patches, or upgrades to the features, functionality or performance of
// the source code ("Enhancements") to anyone; however, if you choose to
// make your Enhancements available either publicly, or directly to
// Lawrence Berkeley National Laboratory, without imposing a separate
// written license agreement for such Enhancements, then you hereby grant
// the following license: a  non-exclusive, royalty-free perpetual license
// to install, use, modify, prepare derivative works, incorporate into
// other computer software, distribute, and sublicense such enhancements or
// derivative works thereof, in binary and source code form.

#pragma once

#include <iostream>
#include <memory>
#include <streambuf>
#include <vector>

#include <bytesobject.h>
#include <nanobind/nanobind.h>
#include <pyerrors.h>

namespace pystream {

namespace nb = nanobind;

/// A stream buffer getting data from and putting data into a Python file object
/** The aims are as follow:

    - Given a C++ function acting on a standard stream, e.g.

      \code
      void read_inputs(std::istream& input) {
        ...
        input >> something >> something_else;
      }
      \endcode

      and given a piece of Python code which creates a file-like object,
      to be able to pass this file object to that C++ function, e.g.

      \code
      import gzip
      gzip_file_obj = gzip.GzipFile(...)
      read_inputs(gzip_file_obj)
      \endcode

      and have the standard stream pull data from and put data into the Python
      file object.

    - When Python \c read_inputs() returns, the Python object is able to
      continue reading or writing where the C++ code left off.

    - Operations in C++ on mere files should be competitively fast compared
      to the direct use of \c std::fstream.


    \b Motivation

      - the standard Python library offer of file-like objects (files,
        compressed files and archives, network, ...) is far superior to the
        offer of streams in the C++ standard library and Boost C++ libraries.

      - i/o code involves a fair amount of text processing which is more
        efficiently prototyped in Python but then one may need to rewrite
        a time-critical part in C++, in as seamless a manner as possible.

    \b Usage

    This is 2-step:

      - a trivial wrapper function

        \code
          using pystream::streambuf;
          void read_inputs_wrapper(streambuf& input)
          {
            streambuf::istream is(input);
            read_inputs(is);
          }

          def("read_inputs", read_inputs_wrapper);
        \endcode

        which has to be written every time one wants a Python binding for
        such a C++ function.

      - the Python side

        \code
          from pystream import streambuf
          read_inputs(streambuf(python_file_obj=obj, buffer_size=1024))
        \endcode

        \c buffer_size is optional. See also: \c default_buffer_size

  Note: references are to the C++ standard (the numbers between parentheses
  at the end of references are margin markers).
*/
class streambuf : public std::basic_streambuf<char>
{
  private:
    using base_t = std::basic_streambuf<char>;

  public:
    using char_type = base_t::char_type;
    using int_type = base_t::int_type;
    using pos_type = base_t::pos_type;
    using off_type = base_t::off_type;
    using traits_type = base_t::traits_type;

    /// The default size of the read and write buffer.
    /** They are respectively used to buffer data read from and data written to
        the Python file object. It can be modified from Python.
    */
    static const size_t default_buffer_size = 1024;

    /// Construct from a Python file object
    /** if buffer_size is 0 the current default_buffer_size is used.
     */
    explicit streambuf(nb::object& python_file_obj, size_t buffer_size_ = 0)
        : py_read(getattr(python_file_obj, "read", nb::none())), py_write(getattr(python_file_obj, "write", nb::none())),
          py_seek(getattr(python_file_obj, "seek", nb::none())), py_tell(getattr(python_file_obj, "tell", nb::none())),
          buffer_size(buffer_size_ != 0 ? buffer_size_ : default_buffer_size), pos_of_read_buffer_end_in_py_file(0),
          pos_of_write_buffer_end_in_py_file((off_type)buffer_size), farthest_pptr(nullptr)
    {
        assert(buffer_size != 0);
        /* Some Python file objects (e.g. sys.stdout and sys.stdin)
           have non-functional seek and tell. If so, assign None to
           py_tell and py_seek.
         */
        if (!py_tell.is_none())
        {
            try
            {
                py_tell();
            }
            catch (nb::python_error& err)
            {
                py_tell = nb::none();
                py_seek = nb::none();
                err.restore();
                PyErr_Clear();
            }
        }

        if (!py_write.is_none())
        {
            write_buffer.resize(buffer_size);
            setp(write_buffer.data(), write_buffer.data() + buffer_size); // 27.5.2.4.5 (5)
            farthest_pptr = pptr();
        }
        else
        {
            // The first attempt at output will result in a call to overflow
            setp(nullptr, nullptr);
        }

        if (!py_tell.is_none())
        {
            auto py_pos = nb::cast<off_type>(py_tell());
            pos_of_read_buffer_end_in_py_file = py_pos;
            pos_of_write_buffer_end_in_py_file = py_pos;
        }
    }

    /// C.f. C++ standard section 27.5.2.4.3
    /** It is essential to override this virtual function for the stream
        member function readsome to work correctly (c.f. 27.6.1.3, alinea 30)
     */
    std::streamsize showmanyc() override
    {
        int_type const failure = traits_type::eof();
        int_type status = underflow();
        if (status == failure) return -1;
        return egptr() - gptr();
    }

    /// C.f. C++ standard section 27.5.2.4.3
    int_type underflow() override
    {
        int_type const failure = traits_type::eof();
        if (py_read.is_none()) { throw std::invalid_argument("That Python file object has no 'read' attribute"); }
        read_buffer = nb::cast<nb::bytes>(py_read(buffer_size));
        char* read_buffer_data;
        Py_ssize_t py_n_read;
        if (PyBytes_AsStringAndSize(read_buffer.ptr(), &read_buffer_data, &py_n_read) == -1)
        {
            setg(nullptr, nullptr, nullptr);
            throw std::invalid_argument("The method 'read' of the Python file object "
                                        "did not return a string.");
        }
        off_type n_read = py_n_read;
        pos_of_read_buffer_end_in_py_file += n_read;
        setg(read_buffer_data, read_buffer_data, read_buffer_data + n_read);
        // ^^^27.5.2.3.1 (4)
        if (n_read == 0) return failure;
        return traits_type::to_int_type(read_buffer_data[0]);
    }

    /// C.f. C++ standard section 27.5.2.4.5
    int_type overflow(int_type c = traits_type::eof()) override
    {
        if (py_write.is_none()) { throw std::invalid_argument("That Python file object has no 'write' attribute"); }
        farthest_pptr = std::max(farthest_pptr, pptr());
        off_type n_written = farthest_pptr - pbase();
        nb::bytes chunk(pbase(), (size_t)n_written);
        py_write(chunk);
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            char cs = traits_type::to_char_type(c);
            py_write(nb::bytes(&cs, 1));
            n_written++;
        }
        if (n_written)
        {
            pos_of_write_buffer_end_in_py_file += n_written;
            setp(pbase(), epptr());
            // ^^^ 27.5.2.4.5 (5)
            farthest_pptr = pptr();
        }
        return traits_type::eq_int_type(c, traits_type::eof()) ? traits_type::not_eof(c) : c;
    }

    /// Update the python file to reflect the state of this stream buffer
    /** Empty the write buffer into the Python file object and set the seek
        position of the latter accordingly (C++ standard section 27.5.2.4.2).
        If there is no write buffer or it is empty, but there is a non-empty
        read buffer, set the Python file object seek position to the
        seek position in that read buffer.
    */
    int sync() override
    {
        int result = 0;
        farthest_pptr = std::max(farthest_pptr, pptr());
        if (farthest_pptr && farthest_pptr > pbase())
        {
            off_type delta = pptr() - farthest_pptr;
            int_type status = overflow();
            if (traits_type::eq_int_type(status, traits_type::eof())) result = -1;
            if (!py_seek.is_none()) py_seek(delta, 1);
        }
        else if (gptr() && gptr() < egptr())
        {
            if (!py_seek.is_none()) py_seek(gptr() - egptr(), 1);
        }
        return result;
    }

    /// C.f. C++ standard section 27.5.2.4.2
    /** This implementation is optimised to look whether the position is within
        the buffers, so as to avoid calling Python seek or tell. It is
        important for many applications that the overhead of calling into Python
        is avoided as much as possible (e.g. parsers which may do a lot of
        backtracking)
    */
    pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
    {
        /* In practice, "which" is either std::ios_base::in or out
           since we end up here because either seekp or seekg was called
           on the stream using this buffer. That simplifies the code
           in a few places.
        */
        int const failure = off_type(-1);

        if (py_seek.is_none()) { throw std::invalid_argument("That Python file object has no 'seek' attribute"); }

        // we need the read buffer to contain something!
        if (which == std::ios_base::in && !gptr())
        {
            if (traits_type::eq_int_type(underflow(), traits_type::eof())) { return failure; }
        }

        // compute the whence parameter for Python seek
        int whence;
        switch (way)
        {
        case std::ios_base::beg: whence = 0; break;
        case std::ios_base::cur: whence = 1; break;
        case std::ios_base::end: whence = 2; break;
        default: return failure;
        }

        // Let's have a go
        off_type result;
        if (!seekoff_without_calling_python(off, way, which, result))
        {
            // we need to call Python
            if (which == std::ios_base::out) overflow();
            if (way == std::ios_base::cur)
            {
                if (which == std::ios_base::in)
                    off -= egptr() - gptr();
                else if (which == std::ios_base::out)
                    off += pptr() - pbase();
            }
            py_seek(off, whence);
            result = nb::cast<off_type>(py_tell());
            if (which == std::ios_base::in) underflow();
        }
        return result;
    }

    /// C.f. C++ standard section 27.5.2.4.2
    pos_type seekpos(pos_type sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
    {
        return streambuf::seekoff(sp, std::ios_base::beg, which);
    }

  private:
    nb::object py_read, py_write, py_seek, py_tell;

    size_t buffer_size;

    /* This is actually a Python bytes object and the actual read buffer is
       its internal data, i.e. an array of characters.
     */
    nb::bytes read_buffer;

    /* A mere array of char's allocated on the heap at construction time and
       de-allocated only at destruction time.
    */
    std::vector<char> write_buffer;

    off_type pos_of_read_buffer_end_in_py_file, pos_of_write_buffer_end_in_py_file;

    // the farthest place the buffer has been written into
    char* farthest_pptr;

    bool seekoff_without_calling_python(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which, off_type& result)
    {
        // Buffer range and current position
        off_type buf_begin, buf_end, buf_cur, upper_bound;
        off_type pos_of_buffer_end_in_py_file;
        if (which == std::ios_base::in)
        {
            pos_of_buffer_end_in_py_file = pos_of_read_buffer_end_in_py_file;
            buf_begin = reinterpret_cast<std::streamsize>(eback());
            buf_cur = reinterpret_cast<std::streamsize>(gptr());
            buf_end = reinterpret_cast<std::streamsize>(egptr());
            upper_bound = buf_end;
        }
        else if (which == std::ios_base::out)
        {
            pos_of_buffer_end_in_py_file = pos_of_write_buffer_end_in_py_file;
            buf_begin = reinterpret_cast<std::streamsize>(pbase());
            buf_cur = reinterpret_cast<std::streamsize>(pptr());
            buf_end = reinterpret_cast<std::streamsize>(epptr());
            farthest_pptr = std::max(farthest_pptr, pptr());
            upper_bound = reinterpret_cast<std::streamsize>(farthest_pptr) + 1;
        }
        else { throw std::runtime_error("Control flow passes through branch that should be unreachable."); }

        // Sought position in "buffer coordinate"
        off_type buf_sought;
        if (way == std::ios_base::cur) { buf_sought = buf_cur + off; }
        else if (way == std::ios_base::beg) { buf_sought = buf_end + (off - pos_of_buffer_end_in_py_file); }
        else if (way == std::ios_base::end) { return false; }
        else { throw std::runtime_error("Control flow passes through branch that should be unreachable."); }

        // if the sought position is not in the buffer, give up
        if (buf_sought < buf_begin || buf_sought >= upper_bound) return false;

        // we are in wonderland
        if (which == std::ios_base::in)
            gbump((int)(buf_sought - buf_cur));
        else if (which == std::ios_base::out)
            pbump((int)(buf_sought - buf_cur));

        result = pos_of_buffer_end_in_py_file + (buf_sought - buf_end);
        return true;
    }

  public:
    class istream : public std::istream
    {
      public:
        explicit istream(streambuf& buf) : std::istream(&buf) {
            exceptions(std::ios_base::badbit); }

        ~istream() override
        {
            if (this->good()) this->sync();
        }
    };

    class ostream : public std::ostream
    {
      public:
        explicit ostream(streambuf& buf) : std::ostream(&buf) { exceptions(std::ios_base::badbit); }

        ~ostream() override
        {
            if (this->good()) this->flush();
        }
    };
};

struct streambuf_capsule
{
    streambuf python_streambuf;

    explicit streambuf_capsule(nb::object& python_file_obj, size_t buffer_size = 0) : python_streambuf(python_file_obj, buffer_size) {}
};

struct ostream : private streambuf_capsule, streambuf::ostream
{
    explicit ostream(nb::object& python_file_obj, size_t buffer_size = 0)
        : streambuf_capsule(python_file_obj, buffer_size), streambuf::ostream(python_streambuf)
    {
    }

    ~ostream() override
    {
        if (this->good()) { this->flush(); }
    }
};

struct istream : private streambuf_capsule, streambuf::istream
{
    explicit istream(nb::object& python_file_obj, size_t buffer_size = 0)
        : streambuf_capsule(python_file_obj, buffer_size), streambuf::istream(python_streambuf)
    {
    }

    ~istream() override
    {
        if (this->good()) { this->sync(); }
    }
};

} // namespace pystream

namespace nanobind::detail {
namespace nb = nanobind;

template <> struct type_caster<std::istream>
{
    using Value = std::istream;
    template <typename T_> using Cast = movable_cast_t<T_>;
    static constexpr auto Name = const_name("BinaryIO");
    static constexpr bool IsClass = false;

    bool from_python(handle src, [[maybe_unused]] uint8_t flags, cleanup_list*) noexcept
    {
        if (!hasattr(src, "read")) return false;
        obj = nb::borrow(src);
        value = std::make_unique<pystream::istream>(obj, 0);
        return true;
    }

    static handle from_cpp([[maybe_unused]] Value& value, rv_policy, cleanup_list*) { return none().release(); }

    operator Value*() { return value.get(); }
    operator Value&() { return *value; }
    operator Value&&() && { return (Value&&)*value; }

  protected:
    nb::object obj;
    std::unique_ptr<pystream::istream> value;
};

template <> struct type_caster<std::ostream>
{
    using Value = std::ostream;
    template <typename T_> using Cast = movable_cast_t<T_>;
    static constexpr auto Name = const_name("BinaryIO");
    static constexpr bool IsClass = false;

  public:
    bool from_python(handle src, [[maybe_unused]] uint8_t flags, cleanup_list*) noexcept
    {
        if (!hasattr(src, "write")) return false;
        obj = nb::borrow(src);
        value = std::make_unique<pystream::ostream>(obj, 0);
        return true;
    }

    static handle from_cpp([[maybe_unused]] Value& value, rv_policy, cleanup_list*) { return none().release(); }

    operator Value*() { return value.get(); }
    operator Value&() { return *value; }
    operator Value&&() && { return (Value&&)*value; }

  protected:
    nb::object obj;
    std::unique_ptr<pystream::ostream> value;
};
} // namespace nanobind::detail