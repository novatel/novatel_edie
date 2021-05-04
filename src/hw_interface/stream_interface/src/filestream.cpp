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
//
//  DESCRIPTION: Basic File Stream Functions.
//    
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include "filestream.hpp"
#include <sys/stat.h>
using namespace std;
#undef max
// code
#ifdef WIDE_CHAR_SUPPORT
// ---------------------------------------------------------
FileStream::FileStream(const wchar_t* pwFileName)
{
	if (pwFileName == NULL)
	{
		throw nExcept("\"%s\" file  name not valid", pcMyFileName);
	}

	pwcMyFileName = new wchar_t[wcslen(pwFileName) + 1];
	wcscpy((wchar_t*)pwcMyFileName, pwFileName);

	ullMyFileLength = 0;
	ullMyCurrentFileSize = 0;
	ullMyCurrentFileOffset = 0;
	pcMyFileName = NULL;
	bEnableWideCharSupport = TRUE;
}
#endif

// ---------------------------------------------------------
FileStream::FileStream(const CHAR* pcFileName)
{
	if (pcFileName == NULL)
	{
		throw nExcept("Filename  name not valid");
	}

	pcMyFileName = new CHAR[strlen(pcFileName)+1];
	strcpy((CHAR*)pcMyFileName, pcFileName);

	ullMyFileLength = 0;
	ullMyCurrentFileSize = 0;
	ullMyCurrentFileOffset = 0;
#ifdef WIDE_CHAR_SUPPORT
	pwcMyFileName = NULL;
	bEnableWideCharSupport = FALSE;
#endif
}

// ---------------------------------------------------------
FileStream::~FileStream()
{
#ifdef WIDE_CHAR_SUPPORT
	if (pwcMyFileName != NULL)
	{
		delete[] pwcMyFileName;
		pwcMyFileName = NULL;
	}
#endif
	if (pcMyFileName != NULL)
	{
		delete[] pcMyFileName;
		pcMyFileName = NULL;
	}
}

// ---------------------------------------------------------
// Open the file in different modes.
void FileStream::OpenFile(FILEMODES eMode)
{   
	switch(eMode)
	{
	case APPEND:
#ifdef WIDE_CHAR_SUPPORT
		if (bEnableWideCharSupport == TRUE)
			MyStream.open(pwcMyFileName, ios::out | ios::app | ios::binary);
		else
#endif
			MyStream.open(pcMyFileName, ios::out | ios::app | ios::binary);

		if(MyStream.fail())
			throw nExcept("\"%s\" file  cannot be opened", pcMyFileName);
		break;      
	case INSERT:
#ifdef WIDE_CHAR_SUPPORT
		if (bEnableWideCharSupport == TRUE)
			MyStream.open(pwcMyFileName, ios::out | ios::ate | ios::binary);
		else
#endif
			MyStream.open(pcMyFileName, ios::out | ios::ate | ios::binary);

		if(MyStream.fail())
			throw nExcept("\"%s\" file  does not exist", pcMyFileName);
		break;
	case INPUT:
#ifdef WIDE_CHAR_SUPPORT
		if (bEnableWideCharSupport == TRUE)
			MyStream.open(pwcMyFileName, ios::in | ios::binary);
		else
#endif
			MyStream.open(pcMyFileName, ios::in | ios::binary);

		if(MyStream.fail())
			throw nExcept("\"%s\" file  does not exist", pcMyFileName);
		break;
	case OUTPUT:
#ifdef WIDE_CHAR_SUPPORT
		if (bEnableWideCharSupport == TRUE)
			MyStream.open(pwcMyFileName, ios::out| ios::binary);
		else
#endif
			MyStream.open(pcMyFileName, ios::out| ios::binary);

		if(MyStream.fail())
			throw nExcept("\"%s\" file  cannot open", pcMyFileName);
		break;
	case TRUNCATE:
#ifdef WIDE_CHAR_SUPPORT
		if (bEnableWideCharSupport == TRUE)
			MyStream.open(pwcMyFileName, ios::in | ios::out | ios::trunc | ios::binary);
		else
#endif
			MyStream.open(pcMyFileName, ios::in | ios::out | ios::trunc | ios::binary);

		if(MyStream.fail())
			throw nExcept("\"%s\" file  cannot open", pcMyFileName);
		break;
	default:
		throw nExcept("\"%s\" file  mode not valid", pcMyFileName);
	}
}

// ---------------------------------------------------------
// This function may not be required ,because fstream closes 
// the files when out of scope. This may be helpuful 
// if somebody wants to check the close status. 
void FileStream::CloseFile()
{
	MyStream.close();
	if(MyStream.fail())
		throw nExcept("\"%s\" close file failed", pcMyFileName);
}

// ---------------------------------------------------------
void FileStream::FlushFile()
{
	MyStream.flush();
	if(MyStream.fail())
		throw nExcept("\"%s\" flush file failed", pcMyFileName);
}

