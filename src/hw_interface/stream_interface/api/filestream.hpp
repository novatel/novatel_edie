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

/*! \file filestream.hpp
 *  \brief Class definition to provide basic file operations.
 * 
 *  Class has defined in a such a way that will provide basic file operations, 
 *  Like, Open/Close/Write/Read ...etc.
 */ 

//-----------------------------------------------------------------------
// Recursive Inclusion
//-----------------------------------------------------------------------
#ifndef FILESTREAM_HPP
#define FILESTREAM_HPP

//-----------------------------------------------------------------------
// Includes                                                               
//-----------------------------------------------------------------------
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include "decoders/common/api/common.hpp"
#include "decoders/common/api/nexcept.h"

/*! \class FileStream
 *   \brief A class will provide file operation handlers.
 * 
 *  More detailed FileStream class description. It will also supports
 *  File names with Wide characters.
*/
class FileStream
{
private:
#ifdef WIDE_CHAR_SUPPORT
	wchar_t* pwcMyFileName;         /**< Wide character pointer for file name */
	BOOL bEnableWideCharSupport;    /**< Boolean variable to enable or disable Wide Character support */
   /*! \fn void CalculateWCFileSize() 
    *  \brief Calculate Wide Charater file size  
	* 
    */ 	
	void CalculateWCFileSize();
   /*! \fn wchar_t* GetWCFileName() 
    *  \brief Returns Wide Char file pointer. 
	* 
    */ 
   	wchar_t* GetWCFileName(){return pwcMyFileName;};
#endif

	const CHAR* pcMyFileName;        /**< Character pointer for file name */
	ULONGLONG ullMyFileLength;       /**< Total File Length */
	ULONGLONG ullMyCurrentFileSize;  /**< Read size so far from the file */
	ULONGLONG ullMyCurrentFileOffset;/**< Current File offset pointer from which next read will commense */
	/*! \var MyStream
	 *
	 *  Input/output stream class to operate on files.
	 *  Objects of this class maintain a filebuf object as their internal stream buffer, which performs input/output operations on the file they are associated with (if any).
	 *  File streams are associated with files on construction itself in this calss. 
	 */
	std::fstream MyStream;

	/*! Private Copy Constructor 
	 *
	 *  A copy constructor is a member function which initializes an object using another object of the same class. 
	 */ 
    FileStream(const FileStream& clOther);

	/*! Private assignment operator 
	 *
	 *  The copy assignment operator is called whenever selected by overload resolution, 
	 *  e.g. when an object appears on the left side of an assignment expression.
	 */ 
	const FileStream& operator= (const FileStream& clOther);

   /*! \fn void CalculateFileSize() 
    *  \brief Calculate Total file size  
	* 
    *
	*/ 
	void CalculateFileSize();

public:

	/*! An enum.
	*
	*   File Operations Enumaration.
	*/
	typedef enum 
	{ 
		APPEND,   /*!< Append may refer to the process of attaching or combining data with another file or set of data.*/
		INSERT,   /*!< Insert Data into the file */
		INPUT,    /*!< Open file as Input for read */ 
		OUTPUT,   /*!< Open file as output for write into the file */
		TRUNCATE  /*!< Truncate some part of data in file */
	} FILEMODES;

#ifdef WIDE_CHAR_SUPPORT
    /*! A Constructor
	 *  \brief  Create wide character filename string with name provided as argument.
	 *          And intializes File offsets and length of it.
	 * 
	 *  \param [in] pwcFileName Wide Characer file name as Character pointer.
	 * 
	 *  \remark If pwcFileName is NULL, then exception "Filename name not valid" will be thrown.
	 */ 
	FileStream(const wchar_t* pwcFileName);
#endif
    /*! A Constructor
	 *  \brief  Create filename string with name provided as argument.
	 *          And intializes File offsets and length of it.
	 * 
	 *  \param [in] pcFileName File name as Character pointer.
	 * 
	 *  \remark If pcFileName is NULL, then exception "Filename name not valid" will be thrown.
	 */ 
	FileStream(const CHAR* pcFileName);

    /*! A default Destructor
	 *
	 * All dynamic created pointers will be deleted.
	 */ 
	~FileStream();

    /*! A Freind class
	 *
	 * To test private methods inside this class from unittest class.
	 */ 
	friend class FileStreamUnitTest;

   /*! \fn void OpenFile(FILEMODES eMode)
    *  \brief Open File to read/write/truncate in different modes of open.
    *
    *  \param [in] eMode File mode enumaration
    *  \return "Does not return a value"
	*  \remark If eMode is not valid exception ("") will thrown.
	*          If open fails exception("File mode not valid") will thrown.	  
    */
	void OpenFile(FILEMODES eMode);

