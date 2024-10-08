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
// ! \file filestreamunittest.cpp
// ===============================================================================

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "novatel_edie/common/nexcept.hpp"
#include "novatel_edie/stream_interface/filestream.hpp"

class FileStreamTest : public ::testing::Test
{
  public:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(FileStreamTest, Constructors)
{
    FileStream pMyTestCommand("filestream_file1.asc");
    std::string testString = "filestream_file1.asc";
    ASSERT_EQ(testString, pMyTestCommand.GetFileName());

    try
    {
        FileStream pMyTestCommand1(nullptr);
    }
    catch (NExcept ne)
    {
        ASSERT_STREQ(ne.buffer, "file name is not valid");
    }
}

TEST_F(FileStreamTest, ConstructorWideChar)
{
    FileStream pMyTestCommand(std::u32string(U"filestream不同语言的文件.gps"));
    const std::u32string test32String = U"filestream不同语言的文件.gps";

    // const char32_t* test = pMyTestCommand.GetWCFileName();
    // C++17 implicitly converts char32_t type values into int type values. This line will likely
    // break in C++20
    ASSERT_EQ(test32String, pMyTestCommand.Get32StringFileName());
}

TEST_F(FileStreamTest, WideCharOpenFile)
{
    try
    {
        FileStream pMyTestCommand(
            std::u32string((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / U"filestream不同语言的文件.gps").generic_u32string()));
        pMyTestCommand.OpenFile(FileStream::FILE_MODES::OUTPUT);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::APPEND);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::INPUT);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::INSERT);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::TRUNCATE);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();
    }
    catch (NExcept ne)
    {
        ASSERT_NE(std::string(ne.buffer).find("file cannot open"), std::string::npos);
    }
}

TEST_F(FileStreamTest, OpenFile)
{
    try
    {
        FileStream pMyTestCommand((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "filestream_file2.asc").string().c_str());

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::OUTPUT);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::APPEND);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::INPUT);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::INSERT);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();

        pMyTestCommand.OpenFile(FileStream::FILE_MODES::TRUNCATE);
        ASSERT_FALSE(pMyTestCommand.GetMyFileStream()->fail());
        pMyTestCommand.CloseFile();
    }
    catch (NExcept ne)
    {
        ASSERT_NE(std::string(ne.buffer).find("file  cannot open"), std::string::npos);
    }
}

TEST_F(FileStreamTest, Exception)
{
    try
    {
        FileStream pMyTestCommand((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "abcd.xyz").string().c_str());
        pMyTestCommand.CloseFile();
        ASSERT_TRUE(1 == 0); // Should not reach this line
    }
    catch (...)
    {
        ASSERT_TRUE(1 == 1); // User can catch exception here.
    }
}
