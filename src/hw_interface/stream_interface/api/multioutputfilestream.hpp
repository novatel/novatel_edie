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
 *  Created to satisfy the need of maintaining multiple output files at a time. It will mainly be used for file split functionalities.
 *  It also provides an API called SelectFileStream to select the active file stream based on passed filename key.
 */

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef MULTIOUTPUTFILESTREAM_HPP
#define MULTIOUTPUTFILESTREAM_HPP

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "outputstreaminterface.hpp"
#include "filestream.hpp"
#include "decoders/novatel/api/filters/messagedatafilter.hpp"
#include "decoders/common/api/nexcept.h"



/*! \def MIN_TIME_SPLIT_SEC
 *  \brief Minimum split time in seconds.
 *
 */
#define MIN_TIME_SPLIT_SEC    36

/*! \def MIN_FILE_SPLIT_SIZE
 *  \brief Minimum splitting size of file in mb.
 *
 */
#define MIN_FILE_SPLIT_SIZE    1

/*! \def HR_TO_SEC
 *  \brief Number of seconds in a Hour.
 *
 */
#define HR_TO_SEC    3600

/*! \def MBYTE_TO_BYTE
 *  \brief Number of bytes in one mega byte?.
 *
 */
#define MBYTE_TO_BYTE    1024*1024

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
	 *  \remark All private members are defaults to 0.
	 */
   MultiOutputFileStream();

    /*! A Constructor
	 *  \brief  Class constructor with MessageDataFilter object.
	 *
	 *  \remark MessageDataFilter object Will be apply when filtering enbled
    *  in writing decoded data to output file.
	 */
   MultiOutputFileStream(MessageDataFilter&);

    /*! A virtual Destructor
	 *  \brief  Class destructor.
	 *
	 *  \remark Will delete dynamically created objects in constructor
	 */
   virtual ~MultiOutputFileStream();

#ifdef WIDE_CHAR_SUPPORT
   /*! \fn void SelectFileStream(std::wstring stFileName)
    *  \brief Sets the output file in which to be decoded ouput will be written.
    *  \param [in] stFileName - Name of the File to be created.
    *  \sa FileStream
    *  \remark FileStream Object will be created and added to map.
    *  If already created, The file with name stFileName will be set for writing.
    */
   void SelectFileStream(std::wstring );

   /*! \fn void ConfigureBaseFileName(std::wstring stFileName)
    *  \brief Gets base file name with wide characters and extension of it.
    *  \param [in] stFileName - Name of the File.
    *  \remark Sets Base file name (before '.' in file name)
    *          Sets Extension of the file.
    */
   void ConfigureBaseFileName(std::wstring);

   /*! \fn void ClearWCFileStreamMap()
    *  \brief Delete all the output files assosiated Wide Character FileStream objects.
    *  \remark Clears the map in which all the output file stream objects will be saved.
    */
   void ClearWCFileStreamMap();

   /*! \fn void SelectWCLogFile(BaseMessageData&)
    *  \brief Sets the output file name  from the log name from BaseMessageData.
    *  \param [in] BaseMessageData pointer
    *  \remark Sets output file name like "basename_logname".
    */
   void SelectWCLogFile(BaseMessageData&);

   /*! \fn void SelectWCSizeFile(BaseMessageData&)
    *  \brief Sets the output file name(with Wide Characters) included with the split size
    *  \param [in] BaseMessageData pointer
    *  \remark Example: output files could be basename_part<1/2/3...etc>
    */
   void SelectWCSizeFile(BaseMessageData&);

   /*! \fn void SelectWCTimeFile(BaseMessageData&)
    *  \brief Sets the number of output files can be created based the time provided
    *  \param [in] BaseMessageData pointer
    *  \remark Example: output files could be basename_part<1/2/3...etc>
    */
   void SelectWCTimeFile(BaseMessageData&);
