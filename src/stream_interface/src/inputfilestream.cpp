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
// ! \file inputfilestream.cpp
// ===============================================================================

#include "novatel_edie/stream_interface/inputfilestream.hpp"

#include <codecvt>

// ---------------------------------------------------------
InputFileStream::InputFileStream(const std::u32string& s32FileName_)
{
    pInFileStream = new FileStream(s32FileName_);
    pInFileStream->OpenFile(FileStream::FILE_MODES::INPUT);
    pInFileStream->GetFileSize();
    bEnableWideCharSupport = true;
}

// ---------------------------------------------------------
InputFileStream::InputFileStream(const char* pFileName_)
{
    pInFileStream = new FileStream(pFileName_);
    pInFileStream->OpenFile(FileStream::FILE_MODES::INPUT);
    pInFileStream->GetFileSize();

    bEnableWideCharSupport = false;
}

// ---------------------------------------------------------
InputFileStream::~InputFileStream() { delete pInFileStream; }

// ---------------------------------------------------------
StreamReadStatus InputFileStream::ReadData(ReadDataStructure& pReadDataStructure)
{
    return pInFileStream->ReadFile(pReadDataStructure.cData, pReadDataStructure.uiDataSize);
}

// ---------------------------------------------------------
StreamReadStatus InputFileStream::ReadLine(std::string& szLine) { return pInFileStream->ReadLine(szLine); }

// ---------------------------------------------------------
void InputFileStream::Reset(std::streamoff offset, std::ios_base::seekdir dir) { pInFileStream->SetFilePosition(offset, dir); }

// ---------------------------------------------------------
uint64_t InputFileStream::GetCurrentFilePosition() { return pInFileStream->GetCurrentFileSize(); }
// ---------------------------------------------------------
void InputFileStream::SetCurrentFileOffset(uint64_t ullCurrentFileOffset) { pInFileStream->SetCurrentFileOffset(ullCurrentFileOffset); }

uint64_t InputFileStream::GetCurrentFileOffset() const { return pInFileStream->GetCurrentFileOffset(); }

// ---------------------------------------------------------
std::string InputFileStream::FileExtension() const
{
    std::string stFileName = GetFileName();
    size_t baseNameLength = stFileName.find_last_of('.');
    return baseNameLength != std::string::npos ? stFileName.substr(baseNameLength + 1) : "";
}

// ---------------------------------------------------------
std::string InputFileStream::WCFileExtension() const
{
    size_t BaseNameLength = stwFileName.find_last_of(L'.');
    if (BaseNameLength != std::string::npos)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(stwFileName.substr(BaseNameLength + 1));
    }
    return "";
}

// ---------------------------------------------------------
std::string InputFileStream::GetFileExtension() const { return bEnableWideCharSupport ? WCFileExtension() : FileExtension(); }

// ---------------------------------------------------------
std::string InputFileStream::GetFileName() const { return pInFileStream->GetFileName(); }
