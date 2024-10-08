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
// ! \file multioutputfilestreamunittest.cpp
// ===============================================================================

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "novatel_edie/common/nexcept.hpp"
#include "novatel_edie/stream_interface/multioutputfilestream.hpp"

class MultiOutputFileStreamTest : public ::testing::Test
{
  public:
    void SetUp() override {}

    void TearDown() override {}

    // TODO: this is bad
    [[nodiscard]] bool IsFileSplit() const { return pMyTestCommand.bMyFileSplit; }
    [[nodiscard]] FileStream* GetLocalFileStream() const { return pMyTestCommand.pLocalFileStream; }
    [[nodiscard]] FileSplitMethodEnum GetFileSplitMethodEnum() const { return pMyTestCommand.eMyFileSplitMethodEnum; }
    [[nodiscard]] std::string GetBaseFileName() const { return pMyTestCommand.stMyBaseName; }
    [[nodiscard]] std::string GetExtensionName() const { return pMyTestCommand.stMyExtensionName; }
    [[nodiscard]] std::u32string GetBase32FileName() const { return pMyTestCommand.s32MyBaseName; }
    [[nodiscard]] std::u32string Get32ExtensionName() const { return pMyTestCommand.s32MyExtensionName; }
    [[nodiscard]] uint64_t GetFileSplitSize() const { return pMyTestCommand.ullMyFileSplitSize; }
    [[nodiscard]] uint64_t GetFileSize() const { return pMyTestCommand.ullMyFileSize; }
    [[nodiscard]] uint32_t GetFileCount() const { return pMyTestCommand.uiMyFileCount; }
    [[nodiscard]] double GetTimeSplitSize() const { return pMyTestCommand.dMyTimeSplitSize; }
    [[nodiscard]] double GetTimeInSeconds() const { return pMyTestCommand.dMyTimeInSeconds; }
    [[nodiscard]] double GetStartTimeInSecs() const { return pMyTestCommand.dMyStartTimeInSeconds; }
    [[nodiscard]] uint32_t GetWeek() const { return pMyTestCommand.ulMyWeek; }
    [[nodiscard]] uint32_t GetStartWeek() const { return pMyTestCommand.ulMyStartWeek; }

  protected:
    MultiOutputFileStream pMyTestCommand;
};

// TODO: this test file seems to do no testing at all of the multiple output functionality of this class
TEST_F(MultiOutputFileStreamTest, ConfigureSplitByLog)
{
    pMyTestCommand.ConfigureSplitByLog(true);
    ASSERT_TRUE(IsFileSplit());

    pMyTestCommand.ConfigureSplitByLog(false);
    ASSERT_FALSE(IsFileSplit());
}

// Test the ConfigureBaseFileName method.
TEST_F(MultiOutputFileStreamTest, ConfigureBaseFileName)
{
    pMyTestCommand.ConfigureBaseFileName("multiOutputfilestream_file13");
    ASSERT_STREQ("multiOutputfilestream_file13", GetBaseFileName().c_str());
    ASSERT_STREQ("DefaultExt", GetExtensionName().c_str());

    pMyTestCommand.ConfigureBaseFileName("multiOutputfilestream_file13.txt");
    ASSERT_STREQ("multiOutputfilestream_file13", GetBaseFileName().c_str());
    ASSERT_STREQ("txt", GetExtensionName().c_str());
}

TEST_F(MultiOutputFileStreamTest, ConfigureBase32StringFileName)
{
    pMyTestCommand.ConfigureBaseFileName(U"multiOutputfilestream_不同语言的文件");
    ASSERT_EQ(std::u32string(U"multiOutputfilestream_不同语言的文件"), GetBase32FileName());
    ASSERT_EQ(std::u32string(U"DefaultExt"), Get32ExtensionName());

    pMyTestCommand.ConfigureBaseFileName(U"multiOutputfilestream_不同语言的文件.txt");
    ASSERT_EQ(std::u32string(U"multiOutputfilestream_不同语言的文件"), GetBase32FileName());
    ASSERT_EQ(std::u32string(U"txt"), Get32ExtensionName());
}

// Test the SelectLogFile method.
TEST_F(MultiOutputFileStreamTest, DISABLED_SelectLogFile)
{
    // TODO: this test does nothing but check the file name
    pMyTestCommand.ConfigureBaseFileName("multiOutputfilestream_file14.asc");
    pMyTestCommand.SelectLogFile("bestpos");
    std::string strFileName = GetBaseFileName() + "_" + "bestpos" + "." + GetExtensionName();

    FileStream* pFileStream = GetLocalFileStream();
    ASSERT_EQ(strFileName, pFileStream->GetFileName());

    // pMyTestCommand.ConfigureBaseFileName("multiOutputfilestream_file15");
    // pMyTestCommand.SetExtensionName("DefaultExt");
    // pMyTestCommand.SelectLogFile("bestpos");
    // strFileName = GetBaseFileName() + "_" + "bestpos";
    // printf("myMap size: %d\n", myMap.size());
    // printf("first: %s\n", (itFstreamMapIterator->first).c_str());
    // ASSERT_STREQ(strFileName.c_str(), (itFstreamMapIterator->first).c_str());
}

