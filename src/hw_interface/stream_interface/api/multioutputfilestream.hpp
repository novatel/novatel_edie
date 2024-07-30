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
// ! \file multioutputfilestream.hpp
// ===============================================================================

#ifndef MULTI_OUTPUT_FILE_STREAM_HPP
#define MULTI_OUTPUT_FILE_STREAM_HPP

#include <map>
#include <string>

#include "decoders/common/api/common.hpp"
#include "decoders/novatel/api/common.hpp"
#include "filestream.hpp"
#include "outputstreaminterface.hpp"

/*! \def MIN_TIME_SPLIT_SEC
 *  \brief Minimum split time in seconds.
 *
 */
constexpr uint32_t MIN_TIME_SPLIT_SEC = 36;

/*! \def MIN_FILE_SPLIT_SIZE
 *  \brief Minimum splitting size of file in mb.
 *
 */
constexpr uint32_t MIN_FILE_SPLIT_SIZE = 1;

/*! \def HR_TO_SEC
 *  \brief Number of seconds in a Hour.
 *
 */
constexpr uint32_t HR_TO_SEC = 3600;

/*! \def MBYTE_TO_BYTE
 *  \brief Number of bytes in one megabyte?.
 *
 */
constexpr uint32_t MBYTE_TO_BYTE = 1024 * 1024;

/*! \class MultiOutputFileStream
 *  \brief A class will provide APIs for writing decoded output into multiple files.
 *
 *  Derived from parent OutputStreamInterface class.
 */
class MultiOutputFileStream : public OutputStreamInterface
{
  public:
    /*! A Constructor
     *  \brief  Default class constructor.
     *
     *  \remark All private members default to 0.
     */
    MultiOutputFileStream() = default;

    /*! Disabled Copy Constructor
     *
     *  A copy constructor is a member function which initializes an object using another object of
     * the same class.
     */
    MultiOutputFileStream(const MultiOutputFileStream& clTemp_) = delete;

    /*! Disabled assignment operator
     *
     *  The copy assignment operator is called whenever selected by overload resolution,
     *  e.g. when an object appears on the left side of an assignment expression.
     */
    const MultiOutputFileStream& operator=(const MultiOutputFileStream& clTemp_) = delete;

    /*! A virtual Destructor
     *  \brief  Class destructor.
     *
     *  \remark Will delete dynamically created objects in constructor
     */
    ~MultiOutputFileStream() override;

    /*! \brief Sets the output file in which to be decoded output will be written.
     *  \param[in] s32FileName_ - Name of the File to be created.
     *
     *  \remark FileStream Object will be created and added to map.
     *  If already created, The file with name stFileName will be set for writing.
     */
    void SelectFileStream(const std::u32string& s32FileName_) override;

    /*! \brief Delete all the output files associated Wide Character FileStream objects.
     *  \remark Clears the map in which all the output file stream objects will be saved.
     */

    void ClearWCFileStreamMap();

    /*! \brief Gets base file name with wide characters and extension of it.
     *  \param[in] s32FileName_ - Name of the File.
     *  \remark Sets Base file name (before '.' in file name)
     *          Sets Extension of the file.
     */
    void ConfigureBaseFileName(const std::u32string& s32FileName_) override;

    /*! \brief Sets the output file name  from the log name from strMsgName_.
     *  \param[in] strMsgName_
     *  \remark Sets output file name like "base_name_log_name".
     */
    void SelectWCLogFile(std::string strMsgName_);

    /*! \brief Sets the output file name(with Wide Characters) included with the split size
     *  \param[in] uiSize_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectWCSizeFile(uint32_t uiSize_);

    /*! \brief Sets the number of output files can be created based the time provided
     *  \param[in] eStatus_
     *  \param[in] usWeek_
     *  \param[in] dMilliseconds_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectWCTimeFile(novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_);

    /*! \brief Writes data to the file stream.
     *  \param[in] pcData_
     *  \param[in] uiDataLength_
     *  \param[in] strMsgName_
     *  \param[in] uiSize_
     *  \param[in] eStatus_
     *  \param[in] usWeek_
     *  \param[in] dMilliseconds_
     *  \return Number of bytes written to output file.
     *  \remark Set Split type and write data to output files. If split type was not set,
     *  Then writing can be done to only one file.
     */
    uint32_t WriteData(const char* pcData_, uint32_t uiDataLength_, const std::string& strMsgName_, uint32_t uiSize_,
                       novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_);

    /*! \brief Write Buffer to output file.
     *  \param[in] pcData_ pointer to buffer to be written to output file
     *  \param[in] uiDataLength_ Length of the buffer to be written to output file
     *  \return Number of bytes written to output file.
     *  \remark Set Split type and write data to output files. If split type was not set,
     *  Then writing can be done to only one file.
     */
    uint32_t WriteData(const char* pcData_, uint32_t uiDataLength_) override;

    /*! \brief Sets the output file in which to be decoded output will be written.
     *  \param[in] stFileName - Name of the File to be created.
     *
     *  \remark FileStream Object will be created and added to map.
     *  If already created, The file with name stFileName will be set for writing.
     */
    void SelectFileStream(const std::string& stFileName) override;

