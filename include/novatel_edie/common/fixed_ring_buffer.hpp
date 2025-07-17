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
// ! \file fixed_ring_buffer.hpp
// ===============================================================================

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <type_traits>

//============================================================================
//! \class FixedRingBuffer
//! \brief A minimal, fixed-size ring buffer optimized for power-of-2 capacity.
//! \tparam T The type of elements stored in the buffer. Must be trivially copyable.
//! \tparam N The fixed capacity, MUST be a power of 2.
//============================================================================
template <typename T, size_t N> class FixedRingBuffer
{
    static_assert(N > 0 && (N & (N - 1)) == 0, "FixedRingBuffer capacity N must be a power of 2");
    static_assert(std::is_trivially_copyable_v<T>, "FixedRingBuffer requires a trivially copyable type T for memcpy usage");

  public:
    //! \brief Adds an element to the end of the buffer, overwriting the oldest if full.
    void push_back(const T& value) noexcept { buffer[(head + sz++) & Mask] = value; }

    //! \brief Accesses the element at the specified logical index (0 = oldest). Const version.
    [[nodiscard]] const T& operator[](size_t i) const noexcept { return buffer[(head + i) & Mask]; }

    //! \brief Accesses the element at the specified logical index (0 = oldest).
    [[nodiscard]] T& operator[](size_t i) noexcept { return buffer[(head + i) & Mask]; }

    //! \brief Removes elements from the beginning (oldest elements).
    void erase_begin(size_t count) noexcept
    {
        count = std::min(count, sz);
        if (count == 0) { return; }

        head = (head + count) & Mask;
        sz -= count;
    }

    //! \brief Copies data starting from the beginning (oldest element) to a destination buffer.
    //! \param[out] destination Pointer to the destination buffer. Must be large enough (count * sizeof(T) bytes).
    //! \param[in] count The maximum number of *elements* (not bytes) to copy.
    //! \return The actual number of *elements* copied (min of count and buffer size).
    void copy_out(T* destination, size_t count) const noexcept
    {
        if (destination == nullptr || count == 0 || sz == 0) { return; }

        count = std::min(count, sz);

        const size_t first_chunk_elements = std::min(count, N - head);
        const size_t first_chunk_bytes = first_chunk_elements * sizeof(T);

        memcpy(destination, buffer.get() + head, first_chunk_bytes);

        if (count == first_chunk_elements) { return; }

        const size_t second_chunk_elements = count - first_chunk_elements;
        const size_t second_chunk_bytes = second_chunk_elements * sizeof(T);

        memcpy(reinterpret_cast<unsigned char*>(destination) + first_chunk_bytes, buffer.get(), second_chunk_bytes);
    }

    //! \brief Writes a block of data to the end of the buffer if space is available.
    //! \param[in] data_ptr Pointer to the source data buffer.
    //! \param[in] count The number of *elements* (of type T) to write.
    //! \return True if all elements were written (enough space), false otherwise.
    [[nodiscard]] size_t Write(const T* data_ptr, size_t count) noexcept
    {
        if (data_ptr == nullptr) { return 0; }
        count = std::min(count, available_space());
        if (count == 0) { return 0; }

        const size_t write_start_idx = (head + sz) & Mask;
        const size_t first_chunk_elements = std::min(count, N - write_start_idx);
        const size_t first_chunk_bytes = first_chunk_elements * sizeof(T);

        memcpy(buffer.get() + write_start_idx, data_ptr, first_chunk_bytes);

        if (count != first_chunk_elements)
        {
            const size_t second_chunk_elements = count - first_chunk_elements;
            const size_t second_chunk_bytes = second_chunk_elements * sizeof(T);

            memcpy(buffer.get(), reinterpret_cast<const unsigned char*>(data_ptr) + first_chunk_bytes, second_chunk_bytes);
        }

        sz += count;
        return count;
    }

    //! \brief Returns the number of elements currently in the buffer.
    [[nodiscard]] constexpr size_t size() const noexcept { return sz; }
    //! \brief Returns the maximum number of elements the buffer can hold.
    [[nodiscard]] constexpr size_t capacity() const noexcept { return N; }
    //! \brief Returns the number of elements that can be written without overwriting.
    [[nodiscard]] constexpr size_t available_space() const noexcept { return N - sz; }
    //! \brief Returns true if the buffer is empty.
    [[nodiscard]] constexpr bool empty() const noexcept { return sz == 0; }
    //! \brief Returns true if the buffer is full.
    [[nodiscard]] constexpr bool full() const noexcept { return sz == N; }

  private:
    static constexpr size_t Mask = N - 1;

    std::unique_ptr<T[]> buffer{std::make_unique<T[]>(N)};
    size_t head = 0;
    size_t sz = 0;
};
