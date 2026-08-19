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
// ! \file message_counts_tracker.hpp
// ===============================================================================

#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace novatel::edie {

//============================================================================
//! \class MessageCountsTracker
//! \brief Count how many times a decoder decodes each message.
//!
//! \details This class maps a decoder-specific key to a count. Each decoder
//! selects its own \a Key type and the \a Hash functor for that key. Decoders
//! that identify messages in different ways can then use the same counting
//! logic. For example, the OEM header decoder keys on the format, the message
//! ID and the sibling ID. Other decoders can key on the message ID only.
//!
//! This class is not thread-safe. Do not increment the counts of one tracker
//! from more than one thread.
//!
//! \tparam Key  The key that uniquely identifies a message to count.
//! \tparam Hash The hash functor that stores \a Key in the internal map.
//============================================================================
template <typename Key, typename Hash = std::hash<Key>> class MessageCountsTracker
{
  public:
    //! \brief Type alias for the underlying counts map.
    using CountsMap = std::unordered_map<Key, uint64_t, Hash>;

    //----------------------------------------------------------------------------
    //! \brief Increment the count of the given key. The count starts at 0.
    //
    //! \param[in] key_ The key of the count to increment.
    //----------------------------------------------------------------------------
    void Increment(const Key& key_) { mapMyCounts[key_]++; }

    //----------------------------------------------------------------------------
    //! \brief Get the map of message counts. Keys with a count of 0 are absent.
    //
    //! \return A const reference to the map of counts. The caller cannot modify
    //! the returned counts.
    //----------------------------------------------------------------------------
    [[nodiscard]] const CountsMap& GetCounts() const { return mapMyCounts; }

    //----------------------------------------------------------------------------
    //! \brief Clear all tracked message counts.
    //----------------------------------------------------------------------------
    void Reset() { mapMyCounts.clear(); }

  private:
    CountsMap mapMyCounts{};
};

} // namespace novatel::edie
