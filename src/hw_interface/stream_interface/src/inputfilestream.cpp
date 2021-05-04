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

// code
#ifdef WIDE_CHAR_SUPPORT
// ---------------------------------------------------------
InputFileStream::InputFileStream(const wchar_t* pwFileName)
   :stwFileName(pwFileName)
{
   pInFileStream = new FileStream(pwFileName);
   pInFileStream->OpenFile(FileStream::INPUT);
   pInFileStream->GetFileSize();
   bEnableWideCharSupport = TRUE;
}
#endif
// ---------------------------------------------------------
InputFileStream::InputFileStream(const CHAR* pName)
   :stFileName(pName)
{
   pInFileStream = new FileStream(pName);
   pInFileStream->OpenFile(FileStream::INPUT);
   pInFileStream->GetFileSize();
#ifdef WIDE_CHAR_SUPPORT
   bEnableWideCharSupport = FALSE;
#endif
}

// ---------------------------------------------------------
InputFileStream::~InputFileStream()
{
   delete pInFileStream;
}

// ---------------------------------------------------------
StreamReadStatus InputFileStream::ReadData(ReadDataStructure& pReadDataStructure )
{
   StreamReadStatus stFileReadStatus;
   stFileReadStatus = pInFileStream->ReadFile(pReadDataStructure.cData , pReadDataStructure.uiDataSize );
   return stFileReadStatus;
}

// ---------------------------------------------------------
StreamReadStatus InputFileStream::ReadLine(std::string& szLine)
{
   return pInFileStream->ReadLine(szLine);
}

// ---------------------------------------------------------
void InputFileStream::Reset(std::streamoff offset, std::ios_base::seekdir dir)
{
   pInFileStream->SetFilePosition(offset, dir);
}

// ---------------------------------------------------------
ULONGLONG InputFileStream:: GetCurrentFilePosition()
{
   return pInFileStream->GetCurrentFileSize();
}
// ---------------------------------------------------------
void InputFileStream::SetCurrentFileOffset(ULONGLONG ullCurrentFileOffset) 
{
   pInFileStream->SetCurrentFileOffset(ullCurrentFileOffset);
}

ULONGLONG InputFileStream::GetCurrentFileOffset(void) const
{ 
   return pInFileStream->GetCurrentFileOffset();
}

// ---------------------------------------------------------
std::string InputFileStream::FileExtension()
{
   size_t BaseNameLength = stFileName.find_last_of(".");
   if (BaseNameLength != std::string::npos)
   {
      return stFileName.substr(BaseNameLength + 1);
   }
   return NULL;
}

// ---------------------------------------------------------
#ifdef WIDE_CHAR_SUPPORT
std::string InputFileStream::WCFileExtension()
{
   size_t BaseNameLength = stwFileName.find_last_of(L".");
   if (BaseNameLength != std::string::npos)
   {
      std::wstring wextension = stwFileName.substr(BaseNameLength + 1);
      std::string extension(wextension.begin(), wextension.end());
      return extension;
   }
   return NULL;
}
#endif

// ---------------------------------------------------------
std::string InputFileStream::GetFileExtension()
{
#ifdef WIDE_CHAR_SUPPORT
   if (bEnableWideCharSupport == TRUE)
      return WCFileExtension();
   else
#endif
      return FileExtension();
}
