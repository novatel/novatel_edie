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

#ifndef NOVATEL_EDIE_STREAM_INTERFACE_COMMON_HPP
#define NOVATEL_EDIE_STREAM_INTERFACE_COMMON_HPP

/*! An Enum.
 *
 * Splitting type of log enumeration while writing to different output files.
 */
typedef enum
{
    SPLIT_SIZE, /*!< Split based on Size of log */
    SPLIT_LOG,  /*!< Split based on Log name */
    SPLIT_TIME, /*!< Split based on time of log */
    SPLIT_NONE  /*!< Do not split */
} FileSplitMethodEnum;

/*! A Structure
 *
 * Hold/copy decoded log and size of it.
 */
struct ReadDataStructure
{
    uint32_t uiDataSize; /*!< Size of decoded log */
    char* cData;         /*!< Memory pointer*/

    /*! Default Initializer */
    ReadDataStructure()
    {
        uiDataSize = 0;
        cData = nullptr;
    }

    ReadDataStructure(uint32_t uiDataSize, char* cData) : uiDataSize(uiDataSize), cData(cData) {}
};

/*! A Structure
 *
 *  Provides decoder read statistics.
 */
struct StreamReadStatus
{
    uint32_t uiPercentStreamRead{0}; /*!< % of amount of data read from file */
    uint32_t uiCurrentStreamRead{0}; /*!< Number of bytes read for one Read */
    uint64_t ullStreamLength{0};     /*!< Length of the bytes of decoded log */
    bool bEOS{false};                /*!< IS EOF of file reached or not? */

    /*! Default Initializer */
    constexpr StreamReadStatus() = default;
};

#endif
