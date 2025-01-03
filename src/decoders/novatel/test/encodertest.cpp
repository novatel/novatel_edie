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

#include <lib/nlohmann/json.hpp>

#include "hw_interface/stream_interface/api/inputstreaminterface.hpp"
#include "hw_interface/stream_interface/api/inputfilestream.hpp"
#include "hw_interface/stream_interface/api/outputstreaminterface.hpp"
#include "hw_interface/stream_interface/api/outputfilestream.hpp"
#include "decoders/novatel/api/framer.hpp"
#include "decoders/novatel/api/decoder.hpp"
#include "decoders/jsoninterface/api/logutils.hpp"
#include "decoders/novatel/api/encoder.hpp"
#include <gtest/gtest.h>

#include <iostream>
#include <fstream>

using json = nlohmann::json;


#ifndef DATADIR
    #define DATADIR
#endif

#define BESTPOS_ASCII_LENGTH    217
#define BESTPOS_BINARY_LENGTH   104
#define BESTPOS_AbASCII_LENGTH  212

class EncoderTest : public ::testing::Test {
public:
   virtual void SetUp()
   {
      std::string filename_json = std::string(DATADIR) + "dynamicdef.json";
      const char*	filepath_json = filename_json.c_str();
      loadDataFromJSON::initializeDatabaseObj();
      JSONFileReader* jsonReader = NULL;
      jsonReader = loadDataFromJSON::getDatabaseObj(filepath_json);
      EXPECT_TRUE(jsonReader != NULL);
   }

   virtual void TearDown()
   {

   }

private:
};

TEST_F(EncoderTest, Bestpos_Ascii)
{
   InputStreamInterface *pclInputStream = NULL;

   std::string filename = std::string(DATADIR) + "advancedecoder_bestpos.ASC";
   const char* filepath = filename.c_str();
   pclInputStream = new InputFileStream(filepath);
   EXPECT_TRUE(pclInputStream != NULL);

   Decoder clDecoder(pclInputStream);
   clDecoder.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = clDecoder.ReadMessage(&BMD);
   EXPECT_TRUE(BMD != NULL);

   //Code to write out data to a file through output stream.
   //Writing out the messages through encoder objects.
   OutputStreamInterface *pclOutputStream = NULL;
   filename = std::string(DATADIR) + "Output_Bestpos_Binnary_Sample.BIN";
   filepath = filename.c_str();
   pclOutputStream = new OutputFileStream(filepath);
   EXPECT_TRUE(pclOutputStream != NULL);

   Encoder *pclEncoder = new Encoder();

   MsgConvertStatusEnum eConversionStatus = pclEncoder->WriteMessage(BMD, Format::BINARY);
   EXPECT_TRUE(GOOD_MESSAGE == eConversionStatus);

   UINT uiLen = pclOutputStream->WriteData(*BMD);
   EXPECT_TRUE(BMD->getMessageLength() == uiLen);
   EXPECT_TRUE(BMD->getMessageID() == 42);
   EXPECT_TRUE(BMD->getMessageFormat() == MESSAGE_BINARY);
   EXPECT_TRUE(BMD->getMessageLength() == BESTPOS_BINARY_LENGTH);

   std::ifstream infile(filepath);
   EXPECT_TRUE(infile.good() == TRUE);
   EXPECT_TRUE(std::remove(filepath) == 0);
}

TEST_F(EncoderTest, Bestpos_Bin)
{
   InputStreamInterface *pclInputStream = NULL;

   std::string filename = std::string(DATADIR) + "advancedecoder_bestpos.bin";
   const char* filepath = filename.c_str();
   pclInputStream = new InputFileStream(filepath);
   EXPECT_TRUE(pclInputStream != NULL);

   Decoder clDecoder(pclInputStream);
   clDecoder.EnableUnknownData(FALSE);
   StreamReadStatus stStreamReadStatus;
   //Declare base message data
   BaseMessageData *BMD = NULL;
   stStreamReadStatus = clDecoder.ReadMessage(&BMD);

   //Code to write out data to a file through output stream.
   //Writing out the messages through encoder objects.
   OutputStreamInterface *pclOutputStream = NULL;
   filename = std::string(DATADIR) + "Output_Bestpos_Ascii_Sample.txt";
   filepath = filename.c_str();
   pclOutputStream = new OutputFileStream(filepath);
   EXPECT_TRUE(pclOutputStream != NULL);

   Encoder *pclEncoder = new Encoder();

   pclEncoder->WriteMessage(BMD, Format::ASCII);

   UINT uiLen = pclOutputStream->WriteData(*BMD);
   EXPECT_TRUE(BMD->getMessageLength() == uiLen);

   EXPECT_TRUE(BMD->getMessageID() == 42);
   EXPECT_TRUE(BMD->getMessageFormat() == MESSAGE_ASCII);
   EXPECT_TRUE(BMD->getMessageLength() == BESTPOS_ASCII_LENGTH+1);

   std::ifstream infile(filepath);
   EXPECT_TRUE(infile.good() == TRUE);
   EXPECT_TRUE(std::remove(filepath) == 0);
}
