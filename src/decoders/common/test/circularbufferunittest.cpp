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
//  DESCRIPTION:
//    unit test cases for circular buffer implemenatation
//
////////////////////////////////////////////////////////////////////////////////
#include "common/api/circularbuffer.hpp"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class CircularBufferTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:
};


TEST_F(CircularBufferTest, SetCapacity)
{
   UINT uiCapacity = 100;
   CircularBuffer cCircularBuffer;
   cCircularBuffer.SetCapacity(uiCapacity);
   ASSERT_DOUBLE_EQ(cCircularBuffer.GetCapacity(), uiCapacity);

   cCircularBuffer.SetCapacity(0);
   ASSERT_DOUBLE_EQ(cCircularBuffer.GetCapacity(), uiCapacity);

   try
   {
      // Allocate huge amount of memory 
      long lHugeMemory_Size = 0x7fffffff*5; 
      cCircularBuffer.SetCapacity(lHugeMemory_Size);
      ASSERT_EQ(0, 0);
   }
   catch(const std::exception& e)
   {
      std::cerr << e.what() << '\n';
   }   
}

TEST_F(CircularBufferTest, Append)
{
   const CHAR* pcData = "test data";
   UINT uiBytes = (UINT)strlen(pcData);
   CircularBuffer cCircularBuffer;
   ASSERT_DOUBLE_EQ(cCircularBuffer.Append((UCHAR*)pcData, uiBytes), uiBytes);
}

TEST_F(CircularBufferTest, GetByte)
{
   const CHAR* pcData = "test data";
   UINT uiBytes = (UINT)strlen(pcData);

   CircularBuffer cCircularBuffer;
   cCircularBuffer.Append((UCHAR*)pcData, uiBytes);

   ASSERT_EQ(cCircularBuffer.GetByte(0), 't');
   ASSERT_EQ(cCircularBuffer.GetByte(uiBytes-1), 'a');

   ASSERT_EQ(cCircularBuffer.GetByte(uiBytes+1), '\0');
   ASSERT_EQ(cCircularBuffer.GetByte(-1), '\0');

   cCircularBuffer.Discard(4);
   ASSERT_EQ(cCircularBuffer.GetByte(uiBytes), '\0');   
}

TEST_F(CircularBufferTest, Discard)
{
   const CHAR* pcData = "test data";
   size_t uiBytes = strlen(pcData);
   CircularBuffer cCircularBuffer;
   cCircularBuffer.Append((UCHAR*)pcData, (UINT)uiBytes);
   cCircularBuffer.Discard(1);
   ASSERT_TRUE(cCircularBuffer.GetLength() == uiBytes-1);
   cCircularBuffer.Discard(1);
   ASSERT_TRUE(cCircularBuffer.GetLength() == uiBytes-2);
   cCircularBuffer.Discard(uiBytes-2);
   ASSERT_TRUE(cCircularBuffer.GetLength() == 0);
   cCircularBuffer.Discard(1);
   ASSERT_TRUE(cCircularBuffer.GetLength() == 0);      
   cCircularBuffer.Append((UCHAR*)pcData, (UINT)uiBytes);
   ASSERT_TRUE(cCircularBuffer.GetLength() == uiBytes);
   cCircularBuffer.Discard(uiBytes+1);   
}

TEST_F(CircularBufferTest, Clear)
{
   const CHAR* pcData = "test data";
   size_t uiBytes = strlen(pcData);
   CircularBuffer cCircularBuffer;
   cCircularBuffer.Append((UCHAR*)pcData, (UINT)uiBytes);
   cCircularBuffer.Clear();
   ASSERT_TRUE(cCircularBuffer.GetLength() == 0);
}

TEST_F(CircularBufferTest, Copy)
{
   CircularBuffer cCircularBuffer;

   const CHAR* pcData = "test data";
   size_t uiBytes = strlen(pcData);
   cCircularBuffer.Append((UCHAR*)pcData, (UINT)uiBytes);

   UCHAR* pcData_ = new UCHAR[uiBytes+1]; // more than uiBytes
   UINT uiBytes_ = cCircularBuffer.Copy(pcData_, uiBytes+1);
   ASSERT_EQ(uiBytes_, uiBytes);
}
