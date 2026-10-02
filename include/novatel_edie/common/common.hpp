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
// ! \file common.hpp
// ===============================================================================

#ifndef COMMON_COMMON_HPP
#define COMMON_COMMON_HPP

#include <cassert>
#include <cstdint>

namespace novatel::edie {

//-----------------------------------------------------------------------
//! \enum TIME_STATUS
//! \brief Enumeration describing the time status on a NovAtel receiver
//! when a log is produced. See GPS Reference Time Status.
//-----------------------------------------------------------------------
enum class TIME_STATUS : uint8_t
{
    UNKNOWN = 20,             //!< Time validity is unknown.
    APPROXIMATE = 60,         //!< Time is set approximately.
    COARSEADJUSTING = 80,     //!< Time is approaching coarse precision.
    COARSE = 100,             //!< This time is valid to coarse precision.
    COARSESTEERING = 120,     //!< Time is coarse set and is being steered.
    FREEWHEELING = 130,       //!< Position is lost and the range bias cannot be calculated.
    FINEADJUSTING = 140,      //!< Time is adjusting to fine precision.
    FINE = 160,               //!< Time has fine precision.
    FINEBACKUPSTEERING = 170, //!< Time is fine set and is being steered by the backup system.
    FINESTEERING = 180,       //!< Time is fine set and is being steered.
    SATTIME = 200,            //!< Time from satellite. Only used in logs containing satellite data such as ephemeris and almanac.
    EXTERN = 220,             //!< Time source is external to the Receiver.
    EXACT = 240               //!< Time is exact.
};

//-----------------------------------------------------------------------
//! \brief Compare two double values.
//
//! \param[in] dVal1_ First double type Value.
//! \param[in] dVal2_ Second double type value.
//! \param[in] dEpsilon_ The tolerance with which to justify "equal".
//
//! \return Boolean Value - Returns both values are equal or not?
//-----------------------------------------------------------------------
bool IsEqual(double dVal1_, double dVal2_, double dEpsilon_ = 0.001);

//-----------------------------------------------------------------------
//! \brief Get the char as an integer.
//
//! \param[in] c_ The char to get as an integer.
//
//! \return The char as an integer.
//-----------------------------------------------------------------------
int32_t ToDigit(char c_);

//-----------------------------------------------------------------------
//! \struct BitMask
//! \brief A contiguous run of bits within a value, described by its offset
//! from bit 0 and its width. Provides helpers to build the mask and to
//! extract the field's value from a larger integer.
//-----------------------------------------------------------------------
struct BitMask
{
    uint8_t offset;
    uint8_t width;

    constexpr BitMask(uint8_t offset_, uint8_t width_) : offset(offset_), width(width_) { assert(width > 0 && offset + width <= 32); }

    // Constructs from an inclusive lower bound and exclusive upper bound, e.g. fromRange(4, 8) covers bits [4,8).
    static constexpr BitMask fromRange(uint8_t lower_, uint8_t upperExclusive_)
    {
        return BitMask(lower_, static_cast<uint8_t>(upperExclusive_ - lower_));
    }

    // Mask positioned in-place
    constexpr uint32_t mask() const { return lowMask() << offset; }

    // Mask at bit 0
    constexpr uint32_t lowMask() const { return width == 32 ? UINT32_MAX : (1u << width) - 1u; }

    constexpr bool operator==(const BitMask& other_) const { return offset == other_.offset && width == other_.width; }
    constexpr bool operator!=(const BitMask& other_) const { return !(*this == other_); }
};

//-----------------------------------------------------------------------
//! \brief Extract the value of a bitfield from a larger integer.
//
//! \param[in] value_ The integer to extract the field from.
//! \param[in] bitMask_ The bitmask describing the field's offset and width.
//
//! \return The field's value, shifted down to bit 0.
//-----------------------------------------------------------------------
template <typename T> constexpr uint32_t ExtractMaskedValue(T value_, const BitMask& bitMask_)
{
    return static_cast<uint32_t>(value_ >> bitMask_.offset) & bitMask_.lowMask();
}

//-----------------------------------------------------------------------
// Common miscellaneous defines
//-----------------------------------------------------------------------
constexpr uint32_t SEC_TO_MILLI_SEC = 1000; //!< A Macro definition for number of milliseconds in a second.
constexpr uint32_t SECS_IN_WEEK = 604800;   //!< A Macro definition for number of milliseconds in a week.

} // namespace novatel::edie

#endif