// ---------------------------------------------------------
// Reads uiSize character of data from fstream file and stores 
// them i the array pointed by cData also fills the
// StreamReadStatus structure.
StreamReadStatus FileStream::ReadFile(CHAR* cData, UINT uiSize)
{
	StreamReadStatus stFileReadStatus;

	MyStream.read(cData, uiSize);
	if (MyStream.bad())
	{
		throw nExcept("\"%s\" file  read failed", pcMyFileName);
	}

	// This size will be used to calculate file read percentage
	ullMyCurrentFileSize = ullMyCurrentFileSize + MyStream.gcount();

	stFileReadStatus.uiCurrentStreamRead = (UINT)MyStream.gcount();                  // Current read byte count
	stFileReadStatus.uiPercentStreamRead = CalculatePercentage(ullMyCurrentFileSize);   // Total read percentage
	stFileReadStatus.ullStreamLength = (ULONGLONG)ullMyFileLength;                            // Total File Length (in Bytes)
	stFileReadStatus.bEOS = FALSE;

	if (MyStream.eof())
	{
		stFileReadStatus.bEOS = TRUE;                                                 // Reached End Of File
	}
	return stFileReadStatus;
}

StreamReadStatus FileStream::ReadLine(std::string& szLine)
{
	StreamReadStatus stFileReadStatus;

	bool bEOF = std::getline(MyStream, szLine).eof();
	if (bEOF)
	{
		stFileReadStatus.bEOS = TRUE;
		return stFileReadStatus;
	}

	// This size will be used to calculate file read percentage
	ullMyCurrentFileSize = ullMyCurrentFileSize + szLine.length();

	stFileReadStatus.uiCurrentStreamRead = (UINT)szLine.length(); // Current read byte count
	stFileReadStatus.uiPercentStreamRead = CalculatePercentage(ullMyCurrentFileSize); // Total read percentage
	stFileReadStatus.ullStreamLength = (ULONGLONG)ullMyFileLength; // Total File Length (in Bytes)
	stFileReadStatus.bEOS = FALSE;

	return stFileReadStatus;
}

// ---------------------------------------------------------
// Writes the first uiSize character poited by cData into 
// fstream 
UINT FileStream::WriteFile(CHAR* cData, UINT uiSize)
{
	MyStream.write(cData, uiSize);
	if (MyStream.bad())
	{
		throw nExcept("\"%s\" file  write failed", pcMyFileName);
	}
	FlushFile();
	return uiSize;
}

// ---------------------------------------------------------
void FileStream::CalculateFileSize()
{
#ifdef WIN32
#define stat64 _stat64
	struct _stat64 stat_buf;
#else
#define stat64 stat64
	struct stat64 stat_buf;
#endif

	//struct __stat64 stat_buf;
	stat64(pcMyFileName, &stat_buf);
	if(stat_buf.st_size > 0)
		ullMyFileLength = (ULONGLONG)stat_buf.st_size;
}

#ifdef WIDE_CHAR_SUPPORT
// ---------------------------------------------------------
// Calculates the File size of wide char file name and update it in MyFileLength.
void FileStream::CalculateWCFileSize()
{
#ifdef WIN32
#define stat64 _wstat64
	struct _stat64 stat_buf;
#else
#define stat64 stat64
	struct stat64 stat_buf;
#endif

	//struct __stat64 stat_buf;
	stat64(pwcMyFileName, &stat_buf);
	if(stat_buf.st_size > 0)
		ullMyFileLength = (ULONGLONG)stat_buf.st_size;
}
#endif

// ---------------------------------------------------------
// Calculates the File size and update it in MyFileLength.
void FileStream::GetFileSize()
{
#if _DEBUG
	MyStream.ignore( std::numeric_limits<ULONGLONG>::max() );
	ullMyFileLength = MyStream.gcount();
	MyStream.clear();
	MyStream.seekg(0, MyStream.beg);
#endif

#ifdef WIDE_CHAR_SUPPORT
	if (bEnableWideCharSupport == TRUE)
		CalculateWCFileSize();
	else
#endif
		CalculateFileSize();
}

// ---------------------------------------------------------
// Calculates the percentage of current file read.
UINT FileStream::CalculatePercentage(ULONGLONG ullCurrentFileRead)
{
	if(ullMyFileLength <= 0)
	{
		throw nExcept("\"%s\" file  size not valid", pcMyFileName);
		//return 0;
	}
	UINT uiResult = (UINT)((ullCurrentFileRead*100)/ullMyFileLength);
	return uiResult;
}

// ---------------------------------------------------------
// Set File Position.
void FileStream::SetFilePosition(std::streamoff offset, std::ios_base::seekdir dir)
{
	MyStream.clear();
	MyStream.seekg(offset, dir);
	ullMyCurrentFileSize = MyStream.tellg();
	ullMyCurrentFileOffset = ullMyCurrentFileSize;
}

// ---------------------------------------------------------
void FileStream::SetCurrentFileOffset(ULONGLONG ullCurrentFileOffset)
{
	ullMyCurrentFileOffset += ullCurrentFileOffset;
}
