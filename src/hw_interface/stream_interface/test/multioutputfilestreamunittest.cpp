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
//  DESCRIPTION: Multi Output File Stream Unit Test.
//
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include <gtest/gtest.h>

#include <filesystem>

#include "hw_interface/stream_interface/api/multioutputfilestream.hpp"
#include "string"

class MultiOutputFileStreamTest : public ::testing::Test
{
  public:
    virtual void SetUp() {}

    virtual void TearDown() {}

    typedef std::map<std::string, FileStream*> FstreamMap;
    MultiOutputFileStream::FstreamMap::iterator itFstreamMapIterator;
    MultiOutputFileStream::WCFstreamMap::iterator itWFstreamMapIterator;

    bool IsFileSplit() { return pMyTestCommand->bMyFileSplit; }
    FileStream* GetLocalFileStream() { return pMyTestCommand->pLocalFileStream; }
    FileSplitMethodEnum GetFileSplitMethodEnum() { return pMyTestCommand->eMyFileSplitMethodEnum; }
    std::string GetBaseFileName() { return pMyTestCommand->stMyBaseName; }
    std::string GetExtensionName() { return pMyTestCommand->stMyExtentionName; }
    std::u32string GetBase32FileName() { return pMyTestCommand->s32MyBaseName; }
    std::u32string Get32ExtensionName() { return pMyTestCommand->s32MyExtentionName; }
    uint64_t GetFileSplitSize() { return pMyTestCommand->ullMyFileSplitSize; }
    uint64_t GetFileSize() { return pMyTestCommand->ullMyFileSize; }
    uint32_t GetFileCount() { return pMyTestCommand->uiMyFileCount; }
    double GetTimeSplitSize() { return pMyTestCommand->dMyTimeSplitSize; }
    double GetTimeInSeconds() { return pMyTestCommand->dMyTimeInSeconds; }
    double GetStartTimeInSecs() { return pMyTestCommand->dMyStartTimeInSeconds; }
    uint32_t GetWeek() { return pMyTestCommand->ulMyWeek; }
    uint32_t GetStartWeek() { return pMyTestCommand->ulMyStartWeek; }
    FstreamMap GetMap() { return pMyTestCommand->GetFileMap(); }
    MultiOutputFileStream::WCFstreamMap Get32StringMap() { return pMyTestCommand->Get32FileMap(); }

  private:
  protected:
    MultiOutputFileStream* pMyTestCommand = NULL;
    // typedef std::map<std::string, FileStream* > FstreamMap;
    // FstreamMap mMyFstreamMap;
};

TEST_F(MultiOutputFileStreamTest, ConfigureSplitByLog)
{
    pMyTestCommand = new MultiOutputFileStream();

    pMyTestCommand->ConfigureSplitByLog(true);
    ASSERT_TRUE(IsFileSplit());

    pMyTestCommand->ConfigureSplitByLog(false);
    ASSERT_FALSE(IsFileSplit());

    delete pMyTestCommand;
}

// Test the ConfigureBaseFileName method.
TEST_F(MultiOutputFileStreamTest, ConfigureBaseFileName)
{
    pMyTestCommand = new MultiOutputFileStream();

    pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file13");
    ASSERT_STREQ("multiOutputfilestream_file13", GetBaseFileName().c_str());
    ASSERT_STREQ("DefaultExt", GetExtensionName().c_str());

    pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file13.txt");
    ASSERT_STREQ("multiOutputfilestream_file13", GetBaseFileName().c_str());
    ASSERT_STREQ("txt", GetExtensionName().c_str());

    delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, ConfigureBase32StringFileName)
{
    pMyTestCommand = new MultiOutputFileStream();

    pMyTestCommand->ConfigureBaseFileName(U"multiOutputfilestream_不同语言的文件");
    ASSERT_EQ(std::u32string(U"multiOutputfilestream_不同语言的文件"), GetBase32FileName());
    ASSERT_EQ(std::u32string(U"DefaultExt"), Get32ExtensionName());

    pMyTestCommand->ConfigureBaseFileName(U"multiOutputfilestream_不同语言的文件.txt");
    ASSERT_EQ(std::u32string(U"multiOutputfilestream_不同语言的文件"), GetBase32FileName());
    ASSERT_EQ(std::u32string(U"txt"), Get32ExtensionName());

    delete pMyTestCommand;
}

