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
// Common miscellaneous defines
//-----------------------------------------------------------------------
constexpr uint32_t SEC_TO_MILLI_SEC = 1000; //!< A Macro definition for number of milliseconds in a second.
constexpr uint32_t SECS_IN_WEEK = 604800;   //!< A Macro definition for number of milliseconds in a week.

} // namespace novatel::edie

#endif
