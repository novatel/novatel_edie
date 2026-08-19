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
// ! \file message_counts_tracker_unit_test.cpp
// ===============================================================================

#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>

#include <gtest/gtest.h>

#include "novatel_edie/decoders/common/message_counts_tracker.hpp"

using namespace novatel::edie;

// -------------------------------------------------------------------------------------------------------
// A single integer key. This is the simplest way a decoder can identify a message.
// -------------------------------------------------------------------------------------------------------
using IdTracker = MessageCountsTracker<uint16_t>;

// -------------------------------------------------------------------------------------------------------
// A tuple key with a custom hash. This is how the OEM header decoder identifies a message.
// -------------------------------------------------------------------------------------------------------
using TupleKey = std::tuple<uint8_t, uint16_t, uint8_t>;

struct TupleKeyHash
{
    std::size_t operator()(const TupleKey& key_) const
    {
        return (static_cast<std::size_t>(std::get<0>(key_)) << 24) | (static_cast<std::size_t>(std::get<1>(key_)) << 8) | std::get<2>(key_);
    }
};

using TupleTracker = MessageCountsTracker<TupleKey, TupleKeyHash>;

TEST(MessageCountsTrackerTest, NEW_TRACKER_IS_EMPTY)
{
    const IdTracker clTracker;
    ASSERT_TRUE(clTracker.GetCounts().empty());
}

TEST(MessageCountsTrackerTest, INCREMENT_COUNTS_EACH_KEY_SEPARATELY)
{
    IdTracker clTracker;
    clTracker.Increment(42);
    clTracker.Increment(42);
    clTracker.Increment(42);
    clTracker.Increment(43);

    const auto& mapCounts = clTracker.GetCounts();
    ASSERT_EQ(mapCounts.size(), 2U);
    ASSERT_EQ(mapCounts.at(42), 3U);
    ASSERT_EQ(mapCounts.at(43), 1U);
    ASSERT_EQ(mapCounts.count(44), 0U); // A key that was never seen is absent, not 0.
}

TEST(MessageCountsTrackerTest, RESET_CLEARS_ALL_COUNTS)
{
    IdTracker clTracker;
    clTracker.Increment(42);
    clTracker.Increment(43);
    ASSERT_EQ(clTracker.GetCounts().size(), 2U);

    clTracker.Reset();
    ASSERT_TRUE(clTracker.GetCounts().empty());

    // The tracker still counts after a reset, and it starts again from 0.
    clTracker.Increment(42);
    ASSERT_EQ(clTracker.GetCounts().at(42), 1U);
}

TEST(MessageCountsTrackerTest, TUPLE_KEY_DISTINGUISHES_EVERY_FIELD)
{
    TupleTracker clTracker;
    clTracker.Increment({2, 42, 0}); // format 2, message 42, sibling 0
    clTracker.Increment({2, 42, 0});
    clTracker.Increment({4, 42, 0}); // Same message in another format.
    clTracker.Increment({2, 43, 0}); // Another message in the same format.
    clTracker.Increment({2, 42, 1}); // Same message and format, other sibling.

    const auto& mapCounts = clTracker.GetCounts();
    ASSERT_EQ(mapCounts.size(), 4U);
    ASSERT_EQ(mapCounts.at({2, 42, 0}), 2U);
    ASSERT_EQ(mapCounts.at({4, 42, 0}), 1U);
    ASSERT_EQ(mapCounts.at({2, 43, 0}), 1U);
    ASSERT_EQ(mapCounts.at({2, 42, 1}), 1U);
}

TEST(MessageCountsTrackerTest, GET_COUNTS_RETURNS_A_CONST_REFERENCE)
{
    IdTracker clTracker;
    static_assert(std::is_same_v<decltype(clTracker.GetCounts()), const IdTracker::CountsMap&>,
                  "GetCounts() must return a const reference, so that callers cannot modify the counts and do not copy the map.");

    // The reference stays valid, and it shows the counts that arrive after the call.
    const auto& mapCounts = clTracker.GetCounts();
    clTracker.Increment(42);
    ASSERT_EQ(mapCounts.at(42), 1U);
}

TEST(MessageCountsTrackerTest, COUNTS_DO_NOT_OVERFLOW_A_32_BIT_TYPE)
{
    MessageCountsTracker<std::string> clTracker;
    static_assert(std::is_same_v<MessageCountsTracker<std::string>::CountsMap::mapped_type, uint64_t>, "Counts must be 64 bits wide.");
    ASSERT_TRUE(clTracker.GetCounts().empty());
}