// Test the SelectLogFile method.
TEST_F(MultiOutputFileStreamTest, DISABLED_Select32StringLogFile)
{
    // TODO: this test does nothing but check the file name
    pMyTestCommand.ConfigureBaseFileName(U"multiOutputfilestream不同_file14.asc");
    pMyTestCommand.SelectWCLogFile("bestpos");
    std::u32string strFileName = GetBase32FileName() + U"_" + U"bestpos" + U"." + Get32ExtensionName();

    FileStream* pFileStream = GetLocalFileStream();
    ASSERT_EQ(strFileName, pFileStream->Get32StringFileName());

    // pMyTestCommand.ConfigureBaseFileName(U"multiOutputfilestream不同_file15");
    // pMyTestCommand.SetExtensionName(U"DefaultExt");
    // pMyTestCommand.SelectLogFile("bestpos");
    // strFileName = GetBase32FileName() + U"_" + U"bestpos";
    // printf("myMap size: %d\n", myMap.size());
    // printf("first: %s\n", (itFstreamMapIterator->first).c_str());
    // ASSERT_STREQ(strFileName.c_str(), (itFstreamMapIterator->first).c_str());
}

TEST_F(MultiOutputFileStreamTest, WriteData)
{
    pMyTestCommand.ConfigureSplitByLog(true);
    pMyTestCommand.ConfigureBaseFileName("Log.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand.WriteData(pcCommand, 5, std::string("BESTPOS"), 0, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.0);
    ASSERT_TRUE(iLen == 5);

    std::ifstream ifile("Log_BESTPOS.txt");
    ASSERT_TRUE(ifile.is_open());
    ifile.close();

    pMyTestCommand.ClearFileStreamMap();
}

TEST_F(MultiOutputFileStreamTest, WriteDataWideFile)
{
    pMyTestCommand.ConfigureSplitByLog(true);
    pMyTestCommand.ConfigureBaseFileName(U"Log不同.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand.WriteData(pcCommand, 5, "BESTPOS", 0, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.0);
    ASSERT_TRUE(iLen == 5);

    std::filesystem::path clUnicodePath(U"Log不同_BESTPOS.txt");

    std::ifstream ifile(clUnicodePath);
    ASSERT_TRUE(ifile.is_open());
    ifile.close();

    pMyTestCommand.ClearWCFileStreamMap();
}

TEST_F(MultiOutputFileStreamTest, ConfigureSplitBySize)
{
    pMyTestCommand.ConfigureSplitBySize(1);
    ASSERT_TRUE(IsFileSplit());
    ASSERT_TRUE(GetFileSplitSize() == 1);

    pMyTestCommand.ConfigureBaseFileName("Log.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand.WriteData(pcCommand, 5, "", 1, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.0);
    ASSERT_TRUE(iLen == 5);

    std::ifstream ifile("Log_Part0.txt");
    ASSERT_TRUE(ifile.is_open());
    ifile.close();

    pMyTestCommand.ClearFileStreamMap();

    ASSERT_THROW(pMyTestCommand.ConfigureSplitBySize(MIN_FILE_SPLIT_SIZE - 1), NExcept);
}

TEST_F(MultiOutputFileStreamTest, ConfigureSplitByTime)
{
    pMyTestCommand.ConfigureSplitByTime(0.01);
    pMyTestCommand.ConfigureBaseFileName("Log.txt");

    char pcCommand[] = "HELLO";
    int32_t iLen = pMyTestCommand.WriteData(pcCommand, 5, "", 0, novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.01);
    ASSERT_TRUE(iLen == 5);

    std::ifstream ifile("Log_Part0.txt");
    ASSERT_TRUE(ifile.is_open());
    ifile.close();

    pMyTestCommand.ClearFileStreamMap();

    ASSERT_THROW(pMyTestCommand.ConfigureSplitByTime(0.0), NExcept);
}

TEST_F(MultiOutputFileStreamTest, SelectTimeFile)
{
    pMyTestCommand.ConfigureSplitByTime(0.01);
    pMyTestCommand.SelectFileStream("Log.txt");
    pMyTestCommand.SelectTimeFile(novatel::edie::TIME_STATUS::UNKNOWN, 0, 0.01);

    ASSERT_NEAR(GetTimeInSeconds(), 0.0, 1E-20);
    ASSERT_NEAR(GetStartTimeInSecs(), 0.0, 1E-20);
    ASSERT_EQ(GetWeek(), 0);
    ASSERT_EQ(GetStartWeek(), 0);

    pMyTestCommand.SelectTimeFile(novatel::edie::TIME_STATUS::SATTIME, 0, 0.0);

    ASSERT_NEAR(GetTimeInSeconds(), 0.0, 1E-20);
    ASSERT_NEAR(GetStartTimeInSecs(), 0.0, 1E-20);
    ASSERT_EQ(GetWeek(), 0);
    ASSERT_EQ(GetStartWeek(), 0);
}
