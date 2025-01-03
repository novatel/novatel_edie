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
//    unit test cases for UnknownDataHandler implementation
//
////////////////////////////////////////////////////////////////////////////////
#include "common/api/unknowndatahandler.hpp"
#include "common/api/common.hpp"
#include <gtest/gtest.h>

#ifndef DATADIR
    #define DATADIR
#endif

class UnknownDataHandlerTest : public ::testing::Test {
public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

private:
};


TEST_F(UnknownDataHandlerTest, HandleUnknownDataBytes)
{
   UnknownDataHandler clUnknownDataHandler;
   UnknownDataStatistics stMyUnknownDataStatistics;

   // Checking with valid count of COM prompt, LF, CR, OK prompt and unknown ascii bytes with out EOF flag
   // In this case the <O bytes should be considered in the next iteration to check for "<OK"
   clUnknownDataHandler.HandleUnknownDataBytes("[COM1]000000000000000000000<OK000000000000000000000\r\n<O", 55, FALSE);
   clUnknownDataHandler.HandleUnknownDataBytes("K[COM1]000000000000000000000<OK000000000000000000000\r\n<OK", 57, TRUE);

   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 84);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 4);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));

	// Checking with valid count of COM prompt, LF, CR, OK prompt and unknown ascii bytes with out EOF flag
	// In this case the <O bytes should be considered in the next iteration to check for "<OK", No 'K' in next iteration
	// So consider "<O" as unknown bytes 
   clUnknownDataHandler.HandleUnknownDataBytes("[COM1]000000000000000000000<OK000000000000000000000\r\n<O", 55, FALSE);
   clUnknownDataHandler.HandleUnknownDataBytes("[COM1]000000000000000000000<OK000000000000000000000\r\n<OK", 56, TRUE);

   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 86);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 3);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));

   // Checking with valid count of COM prompt, LF, CR, OK prompt and unknown ascii bytes with out EOF flag
   // In this case the [C bytes should be considered in the next iteration to check for "[COM1]"
   clUnknownDataHandler.HandleUnknownDataBytes("[COM1]000000000000000000000<OK000000000000000000000\r\n[C", 55, FALSE);
   clUnknownDataHandler.HandleUnknownDataBytes("OM1]000000000000000000000<OK000000000000000000000\r\n<OK", 54, TRUE);

   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 84);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 3);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));

   // Checking with valid count of COM prompt, LF, CR, OK prompt and unknown ascii bytes with out EOF flag
   // In this case the [C111 bytes should be considered in the next iteration to check for invalid COM ports
   clUnknownDataHandler.HandleUnknownDataBytes("[COM1]000000000000000000000<OK000000000000000000000\r\n[C111", 58, FALSE);
   clUnknownDataHandler.HandleUnknownDataBytes("OM1]000000000000000000000<OK000000000000000000000\r\n<OK", 54, TRUE);

   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 93);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 3);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));

   // Checking with invalid OK prompt and unknown ascii bytes
   clUnknownDataHandler.HandleUnknownDataBytes("[COM1]000000000000000000000<O000000000000000000000\r\n", 52, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 44);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 0);
   clUnknownDataHandler.ResetUnknownDataStatistics();
		
   // Checking with com prompt does not ending with ']'
   clUnknownDataHandler.HandleUnknownDataBytes("[COM1]\r\n[COM1]\r\n<OKOK<OK{COM<OK", 31, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 6);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 3);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));

   // Checking with single byte
   clUnknownDataHandler.HandleUnknownDataBytes("1", 1, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 0);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));

   // Checking with empty string
   clUnknownDataHandler.HandleUnknownDataBytes("", 0, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 0);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));

   // Checking with NULL character which is not printable, So considering as binary byte
   clUnknownDataHandler.HandleUnknownDataBytes("\0", 1, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownBinaryBytes == 1);
   clUnknownDataHandler.ResetUnknownDataStatistics();
   memset(&stMyUnknownDataStatistics, 0x00, sizeof(stMyUnknownDataStatistics));
}

