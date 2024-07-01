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
// ! \file multioutputfilestream.cpp
// ===============================================================================

#include "novatel_edie/stream_interface/multioutputfilestream.hpp"

#include <iterator>
#include <map>

#include "novatel_edie/common/nexcept.hpp"

// ---------------------------------------------------------
MultiOutputFileStream::~MultiOutputFileStream()
{
    ClearWCFileStreamMap();
    ClearFileStreamMap();

    if (bEnableWideCharSupport) { ClearWCFileStreamMap(); }
}

// ---------------------------------------------------------
// #ifdef WIDE_CHAR_SUPPORT
void MultiOutputFileStream::SelectFileStream(const std::u32string& s32FileName_)
{
    bEnableWideCharSupport = true;
    auto itFstreamMapIterator = wmMyFstreamMap.find(s32FileName_);
    if (itFstreamMapIterator != wmMyFstreamMap.end()) { pLocalFileStream = itFstreamMapIterator->second; }
    else
    {
        pLocalFileStream = new FileStream(s32FileName_);
        pLocalFileStream->OpenFile(FileStream::FILE_MODES::OUTPUT);
        wmMyFstreamMap.emplace(std::pair(s32FileName_, pLocalFileStream));
    }
}
// #endif

void MultiOutputFileStream::SelectFileStream(const std::string& stFileName)
{
    auto itFstreamMapIterator = mMyFstreamMap.find(stFileName);
    if (itFstreamMapIterator != mMyFstreamMap.end()) { pLocalFileStream = itFstreamMapIterator->second; }
    else
    {
        pLocalFileStream = new FileStream(stFileName.c_str());
        pLocalFileStream->OpenFile(FileStream::FILE_MODES::OUTPUT);
        mMyFstreamMap.emplace(std::pair(stFileName, pLocalFileStream));
    }
}

// #ifdef WIDE_CHAR_SUPPORT
//  ---------------------------------------------------------
void MultiOutputFileStream::ClearWCFileStreamMap()
{
    for (auto itFstreamMapIterator = wmMyFstreamMap.begin(); itFstreamMapIterator != wmMyFstreamMap.end();)
    {
        delete itFstreamMapIterator->second;
        itFstreamMapIterator = wmMyFstreamMap.erase(itFstreamMapIterator);
    }
}
// #endif

// ---------------------------------------------------------
void MultiOutputFileStream::ClearFileStreamMap()
{
    for (auto itFstreamMapIterator = mMyFstreamMap.begin(); itFstreamMapIterator != mMyFstreamMap.end();)
    {
        delete itFstreamMapIterator->second;
        itFstreamMapIterator = mMyFstreamMap.erase(itFstreamMapIterator);
    }
}

// ---------------------------------------------------------
void MultiOutputFileStream::ConfigureSplitByLog(bool bStatus)
{
    if (bStatus)
    {
        bMyFileSplit = true;
        eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_LOG;
    }
    else
    {
        bMyFileSplit = false;
        eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_NONE;
    }
}

// ---------------------------------------------------------
void MultiOutputFileStream::ConfigureBaseFileName(const std::u32string& s32FileName_)
{
    bEnableWideCharSupport = true;
    size_t BaseNameLength = s32FileName_.find_last_of(U'.');
    if (BaseNameLength != std::u32string::npos)
    {
        s32MyBaseName = s32FileName_.substr(0, BaseNameLength);
        s32MyExtensionName = s32FileName_.substr(BaseNameLength + 1);
    }
    else { s32MyBaseName = s32FileName_; }
}

void MultiOutputFileStream::ConfigureBaseFileName(const std::string& stFileName)
{
    size_t BaseNameLength = stFileName.find_last_of('.');
    if (BaseNameLength != std::string::npos)
    {
        stMyBaseName = stFileName.substr(0, BaseNameLength);
        stMyExtensionName = stFileName.substr(BaseNameLength + 1);
    }
    else { stMyBaseName = stFileName; }
}

// ---------------------------------------------------------
void MultiOutputFileStream::SelectWCLogFile(std::string strMsgName_)
{
    bEnableWideCharSupport = true;
    std::u32string wstMessageName(strMsgName_.begin(), strMsgName_.end());
    std::u32string wstLocalBaseName = s32MyBaseName;
    std::u32string wstLocalExtensionName = s32MyExtensionName;
    if (s32MyExtensionName != U"DefaultExt") { SelectFileStream(wstLocalBaseName + U"_" + wstMessageName + U"." + wstLocalExtensionName); }
    else { SelectFileStream(s32MyBaseName + U"_" + wstMessageName); }
}

