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
// ! \file filestream.cpp
// ===============================================================================

#include "novatel_edie/stream_interface/filestream.hpp"

#include <string>

#include "novatel_edie/common/nexcept.hpp"

// ---------------------------------------------------------
FileStream::FileStream(const std::u32string& s32FileName_)
{
    if (s32FileName_.length() == 0) { throw NExcept("file name is not valid"); }

    clFilePath = std::filesystem::path(s32FileName_);

    ullMyFileLength = 0;
    ullMyCurrentFileSize = 0;
    ullMyCurrentFileOffset = 0;
}

// ---------------------------------------------------------
FileStream::FileStream(const char* pcFileName_)
{
    if (pcFileName_ == nullptr) { throw NExcept("file name is not valid"); }

    clFilePath = std::filesystem::path(pcFileName_);

    ullMyFileLength = 0;
    ullMyCurrentFileSize = 0;
    ullMyCurrentFileOffset = 0;
}

// ---------------------------------------------------------
FileStream::~FileStream() = default;

// ---------------------------------------------------------
// Open the file in different modes.
void FileStream::OpenFile(FILE_MODES eMode)
{
    switch (eMode)
    {
    case FILE_MODES::APPEND:
        MyStream.open(clFilePath, std::ios::out | std::ios::app | std::ios::binary);
        if (MyStream.fail()) { throw NExcept("file does not exist"); }
        break;

    case FILE_MODES::INSERT:
        MyStream.open(clFilePath, std::ios::out | std::ios::ate | std::ios::binary);
        if (MyStream.fail()) { throw NExcept("file does not exist"); }

        break;

    case FILE_MODES::INPUT:
        MyStream.open(clFilePath, std::ios::in | std::ios::binary);
        if (MyStream.fail()) { throw NExcept("file does not exist"); }

        break;

    case FILE_MODES::OUTPUT:
        MyStream.open(clFilePath, std::ios::out | std::ios::binary);
        if (MyStream.fail()) { throw NExcept("file does not exist"); }

        break;

    case FILE_MODES::TRUNCATE:
        MyStream.open(clFilePath, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
        if (MyStream.fail()) { throw NExcept("file does not exist"); }

        break;

    default: throw NExcept("file does not exist");
    }
}

// ---------------------------------------------------------
// This function may not be required ,because fstream closes
// the files when out of scope. This may be helpful
// if somebody wants to check the close status.
void FileStream::CloseFile()
{
    MyStream.close();
    if (MyStream.fail()) { throw NExcept("\"%s\" close file failed", clFilePath.u32string().c_str()); }
}

// ---------------------------------------------------------
void FileStream::FlushFile()
{
    MyStream.flush();
    if (MyStream.fail()) { throw NExcept("\"%s\" flush file failed", clFilePath.string().c_str()); }
}

// ---------------------------------------------------------
// Reads uiSize character of data from fstream file and stores
// them i the array pointed by cData also fills the
// StreamReadStatus structure.
StreamReadStatus FileStream::ReadFile(char* cData, uint32_t uiSize)
{
    StreamReadStatus stFileReadStatus;

    MyStream.read(cData, uiSize);
    if (MyStream.bad()) { throw NExcept("\"%s\" file  read failed", clFilePath.generic_u32string().c_str()); }

    // This size will be used to calculate file read percentage
    ullMyCurrentFileSize = ullMyCurrentFileSize + MyStream.gcount();

    stFileReadStatus.uiCurrentStreamRead = static_cast<uint32_t>(MyStream.gcount());  // Current read byte count
    stFileReadStatus.uiPercentStreamRead = CalculatePercentage(ullMyCurrentFileSize); // Total read percentage
    stFileReadStatus.ullStreamLength = ullMyFileLength;                               // Total File Length (in Bytes)
    stFileReadStatus.bEOS = false;

    if (MyStream.eof()) { stFileReadStatus.bEOS = true; } // Reached End Of File

    return stFileReadStatus;
}

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
// Writes the first uiSize character pointed to by cData into
// fstream
uint32_t FileStream::WriteFile(const char* cData, uint32_t uiSize)
{
    MyStream.write(cData, uiSize);
    if (MyStream.bad()) { throw NExcept("\"%s\" file  write failed", clFilePath.generic_u32string().c_str()); }
    FlushFile();
    return uiSize;
}

// ---------------------------------------------------------
void FileStream::CalculateFileSize()
{
    uintmax_t fileSize = std::filesystem::file_size(clFilePath);
    if (fileSize > 0) { ullMyFileLength = fileSize; }
}

// ---------------------------------------------------------
// Calculates the File size and update it in MyFileLength.
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
// Calculates the percentage of current file read.
uint32_t FileStream::CalculatePercentage(uint64_t ullCurrentFileRead) const
{
    return ullMyFileLength == 0 ? 100 : static_cast<uint32_t>(ullCurrentFileRead * 100 / ullMyFileLength);
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
void FileStream::SetCurrentFileOffset(uint64_t ullCurrentFileOffset) { ullMyCurrentFileOffset += ullCurrentFileOffset; }