#endif
   /*! \fn UINT WriteData(BaseMessageData&)
    *  \brief
    *  \param [in] pclBaseMessageData BaseMessageData object.
    *  \return Number of bytes written to output file.
    *  \remark Set Split type and write data to output files. If split type was not set,
    *  Then writing can be done to only one file.
    */
   UINT WriteData(BaseMessageData& pclBaseMessageData);

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

   /*! \fn void ConfigureSplitByLog(BOOL)
    *  \brief Enalbe/Diable Splitting of logs into different output files.
    *  \param [in] bStatus
    *  \remark Enable/Disable log Splitting. If enable, split type will be set to SPLIT_LOG
    *  If disabled split type will be set to SPLIT_NONE
    */
   void ConfigureSplitByLog(BOOL bStatus);

   /*! \fn void SelectLogFile(BaseMessageData&)
    *  \brief Sets the output file name  from the log name from BaseMessageData.
    *  \param [in] pclBaseMessageData BaseMessageData object
    *  \remark Sets output file name like "basename_logname".
    */
   void SelectLogFile(BaseMessageData& pclBaseMessageData);

   /*! \fn void ConfigureSplitBySize(ULONGLONG)
    *  \brief Split file into different output file with defined size.
    *  \param [in] ullFileSplitSize
    *  \remark Output files with ullFileSplitSize size will be created while writing to the output.
    */
   void ConfigureSplitBySize(ULONGLONG ullFileSplitSize);

   /*! \fn void SelectSizeFile(BaseMessageData&)
    *  \brief Sets the output file name included with the split size
    *  \param [in] pclBaseMessageData BaseMessageData object
    *  \remark Example: output files could be basename_part<1/2/3...etc>
    */
   void SelectSizeFile(BaseMessageData& pclBaseMessageData);

   /*! \fn void ConfigureSplitByTime(DOUBLE dFileSplitTime)
    *  \brief Sets the interval of time the file to be splitted.
    *  \param [in] dFileSplitTime
    *  \remark Diffrent output files will be created with the logs,
    *  in which will be captured in the tiem interval provided.
    */
   void ConfigureSplitByTime(DOUBLE dFileSplitTime);

   /*! \fn void SelectTimeFile(BaseMessageData&)
    *  \brief Sets the number of output files can be created based the time provided
    *  \param [in] pclBaseMessageData BaseMessageData object
    *  \remark Example: output files could be basename_part<1/2/3...etc>
    */
   void SelectTimeFile(BaseMessageData& pclBaseMessageData);

   /*! \fn std::map<std::string, FileStream*> GetFileMap()
    *  \brief Gets the output file map
    *  \return Map with filename and FileStream Struct as key-value pair
    *  \sa FileStream
    */
   std::map<std::string, FileStream*> GetFileMap() { return mMyFstreamMap;}

   /*! \fn void SetExtensionName(std::string
    *  \brief Sets the extension name of the output file
    *  \param [in] strExt std::string - Output file name
    */
   void SetExtensionName(std::string strExt) { stMyExtentionName = strExt; }   

   /*! Frind class to test private methods. */
   friend class MultiOutputFileStreamTest;

private:
	/*! Private Copy Constructor
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class.
	 */
   MultiOutputFileStream(const MultiOutputFileStream& clTemp);

	/*! Private assignment operator
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution,
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */
   const MultiOutputFileStream& operator= (const MultiOutputFileStream& clTemp);

   /*! FileStream class object pointer
    * \sa FileStream
    */
   FileStream* pLocalFileStream;

   /*! MessageDataFilter class object pointer
    * \sa MessageDataFilter
    */
   MessageDataFilter* pMyMessageDataFilter;

   /*! \var eMyFileSplitMethodEnum.
    * \sa FileSplitMethodEnum
    *
    * An enumaration variable
    */
   FileSplitMethodEnum eMyFileSplitMethodEnum;

   /*! \var bMyFileSplit
    *  \remark Values can be TRUE or FALSE.
    *
    *  Boolean variable to enable/disable split mechanism.
    */
   BOOL bMyFileSplit;

   /*! \var uiMyFileCount
    *
    * Number of output files created.
    */
   UINT uiMyFileCount;

   /*! \var dMyTimeSplitSize
    *
    *  Time interval to be used for splitting of file.
    */
   DOUBLE dMyTimeSplitSize;

   /*! \var dMyTimeInSeconds
    *
    * Time interval to be used for calculating start/end time of logs to be written to output.
    */
   DOUBLE dMyTimeInSeconds;

   /*! \var dMyStartTimeInSeconds
    *
    * Start time of the Logs to be written to the output.
    */
   DOUBLE dMyStartTimeInSeconds;

   /*! \var dMyStartTimeInSeconds
    *
    * Start GPS Week of the Logs to be written to the output.
    */
   ULONG ulMyStartWeek;

   /*! \var dMyStartTimeInSeconds
    *
    * Start GPS Week of the Logs to be written to the output.
    */
   ULONG ulMyWeek;

#ifdef WIDE_CHAR_SUPPORT
   /*! Wide Character Base name of the output file */
   std::wstring wstMyBaseName;
   /*! Wide Character extension name of the output file */
   std::wstring wstMyExtentionName;
   /*! std::map to save output file name with wide charater and associated FileStream class object. */
   typedef std::map<std::wstring, FileStream* > WCFstreamMap;
   /*! Wide Character file Map variable */
   WCFstreamMap wmMyFstreamMap;
   /*! Enable or Disable Wide Character support of file names.*/
   BOOL bEnableWideCharSupport;
#endif
   /*! Base name of the output file */
   std::string stMyBaseName;
   /*! Extension name of the output file */
   std::string stMyExtentionName;
   /*! std::map to save output file name and associated FileStream class object. */
   typedef std::map<std::string, FileStream* > FstreamMap;
   /*! Map variable */
   FstreamMap mMyFstreamMap;
   /*! The splitted output file size */
   ULONGLONG ullMyFileSplitSize;
   /*! Total File size */
   ULONGLONG ullMyFileSize;

};

#endif