TEST_F(UnknownDataHandlerTest, HandleUnknownData)
{
   UnknownDataHandler clUnknownDataHandler;
   UnknownDataStatistics stMyUnknownDataStatistics;
   MessageHeader stMessageHeader;

   // Checking with empty string and Invalid ASCII message
   stMessageHeader.eMessageFormat = MESSAGE_ASCII;
   clUnknownDataHandler.HandleUnknownData("", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidAsciiMsgs == 0);
   clUnknownDataHandler.ResetUnknownDataStatistics();

   // Checking with invalid Binary message
   stMessageHeader.eMessageFormat = MESSAGE_ASCII;
   stMessageHeader.uiMessageLength = 4;
   clUnknownDataHandler.HandleUnknownData("ABCD", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidAsciiMsgs == 1);
   clUnknownDataHandler.ResetUnknownDataStatistics();

   // Checking with ASCII atring of COM propmpts and LF anf CR
   stMessageHeader.eMessageFormat = MESSAGE_UNKNOWN;
   stMessageHeader.uiMessageLength = 53;
   clUnknownDataHandler.HandleUnknownData("[COM1]000000000000000000000<OK000000000000000000000\r\n", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 42);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 1);
   clUnknownDataHandler.ResetUnknownDataStatistics();

   // Checking with ASCII atring of COM propmpts and LF anf CR
   stMessageHeader.eMessageFormat = MESSAGE_UNKNOWN;
   stMessageHeader.uiMessageLength = 53;
   clUnknownDataHandler.HandleUnknownData("[COM1]000000000000000000000<OK000000000000000000000\r\n", &stMessageHeader, FALSE);
   clUnknownDataHandler.HandleUnknownData("[COM1]000000000000000000000<OK000000000000000000000\r\n", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 84);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 2);
   clUnknownDataHandler.ResetUnknownDataStatistics();

   // Checking with ASCII atring of COM propmpts and LF anf CR with bEOS FALSE/TRUE
   stMessageHeader.eMessageFormat = MESSAGE_UNKNOWN;
   stMessageHeader.uiMessageLength = 54;
   clUnknownDataHandler.HandleUnknownData("[COM1]000000000000000000000<OK000000000000000000000\r\n<", &stMessageHeader, FALSE);
   stMessageHeader.uiMessageLength = 55;
   clUnknownDataHandler.HandleUnknownData("OK[COM1]000000000000000000000<OK000000000000000000000\r\n", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 84);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 3);
   clUnknownDataHandler.ResetUnknownDataStatistics();

   // Checking with ASCII string of COM propmpts and LF anf CR with bEOS FALSE/TRUE
   stMessageHeader.eMessageFormat = MESSAGE_UNKNOWN;
   stMessageHeader.uiMessageLength = 54;
   clUnknownDataHandler.HandleUnknownData("[COM1]000000000000000000000<OK000000000000000000000\r\n<", &stMessageHeader, FALSE);
   stMessageHeader.uiMessageLength = 54;
   clUnknownDataHandler.HandleUnknownData("K[COM1]000000000000000000000<OK000000000000000000000\r\n", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 86);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 2);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 2);
   clUnknownDataHandler.ResetUnknownDataStatistics();

   // Checking with invalid empty Binary message
   stMessageHeader.eMessageFormat = MESSAGE_BINARY;
   clUnknownDataHandler.HandleUnknownData("", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidBinaryMsgs == 1);
   clUnknownDataHandler.ResetUnknownDataStatistics();

   // Checking with invalid Binary message
   stMessageHeader.eMessageFormat = MESSAGE_BINARY;
   stMessageHeader.uiMessageLength = 4;
   clUnknownDataHandler.HandleUnknownData("ABCD", &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidBinaryMsgs == 1);
   clUnknownDataHandler.ResetUnknownDataStatistics();
}

TEST_F(UnknownDataHandlerTest, ResetUnknownDataStatistics)
{
   UnknownDataHandler clUnknownDataHandler;
   UnknownDataStatistics stMyUnknownDataStatistics;
   MessageHeader stMessageHeader;
   // Checking the values of default staistics, it should be all 0's
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidBinaryMsgs == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidAsciiMsgs == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownBinaryBytes == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidCOMPorts == 0);

   stMessageHeader.eMessageFormat = MESSAGE_UNKNOWN;
   std::string strBuffer("[COM1]000[COMG]000000000000000000<O000000000000000000000\r\n");
   stMessageHeader.uiMessageLength = (UINT)strBuffer.length();
   clUnknownDataHandler.HandleUnknownData((CHAR*)strBuffer.c_str(), &stMessageHeader, TRUE);
   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 50);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 1);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidCOMPorts == 1);
			
   // Reset the statistics and check all members values are 0's or not
   clUnknownDataHandler.ResetUnknownDataStatistics();

   stMyUnknownDataStatistics = clUnknownDataHandler.GetUnknownDataStatistics();
   ASSERT_TRUE(stMyUnknownDataStatistics.ulLineFeeds == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCarriageReturns == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidBinaryMsgs == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidAsciiMsgs == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulCOMPorts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulOKPrompts == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownAsciiBytes == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulUnknownBinaryBytes == 0);
   ASSERT_TRUE(stMyUnknownDataStatistics.ulInvalidCOMPorts == 0);
}
