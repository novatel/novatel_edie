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
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "hw_interface/stream_interface/api/inputfilestream.hpp"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR
#endif

class InputFileStreamTest : public ::testing::Test
{
  public:
    virtual void SetUp() {}

    virtual void TearDown() {}

  private:
  protected:
};

TEST_F(InputFileStreamTest, ReadData)
{
    ReadDataStructure stReadDataStructure;
    InputFileStream* pMyTestCommand = nullptr;
    pMyTestCommand =
        new InputFileStream((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "streaminterface_testread.asc").string().c_str());
    stReadDataStructure.uiDataSize = 20;
    stReadDataStructure.cData = new char[stReadDataStructure.uiDataSize + 1];
    stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
    pMyTestCommand->ReadData(stReadDataStructure);
    ASSERT_STREQ("This is a test file.", stReadDataStructure.cData);
    delete[] stReadDataStructure.cData;
    delete pMyTestCommand;
}

TEST_F(InputFileStreamTest, ReadDataWideCharPath)
{
    ReadDataStructure stReadDataStructure;
    InputFileStream* pMyTestCommand = nullptr;
    pMyTestCommand = new InputFileStream(
        std::u32string(((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / U"inputfilestream不同语言的文件.gps").generic_u32string())));
    stReadDataStructure.uiDataSize = 69;
    stReadDataStructure.cData = new char[stReadDataStructure.uiDataSize + 1];
    stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
    pMyTestCommand->ReadData(stReadDataStructure);
    std::cout << "Made it past READ. Input Stream Test, Read Data WC" << std::endl;
    ASSERT_STREQ("#RANGEA,COM1,0,77.5,FINESTEERING,2195,512277.000,02000020,9691,16696;", stReadDataStructure.cData);
    delete[] stReadDataStructure.cData;
    std::cout << "Made it past 1st delete. Input Stream Test, Read Data WC" << std::endl;
    delete pMyTestCommand;
    std::cout << "Made it past 2nd delete. Input Stream Test, Read Data WC" << std::endl;
}

// ReadLine
TEST_F(InputFileStreamTest, ReadLine)
{
    ReadDataStructure stReadDataStructure;
    StreamReadStatus stReadStatus;

    InputFileStream* pMyTestCommand = nullptr;
    pMyTestCommand =
        new InputFileStream((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "streaminterface_testread.asc").string().c_str());
    std::string linestring;
    stReadStatus = pMyTestCommand->ReadLine(linestring);
    ASSERT_STREQ("This is a test file. it will\r", linestring.c_str());
    ASSERT_TRUE(linestring.size() == stReadStatus.uiCurrentStreamRead);
    ASSERT_FALSE(stReadStatus.bEOS);

    stReadStatus = pMyTestCommand->ReadLine(linestring);
    ASSERT_STREQ("be used to perform unit test cases\r", linestring.c_str());
    ASSERT_TRUE(linestring.size() == stReadStatus.uiCurrentStreamRead);
    ASSERT_FALSE(stReadStatus.bEOS);

    stReadStatus = pMyTestCommand->ReadLine(linestring);
    ASSERT_STREQ("for file stream functionalities.\r", linestring.c_str());
    ASSERT_TRUE(linestring.size() == stReadStatus.uiCurrentStreamRead);
    ASSERT_FALSE(stReadStatus.bEOS);

    stReadStatus = pMyTestCommand->ReadLine(linestring);
    ASSERT_TRUE(linestring.size() == 0);
    ASSERT_TRUE(linestring.size() == stReadStatus.uiCurrentStreamRead);
    ASSERT_TRUE(stReadStatus.bEOS);

    delete pMyTestCommand;
}

// Test Reset Method
TEST_F(InputFileStreamTest, Reset)
{
    ReadDataStructure stReadDataStructure;
    InputFileStream* pMyTestCommand = NULL;
    pMyTestCommand =
        new InputFileStream((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "streaminterface_testread.asc").string().c_str());
    stReadDataStructure.uiDataSize = 20;
    stReadDataStructure.cData = new char[stReadDataStructure.uiDataSize + 1];
    stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
    pMyTestCommand->ReadData(stReadDataStructure);
    ASSERT_STREQ("This is a test file.", stReadDataStructure.cData);

    memset(&stReadDataStructure.cData[0], 0, stReadDataStructure.uiDataSize);
    pMyTestCommand->Reset();
    stReadDataStructure.uiDataSize = 5;
    stReadDataStructure.cData[5] = 0;
    pMyTestCommand->ReadData(stReadDataStructure);
    ASSERT_STREQ("This ", stReadDataStructure.cData);

    memset(&stReadDataStructure.cData[0], 0, stReadDataStructure.uiDataSize);
    pMyTestCommand->Reset(2, std::ios::beg);
    stReadDataStructure.uiDataSize = 5;
    stReadDataStructure.cData[5] = 0;
    pMyTestCommand->ReadData(stReadDataStructure);
    ASSERT_STREQ("is is", stReadDataStructure.cData);

    delete[] stReadDataStructure.cData;
    delete pMyTestCommand;
}

// Test File Extension
TEST_F(InputFileStreamTest, GetFileExtension)
{
    InputFileStream* pMyTestCommand = nullptr;

    pMyTestCommand =
        new InputFileStream((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "streaminterface_testread.asc").string().c_str());
    ASSERT_TRUE("asc" == pMyTestCommand->GetFileExtension());
    delete pMyTestCommand;
}

TEST_F(InputFileStreamTest, GetCurrentFileStats)
{
    ReadDataStructure stReadDataStructure;
    InputFileStream* pMyTestCommand = nullptr;
    pMyTestCommand =
        new InputFileStream((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "streaminterface_testread.asc").string().c_str());
    stReadDataStructure.uiDataSize = 20;
    stReadDataStructure.cData = new char[stReadDataStructure.uiDataSize + 1];
    stReadDataStructure.cData[stReadDataStructure.uiDataSize] = 0;
    pMyTestCommand->ReadData(stReadDataStructure);
    ASSERT_STREQ("This is a test file.", stReadDataStructure.cData);

    memset(&stReadDataStructure.cData[0], 0, stReadDataStructure.uiDataSize);
    pMyTestCommand->Reset();
    stReadDataStructure.uiDataSize = 5;
    stReadDataStructure.cData[5] = 0;
    pMyTestCommand->ReadData(stReadDataStructure);
    ASSERT_STREQ("This ", stReadDataStructure.cData);

    ASSERT_TRUE(5 == pMyTestCommand->GetCurrentFilePosition());
    ASSERT_TRUE(0 == pMyTestCommand->GetCurrentFileOffset());

    pMyTestCommand->Reset(7);
    ASSERT_TRUE(7 == pMyTestCommand->GetCurrentFilePosition());
    ASSERT_TRUE(7 == pMyTestCommand->GetCurrentFileOffset());

    delete[] stReadDataStructure.cData;
    delete pMyTestCommand;
}