// ---------------------------------------------------------
void MultiOutputFileStream::SelectLogFile(const std::string& strMsgName_)
{
    std::string stLocalBaseName = stMyBaseName;
    std::string stLocalExtensionName = stMyExtensionName;
    if (stMyExtensionName != "DefaultExt") { SelectFileStream(stLocalBaseName + "_" + strMsgName_ + "." + stLocalExtensionName); }
    else { SelectFileStream(stMyBaseName + "_" + strMsgName_); }
}

// ---------------------------------------------------------
void MultiOutputFileStream::ConfigureSplitBySize(uint64_t ullFileSplitSize)
{
    bMyFileSplit = true;
    eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_SIZE;

    if (ullFileSplitSize >= MIN_FILE_SPLIT_SIZE) { ullMyFileSplitSize = ullFileSplitSize; }
    else { throw NExcept("File Split by Size not valid"); }
}

// ---------------------------------------------------------
void MultiOutputFileStream::SelectWCSizeFile(uint32_t uiSize_)
{
    if (ullMyFileSplitSize >= MIN_FILE_SPLIT_SIZE)
    {
        if (ullMyFileSize >= ullMyFileSplitSize * MBYTE_TO_BYTE)
        {
            ullMyFileSize = 0;
            ClearWCFileStreamMap();
            uiMyFileCount = uiMyFileCount + 1;
        }
        if (ullMyFileSize == 0)
        {
            std::string sSplitNum = std::to_string(uiMyFileCount);
            if (s32MyExtensionName != U"DefaultExt")
            {
                SelectFileStream(s32MyBaseName + U"_Part" + std::u32string(sSplitNum.begin(), sSplitNum.end()) + U"." + s32MyExtensionName);
            }
            else { SelectFileStream(s32MyBaseName + U"_Part" + std::u32string(sSplitNum.begin(), sSplitNum.end())); }
        }
        ullMyFileSize = ullMyFileSize + uiSize_;
    }
}

// ---------------------------------------------------------
void MultiOutputFileStream::SelectSizeFile(uint32_t uiSize_)
{
    if (ullMyFileSplitSize >= MIN_FILE_SPLIT_SIZE)
    {
        if (ullMyFileSize >= ullMyFileSplitSize * MBYTE_TO_BYTE)
        {
            ullMyFileSize = 0;
            ClearFileStreamMap();
            uiMyFileCount = uiMyFileCount + 1;
        }
        if (ullMyFileSize == 0)
        {
            if (stMyExtensionName != "DefaultExt")
            {
                SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount) + "." + stMyExtensionName);
            }
            else { SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount)); }
        }
        ullMyFileSize = ullMyFileSize + uiSize_;
    }
}

// ---------------------------------------------------------
void MultiOutputFileStream::ConfigureSplitByTime(double FileSplitTime)
{
    bMyFileSplit = true;
    eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_TIME;

    if (FileSplitTime * HR_TO_SEC >= MIN_TIME_SPLIT_SEC) { dMyTimeSplitSize = FileSplitTime; }
    else { throw NExcept("File Split by time not valid"); }
}

