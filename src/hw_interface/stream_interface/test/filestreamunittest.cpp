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
#include "hw_interface/stream_interface/api/filestream.hpp"
#include "string"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class FileStreamTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:

protected:
};

TEST_F(FileStreamTest, Constructors)
{
   FileStream* pMyTestCommand = NULL;
   FileStream* pMyTestCommand1 = NULL;

   pMyTestCommand = new FileStream("filestream_file1.asc");

   ASSERT_STREQ("filestream_file1.asc" ,pMyTestCommand->GetFileName());

   try
   {
      pMyTestCommand1 = new FileStream((const char*)NULL);
      delete pMyTestCommand;
      delete pMyTestCommand1;
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Filename  name not valid");
      ASSERT_TRUE(pMyTestCommand1 == NULL);
      delete pMyTestCommand;
      delete pMyTestCommand1;
   }
   catch (...)
   {
      ASSERT_TRUE(pMyTestCommand1 == NULL);
      delete pMyTestCommand;
      delete pMyTestCommand1;
   }
}

TEST_F(FileStreamTest, OpenFile)
{
   FileStream* pMyTestCommand = NULL;
   try {
      std::string filename = std::string(DATADIR) + "filestream_file2.asc";

      const char* filepath = filename.c_str();
      pMyTestCommand = new FileStream(filepath);

      pMyTestCommand->OpenFile(FileStream::OUTPUT);
      ASSERT_EQ(pMyTestCommand->GetMyFileStream()->fail(), FALSE);
      pMyTestCommand->CloseFile();

      pMyTestCommand->OpenFile(FileStream::APPEND);
      ASSERT_EQ(pMyTestCommand->GetMyFileStream()->fail(), FALSE);
      pMyTestCommand->CloseFile();

      pMyTestCommand->OpenFile(FileStream::INPUT);
      ASSERT_EQ(pMyTestCommand->GetMyFileStream()->fail(), FALSE);
      pMyTestCommand->CloseFile();

      pMyTestCommand->OpenFile(FileStream::INSERT);
      ASSERT_EQ(pMyTestCommand->GetMyFileStream()->fail(), FALSE);
      pMyTestCommand->CloseFile();

      pMyTestCommand->OpenFile(FileStream::TRUNCATE);
      ASSERT_EQ(pMyTestCommand->GetMyFileStream()->fail(), FALSE);
      pMyTestCommand->CloseFile();

      remove(filepath);
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
   FileStream* pMyTestCommand = NULL;
   try
   {
      std::string filename = std::string(DATADIR) + "abcd.xyz";       
	  
      const char* filepath = filename.c_str();
      pMyTestCommand = new FileStream(filepath);   

      pMyTestCommand->CloseFile();
      ASSERT_TRUE(1 == 0); // Should not reach this line
   }
   catch(nExcept  e)
   {
      ASSERT_TRUE(1 == 1); // User can catch exception here.
   }

   delete  pMyTestCommand; 
}
