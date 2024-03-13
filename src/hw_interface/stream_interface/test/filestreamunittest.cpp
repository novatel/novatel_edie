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
//  DESCRIPTION: File Stream Unit Test.
//
//
////////////////////////////////////////////////////////////////////////////////

// Includes
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "decoders/common/api/nexcept.h"
#include "hw_interface/stream_interface/api/filestream.hpp"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR
#endif

class FileStreamTest : public ::testing::Test
{
  public:
    virtual void SetUp() {}

    virtual void TearDown() {}

  private:
  protected:
};

TEST_F(FileStreamTest, Constructors)
{
    FileStream* pMyTestCommand = nullptr;
    FileStream* pMyTestCommand1 = nullptr;

    pMyTestCommand = new FileStream("filestream_file1.asc");
    std::string testString = std::string("filestream_file1.asc");
    ASSERT_EQ(testString, pMyTestCommand->GetFileName());

    try
    {
        pMyTestCommand1 = new FileStream(nullptr);
        delete pMyTestCommand;
        delete pMyTestCommand1;
    }
    catch (nExcept ne)
    {
        ASSERT_STREQ(ne.buffer, "file name is not valid");
        ASSERT_TRUE(pMyTestCommand1 == nullptr);
        delete pMyTestCommand;
        delete pMyTestCommand1;
    }
    catch (...)
    {
        ASSERT_TRUE(pMyTestCommand1 == nullptr);
        delete pMyTestCommand;
        delete pMyTestCommand1;
    }
}

TEST_F(FileStreamTest, ConstructorWideChar)
{
    FileStream* pMyTestCommand = nullptr;

    pMyTestCommand = new FileStream(std::u32string(U"filestream不同语言的文件.gps"));
    const std::u32string test32String = U"filestream不同语言的文件.gps";

    // const char32_t* test = pMyTestCommand->GetWCFileName();
    // C++17 implicitly converts char32_t type values into int type values. This line will likely
    // break in C++20
    ASSERT_EQ(test32String, pMyTestCommand->Get32StringFileName());
    delete pMyTestCommand;
}

TEST_F(FileStreamTest, WideCharOpenFile)
{
    FileStream* pMyTestCommand = nullptr;
    try
    {
        pMyTestCommand = new FileStream(
            std::u32string((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / U"filestream不同语言的文件.gps").generic_u32string()));
        pMyTestCommand->OpenFile(FileStream::FILEMODES::OUTPUT);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::APPEND);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::INPUT);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::INSERT);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::TRUNCATE);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        delete pMyTestCommand;
    }
    catch (nExcept ne)
    {
        ASSERT_NE(std::string(ne.buffer).find("file  cannot open"), std::string::npos);
    }
    catch (...)
    {
        delete pMyTestCommand;
    }
}

TEST_F(FileStreamTest, OpenFile)
{
    FileStream* pMyTestCommand = nullptr;
    try
    {
        pMyTestCommand = new FileStream((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "filestream_file2.asc").string().c_str());

        pMyTestCommand->OpenFile(FileStream::FILEMODES::OUTPUT);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::APPEND);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::INPUT);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::INSERT);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        pMyTestCommand->OpenFile(FileStream::FILEMODES::TRUNCATE);
        ASSERT_FALSE(pMyTestCommand->GetMyFileStream()->fail());
        pMyTestCommand->CloseFile();

        delete pMyTestCommand;
    }
    catch (nExcept ne)
    {
        ASSERT_NE(std::string(ne.buffer).find("file  cannot open"), std::string::npos);
    }
    catch (...)
    {
        delete pMyTestCommand;
    }
}

TEST_F(FileStreamTest, Exception)
{
    FileStream* pMyTestCommand = nullptr;
    try
    {
        pMyTestCommand = new FileStream((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "abcd.xyz").string().c_str());

        pMyTestCommand->CloseFile();
        ASSERT_TRUE(1 == 0); // Should not reach this line
    }
    catch (...)
    {
        ASSERT_TRUE(1 == 1); // User can catch exception here.
    }

    delete pMyTestCommand;
}