// Test the SelectLogFile method.
TEST_F(MultiOutputFileStreamTest, SelectLogFile)
{
    typedef std::map<std::string, FileStream*> FstreamMap;
    FstreamMap myMap;

    pMyTestCommand = new MultiOutputFileStream();
    pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file14.asc");

    pMyTestCommand->SelectLogFile("bestpos");
    myMap = GetMap();
    itFstreamMapIterator = myMap.begin();

    std::string strFileName = GetBaseFileName() + "_" + "bestpos" + "." + GetExtensionName();
    ASSERT_STREQ(strFileName.c_str(), (itFstreamMapIterator->first).c_str());

    ASSERT_EQ(strFileName, itFstreamMapIterator->second->GetFileName());

    pMyTestCommand->ConfigureBaseFileName("multiOutputfilestream_file15");
    pMyTestCommand->SetExtensionName("DefaultExt");
    pMyTestCommand->SelectLogFile("bestpos");

    itFstreamMapIterator++;
    strFileName = GetBaseFileName() + "_" + "bestpos";

    // Need to improve further here
    // printf("myMap size: %d\n", myMap.size());
    // printf("first: %s\n", (itFstreamMapIterator->first).c_str());
    // ASSERT_STREQ(strFileName.c_str(), (itFstreamMapIterator->first).c_str());
    delete pMyTestCommand;
}

