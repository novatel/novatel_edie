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
//! \file circularbuffer.cpp
//! \brief Circular buffer class to hold data read from file or port
//! used by parser
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "circularbuffer.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "nexcept.h"

//---------------------------------------------------------------------------
void CircularBuffer::SetCapacity(uint32_t uiCapacity_)
{
    // Set the size of the buffer (bytes)
    if (uiCapacity_ <= uiMyCapacity) return;

    const auto pucBuffer = new unsigned char[uiCapacity_];

    // Do nothing if new failed.... just use existing buffer
    if (pucBuffer != nullptr)
    {
        memset(pucBuffer, '*', uiCapacity_);

        // Copy the data from our old buffer to the new one
        Copy(pucBuffer, uiMyLength);

        // Free the old buffer and take ownership of the new buffer
        delete[] pucMyBuffer;
        pucMyBuffer = pucBuffer;

        // Update our pointers to point into the new buffer
        pucMyHead = pucMyBuffer;
        pucMyTail = pucMyHead + uiMyLength;
        uiMyCapacity = uiCapacity_;
    }

    // Sanity check
    if (pucMyBuffer == nullptr) throw nExcept("Out of memory!");
}

//---------------------------------------------------------------------------
uint32_t CircularBuffer::Append(const unsigned char* pucData_, uint32_t uiBytes_)
{
    // Enlarge buffer if necessary
    if (uiMyLength + uiBytes_ > uiMyCapacity)
    {
        // To prevent having to allocate memory excessively, we'll allocate
        // a little more than we really need
        constexpr uint32_t uiExtraRoom = 512;
        SetCapacity(uiMyLength + uiBytes_ + uiExtraRoom);

        // If we couldn't enlarge the buffer, reduce the number of bytes
        // that will be copied to (prevent overwriting data)
        if (uiBytes_ > uiMyCapacity - uiMyLength) uiBytes_ = uiMyCapacity - uiMyLength;
    }

    // Append data to buffer.  Do this in 2 steps, in case of wrap around.
    // Don't need to worry about overwriting data, because we enlarged the
    // buffer to guarantee a fit.
    auto uiCopyBytes = std::min(static_cast<uint32_t>(pucMyBuffer + uiMyCapacity - pucMyTail), uiBytes_);

    memcpy(pucMyTail, pucData_, uiCopyBytes);

    // Update housekeeping
    uiMyLength += uiCopyBytes;
    pucMyTail += uiCopyBytes;
    pucData_ += uiCopyBytes;

    // Copy any remaining data.  No need to check for overwriting data because
    // uiBytes_ was adjusted as necessary in the 'Enlarge buffer' block above
    uiCopyBytes = uiBytes_ - uiCopyBytes;
    if (uiCopyBytes > 0)
    {
        // Could only have got here if wraparound occurred.
        assert(pucMyTail >= pucMyBuffer + uiMyCapacity);
        pucMyTail = pucMyBuffer;

        memcpy(pucMyTail, pucData_, uiCopyBytes);
        uiMyLength += uiCopyBytes;
        pucMyTail += uiCopyBytes;
    }

    return uiBytes_; // Number of bytes actually copied
}

//---------------------------------------------------------------------------
unsigned char CircularBuffer::GetByte(int32_t iIndex_) const
{
    // Return byte at iIndex_ (throw exception if iIndex_ out of bounds)
    if (iIndex_ < 0 || static_cast<uint32_t>(iIndex_) >= uiMyLength)
    {
        // throw nExcept("CircularBuffer: Attempt to index out of bounds");
        return '\0';
    }

    unsigned const char* pucChar = pucMyHead + iIndex_;
    if (pucChar >= pucMyBuffer + uiMyCapacity) pucChar -= uiMyCapacity;

    return *pucChar;
}

//---------------------------------------------------------------------------
uint32_t CircularBuffer::Copy(unsigned char* pucTarget_, uint32_t uiBytes_) const
{
    // Don't copy more than we have
    uiBytes_ = std::min(uiBytes_, uiMyLength);

    if (uiBytes_ > 0)
    {
        // Copy data from our buffer to the target buffer, beginning
        // at the logical beginning of our buffer.  We do this in two
        // steps, in case of wraparound.
        auto uiCopyBytes = std::min(static_cast<uint32_t>(pucMyBuffer + uiMyCapacity - pucMyHead), uiBytes_);

        memcpy(pucTarget_, pucMyHead, uiCopyBytes);
        pucTarget_ += uiCopyBytes;

        // Now copy the rest of the requested data
        uiCopyBytes = uiBytes_ - uiCopyBytes;
        if (uiCopyBytes > 0) memcpy(pucTarget_, pucMyBuffer, uiCopyBytes);
    }

    return uiBytes_;
}

//---------------------------------------------------------------------------
void CircularBuffer::Discard(uint32_t uiBytes_)
{
    // Can't remove more data than is currently in buffer
    uiBytes_ = std::min(uiBytes_, uiMyLength);

    // Adjust head pointer by uiBytes_
    pucMyHead += uiBytes_;
    uiMyLength -= uiBytes_;

    // Check for wraparound
    if (pucMyHead >= pucMyBuffer + uiMyCapacity) pucMyHead -= uiMyCapacity;
}
