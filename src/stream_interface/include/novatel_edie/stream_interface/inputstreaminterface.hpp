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
// ! \file inputstreaminterface.hpp
// ===============================================================================

#ifndef NOVATEL_EDIE_STREAM_INTERFACE_INPUT_STREAM_INTERFACE_HPP
#define NOVATEL_EDIE_STREAM_INTERFACE_INPUT_STREAM_INTERFACE_HPP

#include <stdexcept>

#include "novatel_edie/stream_interface/common.hpp"

//-----------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------
class MemoryStream;

/*! \class InputStreamInterface
 *   \brief An Interface Class used by application ro provide input to the decoder.
 *
 *  Interface class for reading data from all types of decoder input(File/Port/Buffer)
 */
class InputStreamInterface
{
  public:
    /** A pure virtual member.
     *
     * \sa ReadData()
     * \param [in] pReadDataStructure ReadDataStructure variable to hold decoded log
     * \return StreamReadStatus structure - Read data statistics
     */
    virtual StreamReadStatus ReadData(ReadDataStructure& pReadDataStructure) = 0;

    /** A virtual member.
     * \brief Checks whether the data in circular buffer available to decode or not.
     * \sa IsStreamAvailable().
     * \return true or false.
     * \remark If no concrete derived method, It simply returns false.
     */
    virtual bool IsStreamAvailable() { return false; }

    /** A virtual member.
     * \brief Read one line from the file.
     * \sa ReadLine().
     * \return Returns Read statistics structure (StreamReadStatus)
     * \remark If no concrete derived method, It simply returns default StreamReadStatus value.
     */
    virtual StreamReadStatus ReadLine(std::string&) { return {}; }

    /** A virtual member.
     * \brief Returns the extension of the input file to be decoded.
     * \sa GetFileExtension().
     * \return Returns the extension of the input file to be decoded.
     */
    virtual std::string GetFileExtension() const { throw std::logic_error("Unsupported method"); };

    virtual std::string GetFileName() const { throw std::logic_error("Unsupported method"); };

    /** A virtual member.
     * \brief Waiting period on port for incoming data to be decoded.
     * \sa SetTimeOut().
     * \remark No default implementation.
     */
    virtual void SetTimeOut(double) {}

    /** A virtual member.
     * \brief Set/Reset File Position from which next read will be done.
     * \sa Reset().
     * \remark No default implementation.
     */
    virtual void Reset(std::streamoff, std::ios_base::seekdir) {}

    /** A virtual member.
     * \brief Returns the current file position from which next read will be done.
     * \sa GetCurrentFilePosition().
     * \remark Default returns 0.
     */
    virtual uint64_t GetCurrentFilePosition() { return 0; }

    /** A virtual default destructor.
     */
    virtual ~InputStreamInterface() = default;

    /** A virtual member.
     * \brief Sets the current file offset. It could be read bytes so far.
     * \sa SetCurrentFileOffset().
     * \remark No default implementation.
     */
    virtual void SetCurrentFileOffset(uint64_t) {}

    /** A virtual member.
     * \brief Returns Current file offset..
     * \sa GetCurrentFileOffset().
     * \remark returns 0, if no concrete derived method for it.
     */
    virtual uint64_t GetCurrentFileOffset() const { return 0; }

    /** A virtual member.
     * \brief Returns the class object which has interfaced or derived from circular buffer.
     * \sa GetMemoryStream().
     * \remark MemoryStream* class Object to access circular buffer.
     */
    virtual MemoryStream* GetMemoryStream() { return nullptr; }
};

#endif
