////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 NovAtel Inc.
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
////////////////////////////////////////////////////////////////////////////////

/*! \file outputfilestream.hpp
 *  \brief Class definition to provide basic file operations for writing output files.
 *
 *  Class has defined in a such a way that will provide basic file operations writing data to it,
 *  Will support, Open/Close/Write/Read ... operations etc.
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef OUTPUTFILESTREAM_HPP
#define OUTPUTFILESTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "filestream.hpp"
#include "outputstreaminterface.hpp"

/*! \class OutputFileStream
 *  \brief A Derived class from parent interface class OutputStreamInterface.
 *
 *  More detailed FileStream class description. It will also supports
 *  File names with Wide characters.
 */
class OutputFileStream : public OutputStreamInterface
{
  public:
    /*! A Constructor.
     *  \brief  Creates FileStream Object with wide character file name for writing output.
     *  And intializes MessageDataFilter object to NULL.
     *  \param [in] pcFileName Output file name with wide characters.
     *
     */
    OutputFileStream(const std::u32string ps32FileName_);

    /*! A Constructor.
     *  \brief  Creates FileStream Object for writing output. And intializes MessageDataFilter
     * object to NULL. \param [in] pcFileName Output file name.
     *
     */
    OutputFileStream(const char* pcFileName);

    /*! A virtual destructor.
     *  \brief  Clears MessageDataFilter and FileStream objects.
     *
     */
    virtual ~OutputFileStream();

    /*! FileStream Class object.
     * \sa FileStream
     */
    FileStream* pOutFileStream;

    /*! \fn uint32_t WriteData(char* cData, uint32_t uiSize)
     *  \brief Write data to output file.
     *  \param [in] cData Buffer pointer.
     *  \param [in] uiSize size of the buffer.
     *  \return Number of bytes written to output file.
     *  \remark Set Split type and write data to output files. If split type was not set,
     *  Then writing can be done to only one file.
     */
    uint32_t WriteData(char* cData, uint32_t uiSize);

  private:
    /*! Private Copy Constructor
     *
     *  A copy constructor is a member function which initializes an object using another object of
     * the same class.
     */
    OutputFileStream(const OutputFileStream& clTemp);

    /*! Private assignment operator
     *
     *  The copy assignment operator is called whenever selected by overload resolution,
     *  e.g. when an object appears on the left side of an assignment expression.
     */
    const OutputFileStream& operator=(const OutputFileStream& clTemp);
};

#endif