   /*! \fn void CloseFile()
    *  \brief Close the file.
    *
	*  \remark This function may not be required, Because fstream closes 
	*  the files when out of scope. This may be helpuful if somebody wants to check the close status. 	  
    */	
	void CloseFile();

   /*! \fn UINT CalculatePercentage(ULONGLONG ullCurrentFileRead);
    *  \brief Calculates the percentage of current file read.
	* 
	*  \param [in] ullCurrentFileRead ULONGLONG currentfile read length 
    *
	*  \return Percentage of Read.
	*  \remark If file length is '0', Then exception"...file  size not valid" will thrown. 
    */
	UINT CalculatePercentage(ULONGLONG);


   /*! \fn StreamReadStatus ReadFile(CHAR* cData, UINT );
    *  \brief Reads uiSize characters of data from fstream file and stores 
	*  them in the array pointed by cData also fills the StreamReadStatus structure.
	* 
	*  \param [in] cData Holds the read data from file.
	*  \param [in] uiSize Size of the data to be read data from file.
    *
	*  \return StreamReadStatus, Read statistics structure
	*  \remark If read fails, then exception"... file  read failed" will thrown. 
    */
	StreamReadStatus ReadFile(CHAR* cData, UINT uiSize);

   /*! \fn UINT WriteFile(CHAR* cData, UINT uiSize)
    *  \brief Writes the first uiSize character poited by cData into fstream 
	* 
	*  \param [in] cData Data to be written to the fstream.
	*  \param [in] uiSize Size of the data to be write to the file.
    *
	*  \return Number of bytes written into the file
	*  \remark If write fails, then exception"... file  write failed" will thrown. 
    */
	UINT WriteFile(CHAR* cData, UINT uiSize);

   /*! \fn void GetFileSize()
    *  \brief Calculates the File size and update it in MyFileLength private variable. 
	* 
	*  \return 
	*  \remark  
    */
	void GetFileSize();

   /*! \fn void FlushFile()
    *  \brief  clears the internal buffer of the file. 
	* 
	*  \return 
	*  \remark  
    */
	void FlushFile();

   /*! \fn void SetFilePosition(std::streamoff = 0, std::ios_base::seekdir = std::ios::beg)
    *  \brief Set File Position from which next read will be done.
	* 
	*  \param [in] offset the position of the file pointer to read.
	*  \param [in] dir Seeking direction from begining or end.
    *
	*  \remark After seeking the current file size will be changed accrdingly. 
    */
	void SetFilePosition(std::streamoff offset = 0, std::ios_base::seekdir dir = std::ios::beg);

   /*! \fn ULONGLONG GetFileLength()
    *  \brief Returns Total file length.
	* 
    *  \return Total file length. 
    */
	ULONGLONG GetFileLength(){return ullMyFileLength;};

   /*! \fn const CHAR* GetFileName()
    *  \brief Returns File Name.
	* 
    *  \return Name of the File which currently operating. 
    */
	const CHAR* GetFileName(){return pcMyFileName;};  

   /*! \fn ULONGLONG GetCurrentFileSize()
    *  \brief Returns Cuurent file size which user has read so far.
	* 
    *  \return Cuurent file size which user has read so far. 
    */
	ULONGLONG GetCurrentFileSize(){return ullMyCurrentFileSize;};

   /*! \fn std::fstream* GetMyFileStream()
    *  \brief Returns file stream pointer.
	* 
    *  \return fstream Pointer of the file stream. 
    */
	std::fstream* GetMyFileStream(){return &MyStream;};

   /*! \fn StreamReadStatus ReadLine
    *  \brief Read one line from the file.
	* 
    *  \param [in] szLine reference String to hole one line dats read from the file.
    *  \return Returns Read statistics structure (StreamReadStatus)
    */
	StreamReadStatus ReadLine(std::string& szLine);

   /*! \fn void SetCurrentFileOffset(ULONGLONG ullCurrentFileOffset)
    *  \brief Sets the current file offset. It could be read bytes so far.
	* 
    *  \param [in] ullCurrentFileOffset Size of the data from one read size, 
	*  Will be append to calculate read bytes so far
    */
	void SetCurrentFileOffset(ULONGLONG ullCurrentFileOffset);

   /*! \fn ULONGLONG GetCurrentFileOffset(void)
    *  \brief Returns Cuurent file offset.
	* 
    *  \return Cuurent file offset. 
    */
	ULONGLONG GetCurrentFileOffset(void) const { return ullMyCurrentFileOffset; };
};

#endif
