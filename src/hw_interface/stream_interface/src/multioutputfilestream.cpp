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
#include "multioutputfilestream.hpp"

// code
// ---------------------------------------------------------
MultiOutputFileStream::MultiOutputFileStream()
	:mMyFstreamMap(),
#ifdef WIDE_CHAR_SUPPORT
	wmMyFstreamMap(),
	wstMyBaseName(L"DefaultBase"),
	wstMyExtentionName(L"DefaultExt"),
	bEnableWideCharSupport(FALSE),
#endif
	bMyFileSplit(FALSE),
	eMyFileSplitMethodEnum(SPLIT_NONE),
	stMyBaseName("DefaultBase"),
	stMyExtentionName("DefaultExt"),
	ullMyFileSplitSize(0),
	ullMyFileSize(0),
	uiMyFileCount(0),
	dMyTimeSplitSize(0.0),
	dMyTimeInSeconds(0.0),
	dMyStartTimeInSeconds(0.0),
	ulMyStartWeek(0),
	ulMyWeek(0),
	pLocalFileStream(nullptr),
	pMyMessageDataFilter(NULL)
{
}
// ---------------------------------------------------------
MultiOutputFileStream::MultiOutputFileStream(MessageDataFilter& rMessageDataFilter)
	:mMyFstreamMap(),
#ifdef WIDE_CHAR_SUPPORT
	wmMyFstreamMap(),
	wstMyBaseName(L"DefaultBase"),
	wstMyExtentionName(L"DefaultExt"),
	bEnableWideCharSupport(FALSE),
#endif
	pLocalFileStream(nullptr),
	pMyMessageDataFilter(&rMessageDataFilter),
	bMyFileSplit(FALSE),
	eMyFileSplitMethodEnum(SPLIT_NONE),
	stMyBaseName("DefaultBase"),
	stMyExtentionName("DefaultExt"),
	ullMyFileSplitSize(0),
	ullMyFileSize(0),
	uiMyFileCount(0),
	dMyTimeSplitSize(0.0),
	dMyTimeInSeconds(0.0),
	dMyStartTimeInSeconds(0.0),
	ulMyStartWeek(0),
	ulMyWeek(0)
{

}

// ---------------------------------------------------------
MultiOutputFileStream::~MultiOutputFileStream()
{
	ClearFileStreamMap();
#ifdef WIDE_CHAR_SUPPORT
	if (bEnableWideCharSupport == TRUE)
		ClearWCFileStreamMap();
#endif
	pMyMessageDataFilter = NULL;
}

// ---------------------------------------------------------
#ifdef WIDE_CHAR_SUPPORT
void MultiOutputFileStream::SelectFileStream(std::wstring stFileName)
{
	bEnableWideCharSupport = TRUE;
	WCFstreamMap::iterator itFstreamMapIterator = wmMyFstreamMap.find(stFileName);
	if (itFstreamMapIterator != wmMyFstreamMap.end())
	{
		pLocalFileStream = itFstreamMapIterator->second;
	}
	else
	{
		pLocalFileStream = new FileStream(stFileName.c_str());
		pLocalFileStream->OpenFile(FileStream::OUTPUT);
		wmMyFstreamMap.emplace(std::pair <std::wstring, FileStream*>(stFileName, pLocalFileStream));
	}
}
#endif

void MultiOutputFileStream::SelectFileStream(std::string stFileName)
{
	FstreamMap::iterator itFstreamMapIterator = mMyFstreamMap.find(stFileName);
	if (itFstreamMapIterator != mMyFstreamMap.end())
	{
		pLocalFileStream = itFstreamMapIterator->second;
	}
	else
	{
		pLocalFileStream = new FileStream(stFileName.c_str());
		pLocalFileStream->OpenFile(FileStream::OUTPUT);
		mMyFstreamMap.emplace(std::pair <std::string, FileStream*>(stFileName, pLocalFileStream));
	}
}

