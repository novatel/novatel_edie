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
// ! \file circular_buffer.hpp
// ===============================================================================

#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include <cstdint>
#include <memory>

//============================================================================
//! \class CircularBuffer
//! \brief Used for maintaining a continuous/rotating buffer of set capacity.
//============================================================================
class CircularBuffer
{
  private:
    std::unique_ptr<unsigned char[]> pucMyBuffer; //!< Data buffer (circular buffer)
    uint32_t uiMyCapacity{0};                     //!< Capacity of data buffer (bytes)
    uint32_t uiMyLength{0};                       //!< Amount of data currently in buffer (bytes)
    unsigned char* pucMyHead{nullptr};            //!< Logical beginning of buffer
    unsigned char* pucMyTail{nullptr};            //!< Logical tail of buffer (could be computed, but maintained as a convenience)

  public:
    //----------------------------------------------------------------------------
    //! \brief A constructor for the CircularBuffer class.
    //! \remark Circular Buffer Head, Tail, Buffer will be initialized to nullptr.
    //! And Capacity of Buffer and current length of data will be set to 0.
    //----------------------------------------------------------------------------
    CircularBuffer() = default;

    //----------------------------------------------------------------------------
    //! \brief Sets the size of the circular buffer (bytes)
    //
    //! \param[in] uiCapacity_ Size of buffer (bytes).
    //
    //! \remark If uiCapacity_, smaller than current capacity, no effect.
    //----------------------------------------------------------------------------
    void SetCapacity(uint32_t uiCapacity_);

    //----------------------------------------------------------------------------
    //! \return current capacity of buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] uint32_t GetCapacity() const { return uiMyCapacity; }

    //----------------------------------------------------------------------------
    //! \return number of bytes of data in buffer. Number of bytes between
    //! beginning of buffer and write cursor.
    //----------------------------------------------------------------------------
    [[nodiscard]] uint32_t GetLength() const { return uiMyLength; }

    //----------------------------------------------------------------------------
    //! \return the current buffer.
    //----------------------------------------------------------------------------
    [[nodiscard]] unsigned char* GetBuffer() const { return pucMyBuffer.get(); }

    //----------------------------------------------------------------------------
    //! \brief Append data to end of buffer. Will increase buffer size if needed.
    //
    //! \param[in] pucData_ unsigned char buffer pointer from which data to
    //! append to the queue.
    //! \param[in] uiBytes_ Size of data (in bytes) to append from above address.
    //
    //! \return Number of bytes actually appended
    //! \remark If size of data is more than the capacity of buffer, heap will be
    //! created on time.
    //----------------------------------------------------------------------------
    uint32_t Append(const unsigned char* pucData_, uint32_t uiBytes_);

    //----------------------------------------------------------------------------
    //! \brief Remove data from beginning of buffer.
    //
    //! \param[in] uiBytes_ Size of data (in bytes) to be removed from buffer.
    //
    //! \remark If data is not avail in circular buffer nothing to be done here.
    //----------------------------------------------------------------------------
    void Discard(uint32_t uiBytes_);

    //----------------------------------------------------------------------------
    //! \brief Remove all data from buffer, Buffer will be clear after this call.
    //----------------------------------------------------------------------------
    void Clear() { Discard(uiMyLength); }

    //----------------------------------------------------------------------------
    //! \brief Copy buffer from circular buffer to target
    //
    //! \param[in] pucTarget_ Destination of copy data from circular buffer
    //! \param[in] uiBytes_ Amount of data (in bytes) to copy
    //
    //! \return Number of bytes copied to destination
    //----------------------------------------------------------------------------
    uint32_t Copy(unsigned char* pucTarget_, uint32_t uiBytes_) const;

    //----------------------------------------------------------------------------
    //! \brief Overloading Subscript or array index operator []
    //----------------------------------------------------------------------------
    unsigned char operator[](int32_t iIndex_) const { return GetByte(iIndex_); }

    //----------------------------------------------------------------------------
    //! \brief Return copy of byte at iIndex_ (throw exception if iIndex_ out of
    //! bounds)
    //
    //! \param[in] iIndex_ integer value
    //
    //! \return The byte at the provided index.
    //----------------------------------------------------------------------------
    [[nodiscard]] unsigned char GetByte(int32_t iIndex_) const;
};

#endif // CIRCULAR_BUFFER_HPP
