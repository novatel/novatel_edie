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
//    unit test cases for String to types conversion API's
//
////////////////////////////////////////////////////////////////////////////////
#include "common/api/stringtotypes.hpp"
#include <gtest/gtest.h>
#include "common/api/nexcept.h"

#ifndef DATADIR
    #define DATADIR
#endif

class StringToTypesTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:
};


TEST_F(StringToTypesTest, StringToUChar)
{
   try
   {
      CHAR ch = 'A';
      UCHAR uch;

      StringToUChar(&ch, &uch);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in converting string to unsigned char");
   }

   UCHAR uCHAR;
   StringToUChar("65", &uCHAR);
   ASSERT_TRUE(uCHAR == 'A');
}

TEST_F(StringToTypesTest, StringToChar)
{
   try
   {
      CHAR ch = 'A';
      CHAR ch_;

      StringToChar(&ch, &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in converting string to char");
   }

   CHAR cCHAR;
   StringToChar("65", &cCHAR);
   ASSERT_TRUE(cCHAR == 'A');
}

// TEST_F(StringToTypesTest, StringToHexChar)
// {
//    try
//    {
//       CHAR ch = 'A';
//       UCHAR ch_;

//       StringToHexChar(&ch, &ch_);
//       ASSERT_TRUE(1 == 0); // Should not occur
//    }
//    catch(nExcept ne)
//    {
//       ASSERT_STREQ(ne.buffer, "Can't convert string to hex char");
//    }

//    UCHAR ucCHAR;
//    StringToHexChar("65", &ucCHAR);
//    ASSERT_TRUE(ucCHAR == 0x65);
//}

TEST_F(StringToTypesTest, StringToDouble)
{
   try
   {
      DOUBLE ch_;
      StringToDouble("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting String to Double");
   }

   DOUBLE dVal = 0.0;
   StringToDouble("12.12", &dVal);
   ASSERT_TRUE(dVal == 12.12);
}

TEST_F(StringToTypesTest, StringToFloat)
{
   try
   {
      FLOAT ch_;
      StringToFloat("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting String to Float");
   }

   FLOAT fVal = 0.0;
   StringToFloat("12", &fVal);
   printf("fVal: %f\n", fVal);
   ASSERT_TRUE(fVal == 12.00);
}

TEST_F(StringToTypesTest, StringToULong)
{
   try
   {
      ULONG ch_;
      StringToULong("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting String to Unsigned Long");
   }

   ULONG ulVal = 0;
   StringToULong("123456", &ulVal);
   ASSERT_TRUE(ulVal == 123456);
}

TEST_F(StringToTypesTest, StringToHexULong)
{
   try
   {
      ULONG ch_;
      StringToHexULong("", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting hex string to unsigned long");
   }

   ULONG ulVal = 0;
   StringToHexULong("123456", &ulVal);
   ASSERT_TRUE(ulVal == 0x123456);
}

TEST_F(StringToTypesTest, StringToULongLong)
{
   try
   {
      ULONGLONG ch_;
      StringToULongLong("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting String to Unsigned Long Long");
   }

   ULONGLONG ulVal = 0;
   StringToULongLong("123456", &ulVal);
   ASSERT_TRUE(ulVal == 123456);
}

TEST_F(StringToTypesTest, StringToLongLong)
{
   try
   {
      LONGLONG ch_;
      StringToLongLong("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting String to Long Long");
   }

   LONGLONG ulVal = 0;
   StringToLongLong("123456", &ulVal);
   ASSERT_TRUE(ulVal == 123456);
}

TEST_F(StringToTypesTest, StringToLong)
{
   try
   {
      LONG ch_;
      StringToLong("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting String to Long");
   }

   LONG ulVal = 0;
   StringToLong("123456", &ulVal);
   ASSERT_TRUE(ulVal == 123456);
}

TEST_F(StringToTypesTest, StringToInt)
{
   try
   {
      INT ch_;
      StringToInt("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in Converting String to Int");
   }

   INT ulVal = 0;
   StringToInt("-1", &ulVal);
   ASSERT_TRUE(ulVal == -1);
}

TEST_F(StringToTypesTest, StringToUInt)
{
   try
   {
      UINT ch_;
      StringToUInt("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in converting string to unsigned int");
   }

   UINT ulVal = 0;
   StringToUInt("256", &ulVal);
   ASSERT_TRUE(ulVal == 256);
}

TEST_F(StringToTypesTest, StringToShort)
{
   try
   {
      SHORT ch_;
      StringToShort("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in converting string to short");
   }

   SHORT ulVal = 0;
   StringToShort("-1", &ulVal);
   ASSERT_TRUE(ulVal == -1);
}

TEST_F(StringToTypesTest, StringToUShort)
{
   try
   {
      USHORT ch_;
      StringToUShort("A", &ch_);
      ASSERT_TRUE(1 == 0); // Should not occur
   }
   catch(nExcept ne)
   {
      ASSERT_STREQ(ne.buffer, "Error in converting string to unsigned short");
   }

   USHORT ulVal = 0;
   StringToUShort("256", &ulVal);
   ASSERT_TRUE(ulVal == 256);
}

TEST_F(StringToTypesTest, StringToStringWithUCHAR)
{
   UCHAR* cDest = new UCHAR[3];
   memset(cDest, '\0', 3);
   StringToString("101", cDest);
   ASSERT_TRUE(cDest[0] == '1');
   ASSERT_TRUE(cDest[1] == '0');
   ASSERT_TRUE(cDest[2] == '1');
   delete cDest;
   cDest = NULL;
}

TEST_F(StringToTypesTest, StringToStringWithCHAR)
{
   CHAR* cDest = new CHAR[2];
   memset(cDest, '\0', 2);
   StringToString("65", cDest);
   ASSERT_TRUE(cDest[0] == '6');
   ASSERT_TRUE(cDest[1] == '5');
   delete cDest;
   cDest = NULL;
}

TEST_F(StringToTypesTest, StringToBool)
{
   BOOL iVal = 0;
   StringToBool("TRUE", &iVal);
   ASSERT_EQ(iVal, 1);

   StringToBool("FALSE", &iVal);
   ASSERT_EQ(iVal, 0);

   StringToBool("BOMB!", &iVal);
   ASSERT_EQ(iVal, 0);
}

TEST_F(StringToTypesTest, StringToXCharArray)
{
   UCHAR dest[4] = {};
   StringToXCharArray("9142a1A1", dest);
   ASSERT_TRUE(dest[0] == 0x91);
   ASSERT_TRUE(dest[1] == 66);
   ASSERT_TRUE(dest[2] == 0xa1);
   ASSERT_TRUE(dest[3] == 0xA1);
}
