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

/*! \file outputmemorystream.hpp
 *  \brief Class is an interace to OutputMemoryStream class.]
 *  \sa OutputMemoryStream
 * 
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef OUTPUTMEMORYSTREAM_HPP
#define OUTPUTMEMORYSTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "outputstreaminterface.hpp"
#include "memorystream.hpp"

/*! \class OutputMemoryStream
 *  \brief A derived class from parent class OutputStreamInterface.
 *  \sa OutputStreamInterface
 * 
 *  More detailed FileStream class description. 
*/
class OutputMemoryStream : public OutputStreamInterface
{
public:
  /*! A Constructor.
   * \brief Default Constructor. Will create MemoryStream object.
   * \sa MemoryStream
   * \remark Created circullar buffer inside it to write data into it.
   */ 
   OutputMemoryStream();
  /*! A Constructor.
   * \brief Created MemoryStream object with desired length of buffer.
   * \sa MemoryStream
   * \param [in] uiBufferSize Desired buffer size
   * \remark Created circullar buffer with length provided to write data into it.
   * Default size is 1024 bytes.
   */  
   OutputMemoryStream(UINT uiBufferSize);
  
  /*! A Constructor.
   * \brief Created MemoryStream object with desired length of buffer and append provided data to it.
   * \sa MemoryStream
   * \param [in] pucBuffer   Buffer to be append to circullar buffer
   * \param [in] uiBufferSize Desired length of buffer to be append circullar buffer
   */     
   OutputMemoryStream(UCHAR* pucBuffer, UINT uiBufferSize);

   /*! A virtual destructor
    * \brief Deletes Creaged MemoryStream Obejct to which data has been written.
    */ 
   virtual ~OutputMemoryStream();

   /*! \fn UINT WriteData(BaseMessageData&)
    *  \brief Write data to the buffer from BaseMessageData
    *  \param [in] pclBaseMessageData BaseMessageData objec.
    *  \sa BaseMessageData
    *  \return Number of bytes written to output buffer.
    */ 
   UINT WriteData(BaseMessageData& pclBaseMessageData);

   /*! \fn MemoryStream* GetMemoryStream()
    * \sa MemoryStream
    *  \return MemoryStream Obejct which had buffer.
    */    
   MemoryStream* GetMemoryStream() {return pMyOutMemoryStream;}

private:
	/*! Private Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class. 
	 */ 
   OutputMemoryStream(const OutputMemoryStream& clTemp);

	/*! Private assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */ 
   const OutputMemoryStream& operator= (const OutputMemoryStream& clTemp);

   /*! var pMyOutMemoryStream
    * \brief MemoryStream Object pointer
    * \sa MemoryStream
    */ 
   MemoryStream* pMyOutMemoryStream;
};

#endif
