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

/*! \file inputmemorystream.hpp
 *  \brief Class for writing buffer data to circullar buffer which will be used for decoding by
 * decoder.
 *
 *  Class will provide methods to update circullar buffer which will be used by decoder for decoding
 * data. Charater buffer data could be Ascii/Binary type.
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef INPUTMEMORYSTREAM_HPP
#define INPUTMEMORYSTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "inputstreaminterface.hpp"
#include "memorystream.hpp"

/*! \class InputMemoryStream
 *   \brief A Derived Class for writing buffer data to circullar buffer which will be used for
 * decoding by decoder.
 *
 *  Class will provide methods to update circullar buffer which will be used by decoder for decoding
 * data. Charater buffer data could be Ascii/Binary type.
 */
class InputMemoryStream : public InputStreamInterface
{
  public:
    /*! A Constructor
     *  \brief  Creates New MemoryStream class object. This will be used for writing data to buffer
     *  \sa MemoryStream
     *
     */
    InputMemoryStream();

    /*! A Constructor
     *  \brief  Created MemoryStream object with default buffer with size 1024 bytes.
     *  \sa MemoryStream
     *  \param [in] uiBufferSize Buffer size
     */
    InputMemoryStream(uint32_t uiBufferSize);

    /*! A Constructor
     *  \brief  Created buffer of given size and append provided data.
     *  \sa MemoryStream
     *  \param [in] pucBuffer Data to be append to the buffer
     *  \param [in] uiContentSize size of the data
     *
     */
    InputMemoryStream(uint8_t* pucBuffer, uint32_t uiContentSize);

    /*! a default destructor */
    virtual ~InputMemoryStream();

    /*! \fn StreamReadStatus ReadData(ReadDataStructure&)
     *  \brief Hold/copy decoded log and size of it in ReadDataStructure
     *
     *  \param [in] pReadDataStructure ReadDataStructure variable to hold decoded log
     *  \return StreamReadStatus read data statistics
     */
    StreamReadStatus ReadData(ReadDataStructure& pReadDataStructure);

    /*! \fn uint32_t Write(uint8_t* pcData_, uint32_t uiBytes_ )
     *  \brief Write buffer data with desired size uiBytes_.
     *
     *  \param [in] pcData_ Data to be written the circullar buffer, will be used to decode.
     *  \param [in] uiBytes_ Number of bytes in data buffer.
     *  \return Number of bytes written to the stream.
     */
    uint32_t Write(uint8_t* pcData_, uint32_t uiBytes_);

    /*! \fn bool IsStreamAvailable()
     *  \brief Checks whether the data in circullar buffer avaialble to decode or not.
     *
     *  \return true or false.
     */
    bool IsStreamAvailable();

    /*! \fn MemoryStream* GetMemoryStream()
     *  \brief Returns the class object which has interfacesed or derived from circuallar buffer.
     *
     *  \return MemoryStream class Object to access circullar buffer.
     *  \sa MemoryStream
     */
    MemoryStream* GetMemoryStream() { return pMyInMemoryStream; }

  private:
    /*! Private Copy Constructor
     *
     *  A copy constructor is a member function which initializes an object using another object of
     * the same class.
     */
    InputMemoryStream(const InputMemoryStream& clTemp);

    /*! Private assignment operator
     *
     *  The copy assignment operator is called whenever selected by overload resolution,
     *  e.g. when an object appears on the left side of an assignment expression.
     */
    const InputMemoryStream& operator=(const InputMemoryStream& clTemp);

    /*! Memory Stream (Buffer) class holder. */
    MemoryStream* pMyInMemoryStream;
};

#endif
