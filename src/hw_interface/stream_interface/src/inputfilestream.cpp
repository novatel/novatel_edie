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

// Includes
#include "inputfilestream.hpp"

#include <codecvt>

// code
// ---------------------------------------------------------
InputFileStream::InputFileStream(const std::u32string s32FileName_)
{
    pInFileStream = new FileStream(s32FileName_);
    pInFileStream->OpenFile(FileStream::FILEMODES::INPUT);
    pInFileStream->GetFileSize();
    bEnableWideCharSupport = true;
}

// ---------------------------------------------------------
InputFileStream::InputFileStream(const char* pName) : stFileName(pName)
{
    pInFileStream = new FileStream(pName);
    pInFileStream->OpenFile(FileStream::FILEMODES::INPUT);
    pInFileStream->GetFileSize();

    bEnableWideCharSupport = false;
}

// ---------------------------------------------------------
InputFileStream::~InputFileStream() { delete pInFileStream; }

// ---------------------------------------------------------
StreamReadStatus InputFileStream::ReadData(ReadDataStructure& pReadDataStructure)
{
    StreamReadStatus stFileReadStatus;
    stFileReadStatus = pInFileStream->ReadFile(pReadDataStructure.cData, pReadDataStructure.uiDataSize);
    return stFileReadStatus;
}

// ---------------------------------------------------------
StreamReadStatus InputFileStream::ReadLine(std::string& szLine) { return pInFileStream->ReadLine(szLine); }

// ---------------------------------------------------------
void InputFileStream::Reset(std::streamoff offset, std::ios_base::seekdir dir) { pInFileStream->SetFilePosition(offset, dir); }

// ---------------------------------------------------------
uint64_t InputFileStream::GetCurrentFilePosition() { return pInFileStream->GetCurrentFileSize(); }
// ---------------------------------------------------------
void InputFileStream::SetCurrentFileOffset(uint64_t ullCurrentFileOffset) { pInFileStream->SetCurrentFileOffset(ullCurrentFileOffset); }

uint64_t InputFileStream::GetCurrentFileOffset(void) const { return pInFileStream->GetCurrentFileOffset(); }

// ---------------------------------------------------------
std::string InputFileStream::FileExtension()
{
    size_t BaseNameLength = stFileName.find_last_of(".");
    if (BaseNameLength != std::string::npos) { return stFileName.substr(BaseNameLength + 1); }
    return NULL;
}

// ---------------------------------------------------------
std::string InputFileStream::WCFileExtension()
{
    size_t BaseNameLength = stwFileName.find_last_of(L".");
    if (BaseNameLength != std::string::npos)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(stwFileName.substr(BaseNameLength + 1));
    }
    return NULL;
}

// ---------------------------------------------------------
std::string InputFileStream::GetFileExtension()
{
    if (bEnableWideCharSupport)
        return WCFileExtension();
    else
        return FileExtension();
}

// ---------------------------------------------------------
std::string InputFileStream::GetFileName() { return stFileName; }