#ifdef WIDE_CHAR_SUPPORT
// ---------------------------------------------------------
void MultiOutputFileStream::ClearWCFileStreamMap()
{
	for (WCFstreamMap::iterator itFstreamMapIterator = wmMyFstreamMap.begin(); itFstreamMapIterator != wmMyFstreamMap.end();)
	{
		if (itFstreamMapIterator->second)
		{
			delete itFstreamMapIterator->second;
		}
		itFstreamMapIterator = wmMyFstreamMap.erase(itFstreamMapIterator);
	}
}
#endif

// ---------------------------------------------------------
void MultiOutputFileStream::ClearFileStreamMap()
{
	for (FstreamMap::iterator itFstreamMapIterator = mMyFstreamMap.begin(); itFstreamMapIterator != mMyFstreamMap.end();)
	{
		if (itFstreamMapIterator->second)
		{
			delete itFstreamMapIterator->second;
		}
		itFstreamMapIterator = mMyFstreamMap.erase(itFstreamMapIterator);
	}
}
// ---------------------------------------------------------
void MultiOutputFileStream::ConfigureSplitByLog(BOOL bStatus)
{
	if (bStatus == TRUE)
	{
		bMyFileSplit = TRUE;
		eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_LOG;
	}
	else
	{
		bMyFileSplit = FALSE;
		eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_NONE;
	}
}

// ---------------------------------------------------------
#ifdef WIDE_CHAR_SUPPORT
void MultiOutputFileStream::ConfigureBaseFileName(std::wstring stFileName)
{
	bEnableWideCharSupport = TRUE;
	size_t BaseNameLength = stFileName.find_last_of(L".");
	if (BaseNameLength != std::string::npos)
	{
		wstMyBaseName = stFileName.substr(0, BaseNameLength);
		wstMyExtentionName = stFileName.substr(BaseNameLength + 1);
	}
	else
	{
		wstMyBaseName = stFileName;
	}
}
#endif

void MultiOutputFileStream::ConfigureBaseFileName(std::string stFileName)
{
	size_t BaseNameLength = stFileName.find_last_of(".");
	if (BaseNameLength != std::string::npos)
	{
		stMyBaseName = stFileName.substr(0, BaseNameLength);
		stMyExtentionName = stFileName.substr(BaseNameLength + 1);
	}
	else
	{
		stMyBaseName = stFileName;
	}
}

#ifdef WIDE_CHAR_SUPPORT
// ---------------------------------------------------------
void MultiOutputFileStream::SelectWCLogFile(BaseMessageData& pBaseMessageData)
{
	std::string sTempName(pBaseMessageData.getMessageName());
	std::wstring wstMessageName(sTempName.begin(), sTempName.end());
	std::wstring wstLocalBaseName = wstMyBaseName;
	std::wstring wstLocalExtensionName = wstMyExtentionName;
	if (wstMyExtentionName != L"DefaultExt")
	{
		SelectFileStream(wstLocalBaseName + L"_" + wstMessageName + L"." + wstLocalExtensionName);
	}
	else
	{
		SelectFileStream(wstMyBaseName + L"_" + wstMessageName);
	}
}

#endif

// ---------------------------------------------------------
void MultiOutputFileStream::SelectLogFile(BaseMessageData& pBaseMessageData)
{
	std::string stMessageName = pBaseMessageData.getMessageName();
	std::string stLocalBaseName = stMyBaseName;
	std::string stLocalExtensionName = stMyExtentionName;
	if (stMyExtentionName != "DefaultExt")
	{
		SelectFileStream(stLocalBaseName + "_" + stMessageName + "." + stLocalExtensionName);
	}
	else
	{
		SelectFileStream(stMyBaseName + "_" + stMessageName);
	}
}

// ---------------------------------------------------------
void MultiOutputFileStream::ConfigureSplitBySize(ULONGLONG ullFileSplitSize)
{
	bMyFileSplit = TRUE;
	eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_SIZE;

	if (ullFileSplitSize >= MIN_FILE_SPLIT_SIZE)
	{
		ullMyFileSplitSize = ullFileSplitSize;
	}
	else
	{
		throw nExcept("File Split by Size not valid");
	}

}

