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

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

/*! \file circularbuffer.hpp
 *  \brief Circular buffer class to hold data read from file or port used by parser
 *  \author Gopi R
 *  \date   FEB 2021
 *  Additional details can follow in subsequent paragraphs.
 */ 

//-----------------------------------------------------------------------
// Includes                                                               
//-----------------------------------------------------------------------
#include "env.hpp"
#include "nexcept.h"

/*! \brief Circular buffer class to hold data read from file or port used by parser
 *
 * This class had methods like Append/Discard and clear as well.
 * No need to expose to the Application/User.
 */ 
class CircularBuffer
{
private:

   UCHAR* pucMyBuffer;       /**< Data buffer (circular buffer) */
   UINT   uiMyCapacity;      /**< Capacity of data buffer (bytes) */
   UINT   uiMyLength;        /**< Amount of data currently in buffer (bytes) */
   UCHAR* pucMyHead;         /**< Logical beginning of buffer */
   UCHAR* pucMyTail;         /**< Logical tail of buffer (could be computed, but maintained as a convenience) */

public:
   /*! \brief default circular buffer class constructor.
    *
    *  Circular Buffer Head, Tail, Buffer will be intialized to NULL. 
    *  And Capicity of Buffer and current length of data will be set to 0.
    */
   CircularBuffer();        

   /*! \brief default circular buffer class Destructor.*/
   ~CircularBuffer();

   /*! \brief Sets the size of the circular buffer (bytes)
    *
    *  Dynamic memory will be created for the capicity provided.
    *  if new failed.... exception will be thrown.
    *
    *  \pre None
    *  \post None
    * 
    *  \param[in] uiCapacity_ Size of buffer (bytes).  
    * 
    *  \return None
    *  \remark If uiCapacity_, smaller than current capacity, no effect..
    */ 
   void SetCapacity (UINT uiCapacity_);          

   /*! Returns current capacity of buffer. */
   inline UINT GetCapacity();     

   /*! Returns number of bytes of data in buffer. Number of bytes between beggining of buffer and write cursor.*/
   inline UINT GetLength();       

   /*! Returns the current buffer.*/
   inline UCHAR* GetBuffer();     

   /*! \brief Append data to end of buffer.  Will increase buffer size if needed.
    *
    *  Will Enlarge the buffer if byte count is more than capacity.
    *  To prevent having to allocate memory excessively, will allocate a little 
    *  more than we really need.
    * 
    *  \pre None
    *  \post None
    * 
    *  \param[in] pucData_ CHAR buffer pointer from which data to append to the queue
    *  \param[in] uiBytes_  Size of data (in bytes) to append from above address
    * 
    *  \return UINT - Return number of bytes actually appended
    *  \remark If size of data is nore then capacity of buffer, heap will be created on time.
    */ 
   UINT Append(UCHAR* pucData_, UINT uiBytes_);

   /*! \brief Remove data from beginning of buffer.
    *
    *  It can't remove more data than is currently in buffer. 
    *  And will be wrapped after removing.
    * 
    *  \pre None
    *  \post Length of bytes will be removed
    * 
    *  \param[in] uiBytes_  Size of data (in bytes) to be rmoved from buffer
    * 
    *  \return None
    *  \remark If data is not avail in circular buffer nothing to be done here.
    */ 
   void Discard(UINT uiBytes_);         

   /*! Remove all data from buffer, Buffer will be clear after this call. */
   inline void Clear();      

   /*! \brief Copy buffer from circular buffer to target
    *
    *  Don't copy more than we have. Will copy what ever it has.
    * 
    *  \pre None
    *  \post None
    * 
    *  \param[in] pucTarget_  Destination of copy data from circular buffer
    *  \param[in] uiBytes_  Amount of data (in bytes) to copy
    * 
    *  \return Number of bytes copied to destination
    *  \remark If data is not avail in circular buffer, should return some value?
    */ 
   UINT Copy(UCHAR* pucTarget_, UINT uiBytes_);      

   /*! Overloading Subscript or array index operator [] */
   inline UCHAR operator[] (INT iIndex_) const;    

   /*! \brief Return copy of byte at iIndex_ (throw exception if iIndex_ out of bounds)
    *
    *  \param [in] iIndex_ integer value
    *  \return unsigned char value 
    */ 
   UCHAR GetByte(INT iIndex_) const;    
};

///////////////////////////////////////////////////////////////////////////////
// Implementation of inline functions
///////////////////////////////////////////////////////////////////////////////

/*! \brief Returns the current capacity of buffer.*/
inline UINT CircularBuffer::GetCapacity()
{  
   return uiMyCapacity;
}

/*! \brief Returns number of bytes of data in buffer.
 *
 * Number of bytes between beggining of buffer and write cursor.
 */ 
inline UINT CircularBuffer::GetLength()
{ 
   return uiMyLength;
}

/*! \brief Delete contents of buffer, Internally called Discard method. */
inline void CircularBuffer::Clear()
{ 
   Discard( uiMyLength );
   return;
}

/*! \brief Returns entire circulat buffer. */
inline UCHAR* CircularBuffer::GetBuffer()
{ 
   return pucMyBuffer;
}

/*! \brief Returns copy of byte at iIndex_ (throw exception if iIndex_ out of bounds)
 *
 *  \param [in] iIndex_ integer value
 *  \return unsigned chatacter
 */ 
inline UCHAR CircularBuffer::operator[] (INT iIndex_) const
{
   return GetByte( iIndex_ );
}

#endif // CIRCULARBUFFER_H
