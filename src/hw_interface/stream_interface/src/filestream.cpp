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

#include <string>

#include "decoders/common/api/nexcept.h"

using namespace std;

// ---------------------------------------------------------
FileStream::FileStream(const std::u32string s32FileName_)
{
    if (s32FileName_.length() == 0) { throw nExcept("file name is not valid"); }

    clFilePath = std::filesystem::path(s32FileName_);

    ullMyFileLength = 0;
    ullMyCurrentFileSize = 0;
    ullMyCurrentFileOffset = 0;
}

// ---------------------------------------------------------
FileStream::FileStream(const char* pcFileName_)
{
    if (pcFileName_ == nullptr) { throw nExcept("file name is not valid"); }

    clFilePath = std::filesystem::path(pcFileName_);

    ullMyFileLength = 0;
    ullMyCurrentFileSize = 0;
    ullMyCurrentFileOffset = 0;
}

// ---------------------------------------------------------
FileStream::~FileStream() {}

// ---------------------------------------------------------
void FileStream::OpenFile(FILEMODES eMode)
{
    switch (eMode)
    {
    case FILEMODES::APPEND:
        MyStream.open(clFilePath, ios::out | ios::app | ios::binary);
        if (MyStream.fail()) throw nExcept("file does not exist");
        break;

    case FILEMODES::INSERT:
        MyStream.open(clFilePath, ios::out | ios::ate | ios::binary);
        if (MyStream.fail()) throw nExcept("file does not exist");

        break;

    case FILEMODES::INPUT:
        MyStream.open(clFilePath, ios::in | ios::binary);
        if (MyStream.fail()) throw nExcept("file does not exist");

        break;

    case FILEMODES::OUTPUT:
        MyStream.open(clFilePath, ios::out | ios::binary);
        if (MyStream.fail()) throw nExcept("file does not exist");

        break;

    case FILEMODES::TRUNCATE:
        MyStream.open(clFilePath, ios::in | ios::out | ios::trunc | ios::binary);
        if (MyStream.fail()) throw nExcept("file does not exist");

        break;

    default: throw nExcept("file does not exist");
    }
}

// ---------------------------------------------------------
void FileStream::CloseFile()
{
    MyStream.close();
    if (MyStream.fail()) throw nExcept("\"%s\" close file failed", clFilePath.u32string().c_str());
}

// ---------------------------------------------------------
void FileStream::FlushFile()
{
    MyStream.flush();
    if (MyStream.fail()) throw nExcept("\"%s\" flush file failed", clFilePath.string().c_str());
}

// ---------------------------------------------------------
StreamReadStatus FileStream::ReadFile(char* cData, uint32_t uiSize)
{
    StreamReadStatus stFileReadStatus;

    MyStream.read(cData, uiSize);
    if (MyStream.bad()) { throw nExcept("\"%s\" file  read failed", clFilePath.generic_u32string().c_str()); }

    // This size will be used to calculate file read percentage
    ullMyCurrentFileSize = ullMyCurrentFileSize + MyStream.gcount();

    stFileReadStatus.uiCurrentStreamRead = static_cast<uint32_t>(MyStream.gcount());  // Current read byte count
    stFileReadStatus.uiPercentStreamRead = CalculatePercentage(ullMyCurrentFileSize); // Total read percentage
    stFileReadStatus.ullStreamLength = ullMyFileLength;                               // Total File Length (in Bytes)
    stFileReadStatus.bEOS = false;

    if (MyStream.eof())
    {
        stFileReadStatus.bEOS = true; // Reached End Of File
    }
    return stFileReadStatus;
}

// ---------------------------------------------------------
StreamReadStatus FileStream::ReadLine(std::string& szLine)
{
    StreamReadStatus stFileReadStatus;

    if (std::getline(MyStream, szLine).eof())
    {
        stFileReadStatus.bEOS = true;
        return stFileReadStatus;
    }

    // This size will be used to calculate file read percentage
    ullMyCurrentFileSize = ullMyCurrentFileSize + szLine.length();

    stFileReadStatus.uiCurrentStreamRead = static_cast<uint32_t>(szLine.length());    // Current read byte count
    stFileReadStatus.uiPercentStreamRead = CalculatePercentage(ullMyCurrentFileSize); // Total read percentage
    stFileReadStatus.ullStreamLength = ullMyFileLength;                               // Total File Length (in Bytes)
    stFileReadStatus.bEOS = false;

    return stFileReadStatus;
}

// ---------------------------------------------------------
uint32_t FileStream::WriteFile(char* cData, uint32_t uiSize)
{
    MyStream.write(cData, uiSize);
    if (MyStream.bad()) { throw nExcept("\"%s\" file  write failed", clFilePath.generic_u32string().c_str()); }
    FlushFile();
    return uiSize;
}

// ---------------------------------------------------------
void FileStream::CalculateFileSize()
{
    uintmax_t filesize = std::filesystem::file_size(clFilePath);
    if (filesize > 0) ullMyFileLength = filesize;
}

// ---------------------------------------------------------
void FileStream::GetFileSize()
{
#if _DEBUG
    MyStream.ignore(std::numeric_limits<uint64_t>::max());
    ullMyFileLength = MyStream.gcount();
    MyStream.clear();
    MyStream.seekg(0, MyStream.beg);
#endif
    CalculateFileSize();
}

// ---------------------------------------------------------
uint32_t FileStream::CalculatePercentage(uint64_t ullCurrentFileRead) const
{
    return ullMyFileLength == 0L ? 100 : static_cast<uint32_t>(ullCurrentFileRead * 100 / ullMyFileLength);
}

// ---------------------------------------------------------
void FileStream::SetFilePosition(std::streamoff offset, std::ios_base::seekdir dir)
{
    MyStream.clear();
    MyStream.seekg(offset, dir);
    ullMyCurrentFileSize = MyStream.tellg();
    ullMyCurrentFileOffset = ullMyCurrentFileSize;
}

// ---------------------------------------------------------
void FileStream::SetCurrentFileOffset(uint64_t ullCurrentFileOffset) { ullMyCurrentFileOffset += ullCurrentFileOffset; }