#ifdef WIDE_CHAR_SUPPORT
// ---------------------------------------------------------
void MultiOutputFileStream::SelectWCSizeFile(BaseMessageData& pBaseMessageData)
{
	if (ullMyFileSplitSize >= MIN_FILE_SPLIT_SIZE)
	{
		if (ullMyFileSize >= ullMyFileSplitSize*MBYTE_TO_BYTE)
		{
			ullMyFileSize = 0;
			ClearWCFileStreamMap();
			uiMyFileCount = uiMyFileCount + 1;
		}
		if (ullMyFileSize == 0)
		{
			if (wstMyExtentionName != L"DefaultExt")
			{
				SelectFileStream(wstMyBaseName + L"_Part" + std::to_wstring(uiMyFileCount).c_str() + L"." + wstMyExtentionName);
			}
			else
			{
				SelectFileStream(wstMyBaseName + L"_Part" + std::to_wstring(uiMyFileCount).c_str());
			}
		}
		ullMyFileSize = ullMyFileSize + pBaseMessageData.getMessageLength();
	}
}
#endif

// ---------------------------------------------------------
void MultiOutputFileStream::SelectSizeFile(BaseMessageData& pBaseMessageData)
{
	if (ullMyFileSplitSize >= MIN_FILE_SPLIT_SIZE)
	{
		if (ullMyFileSize >= ullMyFileSplitSize*MBYTE_TO_BYTE)
		{
			ullMyFileSize = 0;
			ClearFileStreamMap();
			uiMyFileCount = uiMyFileCount + 1;
		}
		if (ullMyFileSize == 0)
		{
			if (stMyExtentionName != "DefaultExt")
			{
				SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount).c_str() + "." + stMyExtentionName);
			}
			else
			{
				SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount).c_str());
			}
		}
		ullMyFileSize = ullMyFileSize + pBaseMessageData.getMessageLength();
	}
}

// ---------------------------------------------------------
void MultiOutputFileStream::ConfigureSplitByTime(DOUBLE FileSplitTime)
{
	bMyFileSplit = TRUE;
	eMyFileSplitMethodEnum = FileSplitMethodEnum::SPLIT_TIME;

	if (FileSplitTime*HR_TO_SEC >= MIN_TIME_SPLIT_SEC)
	{
		dMyTimeSplitSize = FileSplitTime;
	}
	else
	{
		throw nExcept("File Split by time not valid");
	}
}

#ifdef WIDE_CHAR_SUPPORT
// ---------------------------------------------------------
void MultiOutputFileStream::SelectWCTimeFile(BaseMessageData& pBaseMessageData)
{
	// If a file already exist, write the UNKNOWN and SATTIME log into that.
	// Dont consider these time stutus for calculation.
	if (pLocalFileStream != NULL)
	{
		if ((pBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_UNKNOWN) ||
			(pBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_SATTIME))
			return;
	}
	if (dMyTimeSplitSize*HR_TO_SEC >= MIN_TIME_SPLIT_SEC)
	{
		if (ulMyStartWeek < ulMyWeek)
		{
			ulMyStartWeek = ulMyWeek;
			dMyStartTimeInSeconds -= SECS_IN_WEEK;
		}
		if ((dMyTimeInSeconds - dMyStartTimeInSeconds) >= dMyTimeSplitSize*HR_TO_SEC)
		{
			if (!IsEqual(dMyTimeInSeconds, pBaseMessageData.getMessageTimeMilliSeconds() / 1000.0))
			{
				dMyStartTimeInSeconds = 0.0;
				ulMyStartWeek = 0;
				ClearWCFileStreamMap();
				uiMyFileCount = uiMyFileCount + 1;
			}
		}
		if (dMyStartTimeInSeconds == 0.0)
		{
			if (wstMyExtentionName != L"DefaultExt")
			{
				SelectFileStream(wstMyBaseName + L"_Part" + std::to_wstring(uiMyFileCount).c_str() + L"." + wstMyExtentionName);
			}
			else
			{
				SelectFileStream(wstMyBaseName + L"_Part" + std::to_wstring(uiMyFileCount).c_str());
			}
			dMyStartTimeInSeconds = pBaseMessageData.getMessageTimeMilliSeconds() / 1000.0;
			ulMyStartWeek = pBaseMessageData.getMessageTimeWeek();
		}
		dMyTimeInSeconds = pBaseMessageData.getMessageTimeMilliSeconds() / 1000.0;
		ulMyWeek = pBaseMessageData.getMessageTimeWeek();
	}
}
#endif

