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
// ! \file outputfilestream.hpp
// ===============================================================================

#ifndef OUTPUT_FILE_STREAM_HPP
#define OUTPUT_FILE_STREAM_HPP

#include "novatel_edie/stream_interface/filestream.hpp"
#include "novatel_edie/stream_interface/outputstreaminterface.hpp"

/*! \class OutputFileStream
 *  \brief A Derived class from parent interface class OutputStreamInterface.
 *
 *  More detailed FileStream class description. It will also support
 *  File names with Wide characters.
 */
class OutputFileStream : public OutputStreamInterface
{
  public:
    /*! A Constructor.
     *  \brief  Creates FileStream Object with wide character file name for writing output.
     *  And initializes MessageDataFilter object to NULL.
     *  \param[in] s32FileName_ Output file name with wide characters.
     *
     */
    OutputFileStream(const std::u32string& s32FileName_);

    /*! A Constructor.
     *  \brief  Creates FileStream Object for writing output. And initializes MessageDataFilter
     *  object to nullptr.
     *  \param[in] pcFileName Output file name.
     */
    OutputFileStream(const char* pcFileName);

    /*! Disabled Copy Constructor
     *
     *  A copy constructor is a member function which initializes an object using another object of
     * the same class.
     */
    OutputFileStream(const OutputFileStream& clTemp_) = delete;

    /*! Disabled assignment operator
     *
     *  The copy assignment operator is called whenever selected by overload resolution,
     *  e.g. when an object appears on the left side of an assignment expression.
     */
    const OutputFileStream& operator=(const OutputFileStream& clTemp_) = delete;

    /*! A virtual destructor.
     *  \brief  Clears MessageDataFilter and FileStream objects.
     */
    ~OutputFileStream() override;

    /*! FileStream Class object.
     */
    FileStream* pOutFileStream;

    /*! \brief Write data to output file.
     *  \param[in] cData Buffer pointer.
     *  \param[in] uiSize size of the buffer.
     *  \return Number of bytes written to output file.
     *  \remark Set Split type and write data to output files. If split type was not set,
     *  Then writing can be done to only one file.
     */
    uint32_t WriteData(const char* cData, uint32_t uiSize) override;

  private:
};

#endif