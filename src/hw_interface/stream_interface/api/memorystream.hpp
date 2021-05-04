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

/*! \file memorystream.hpp
 *  \brief Derived class from parent circullar buffer.
 *  Provide API's to use Circullar buffer.
 * 
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef MEMORYSTREAM_HPP
#define MEMORYSTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "decoders/common/api/common.hpp"
#include "decoders/common/api/circularbuffer.hpp"

/*! \class MemoryStream
 *  \brief Derived class from parent CircullarBuffer.
 * 
 *  All methods supported from CircullarBuffer class will do the same here.
*/
class MemoryStream : public CircularBuffer
{
public:
    /*! A Constructor
	 *  \brief  Created default Circuallar buffer class object. 
	 *  It will create circuallr buffer with size(uiBufferSize) provided
	 *  
	 * \param [in] uiBufferSize - Length of circullar buffer to be created.
	 */ 
	MemoryStream(UINT uiBufferSize);

    /*! A Constructor
	 *  \brief  Creates circullar buffer and append buffer content provided.
	 * 
	 *  \param [in] pucBuffer Data Buffer.
	 *  \param [in] uiContentSize Length of data buffer.
	 * 
	 *  \remark If Circullar buffer length is lesser than provided uiContentSize, 
	 *   then circullar buffer will auto enlarge to created reuested size before append.
	 */ 	
	MemoryStream(UCHAR* pucBuffer, UINT uiContentSize);

    /*! A Constructor
	 *  \brief  Creates default MemoryStream object. Inside, Calls default CircullarBuffer class object.
	 * 
	 */ 	
    MemoryStream();

	/*! Default destructor */
	~MemoryStream();

    /*! \fn INT Available()
	 *  \brief Returns the actual content size, this is not the memory allocated size
	 *  The size increases when we write and it decreases when read.
	 * 
	 *  \return Available data in buffer to decode.
	 */ 
	INT Available();

    /*! \fn void Flush()
	 *  \brief Clear the data in the buffer.
	 * 
	 */ 
	void Flush();

    /*! \fn INStreamReadStatusT Read(CHAR* pucBuffer, UINT uiSize)
	 *  \brief Reads the required amount(uiSize) from buffer into pucBuffer
	 * 
	 *  \return StreamReadStatus structure with read statistics.
	 */ 
	StreamReadStatus Read(CHAR* pucBuffer, UINT uiSize);

    /*! \fn UINT Read(void)
	 *  \brief Read the one byte from the buffer
	 * 
	 *  \return Byte read from buffer.
	 *  \remark update StreamReadStatus structure with read statistics.
	 */ 
    UINT Read(void);

    /*! \fn UINT Write(UCHAR)
	 *  \brief Write one byte or character to buffer and update read statistics(StreamReadStatus)
	 * 
	 */ 
    UINT Write(UCHAR);

    /*! \fn UINT Write(UCHAR* pucBuffer, UINT uisize)
	 *  \brief Write provided buffer of dezired length.
	 *  \return Returns length of bytes append/write.
	 */ 	
	UINT Write(UCHAR* pucBuffer, UINT uisize);

    /*! \fn UINT CalculatePercentage(UINT uipercentage)
	 *  \brief Calculates the percentage of current Memory(Circuallr Buffer) read.
	 *  \return percentage of read compare to total buffer.
	 */ 	
	UINT CalculatePercentage(UINT);

    /*! For testing MemoryStream class private methods */ 
	friend class MemoryStreamTest;
    /*! For testing MemoryStream class private methods */ 
	friend class IOMemoryStreamTest;

private:
	/*! Private Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class. 
	 */ 
	MemoryStream(const MemoryStream& clOther);
	
	/*! Private assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */ 
	const MemoryStream& operator= (const MemoryStream& clOther);
	
	/*! StreamReadStatus Enumarated value. */
	StreamReadStatus stMemoryReadStatus;
};

#endif