// Test the SelectLogFile method.
TEST_F(MultiOutputFileStreamTest, Select32StringLogFile)
{
    typedef std::map<std::u32string, FileStream*> WCFstreamMap;
    WCFstreamMap myMap;

    pMyTestCommand = new MultiOutputFileStream();
    pMyTestCommand->ConfigureBaseFileName(U"multiOutputfilestream不同_file14.asc");

    pMyTestCommand->SelectWCLogFile("bestpos");
    myMap = Get32StringMap();
    itWFstreamMapIterator = myMap.begin();

    std::u32string strFileName = GetBase32FileName() + U"_" + U"bestpos" + U"." + Get32ExtensionName();
    ASSERT_EQ(strFileName, (itWFstreamMapIterator->first));

    ASSERT_EQ(strFileName, itWFstreamMapIterator->second->Get32StringFileName());

    pMyTestCommand->ConfigureBaseFileName(U"multiOutputfilestream不同_file15");
    pMyTestCommand->SetExtensionName(U"DefaultExt");
    pMyTestCommand->SelectLogFile("bestpos");

    itWFstreamMapIterator++;
    strFileName = GetBase32FileName() + U"_" + U"bestpos";

    // Need to improve further here
    // printf("myMap size: %d\n", myMap.size());
    // printf("first: %s\n", (itFstreamMapIterator->first).c_str());
    // ASSERT_STREQ(strFileName.c_str(), (itFstreamMapIterator->first).c_str());
    delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, WriteData)
{
    pMyTestCommand = new MultiOutputFileStream();
    pMyTestCommand->ConfigureSplitByLog(true);
    pMyTestCommand->ConfigureBaseFileName("Log.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand->WriteData(pcCommand, 5, std::string("BESTPOS"), 0, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.0);
    ASSERT_TRUE(iLen == 5);

    std::ifstream ifile("Log_BESTPOS.txt");
    if (ifile)
    {
        // The file exists, and is open for input
    }
    else
        ASSERT_TRUE(4 == 5); // Simple fails

    ifile.close();

    pMyTestCommand->ClearFileStreamMap();
}

TEST_F(MultiOutputFileStreamTest, WriteDataWideFile)
{
    pMyTestCommand = new MultiOutputFileStream();
    pMyTestCommand->ConfigureSplitByLog(true);
    pMyTestCommand->ConfigureBaseFileName(U"Log不同.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand->WriteData(pcCommand, 5, "BESTPOS", 0, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.0);
    ASSERT_TRUE(iLen == 5);

    std::filesystem::path clUnicodePath(U"Log不同_BESTPOS.txt");

    std::ifstream ifile(clUnicodePath);
    if (ifile)
    {
        // The file exists, and is open for input
    }
    else
        ASSERT_TRUE(4 == 5); // Simple fails

    ifile.close();

    pMyTestCommand->ClearWCFileStreamMap();
}

TEST_F(MultiOutputFileStreamTest, ConfigureSplitBySize)
{
    pMyTestCommand = new MultiOutputFileStream();
    pMyTestCommand->ConfigureSplitBySize(1);
    ASSERT_TRUE(IsFileSplit());
    ASSERT_TRUE(GetFileSplitSize() == 1);

    pMyTestCommand->ConfigureBaseFileName("Log.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand->WriteData(pcCommand, 5, "", 1, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.0);
    ASSERT_TRUE(iLen == 5);

    std::ifstream ifile("Log_Part0.txt");
    if (ifile)
    {
        // The file exists, and is open for input
    }
    else
        ASSERT_TRUE(4 == 5); // Simple fails

    ifile.close();

    pMyTestCommand->ClearFileStreamMap();

    try
    {
        pMyTestCommand->ConfigureSplitBySize(MIN_FILE_SPLIT_SIZE - 1); // 0
        ASSERT_TRUE(4 == 1);                                           // Should not execute
    }
    catch (nExcept e)
    {
        ASSERT_STREQ("File Split by Size not valid", e.buffer);
    }

    delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, ConfigureSplitByTime)
{
    pMyTestCommand = new MultiOutputFileStream();
    pMyTestCommand->ConfigureSplitByTime(0.01);
    pMyTestCommand->ConfigureBaseFileName("Log.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand->WriteData(pcCommand, 5, "", 0, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.01);
    ASSERT_TRUE(iLen == 5);

    std::ifstream ifile("Log_Part0.txt");
    if (ifile)
    {
        // The file exists, and is open for input
    }
    else
        ASSERT_TRUE(4 == 5); // Simpley fails

    ifile.close();

    pMyTestCommand->ClearFileStreamMap();

    try
    {
        pMyTestCommand->ConfigureSplitByTime(0.0); // 0
        ASSERT_TRUE(4 == 1);                       // Should not execute
    }
    catch (nExcept e)
    {
        ASSERT_STREQ("File Split by time not valid", e.buffer);
    }
    delete pMyTestCommand;
}

TEST_F(MultiOutputFileStreamTest, SelectTimeFile)
{
    pMyTestCommand = new MultiOutputFileStream();
    pMyTestCommand->ConfigureSplitByTime(0.01);
    pMyTestCommand->SelectFileStream("Log.txt");
    pMyTestCommand->SelectTimeFile(novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.01);

    ASSERT_TRUE(GetTimeInSeconds() == 0.0);
    ASSERT_TRUE(GetStartTimeInSecs() == 0.0);
    ASSERT_TRUE(GetWeek() == 0);
    ASSERT_TRUE(GetStartWeek() == 0);

    pMyTestCommand->SelectTimeFile(novatel::edie::TIME_STATUS::SATTIME, 0, 0.0);

    ASSERT_TRUE(GetTimeInSeconds() == 0.0);
    ASSERT_TRUE(GetStartTimeInSecs() == 0.0);
    ASSERT_TRUE(GetWeek() == 0);
    ASSERT_TRUE(GetStartWeek() == 0);

    delete pMyTestCommand;
}
