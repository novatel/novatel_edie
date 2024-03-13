////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020 NovAtel Inc.
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
////////////////////////////////////////////////////////////////////////////////

/*! \file multioutputfilestream.hpp
 *  \brief
 *  Created to satisfy the need of maintaining multiple output files at a time. It will mainly be
 * used for file split functionalities. It also provides an API called SelectFileStream to select
 * the active file stream based on passed filename key.
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef MULTIOUTPUTFILESTREAM_HPP
#define MULTIOUTPUTFILESTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <map>
#include <string>

#include "decoders/common/api/common.hpp"
#include "decoders/common/api/nexcept.h"
#include "decoders/novatel/api/common.hpp"
#include "filestream.hpp"
#include "outputstreaminterface.hpp"

/*! \def MIN_TIME_SPLIT_SEC
 *  \brief Minimum split time in seconds.
 *
 */
#define MIN_TIME_SPLIT_SEC 36

/*! \def MIN_FILE_SPLIT_SIZE
 *  \brief Minimum splitting size of file in mb.
 *
 */
#define MIN_FILE_SPLIT_SIZE 1

/*! \def HR_TO_SEC
 *  \brief Number of seconds in a Hour.
 *
 */
#define HR_TO_SEC 3600

/*! \def MBYTE_TO_BYTE
 *  \brief Number of bytes in one mega byte?.
 *
 */
#define MBYTE_TO_BYTE 1024 * 1024

