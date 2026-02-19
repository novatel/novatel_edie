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
// ! \file fixed_buffer.hpp
// ===============================================================================

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <numeric>
#include <type_traits>

#include "novatel_edie/common/crc32.hpp"

//============================================================================
//! \class FixedBuffer
//! \brief A minimal, fixed-size linear buffer. Data is stored contiguously
//! and consumed from the front. When a write would exceed the buffer end,
//! unconsumed data is shifted back to the beginning before writing. A FixedBuffer
//! of logical capacity N uses an underlying array of size 2*N to mitigate the
//! worst-case impact of shifting.
//! \tparam T The type of elements stored in the buffer. Must be trivially copyable.
//! \tparam N The fixed logical capacity.
//============================================================================
template <typename T, size_t N> class FixedBuffer
{
    static_assert(N > 0, "FixedBuffer capacity N must be positive");
    static_assert(std::is_trivially_copyable_v<T>, "FixedBuffer requires a trivially copyable type T for memcpy usage");

  public:
    //! \brief A special value indicating "not found" in search operations.
    static constexpr size_t npos = static_cast<size_t>(-1);

    //! \brief Adds an element to the end of the buffer.
    void push_back(const T& value) noexcept { buffer[head + sz++] = value; }

    //! \brief Accesses the element at the specified logical index (0 = oldest). Const version.
    [[nodiscard]] const T& operator[](size_t i) const noexcept { return buffer[head + i]; }

    //! \brief Accesses the element at the specified logical index (0 = oldest).
    [[nodiscard]] T& operator[](size_t i) noexcept { return buffer[head + i]; }

    //! \brief Removes elements from the beginning (oldest elements).
    void erase_begin(size_t count) noexcept
    {
        count = std::min(count, sz);
        if (count == 0) { return; }

        sz -= count;
        head = sz == 0 ? 0 : head + count;
    }

    //! \brief Copies data starting from the beginning (oldest element) to a destination buffer.
    //! \param[out] destination Pointer to the destination buffer. Must be large enough (count * sizeof(T) bytes).
    //! \param[in] count The maximum number of *elements* (not bytes) to copy.
    void copy_out(T* destination, size_t count) const noexcept
    {
        if (destination == nullptr || count == 0 || sz == 0) { return; }

        count = std::min(count, sz);
        std::memcpy(destination, buffer.get() + head, count * sizeof(T));
    }

    //! \brief Writes a block of data to the end of the buffer if space is available.
    //! When the write would exceed the buffer end, unconsumed data is first
    //! shifted back to the beginning using memmove.
    //! \param[in] data_ptr Pointer to the source data buffer.
    //! \param[in] count The number of *elements* (of type T) to write.
    //! \return The number of elements written, or 0 if there is not enough total space.
    [[nodiscard]] size_t Write(const T* data_ptr, size_t count) noexcept
    {
        if (data_ptr == nullptr || count > available_space() || count == 0) { return 0; }

        // If the write would go past the end of the buffer, compact unconsumed data to the front.
        if (head + sz + count > 2 * N)
        {
            // The available_space() check above guarantees that count <= N - sz, so sz + count <= N.
            // Therefore, head + sz + count > 2*N implies head > N, so we can safely use
            // memcpy rather than memmove here.
            std::memcpy(buffer.get(), buffer.get() + head, sz * sizeof(T));
            head = 0;
        }

        std::memcpy(buffer.get() + head + sz, data_ptr, count * sizeof(T));
        sz += count;
        return count;
    }

    //! \brief Finds the first occurrence of a byte value in the buffer (logical order).
    //! \return Logical index (0 = oldest) or npos if not found.
    template <typename U = T, std::enable_if_t<std::is_same_v<U, unsigned char>, int> = 0>
    [[nodiscard]] size_t search_char(unsigned char value, size_t start, size_t count = npos) const noexcept
    {
        if (start >= sz) { return npos; }
        count = std::min(count, sz - start);
        if (count == 0) { return npos; }

        const void* ptr = std::memchr(buffer.get() + head + start, value, count);
        if (ptr != nullptr) { return static_cast<size_t>(static_cast<const unsigned char*>(ptr) - (buffer.get() + head + start)) + start; }

        return npos;
    }

    //! \brief Accumulates a range of elements in logical order (0 = oldest).
    //! \param[in] start Logical start index.
    //! \param[in] count Number of elements to include.
    //! \param[in] init Initial value for accumulation.
    template <typename BinaryOp, typename AccT, typename = std::enable_if_t<std::is_invocable_r_v<AccT, BinaryOp, AccT, T>>>
    [[nodiscard]] AccT accumulate(size_t start, size_t count, AccT init, BinaryOp op) const
    {
        if (start >= sz || count == 0) { return init; }
        count = std::min(count, sz - start);

        return std::accumulate(buffer.get() + head + start, buffer.get() + head + start + count, init, op);
    }

    //! \brief Reads a multi-byte value from the buffer.
    //! \tparam U The type to read (e.g., uint16_t, uint32_t). Must be trivially copyable.
    //! \param[in] logical_index The starting logical index of the value.
    //! \return The value read from the buffer.
    template <typename U, typename = std::enable_if_t<std::is_trivially_copyable_v<U>>>
    [[nodiscard]] U read_value(size_t logical_index) const noexcept
    {
        static_assert(sizeof(U) % sizeof(T) == 0, "Value size must be a multiple of element size");

        // Note: alignment requirement of U is not guaranteed, so we cannot simply cast to U
        U result;
        std::memcpy(&result, buffer.get() + head + logical_index, sizeof(U));
        return result;
    }

    //! \brief Calculates the CRC-32 of a range of elements in logical order (0 = oldest).
    //! \param[in] start Logical start index.
    //! \param[in] count Number of elements to include.
    [[nodiscard]] uint32_t CalculateBlockCrc32(size_t start, size_t count) const
    {
        return ::CalculateBlockCrc32(buffer.get() + head + start, static_cast<uint32_t>(std::min(count, sz - start) * sizeof(T)));
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
    std::unique_ptr<T[]> buffer{std::make_unique<T[]>(2 * N)};
    size_t head = 0;
    size_t sz = 0;
};

namespace novatel::edie {
//! \brief FixedBuffer specialization for unsigned char with 256KB capacity.
using UCharFixedBuffer = FixedBuffer<unsigned char, (1 << 18)>;
} // namespace novatel::edie