// ---------------------------------------------------------
void MultiOutputFileStream::SelectWCTimeFile(novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_)
{
    // If a file already exist, write the UNKNOWN and SATTIME log into that.
    // Dont consider these time status for calculation.
    if (pLocalFileStream != nullptr)
    {
        if ((eStatus_ == novatel::edie::TIME_STATUS::UNKNOWN) || (eStatus_ == novatel::edie::TIME_STATUS::SATTIME)) { return; }
    }
    if (dMyTimeSplitSize * HR_TO_SEC >= MIN_TIME_SPLIT_SEC)
    {
        if (ulMyStartWeek < ulMyWeek)
        {
            ulMyStartWeek = ulMyWeek;
            dMyStartTimeInSeconds -= SECS_IN_WEEK;
        }
        if ((dMyTimeInSeconds - dMyStartTimeInSeconds) >= dMyTimeSplitSize * HR_TO_SEC)
        {
            if (!IsEqual(dMyTimeInSeconds, dMilliseconds_ / 1000.0))
            {
                dMyStartTimeInSeconds = 0.0;
                ulMyStartWeek = 0;
                ClearWCFileStreamMap();
                uiMyFileCount = uiMyFileCount + 1;
            }
        }
        if (dMyStartTimeInSeconds == 0.0)
        {
            std::string sSplitNum = std::to_string(uiMyFileCount);
            if (s32MyExtensionName != U"DefaultExt")
            {
                SelectFileStream(s32MyBaseName + U"_Part" + std::u32string(sSplitNum.begin(), sSplitNum.end()) + U"." + s32MyExtensionName);
            }
            else { SelectFileStream(s32MyBaseName + U"_Part" + std::u32string(sSplitNum.begin(), sSplitNum.end())); }
            dMyStartTimeInSeconds = dMilliseconds_ / 1000.0;
            ulMyStartWeek = static_cast<uint32_t>(usWeek_);
        }
        dMyTimeInSeconds = dMilliseconds_ / 1000.0;
        ulMyWeek = static_cast<uint32_t>(usWeek_);
    }
}

// ---------------------------------------------------------
void MultiOutputFileStream::SelectTimeFile(novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_)
{
    // If a file already exist, write the UNKNOWN and SATTIME log into that.
    // Don't consider these time status for calculation.
    if (pLocalFileStream != nullptr)
    {
        if (eStatus_ == novatel::edie::TIME_STATUS::UNKNOWN || eStatus_ == novatel::edie::TIME_STATUS::SATTIME) { return; }
    }
    if (dMyTimeSplitSize * HR_TO_SEC >= MIN_TIME_SPLIT_SEC)
    {
        if (ulMyStartWeek < ulMyWeek)
        {
            ulMyStartWeek = ulMyWeek;
            dMyStartTimeInSeconds -= SECS_IN_WEEK;
        }
        if ((dMyTimeInSeconds - dMyStartTimeInSeconds) >= dMyTimeSplitSize * HR_TO_SEC)
        {
            if (!IsEqual(dMyTimeInSeconds, dMilliseconds_ / 1000.0))
            {
                dMyStartTimeInSeconds = 0.0;
                ulMyStartWeek = 0;
                ClearFileStreamMap();
                uiMyFileCount = uiMyFileCount + 1;
            }
        }
        if (dMyStartTimeInSeconds == 0.0)
        {
            if (stMyExtensionName != "DefaultExt")
            {
                SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount) + "." + stMyExtensionName);
            }
            else { SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount)); }
            dMyStartTimeInSeconds = dMilliseconds_ / 1000.0;
            ulMyStartWeek = static_cast<uint32_t>(usWeek_);
        }
        dMyTimeInSeconds = dMilliseconds_ / 1000.0;
        ulMyWeek = static_cast<uint32_t>(usWeek_);
    }
}

// ---------------------------------------------------------
uint32_t MultiOutputFileStream::WriteData(const char* pcData_, uint32_t uiDataLength_, const std::string& strMsgName_, uint32_t uiSize_,
                                          novatel::edie::TIME_STATUS eStatus_, uint16_t usWeek_, double dMilliseconds_)
{
    if (bMyFileSplit)
    {
        switch (eMyFileSplitMethodEnum)
        {
        case SPLIT_LOG:
            if (bEnableWideCharSupport) { SelectWCLogFile(strMsgName_); }
            else { SelectLogFile(strMsgName_); }
            break;

        case SPLIT_SIZE:
            if (bEnableWideCharSupport) { SelectWCSizeFile(uiSize_); }
            else { SelectSizeFile(uiSize_); }
            break;

        case SPLIT_TIME:
            if (bEnableWideCharSupport) { SelectWCTimeFile(eStatus_, usWeek_, dMilliseconds_); }
            else { SelectTimeFile(eStatus_, usWeek_, dMilliseconds_); }
            break;
        default: break;
        }
    }
    return WriteData(pcData_, uiDataLength_);
}

// ---------------------------------------------------------
uint32_t MultiOutputFileStream::WriteData(const char* pcData_, uint32_t uiDataLength_)
{
    return pLocalFileStream ? pLocalFileStream->WriteFile(pcData_, uiDataLength_) : 0;
}