/*! \class MultiOutputFileStream
 *  \brief A class will provide API's to writing decoded output into multiple out files.
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

    /*! A virtual Destructor
     *  \brief  Class destructor.
     *
     *  \remark Will delete dynamically created objects in constructor
     */
    virtual ~MultiOutputFileStream();

    /*! \fn void SelectFileStream(std::wstring stFileName)
     *  \brief Sets the output file in which to be decoded ouput will be written.
     *  \param [in] stFileName - Name of the File to be created.
     *  \sa FileStream
     *  \remark FileStream Object will be created and added to map.
     *  If already created, The file with name stFileName will be set for writing.
     */
    void SelectFileStream(std::u32string s32FileName_);

    /*! \fn void ClearWCFileStreamMap()
     *  \brief Delete all the output files assosiated Wide Character FileStream objects.
     *  \remark Clears the map in which all the output file stream objects will be saved.
     */

    void ClearWCFileStreamMap();

    /*! \fn void ConfigureBaseFileName(std::wstring stFileName)
     *  \brief Gets base file name with wide characters and extension of it.
     *  \param [in] stFileName - Name of the File.
     *  \remark Sets Base file name (before '.' in file name)
     *          Sets Extension of the file.
     */
    void ConfigureBaseFileName(std::u32string s32FileName_);

    /*! \fn void SelectWCLogFile(std::string strMsgName_)
     *  \brief Sets the output file name  from the log name from strMsgName_.
     *  \param [in] std::string strMsgName_
     *  \remark Sets output file name like "basename_logname".
     */
    void SelectWCLogFile(std::string strMsgName_);

    /*! \fn void SelectWCSizeFile(uint32_t uiSize_)
     *  \brief Sets the output file name(with Wide Characters) included with the split size
     *  \param [in] uint32_t uiSize_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectWCSizeFile(uint32_t uiSize_);

    /*! \fn void SelectWCTimeFile(TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_)
     *  \brief Sets the number of output files can be created based the time provided
     *  \param [in] TIME_STATUS eStatus_
     *  \param [in] uint16_t usWeek_
     *  \param [in] double dMilliseconds_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectWCTimeFile(novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_);

    /*! \fn uint32_t WriteData(char* pcData_, uint32_t uiDataLength_, std::string strMsgName_,
     * uint32_t uiSize_, novatel::edie::oem::TIME_STATUS eStatus_, uint16_t usWeek_, DOUBLE
     * dMilliseconds_) \param [in] char* pcData_ \param [in] uint32_t uiDataLength_ \param [in]
     * std::string strMsgName_ \param [in] uint32_t uiSize_ \param [in]
     * novatel::edie::oem::TIME_STATUS eStatus_ \param [in] uint16_t usWeek_ \param [in] double
     * dMilliseconds_ \return Number of bytes written to output file. \remark Set Split type and
     * write data to output files. If split type was not set, Then writing can be done to only one
     * file.
     */
    uint32_t WriteData(char* pcData_, uint32_t uiDataLength_, std::string strMsgName_, uint32_t uiSize_, novatel::edie::TIME_STATUS eStatus_,
                       uint16_t usWeek_, double dMilliseconds_);

    /*! \fn uint32_t WriteData(CHAR *pcFrameBuf_, uint32_t uiLength)
     *  \brief Write Buffer to outputfile.
     *  \param [in] *pcFrameBuf_ pointer to buffer to be written to output file
     *  \param [in] uiLength Length of the buffer to be written to output file
     *  \return Number of bytes written to output file.
     *  \remark Set Split type and write data to output files. If split type was not set,
     *  Then writing can be done to only one file.
     */
    uint32_t WriteData(char* pcData_, uint32_t uiDataLength_);

    /*! \fn void SelectFileStream(std::string stFileName)
     *  \brief Sets the output file in which to be decoded ouput will be written.
     *  \param [in] stFileName - Name of the File to be created.
     *  \sa FileStream
     *  \remark FileStream Object will be created and added to map.
     *  If already created, The file with name stFileName will be set for writing.
     */
    void SelectFileStream(std::string stFileName);

    /*! \fn void ConfigureBaseFileName(std::string stFileName)
     *  \brief Gets base file name and extension of it.
     *  \param [in] stFileName - Name of the File.
     *  \remark Sets Base file name (before '.' in file name)
     *          Sets Extension of the file.
     */
    void ConfigureBaseFileName(std::string stFileName);

    /*! \fn void ClearFileStreamMap()
     *  \brief Delete all the output files assosiated FileStream objects.
     *  \remark Clears the map in which all the output file stream objects will be saved.
     */
    void ClearFileStreamMap();

    /*! \fn void ConfigureSplitByLog(bool)
     *  \brief Enalbe/Diable Splitting of logs into different output files.
     *  \param [in] bStatus
     *  \remark Enable/Disable log Splitting. If enable, split type will be set to SPLIT_LOG
     *  If disabled split type will be set to SPLIT_NONE
     */
    void ConfigureSplitByLog(bool bStatus);

    /*! \fn void SelectLogFile(std::string strMsgName_)
     *  \brief Sets the output file name  from the log name from strMsgName_.
     *  \param [in] std::string strMsgName_
     *  \remark Sets output file name like "basename_logname".
     */
    void SelectLogFile(std::string strMsgName_);

    /*! \fn void ConfigureSplitBySize(uint64_t )
     *  \brief Split file into different output file with defined size.
     *  \param [in] ullFileSplitSize
     *  \remark Output files with ullFileSplitSize size will be created while writing to the output.
     */
    void ConfigureSplitBySize(uint64_t ullFileSplitSize);

    /*! \fn void SelectSizeFile(uint32_t uiSize_)
     *  \brief Sets the output file name included with the split size
     *  \param [in] uint32_t uiSize_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectSizeFile(uint32_t uiSize_);

    /*! \fn void ConfigureSplitByTime(double dFileSplitTime)
     *  \brief Sets the interval of time the file to be splitted.
     *  \param [in] dFileSplitTime
     *  \remark Diffrent output files will be created with the logs,
     *  in which will be captured in the tiem interval provided.
     */
    void ConfigureSplitByTime(double dFileSplitTime);

    /*! \fn void SelectTimeFile(TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_)
     *  \brief Sets the number of output files can be created based the time provided
     *  \param [in] TIME_STATUS eStatus_
     *  \param [in] uint16_t usWeek_
     *  \param [in] double dMilliseconds_
     *  \remark Example: output files could be basename_part<1/2/3...etc>
     */
    void SelectTimeFile(novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_);

    /*! \fn std::map<std::string, FileStream*> GetFileMap()
     *  \brief Gets the output file map
     *  \return Map with filename and FileStream Struct as key-value pair
     *  \sa FileStream
     */
    std::map<std::string, FileStream*> GetFileMap() { return mMyFstreamMap; }

    /*! \fn std::map<std::u32string, FileStream*> GetFileMap()
     *  \brief Gets the output file map
     *  \return Map with filename and FileStream Struct as key-value pair
     *  \sa FileStream
     */
    std::map<std::u32string, FileStream*> Get32FileMap() { return wmMyFstreamMap; }

    /*! \fn void SetExtensionName(std::string
     *  \brief Sets the extension name of the output file
     *  \param [in] strExt std::string - Output file name
     */
    void SetExtensionName(std::string strExt) { stMyExtentionName = strExt; }
    void SetExtensionName(std::u32string strExt) { s32MyExtentionName = strExt; }

    /*! Frind class to test private methods. */
    friend class MultiOutputFileStreamTest;

  private:
    /*! Private Copy Constructor
     *
     *  A copy constructor is a member function which initializes an object using another object of
     * the same class.
     */
    MultiOutputFileStream(const MultiOutputFileStream& clTemp);

    /*! Private assignment operator
     *
     *  The copy assignment operator is called whenever selected by overload resolution,
     *  e.g. when an object appears on the left side of an assignment expression.
     */
    const MultiOutputFileStream& operator=(const MultiOutputFileStream& clTemp);

    /*! FileStream class object pointer
     * \sa FileStream
     */
    FileStream* pLocalFileStream{nullptr};

    /*! \var eMyFileSplitMethodEnum.
     * \sa FileSplitMethodEnum
     *
     * An enumaration variable
     */
    FileSplitMethodEnum eMyFileSplitMethodEnum{SPLIT_NONE};

    /*! \var bMyFileSplit
     *  \remark Values can be true or false.
     *
     *  Boolean variable to enable/disable split mechanism.
     */
    bool bMyFileSplit{false};

    /*! \var uiMyFileCount
     *
     * Number of output files created.
     */
    uint32_t uiMyFileCount{0};

    /*! \var dMyTimeSplitSize
     *
     *  Time interval to be used for splitting of file.
     */
    double dMyTimeSplitSize{0.0};

    /*! \var dMyTimeInSeconds
     *
     * Time interval to be used for calculating start/end time of logs to be written to output.
     */
    double dMyTimeInSeconds{0.0};

    /*! \var dMyStartTimeInSeconds
     *
     * Start time of the Logs to be written to the output.
     */
    double dMyStartTimeInSeconds{0.0};

    /*! \var dMyStartTimeInSeconds
     *
     * Start GPS Week of the Logs to be written to the output.
     */
    uint32_t ulMyStartWeek{0UL};

    /*! \var dMyStartTimeInSeconds
     *
     * Start GPS Week of the Logs to be written to the output.
     */
    uint32_t ulMyWeek{0UL};

    /*! Wide Character Base name of the output file */
    std::u32string s32MyBaseName{U"DefaultBase"};
    /*! Wide Character extension name of the output file */
    std::u32string s32MyExtentionName{U"DefaultExt"};

    /*! std::map to save output file name with wide charater and associated FileStream class object.
     */
    typedef std::map<std::u32string, FileStream*> WCFstreamMap;
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
    std::string stMyExtentionName{"DefaultExt"};
    /*! std::map to save output file name and associated FileStream class object. */
    typedef std::map<std::string, FileStream*> FstreamMap;
    /*! Map variable */
    FstreamMap mMyFstreamMap;
    /*! The splitted output file size */
    uint64_t ullMyFileSplitSize{0ULL};
    /*! Total File size */
    uint64_t ullMyFileSize{0ULL};
};

#endif