// ---------------------------------------------------------
void MultiOutputFileStream::SelectTimeFile(BaseMessageData& pBaseMessageData)
{
	// If a file already exist, write the UNKNOWN and SATTIME log into that.
	// Dont consider these time stutus for calculation.
	if (pLocalFileStream != NULL)
	{
		if ((pBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_UNKNOWN) ||
			(pBaseMessageData.getMessageTimeStatus() == MessageTimeStatusEnum::TIME_SATTIME))
			return;
	}
	if (dMyTimeSplitSize*HR_TO_SEC >= MIN_TIME_SPLIT_SEC)
	{
		if (ulMyStartWeek < ulMyWeek)
		{
			ulMyStartWeek = ulMyWeek;
			dMyStartTimeInSeconds -= SECS_IN_WEEK;
		}
		if ((dMyTimeInSeconds - dMyStartTimeInSeconds) >= dMyTimeSplitSize*HR_TO_SEC)
		{
			if (!IsEqual(dMyTimeInSeconds, pBaseMessageData.getMessageTimeMilliSeconds() / 1000.0))
			{
				dMyStartTimeInSeconds = 0.0;
				ulMyStartWeek = 0;
				ClearFileStreamMap();
				uiMyFileCount = uiMyFileCount + 1;
			}
		}
		if (dMyStartTimeInSeconds == 0.0)
		{
			if (stMyExtentionName != "DefaultExt")
			{
				SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount).c_str() + "." + stMyExtentionName);
			}
			else
			{
				SelectFileStream(stMyBaseName + "_Part" + std::to_string(uiMyFileCount).c_str());
			}
			dMyStartTimeInSeconds = pBaseMessageData.getMessageTimeMilliSeconds() / 1000.0;
			ulMyStartWeek = pBaseMessageData.getMessageTimeWeek();
		}
		dMyTimeInSeconds = pBaseMessageData.getMessageTimeMilliSeconds() / 1000.0;
		ulMyWeek = pBaseMessageData.getMessageTimeWeek();
	}
}

// ---------------------------------------------------------
UINT MultiOutputFileStream::WriteData(BaseMessageData& pBaseMessageData)
{
	UINT uiReturn = 0;
	if (bMyFileSplit == TRUE)
	{
		switch (eMyFileSplitMethodEnum)
		{
		case SPLIT_LOG:
#ifdef WIDE_CHAR_SUPPORT
			if (bEnableWideCharSupport == TRUE)
				SelectWCLogFile(pBaseMessageData);
			else
#endif
				SelectLogFile(pBaseMessageData);
			break;
		case SPLIT_SIZE:
#ifdef WIDE_CHAR_SUPPORT
			if (bEnableWideCharSupport == TRUE)
				SelectWCSizeFile(pBaseMessageData);
			else
#endif
				SelectSizeFile(pBaseMessageData);
			break;
		case SPLIT_TIME:
#ifdef WIDE_CHAR_SUPPORT
			if (bEnableWideCharSupport == TRUE)
				SelectWCTimeFile(pBaseMessageData);
			else
#endif
				SelectTimeFile(pBaseMessageData);
			break;
		default:
			break;
		}
	}
	if (pLocalFileStream != NULL)
	{
		if (pMyMessageDataFilter != NULL)
		{
			if (pMyMessageDataFilter->Filter(pBaseMessageData) == TRUE)
			{
				uiReturn = pLocalFileStream->WriteFile(pBaseMessageData.getMessageData(), pBaseMessageData.getMessageLength());
			}
		}
		else
		{
			uiReturn = pLocalFileStream->WriteFile(pBaseMessageData.getMessageData(), pBaseMessageData.getMessageLength());
		}
	}
	return uiReturn;
}