    /*! \brief Gets base file name and extension of it.
     *  \param[in] stFileName - Name of the File.
     *  \remark Sets Base file name (before '.' in file name)
     *          Sets Extension of the file.
     */
    void ConfigureBaseFileName(const std::string& stFileName) override;

    /*! \brief Delete all the output files associated FileStream objects.
     *  \remark Clears the map in which all the output file stream objects will be saved.
     */
    void ClearFileStreamMap();

    /*! \brief Enable/Disable Splitting of logs into different output files.
     *  \param[in] bStatus
     *  \remark Enable/Disable log Splitting. If enabled, split type will be set to SPLIT_LOG
     *  If disabled split type will be set to SPLIT_NONE
     */
    void ConfigureSplitByLog(bool bStatus) override;

    /*! \brief Sets the output file name  from the log name from strMsgName_.
     *  \param[in] strMsgName_
     *  \remark Sets output file name like "base_name_log_name".
     */
    void SelectLogFile(const std::string& strMsgName_);

    /*! \brief Split file into different output file with defined size.
     *  \param[in] ullFileSplitSize
     *  \remark Output files with ullFileSplitSize size will be created while writing to the output.
     */
    void ConfigureSplitBySize(uint64_t ullFileSplitSize) override;

    /*! \brief Sets the output file name included with the split size
     *  \param[in] uiSize_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectSizeFile(uint32_t uiSize_);

    /*! \brief Sets the interval of time the file to be split.
     *  \param[in] dFileSplitTime
     *  \remark Different output files will be created with the logs,
     *  in which will be captured in the time interval provided.
     */
    void ConfigureSplitByTime(double dFileSplitTime) override;

    /*! \brief Sets the number of output files can be created based the time provided
     *  \param[in] eStatus_
     *  \param[in] usWeek_
     *  \param[in] dMilliseconds_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectTimeFile(novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_);

    /*! \brief Gets the output file map
     *  \return Map with filename and FileStream Struct as key-value pair
     */
    std::map<std::string, FileStream*> GetFileMap() { return mMyFstreamMap; }

    /*! \brief Gets the output file map
     *  \return Map with filename and FileStream Struct as key-value pair
     */
    std::map<std::u32string, FileStream*> Get32FileMap() { return wmMyFstreamMap; }

    /*! \brief Sets the extension name of the output file
     *  \param[in] strExt std::string - Output file name
     */
    void SetExtensionName(const std::string& strExt) { stMyExtensionName = strExt; }
    void SetExtensionName(const std::u32string& strExt) { s32MyExtensionName = strExt; }

    /*! Friend class to test private methods. */
    friend class MultiOutputFileStreamTest;

  private:
    /*! FileStream class object pointer
     */
    FileStream* pLocalFileStream{nullptr};

    /*! An enumeration variable
     */
    FileSplitMethodEnum eMyFileSplitMethodEnum{FileSplitMethodEnum::SPLIT_NONE};

    /*! \remark Values can be true or false.
     *
     *  Boolean variable to enable/disable split mechanism.
     */
    bool bMyFileSplit{false};

    /*! Number of output files created.
     */
    uint32_t uiMyFileCount{0};

    /*! Time interval to be used for splitting of file.
     */
    double dMyTimeSplitSize{0.0};

    /*! Time interval to be used for calculating start/end time of logs to be written to output.
     */
    double dMyTimeInSeconds{0.0};

    /*! Start time of the Logs to be written to the output.
     */
    double dMyStartTimeInSeconds{0.0};

    /*! Start GPS Week of the Logs to be written to the output.
     */
    uint32_t ulMyStartWeek{0UL};

    /*! Start GPS Week of the Logs to be written to the output.
     */
    uint32_t ulMyWeek{0UL};

    /*! Wide Character Base name of the output file */
    std::u32string s32MyBaseName{U"DefaultBase"};
    /*! Wide Character extension name of the output file */
    std::u32string s32MyExtensionName{U"DefaultExt"};

    /*! std::map to save output file name with wide character and associated FileStream class object. */
    using WCFstreamMap = std::map<std::u32string, FileStream*>;
    /*! Wide Character file Map variable */
    WCFstreamMap wmMyFstreamMap;
    /*! Enable or Disable Wide Character support of file names.*/
#ifdef WIDE_CHAR_SUPPORT
    bool bEnableWideCharSupport{true};
#else
    bool bEnableWideCharSupport{false};
#endif
    /*! Base name of the output file */
    std::string stMyBaseName{"DefaultBase"};
    /*! Extension name of the output file */
    std::string stMyExtensionName{"DefaultExt"};
    /*! std::map to save output file name and associated FileStream class object. */
    using FstreamMap = std::map<std::string, FileStream*>;
    /*! Map variable */
    FstreamMap mMyFstreamMap;
    /*! The split output file size */
    uint64_t ullMyFileSplitSize{0ULL};
    /*! Total File size */
    uint64_t ullMyFileSize{0ULL};
};

#endif
