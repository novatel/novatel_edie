################################################################################
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
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
################################################################################
//                            DESCRIPTION
//
//! \file novateltest.hpp
//! \brief Unit tests for OEM Framer, HeaderDecoder, MessageDecoder,
//! Encoder and Filter.
################################################################################

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "paths.hpp"
#include <nlohmann/json.hpp>

#include "hw_interface/stream_interface/inputstreaminterface.hpp"
#include "hw_interface/stream_interface/inputfilestream.hpp"
#include "decoders/novatel/commander.hpp"
#include "decoders/novatel/encoder.hpp"
#include "decoders/novatel/filter.hpp"
#include "decoders/novatel/framer.hpp"
#include "decoders/novatel/header_decoder.hpp"
#include "decoders/novatel/message_decoder.hpp"
#include "decoders/novatel/fileparser.hpp"
#include "common/jsonreader.hpp"
#include "resources/novatel_message_definitions.hpp"
#include <gtest/gtest.h>

#include <climits>
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <locale>
#include <codecvt>

using json = nlohmann::json;

// Declare FLATTENED_BINARY Conversion Decode/Encode Assertion Functions.
void ASSERT_HEADER_EQ(unsigned char* pucHeader1_, unsigned char* pucHeader2_);
void ASSERT_SHORT_HEADER_EQ(unsigned char* pucShortHeader_, unsigned char* pucHeader_);
void ASSERT_BESTSATS_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_);
void ASSERT_BESTPOS_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_);
void ASSERT_LOGLIST_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_);
void ASSERT_RAWGPSSUBFRAME_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_);
void ASSERT_TRACKSTAT_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_);

class FramerTest : public ::testing::Test
{

protected:
   static Framer* pclMyFramer;
   static InputFileStream* pclMyIFS;
   static unsigned char* pucMyTestFrameBuffer;

   // Per-test-suite setup
   static void SetUpTestSuite()
   {
      pclMyFramer = new Framer();
      pclMyFramer->SetReportUnknownBytes(true);
      pclMyFramer->SetPayloadOnly(false);
      pucMyTestFrameBuffer = new unsigned char[131071]; // 128kB
   }

   // Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if (pclMyFramer)
      {
         pclMyFramer->ShutdownLogger();
         delete pclMyFramer;
         pclMyFramer = nullptr;
      }

      if (pclMyIFS)
      {
         delete pclMyIFS;
         pclMyIFS = nullptr;
      }

      if (pucMyTestFrameBuffer)
      {
         delete[] pucMyTestFrameBuffer;
         pucMyTestFrameBuffer = nullptr;
      }
   }

   // Per-test setup
   void SetUp()
   {
      FlushFramer();
   }

   // Per-test teardown
   void TearDown()
   {
      FlushFramer();
   }

public:
   void WriteFileStreamToFramer(std::string sFilename_)
   {
      pclMyIFS = new InputFileStream((std::filesystem::path(*TEST_RESOURCE_PATH) / sFilename_).string().c_str());

      StreamReadStatus stReadStatus;
      ReadDataStructure stReadData;
      stReadData.cData = new char[MAX_ASCII_MESSAGE_LENGTH];
      stReadData.uiDataSize = MAX_ASCII_MESSAGE_LENGTH;
      uint32_t uiBytesWritten = 0;

      while (!stReadStatus.bEOS)
      {
         stReadStatus = pclMyIFS->ReadData(stReadData);
         uiBytesWritten = pclMyFramer->Write(reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);
         ASSERT_EQ(uiBytesWritten, stReadStatus.uiCurrentStreamRead);
      }

      delete[] stReadData.cData;
      stReadData.cData = nullptr;

      delete pclMyIFS;
      pclMyIFS = nullptr;
   }

   void WriteBytesToFramer(unsigned char* pucBytes_, uint32_t uiNumBytes_)
   {
      ASSERT_EQ(pclMyFramer->Write(pucBytes_, uiNumBytes_), uiNumBytes_);
   }

   bool CompareMetaData(MetaDataStruct* pstTestMD_, MetaDataStruct* pstExpectedMetaData_)
   {
      bool bResult = true;
      if (pstTestMD_->eFormat != pstExpectedMetaData_->eFormat)
      {
         cout << "MetaData.eFormat (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eFormat) << ", got " << static_cast<uint32_t>(pstTestMD_->eFormat) << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->eMeasurementSource != pstExpectedMetaData_->eMeasurementSource)
      {
         cout << "MetaData.eMeasurementSource (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eMeasurementSource) << ", got " << static_cast<uint32_t>(pstTestMD_->eMeasurementSource) << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->eTimeStatus != pstExpectedMetaData_->eTimeStatus)
      {
         cout << "MetaData.eTimeStatus (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eTimeStatus) << ", got " << static_cast<uint32_t>(pstTestMD_->eTimeStatus) << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->bResponse != pstExpectedMetaData_->bResponse)
      {
         cout << "MetaData.bResponse (expected " << pstExpectedMetaData_->bResponse << ", got " << pstTestMD_->bResponse << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->usWeek != pstExpectedMetaData_->usWeek)
      {
         cout << "MetaData.usWeek (expected " << pstExpectedMetaData_->usWeek << ", got " << pstTestMD_->usWeek << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->dMilliseconds != pstExpectedMetaData_->dMilliseconds)
      {
         cout << "MetaData.dMilliseconds (expected " << pstExpectedMetaData_->dMilliseconds << ", got " << pstTestMD_->dMilliseconds << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiBinaryMsgLength != pstExpectedMetaData_->uiBinaryMsgLength)
      {
         cout << "MetaData.uiBinaryMsgLength (expected " << pstExpectedMetaData_->uiBinaryMsgLength << ", got " << pstTestMD_->uiBinaryMsgLength << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiLength != pstExpectedMetaData_->uiLength)
      {
         cout << "MetaData.uiLength (expected " << pstExpectedMetaData_->uiLength << ", got " << pstTestMD_->uiLength << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiHeaderLength != pstExpectedMetaData_->uiHeaderLength)
      {
         cout << "MetaData.uiHeaderLength (expected " << pstExpectedMetaData_->uiHeaderLength << ", got " << pstTestMD_->uiHeaderLength << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->usMessageID != pstExpectedMetaData_->usMessageID)
      {
         cout << "MetaData.usMessageID (expected " << pstExpectedMetaData_->usMessageID << ", got " << pstTestMD_->usMessageID << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiMessageCRC != pstExpectedMetaData_->uiMessageCRC)
      {
         cout << "MetaData.uiMessageCRC (expected " << pstExpectedMetaData_->uiMessageCRC << ", got " << pstTestMD_->uiMessageCRC << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->MessageName() != pstExpectedMetaData_->MessageName())
      {
         cout << "MetaData.acMessageName (expected " << pstExpectedMetaData_->MessageName() << ", got " << pstTestMD_->MessageName() << ")" << endl;
         bResult = false;
      }
      return bResult;
   }

   void FlushFramer()
   {
      uint32_t uiBytes = 0;
      do
      {
         uiBytes = pclMyFramer->Flush(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH);
      } while (uiBytes > 0);
   }
};
Framer* FramerTest::pclMyFramer = nullptr;
InputFileStream* FramerTest::pclMyIFS = nullptr;
unsigned char* FramerTest::pucMyTestFrameBuffer = nullptr;

// -------------------------------------------------------------------------------------------------------
// Logger Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, LOGGER)
{
   spdlog::level::level_enum eLevel = spdlog::level::off;

   ASSERT_NE(spdlog::get("novatel_framer"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_framer = pclMyFramer->GetLogger();
   pclMyFramer->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_framer->level(), eLevel);
}

// -------------------------------------------------------------------------------------------------------
// ASCII Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, ASCII_COMPLETE)
{
   unsigned char aucData[] = "GARBAGE_DATA#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 12;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_EQ(pclMyFramer->GetBytesAvailableInBuffer(), 524U);
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 217;
   stExpectedMetaData.eFormat = HEADERFORMAT::ASCII;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ASCII_INCOMPLETE)
{
   unsigned char aucData[] = "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 165;
   stExpectedMetaData.eFormat = HEADERFORMAT::ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ASCII_SYNC_ERROR)
{
   WriteFileStreamToFramer("ascii_sync_error.ASC");

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = MAX_ASCII_MESSAGE_LENGTH;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ASCII_BAD_CRC)
{
   unsigned char aucData[] = "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*ffffffff\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 217;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ASCII_RUN_ON_CRC)
{
   unsigned char aucData[] = "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35ff\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 219;
   stExpectedMetaData.eFormat = HEADERFORMAT::ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ASCII_INADEQUATE_BUFFER)
{
   unsigned char aucData[] = "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 217;
   stExpectedMetaData.eFormat = HEADERFORMAT::ASCII;
   ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 108, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 217, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ASCII_BYTE_BY_BYTE)
{
   unsigned char aucData[] = "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   uint32_t uiRemainingBytes = uiLogSize;

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::ASCII;

   while (true)
   {
      WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
      uiRemainingBytes--;
      stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

      // We have to process the CRC all at the same time, so we can't test byte-by-byte
      // within it
      if (uiRemainingBytes >= OEM4_ASCII_CRC_LENGTH + 2) // CRC + CRLF
      {
         ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
         ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
      }

      if (!uiRemainingBytes)
         break;
   }
   stExpectedMetaData.uiLength = uiLogSize;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ASCII_SEGMENTED)
{
   unsigned char aucData[] = "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   uint32_t uiBytesWritten = 0;
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::ASCII;

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_ASCII_SYNC_LENGTH);
   uiBytesWritten += OEM4_ASCII_SYNC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 70);
   uiBytesWritten += 70;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 135);
   uiBytesWritten += 135;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 1);
   uiBytesWritten += 1;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_ASCII_CRC_LENGTH + 2);
   uiBytesWritten += OEM4_ASCII_CRC_LENGTH + 2;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(uiLogSize, uiBytesWritten);
}

TEST_F(FramerTest, ASCII_TRICK)
{
   unsigned char aucData[] = "#TEST;*ffffffff\r\n#;*\r\n#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   MetaDataStruct stExpectedMetaData, stTestMetaData;

   WriteBytesToFramer(aucData, uiLogSize);

   stExpectedMetaData.uiLength = 17;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 5;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 217;
   stExpectedMetaData.eFormat = HEADERFORMAT::ASCII;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_SEGMENTED)
{
   unsigned char aucData[] = "<RAWIMUX ICOM7 0 68.5 FINESTEERING 2222 136132.845 02040120 0dc5 16860\r\n< 04 41 2222 136132.844765 edb7fe00 327412165 - 7829932 13988218 - 498546 213188 - 987039\r\n[COM1]";
   uint32_t uiLogSize = sizeof(aucData) - 1 - 6;  // Remove the [ICOM] from the log size
   uint32_t uiBytesWritten = 0;
   MetaDataStruct stExpectedFrameData, stTestMetaData;

   WriteBytesToFramer(&aucData[uiBytesWritten], 1); // Sync Byte
   uiBytesWritten += 1;
   stExpectedFrameData.uiLength = uiBytesWritten;
   stExpectedFrameData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedFrameData));


   WriteBytesToFramer(&aucData[uiBytesWritten], 69); // Header with no CRLF
   uiBytesWritten += 69;
   stExpectedFrameData.uiLength = uiBytesWritten;
   stExpectedFrameData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedFrameData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 1); // CR
   uiBytesWritten += 1;
   stExpectedFrameData.uiLength = uiBytesWritten;
   stExpectedFrameData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedFrameData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 1); // LF
   uiBytesWritten += 1;
   stExpectedFrameData.uiLength = uiBytesWritten - 2; // Framer is going to step back 2 bytes to keep alignment with the CR
   // so no extra bytes to detect. Odd quirk with abbv ascii framing.
   stExpectedFrameData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedFrameData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 89); // Body
   uiBytesWritten += 89;
   stExpectedFrameData.uiLength = uiBytesWritten;
   stExpectedFrameData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedFrameData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 6 + 2); // CRLF + [COM1]
   uiBytesWritten += 2;  // Ignore the [COM1]
   stExpectedFrameData.uiLength = uiBytesWritten;
   stExpectedFrameData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedFrameData));
   ASSERT_EQ(uiLogSize, uiBytesWritten);
   FlushFramer();
}

// -------------------------------------------------------------------------------------------------------
// Binary Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, BINARY_COMPLETE)
{
   // "GARBAGE_DATA<binary bestpos log>"
   unsigned char aucData[] = { 0x47, 0x41, 0x52, 0x42, 0x41, 0x47, 0x45, 0x5F, 0x44, 0x41, 0x54, 0x41, 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89 };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 12;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 104;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, BINARY_INCOMPLETE)
{
   // "<incomplete binary bestpos log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00 };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 82;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, BINARY_SYNC_ERROR)
{
   WriteFileStreamToFramer("binary_sync_error.BIN");

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = MAX_BINARY_MESSAGE_LENGTH;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, BINARY_BAD_CRC)
{
   // "<binary bestpos log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0xFF };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 57; // There is an ASCII sync character at this point in the binary log.
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, BINARY_RUN_ON_CRC)
{
   // "<binary bestpos log>FF"
   unsigned char aucData[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89, 0xFF, 0xFF };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 104;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, BINARY_INADEQUATE_BUFFER)
{
   // "<binary bestpos log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89 };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 104;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;
   ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 52, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 104, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, BINARY_BYTE_BY_BYTE)
{
   // "<binary bestpos log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89 };
   uint32_t uiLogSize = sizeof(aucData);
   uint32_t uiRemainingBytes = uiLogSize;

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

   while (true)
   {
      WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
      uiRemainingBytes--;
      stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

      if (stExpectedMetaData.uiLength == OEM4_BINARY_SYNC_LENGTH)
      {
         stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;
      }

      if (uiRemainingBytes > 0)
      {
         ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
         ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
      }
      else
      {
         break;
      }
   }
   stExpectedMetaData.uiLength = uiLogSize;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, BINARY_SEGMENTED)
{
   // "<binary bestpos log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89 };
   uint32_t uiLogSize = sizeof(aucData);
   uint32_t uiBytesWritten = 0;
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_BINARY_SYNC_LENGTH);
   uiBytesWritten += OEM4_BINARY_SYNC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH));
   uiBytesWritten += (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH);
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 72);
   uiBytesWritten += 72;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_BINARY_CRC_LENGTH);
   uiBytesWritten += OEM4_BINARY_CRC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
   ASSERT_EQ(uiLogSize, uiBytesWritten);
}

TEST_F(FramerTest, BINARY_TRICK)
{
   // "<binary syncs><binary sync + half header><binary sync byte 1><binary bestpos log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x12, 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0xAA, 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89 };
   uint32_t uiLogSize = sizeof(aucData);
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

   WriteBytesToFramer(aucData, uiLogSize);
   stExpectedMetaData.uiLength = 3;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 15;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 1;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 104;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

// -------------------------------------------------------------------------------------------------------
// Short ASCII Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, SHORT_ASCII_COMPLETE)
{
   unsigned char aucData[] = "GARBAGE_DATA%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 12;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 120;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_ASCII;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_ASCII_INCOMPLETE)
{
   unsigned char aucData[] = "%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 93;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_ASCII_SYNC_ERROR)
{
   WriteFileStreamToFramer("short_ascii_sync_error.ASC");

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = MAX_SHORT_ASCII_MESSAGE_LENGTH;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_ASCII_BAD_CRC)
{
   unsigned char aucData[] = "%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*ffffffff\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 120;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_ASCII_RUN_ON_CRC)
{
   unsigned char aucData[] = "%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7bff\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 122;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_ASCII_INADEQUATE_BUFFER)
{
   // "<binary bestpos log>"
   unsigned char aucData[] = "%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 120;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_ASCII;
   ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 60, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 120, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_ASCII_BYTE_BY_BYTE)
{
   unsigned char aucData[] = "%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   uint32_t uiRemainingBytes = uiLogSize;

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_ASCII;

   while (true)
   {
      WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
      uiRemainingBytes--;
      stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

      // We have to process the CRC all at the same time, so we can't test byte-by-byte
      // within it
      if (uiRemainingBytes >= OEM4_ASCII_CRC_LENGTH + 2) // CRC + CRLF
      {
         ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
         ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
      }

      if (!uiRemainingBytes)
         break;
   }
   stExpectedMetaData.uiLength = uiLogSize;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_ASCII_SEGMENTED)
{
   unsigned char aucData[] = "%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   uint32_t uiBytesWritten = 0;
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_ASCII;

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_SHORT_ASCII_SYNC_LENGTH);
   uiBytesWritten += OEM4_SHORT_ASCII_SYNC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 26);
   uiBytesWritten += 26;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 82);
   uiBytesWritten += 82;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 1);
   uiBytesWritten += 1;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_ASCII_CRC_LENGTH + 2);
   uiBytesWritten += OEM4_ASCII_CRC_LENGTH + 2;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(uiLogSize, uiBytesWritten);
}

TEST_F(FramerTest, SHORT_ASCII_TRICK)
{
   unsigned char aucData[] = "%;*\r\n%%**\r\n%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

   WriteBytesToFramer(aucData, uiLogSize);

   stExpectedMetaData.uiLength = 5;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 1;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 5;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 120;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_ASCII;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

// -------------------------------------------------------------------------------------------------------
// Short Binary Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, SHORT_BINARY_COMPLETE)
{
   // "GARBAGE_DATA<short binary rawimusx log>"
   unsigned char aucData[] = { 0x47, 0x41, 0x52, 0x42, 0x41, 0x47, 0x45, 0x5F, 0x44, 0x41, 0x54, 0x41, 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 12;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 56;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_BINARY_INCOMPLETE)
{
   // "<incomplete short binary rawimusx log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87 };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 34;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_BINARY_SYNC_ERROR)
{
   WriteFileStreamToFramer("short_binary_sync_error.BIN");

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = MAX_SHORT_BINARY_MESSAGE_LENGTH;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_BINARY_BAD_CRC)
{
   // "<short binary rawimusx log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xFF };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 56;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_BINARY_RUN_ON_CRC)
{
   // "<short binary rawimusx log>FF"
   unsigned char aucData[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA, 0xFF, 0xFF };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 56;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_BINARY_INADEQUATE_BUFFER)
{
   // "<short binary rawimusx log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;
   stExpectedMetaData.uiLength = 56;
   ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 28, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 56, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_BINARY_BYTE_BY_BYTE)
{
   // "<short binary rawimusx log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA };
   uint32_t uiLogSize = sizeof(aucData);
   uint32_t uiRemainingBytes = uiLogSize;

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

   while (true)
   {
      WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
      uiRemainingBytes--;
      stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

      if (stExpectedMetaData.uiLength == OEM4_SHORT_BINARY_SYNC_LENGTH)
      {
         stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;
      }

      if (uiRemainingBytes > 0)
      {
         ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
         ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
      }
      else
      {
         break;
      }
   }
   stExpectedMetaData.uiLength = uiLogSize;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, SHORT_BINARY_SEGMENTED)
{
   // "<short binary rawimusx log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA };
   uint32_t uiLogSize = sizeof(aucData);
   uint32_t uiBytesWritten = 0;
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_SHORT_BINARY_SYNC_LENGTH);
   uiBytesWritten += OEM4_SHORT_BINARY_SYNC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], (OEM4_SHORT_BINARY_HEADER_LENGTH - OEM4_SHORT_BINARY_SYNC_LENGTH));
   uiBytesWritten += (OEM4_SHORT_BINARY_HEADER_LENGTH - OEM4_SHORT_BINARY_SYNC_LENGTH);
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 40);
   uiBytesWritten += 40;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_BINARY_CRC_LENGTH);
   uiBytesWritten += OEM4_BINARY_CRC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
   ASSERT_EQ(uiLogSize, uiBytesWritten);
}

TEST_F(FramerTest, SHORT_BINARY_TRICK)
{
   // "<short binary sync><short binary sync + part header><short binary sync 1><short binary rawimusx log>"
   unsigned char aucData[] = { 0xAA, 0x44, 0x13, 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xAA, 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
   uint32_t uiLogSize = sizeof(aucData);
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

   WriteBytesToFramer(aucData, uiLogSize);
   stExpectedMetaData.uiLength = 3;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 10;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 1;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 56;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 116;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_SHORT_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

// -------------------------------------------------------------------------------------------------------
// NMEA Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, NMEA_COMPLETE)
{
   unsigned char aucData[] = "GARBAGE_DATA$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 12;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 82;
   stExpectedMetaData.eFormat = HEADERFORMAT::NMEA;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, NMEA_INCOMPLETE)
{
   unsigned char aucData[] = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc4";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 67;
   stExpectedMetaData.eFormat = HEADERFORMAT::NMEA;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, NMEA_SYNC_ERROR)
{
   WriteFileStreamToFramer("nmea_sync_error.txt");

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = MAX_NMEA_MESSAGE_LENGTH;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, NMEA_BAD_CRC)
{
   unsigned char aucData[] = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*11\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 82;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, NMEA_RUN_ON_CRC)
{
   unsigned char aucData[] = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29ff\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 84;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, NMEA_INADEQUATE_BUFFER)
{
   unsigned char aucData[] = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 82;
   stExpectedMetaData.eFormat = HEADERFORMAT::NMEA;
   ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 41, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 82, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, NMEA_BYTE_BY_BYTE)
{
   unsigned char aucData[] = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   uint32_t uiRemainingBytes = uiLogSize;

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::NMEA;

   while (true)
   {
      WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
      uiRemainingBytes--;
      stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

      if (uiRemainingBytes > 0)
      {
         ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
         ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
      }
      else
      {
         break;
      }
   }
   stExpectedMetaData.uiLength = uiLogSize;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, NMEA_SEGMENTED)
{
   unsigned char aucData[] = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   uint32_t uiBytesWritten = 0;
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::NMEA;

   WriteBytesToFramer(&aucData[uiBytesWritten], NMEA_SYNC_LENGTH);
   uiBytesWritten += NMEA_SYNC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 76);
   uiBytesWritten += 76;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 1);
   uiBytesWritten += 1;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], NMEA_CRC_LENGTH);
   uiBytesWritten += NMEA_CRC_LENGTH;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(&aucData[uiBytesWritten], 2);
   uiBytesWritten += 2;
   stExpectedMetaData.uiLength = uiBytesWritten;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(uiLogSize, uiBytesWritten);
}

TEST_F(FramerTest, NMEA_TRICK)
{
   unsigned char aucData[] = "$*ff\r\n$$**\r\n$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n";
   uint32_t uiLogSize = sizeof(aucData) - 1;
   MetaDataStruct stExpectedMetaData, stTestMetaData;

   WriteBytesToFramer(aucData, uiLogSize);

   stExpectedMetaData.uiLength = 6;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 1;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 5;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 82;
   stExpectedMetaData.eFormat = HEADERFORMAT::NMEA;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_NMEA_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

// -------------------------------------------------------------------------------------------------------
// ASCII Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, ABBREV_ASCII_COMPLETE)
{
   unsigned char aucData[] = "<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"\
      "<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n[COM1]";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 215;
   stExpectedMetaData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_INCOMPLETE)
{
   unsigned char aucData[] = "<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"\
      "<     SOL_COMPUTED SINGLE 51.15043711386 ";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 112;
   stExpectedMetaData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_SYNC_ERROR)
{
   WriteFileStreamToFramer("abbreviated_ascii_sync_error.ASC");

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = MAX_ASCII_MESSAGE_LENGTH;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_INADEQUATE_BUFFER)
{
   unsigned char aucData[] = "<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"\
      "<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n[COM1]";

   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 215;
   stExpectedMetaData.eFormat = HEADERFORMAT::ABB_ASCII;
   ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 108, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 215, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_NO_PROMPT)
{
   unsigned char aucData[] = "<TIME COM1 0 48.5 FINESTEERING 2211 314480.000 02000000 9924 32768\r\n"\
      "<     VALID -1.055585415e-09 7.492303535e-10 -17.99999999958 2022 5 25 15 21 2000 VALID\r\n"\
      "<TIME COM1 0 46.5 FINESTEERING 2211 314490.000 02000000 9924 32768\r\n"\
      "<     VALID 5.035219694e-10 7.564775104e-10 -17.99999999958 2022 5 25 15 21 12000 VALID\r\n";
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::ABB_ASCII;

   WriteBytesToFramer(aucData, sizeof(aucData) - 1);
   stExpectedMetaData.uiLength = 157;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   WriteBytesToFramer(aucData, sizeof(aucData) - 1);
   stExpectedMetaData.uiLength = 157;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_MULTILINE)
{
   unsigned char aucData[] = "<SAVEDSURVEYPOSITIONS COM1 0 55.5 FINESTEERING 2211 324085.143 02000000 ddf2 32768\r\n"\
      "<     2 \r\n"\
      "<          \"MN01\" 51.11600000000 -114.03800000000 1065.0000 \r\n"\
      "<          \"MN02\" 51.11400000000 -114.03700000000 1063.1000\r\n[COM1]";
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::ABB_ASCII;

   WriteBytesToFramer(aucData, sizeof(aucData) - 1);
   stExpectedMetaData.uiLength = 217;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_RESPONSE)
{
   unsigned char aucData[] = "<ERROR:Message is invalid for this model\r\n[COM1]";
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::ABB_ASCII;

   WriteBytesToFramer(aucData, sizeof(aucData) - 1);
   stExpectedMetaData.bResponse = true;
   stExpectedMetaData.uiLength = 42;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_SWAPPED)
{
   unsigned char aucData[] = "<     64 60 B1D2 4 e2410e75b821e2664201b02000b022816c36140020001ddde0000000\r\n"\
      "<BDSRAWNAVSUBFRAME ICOM1_29 0 40.5 FINESTEERING 2204 236927.000 02060000 88f3 16807\r\n<GARBAGE";
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   stExpectedMetaData.uiLength = 77;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 85;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(FramerTest, ABBREV_ASCII_EMPTY_ARRAY)
{
   unsigned char aucData[] = "<RANGE COM1 0 95.5 UNKNOWN 0 170.000 025c0020 5103 16807\r\n<     0 \r\n<         \r\n[COM1]";
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::ABB_ASCII;

   WriteBytesToFramer(aucData, sizeof(aucData)-1);

   stExpectedMetaData.uiLength = 80;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

// -------------------------------------------------------------------------------------------------------
// JSON Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, JSON_COMPLETE)
{
   unsigned char aucData[] = R"({"header": {"message": "BESTSATS","id": 1194,"port": "COM1","sequence_num": 0,"percent_idle_time": 50.0,"time_status": "FINESTEERING","week": 2167,"seconds": 244820.000,"receiver_status": 33554432,"HEADER_reserved1": 48645,"receiver_sw_version": 16248},"body": {"satellite_entries": [{"system_type": "GPS","id": "2","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "20","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "29","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "13","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "16","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "18","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "25","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "5","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "26","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "23","status": "GOOD","status_mask": 7},{"system_type": "QZSS","id": "194","status": "SUPPLEMENTARY","status_mask": 7},{"system_type": "SBAS","id": "131","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "133","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "138","status": "NOTUSED","status_mask": 0},{"system_type": "GLONASS","id": "8+6","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "9-2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "1+1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "24+2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "2-4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "17+4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "16-1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "18-3","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GALILEO","id": "26","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "12","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "19","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "31","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "33","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "8","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "7","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "24","status": "GOOD","status_mask": 15},{"system_type": "BEIDOU","id": "35","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "29","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "20","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "22","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "44","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "57","status": "NOEPHEMERIS","status_mask": 0},{"system_type": "BEIDOU","id": "12","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "24","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "19","status": "SUPPLEMENTARY","status_mask": 1}]}})";
   WriteBytesToFramer(aucData, sizeof(aucData) - 1);

   pclMyFramer->SetFrameJson(true);

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 3464;
   stExpectedMetaData.eFormat = HEADERFORMAT::JSON;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   pclMyFramer->SetFrameJson(false);
}

// -------------------------------------------------------------------------------------------------------
// Edge-case Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(FramerTest, UNKNOWN_BINARY_WITH_ASCII_SYNC)
{
   unsigned char aucData[] = { 0x07, 0x23, 0x82 }; // 0x23 is '#' This is used-to identify binary payload with'#'
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 1;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}


// -------------------------------------------------------------------------------------------------------
// Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
class DecodeEncodeTest : public ::testing::Test
{

protected:
   static JsonReader* pclMyJsonDb;
   static HeaderDecoder* pclMyHeaderDecoder;
   static MessageDecoder* pclMyMessageDecoder;
   static Encoder* pclMyEncoder;

   // Per-test-suite setup
   static void SetUpTestSuite()
   {
      try
      {
         pclMyJsonDb = new JsonReader();
         pclMyJsonDb->LoadFile(*TEST_DB_PATH);
         pclMyHeaderDecoder = new HeaderDecoder(pclMyJsonDb);
         pclMyMessageDecoder = new MessageDecoder(pclMyJsonDb);
         pclMyEncoder = new Encoder(pclMyJsonDb);
      }
      catch (JsonReaderFailure& e)
      {
         printf("%s\n", e.what());

         if (pclMyJsonDb)
         {
            delete pclMyJsonDb;
            pclMyJsonDb = nullptr;
         }

         if (pclMyHeaderDecoder)
         {
            delete pclMyHeaderDecoder;
            pclMyHeaderDecoder = nullptr;
         }

         if (pclMyMessageDecoder)
         {
            delete pclMyMessageDecoder;
            pclMyMessageDecoder = nullptr;
         }

         if (pclMyEncoder)
         {
            delete pclMyEncoder;
            pclMyEncoder = nullptr;
         }
      }
   }

   // Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if (pclMyJsonDb)
      {
         delete pclMyJsonDb;
         pclMyJsonDb = nullptr;
      }

      if (pclMyHeaderDecoder)
      {
         pclMyHeaderDecoder->ShutdownLogger();
         delete pclMyHeaderDecoder;
         pclMyHeaderDecoder = nullptr;
      }

      if (pclMyMessageDecoder)
      {
         pclMyMessageDecoder->ShutdownLogger();
         delete pclMyMessageDecoder;
         pclMyMessageDecoder = nullptr;
      }

      if (pclMyEncoder)
      {
         pclMyEncoder->ShutdownLogger();
         delete pclMyEncoder;
         pclMyEncoder = nullptr;
      }
   }

public:

   typedef void (*logchecker)(char*, char*);

   bool CompareBinaryHeaders(OEM4BinaryHeader* pstFromBinaryHeader, OEM4BinaryHeader* pstFromAsciiHeader)
   {
      bool bResult = true;
      if (pstFromBinaryHeader->ucSync1 != pstFromAsciiHeader->ucSync1)
      {
         cout << "OEM4BinaryHeader.ucSync1 (expected " << pstFromBinaryHeader->ucSync1 << ", got " << pstFromAsciiHeader->ucSync1 << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->ucSync2 != pstFromAsciiHeader->ucSync2)
      {
         cout << "OEM4BinaryHeader.ucSync2 (expected " << pstFromBinaryHeader->ucSync2 << ", got " << pstFromAsciiHeader->ucSync2 << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->ucSync3 != pstFromAsciiHeader->ucSync3)
      {
         cout << "OEM4BinaryHeader.ucSync3 (expected " << pstFromBinaryHeader->ucSync3 << ", got " << pstFromAsciiHeader->ucSync3 << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->ucHeaderLength != pstFromAsciiHeader->ucHeaderLength)
      {
         cout << "OEM4BinaryHeader.ucHeaderLength (expected " << pstFromBinaryHeader->ucHeaderLength << ", got " << pstFromAsciiHeader->ucHeaderLength << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->usMsgNumber != pstFromAsciiHeader->usMsgNumber)
      {
         cout << "OEM4BinaryHeader.usMsgNumber (expected " << pstFromBinaryHeader->usMsgNumber << ", got " << pstFromAsciiHeader->usMsgNumber << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->ucMsgType != pstFromAsciiHeader->ucMsgType)
      {
         cout << "OEM4BinaryHeader.ucMsgType (expected " << pstFromBinaryHeader->ucMsgType << ", got " << pstFromAsciiHeader->ucMsgType << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->ucPort != pstFromAsciiHeader->ucPort)
      {
         cout << "OEM4BinaryHeader.ucPort (expected " << pstFromBinaryHeader->ucPort << ", got " << pstFromAsciiHeader->ucPort << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->usLength != pstFromAsciiHeader->usLength)
      {
         cout << "OEM4BinaryHeader.usLength (expected " << pstFromBinaryHeader->usLength << ", got " << pstFromAsciiHeader->usLength << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->usSequenceNumber != pstFromAsciiHeader->usSequenceNumber)
      {
         cout << "OEM4BinaryHeader.usSequenceNumber (expected " << pstFromBinaryHeader->usSequenceNumber << ", got " << pstFromAsciiHeader->usSequenceNumber << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->ucIdleTime != pstFromAsciiHeader->ucIdleTime)
      {
         cout << "OEM4BinaryHeader.ucIdleTime (expected " << pstFromBinaryHeader->ucIdleTime << ", got " << pstFromAsciiHeader->ucIdleTime << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->ucTimeStatus != pstFromAsciiHeader->ucTimeStatus)
      {
         cout << "OEM4BinaryHeader.ucTimeStatus (expected " << pstFromBinaryHeader->ucTimeStatus << ", got " << pstFromAsciiHeader->ucTimeStatus << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->usWeekNo != pstFromAsciiHeader->usWeekNo)
      {
         cout << "OEM4BinaryHeader.usWeekNo (expected " << pstFromBinaryHeader->usWeekNo << ", got " << pstFromAsciiHeader->usWeekNo << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->uiWeekMSec != pstFromAsciiHeader->uiWeekMSec)
      {
         cout << "OEM4BinaryHeader.uiWeekMSec (expected " << pstFromBinaryHeader->uiWeekMSec << ", got " << pstFromAsciiHeader->uiWeekMSec << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->uiStatus != pstFromAsciiHeader->uiStatus)
      {
         cout << "OEM4BinaryHeader.uiStatus (expected " << pstFromBinaryHeader->uiStatus << ", got " << pstFromAsciiHeader->uiStatus << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->usMsgDefCRC != pstFromAsciiHeader->usMsgDefCRC)
      {
         cout << "OEM4BinaryHeader.usMsgDefCRC (expected " << pstFromBinaryHeader->usMsgDefCRC << ", got " << pstFromAsciiHeader->usMsgDefCRC << ")" << endl;
         bResult = false;
      }
      if (pstFromBinaryHeader->usReceiverSWVersion != pstFromAsciiHeader->usReceiverSWVersion)
      {
         cout << "OEM4BinaryHeader.usReceiverSWVersion (expected " << pstFromBinaryHeader->usReceiverSWVersion << ", got " << pstFromAsciiHeader->usReceiverSWVersion << ")" << endl;
         bResult = false;
      }
      return bResult;
   }

   bool CompareMetaData(MetaDataStruct* pstTestMD_, MetaDataStruct* pstExpectedMetaData_)
   {
      bool bResult = true;
      if (pstTestMD_->eFormat != pstExpectedMetaData_->eFormat)
      {
         cout << "MetaData.eFormat (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eFormat) << ", got " << static_cast<uint32_t>(pstTestMD_->eFormat) << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->eMeasurementSource != pstExpectedMetaData_->eMeasurementSource)
      {
         cout << "MetaData.eMeasurementSource (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eMeasurementSource) << ", got " << static_cast<uint32_t>(pstTestMD_->eMeasurementSource) << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->eTimeStatus != pstExpectedMetaData_->eTimeStatus)
      {
         cout << "MetaData.eTimeStatus (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eTimeStatus) << ", got " << static_cast<uint32_t>(pstTestMD_->eTimeStatus) << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->bResponse != pstExpectedMetaData_->bResponse)
      {
         cout << "MetaData.bResponse (expected " << pstExpectedMetaData_->bResponse << ", got " << pstTestMD_->bResponse << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->usWeek != pstExpectedMetaData_->usWeek)
      {
         cout << "MetaData.usWeek (expected " << pstExpectedMetaData_->usWeek << ", got " << pstTestMD_->usWeek << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->dMilliseconds != pstExpectedMetaData_->dMilliseconds)
      {
         cout << "MetaData.dMilliseconds (expected " << pstExpectedMetaData_->dMilliseconds << ", got " << pstTestMD_->dMilliseconds << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiLength != pstExpectedMetaData_->uiLength)
      {
         cout << "MetaData.uiLength (expected " << pstExpectedMetaData_->uiLength << ", got " << pstTestMD_->uiLength << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiHeaderLength != pstExpectedMetaData_->uiHeaderLength)
      {
         cout << "MetaData.uiHeaderLength (expected " << pstExpectedMetaData_->uiHeaderLength << ", got " << pstTestMD_->uiHeaderLength << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->usMessageID != pstExpectedMetaData_->usMessageID)
      {
         cout << "MetaData.usMessageID (expected " << pstExpectedMetaData_->usMessageID << ", got " << pstTestMD_->usMessageID << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiMessageCRC != pstExpectedMetaData_->uiMessageCRC)
      {
         cout << "MetaData.uiMessageCRC (expected " << pstExpectedMetaData_->uiMessageCRC << ", got " << pstTestMD_->uiMessageCRC << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->MessageName() != pstExpectedMetaData_->MessageName())
      {
         cout << "MetaData.acMessageName (expected " << pstExpectedMetaData_->MessageName() << ", got " << pstTestMD_->MessageName() << ")" << endl;
         bResult = false;
      }
      return bResult;
   }

   bool CompareMessageData(MessageDataStruct* pstTestMD_, MessageDataStruct* pstExpectedMetaData_)
   {
      bool bResult = true;
      if (pstTestMD_->uiMessageHeaderLength != pstExpectedMetaData_->uiMessageHeaderLength)
      {
         cout << "MessageData.uiMessageHeaderLength (expected " << pstExpectedMetaData_->uiMessageHeaderLength << ", got " << pstTestMD_->uiMessageHeaderLength << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiMessageBodyLength != pstExpectedMetaData_->uiMessageBodyLength)
      {
         cout << "MessageData.uiMessageBodyLength (expected " << pstExpectedMetaData_->uiMessageBodyLength << ", got " << pstTestMD_->uiMessageBodyLength << ")" << endl;
         bResult = false;
      }
      if (pstTestMD_->uiMessageLength != pstExpectedMetaData_->uiMessageLength)
      {
         cout << "MessageData.uiMessageLength (expected " << pstExpectedMetaData_->uiMessageLength << ", got " << pstTestMD_->uiMessageLength << ")" << endl;
         bResult = false;
      }
      if (0 != memcmp(pstTestMD_->pucMessageHeader, pstExpectedMetaData_->pucMessageHeader, pstTestMD_->uiMessageHeaderLength))
      {
         cout << "MessageData.pucMessageHeader contents do not match" << endl;
         bResult = false;
      }
      if (0 != memcmp(pstTestMD_->pucMessageBody, pstExpectedMetaData_->pucMessageBody, pstTestMD_->uiMessageBodyLength))
      {
         cout << "MessageData.pucMessageBody contents do not match" << endl;
         bResult = false;
      }
      if (0 != memcmp(pstTestMD_->pucMessage, pstExpectedMetaData_->pucMessage, pstTestMD_->uiMessageLength))
      {
         cout << "MessageData.pucMessage contents do not match" << endl;
         bResult = false;
      }
      return bResult;
   }

   enum
   {
      SUCCESS,
      HEADER_DECODER_ERROR,
      MESSAGE_DECODER_ERROR,
      ENCODER_ERROR,
      HEADER_LENGTH_ERROR,
      LENGTH_ERROR,
      METADATA_COMPARISON_ERROR,
      MESSAGEDATA_COMPARISON_ERROR
   };

   int32_t DecodeEncode(ENCODEFORMAT eFormat_, unsigned char* pucMessageBuffer_, unsigned char* pucEncodeBuffer_, uint32_t uiEncodeBufferSize_, MetaDataStruct& stMetaData_, MessageDataStruct& stMessageData_)
   {
      STATUS eDecoderStatus = STATUS::UNKNOWN;
      STATUS eEncoderStatus = STATUS::UNKNOWN;

      IntermediateHeader stHeader;
      IntermediateMessage stMessage;

      unsigned char* pucTempPtr = pucMessageBuffer_;
      eDecoderStatus = pclMyHeaderDecoder->Decode(pucTempPtr, stHeader, stMetaData_);
      if (STATUS::SUCCESS != eDecoderStatus)
      {
         printf("HeaderDecoder error %d\n", static_cast<uint32_t>(eDecoderStatus));
         return HEADER_DECODER_ERROR;
      }

      pucTempPtr += stMetaData_.uiHeaderLength;
      eDecoderStatus = pclMyMessageDecoder->Decode(pucTempPtr, stMessage, stMetaData_);
      if (STATUS::SUCCESS != eDecoderStatus)
      {
         printf("MessageDecoder error %d\n", static_cast<uint32_t>(eDecoderStatus));
         return MESSAGE_DECODER_ERROR;
      }

      eEncoderStatus = pclMyEncoder->Encode(&pucEncodeBuffer_, uiEncodeBufferSize_, stHeader, stMessage, stMessageData_, stMetaData_, eFormat_);
      if (STATUS::SUCCESS != eEncoderStatus)
      {
         printf("Encoder error %d\n", static_cast<uint32_t>(eDecoderStatus));
         return ENCODER_ERROR;
      }

      return SUCCESS;
   }

   int TestDecodeEncode(ENCODEFORMAT eFormat_, unsigned char* pucMessageBuffer_)
   {
      MetaDataStruct stTestMetaData;
      MessageDataStruct stTestMessageData;

      unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
      unsigned char* pucEncodeBuffer = acEncodeBuffer;

      return DecodeEncode(eFormat_, pucMessageBuffer_, pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData, stTestMessageData);
   }

   int TestSameFormatCompare(ENCODEFORMAT eFormat_, MessageDataStruct* pstExpectedMessageData_, MetaDataStruct* pstExpectedMetaData_, bool bTestMetaData_)
   {
      MetaDataStruct stTestMetaData;
      MessageDataStruct stTestMessageData;
      stTestMetaData.uiLength = pstExpectedMessageData_->uiMessageLength; // This would have been set by the framer, so we just need to trust the expected value.

      unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
      unsigned char* pucEncodeBuffer = acEncodeBuffer;

      int32_t iRetCode = DecodeEncode(eFormat_, pstExpectedMessageData_->pucMessage, pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData, stTestMessageData);
      if (iRetCode != SUCCESS) return iRetCode;

      if (eFormat_ == ENCODEFORMAT::JSON) stTestMetaData.uiHeaderLength = pstExpectedMessageData_->uiMessageHeaderLength;

      if (stTestMetaData.uiHeaderLength != stTestMessageData.uiMessageHeaderLength)
      {
         printf("MetaData.uiHeaderLength is not the same as MessageData.uiMessageHeaderLength (%d != %d)\n", stTestMetaData.uiHeaderLength, stTestMessageData.uiMessageHeaderLength);
         return HEADER_LENGTH_ERROR;
      }

      if (stTestMetaData.uiLength != stTestMessageData.uiMessageLength)
      {
         printf("MetaData.uiLength is not the same as MessageData.uiMessageLength (%d != %d)\n", stTestMetaData.uiLength, stTestMessageData.uiMessageLength);
         return LENGTH_ERROR;
      }

      if (bTestMetaData_)
      {
         if (!CompareMetaData(&stTestMetaData, pstExpectedMetaData_))
         {
            return METADATA_COMPARISON_ERROR;
         }
      }

      if (!CompareMessageData(&stTestMessageData, pstExpectedMessageData_))
      {
         printf("MessageData doesn't match ExpectedMessageData\n");
         return MESSAGEDATA_COMPARISON_ERROR;
      }

      return SUCCESS;
   }

   int32_t TestConversion(ENCODEFORMAT eFormat_, unsigned char* pucMessageBuffer_, MessageDataStruct* pstExpectedMessageData_, MetaDataStruct* pstExpectedMetaData_, bool bTestMetaData_)
   {
      MetaDataStruct stTestMetaData;
      MessageDataStruct stTestMessageData;

      unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
      unsigned char* pucEncodeBuffer = acEncodeBuffer;

      int iRetCode = DecodeEncode(eFormat_, pucMessageBuffer_, pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stTestMetaData, stTestMessageData);
      if (iRetCode != SUCCESS)
      {
         return iRetCode;
      }

      if (pstExpectedMessageData_->uiMessageHeaderLength != stTestMessageData.uiMessageHeaderLength)
      {
         printf("MessageData.uiHeaderLength error (expected %d, got %d)\n", pstExpectedMessageData_->uiMessageHeaderLength, stTestMessageData.uiMessageHeaderLength);
         return HEADER_LENGTH_ERROR;
      }

      if (pstExpectedMessageData_->uiMessageLength != stTestMessageData.uiMessageLength)
      {
         printf("MessageData.uiMessageLength error (expected %d, got %d)\n", pstExpectedMessageData_->uiMessageLength, stTestMessageData.uiMessageLength);
         return LENGTH_ERROR;
      }

      if (bTestMetaData_)
      {
         if (!CompareMetaData(&stTestMetaData, pstExpectedMetaData_))
         {
            return METADATA_COMPARISON_ERROR;
         }
      }

      if (!CompareMessageData(&stTestMessageData, pstExpectedMessageData_))
      {
         return MESSAGEDATA_COMPARISON_ERROR;
      }

      return SUCCESS;
   }

private:
};
JsonReader* DecodeEncodeTest::pclMyJsonDb = nullptr;
HeaderDecoder* DecodeEncodeTest::pclMyHeaderDecoder = nullptr;
MessageDecoder* DecodeEncodeTest::pclMyMessageDecoder = nullptr;
Encoder* DecodeEncodeTest::pclMyEncoder = nullptr;

// -------------------------------------------------------------------------------------------------------
// Logger Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, LOGGER)
{
   spdlog::level::level_enum eLevel = spdlog::level::off;

   ASSERT_NE(spdlog::get("novatel_header_decoder"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_header_decoder = pclMyMessageDecoder->GetLogger();
   pclMyMessageDecoder->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_header_decoder->level(), eLevel);

   ASSERT_NE(spdlog::get("novatel_message_decoder"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_message_decoder = pclMyMessageDecoder->GetLogger();
   pclMyMessageDecoder->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_message_decoder->level(), eLevel);

   ASSERT_NE(spdlog::get("novatel_encoder"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_encoder = pclMyMessageDecoder->GetLogger();
   pclMyMessageDecoder->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_encoder->level(), eLevel);
}

// -------------------------------------------------------------------------------------------------------
// ASCII Log Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_BESTPOS)
{
   unsigned char aucLog[] = "#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 71;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_BESTSATS)
{
   unsigned char aucLog[] = "#BESTSATSA,COM1,0,50.0,FINESTEERING,2167,244820.000,02000000,be05,16248;43,GPS,2,GOOD,00000003,GPS,20,GOOD,00000003,GPS,29,GOOD,00000003,GPS,13,GOOD,00000003,GPS,15,GOOD,00000003,GPS,16,GOOD,00000003,GPS,18,GOOD,00000007,GPS,25,GOOD,00000007,GPS,5,GOOD,00000003,GPS,26,GOOD,00000007,GPS,23,GOOD,00000007,QZSS,194,SUPPLEMENTARY,00000007,SBAS,131,NOTUSED,00000000,SBAS,133,NOTUSED,00000000,SBAS,138,NOTUSED,00000000,GLONASS,8+6,GOOD,00000003,GLONASS,9-2,GOOD,00000003,GLONASS,1+1,GOOD,00000003,GLONASS,24+2,GOOD,00000003,GLONASS,2-4,GOOD,00000003,GLONASS,17+4,GOOD,00000003,GLONASS,16-1,GOOD,00000003,GLONASS,18-3,GOOD,00000003,GLONASS,15,GOOD,00000003,GALILEO,26,GOOD,0000000f,GALILEO,12,GOOD,0000000f,GALILEO,19,ELEVATIONERROR,00000000,GALILEO,31,GOOD,0000000f,GALILEO,25,ELEVATIONERROR,00000000,GALILEO,33,GOOD,0000000f,GALILEO,8,ELEVATIONERROR,00000000,GALILEO,7,GOOD,0000000f,GALILEO,24,GOOD,0000000f,BEIDOU,35,LOCKEDOUT,00000000,BEIDOU,29,SUPPLEMENTARY,00000001,BEIDOU,25,ELEVATIONERROR,00000000,BEIDOU,20,SUPPLEMENTARY,00000001,BEIDOU,22,SUPPLEMENTARY,00000001,BEIDOU,44,LOCKEDOUT,00000000,BEIDOU,57,NOEPHEMERIS,00000000,BEIDOU,12,ELEVATIONERROR,00000000,BEIDOU,24,SUPPLEMENTARY,00000001,BEIDOU,19,SUPPLEMENTARY,00000001*7abea593\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 72;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_CLOCKMODEL)
{
   unsigned char aucLog[] = "#CLOCKMODELA,COM1,0,57.5,FINESTEERING,2167,255598.000,02000000,98f9,16248;VALID,0,255598.000,255598.000,-8.630928664e-01,1.883231757e-02,0.000000000,4.433118538e-02,1.028561778e-03,0.000000000,1.028561778e-03,4.318654824e-03,0.000000000,0.000000000,0.000000000,0.000000000,-0.896,3.704174499e-02,FALSE*18015524\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 74;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_GLOALMANAC)
{
   unsigned char aucLog[] = "#GLOALMANACA,COM1,0,54.0,SATTIME,2167,159108.000,02000000,ba83,16248;24,2167,115150.343,1,1,1,0,39532.343750000,1.316556988,0.022644193,0.000380516,-0.200759736,-2655.375000000,-0.000305176,-0.000080109,2167,80546.843,2,-4,1,0,4928.843750000,-2.415465471,0.030652651,0.001904488,-2.240858310,-2655.851562500,0.000305176,-0.000484467,2167,85554.000,3,5,1,0,9936.000000000,-2.786838624,0.027761457,0.001849174,-2.155051259,-2655.845703125,0.000244141,-0.000038147,2167,90494.343,4,6,1,0,14876.343750000,3.138599593,0.033250232,0.000991821,-1.632539054,-2655.865234375,0.000061035,-0.000095367,2167,95426.781,5,1,1,0,19808.781250000,2.781340861,0.033516881,0.000567436,-2.318803708,-2655.912109375,0.000000000,-0.000072479,2167,101118.375,6,-4,1,0,25500.375000000,2.337855630,0.022338595,0.000492096,2.475749118,-2655.802734375,-0.000183105,-0.000198364,2167,106019.281,7,5,1,0,30401.281250000,2.004915886,0.027902272,0.001599312,-2.137026985,-2655.806640625,-0.000122070,0.000041962,2167,110772.718,8,6,1,0,35154.718750000,1.658347082,0.028034098,0.002008438,-1.757079119,-2655.869140625,-0.000183105,0.000057220,2167,114259.031,9,-2,1,0,38641.031250000,-2.800770285,0.013374395,0.001688004,-2.688301331,-2655.992187500,-0.000915527,-0.000000000,2167,78451.250,10,-7,1,0,2833.250000000,-0.189087101,0.025562352,0.001439095,0.043239083,-2656.169921875,-0.001342773,0.000072479,2167,83619.250,11,0,1,1,8001.250000000,-0.568264981,0.030221219,0.000588417,-2.044029400,-2656.169921875,-0.001342773,0.000022888,2167,88863.437,12,-1,1,0,13245.437500000,-0.938955033,0.026368291,0.001175880,-1.256138518,-2655.986328125,-0.001159668,-0.000244141,2167,93781.406,13,-2,1,0,18163.406250000,-1.308018227,0.025406557,0.000337601,1.744136156,-2656.201171875,-0.001037598,0.000057220,2167,99049.875,14,-7,1,0,23431.875000000,-1.683226333,0.021385849,0.000715256,-2.112099797,-2656.009765625,-0.001098633,-0.000064850,2167,104050.250,15,0,1,0,28432.250000000,-2.043945510,0.025130920,0.000899315,-1.639250219,-2656.019531250,-0.001037598,-0.000099182,2167,109475.187,16,-1,1,0,33857.187500000,-2.465775247,0.018401777,0.002746582,0.205936921,-2655.822265625,-0.000854492,0.000015259,2167,112381.000,17,4,1,0,36763.000000000,-0.550378525,0.044683183,0.000854492,-3.118007699,-2655.976562500,0.001098633,-0.000438690,2167,76649.656,18,-3,1,0,1031.656250000,2.061364581,0.049192247,0.001056671,-0.229426002,-2656.011718750,0.001037598,-0.000083923,2167,81216.375,19,3,1,0,5598.375000000,1.753316072,0.053257895,0.000308990,2.031661680,-2656.169921875,0.000915527,0.000148773,2167,86932.187,20,2,1,0,11314.187500000,1.338137581,0.053485596,0.000810623,0.016106798,-2656.033203125,0.001037598,0.000049591,2167,92471.875,21,4,1,0,16853.875000000,0.905492081,0.048149620,0.000671387,2.711982159,-2655.875000000,0.001098633,0.000225067,2167,97225.437,22,-3,1,0,21607.437500000,0.566332524,0.051370380,0.002092361,0.380906604,-2656.091796875,0.001159668,0.000122070,2167,103403.781,23,3,1,0,27785.781250000,0.114991634,0.051142680,0.000539780,1.610679827,-2656.626953125,0.001098633,0.000007629,2167,107403.343,24,2,1,0,31785.343750000,-0.171967635,0.033052492,0.000456810,-2.399433574,-2656.039062500,0.001037598,-0.000049591*6dee109c\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 69;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_GLOEPHEM)
{
   unsigned char aucLog[] = "#GLOEPHEMERISA,COM1,11,67.0,SATTIME,2168,160218.000,02000820,8d29,32768;51,0,1,80,2168,161118000,10782,573,0,0,95,0,-2.3917966796875000e+07,4.8163881835937500e+06,7.4258510742187500e+06,-1.0062713623046875e+03,1.8321990966796875e+02,-3.3695755004882813e+03,1.86264514923095700e-06,-9.31322574615478510e-07,-0.00000000000000000,-6.69313594698905940e-05,5.587935448e-09,0.00000000000000000,84600,3,2,0,13*ad20fc5f\r\n";

   unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = acEncodeBuffer;
   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(0, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucEncodeBuffer, sizeof(acEncodeBuffer), stMetaData, stMessageData));

   auto* pstGloEphpemeris = reinterpret_cast<GLOEPHEMERIS*>(stMessageData.pucMessageBody);
   ASSERT_EQ(pstGloEphpemeris->sloto, 51);
   ASSERT_EQ(pstGloEphpemeris->freqo, 0);
   ASSERT_EQ(pstGloEphpemeris->sat_type, 1);
   ASSERT_EQ(pstGloEphpemeris->false_iod, 80);
   ASSERT_EQ(pstGloEphpemeris->ephem_week, 2168);
   ASSERT_EQ(pstGloEphpemeris->ephem_time, 161118000U);
   ASSERT_EQ(pstGloEphpemeris->time_offset, 10782U);
   ASSERT_EQ(pstGloEphpemeris->nt, 573);
   ASSERT_EQ(pstGloEphpemeris->GLOEPHEMERIS_reserved, 0);
   ASSERT_EQ(pstGloEphpemeris->GLOEPHEMERIS_reserved_9, 0);
   ASSERT_EQ(pstGloEphpemeris->issue, 95U);
   ASSERT_EQ(pstGloEphpemeris->broadcast_health, 0U);
   ASSERT_EQ(pstGloEphpemeris->pos_x, -2.3917966796875000e+07);
   ASSERT_EQ(pstGloEphpemeris->pos_y, 4.8163881835937500e+06);
   ASSERT_EQ(pstGloEphpemeris->pos_z, 7.4258510742187500e+06);
   ASSERT_EQ(pstGloEphpemeris->vel_x, -1.0062713623046875e+03);
   ASSERT_EQ(pstGloEphpemeris->vel_y, 1.8321990966796875e+02);
   ASSERT_EQ(pstGloEphpemeris->vel_z, -3.3695755004882813e+03);
   ASSERT_NEAR(pstGloEphpemeris->ls_acc_x, 1.86264514923095700e-06, 0.0000000000000001e-06);
   ASSERT_NEAR(pstGloEphpemeris->ls_acc_y, -9.31322574615478510e-07, 0.0000000000000001e-07);
   ASSERT_NEAR(pstGloEphpemeris->ls_acc_z, -0.00000000000000000, 0.0000000000000001);
   ASSERT_NEAR(pstGloEphpemeris->tau, -6.69313594698905940e-05, 0.0000000000000001e-05);
   ASSERT_EQ(pstGloEphpemeris->delta_tau, 5.587935448e-09);
   ASSERT_NEAR(pstGloEphpemeris->gamma, 0.00000000000000000, 0.0000000000000001);
   ASSERT_EQ(pstGloEphpemeris->tk, 84600U);
   ASSERT_EQ(pstGloEphpemeris->p, 3U);
   ASSERT_EQ(pstGloEphpemeris->ft, 2U);
   ASSERT_EQ(pstGloEphpemeris->age, 0U);
   ASSERT_EQ(pstGloEphpemeris->flags, 13U);
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_LOGLIST)
{
   unsigned char aucLog[] = "#LOGLISTA,COM1,0,63.5,FINESTEERING,2172,164226.000,02010000,c00c,16248;6,COM1,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,COM1,INTERFACEMODE,ONTIME,20.000000,0.000000,NOHOLD,COM1,LOGLISTA,ONCE,0.000000,0.000000,NOHOLD,COM2,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,CCOM1,INSPVACMPB,ONTIME,0.050000,0.000000,HOLD,CCOM1,INSPVASDCMPB,ONTIME,1.000000,0.000000,HOLD*53104c0f\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 71;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_RANGE)
{
   unsigned char aucLog[] = "#RANGEA,COM1,0,49.0,FINESTEERING,2167,159740.000,02000000,5103,16248;115,20,0,23291390.821,0.051,-122397109.320305,0.011,-3214.414,44.0,16498.975,1810bc04,20,0,23291389.349,0.184,-95374376.486282,0.013,-2504.740,41.1,16492.754,11303c0b,29,0,20705108.295,0.022,-108806118.750397,0.005,-1620.698,51.2,14908.774,0810bc44,29,0,20705106.528,0.061,-84783996.294254,0.006,-1262.882,50.7,14902.754,01303c4b,29,0,20705107.069,0.025,-84783990.539554,0.005,-1262.882,51.3,14905.534,02309c4b,13,0,22941454.717,0.042,-120558183.531157,0.009,2111.590,45.7,3984.555,1810bc64,13,0,22941453.693,0.180,-93941456.741696,0.011,1645.396,41.2,3978.255,11303c6b,15,0,22752775.227,0.046,-119566687.005913,0.012,3112.730,44.8,3628.275,0810bc84,15,0,22752775.420,0.118,-93168858.308908,0.012,2425.506,44.9,3622.754,01303c8b,15,0,22752775.932,0.043,-93168851.550483,0.012,2425.505,46.7,3625.014,02309c8b,16,0,23437165.563,0.058,-123163154.592686,0.010,1667.743,42.8,3318.855,1810bca4,16,0,23437164.832,0.179,-95971289.711375,0.012,1299.540,41.3,3313.415,11303cab,18,0,20870226.835,0.024,-109673843.056839,0.005,1782.527,50.4,7988.975,1810bcc4,18,0,20870225.520,0.080,-85460161.951054,0.006,1388.983,48.3,7983.255,11303ccb,18,0,20870225.982,0.024,-85460166.200662,0.006,1388.983,51.9,7985.814,02309ccb,18,0,20870231.341,0.004,-81899348.012827,0.003,1331.096,55.4,7987.255,01d03cc4,5,0,20977730.840,0.024,-110238771.341810,0.006,-1850.683,50.6,12588.896,0810bd04,5,0,20977729.403,0.070,-85900356.288455,0.006,-1442.091,49.4,12583.755,01303d0b,5,0,20977730.127,0.030,-85900355.538492,0.006,-1442.091,49.8,12585.716,02309d0b,26,0,22753733.201,0.042,-119571694.561007,0.008,-494.798,45.7,7318.775,1810bd24,26,0,22753735.163,0.118,-93172767.333088,0.010,-385.557,44.9,7312.755,11303d2b,26,0,22753735.387,0.043,-93172769.582418,0.009,-385.557,46.7,7315.375,02309d2b,26,0,22753735.481,0.008,-89290576.088766,0.005,-369.472,50.1,7316.815,01d03d24,23,0,23067782.934,0.040,-121222050.181679,0.009,3453.274,46.0,3078.894,0810bd44,23,0,23067783.254,0.142,-94458759.273215,0.010,2690.865,43.3,3073.754,01303d4b,23,0,23067783.763,0.040,-94458764.522108,0.009,2690.865,47.3,3076.395,02309d4b,23,0,23067789.450,0.007,-90523004.360543,0.004,2578.883,51.2,3077.834,01d03d44,194,0,43027813.095,0.065,-226112681.899748,0.013,43.499,41.9,17178.695,1815be04,194,0,43027815.196,0.059,-176191709.875251,0.014,33.896,44.0,17173.014,02359e0b,194,0,43027817.865,0.014,-168850394.176443,0.007,32.406,45.4,17177.053,01d53e04,131,0,38480107.260,0.124,-202214296.438902,0.009,-0.922,46.2,292335.531,48023e84,133,0,38618703.555,0.119,-202942631.161186,0.007,0.421,46.7,916697.188,58023ec4,138,0,38495561.597,0.116,-202295515.714333,0.008,-4.752,46.8,292343.625,48023ee4,45,13,20655319.254,0.111,-110608334.938276,0.006,-1928.119,46.3,9728.839,18119f04,45,13,20655320.731,0.021,-86028727.119001,0.006,-1499.649,45.9,9724.239,10b13f0b,45,13,20655321.099,0.092,-86028721.367030,0.006,-1499.649,46.1,9725.238,10319f0b,53,6,23361335.550,0.284,-124792043.406215,0.017,1741.893,38.1,444.840,08119f24,53,6,23361340.271,0.098,-97060514.793435,0.017,1354.807,32.6,444.741,00b13f2b,53,6,23361339.423,0.393,-97060517.036654,0.018,1354.806,33.5,444.801,10319f2b,60,10,20724752.466,0.106,-110863493.957380,0.007,-2492.451,46.7,16549.037,18019f44,39,3,23534282.253,0.169,-125583452.109842,0.012,4608.280,42.6,557.668,08119f84,39,3,23534291.023,0.027,-97676038.550992,0.013,3584.223,43.8,552.119,10b13f8b,39,3,23534290.639,0.108,-97676048.806539,0.013,3584.223,44.7,552.959,10319f8b,61,9,19285134.504,0.086,-103126338.171372,0.005,228.766,48.6,11128.199,08119fa4,61,9,19285138.043,0.020,-80209402.132964,0.005,177.929,46.3,11124.118,00b13fab,61,9,19285138.376,0.084,-80209411.390794,0.005,177.929,46.9,11125.037,00319fab,52,7,22348227.548,0.137,-119422164.171132,0.008,-1798.230,44.4,7458.668,08119fc4,52,7,22348232.044,0.025,-92883929.564420,0.008,-1398.625,44.4,7453.898,00b13fcb,52,7,22348232.124,0.104,-92883930.822797,0.008,-1398.624,45.0,7455.038,10319fcb,54,11,21518220.426,0.169,-115148393.440041,0.010,3262.249,42.6,3877.098,18119fe4,54,11,21518225.678,0.025,-89559888.534930,0.010,2537.306,44.6,3871.818,00b13feb,54,11,21518226.376,0.107,-89559882.794247,0.010,2537.307,44.8,3872.818,10319feb,51,0,23917426.780,0.130,-127493324.706900,0.008,-3976.867,44.9,13028.379,08119c04,51,0,23917434.944,0.031,-99161492.405944,0.010,-3093.121,42.6,13024.238,10b13c0b,51,0,23917434.780,0.126,-99161488.657552,0.010,-3093.121,43.4,13025.178,00319c0b,38,8,19851538.779,0.107,-106117893.493769,0.007,1849.414,46.6,6208.818,08119c24,38,8,19851544.763,0.031,-82536182.471767,0.007,1438.434,42.6,6204.118,00b13c2b,38,8,19851543.771,0.124,-82536181.722576,0.007,1438.434,43.6,6205.038,00319c2b,25,0,27861125.116,0.078,-146411169.405727,0.011,-3136.592,43.2,21188.543,08539cc4,25,0,27861133.366,0.009,-109333028.194067,0.005,-2342.203,49.0,21186.443,01933cc4,25,0,27861129.463,0.011,-112185182.897162,0.006,-2403.344,47.0,21186.443,02333cc4,25,0,27861129.580,0.007,-110759098.611107,0.006,-2372.787,50.8,21186.164,02933cc4,4,0,25274631.488,0.038,-132819124.897734,0.006,997.361,49.6,7638.783,08539ce4,4,0,25274635.181,0.007,-99183140.380658,0.004,744.803,50.8,7636.565,01933ce4,4,0,25274631.890,0.007,-101770517.169783,0.004,764.254,50.9,7636.444,02333ce4,4,0,25274631.708,0.005,-100476824.840813,0.004,754.545,53.6,7636.363,02933ce4,12,0,26373649.887,0.092,-138594449.111813,0.012,-2565.281,41.8,26740.730,08539d04,12,0,26373653.619,0.019,-103495853.823161,0.008,-1915.449,42.6,26738.648,01933d04,12,0,26373650.081,0.023,-106195738.067164,0.011,-1965.500,41.1,26738.449,02333d04,12,0,26373650.251,0.015,-104845791.009488,0.010,-1940.442,44.6,26738.371,02933d04,11,0,22137124.256,0.039,-116331408.305147,0.015,-1200.216,49.2,19415.590,08539d24,11,0,22137125.344,0.008,-86870883.829203,0.011,-896.289,49.8,19413.172,01933d24,11,0,22137122.146,0.008,-89137066.170706,0.012,-919.719,49.6,19413.248,02333d24,11,0,22137121.891,0.006,-88003971.568373,0.011,-908.028,52.4,19413.172,02933d24,30,0,25928558.680,0.072,-136255508.290211,0.010,743.664,43.9,3960.112,08539d44,30,0,25928564.638,0.011,-101749279.487957,0.006,555.328,47.5,4752.748,01933d44,30,0,25928561.460,0.010,-104403592.595320,0.005,569.759,48.1,4753.047,02333d44,30,0,25928561.332,0.008,-103076425.609137,0.006,562.539,50.5,4752.767,02933d44,2,0,25889111.981,0.043,-136048218.157560,0.006,-1792.931,48.4,12654.424,08539d64,2,0,25889117.006,0.009,-101594476.864922,0.005,-1338.866,48.7,12652.444,01933d64,2,0,25889114.168,0.009,-104244753.680674,0.004,-1373.765,49.5,12651.978,02333d64,2,0,25889113.739,0.007,-102919609.843844,0.005,-1356.370,51.8,12651.943,02933d64,19,0,27039623.380,0.118,-142094196.888887,0.015,-1878.632,39.7,11125.104,08539d84,19,0,27039628.887,0.020,-106109319.847355,0.010,-1402.842,41.9,11123.043,01933d84,19,0,27039625.153,0.024,-108877382.476710,0.011,-1439.341,40.6,11122.757,02333d84,19,0,27039625.337,0.016,-107493348.232960,0.010,-1421.103,44.1,11122.765,02933d84,36,0,23927504.603,0.030,-125739945.419298,0.005,1241.596,51.7,11037.264,08539da4,36,0,23927510.217,0.006,-93896767.646843,0.004,927.156,52.9,11035.164,01933da4,36,0,23927507.273,0.006,-96346233.181780,0.004,951.361,53.2,11035.376,02333da4,36,0,23927507.057,0.004,-95121494.979676,0.004,939.285,55.8,11031.874,02933da4,9,0,24890379.004,0.046,-130799846.144936,0.007,3052.621,47.8,2955.889,08539dc4,9,0,24890384.304,0.009,-97675250.055577,0.005,2279.540,49.1,2953.828,01933dc4,9,0,24890381.065,0.008,-100223286.825938,0.004,2338.979,50.0,2953.762,02333dc4,9,0,24890381.366,0.006,-98949262.506583,0.005,2309.297,52.2,2949.700,02933dc4,23,0,26593863.945,0.036,-138481231.000933,0.010,-48.553,44.1,2628.888,08149ec4,23,0,26593862.563,0.010,-104360035.310223,0.005,-36.590,48.2,2623.647,41343ec4,34,0,23330414.273,0.017,-121487628.857801,0.005,2280.558,50.6,6539.069,58149ee4,34,0,23330415.641,0.008,-91553618.939049,0.005,1718.641,50.1,6533.770,41343ee4,35,0,24822913.452,0.024,-129259432.414616,0.007,-2925.143,47.4,23499.049,58149f04,35,0,24822915.980,0.012,-97410461.716286,0.006,-2204.368,46.7,23493.830,41343f04,11,0,24964039.739,0.052,-129994328.361984,0.014,2939.333,40.8,2708.970,18149f24,11,0,24964038.060,0.022,-100519869.959755,0.006,2272.851,48.3,2708.810,00349f24,19,0,23905947.282,0.033,-124484578.888819,0.009,-2342.726,44.8,13489.051,18149f44,19,0,23905949.046,0.008,-93812119.376225,0.005,-1765.479,50.1,13483.831,41343f44,21,0,24577306.170,0.027,-127980528.823414,0.008,3242.344,46.7,3439.068,18149f84,21,0,24577307.993,0.008,-96446682.849511,0.005,2443.502,49.7,3433.828,41343f84,22,0,22438270.920,0.015,-116842012.781567,0.005,729.096,51.5,8979.049,18149fa4,22,0,22438269.274,0.005,-88052653.428423,0.003,549.506,54.1,8973.770,41343fa4,44,0,21553538.984,0.014,-112234979.640419,0.005,-679.127,52.1,15439.131,48149ca4,44,0,21553540.824,0.005,-84580779.869687,0.003,-511.716,53.6,15433.829,41343ca4,57,0,26771391.610,0.021,-139405685.309616,0.007,-2069.940,48.5,20196.455,48049d04,12,0,21542689.063,0.021,-112178498.767984,0.006,952.964,48.8,11229.051,18149d24,12,0,21542686.409,0.013,-86743545.297369,0.004,736.976,52.6,11228.890,00349d24,25,0,26603375.741,0.069,-138530755.415895,0.019,-2155.462,38.4,9789.050,18149d44,25,0,26603380.238,0.015,-104397363.013083,0.007,-1624.205,44.7,9783.829,41343d44*5e9785bd\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 69;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_RANGE_ROUNDING_ERROR)
{
   unsigned char aucLog[] = "#RANGEA,COM1,0,51.5,FINESTEERING,2166,395964.000,02000000,5103,16248;122,14,0,21667472.815,0.037,-113863359.396500,0.008,-2377.013,46.9,17892.975,0810bc04,14,0,21667471.798,0.100,-88724695.332287,0.008,-1852.218,46.4,17887.254,01303c0b,14,0,21667471.869,0.030,-88724703.580824,0.008,-1852.218,49.9,17889.613,02309c0b,14,0,21667477.147,0.006,-85027853.855276,0.004,-1775.136,52.4,17891.055,01d03c04,30,0,24951876.935,0.052,-131122991.675337,0.011,-3887.187,43.9,22862.973,0810bc24,30,0,24951878.854,0.144,-102173767.330929,0.012,-3028.978,43.2,22858.254,01303c2b,30,0,24951879.324,0.054,-102173767.578720,0.011,-3028.978,44.8,22859.914,02309c2b,30,0,24951881.178,0.010,-97916529.416470,0.005,-2902.729,48.4,22861.453,01d03c24,6,0,23512406.638,0.052,-123558540.466820,0.013,3695.143,43.8,2282.935,1810bc44,6,0,23512408.025,0.137,-96279393.879062,0.013,2879.333,43.6,2276.754,11303c4b,6,0,23512408.964,0.040,-96279396.122659,0.012,2879.333,47.4,2279.715,02309c4b,6,0,23512410.302,0.009,-92267760.963149,0.005,2759.320,49.1,2281.196,01d03c44,15,0,24916477.333,0.059,-130936968.837648,0.013,-3034.036,42.7,12412.816,0810bc64,15,0,24916476.620,0.188,-102028802.341248,0.015,-2364.185,40.9,12407.755,01303c6b,15,0,24916477.107,0.072,-102028805.583397,0.013,-2364.185,42.1,12409.297,02309c6b,22,0,25066199.607,0.069,-131723770.888638,0.023,903.820,41.3,1451.073,1810bc84,22,0,25066195.782,0.255,-102641891.725302,0.027,704.276,38.2,1445.754,11303c8b,19,0,20174355.199,0.023,-106016955.400284,0.006,1017.117,51.0,9052.854,0810bca4,19,0,20174350.725,0.072,-82610603.579487,0.006,792.558,49.2,9047.515,01303cab,24,0,21615388.172,0.031,-113589632.948200,0.007,1528.792,48.4,5999.336,1810bcc4,24,0,21615387.962,0.095,-88511414.895175,0.007,1191.266,46.8,5993.755,11303ccb,24,0,21615388.468,0.032,-88511406.149953,0.007,1191.267,49.2,5996.136,02309ccb,24,0,21615390.665,0.006,-84823446.786208,0.004,1141.508,52.7,5997.635,01d03cc4,13,0,24724806.437,0.066,-129929733.351660,0.014,-3521.752,41.7,16102.854,0810bce4,13,0,24724805.621,0.280,-101243942.572119,0.018,-2744.224,37.4,16097.756,01303ceb,21,0,25716810.210,0.052,-135142718.937328,0.015,-3128.023,43.8,12892.756,1810bd04,21,0,25716807.897,0.234,-105305999.403490,0.016,-2437.422,38.9,12887.416,11303d0b,17,0,20678889.831,0.020,-108668308.997997,0.005,-1124.563,51.9,12322.776,1810bd24,17,0,20678887.276,0.063,-84676598.609518,0.006,-876.283,50.3,12317.415,11303d2b,17,0,20678887.695,0.027,-84676606.858549,0.005,-876.283,50.8,12319.616,02309d2b,1,0,22989835.696,0.052,-120812394.789843,0.011,-2576.784,43.8,10700.536,1810bd44,1,0,22989837.021,0.132,-94139526.428291,0.010,-2007.885,43.9,10694.756,11303d4b,1,0,22989837.105,0.047,-94139525.678403,0.011,-2007.885,45.9,10697.216,02309d4b,1,0,22989837.062,0.008,-90217051.711178,0.005,-1924.169,50.0,10698.655,01d03d44,12,0,24793054.106,0.050,-130288376.315651,0.010,3507.159,44.0,442.614,0810bd64,12,0,24793052.865,0.197,-101523404.561717,0.013,2732.850,40.4,436.254,01303d6b,12,0,24793053.357,0.081,-101523411.805143,0.011,2732.850,41.2,439.215,02309d6b,131,0,38479354.294,0.122,-202210368.857420,0.009,-0.740,46.4,548123.375,48023e84,135,0,38558357.297,1.273,-202625536.414007,0.034,2.750,45.7,2.609,48023ea4,133,0,38626890.881,0.113,-202985679.907909,0.011,1.345,47.0,548121.188,58023ec4,138,0,38496470.429,0.115,-202300172.860430,0.010,2.604,46.9,975466.250,48023ee4,51,0,20175884.008,0.089,-107548831.258419,0.005,-2418.722,48.2,15611.236,18119f04,51,0,20175889.540,0.017,-83649110.819447,0.005,-1881.229,48.0,15606.118,00b13f0b,51,0,20175889.290,0.067,-83649115.067776,0.005,-1881.229,48.8,15606.958,10319f0b,61,9,19797266.250,0.081,-105864882.825125,0.005,-1683.446,49.0,10281.129,08119f24,61,9,19797268.522,0.018,-82339369.053414,0.005,-1309.348,47.1,10275.898,00b13f2b,61,9,19797269.032,0.078,-82339368.313369,0.005,-1309.347,47.5,10276.739,10319f2b,52,7,19441079.059,0.070,-103887233.259309,0.004,1478.277,50.3,8472.738,18119f44,52,7,19441082.501,0.016,-80801195.347749,0.004,1149.771,48.6,8468.118,00b13f4b,52,7,19441082.428,0.067,-80801194.604873,0.005,1149.771,48.9,8468.959,00319f4b,44,12,23592140.342,0.320,-126290574.937003,0.016,1158.068,37.1,1782.268,18119f84,44,12,23592143.543,0.038,-98226016.542674,0.016,900.721,40.8,1778.118,10b13f8b,44,12,23592144.060,0.160,-98226016.795870,0.016,900.720,41.3,1779.038,00319f8b,55,4,23720844.148,0.174,-126623469.505783,0.017,4540.528,42.4,435.288,08119fa4,55,4,23720849.990,0.026,-98484943.926112,0.018,3531.522,44.1,430.118,10b13fab,55,4,23720849.502,0.107,-98484948.184549,0.018,3531.523,44.8,430.959,00319fab,53,6,23345170.428,0.286,-124705714.112983,0.022,4005.376,38.0,976.358,08119fc4,53,6,23345173.285,0.083,-96993346.313948,0.023,3115.291,34.0,972.259,10b13fcb,53,6,23345172.397,0.329,-96993340.557985,0.023,3115.292,35.0,973.238,00319fcb,54,11,19866439.021,0.090,-106309339.516639,0.005,2625.298,48.0,5571.388,18119fe4,54,11,19866441.196,0.017,-82685054.461380,0.005,2041.899,47.7,5566.039,00b13feb,54,11,19866441.770,0.073,-82685054.717276,0.005,2041.899,48.1,5567.039,10319feb,60,10,22600053.320,0.187,-120895040.710794,0.010,-4059.449,41.7,13591.559,18019c04,36,0,28384363.934,0.132,-149160837.631359,0.013,3344.571,42.7,54.004,08539cc4,36,0,28384370.165,0.026,-111386362.575417,0.008,2497.580,44.0,51.641,01933cc4,36,0,28384366.856,0.024,-114292079.525855,0.007,2562.721,44.6,51.642,02333cc4,36,0,28384366.993,0.020,-112839213.113177,0.008,2530.178,46.2,51.565,02933cc4,25,0,23636884.911,0.027,-124212693.603155,0.005,836.383,52.6,12990.103,08539d04,25,0,23636889.504,0.006,-92756258.196410,0.004,624.605,53.3,12988.042,01933d04,25,0,23636886.097,0.005,-95175971.627448,0.003,640.889,53.8,12987.756,02333d04,25,0,23636886.124,0.004,-93966110.980589,0.003,632.726,55.4,12987.764,02933d04,24,0,24846946.159,0.032,-130571605.025648,0.005,-1807.016,51.1,22328.822,08539d24,24,0,24846948.793,0.007,-97504795.166661,0.004,-1349.399,51.6,22327.043,01933d24,24,0,24846945.710,0.006,-100048383.385584,0.004,-1384.560,53.3,22326.754,02333d24,24,0,24846945.678,0.005,-98776584.342612,0.004,-1366.959,54.3,22326.563,02933d24,8,0,25516901.814,0.071,-134092226.312443,0.009,-813.882,44.1,6892.478,08539d44,8,0,25516905.563,0.014,-100133819.235979,0.007,-607.693,44.9,6890.449,01933d44,8,0,25516902.060,0.013,-102745993.466502,0.007,-623.591,45.9,6890.450,02333d44,8,0,25516902.302,0.011,-101439901.420292,0.007,-615.692,47.2,6890.170,02933d44,11,0,23014655.101,0.047,-120942830.634705,0.017,2343.077,47.7,6259.969,08539d64,11,0,23014655.448,0.010,-90314462.515908,0.011,1749.637,47.9,6257.750,01933d64,11,0,23014652.288,0.009,-92670477.327938,0.012,1795.414,48.7,6257.982,02333d64,11,0,23014652.048,0.008,-91492463.991082,0.011,1772.540,50.1,6257.570,02933d64,12,0,22921345.477,0.049,-120452485.770580,0.008,-625.712,47.3,11376.448,08539d84,12,0,22921346.835,0.009,-89948289.930100,0.005,-467.290,49.3,11374.369,01933d84,12,0,22921343.647,0.009,-92294758.701130,0.005,-479.561,49.3,11374.182,02333d84,12,0,22921343.748,0.007,-91121516.379511,0.005,-473.374,51.1,11374.170,02933d84,33,0,27957531.600,0.097,-146917798.950613,0.014,-2793.492,41.4,15070.385,08539da4,33,0,27957536.103,0.014,-109711347.118576,0.007,-2085.986,45.5,15068.044,01933da4,33,0,27957533.204,0.012,-112573368.693827,0.006,-2140.510,46.2,15068.756,02333da4,33,0,27957532.694,0.011,-111142353.973864,0.007,-2113.279,47.7,15067.944,02933da4,2,0,26654945.600,0.082,-140072694.237015,0.011,2558.429,42.8,2028.863,08539dc4,2,0,26654948.909,0.011,-104599749.952841,0.006,1910.490,47.6,3259.644,01933dc4,2,0,26654946.389,0.009,-107328429.627432,0.005,1960.239,49.4,3259.044,02333dc4,2,0,26654945.686,0.008,-105964084.356438,0.005,1935.353,50.3,3258.764,02933dc4,3,0,27925045.367,0.143,-146747109.367117,0.020,-2594.649,38.0,14980.084,08539de4,3,0,27925052.310,0.010,-109583900.965844,0.005,-1937.513,48.3,14978.242,01933de4,3,0,27925049.007,0.010,-112442593.146502,0.005,-1988.074,48.5,14978.243,02333de4,3,0,27925048.907,0.008,-111013242.126755,0.005,-1962.813,50.1,14977.764,02933de4,28,0,24295346.534,0.022,-126512252.843586,0.007,3115.172,48.3,3583.050,08149ee4,28,0,24295347.953,0.010,-95340169.032822,0.005,2347.575,48.5,3577.830,41343ee4,58,0,30352505.594,0.018,-158053468.415728,0.005,2180.668,50.2,6805.764,48049f04,37,0,23654585.807,0.024,-123175640.316366,0.007,2315.890,47.3,5903.148,48149f64,37,0,23654591.469,0.007,-92825691.167170,0.004,1745.292,50.8,5897.829,41343f64,16,0,40981058.030,0.070,-213399122.234510,0.018,1021.730,38.2,2543.120,18149fa4,16,0,40981060.084,0.069,-165013741.944354,0.017,789.719,38.3,2542.900,10349fa4,25,0,24527132.667,0.024,-127719233.869791,0.007,-3012.064,47.3,22603.049,18149fc4,25,0,24527132.642,0.009,-96249757.320986,0.005,-2269.882,49.0,22597.830,41343fc4,11,0,22072594.483,0.021,-114937804.338566,0.007,-1124.209,48.7,12583.050,18149c44,11,0,22072591.972,0.012,-88877191.965456,0.004,-869.322,53.6,12582.891,00349c44,39,0,40758395.962,0.093,-212239668.209597,0.022,1033.544,35.7,1823.061,48149c64,39,0,40758396.181,0.018,-159944708.054331,0.008,779.072,43.1,1817.821,41343c64,6,0,40660168.462,0.068,-211728167.318087,0.020,933.249,38.4,3533.121,08149ca4,6,0,40660164.534,0.062,-163721634.174305,0.015,721.687,39.2,3532.960,00349ca4,23,0,21983418.177,0.014,-114473435.239145,0.004,-605.859,52.1,14413.048,18149d04,23,0,21983413.987,0.005,-86267655.346871,0.003,-456.641,55.0,14407.829,41343d04,12,0,25523288.744,0.050,-132906445.913874,0.014,-3185.714,41.2,17202.971,18149d24,12,0,25523285.648,0.047,-102771684.666844,0.012,-2463.359,41.6,17202.811,00349d24,43,0,21964477.374,0.015,-114374807.893314,0.005,517.000,51.5,10043.069,48149d44,43,0,21964479.560,0.005,-86193345.125571,0.003,389.631,53.6,10037.829,41343d44,34,0,23704649.976,0.029,-123436343.578545,0.007,-2381.759,46.0,15113.069,48149d64,34,0,23704653.450,0.008,-93022152.754343,0.005,-1794.921,50.1,15107.829,41343d64*3e6cb7c9\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 69;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::MESSAGEDATA_COMPARISON_ERROR, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_RANGECMP)
{
   unsigned char aucLog[] = "#RANGECMP2A,COM1,0,56.0,FINESTEERING,2171,404649.000,02010000,1fe3,16248;1870,000200c8ba5b859afb2fe1ffff6b3f0651e830813d00e4ffff43bac60a006c803d0001140034b7f884a8ff2fe1ffff6b3fa428a83c82f0ffe4ffff439c4404c8cb82f0ff021d00043bfd04720330e1ffff6b3f2628086b811200e4ffff439ca605283f811200e5ffff095d860f50b081120003060020dbf8854ef94fe1ffff6b954a513855800a00e4ffff43d56a798813800a00e5ffff09782a88a836800a00e7ffff031ca4a8706980f7ff041f001822d685d8fc3fe1ffff6b5b483218a2003b00e4ffff43f1280ee054003b00e5ffff09b268154897003b00050900ac57ef85effe4fe1ffff6b948c0a705680f7ffe4ffff43d44c1ea87900f7ffe5ffff095bac23987d00f7ffe7ffff031fa249f0148116000612001813cb059e0640e1ffff6b59480fb0da802d00e4ffff43f38a07183e812d00e5ffff09966a12c0f3002e00e7ffff031b2669187782190007190048e81385abfb4fe1ffff2b3e6639208800eaffe4ffff039b4649586400eaffe5ffff095ee651583900eaffe7ffff031f827020ac00e0ff080500f8ce12059b0430e1ffff6b3f842c5829820c00e4ffff439c040b50e5820c00e5ffff095da414788b820c00091a00d4c6dd85140640e1ffff6b92ae0b289300ccffe4ffff43f30e35f0db80cbffe5ffff0978ce38a89100ccffe7ffff031c643a885081c8ff0b0c00e88f7105f0f83fe1ffff2b5c4686e805011c00e4ffff03b82669c03e801b00e5ffff097a866f70a0801b0010c270b8074e8a660030e1ffff2b78e840084080edffe3ffff0978884af01500edffe4ffff0319e671088f80f4ff14852054613589010010e1ffff63bba60ab02200c7ff158a208c6a2d89000010e1ffff63bc0880503f00260017832000972c89000010e1ffff63bb885f2007000000180d15640900851f0030e1ffff290fcd0f18f900deffe4ffff43564e4e70b001deffe3ffff49d30e4cf0a401deff190c168cd722052af93fe1ffff29b9a619283300f4ffe4ffff031b066e00bf80f3ffe3ffff499b266988b380f3ff1a171a60005285370610e1ffff69d7660410220114001b151be8a3298543fa3fe1ffff69d72608885800e2ffe4ffff033a4635788a00e2ffe3ffff499a663e306000e2ff1c16146892a3046bff3fe1ffff6911cd11d03300e8ffe4ffff43714c55482f01e8ffe3ffff09f12c5cf85101e8ff1d071c9c3942853f0730e1ffff69d6c60f705e01e8ffe4ffff0339463a98cf82e8ffe3ffff499ae641682083e8ff1e0e10fc64a785d90630e1ffff29f3ca0e1021801a00e4ffff4337aa7fe833811a00e3ffff09b8ca7610fa801a001f05188c42a9854ef93fe1ffff29f1ea06585080dbffe4ffff4372ea46280f01dbffe3ffff49f20c50504101dbff2006137c4000059e0010e1ffff690e3904080400c5ff261a5064418705fbfd4fe1ffff293f0406908b80ecffe2ffff031f6264f8e601e6ffe3ffff031fc22ec85801ebffe4ffff031fe22ae05681e8ff270c50ec595586230540e1ffff29950a02c04b801900e2ffff031ac6496035812200e3ffff031924168079001f00e4ffff031ca6110086802400280d50488d8506ebfa4fe1ffff29980839600500c9ffe2ffff031a668fd80681d2ffe3ffff0319066b801300bcffe4ffff031c8654401b80c1ff291f5034b8e385a5ff4fe1ffff295f640c683700e0ffe2ffff031f225b802581daffe3ffff031fe21d906b00d9ffe4ffff031f4221609780d8ff2b2150f8eac105a70240e1ffff293f641468ef802500e2ffff031f8240905c022000e3ffff031f82042888812900e4ffff031f6207e0a80124002c0850309a0206250040e1ffff2979e80eb8b9003100e2ffff031b044018e7013200e3ffff031c441dd036812300e4ffff031ea413e0650128002d0150f8db2f068afa4fe1ffff297ce63c0043001b00e2ffff031fe29948fa801a00e3ffff031e84589847801a00e4ffff031f045e883f001a002e0750dc257686740440e1ffff297a680f881f81d4ffe2ffff031e047970f102c2ffe3ffff031ea249f85e02bfffe4ffff031f244390fe81bfff2f1850d08c82065f0440e1ffff2998a803488b00e4ffe2ffff031b664f981202ccffe3ffff031bc40f201201cdffe4ffff031d4418602001ccff362d6040494c060f0420e1ffff6958e80fe837003d00f4ffff031ca4acd845823000371c60983fad8543fb2fe1ffff293a860f388f00cdfff4ffff031ee234b8c680ccff3b1e60ccf2a885dc0420e1ffff293b6606800701effff4ffff031e6446402f02edff3f3a607ca3168851fa1fe1ffff2957a8589829000700410e60701bf60529fc2fe1ffff2976e82a880101ebffe3ffff093ce40148e480e3ff422e607085ec05acfa2fe1ffff293a06614007002600f4ffff031e42e7b01981240044216008d2be85f6fe2fe1ffff293b060f0053813e00f4ffff031f22bf8111863b00451b6048481885190020e1ffff691f0204d06a001800f4ffff031f621b986e810f0047246044cfe4053cff2fe1ffff293bc60ea00a001700f4ffff031d249748ad8219004b29600c07b9859e0420e1ffff293b460e98a9011c00f4ffff031fe2de305f850700*2b134683\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 73;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_RANGECMP2)
{
   unsigned char aucLog[] = "#RANGECMP2A,COM1,0,56.0,FINESTEERING,2171,404649.000,02010000,1fe3,16248;1870,000200c8ba5b859afb2fe1ffff6b3f0651e830813d00e4ffff43bac60a006c803d0001140034b7f884a8ff2fe1ffff6b3fa428a83c82f0ffe4ffff439c4404c8cb82f0ff021d00043bfd04720330e1ffff6b3f2628086b811200e4ffff439ca605283f811200e5ffff095d860f50b081120003060020dbf8854ef94fe1ffff6b954a513855800a00e4ffff43d56a798813800a00e5ffff09782a88a836800a00e7ffff031ca4a8706980f7ff041f001822d685d8fc3fe1ffff6b5b483218a2003b00e4ffff43f1280ee054003b00e5ffff09b268154897003b00050900ac57ef85effe4fe1ffff6b948c0a705680f7ffe4ffff43d44c1ea87900f7ffe5ffff095bac23987d00f7ffe7ffff031fa249f0148116000612001813cb059e0640e1ffff6b59480fb0da802d00e4ffff43f38a07183e812d00e5ffff09966a12c0f3002e00e7ffff031b2669187782190007190048e81385abfb4fe1ffff2b3e6639208800eaffe4ffff039b4649586400eaffe5ffff095ee651583900eaffe7ffff031f827020ac00e0ff080500f8ce12059b0430e1ffff6b3f842c5829820c00e4ffff439c040b50e5820c00e5ffff095da414788b820c00091a00d4c6dd85140640e1ffff6b92ae0b289300ccffe4ffff43f30e35f0db80cbffe5ffff0978ce38a89100ccffe7ffff031c643a885081c8ff0b0c00e88f7105f0f83fe1ffff2b5c4686e805011c00e4ffff03b82669c03e801b00e5ffff097a866f70a0801b0010c270b8074e8a660030e1ffff2b78e840084080edffe3ffff0978884af01500edffe4ffff0319e671088f80f4ff14852054613589010010e1ffff63bba60ab02200c7ff158a208c6a2d89000010e1ffff63bc0880503f00260017832000972c89000010e1ffff63bb885f2007000000180d15640900851f0030e1ffff290fcd0f18f900deffe4ffff43564e4e70b001deffe3ffff49d30e4cf0a401deff190c168cd722052af93fe1ffff29b9a619283300f4ffe4ffff031b066e00bf80f3ffe3ffff499b266988b380f3ff1a171a60005285370610e1ffff69d7660410220114001b151be8a3298543fa3fe1ffff69d72608885800e2ffe4ffff033a4635788a00e2ffe3ffff499a663e306000e2ff1c16146892a3046bff3fe1ffff6911cd11d03300e8ffe4ffff43714c55482f01e8ffe3ffff09f12c5cf85101e8ff1d071c9c3942853f0730e1ffff69d6c60f705e01e8ffe4ffff0339463a98cf82e8ffe3ffff499ae641682083e8ff1e0e10fc64a785d90630e1ffff29f3ca0e1021801a00e4ffff4337aa7fe833811a00e3ffff09b8ca7610fa801a001f05188c42a9854ef93fe1ffff29f1ea06585080dbffe4ffff4372ea46280f01dbffe3ffff49f20c50504101dbff2006137c4000059e0010e1ffff690e3904080400c5ff261a5064418705fbfd4fe1ffff293f0406908b80ecffe2ffff031f6264f8e601e6ffe3ffff031fc22ec85801ebffe4ffff031fe22ae05681e8ff270c50ec595586230540e1ffff29950a02c04b801900e2ffff031ac6496035812200e3ffff031924168079001f00e4ffff031ca6110086802400280d50488d8506ebfa4fe1ffff29980839600500c9ffe2ffff031a668fd80681d2ffe3ffff0319066b801300bcffe4ffff031c8654401b80c1ff291f5034b8e385a5ff4fe1ffff295f640c683700e0ffe2ffff031f225b802581daffe3ffff031fe21d906b00d9ffe4ffff031f4221609780d8ff2b2150f8eac105a70240e1ffff293f641468ef802500e2ffff031f8240905c022000e3ffff031f82042888812900e4ffff031f6207e0a80124002c0850309a0206250040e1ffff2979e80eb8b9003100e2ffff031b044018e7013200e3ffff031c441dd036812300e4ffff031ea413e0650128002d0150f8db2f068afa4fe1ffff297ce63c0043001b00e2ffff031fe29948fa801a00e3ffff031e84589847801a00e4ffff031f045e883f001a002e0750dc257686740440e1ffff297a680f881f81d4ffe2ffff031e047970f102c2ffe3ffff031ea249f85e02bfffe4ffff031f244390fe81bfff2f1850d08c82065f0440e1ffff2998a803488b00e4ffe2ffff031b664f981202ccffe3ffff031bc40f201201cdffe4ffff031d4418602001ccff362d6040494c060f0420e1ffff6958e80fe837003d00f4ffff031ca4acd845823000371c60983fad8543fb2fe1ffff293a860f388f00cdfff4ffff031ee234b8c680ccff3b1e60ccf2a885dc0420e1ffff293b6606800701effff4ffff031e6446402f02edff3f3a607ca3168851fa1fe1ffff2957a8589829000700410e60701bf60529fc2fe1ffff2976e82a880101ebffe3ffff093ce40148e480e3ff422e607085ec05acfa2fe1ffff293a06614007002600f4ffff031e42e7b01981240044216008d2be85f6fe2fe1ffff293b060f0053813e00f4ffff031f22bf8111863b00451b6048481885190020e1ffff691f0204d06a001800f4ffff031f621b986e810f0047246044cfe4053cff2fe1ffff293bc60ea00a001700f4ffff031d249748ad8219004b29600c07b9859e0420e1ffff293b460e98a9011c00f4ffff031fe2de305f850700*2b134683\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 73;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_RAWGPSSUBFRAME)
{
   unsigned char aucLog[] = "#RAWGPSSUBFRAMEA,COM1,0,54.0,SATTIME,2167,254754.000,02000000,0457,16248;4,32,5,8b01dc52ee35516daa63199cfd4c00a10cb7227993c059e0b9c4d63e0054,4*80b22f2e\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 73;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_RAWWAASFRAME_2)
{
   unsigned char aucLog[] = "#RAWWAASFRAMEA_2,COM2,0,77.5,SATTIME,1747,411899.000,00000020,58e4,11526;62,138,9,c6243a0581b555352c4056aae0103cf03daff2e00057ff7fdff8010180,62*b026c677\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 69;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   // TODO: Should we be smarter about this?
   ASSERT_EQ(DecodeEncodeTest::MESSAGE_DECODER_ERROR, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_PASSCOM_CLEAN)
{
   unsigned char aucLog[] = "#PASSCOM3A,COM1,0,58.0,FINESTEERING,2171,404283.962,02010000,4c23,16248;11,1843000570\\x0d*51c2a4b6\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 72;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_PASSCOM_DIRTY)
{
   unsigned char aucLog[] = "#PASSCOM3A,COM1,0,45.5,FINESTEERING,2171,404283.635,02010000,4c23,16248;12,38400,8,N,1\\x0d*c4052078\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 72;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_TRACKSTAT)
{
   unsigned char aucLog[] = "#TRACKSTATA,COM1,0,58.0,FINESTEERING,2166,318996.000,02000000,457c,16248;SOL_COMPUTED,WAAS,5.0,235,2,0,0810bc04,20999784.925,770.496,49.041,8473.355,0.228,GOOD,0.975,2,0,01303c0b,20999781.972,600.387,49.021,8466.896,0.000,OBSL2,0.000,0,0,02208000,0.000,-0.004,0.000,0.000,0.000,NA,0.000,0,0,01c02000,0.000,0.000,0.000,0.000,0.000,NA,0.000,20,0,0810bc24,24120644.940,3512.403,42.138,1624.974,0.464,GOOD,0.588,20,0,01303c2b,24120645.042,2736.937,39.553,1619.755,0.000,OBSL2,0.000,0,0,02208020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02020,0.000,0.000,0.000,0.000,0.000,NA,0.000,6,0,0810bc44,20727107.371,-1161.109,50.325,11454.975,-0.695,GOOD,0.979,6,0,01303c4b,20727108.785,-904.761,50.213,11448.915,0.000,OBSL2,0.000,6,0,02309c4b,20727109.344,-904.761,52.568,11451.815,0.000,OBSL2,0.000,6,0,01d03c44,20727110.520,-867.070,55.259,11453.455,0.000,OBSL5,0.000,29,0,0810bc64,25296813.545,3338.614,43.675,114.534,-0.170,GOOD,0.206,29,0,01303c6b,25296814.118,2601.518,39.636,109.254,0.000,OBSL2,0.000,29,0,02309c6b,25296814.580,2601.517,40.637,111.114,0.000,OBSL2,0.000,0,0,01c02060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02080,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,02208080,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02080,0.000,0.000,0.000,0.000,0.000,NA,0.000,19,0,0810bca4,22493227.199,-3020.625,44.911,18244.973,0.411,GOOD,0.970,19,0,01303cab,22493225.215,-2353.736,44.957,18239.754,0.000,OBSL2,0.000,0,0,022080a0,0.000,-0.006,0.000,0.000,0.000,NA,0.000,0,0,01c020a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,24,0,0810bcc4,23856706.090,-3347.685,43.417,15187.116,-0.358,GOOD,0.957,24,0,01303ccb,23856708.306,-2608.588,43.207,15181.256,0.000,OBSL2,0.000,24,0,02309ccb,23856708.614,-2608.588,46.741,15183.815,0.000,OBSL2,0.000,24,0,01d03cc4,23856711.245,-2499.840,50.038,15185.256,0.000,OBSL5,0.000,25,0,1810bce4,21953295.423,2746.317,46.205,4664.936,0.322,GOOD,0.622,25,0,11303ceb,21953296.482,2139.988,45.623,4658.756,0.000,OBSL2,0.000,25,0,02309ceb,21953296.899,2139.988,47.584,4661.796,0.000,OBSL2,0.000,25,0,01d03ce4,21953298.590,2050.845,51.711,4662.976,0.000,OBSL5,0.000,0,0,0000a100,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02100,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,02208100,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02100,0.000,0.000,0.000,0.000,0.000,NA,0.000,17,0,1810bd24,24833573.179,-3002.286,43.809,21504.975,-0.219,GOOD,0.903,17,0,11303d2b,24833573.345,-2339.444,42.894,21499.256,0.000,OBSL2,0.000,17,0,02309d2b,24833573.677,-2339.444,44.238,21501.717,0.000,OBSL2,0.000,0,0,01c02120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02140,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,02208140,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02140,0.000,0.000,0.000,0.000,0.000,NA,0.000,12,0,0810bd64,20275478.792,742.751,50.336,9634.855,0.166,GOOD,0.977,12,0,01303d6b,20275477.189,578.767,50.042,9629.756,0.000,OBSL2,0.000,12,0,02309d6b,20275477.555,578.767,51.012,9631.516,0.000,OBSL2,0.000,0,0,01c02160,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02180,0.000,0.002,0.000,0.000,0.000,NA,0.000,0,0,02208180,0.000,0.003,0.000,0.000,0.000,NA,0.000,0,0,01c02180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021a0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,022081a0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021c0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,022081c0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021e0,0.000,0.001,0.000,0.000,0.000,NA,0.000,0,0,022081e0,0.000,0.003,0.000,0.000,0.000,NA,0.000,0,0,01c021e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,194,0,0815be04,43478223.927,63.042,38.698,2382.214,0.000,NODIFFCORR,0.000,194,0,02359e0b,43478226.941,49.122,44.508,2378.714,0.000,OBSL2,0.000,194,0,01d53e04,43478228.121,47.080,43.958,2380.253,0.000,OBSL5,0.000,0,0,0005a220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258240,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c52240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52260,0.000,0.000,0.000,0.000,0.000,NA,0.000,131,0,48023e84,38480992.384,-0.167,45.356,471155.406,0.000,LOCKEDOUT,0.000,135,0,58023ea4,38553658.881,3.771,44.648,4.449,0.000,NODIFFCORR,0.000,133,0,58023ec4,38624746.161,1.065,45.618,471153.219,0.000,LOCKEDOUT,0.000,138,0,48023ee4,38493033.873,0.953,45.833,898498.250,0.000,LOCKEDOUT,0.000,55,4,18119f04,21580157.377,3208.835,44.921,3584.798,0.000,NODIFFCORR,0.000,55,4,00b13f0b,21580163.823,2495.762,45.078,3580.119,0.000,OBSL2,0.000,55,4,10319f0b,21580163.635,2495.762,45.682,3581.038,0.000,OBSL2,0.000,45,13,08119f24,23088997.031,-313.758,44.105,4273.538,0.000,NODIFFCORR,0.000,45,13,00b13f2b,23088998.989,-244.036,42.927,4267.818,0.000,OBSL2,0.000,45,13,00319f2b,23088999.269,-244.036,43.297,4268.818,0.000,OBSL2,0.000,54,11,18119f44,19120160.469,178.235,50.805,9344.977,0.000,NODIFFCORR,0.000,54,11,00b13f4b,19120162.255,138.627,46.584,9339.897,0.000,OBSL2,0.000,54,11,00319f4b,19120162.559,138.627,47.049,9340.818,0.000,OBSL2,0.000,0,0,00018360,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12360,0.000,0.004,0.000,0.000,0.000,NA,0.000,0,0,00218360,0.000,0.004,0.000,0.000,0.000,NA,0.000,0,0,00018380,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12380,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218380,0.000,0.000,0.000,0.000,0.000,NA,0.000,53,6,18119fa4,21330036.443,3045.661,43.167,3862.756,0.000,NODIFFCORR,0.000,53,6,00b13fab,21330040.203,2368.849,41.759,3858.039,0.000,OBSL2,0.000,53,6,00319fab,21330039.119,2368.850,42.691,3859.038,0.000,OBSL2,0.000,38,8,18119fc4,22996582.245,2427.724,41.817,2014.338,0.000,NODIFFCORR,0.000,38,8,10b13fcb,22996590.440,1888.231,35.968,2010.119,0.000,OBSL2,0.000,38,8,10319fcb,22996589.454,1888.230,36.755,2011.038,0.000,OBSL2,0.000,52,7,08119fe4,19520740.266,-1275.394,50.736,10712.179,0.000,NODIFFCORR,0.000,52,7,00b13feb,19520744.583,-991.974,47.931,10708.038,0.000,OBSL2,0.000,52,7,10319feb,19520744.527,-991.974,48.251,10709.038,0.000,OBSL2,0.000,51,0,18119c04,22302364.417,-4314.112,43.692,16603.602,0.000,NODIFFCORR,0.000,51,0,00b13c0b,22302371.827,-3355.424,45.975,16603.580,0.000,OBSL2,0.000,51,0,00319c0b,22302371.325,-3355.424,46.904,16603.502,0.000,OBSL2,0.000,61,9,08119c24,21163674.206,-3198.898,47.898,14680.979,0.000,NODIFFCORR,0.000,61,9,10b13c2b,21163677.196,-2488.033,44.960,14675.897,0.000,OBSL2,0.000,61,9,00319c2b,21163677.300,-2488.033,45.628,14676.737,0.000,OBSL2,0.000,0,0,00018040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00018060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12060,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,00218060,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,00018080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a120a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,002180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004380c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,21,0,08539ce4,25416828.004,2077.626,46.584,6337.363,0.000,NODIFFCORR,0.000,21,0,01933ce4,25416833.286,1551.460,49.589,6335.164,0.000,OBSE5,0.000,21,0,02333ce4,25416829.717,1591.910,50.226,6335.176,0.000,OBSE5,0.000,21,0,02933ce4,25416829.814,1571.722,52.198,6334.944,0.000,OBSE5,0.000,27,0,08539d04,23510780.996,-707.419,51.721,16182.524,0.000,NODIFFCORR,0.000,27,0,01933d04,23510785.247,-528.262,53.239,16180.444,0.000,OBSE5,0.000,27,0,02333d04,23510781.458,-542.015,53.731,16180.243,0.000,OBSE5,0.000,27,0,02933d04,23510781.960,-535.149,55.822,16180.165,0.000,OBSE5,0.000,0,0,00438120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832120,0.000,0.000,0.000,0.000,0.000,NA,0.000,15,0,08539d44,23034423.020,183.445,51.283,11971.245,0.000,NODIFFCORR,0.000,15,0,01933d44,23034428.761,136.945,53.293,11969.243,0.000,OBSE5,0.000,15,0,02333d44,23034425.379,140.546,53.897,11969.245,0.000,OBSE5,0.000,15,0,02933d44,23034425.436,138.742,55.909,11968.946,0.000,OBSE5,0.000,13,0,08539d64,25488681.795,2565.988,46.632,4828.445,0.000,NODIFFCORR,0.000,13,0,01933d64,25488687.213,1916.182,47.753,4826.243,0.000,OBSE5,0.000,13,0,02333d64,25488683.967,1966.148,50.045,4826.243,0.000,OBSE5,0.000,13,0,02933d64,25488684.398,1941.169,51.348,4826.165,0.000,OBSE5,0.000,0,0,00438180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,30,0,08539de4,25532715.149,-2938.485,46.289,26421.467,0.000,NODIFFCORR,0.000,30,0,01933de4,25532721.371,-2194.317,49.285,26419.447,0.000,OBSE5,0.000,30,0,02333de4,25532718.174,-2251.520,50.681,26419.447,0.000,OBSE5,0.000,30,0,02933de4,25532717.843,-2222.952,52.291,26419.166,0.000,OBSE5,0.000,0,0,00438200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004382a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,41,0,48149ec4,26228546.068,2731.326,43.047,1244.968,0.000,NODIFFCORR,0.000,41,0,41343ec4,26228560.733,2058.212,46.309,1239.648,0.000,NA,0.000,27,0,08149ee4,21470141.903,-686.571,51.408,13695.229,0.000,NODIFFCORR,0.000,27,0,41343ee4,21470143.417,-517.430,52.724,13690.050,0.000,NA,0.000,6,0,08149f04,40334269.953,-663.889,38.200,12755.121,0.000,NODIFFCORR,0.000,6,0,00349f04,40334265.525,-513.549,39.333,12754.961,0.000,OBSB2,0.000,16,0,08149f24,40591561.211,-689.953,40.783,11755.120,0.000,NODIFFCORR,0.000,16,0,00349f24,40591562.100,-533.388,39.928,11754.960,0.000,OBSB2,0.000,39,0,58149f44,40402963.125,-730.398,41.019,11015.042,0.000,NODIFFCORR,0.000,39,0,41343f44,40402964.083,-550.456,43.408,11009.821,0.000,NA,0.000,30,0,18149f64,22847646.673,2123.913,50.266,6625.051,0.000,NODIFFCORR,0.000,30,0,41343f64,22847649.151,1600.605,49.656,6619.991,0.000,NA,0.000,7,0,08048381,0.000,2500.000,0.000,0.000,0.000,NA,0.000,7,0,08048381,0.000,-2500.000,0.000,0.000,0.000,NA,0.000,33,0,48149fa4,25666349.147,776.929,42.271,3835.148,0.000,NODIFFCORR,0.000,33,0,41343fa4,25666377.385,585.535,48.361,3697.589,0.000,NA,0.000,46,0,48149fc4,23048323.129,-2333.170,49.345,15915.131,0.000,NODIFFCORR,0.000,46,0,41343fc4,23048329.413,-1758.350,52.408,15909.830,0.000,NA,0.000,18,0,080483e1,0.000,4000.000,0.000,0.000,0.000,NA,0.000,18,0,080483e1,0.000,-500.000,0.000,0.000,0.000,NA,0.000,45,0,48149c04,26221109.945,2965.644,44.864,435.050,0.000,NODIFFCORR,0.000,45,0,41343c04,26221119.956,2234.910,47.292,429.831,0.000,NA,0.000,36,0,58149c24,23277715.056,700.443,48.907,8015.069,0.000,NODIFFCORR,0.000,36,0,41343c24,23277723.101,527.848,51.167,8009.829,0.000,NA,0.000,52,0,08048041,0.000,1667.000,0.000,0.000,0.000,NA,0.000,52,0,08048041,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,49,0,08048061,0.000,5832.000,0.000,0.000,0.000,NA,0.000,49,0,08048061,0.000,-4999.000,0.000,0.000,0.000,NA,0.000,47,0,08048081,0.000,1000.000,0.000,0.000,0.000,NA,0.000,47,0,08048081,0.000,-500.000,0.000,0.000,0.000,NA,0.000,58,0,48049ca4,34894393.899,-3079.127,30.345,47.772,0.000,NODIFFCORR,0.000,58,0,012420a9,0.000,-2321.139,0.000,0.000,0.000,NA,0.000,14,0,08149cc4,25730238.361,-588.324,38.191,4795.070,0.000,NODIFFCORR,0.000,14,0,00349cc4,25730237.379,-454.787,44.427,4794.910,0.000,OBSB2,0.000,28,0,08149ce4,24802536.288,-2833.581,46.004,19865.129,0.000,NODIFFCORR,0.000,28,0,41343ce4,24802537.579,-2135.389,46.897,19859.650,0.000,NA,0.000,48,0,08048101,0.000,16000.000,0.000,0.000,0.000,NA,0.000,0,0,00248100,0.000,0.000,0.000,0.000,0.000,NA,0.000,9,0,08149d24,40753569.155,222.237,37.682,1784.493,0.000,NODIFFCORR,0.000,9,0,00349d24,40753568.209,171.813,41.501,4664.961,0.000,OBSB2,0.000,3,0,08848141,0.000,6000.000,0.000,0.000,0.000,NA,0.000,3,0,08848141,0.000,-11000.000,0.000,0.000,0.000,NA,0.000,1,0,08848161,0.000,4999.000,0.000,0.000,0.000,NA,0.000,1,0,08848161,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,6,0,0a670984,0.000,-301.833,36.924,1734607.250,0.000,NA,0.000,1,0,0a6709a4,0.000,83.304,43.782,558002.188,0.000,NA,0.000,0,0,026701c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,3,0,0a6701e1,0.000,419.842,0.000,0.000,0.000,NA,0.000,0,0,02670200,0.000,0.000,0.000,0.000,0.000,NA,0.000*c8963f70\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 73;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_VALIDMODELS)
{
   unsigned char aucLog[] = "#VALIDMODELSA,COM1,0,50.0,FINESTEERING,2167,254543.994,02000000,342f,16248;1,\"FFNBYNTMNP1\",0,0,0*f895bb72\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 75;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_LOG_ROUNDTRIP_VERSION)
{
   unsigned char aucLog[] = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 71;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

// -------------------------------------------------------------------------------------------------------
// SHORT_ASCII Log Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, SHORT_ASCII_LOG_ROUNDTRIP_RAWIMU)
{
   unsigned char aucLog[] = "%RAWIMUSXA,0,5.998;04,41,0,5.998473,0ba4fe00,-327350056,-10403806,-14067095,-33331,111741,345139*db627314\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 19;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

// -------------------------------------------------------------------------------------------------------
// ABBREV_ASCII Log Decode/Encode Unit Tests
// TODO enable these tests once some issues with abbreviated ascii is ironed out.
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, ABBREV_ASCII_LOG_ROUNDTRIP_BESTPOS)
{
   unsigned char aucLog[] = "<BESTPOS COM1 0 80.0 FINESTEERING 2217 164041.000 02000000 cdba 32768\r\n<     SOL_COMPUTED SINGLE 51.15043628556 -114.03068602900 1099.2120 -17.0000 WGS84 1.4033 1.0278 3.0744 \"\" 0.000 0.000 18 17 17 17 00 06 00 33\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 71;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ABBREV_ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ABBREV_ASCII_LOG_ROUNDTRIP_CHANCONFIGLIST)
{
   // Note that this does not match receiver output, but was chosen as a test case because it contains multiple indentation levels
   unsigned char aucLog[] = "<CHANCONFIGLIST COM1 0 80.5 FINESTEERING 2217 231320.204 02000000 d1c0 32768\r\n<     3 5 \r\n<          8 \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               4 SBASL1 \r\n<               14 GLOL1L2 \r\n<               5 LBAND \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               14 GLOL1L2 \r\n<          8 \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               4 SBASL1 \r\n<               14 GLOL1L2 \r\n<               5 LBAND \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               14 GLOL1L2 \r\n<          8 \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               4 SBASL1 \r\n<               14 GLOL1L2 \r\n<               5 LBAND \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               14 GLOL1L2 \r\n<          8 \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               4 SBASL1 \r\n<               14 GLOL1L2 \r\n<               5 LBAND \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               14 GLOL1L2 \r\n<          8 \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               4 SBASL1 \r\n<               14 GLOL1L2 \r\n<               5 LBAND \r\n<               16 GPSL1L2 \r\n<               4 QZSSL1CAL2C \r\n<               14 GLOL1L2\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 78;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ABBREV_ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ABBREV_ASCII_LOG_ROUNDTRIP_INSPVAS)
{
   unsigned char aucLog[] = "<INSPVAS 2217 246221.000\r\n<     2217 246221.000000000 51.15042226211 -114.03068692022 1078.2299 -0.0031 -0.0044 0.0019 -1.546742762 -0.608363519 -0.000000000 WAITING_AZIMUTH\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 26;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ABBREV_ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ABBREV_ASCII_LOG_MALFORMED_INPUT_RXSTATUS)
{
   unsigned char aucLog[] = "<RXSTATUS ICOM5_29 0 45.5 FINESTEERING 2216 112766.000 03004020 2ae1 16809\r\n<     00000000 5 \r\n<          00000080 00001008 00000000 00000000 \r\n<          03004020 00000000 00030000 00020000 \r\n<          d031c000 00000000 ffffffff 00000000\r\n[ICOM29_5]";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 19;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::MESSAGE_DECODER_ERROR, TestSameFormatCompare(ENCODEFORMAT::ABBREV_ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ABBREV_ASCII_EMPTY_ARRAY) {
   unsigned char aucLog[] = "<RANGE COM1 0 95.5 UNKNOWN 0 170.000 025c0020 5103 16807\r\n<     0 \r\n<         \r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 58;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ABBREV_ASCII, &stExpectedMessageData, nullptr, false));
}


// -------------------------------------------------------------------------------------------------------
// BINARY Log Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, BINARY_LOG_ROUNDTRIP_BESTPOS)
{
   // BESTPOS
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA4, 0xB4, 0xAC, 0x07, 0xD8, 0x16, 0x6D, 0x08, 0x08, 0x40, 0x00, 0x02, 0xF6, 0xB1, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xD7, 0x03, 0xB0, 0x4C, 0xE5, 0x8E, 0x49, 0x40, 0x52, 0xC4, 0x26, 0xD1, 0x72, 0x82, 0x5C, 0xC0, 0x29, 0xCB, 0x10, 0xC7, 0x7A, 0xA2, 0x90, 0x40, 0x33, 0x33, 0x87, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xFA, 0x7E, 0xBA, 0x3F, 0x3F, 0x57, 0x83, 0x3F, 0xA9, 0xA4, 0x0A, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x16, 0x16, 0x16, 0x00, 0x06, 0x39, 0x33, 0x23, 0xC4, 0x89, 0x7A };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

TEST_F(DecodeEncodeTest, BINARY_LOG_ROUNDTRIP_LOGLIST)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x05, 0x00, 0x00, 0x20, 0x24, 0x02, 0x00, 0x00, 0x8B, 0xB4, 0x7A, 0x08, 0x40, 0xE9, 0x72, 0x18, 0x20, 0x08, 0x00, 0x02, 0x0C, 0xC0, 0x00, 0x80, 0x11, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x06, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x05, 0x00, 0x00, 0x01, 0x06, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x05, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x06, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x07, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x0F, 0x00, 0x00, 0x01, 0x06, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x0F, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x10, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x11, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x15, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x26, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x27, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x28, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1A, 0xE4, 0x1B, 0x00 };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

TEST_F(DecodeEncodeTest, BINARY_LOG_ROUNDTRIP_SOURCETABLE)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x40, 0x05, 0x00, 0x20, 0x68, 0x00, 0x15, 0x00, 0x80, 0xB4, 0x74, 0x08, 0x00, 0x5B, 0x88, 0x0D, 0x20, 0x80, 0x00, 0x02, 0xDD, 0x71, 0x00, 0x80, 0x68, 0x65, 0x72, 0x61, 0x2E, 0x6E, 0x6F, 0x76, 0x61, 0x74, 0x65, 0x6C, 0x2E, 0x63, 0x6F, 0x6D, 0x3A, 0x32, 0x31, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x54, 0x52, 0x3B, 0x48, 0x79, 0x64, 0x65, 0x72, 0x61, 0x62, 0x61, 0x64, 0x5F, 0x4C, 0x42, 0x32, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x53, 0x4E, 0x49, 0x50, 0x3B, 0x58, 0x58, 0x58, 0x3B, 0x30, 0x2E, 0x30, 0x30, 0x3B, 0x30, 0x2E, 0x30, 0x30, 0x3B, 0x30, 0x3B, 0x30, 0x3B, 0x73, 0x4E, 0x54, 0x52, 0x49, 0x50, 0x3B, 0x6E, 0x6F, 0x6E, 0x65, 0x3B, 0x4E, 0x3B, 0x4E, 0x3B, 0x30, 0x3B, 0x6E, 0x6F, 0x6E, 0x65, 0x3B, 0x00, 0x00, 0x00, 0xB9, 0x6E, 0x19, 0x2E };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

TEST_F(DecodeEncodeTest, BINARY_LOG_ROUNDTRIP_VERSION)
{
   // VERSION
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x25, 0x00, 0x00, 0xA0, 0xDC, 0x00, 0x00, 0x00, 0xA8, 0x14, 0x00, 0x00, 0x89, 0x58, 0x00, 0x00, 0x20, 0x40, 0x4C, 0x02, 0x81, 0x36, 0x00, 0x80, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x46, 0x4E, 0x52, 0x4E, 0x4E, 0x43, 0x42, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x4D, 0x47, 0x57, 0x31, 0x39, 0x33, 0x39, 0x30, 0x31, 0x36, 0x34, 0x5A, 0x00, 0x00, 0x00, 0x4F, 0x45, 0x4D, 0x37, 0x31, 0x39, 0x2D, 0x31, 0x2E, 0x30, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x4D, 0x37, 0x4D, 0x47, 0x30, 0x38, 0x30, 0x30, 0x44, 0x4E, 0x30, 0x30, 0x30, 0x30, 0x00, 0x4F, 0x4D, 0x37, 0x42, 0x52, 0x30, 0x31, 0x30, 0x30, 0x52, 0x42, 0x47, 0x30, 0x30, 0x30, 0x00, 0x32, 0x30, 0x32, 0x31, 0x2F, 0x41, 0x70, 0x72, 0x2F, 0x32, 0x37, 0x00, 0x32, 0x30, 0x3A, 0x31, 0x33, 0x3A, 0x33, 0x38, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x4D, 0x56, 0x30, 0x37, 0x30, 0x30, 0x30, 0x31, 0x52, 0x4E, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x2B, 0xB8, 0xDB };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

// -------------------------------------------------------------------------------------------------------
// SHORT_BINARY Log Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, SHORT_BINARY_LOG_ROUNDTRIP_RAWIMU)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x00, 0x00, 0x6E, 0x17, 0x00, 0x00, 0x04, 0x29, 0x00, 0x00, 0x8C, 0xC1, 0xC3, 0xB4, 0x6F, 0xFE, 0x17, 0x40, 0x00, 0xFE, 0xA4, 0x0B, 0xD8, 0x08, 0x7D, 0xEC, 0x22, 0x40, 0x61, 0xFF, 0x69, 0x5A, 0x29, 0xFF, 0xCD, 0x7D, 0xFF, 0xFF, 0x7D, 0xB4, 0x01, 0x00, 0x33, 0x44, 0x05, 0x00, 0xDA, 0x20, 0x27, 0xB9 };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_SHORT_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::SHORT_BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

// -------------------------------------------------------------------------------------------------------
// FLATTENED_BINARY Log Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_BESTPOS)
{
   unsigned char aucLog[] = "<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"\
      "<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n";

   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));

   auto* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   ASSERT_EQ(pstLogHeader->ucSync1, OEM4_BINARY_SYNC1);
   ASSERT_EQ(pstLogHeader->ucSync2, OEM4_BINARY_SYNC2);
   ASSERT_EQ(pstLogHeader->ucSync3, OEM4_BINARY_SYNC3);
   ASSERT_EQ(pstLogHeader->ucHeaderLength, OEM4_BINARY_HEADER_LENGTH);
   ASSERT_EQ(pstLogHeader->usMsgNumber, 42);
   ASSERT_EQ(pstLogHeader->ucMsgType, 0);
   ASSERT_EQ(pstLogHeader->ucPort, 0x20);
   ASSERT_EQ(pstLogHeader->usLength, 72);
   ASSERT_EQ(pstLogHeader->usSequenceNumber, 0);
   ASSERT_EQ(pstLogHeader->ucIdleTime, 0x90);
   ASSERT_EQ(pstLogHeader->ucTimeStatus, 180);
   ASSERT_EQ(pstLogHeader->usWeekNo, 2215);
   ASSERT_EQ(pstLogHeader->uiWeekMSec, 148248000U);
   ASSERT_EQ(pstLogHeader->uiStatus, 0x02000020U);
   ASSERT_EQ(pstLogHeader->usMsgDefCRC, 0xcdba);
   ASSERT_EQ(pstLogHeader->usReceiverSWVersion, 32768);

   // SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n"
   auto* pstLogBody = reinterpret_cast<BESTPOS*>(stMessageData.pucMessageBody);
   ASSERT_EQ(pstLogBody->solution_status, 0);
   ASSERT_EQ(pstLogBody->position_type, 16);
   ASSERT_EQ(pstLogBody->latitude, 51.15043711386);
   ASSERT_EQ(pstLogBody->longitude, -114.03067767000);
   ASSERT_EQ(pstLogBody->orthometric_height, 1097.2099);
   ASSERT_EQ(pstLogBody->undulation, -17.0000);
   ASSERT_EQ(pstLogBody->datum_id, 61);
   ASSERT_NEAR(pstLogBody->latitude_std_dev, 0.9038, 0.00001);
   ASSERT_NEAR(pstLogBody->longitude_std_dev, 0.8534, 0.00001);
   ASSERT_NEAR(pstLogBody->height_std_dev, 1.7480, 0.00001);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->base_id), ""));
   ASSERT_EQ(pstLogBody->diff_age, 0.000);
   ASSERT_EQ(pstLogBody->solution_age, 0.000);
   ASSERT_EQ(pstLogBody->num_svs, 35);
   ASSERT_EQ(pstLogBody->num_soln_svs, 30);
   ASSERT_EQ(pstLogBody->num_soln_L1_svs, 30);
   ASSERT_EQ(pstLogBody->num_soln_multi_svs, 30);
   ASSERT_EQ(pstLogBody->extended_solution_status2, 0x00);
   ASSERT_EQ(pstLogBody->ext_sol_stat, 0x06);
   ASSERT_EQ(pstLogBody->gal_and_bds_mask, 0x39);
   ASSERT_EQ(pstLogBody->gps_and_glo_mask, 0x33);
}

TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_GLOEPHEMA)
{
   unsigned char aucLog[] = "#GLOEPHEMERISA,COM1,11,67.0,SATTIME,2168,160218.000,02000820,8d29,32768;51,0,1,80,2168,161118000,10782,573,0,0,95,0,-2.3917966796875000e+07,4.8163881835937500e+06,7.4258510742187500e+06,-1.0062713623046875e+03,1.8321990966796875e+02,-3.3695755004882813e+03,1.86264514923095700e-06,-9.31322574615478510e-07,-0.00000000000000000,-6.69313594698905940e-05,5.587935448e-09,0.00000000000000000,84600,3,2,0,13*ad20fc5f\r\n";

   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));

   OEM4BinaryHeader* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   ASSERT_EQ(pstLogHeader->ucSync1, OEM4_BINARY_SYNC1);
   ASSERT_EQ(pstLogHeader->ucSync2, OEM4_BINARY_SYNC2);
   ASSERT_EQ(pstLogHeader->ucSync3, OEM4_BINARY_SYNC3);
   ASSERT_EQ(pstLogHeader->ucHeaderLength, OEM4_BINARY_HEADER_LENGTH);
   ASSERT_EQ(pstLogHeader->usMsgNumber, 723);
   ASSERT_EQ(pstLogHeader->ucMsgType, 0);
   ASSERT_EQ(pstLogHeader->ucPort, 0x20);
   ASSERT_EQ(pstLogHeader->usLength, 144);
   ASSERT_EQ(pstLogHeader->usSequenceNumber, 11);
   ASSERT_EQ(pstLogHeader->ucIdleTime, 0x86);
   ASSERT_EQ(pstLogHeader->ucTimeStatus, 200);
   ASSERT_EQ(pstLogHeader->usWeekNo, 2168);
   ASSERT_EQ(pstLogHeader->uiWeekMSec, 160218000U);
   ASSERT_EQ(pstLogHeader->uiStatus, 0x02000820U);
   ASSERT_EQ(pstLogHeader->usMsgDefCRC, 0x8d29);
   ASSERT_EQ(pstLogHeader->usReceiverSWVersion, 32768);

   GLOEPHEMERIS* pstLogBody = reinterpret_cast<GLOEPHEMERIS*>(stMessageData.pucMessageBody);
   ASSERT_EQ(pstLogBody->sloto, 51);
   ASSERT_EQ(pstLogBody->freqo, 0);
   ASSERT_EQ(pstLogBody->sat_type, 1);
   ASSERT_EQ(pstLogBody->false_iod, 80);
   ASSERT_EQ(pstLogBody->ephem_week, 2168);
   ASSERT_EQ(pstLogBody->ephem_time, 161118000U);
   ASSERT_EQ(pstLogBody->time_offset, 10782U);
   ASSERT_EQ(pstLogBody->nt, 573);
   ASSERT_EQ(pstLogBody->GLOEPHEMERIS_reserved, 0);
   ASSERT_EQ(pstLogBody->GLOEPHEMERIS_reserved_9, 0);
   ASSERT_EQ(pstLogBody->issue, 95U);
   ASSERT_EQ(pstLogBody->broadcast_health, 0U);
   ASSERT_EQ(pstLogBody->pos_x, -2.3917966796875000e+07);
   ASSERT_EQ(pstLogBody->pos_y, 4.8163881835937500e+06);
   ASSERT_EQ(pstLogBody->pos_z, 7.4258510742187500e+06);
   ASSERT_EQ(pstLogBody->vel_x, -1.0062713623046875e+03);
   ASSERT_EQ(pstLogBody->vel_y, 1.8321990966796875e+02);
   ASSERT_EQ(pstLogBody->vel_z, -3.3695755004882813e+03);
   ASSERT_NEAR(pstLogBody->ls_acc_x, 1.86264514923095700e-06, 0.0000000000000001e-06);
   ASSERT_NEAR(pstLogBody->ls_acc_y, -9.31322574615478510e-07, 0.0000000000000001e-07);
   ASSERT_NEAR(pstLogBody->ls_acc_z, -0.00000000000000000, 0.0000000000000001);
   ASSERT_NEAR(pstLogBody->tau, -6.69313594698905940e-05, 0.0000000000000001e-05);
   ASSERT_EQ(pstLogBody->delta_tau, 5.587935448e-09);
   ASSERT_NEAR(pstLogBody->gamma, 0.00000000000000000, 0.0000000000000001);
   ASSERT_EQ(pstLogBody->tk, 84600U);
   ASSERT_EQ(pstLogBody->p, 3U);
   ASSERT_EQ(pstLogBody->ft, 2U);
   ASSERT_EQ(pstLogBody->age, 0U);
   ASSERT_EQ(pstLogBody->flags, 13U);
}

TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_PORTSTATSB)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x48, 0x00, 0x00, 0x20, 0x9C, 0x03, 0x00, 0x00, 0x87, 0xB4, 0x78, 0x08, 0x80, 0x6E, 0x3F, 0x09, 0x20, 0x08, 0x00, 0x02, 0x72, 0xA8, 0x00, 0x80, 0x17, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4F, 0x00, 0x00, 0x00, 0x0C, 0x77, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD8, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0F, 0x01, 0xFF };
   // Truth values
   uint32_t portstats_port_fields[] = { 1, 2, 3, 13, 14, 15, 23, 24, 25, 26, 27, 28, 29, 30, 38, 39, 40, 41, 42, 43, 46, 47, 48 };

   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));
   ASSERT_EQ(1356U, stMessageData.uiMessageLength);

   OEM4BinaryHeader* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   PORTSTATS* pstLogBody = reinterpret_cast<PORTSTATS*>(stMessageData.pucMessageBody);

   OEM4BinaryHeader* pstTestLogHeader = reinterpret_cast<OEM4BinaryHeader*>(aucLog);
   pstTestLogHeader->usLength = 1356 - (OEM4_BINARY_HEADER_LENGTH + OEM4_BINARY_CRC_LENGTH); // Change the length so header comparison passes
   ASSERT_TRUE(CompareBinaryHeaders(pstTestLogHeader, pstLogHeader));

   // Check the populated parts of the log
   ASSERT_EQ(23U, pstLogBody->port_statistics_arraylength);
   for (uint32_t i = 0; i < pstLogBody->port_statistics_arraylength; i++)
   {
      ASSERT_EQ(portstats_port_fields[i], static_cast<uint32_t>(pstLogBody->port_statistics[i].port));
   }

   // Check the padded, unused parts of the log.
   for (uint32_t i = pstLogBody->port_statistics_arraylength; i < (sizeof(pstLogBody->port_statistics) / sizeof(PORTSTATS_port_statistics)); i++)
   {
      ASSERT_EQ(0,  pstLogBody->port_statistics[i].port);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].rx_chars);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].tx_chars);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].good_rx_chars);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].dropped_chars);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].interrupts);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].breaks);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].parity_errors);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].framing_errors);
      ASSERT_EQ(0U, pstLogBody->port_statistics[i].over_runs);
   }
}

TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_PSRDOPB)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0xAE, 0x00, 0x00, 0x20, 0xA8, 0x00, 0x00, 0x00, 0x8C, 0xB4, 0x78, 0x08, 0x00, 0x43, 0xA4, 0x09, 0x20, 0x08, 0x00, 0x02, 0x79, 0x17, 0x00, 0x80, 0x04, 0x56, 0x9E, 0x3F, 0xF0, 0xA7, 0x46, 0x3F, 0xF8, 0x53, 0xE3, 0x3E, 0xA6, 0x9B, 0x24, 0x3F, 0x7B, 0x14, 0xEE, 0x3E, 0x00, 0x00, 0xA0, 0x40, 0x23, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x3D, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x2F, 0x29, 0x7C, 0xEF };
   // Truth values
   uint32_t psrdop_sat_fields[] = { 10, 29, 13, 15, 16, 18, 27, 5, 26, 23, 194, 54, 52, 38, 44, 59, 45, 53, 61, 30, 12, 25, 4, 11, 9, 19, 2, 36, 21, 19, 22, 43, 42, 44, 34 };

   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));
   ASSERT_EQ(1360U, stMessageData.uiMessageLength);

   auto* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   auto* pstLogBody = reinterpret_cast<PSRDOP*>(stMessageData.pucMessageBody);
   auto* pstTestLogHeader = reinterpret_cast<OEM4BinaryHeader*>(aucLog);
   pstTestLogHeader->usLength = 1360 - (OEM4_BINARY_HEADER_LENGTH + OEM4_BINARY_CRC_LENGTH); // Change the length so header comparison passes
   ASSERT_TRUE(CompareBinaryHeaders(pstTestLogHeader, pstLogHeader));

   // Check the populated parts of the log
   ASSERT_EQ(35U, pstLogBody->sats_arraylength);
   for (uint32_t i = 0; i < pstLogBody->sats_arraylength; i++)
   {
      ASSERT_EQ(psrdop_sat_fields[i], pstLogBody->sats[i]);
   }

   // Check the padded, unused parts of the log.
   for (uint32_t i = pstLogBody->sats_arraylength; i < (sizeof(pstLogBody->sats) / sizeof(uint32_t)); i++)
   {
      ASSERT_EQ(0U, pstLogBody->sats[i]);
   }
}

TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_VALIDMODELSB)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0xCE, 0x00, 0x00, 0x20, 0x1C, 0x00, 0x00, 0x00, 0x89, 0xB4, 0x78, 0x08, 0x95, 0xBD, 0x55, 0x09, 0x20, 0x08, 0x00, 0x02, 0x2F, 0x34, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x46, 0x46, 0x4E, 0x52, 0x4E, 0x4E, 0x43, 0x42, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xE0, 0xA2, 0x68 };

   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));
   ASSERT_EQ(708U, stMessageData.uiMessageLength);

   auto* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   auto* pstLogBody = reinterpret_cast<VALIDMODELS*>(stMessageData.pucMessageBody);
   auto* pstTestLogHeader = reinterpret_cast<OEM4BinaryHeader*>(aucLog);
   pstTestLogHeader->usLength = 708 - (OEM4_BINARY_HEADER_LENGTH + OEM4_BINARY_CRC_LENGTH); // Change the length so header comparison passes
   ASSERT_TRUE(CompareBinaryHeaders(pstTestLogHeader, pstLogHeader));

   // Check the populated parts of the log
   ASSERT_EQ(1U, pstLogBody->models_arraylength);
   for (uint32_t i = 0; i < pstLogBody->models_arraylength; i++)
   {
      ASSERT_EQ(0, memcmp("FFNRNNCBN", pstLogBody->models[i].model, 10));
   }

   // Check the padded, unused parts of the log.
   for (uint32_t i = pstLogBody->models_arraylength; i < (sizeof(pstLogBody->models) / sizeof(VALIDMODELS_models)); i++)
   {
      ASSERT_EQ('\0', pstLogBody->models[i].model[0]);
      ASSERT_EQ(  0U, pstLogBody->models[i].expiry_year);
      ASSERT_EQ(  0U, pstLogBody->models[i].expiry_month);
      ASSERT_EQ(  0U, pstLogBody->models[i].expiry_day);
   }
}

TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_VERSION)
{
   unsigned char aucLog[] = "<VERSION COM1 0 70.5 FINESTEERING 2215 148710.357 02000020 3681 32768\r\n"\
      "<     4\r\n"\
      "<          GPSCARD \"FFNRNNCBN\" \"BMGX15360035V\" \"OEM729-0.00H\" \"OM7MG0810DN0000\" \"OM7BR0000ABG001\" \"2022/Jun/17\" \"08:24:06\"\r\n"\
      "<          OEM7FPGA \"\" \"\" \"\" \"OMV070001RN0000\" \"\" \"\" \"\"\r\n"\
      "<          DB_LUA_SCRIPTS \"SCRIPTS\" \"Block1\" \"\" \"Package1_1.0\" \"\" \"2017/Oct/27\" \"16:02:54\"\r\n"\
      "<          DB_WWWISO \"WWWISO\" \"0\" \"\" \"WMC010202RN0002\" \"\" \"2017/Dec/15\" \"9:08:56\"\r\n";
   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));
   ASSERT_EQ(2196U, stMessageData.uiMessageLength);

   auto* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   ASSERT_EQ(pstLogHeader->ucSync1, OEM4_BINARY_SYNC1);
   ASSERT_EQ(pstLogHeader->ucSync2, OEM4_BINARY_SYNC2);
   ASSERT_EQ(pstLogHeader->ucSync3, OEM4_BINARY_SYNC3);
   ASSERT_EQ(pstLogHeader->ucHeaderLength, OEM4_BINARY_HEADER_LENGTH);
   ASSERT_EQ(pstLogHeader->usMsgNumber, 37);
   ASSERT_EQ(pstLogHeader->ucMsgType, 0);
   ASSERT_EQ(pstLogHeader->ucPort, 0x20);
   ASSERT_EQ(pstLogHeader->usLength, 2164);
   ASSERT_EQ(pstLogHeader->usSequenceNumber, 0);
   ASSERT_EQ(pstLogHeader->ucIdleTime, 0x8D);
   ASSERT_EQ(pstLogHeader->ucTimeStatus, 180);
   ASSERT_EQ(pstLogHeader->usWeekNo, 2215);
   ASSERT_EQ(pstLogHeader->uiWeekMSec, 148710357U);
   ASSERT_EQ(pstLogHeader->uiStatus, 0x02000020U);
   ASSERT_EQ(pstLogHeader->usMsgDefCRC, 0x3681);
   ASSERT_EQ(pstLogHeader->usReceiverSWVersion, 32768);

   auto* pstLogBody = reinterpret_cast<VERSION*>(stMessageData.pucMessageBody);

   // Check the populated parts of the log
   ASSERT_EQ(4U, pstLogBody->versions_arraylength);

   // Check GPSCARD fields
   ASSERT_EQ(pstLogBody->versions[0].component_type, 1);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].model_name), "FFNRNNCBN"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].psn), "BMGX15360035V"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].hardware_version), "OEM729-0.00H"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].software_version), "OM7MG0810DN0000"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].boot_version), "OM7BR0000ABG001"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].compile_date), "2022/Jun/17"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].compile_time), "08:24:06"));

   // Check OEM7FPGA fields
   ASSERT_EQ(pstLogBody->versions[1].component_type, 21);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].model_name), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].psn), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].hardware_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].software_version), "OMV070001RN0000"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].boot_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].compile_date), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].compile_time), ""));

   // Check DB_LUA_SCRIPTS fields
   ASSERT_EQ(pstLogBody->versions[2].component_type, 981073930);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[2].model_name), "SCRIPTS"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[2].psn), "Block1"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[2].hardware_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[2].software_version), "Package1_1.0"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[2].boot_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[2].compile_date), "2017/Oct/27"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[2].compile_time), "16:02:54"));

   // Check DB_WWWISO fields
   ASSERT_EQ(pstLogBody->versions[3].component_type, 981073928);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[3].model_name), "WWWISO"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[3].psn), "0"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[3].hardware_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[3].software_version), "WMC010202RN0002"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[3].boot_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[3].compile_date), "2017/Dec/15"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[3].compile_time), "9:08:56"));
}

TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_VERSIONA)
{
   unsigned char aucLog[] = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";

   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));
   ASSERT_EQ(2196U, stMessageData.uiMessageLength);

   auto* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   ASSERT_EQ(pstLogHeader->ucSync1, OEM4_BINARY_SYNC1);
   ASSERT_EQ(pstLogHeader->ucSync2, OEM4_BINARY_SYNC2);
   ASSERT_EQ(pstLogHeader->ucSync3, OEM4_BINARY_SYNC3);
   ASSERT_EQ(pstLogHeader->ucHeaderLength, OEM4_BINARY_HEADER_LENGTH);
   ASSERT_EQ(pstLogHeader->usMsgNumber, 37);
   ASSERT_EQ(pstLogHeader->ucMsgType, 0);
   ASSERT_EQ(pstLogHeader->ucPort, 0x20);
   ASSERT_EQ(pstLogHeader->usLength, 2164);
   ASSERT_EQ(pstLogHeader->usSequenceNumber, 0);
   ASSERT_EQ(pstLogHeader->ucIdleTime, 0x6F);
   ASSERT_EQ(pstLogHeader->ucTimeStatus, 180);
   ASSERT_EQ(pstLogHeader->usWeekNo, 2167);
   ASSERT_EQ(pstLogHeader->uiWeekMSec, 254938857U);
   ASSERT_EQ(pstLogHeader->uiStatus, 0x02000000U);
   ASSERT_EQ(pstLogHeader->usMsgDefCRC, 0x3681);
   ASSERT_EQ(pstLogHeader->usReceiverSWVersion, 16248);

   auto* pstLogBody = reinterpret_cast<VERSION*>(stMessageData.pucMessageBody);

   // Check the populated parts of the log
   ASSERT_EQ(8U, pstLogBody->versions_arraylength);

   // Check GPSCARD fields
   ASSERT_EQ(pstLogBody->versions[0].component_type, 1);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].model_name), "FFNBYNTMNP1"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].psn), "BMHR15470120X"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].hardware_version), "OEM719N-0.00C"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].software_version), "OM7CR0707RN0000"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].boot_version), "OM7BR0000RBG000"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].compile_date), "2020/Apr/09"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[0].compile_time), "13:40:45"));

   // Check OEM7FPGA fields
   ASSERT_EQ(pstLogBody->versions[1].component_type, 21);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].model_name), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].psn), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].hardware_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].software_version), "OMV070001RN0000"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].boot_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].compile_date), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[1].compile_time), ""));

   // If the last component is correct, we can assume the middle ones are as well.

   // Check RADIO fields
   ASSERT_EQ(pstLogBody->versions[7].component_type, 18);
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[7].model_name), "M3-R4"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[7].psn), "1843000570"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[7].hardware_version), "SPL0020d12"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[7].software_version), "V07.34.2.5.1.11"));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[7].boot_version), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[7].compile_date), ""));
   ASSERT_EQ(0, strcmp(reinterpret_cast<char*>(pstLogBody->versions[7].compile_time), ""));
}

TEST_F(DecodeEncodeTest, FLAT_BINARY_LOG_DECODE_VERSIONB)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x25, 0x00, 0x00, 0x20, 0x20, 0x02, 0x00, 0x00, 0xB9, 0x14, 0x00, 0x00, 0xC0, 0x58, 0x13, 0x15, 0x00, 0x00, 0x44, 0x02, 0x81, 0x36, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4D, 0x44, 0x44, 0x52, 0x59, 0x4E, 0x4C, 0x4D, 0x4E, 0x50, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x4D, 0x58, 0x4A, 0x32, 0x30, 0x32, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x37, 0x4D, 0x00, 0x50, 0x49, 0x4D, 0x32, 0x32, 0x32, 0x2D, 0x30, 0x2E, 0x30, 0x30, 0x4A, 0x00, 0x00, 0x00, 0x00, 0x50, 0x49, 0x31, 0x4D, 0x47, 0x4D, 0x41, 0x49, 0x4E, 0x44, 0x55, 0x30, 0x30, 0x30, 0x30, 0x00, 0x50, 0x49, 0x31, 0x42, 0x52, 0x30, 0x30, 0x30, 0x30, 0x41, 0x42, 0x30, 0x30, 0x30, 0x32, 0x00, 0x32, 0x30, 0x32, 0x31, 0x2F, 0x4D, 0x61, 0x79, 0x2F, 0x32, 0x37, 0x00, 0x30, 0x37, 0x3A, 0x34, 0x35, 0x3A, 0x31, 0x38, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x44, 0x45, 0x56, 0x45, 0x4C, 0x4F, 0x50, 0x45, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x7A, 0x3A, 0x43, 0x4F, 0x4E, 0x46, 0x49, 0x47, 0x5F, 0x46, 0x49, 0x4C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x2E, 0x30, 0x2E, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x30, 0x31, 0x39, 0x2F, 0x44, 0x65, 0x63, 0x2F, 0x31, 0x39, 0x00, 0x30, 0x38, 0x3A, 0x33, 0x33, 0x3A, 0x34, 0x39, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x50, 0x49, 0x31, 0x47, 0x50, 0x30, 0x30, 0x30, 0x31, 0x41, 0x4E, 0x30, 0x33, 0x32, 0x30, 0x00, 0x35, 0x2E, 0x38, 0x2E, 0x31, 0x34, 0x5F, 0x52, 0x43, 0x5F, 0x32, 0x36, 0x38, 0x61, 0x62, 0x00, 0x39, 0x2E, 0x36, 0x2E, 0x30, 0x2E, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x2E, 0x30, 0x2E, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x50, 0x49, 0x31, 0x47, 0x53, 0x30, 0x30, 0x30, 0x31, 0x41, 0x4E, 0x30, 0x33, 0x32, 0x30, 0x00, 0x35, 0x2E, 0x38, 0x2E, 0x31, 0x34, 0x5F, 0x52, 0x43, 0x5F, 0x32, 0x36, 0x38, 0x61, 0x62, 0x00, 0x39, 0x2E, 0x36, 0x2E, 0x30, 0x2E, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x2E, 0x30, 0x2E, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x2D, 0xFA, 0xA2 };
   // Truth values
   uint32_t version_type_fields[] = { 1, 30, 981073931, 9, 10 };

   unsigned char acOutBuf[MAX_BINARY_MESSAGE_LENGTH];
   unsigned char* pucOutBuf = acOutBuf;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog, pucOutBuf, MAX_BINARY_MESSAGE_LENGTH, stMetaData, stMessageData));
   ASSERT_EQ(2196U, stMessageData.uiMessageLength);

   auto* pstLogHeader = reinterpret_cast<OEM4BinaryHeader*>(stMessageData.pucMessageHeader);
   auto* pstLogBody = reinterpret_cast<VERSION*>(stMessageData.pucMessageBody);
   auto* pstTestLogHeader = reinterpret_cast<OEM4BinaryHeader*>(aucLog);
   pstTestLogHeader->usLength = 2196 - (OEM4_BINARY_HEADER_LENGTH + OEM4_BINARY_CRC_LENGTH); // Change the length so header comparison passes
   ASSERT_TRUE(CompareBinaryHeaders(pstTestLogHeader, pstLogHeader));

   // Check the populated parts of the log
   ASSERT_EQ(5U, pstLogBody->versions_arraylength);
   for (uint32_t i = 0; i < pstLogBody->versions_arraylength; i++)
   {
      ASSERT_EQ(version_type_fields[i], static_cast<uint32_t>(pstLogBody->versions[i].component_type));
   }

   // Check the padded, unused parts of the log.
   for (uint32_t i = pstLogBody->versions_arraylength; i < (sizeof(pstLogBody->versions) / sizeof(VERSION_versions)); i++)
   {
      ASSERT_EQ(   0, pstLogBody->versions[i].component_type);
      ASSERT_EQ('\0', pstLogBody->versions[i].model_name[0]);
      ASSERT_EQ('\0', pstLogBody->versions[i].psn[0]);
      ASSERT_EQ('\0', pstLogBody->versions[i].hardware_version[0]);
      ASSERT_EQ('\0', pstLogBody->versions[i].software_version[0]);
      ASSERT_EQ('\0', pstLogBody->versions[i].boot_version[0]);
      ASSERT_EQ('\0', pstLogBody->versions[i].compile_date[0]);
      ASSERT_EQ('\0', pstLogBody->versions[i].compile_time[0]);
   }
}

// -------------------------------------------------------------------------------------------------------
// JSON Log Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, JSON_LOG_ROUNDTRIP_BESTPOS)
{
   unsigned char aucLog[] = R"({"header": {"message": "BESTPOS","id": 42,"port": "COM1","sequence_num": 0,"percent_idle_time": 9.5,"time_status": "FINESTEERING","week": 2176,"seconds": 141484.000,"receiver_status": 33554464,"HEADER_reserved1": 52666,"receiver_sw_version": 32768},"body": {"solution_status": "SOL_COMPUTED","position_type": "SINGLE","latitude": 51.15043470167,"longitude": -114.03068044762,"orthometric_height": 1096.4990,"undulation": -17.0000,"datum_id": "WGS84","latitude_std_dev": 0.7985,"longitude_std_dev": 0.6707,"height_std_dev": 1.5215,"base_id": "","diff_age": 0.000,"solution_age": 0.000,"num_svs": 37,"num_soln_svs": 34,"num_soln_L1_svs": 34,"num_soln_multi_svs": 34,"extended_solution_status2": 0,"ext_sol_stat": 6,"gal_and_bds_mask": 57,"gps_and_glo_mask": 51}})";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog + 11;
   stExpectedMessageData.uiMessageHeaderLength = 237;
   stExpectedMessageData.pucMessageBody = stExpectedMessageData.pucMessageHeader + stExpectedMessageData.uiMessageHeaderLength + 9;
   stExpectedMessageData.uiMessageBodyLength = 502;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, JSON_LOG_ROUNDTRIP_BESTSATS)
{
   unsigned char aucLog[] = R"({"header": {"message": "BESTSATS","id": 1194,"port": "COM1","sequence_num": 0,"percent_idle_time": 50.0,"time_status": "FINESTEERING","week": 2167,"seconds": 244820.000,"receiver_status": 33554432,"HEADER_reserved1": 48645,"receiver_sw_version": 16248},"body": {"satellite_entries": [{"system_type": "GPS","id": "2","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "20","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "29","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "13","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "16","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "18","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "25","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "5","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "26","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "23","status": "GOOD","status_mask": 7},{"system_type": "QZSS","id": "194","status": "SUPPLEMENTARY","status_mask": 7},{"system_type": "SBAS","id": "131","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "133","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "138","status": "NOTUSED","status_mask": 0},{"system_type": "GLONASS","id": "8+6","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "9-2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "1+1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "24+2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "2-4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "17+4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "16-1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "18-3","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GALILEO","id": "26","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "12","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "19","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "31","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "33","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "8","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "7","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "24","status": "GOOD","status_mask": 15},{"system_type": "BEIDOU","id": "35","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "29","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "20","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "22","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "44","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "57","status": "NOEPHEMERIS","status_mask": 0},{"system_type": "BEIDOU","id": "12","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "24","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "19","status": "SUPPLEMENTARY","status_mask": 1}]}})";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog + 11;
   stExpectedMessageData.uiMessageHeaderLength = 241;
   stExpectedMessageData.pucMessageBody = stExpectedMessageData.pucMessageHeader + stExpectedMessageData.uiMessageHeaderLength + 9;
   stExpectedMessageData.uiMessageBodyLength = 3202;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, JSON_LOG_ROUNDTRIP_GPSEPHEM)
{
   unsigned char aucLog[] = R"({"header": {"message": "GPSEPHEM","id": 7,"port": "COM1","sequence_num": 12,"percent_idle_time": 45.5,"time_status": "SATTIME","week": 2098,"seconds": 427560.000,"receiver_status": 33816608,"HEADER_reserved1": 4628,"receiver_sw_version": 15668},"body": {"satellite_id": 3,"tow": 427560.0,"health7": 0,"iode1": 68,"iode2": 68,"wn": 2098,"zwn": 2098,"toe": 432000.0,"a": 2.655942598e+07,"delta_n": 4.844487507e-09,"m0": 5.4111299713e-01,"ecc": 2.6812178548e-03,"omega": 6.9765460014e-01,"cuc": -7.003545761e-07,"cus": 4.092231393e-06,"crc": 3.00875000e+02,"crs": -1.35937500e+01,"cic": 1.490116119e-08,"cis": 3.352761269e-08,"i0": 9.6490477291e-01,"i_dot": -2.146517983e-10,"omega0": -1.042300444e+00,"omega_dot": -8.37642034e-09,"iodc": 68,"toc": 432000.0,"tgd": 1.862645149e-09,"af0": -1.28527e-04,"af1": -1.02318e-11,"af2": 0.00000,"anti_spoofing": true,"n": 1.458664175e-04,"eph_var": 4.00000000e+00}})";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog + 11;
   stExpectedMessageData.uiMessageHeaderLength = 233;
   stExpectedMessageData.pucMessageBody = stExpectedMessageData.pucMessageHeader + stExpectedMessageData.uiMessageHeaderLength + 9;
   stExpectedMessageData.uiMessageBodyLength = 649;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, JSON_LOG_ROUNDTRIP_LOGLIST)
{
   unsigned char aucLog[] = R"({"header": {"message": "LOGLIST","id": 5,"port": "COM1","sequence_num": 0,"percent_idle_time": 63.5,"time_status": "FINESTEERING","week": 2172,"seconds": 164226.000,"receiver_status": 33619968,"HEADER_reserved1": 49164,"receiver_sw_version": 16248},"body": {"log_list": [{"log_port_address": "COM1","message_id": "RXSTATUSEVENTA","trigger": "ONNEW","on_time": 0.000000,"offset": 0.000000,"hold": "HOLD"},{"log_port_address": "COM1","message_id": "INTERFACEMODE","trigger": "ONTIME","on_time": 20.000000,"offset": 0.000000,"hold": "NOHOLD"},{"log_port_address": "COM1","message_id": "LOGLISTA","trigger": "ONCE","on_time": 0.000000,"offset": 0.000000,"hold": "NOHOLD"},{"log_port_address": "COM2","message_id": "RXSTATUSEVENTA","trigger": "ONNEW","on_time": 0.000000,"offset": 0.000000,"hold": "HOLD"},{"log_port_address": "CCOM1","message_id": "INSPVACMPB","trigger": "ONTIME","on_time": 0.050000,"offset": 0.000000,"hold": "HOLD"},{"log_port_address": "CCOM1","message_id": "INSPVASDCMPB","trigger": "ONTIME","on_time": 1.000000,"offset": 0.000000,"hold": "HOLD"}]}})";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog + 11;
   stExpectedMessageData.uiMessageHeaderLength = 237;
   stExpectedMessageData.pucMessageBody = stExpectedMessageData.pucMessageHeader + stExpectedMessageData.uiMessageHeaderLength + 9;
   stExpectedMessageData.uiMessageBodyLength = 809;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, JSON_LOG_ROUNDTRIP_RANGECMP2)
{
   unsigned char aucLog[] = R"({"header": {"message": "RANGECMP2","id": 1273,"port": "COM1","sequence_num": 0,"percent_idle_time": 56.0,"time_status": "FINESTEERING","week": 2171,"seconds": 404649.000,"receiver_status": 33619968,"HEADER_reserved1": 8163,"receiver_sw_version": 16248},"body": {"range_data": [0,2,0,200,186,91,133,154,251,47,225,255,255,107,63,6,81,232,48,129,61,0,228,255,255,67,186,198,10,0,108,128,61,0,1,20,0,52,183,248,132,168,255,47,225,255,255,107,63,164,40,168,60,130,240,255,228,255,255,67,156,68,4,200,203,130,240,255,2,29,0,4,59,253,4,114,3,48,225,255,255,107,63,38,40,8,107,129,18,0,228,255,255,67,156,166,5,40,63,129,18,0,229,255,255,9,93,134,15,80,176,129,18,0,3,6,0,32,219,248,133,78,249,79,225,255,255,107,149,74,81,56,85,128,10,0,228,255,255,67,213,106,121,136,19,128,10,0,229,255,255,9,120,42,136,168,54,128,10,0,231,255,255,3,28,164,168,112,105,128,247,255,4,31,0,24,34,214,133,216,252,63,225,255,255,107,91,72,50,24,162,0,59,0,228,255,255,67,241,40,14,224,84,0,59,0,229,255,255,9,178,104,21,72,151,0,59,0,5,9,0,172,87,239,133,239,254,79,225,255,255,107,148,140,10,112,86,128,247,255,228,255,255,67,212,76,30,168,121,0,247,255,229,255,255,9,91,172,35,152,125,0,247,255,231,255,255,3,31,162,73,240,20,129,22,0,6,18,0,24,19,203,5,158,6,64,225,255,255,107,89,72,15,176,218,128,45,0,228,255,255,67,243,138,7,24,62,129,45,0,229,255,255,9,150,106,18,192,243,0,46,0,231,255,255,3,27,38,105,24,119,130,25,0,7,25,0,72,232,19,133,171,251,79,225,255,255,43,62,102,57,32,136,0,234,255,228,255,255,3,155,70,73,88,100,0,234,255,229,255,255,9,94,230,81,88,57,0,234,255,231,255,255,3,31,130,112,32,172,0,224,255,8,5,0,248,206,18,5,155,4,48,225,255,255,107,63,132,44,88,41,130,12,0,228,255,255,67,156,4,11,80,229,130,12,0,229,255,255,9,93,164,20,120,139,130,12,0,9,26,0,212,198,221,133,20,6,64,225,255,255,107,146,174,11,40,147,0,204,255,228,255,255,67,243,14,53,240,219,128,203,255,229,255,255,9,120,206,56,168,145,0,204,255,231,255,255,3,28,100,58,136,80,129,200,255,11,12,0,232,143,113,5,240,248,63,225,255,255,43,92,70,134,232,5,1,28,0,228,255,255,3,184,38,105,192,62,128,27,0,229,255,255,9,122,134,111,112,160,128,27,0,16,194,112,184,7,78,138,102,0,48,225,255,255,43,120,232,64,8,64,128,237,255,227,255,255,9,120,136,74,240,21,0,237,255,228,255,255,3,25,230,113,8,143,128,244,255,20,133,32,84,97,53,137,1,0,16,225,255,255,99,187,166,10,176,34,0,199,255,21,138,32,140,106,45,137,0,0,16,225,255,255,99,188,8,128,80,63,0,38,0,23,131,32,0,151,44,137,0,0,16,225,255,255,99,187,136,95,32,7,0,0,0,24,13,21,100,9,0,133,31,0,48,225,255,255,41,15,205,15,24,249,0,222,255,228,255,255,67,86,78,78,112,176,1,222,255,227,255,255,73,211,14,76,240,164,1,222,255,25,12,22,140,215,34,5,42,249,63,225,255,255,41,185,166,25,40,51,0,244,255,228,255,255,3,27,6,110,0,191,128,243,255,227,255,255,73,155,38,105,136,179,128,243,255,26,23,26,96,0,82,133,55,6,16,225,255,255,105,215,102,4,16,34,1,20,0,27,21,27,232,163,41,133,67,250,63,225,255,255,105,215,38,8,136,88,0,226,255,228,255,255,3,58,70,53,120,138,0,226,255,227,255,255,73,154,102,62,48,96,0,226,255,28,22,20,104,146,163,4,107,255,63,225,255,255,105,17,205,17,208,51,0,232,255,228,255,255,67,113,76,85,72,47,1,232,255,227,255,255,9,241,44,92,248,81,1,232,255,29,7,28,156,57,66,133,63,7,48,225,255,255,105,214,198,15,112,94,1,232,255,228,255,255,3,57,70,58,152,207,130,232,255,227,255,255,73,154,230,65,104,32,131,232,255,30,14,16,252,100,167,133,217,6,48,225,255,255,41,243,202,14,16,33,128,26,0,228,255,255,67,55,170,127,232,51,129,26,0,227,255,255,9,184,202,118,16,250,128,26,0,31,5,24,140,66,169,133,78,249,63,225,255,255,41,241,234,6,88,80,128,219,255,228,255,255,67,114,234,70,40,15,1,219,255,227,255,255,73,242,12,80,80,65,1,219,255,32,6,19,124,64,0,5,158,0,16,225,255,255,105,14,57,4,8,4,0,197,255,38,26,80,100,65,135,5,251,253,79,225,255,255,41,63,4,6,144,139,128,236,255,226,255,255,3,31,98,100,248,230,1,230,255,227,255,255,3,31,194,46,200,88,1,235,255,228,255,255,3,31,226,42,224,86,129,232,255,39,12,80,236,89,85,134,35,5,64,225,255,255,41,149,10,2,192,75,128,25,0,226,255,255,3,26,198,73,96,53,129,34,0,227,255,255,3,25,36,22,128,121,0,31,0,228,255,255,3,28,166,17,0,134,128,36,0,40,13,80,72,141,133,6,235,250,79,225,255,255,41,152,8,57,96,5,0,201,255,226,255,255,3,26,102,143,216,6,129,210,255,227,255,255,3,25,6,107,128,19,0,188,255,228,255,255,3,28,134,84,64,27,128,193,255,41,31,80,52,184,227,133,165,255,79,225,255,255,41,95,100,12,104,55,0,224,255,226,255,255,3,31,34,91,128,37,129,218,255,227,255,255,3,31,226,29,144,107,0,217,255,228,255,255,3,31,66,33,96,151,128,216,255,43,33,80,248,234,193,5,167,2,64,225,255,255,41,63,100,20,104,239,128,37,0,226,255,255,3,31,130,64,144,92,2,32,0,227,255,255,3,31,130,4,40,136,129,41,0,228,255,255,3,31,98,7,224,168,1,36,0,44,8,80,48,154,2,6,37,0,64,225,255,255,41,121,232,14,184,185,0,49,0,226,255,255,3,27,4,64,24,231,1,50,0,227,255,255,3,28,68,29,208,54,129,35,0,228,255,255,3,30,164,19,224,101,1,40,0,45,1,80,248,219,47,6,138,250,79,225,255,255,41,124,230,60,0,67,0,27,0,226,255,255,3,31,226,153,72,250,128,26,0,227,255,255,3,30,132,88,152,71,128,26,0,228,255,255,3,31,4,94,136,63,0,26,0,46,7,80,220,37,118,134,116,4,64,225,255,255,41,122,104,15,136,31,129,212,255,226,255,255,3,30,4,121,112,241,2,194,255,227,255,255,3,30,162,73,248,94,2,191,255,228,255,255,3,31,36,67,144,254,129,191,255,47,24,80,208,140,130,6,95,4,64,225,255,255,41,152,168,3,72,139,0,228,255,226,255,255,3,27,102,79,152,18,2,204,255,227,255,255,3,27,196,15,32,18,1,205,255,228,255,255,3,29,68,24,96,32,1,204,255,54,45,96,64,73,76,6,15,4,32,225,255,255,105,88,232,15,232,55,0,61,0,244,255,255,3,28,164,172,216,69,130,48,0,55,28,96,152,63,173,133,67,251,47,225,255,255,41,58,134,15,56,143,0,205,255,244,255,255,3,30,226,52,184,198,128,204,255,59,30,96,204,242,168,133,220,4,32,225,255,255,41,59,102,6,128,7,1,239,255,244,255,255,3,30,100,70,64,47,2,237,255,63,58,96,124,163,22,136,81,250,31,225,255,255,41,87,168,88,152,41,0,7,0,65,14,96,112,27,246,5,41,252,47,225,255,255,41,118,232,42,136,1,1,235,255,227,255,255,9,60,228,1,72,228,128,227,255,66,46,96,112,133,236,5,172,250,47,225,255,255,41,58,6,97,64,7,0,38,0,244,255,255,3,30,66,231,176,25,129,36,0,68,33,96,8,210,190,133,246,254,47,225,255,255,41,59,6,15,0,83,129,62,0,244,255,255,3,31,34,191,129,17,134,59,0,69,27,96,72,72,24,133,25,0,32,225,255,255,105,31,2,4,208,106,0,24,0,244,255,255,3,31,98,27,152,110,129,15,0,71,36,96,68,207,228,5,60,255,47,225,255,255,41,59,198,14,160,10,0,23,0,244,255,255,3,29,36,151,72,173,130,25,0,75,41,96,12,7,185,133,158,4,32,225,255,255,41,59,70,14,152,169,1,28,0,244,255,255,3,31,226,222,48,95,133,7,0]}})";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog + 11;
   stExpectedMessageData.uiMessageHeaderLength = 241;
   stExpectedMessageData.pucMessageBody = stExpectedMessageData.pucMessageHeader + stExpectedMessageData.uiMessageHeaderLength + 9;
   stExpectedMessageData.uiMessageBodyLength = 6289;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, JSON_LOG_ROUNDTRIP_VERSION)
{
   unsigned char aucLog[] = R"({"header": {"message": "VERSION","id": 37,"port": "COM1","sequence_num": 0,"percent_idle_time": 19.5,"time_status": "UNKNOWN","week": 0,"seconds": 1.473,"receiver_status": 37748736,"HEADER_reserved1": 13953,"receiver_sw_version": 16502},"body": {"versions": [{"component_type": "GPSCARD","model_name": "FDNRNNTBN","psn": "DMGW15300023D","hardware_version": "OEM719-0.00G","software_version": "OM7MR0801RN0000","boot_version": "OM7BR0000RBG000","compile_date": "2021/Jan/19","compile_time": "15:57:45"},{"component_type": "OEM7FPGA","model_name": "","psn": "","hardware_version": "","software_version": "OMV070001RN0000","boot_version": "","compile_date": "","compile_time": ""}]}})";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog + 11;
   stExpectedMessageData.uiMessageHeaderLength = 225;
   stExpectedMessageData.pucMessageBody = stExpectedMessageData.pucMessageHeader + stExpectedMessageData.uiMessageHeaderLength + 9;
   stExpectedMessageData.uiMessageBodyLength = 434;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedMessageData, nullptr, false));
}

// -------------------------------------------------------------------------------------------------------
// ASCII Response Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, ASCII_RESPONSE)
{
   unsigned char aucLog[] = "#FRESETR,COM1,0,73.0,UNKNOWN,0,0.000,00000000,06e5,0;OK*55c70910\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 53;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_RESPONSE_ERROR)
{
   unsigned char aucLog[] = "#SATEL4CONFIGR,COM1,0,0.0,UNKNOWN,0,0.000,00000000,0000,0;ERROR:Radio must be detected before configuration*3aa62ce2\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 58;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_RESPONSE_ERROR_FORMAT_SPEC_D)
{
   unsigned char aucLog[] = "#SATEL4CONFIGR,COM1,0,44.0,UNKNOWN,0,0.000,00000000,d387,0;ERROR:Parameter 4 is out of range*0059f9b3\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 59;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, ASCII_RESPONSE_ERROR_FORMAT_SPEC_S)
{
   unsigned char aucLog[] = "#SATEL4CONFIGR,COM1,0,44.0,UNKNOWN,0,0.000,00000000,d387,0;ERROR:Invalid combination of parameters (1 & 2)*60b8324b\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 59;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedMessageData, nullptr, false));
}

// -------------------------------------------------------------------------------------------------------
// BINARY Response Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, BINARY_RESPONSE)
{
   // SETNAV Command Response
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0xA2, 0x00, 0x80, 0x20, 0x06, 0x00, 0x00, 0x00, 0xBB, 0xB4, 0xF7, 0x06, 0xB0, 0x8A, 0x1A, 0x1D, 0x00, 0x00, 0x00, 0x00, 0xBB, 0xBB, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4F, 0x4B, 0x9C, 0x8B, 0xEE, 0xA3 };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

TEST_F(DecodeEncodeTest, BINARY_RESPONSE_ERROR)
{
   // SETNAV Command Response
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0xA2, 0x00, 0x80, 0x20, 0x1E, 0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB, 0xBB, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x45, 0x52, 0x52, 0x4F, 0x52, 0x3A, 0x4D, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x20, 0x69, 0x73, 0x20, 0x69, 0x6E, 0x63, 0x6F, 0x72, 0x72, 0x65, 0x63, 0x74, 0x61, 0xCB, 0x52, 0x9C };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

// -------------------------------------------------------------------------------------------------------
// ABBREV_ASCII Response Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, ABBREV_ASCII_RESPONSE)
{
   unsigned char aucLog[] = "<OK\r\n";

   ASSERT_EQ(DecodeEncodeTest::HEADER_DECODER_ERROR, TestDecodeEncode(ENCODEFORMAT::ABBREV_ASCII, aucLog));
}

TEST_F(DecodeEncodeTest, ABBREV_ASCII_RESPONSE_MORE_DATA)
{
   unsigned char aucLog[] = "<OK\r\nGARBAGE";

   ASSERT_EQ(DecodeEncodeTest::HEADER_DECODER_ERROR, TestDecodeEncode(ENCODEFORMAT::ABBREV_ASCII, aucLog));
}

TEST_F(DecodeEncodeTest, ABBREV_ASCII_RESPONSE_ERROR)
{
   unsigned char aucLog[] = "<ERROR:Invalid Message. Field = 1\r\n";

   ASSERT_EQ(DecodeEncodeTest::HEADER_DECODER_ERROR, TestDecodeEncode(ENCODEFORMAT::ABBREV_ASCII, aucLog));
}

// -------------------------------------------------------------------------------------------------------
// ASCII Command Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, ASCII_CMD_ROUNDTRIP_INSTHRESHOLD)
{
   unsigned char aucLog[] = "#INSTHRESHOLDSA,THISPORT,0,0.0,UNKNOWN,0,0.000,00000000,48a5,0;LOW,0.000000000,0.000000000,0.000000000*3989c2ac\r\n";

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestDecodeEncode(ENCODEFORMAT::ASCII, aucLog));
}

// -------------------------------------------------------------------------------------------------------
// Binary Command Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, BINARY_CMD_ROUNDTRIP_INSTHRESHOLD)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x5B, 0x06, 0x00, 0xC0, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x49, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0xFF, 0xF8, 0x3A, 0xA7 };

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestDecodeEncode(ENCODEFORMAT::BINARY, aucLog));
}

// -------------------------------------------------------------------------------------------------------
// Conversion Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_BIN_BESTPOS)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x75, 0xB4, 0x7B, 0x08, 0x10, 0x31, 0xD4, 0x0D, 0x00, 0x00, 0x01, 0x02, 0xF6, 0xB1, 0x78, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0xCA, 0xA8, 0x35, 0x86, 0x41, 0x93, 0x49, 0x40, 0x2E, 0x0B, 0x3F, 0xB1, 0xF6, 0x81, 0x5C, 0xC0, 0x4F, 0x1E, 0x16, 0x6A, 0x8D, 0x24, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xDE, 0x02, 0x19, 0x3F, 0x69, 0x00, 0x0F, 0x3F, 0x09, 0x8A, 0x6F, 0x3F, 0x31, 0x33, 0x31, 0x00, 0x00, 0x00, 0xC0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x29, 0x21, 0x21, 0x1A, 0x00, 0x0B, 0x1F, 0x37, 0xFD, 0x9F, 0x6A, 0xD5 };
   uint8_t aucLogToConvert[] = "#BESTPOSA,COM1,0,58.5,FINESTEERING,2171,232010.000,02010000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043714161,-114.03068190724,1097.1381,-17.0000,WGS84,0.5977,0.5586,0.9357,\"131\",6.000,0.000,41,33,33,26,00,0b,1f,37*f1136906\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::BINARY, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_BIN_TO_ASC_BESTPOS)
{
   unsigned char aucLog[] = "#BESTPOSA,COM1,0,69.5,FINESTEERING,2215,141611.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043571404,-114.03067339431,1096.9530,-17.0000,WGS84,0.8022,0.7996,1.5021,\"\",0.000,0.000,34,32,32,32,00,06,39,33*9ad924c6\r\n";
   uint8_t aucLogToConvert[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x8B, 0xB4, 0xA7, 0x08, 0xF8, 0xCF, 0x70, 0x08, 0x20, 0x00, 0x00, 0x02, 0xBA, 0xCD, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x49, 0xF8, 0x3B, 0x7A, 0x41, 0x93, 0x49, 0x40, 0x50, 0x5B, 0x8A, 0x8D, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x70, 0xED, 0xE2, 0xCF, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xAA, 0x5A, 0x4D, 0x3F, 0xF9, 0xB3, 0x4C, 0x3F, 0xE3, 0x45, 0xC0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x20, 0x20, 0x20, 0x00, 0x06, 0x39, 0x33, 0x74, 0x5F, 0x1B, 0x9A };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 71;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_ABB_BESTPOS)
{
   unsigned char aucLog[] = "<BESTPOS COM1 0 54.0 FINESTEERING 2211 407360.000 02000000 cdba 32768\r\n<     SOL_COMPUTED SINGLE 51.15042853573 -114.03067810577 1095.6360 -17.0000 WGS84 0.9594 1.1258 1.9065 \"\" 0.000 0.000 31 21 21 21 00 06 30 33\r\n";
   uint8_t aucLogToConvert[] = "#BESTPOSA,COM1,0,54.0,FINESTEERING,2211,407360.000,02000000,cdba,32768;SOL_COMPUTED,SINGLE,51.15042853573,-114.03067810577,1095.6360,-17.0000,WGS84,0.9594,1.1258,1.9065,\"\",0.000,0.000,31,21,21,21,00,06,30,33*2a42e34b\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 71;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ABBREV_ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_BIN_TO_ASC_TRACKSTAT)
{
   unsigned char aucLog[] = "#TRACKSTATA,COM1,0,49.0,FINESTEERING,2171,398640.000,02010000,457c,16248;SOL_COMPUTED,WAAS,5.0,235,2,0,1810bc04,20896145.694,-217.249,53.850,10448.934,-0.465,GOOD,0.980,2,0,11303c0b,20896140.914,-169.285,51.590,10442.755,0.000,OBSL2,0.000,0,0,02208000,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02000,0.000,0.000,0.000,0.000,0.000,NA,0.000,20,0,1810bc24,22707310.192,3070.568,46.454,3868.615,0.120,GOOD,0.949,20,0,11303c2b,22707308.600,2392.650,43.722,3861.915,0.000,OBSL2,0.000,0,0,02208020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02020,0.000,0.000,0.000,0.000,0.000,NA,0.000,29,0,1810bc44,23905330.434,3168.755,43.740,2368.533,0.177,GOOD,0.427,29,0,11303c4b,23905329.129,2469.161,43.685,2362.015,0.000,OBSL2,0.000,29,0,02309c4b,23905329.780,2469.161,44.261,2365.375,0.000,OBSL2,0.000,0,0,01c02040,0.000,0.000,0.000,0.000,0.000,NA,0.000,6,0,1810bc64,21527085.528,-2321.828,49.094,13738.735,0.347,GOOD,0.978,6,0,11303c6b,21527086.727,-1809.217,49.051,13733.415,0.000,OBSL2,0.000,6,0,02309c6b,21527087.354,-1809.217,51.680,13735.296,0.000,OBSL2,0.000,6,0,01d03c64,21527088.559,-1733.929,53.706,13736.814,0.000,OBSL5,0.000,31,0,1810bc84,24673727.891,1980.320,41.565,1078.815,-0.418,GOOD,0.405,31,0,11303c8b,24673726.284,1543.107,41.202,1073.516,0.000,OBSL2,0.000,31,0,02309c8b,24673726.867,1543.109,43.518,1075.395,0.000,OBSL2,0.000,0,0,01c02080,0.000,0.000,0.000,0.000,0.000,NA,0.000,19,0,0810bca4,23786785.522,-3358.760,45.789,20368.574,-0.101,GOOD,0.956,19,0,01303cab,23786782.184,-2617.217,41.773,20362.254,0.000,OBSL2,0.000,0,0,022080a0,0.000,-0.004,0.000,0.000,0.000,NA,0.000,0,0,01c020a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,24,0,1810bcc4,25183575.384,-3468.042,42.976,17183.717,-0.029,GOOD,0.870,24,0,11303ccb,25183578.439,-2702.372,42.858,17177.516,0.000,OBSL2,0.000,24,0,02309ccb,25183578.702,-2702.372,47.048,17180.277,0.000,OBSL2,0.000,24,0,01d03cc4,25183579.739,-2589.781,49.028,17181.816,0.000,OBSL5,0.000,25,0,0810bce4,20975024.988,1660.168,50.335,6938.935,0.209,GOOD,0.971,25,0,01303ceb,20975025.604,1293.638,48.657,6932.756,0.000,OBSL2,0.000,25,0,02309ceb,20975026.214,1293.638,50.565,6935.815,0.000,OBSL2,0.000,25,0,01d03ce4,20975027.955,1239.682,54.122,6937.255,0.000,OBSL5,0.000,0,0,0000a100,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02100,0.000,-0.010,0.000,0.000,0.000,NA,0.000,0,0,02208100,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02100,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02120,0.000,-0.004,0.000,0.000,0.000,NA,0.000,0,0,02208120,0.000,-0.005,0.000,0.000,0.000,NA,0.000,0,0,01c02120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02140,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,02208140,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02140,0.000,0.000,0.000,0.000,0.000,NA,0.000,12,0,0810bd64,20240881.497,-528.198,52.945,11628.935,0.015,GOOD,0.980,12,0,01303d6b,20240878.785,-411.583,51.730,11622.916,0.000,OBSL2,0.000,12,0,02309d6b,20240879.335,-411.583,51.580,11625.614,0.000,OBSL2,0.000,0,0,01c02160,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02180,0.000,0.005,0.000,0.000,0.000,NA,0.000,0,0,02208180,0.000,0.005,0.000,0.000,0.000,NA,0.000,0,0,01c02180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022081a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021c0,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,022081c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022081e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,194,0,0815be04,43427077.798,119.358,43.060,4358.514,0.000,NODIFFCORR,0.000,194,0,02359e0b,43427078.197,93.006,43.803,4355.394,0.000,OBSL2,0.000,194,0,01d53e04,43427081.326,89.146,45.901,4356.833,0.000,OBSL5,0.000,0,0,0005a220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258260,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c52260,0.000,0.000,0.000,0.000,0.000,NA,0.000,133,0,58023e84,38625837.440,1.458,47.925,322076.969,0.000,LOCKEDOUT,0.000,138,0,58023ea4,38494663.415,1.903,48.045,1565693.625,0.000,LOCKEDOUT,0.000,144,0,080222c1,0.000,166.000,0.000,0.000,0.000,NA,0.000,131,0,58023ee4,38480032.645,-0.167,47.337,1565698.250,0.000,LOCKEDOUT,0.000,50,5,08018301,0.000,4097.000,0.000,0.000,0.000,NA,0.000,0,0,00a12300,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,00218300,0.000,-0.000,0.000,0.000,0.000,NA,0.000,49,6,08119f24,20180413.902,1424.752,48.590,6337.980,0.000,NODIFFCORR,0.000,49,6,00b13f2b,20180418.677,1108.141,48.456,6333.898,0.000,OBSL2,0.000,49,6,10319f2b,20180418.453,1108.141,49.174,6334.818,0.000,OBSL2,0.000,41,13,18119f44,23433009.567,-2006.451,41.675,6327.127,0.000,NODIFFCORR,0.000,41,13,10b13f4b,23433011.946,-1560.574,40.645,6321.898,0.000,OBSL2,0.000,41,13,00319f4b,23433012.447,-1560.574,41.008,6322.738,0.000,OBSL2,0.000,58,11,18119f64,19414951.522,-723.284,48.739,10947.098,0.000,NODIFFCORR,0.000,58,11,00b13f6b,19414952.430,-562.555,46.876,10941.897,0.000,OBSL2,0.000,58,11,10319f6b,19414953.159,-562.555,46.973,10942.738,0.000,OBSL2,0.000,59,4,18119f84,20729590.409,2535.796,36.096,14.960,0.000,NODIFFCORR,0.000,59,4,10b13f8b,20729594.281,1972.287,35.054,14.960,0.000,OBSL2,0.000,59,4,00319f8b,20729594.515,1972.286,35.695,14.960,0.000,OBSL2,0.000,57,9,18119fa4,22767196.378,-3658.212,33.092,14159.973,0.000,NODIFFCORR,0.000,57,9,10b13fab,22767200.843,-2845.278,41.861,14155.907,0.000,OBSL2,0.000,57,9,00319fab,22767202.552,-2845.278,41.492,14156.826,0.000,OBSL2,0.000,0,0,000183c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a123c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,002183c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,42,8,08119fe4,22393503.679,1361.899,43.938,3595.797,0.000,NODIFFCORR,0.000,42,8,10b13feb,22393507.436,1059.255,43.777,3590.039,0.000,OBSL2,0.000,42,8,10319feb,22393507.759,1059.256,44.162,3590.958,0.000,OBSL2,0.000,43,3,08018001,0.000,4037.000,0.000,0.000,0.000,NA,0.000,0,0,00a12000,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00218000,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00018020,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00218020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00018040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00018060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12060,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00218060,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00018080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12080,0.000,-0.007,0.000,0.000,0.000,NA,0.000,0,0,00218080,0.000,-0.007,0.000,0.000,0.000,NA,0.000,0,0,000180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a120a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,002180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,26,0,08539cc4,22963489.915,766.027,53.922,10914.045,0.000,NODIFFCORR,0.000,26,0,01933cc4,22963494.808,571.961,53.434,10911.826,0.000,OBSE5,0.000,26,0,02333cc4,22963491.028,586.885,53.677,10912.045,0.000,OBSE5,0.000,26,0,02933cc4,22963491.069,579.431,56.348,10911.767,0.000,OBSE5,0.000,21,0,08539ce4,25762621.738,-2783.625,48.777,24054.283,0.000,NODIFFCORR,0.000,21,0,01933ce4,25762627.262,-2078.641,49.628,24052.043,0.000,OBSE5,0.000,21,0,02333ce4,25762622.443,-2132.879,49.099,24052.043,0.000,OBSE5,0.000,21,0,02933ce4,25762623.893,-2105.813,52.280,24051.943,0.000,OBSE5,0.000,13,0,08539d04,24612256.370,-2001.181,49.856,17556.785,0.000,NODIFFCORR,0.000,13,0,01933d04,24612260.363,-1494.437,51.242,17554.365,0.000,OBSE5,0.000,13,0,02333d04,24612257.183,-1533.415,51.594,17554.645,0.000,OBSE5,0.000,13,0,02933d04,24612256.859,-1513.902,54.135,17554.363,0.000,OBSE5,0.000,31,0,08539d24,26050071.272,2290.980,45.702,4770.864,0.000,NODIFFCORR,0.000,31,0,01933d24,26050076.452,1710.845,47.980,4768.823,0.000,OBSE5,0.000,31,0,02333d24,26050074.247,1755.423,49.726,4768.756,0.000,OBSE5,0.000,31,0,02933d24,26050073.589,1733.103,51.757,4768.564,0.000,OBSE5,0.000,3,0,08539d44,28323058.671,588.051,46.089,1063.543,0.000,NODIFFCORR,0.000,3,0,01933d44,28323064.262,439.076,47.811,1061.364,0.000,OBSE5,0.000,3,0,02333d44,28323061.323,450.452,48.562,1061.244,0.000,OBSE5,0.000,3,0,02933d44,28323060.676,444.799,50.954,1061.164,0.000,OBSE5,0.000,33,0,08539d64,26478271.103,2652.091,47.102,3645.384,0.000,NODIFFCORR,0.000,33,0,01933d64,26478275.360,1980.515,48.715,3643.044,0.000,OBSE5,0.000,33,0,02333d64,26478271.929,2032.176,48.750,3643.243,0.000,OBSE5,0.000,33,0,02933d64,26478271.282,2006.363,51.367,3642.943,0.000,OBSE5,0.000,0,0,00438180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,1,0,08539da4,23958766.073,-475.463,54.092,14535.322,0.000,NODIFFCORR,0.000,1,0,01933da4,23958769.774,-355.010,53.337,14533.043,0.000,OBSE5,0.000,1,0,02333da4,23958766.248,-364.278,53.535,14533.242,0.000,OBSE5,0.000,1,0,02933da4,23958766.283,-359.648,56.226,14532.942,0.000,OBSE5,0.000,0,0,004381c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004382a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,17,0,080482c1,0.000,1000.000,0.000,0.000,0.000,NA,0.000,17,0,080482c1,0.000,-4500.000,0.000,0.000,0.000,NA,0.000,28,0,08149ee4,21790652.051,-928.805,52.039,14739.068,0.000,NODIFFCORR,0.000,28,0,41343ee4,21790652.674,-699.991,52.547,14733.771,0.000,NA,0.000,39,0,48149f04,40782140.897,-1404.911,41.132,14657.694,0.000,NODIFFCORR,0.000,39,0,41343f04,40782141.013,-1058.816,45.107,16163.642,0.000,NA,0.000,37,0,58149f24,24136553.939,-2822.525,47.093,17079.148,0.000,NODIFFCORR,0.000,37,0,41343f24,24136559.917,-2127.116,50.676,17073.830,0.000,NA,0.000,16,0,18149f44,40866474.444,-958.876,38.978,12759.060,0.000,NODIFFCORR,0.000,16,0,10349f44,40866476.348,-741.557,39.789,12758.899,0.000,OBSB2,0.000,53,0,08048361,0.000,2000.000,0.000,0.000,0.000,NA,0.000,53,0,08048361,0.000,-14000.000,0.000,0.000,0.000,NA,0.000,43,0,48149f84,25483390.657,-2862.193,45.838,21209.148,0.000,NODIFFCORR,0.000,43,0,41343f84,25483394.575,-2157.030,46.839,21203.830,0.000,NA,0.000,2,0,088483a1,0.000,2500.000,0.000,0.000,0.000,NA,0.000,2,0,088483a1,0.000,-2500.000,0.000,0.000,0.000,NA,0.000,42,0,48149fc4,25419583.583,41.278,44.613,5499.149,0.000,NODIFFCORR,0.000,42,0,41343fc4,25419597.097,31.133,49.977,5493.829,0.000,NA,0.000,58,0,48049fe4,30936374.860,-2124.546,49.039,18146.543,0.000,NODIFFCORR,0.000,58,0,012423e9,0.000,-1600.901,0.000,0.000,0.000,NA,0.000,0,0,00048000,0.000,0.000,0.000,0.000,0.000,NA,0.000,18,0,08048002,0.000,-7620.000,0.000,0.000,0.000,NA,0.000,14,0,08149c24,24592161.459,1301.171,46.351,4309.071,0.000,NODIFFCORR,0.000,14,0,00349c24,24592158.131,1006.133,49.425,4308.911,0.000,OBSB2,0.000,46,0,48149c44,23130882.960,85.620,49.050,8689.149,0.000,NODIFFCORR,0.000,46,0,41343c44,23130889.469,64.558,52.186,8683.649,0.000,NA,0.000,0,0,00048060,0.000,0.000,0.000,0.000,0.000,NA,0.000,57,0,08048061,0.000,0.000,0.000,0.000,0.000,NA,0.000,33,0,48149c84,25393217.875,2491.764,43.814,2789.069,0.000,NODIFFCORR,0.000,33,0,41343c84,25393245.187,1877.809,50.270,2253.250,0.000,NA,0.000,27,0,18149ca4,22545097.717,1922.685,51.025,7399.069,0.000,NODIFFCORR,0.000,27,0,41343ca4,22545099.084,1448.937,50.373,7393.830,0.000,NA,0.000,0,0,000480c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,29,0,080480c1,0.000,-5000.000,0.000,0.000,0.000,NA,0.000,36,0,48149ce4,26118959.231,2578.202,40.927,1399.070,0.000,NODIFFCORR,0.000,36,0,41343ce4,26118969.105,1942.746,45.630,1393.830,0.000,NA,0.000,6,0,08149d04,40352408.747,-813.342,30.526,8905.414,0.000,NODIFFCORR,0.000,6,0,10349d04,40352406.750,-628.646,43.700,11028.902,0.000,OBSB2,0.000,51,0,08048121,0.000,2000.000,0.000,0.000,0.000,NA,0.000,51,0,08048122,0.000,-4714.000,0.000,0.000,0.000,NA,0.000,9,0,08149d44,40898830.833,-55.720,37.707,2297.372,0.000,NODIFFCORR,0.000,9,0,10349d44,40898828.592,-42.940,40.025,5808.901,0.000,OBSB2,0.000,0,0,00048160,0.000,0.000,0.000,0.000,0.000,NA,0.000,49,0,08048161,0.000,-5832.000,0.000,0.000,0.000,NA,0.000,6,0,0a670984,0.000,-313.494,37.057,1565675.125,0.000,NA,0.000,1,0,0a6709a4,0.000,64.105,43.017,669421.625,0.000,NA,0.000,0,0,026701c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,3,0,0a6701e1,0.000,-277.904,0.000,0.000,0.000,NA,0.000,0,0,02670200,0.000,0.000,0.000,0.000,0.000,NA,0.000*bc28a409\r\n";
   uint8_t aucLogToConvert[] = { 0xAA, 0x44, 0x12, 0x1C, 0x53, 0x00, 0x00, 0x20, 0xC8, 0x24, 0x00, 0x00, 0x62, 0xB4, 0x7B, 0x08, 0x80, 0xC3, 0xC2, 0x17, 0x00, 0x00, 0x01, 0x02, 0x7C, 0x45, 0x78, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0xEB, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0xBC, 0x10, 0x18, 0xBE, 0x9F, 0x1A, 0x1B, 0x99, 0xED, 0x73, 0x41, 0xBE, 0x3F, 0x59, 0xC3, 0x66, 0x66, 0x57, 0x42, 0xBC, 0x43, 0x23, 0x46, 0x7B, 0x14, 0xEE, 0xBE, 0x00, 0x00, 0x00, 0x00, 0x48, 0xE1, 0x7A, 0x3F, 0x02, 0x00, 0x00, 0x00, 0x0B, 0x3C, 0x30, 0x11, 0x77, 0xBE, 0x9F, 0xCE, 0x98, 0xED, 0x73, 0x41, 0xF6, 0x48, 0x29, 0xC3, 0x29, 0x5C, 0x4E, 0x42, 0x05, 0x2B, 0x23, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x24, 0xBC, 0x10, 0x18, 0x98, 0x6E, 0x12, 0xE3, 0xC6, 0xA7, 0x75, 0x41, 0x17, 0xE9, 0x3F, 0x45, 0xE5, 0xD0, 0x39, 0x42, 0xD7, 0xC9, 0x71, 0x45, 0x8F, 0xC2, 0xF5, 0x3D, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xF1, 0x72, 0x3F, 0x14, 0x00, 0x00, 0x00, 0x2B, 0x3C, 0x30, 0x11, 0x9A, 0x99, 0x99, 0xC9, 0xC6, 0xA7, 0x75, 0x41, 0x66, 0x8A, 0x15, 0x45, 0x54, 0xE3, 0x2E, 0x42, 0xA4, 0x5E, 0x71, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x44, 0xBC, 0x10, 0x18, 0xFC, 0xA9, 0xF1, 0x26, 0x43, 0xCC, 0x76, 0x41, 0x14, 0x0C, 0x46, 0x45, 0xC3, 0xF5, 0x2E, 0x42, 0x87, 0x08, 0x14, 0x45, 0x7D, 0x3F, 0x35, 0x3E, 0x00, 0x00, 0x00, 0x00, 0xBE, 0x9F, 0xDA, 0x3E, 0x1D, 0x00, 0x00, 0x00, 0x4B, 0x3C, 0x30, 0x11, 0x4E, 0x62, 0x10, 0x12, 0x43, 0xCC, 0x76, 0x41, 0x93, 0x52, 0x1A, 0x45, 0x71, 0xBD, 0x2E, 0x42, 0x3D, 0xA0, 0x13, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x4B, 0x9C, 0x30, 0x02, 0x48, 0xE1, 0x7A, 0x1C, 0x43, 0xCC, 0x76, 0x41, 0x93, 0x52, 0x1A, 0x45, 0x44, 0x0B, 0x31, 0x42, 0x00, 0xD6, 0x13, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x64, 0xBC, 0x10, 0x18, 0x21, 0xB0, 0x72, 0xD8, 0xA2, 0x87, 0x74, 0x41, 0x3F, 0x1D, 0x11, 0xC5, 0x42, 0x60, 0x44, 0x42, 0xF1, 0xAA, 0x56, 0x46, 0xFC, 0xA9, 0xB1, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x35, 0x5E, 0x7A, 0x3F, 0x06, 0x00, 0x00, 0x00, 0x6B, 0x3C, 0x30, 0x11, 0xC1, 0xCA, 0xA1, 0xEB, 0xA2, 0x87, 0x74, 0x41, 0xF2, 0x26, 0xE2, 0xC4, 0x39, 0x34, 0x44, 0x42, 0xA9, 0x95, 0x56, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x6B, 0x9C, 0x30, 0x02, 0xE7, 0xFB, 0xA9, 0xF5, 0xA2, 0x87, 0x74, 0x41, 0xF2, 0x26, 0xE2, 0xC4, 0x52, 0xB8, 0x4E, 0x42, 0x2F, 0x9D, 0x56, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x64, 0x3C, 0xD0, 0x01, 0xFC, 0xA9, 0xF1, 0x08, 0xA3, 0x87, 0x74, 0x41, 0xBA, 0xBD, 0xD8, 0xC4, 0xF2, 0xD2, 0x56, 0x42, 0x42, 0xA3, 0x56, 0x46, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x84, 0xBC, 0x10, 0x18, 0x37, 0x89, 0x41, 0xFE, 0xDB, 0x87, 0x77, 0x41, 0x3D, 0x8A, 0xF7, 0x44, 0x8F, 0x42, 0x26, 0x42, 0x14, 0xDA, 0x86, 0x44, 0x19, 0x04, 0xD6, 0xBE, 0x00, 0x00, 0x00, 0x00, 0x29, 0x5C, 0xCF, 0x3E, 0x1F, 0x00, 0x00, 0x00, 0x8B, 0x3C, 0x30, 0x11, 0x96, 0x43, 0x8B, 0xE4, 0xDB, 0x87, 0x77, 0x41, 0x6D, 0xE3, 0xC0, 0x44, 0xD9, 0xCE, 0x24, 0x42, 0x83, 0x30, 0x86, 0x44, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x8B, 0x9C, 0x30, 0x02, 0x64, 0x3B, 0xDF, 0xED, 0xDB, 0x87, 0x77, 0x41, 0x7D, 0xE3, 0xC0, 0x44, 0x6F, 0x12, 0x2E, 0x42, 0xA4, 0x6C, 0x86, 0x44, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0xA4, 0xBC, 0x10, 0x08, 0xAC, 0x1C, 0x5A, 0x18, 0x52, 0xAF, 0x76, 0x41, 0x29, 0xEC, 0x51, 0xC5, 0xF0, 0x27, 0x37, 0x42, 0x26, 0x21, 0x9F, 0x46, 0x17, 0xD9, 0xCE, 0xBD, 0x00, 0x00, 0x00, 0x00, 0x6A, 0xBC, 0x74, 0x3F, 0x13, 0x00, 0x00, 0x00, 0xAB, 0x3C, 0x30, 0x01, 0xFC, 0xA9, 0xF1, 0xE2, 0x51, 0xAF, 0x76, 0x41, 0x79, 0x93, 0x23, 0xC5, 0x8D, 0x17, 0x27, 0x42, 0x82, 0x14, 0x9F, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x83, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xC4, 0xBC, 0x10, 0x18, 0x2F, 0xDD, 0x24, 0x76, 0x55, 0x04, 0x78, 0x41, 0xAC, 0xC0, 0x58, 0xC5, 0x6D, 0xE7, 0x2B, 0x42, 0x6F, 0x3F, 0x86, 0x46, 0x68, 0x91, 0xED, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x52, 0xB8, 0x5E, 0x3F, 0x18, 0x00, 0x00, 0x00, 0xCB, 0x3C, 0x30, 0x11, 0xDD, 0x24, 0x06, 0xA7, 0x55, 0x04, 0x78, 0x41, 0xF4, 0xE5, 0x28, 0xC5, 0x98, 0x6E, 0x2B, 0x42, 0x08, 0x33, 0x86, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xCB, 0x9C, 0x30, 0x02, 0x5A, 0x64, 0x3B, 0xAB, 0x55, 0x04, 0x78, 0x41, 0xF4, 0xE5, 0x28, 0xC5, 0x27, 0x31, 0x3C, 0x42, 0x8E, 0x38, 0x86, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0xD0, 0x01, 0xAA, 0xF1, 0xD2, 0xBB, 0x55, 0x04, 0x78, 0x41, 0x7F, 0xDC, 0x21, 0xC5, 0xAC, 0x1C, 0x44, 0x42, 0xA2, 0x3B, 0x86, 0x46, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xE4, 0xBC, 0x10, 0x08, 0x17, 0xD9, 0xCE, 0x0F, 0xDB, 0x00, 0x74, 0x41, 0x60, 0x85, 0xCF, 0x44, 0x0A, 0x57, 0x49, 0x42, 0x7B, 0xD7, 0xD8, 0x45, 0x19, 0x04, 0x56, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x75, 0x93, 0x78, 0x3F, 0x19, 0x00, 0x00, 0x00, 0xEB, 0x3C, 0x30, 0x01, 0xE7, 0xFB, 0xA9, 0x19, 0xDB, 0x00, 0x74, 0x41, 0x6A, 0xB4, 0xA1, 0x44, 0xC5, 0xA0, 0x42, 0x42, 0x0C, 0xA6, 0xD8, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xEB, 0x9C, 0x30, 0x02, 0x44, 0x8B, 0x6C, 0x23, 0xDB, 0x00, 0x74, 0x41, 0x6A, 0xB4, 0xA1, 0x44, 0x8F, 0x42, 0x4A, 0x42, 0x85, 0xBE, 0xD8, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0xD0, 0x01, 0x14, 0xAE, 0x47, 0x3F, 0xDB, 0x00, 0x74, 0x41, 0xD3, 0xF5, 0x9A, 0x44, 0xEE, 0x7C, 0x58, 0x42, 0x0A, 0xCA, 0xD8, 0x45, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x83, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0xA3, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x83, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x83, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x64, 0xBD, 0x10, 0x08, 0x46, 0xB6, 0xF3, 0x17, 0x9F, 0x4D, 0x73, 0x41, 0xAC, 0x0C, 0x04, 0xC4, 0xAE, 0xC7, 0x53, 0x42, 0xBD, 0xB3, 0x35, 0x46, 0x8F, 0xC2, 0x75, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x48, 0xE1, 0x7A, 0x3F, 0x0C, 0x00, 0x00, 0x00, 0x6B, 0x3D, 0x30, 0x01, 0x29, 0x5C, 0x8F, 0xEC, 0x9E, 0x4D, 0x73, 0x41, 0xA0, 0xCA, 0xCD, 0xC3, 0x85, 0xEB, 0x4E, 0x42, 0xAA, 0x9B, 0x35, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x6B, 0x9D, 0x30, 0x02, 0xF6, 0x28, 0x5C, 0xF5, 0x9E, 0x4D, 0x73, 0x41, 0xA0, 0xCA, 0xCD, 0xC3, 0xEC, 0x51, 0x4E, 0x42, 0x75, 0xA6, 0x35, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0xA3, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0xA3, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x04, 0xBE, 0x15, 0x08, 0xD3, 0x4D, 0x62, 0x2E, 0x28, 0xB5, 0x84, 0x41, 0x4C, 0xB7, 0xEE, 0x42, 0x71, 0x3D, 0x2C, 0x42, 0x1D, 0x34, 0x88, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x0B, 0x9E, 0x35, 0x02, 0xBC, 0x74, 0x93, 0x31, 0x28, 0xB5, 0x84, 0x41, 0x12, 0x03, 0xBA, 0x42, 0x46, 0x36, 0x2F, 0x42, 0x27, 0x1B, 0x88, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x04, 0x3E, 0xD5, 0x01, 0xE3, 0xA5, 0x9B, 0x4A, 0x28, 0xB5, 0x84, 0x41, 0xC1, 0x4A, 0xB2, 0x42, 0xA0, 0x9A, 0x37, 0x42, 0xAA, 0x26, 0x88, 0x45, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xA2, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x82, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0xC5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xA2, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x82, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0xC5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0xA2, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x82, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0xC5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x84, 0x3E, 0x02, 0x58, 0xB8, 0x1E, 0x85, 0x6B, 0x11, 0x6B, 0x82, 0x41, 0xBE, 0x9F, 0xBA, 0x3F, 0x33, 0xB3, 0x3F, 0x42, 0x9F, 0x43, 0x9D, 0x48, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x00, 0x00, 0x00, 0xA4, 0x3E, 0x02, 0x58, 0x85, 0xEB, 0x51, 0x3B, 0x0E, 0x5B, 0x82, 0x41, 0x81, 0x95, 0xF3, 0x3F, 0x14, 0x2E, 0x40, 0x42, 0xED, 0x1F, 0xBF, 0x49, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0xC1, 0x22, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0xE4, 0x3E, 0x02, 0x58, 0xC3, 0xF5, 0x28, 0x05, 0x45, 0x59, 0x82, 0x41, 0x0C, 0x02, 0x2B, 0xBE, 0x17, 0x59, 0x3D, 0x42, 0x12, 0x20, 0xBF, 0x49, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x05, 0x00, 0x01, 0x83, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x06, 0x00, 0x24, 0x9F, 0x11, 0x08, 0x8D, 0x97, 0x6E, 0xDE, 0xDB, 0x3E, 0x73, 0x41, 0x10, 0x18, 0xB2, 0x44, 0x29, 0x5C, 0x42, 0x42, 0xD7, 0x0F, 0xC6, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x06, 0x00, 0x2B, 0x3F, 0xB1, 0x00, 0xF4, 0xFD, 0xD4, 0x2A, 0xDC, 0x3E, 0x73, 0x41, 0x83, 0x84, 0x8A, 0x44, 0xF2, 0xD2, 0x41, 0x42, 0x2F, 0xEF, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x06, 0x00, 0x2B, 0x9F, 0x31, 0x10, 0xEE, 0x7C, 0x3F, 0x27, 0xDC, 0x3E, 0x73, 0x41, 0x83, 0x84, 0x8A, 0x44, 0x2D, 0xB2, 0x44, 0x42, 0x8B, 0xF6, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x0D, 0x00, 0x44, 0x9F, 0x11, 0x18, 0x98, 0x6E, 0x12, 0x19, 0xF3, 0x58, 0x76, 0x41, 0x6F, 0xCE, 0xFA, 0xC4, 0x33, 0xB3, 0x26, 0x42, 0x04, 0xB9, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x0D, 0x00, 0x4B, 0x3F, 0xB1, 0x10, 0xE5, 0xD0, 0x22, 0x3F, 0xF3, 0x58, 0x76, 0x41, 0x5E, 0x12, 0xC3, 0xC4, 0x7B, 0x94, 0x22, 0x42, 0x2F, 0x8F, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x0D, 0x00, 0x4B, 0x9F, 0x31, 0x00, 0x79, 0xE9, 0x26, 0x47, 0xF3, 0x58, 0x76, 0x41, 0x5E, 0x12, 0xC3, 0xC4, 0x31, 0x08, 0x24, 0x42, 0xE7, 0x95, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0B, 0x00, 0x64, 0x9F, 0x11, 0x18, 0xAC, 0x1C, 0x5A, 0x78, 0xFA, 0x83, 0x72, 0x41, 0x2D, 0xD2, 0x34, 0xC4, 0xBC, 0xF4, 0x42, 0x42, 0x64, 0x0C, 0x2B, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0B, 0x00, 0x6B, 0x3F, 0xB1, 0x00, 0xAE, 0x47, 0xE1, 0x86, 0xFA, 0x83, 0x72, 0x41, 0x85, 0xA3, 0x0C, 0xC4, 0x06, 0x81, 0x3B, 0x42, 0x97, 0xF7, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0B, 0x00, 0x6B, 0x9F, 0x31, 0x10, 0x96, 0x43, 0x8B, 0x92, 0xFA, 0x83, 0x72, 0x41, 0x85, 0xA3, 0x0C, 0xC4, 0x5A, 0xE4, 0x3B, 0x42, 0xF4, 0xFA, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x84, 0x9F, 0x11, 0x18, 0x96, 0x43, 0x8B, 0x66, 0xEF, 0xC4, 0x73, 0x41, 0xBC, 0x7C, 0x1E, 0x45, 0x4E, 0x62, 0x10, 0x42, 0x29, 0x5C, 0x6F, 0x41, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x8B, 0x3F, 0xB1, 0x10, 0xDB, 0xF9, 0x7E, 0xA4, 0xEF, 0xC4, 0x73, 0x41, 0x2F, 0x89, 0xF6, 0x44, 0x4C, 0x37, 0x0C, 0x42, 0x29, 0x5C, 0x6F, 0x41, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x8B, 0x9F, 0x31, 0x00, 0xA4, 0x70, 0x3D, 0xA8, 0xEF, 0xC4, 0x73, 0x41, 0x27, 0x89, 0xF6, 0x44, 0xAE, 0xC7, 0x0E, 0x42, 0x29, 0x5C, 0x6F, 0x41, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x09, 0x00, 0xA4, 0x9F, 0x11, 0x18, 0xBA, 0x49, 0x0C, 0xC6, 0x65, 0xB6, 0x75, 0x41, 0x64, 0xA3, 0x64, 0xC5, 0x35, 0x5E, 0x04, 0x42, 0xE4, 0x3F, 0x5D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x09, 0x00, 0xAB, 0x3F, 0xB1, 0x10, 0x91, 0xED, 0x7C, 0x0D, 0x66, 0xB6, 0x75, 0x41, 0x73, 0xD4, 0x31, 0xC5, 0xAA, 0x71, 0x27, 0x42, 0xA1, 0x2F, 0x5D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x09, 0x00, 0xAB, 0x9F, 0x31, 0x00, 0xF4, 0xFD, 0xD4, 0x28, 0x66, 0xB6, 0x75, 0x41, 0x73, 0xD4, 0x31, 0xC5, 0xCF, 0xF7, 0x25, 0x42, 0x4E, 0x33, 0x5D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x23, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x83, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x08, 0x00, 0xE4, 0x9F, 0x11, 0x08, 0x1B, 0x2F, 0xDD, 0xFA, 0x29, 0x5B, 0x75, 0x41, 0xC5, 0x3C, 0xAA, 0x44, 0x83, 0xC0, 0x2F, 0x42, 0xC1, 0xBC, 0x60, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x08, 0x00, 0xEB, 0x3F, 0xB1, 0x10, 0x23, 0xDB, 0xF9, 0x36, 0x2A, 0x5B, 0x75, 0x41, 0x29, 0x68, 0x84, 0x44, 0xA6, 0x1B, 0x2F, 0x42, 0xA0, 0x60, 0x60, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x08, 0x00, 0xEB, 0x9F, 0x31, 0x10, 0x2F, 0xDD, 0x24, 0x3C, 0x2A, 0x5B, 0x75, 0x41, 0x31, 0x68, 0x84, 0x44, 0xE3, 0xA5, 0x30, 0x42, 0x54, 0x6F, 0x60, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x03, 0x00, 0x01, 0x80, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x7C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x60, 0xE5, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x60, 0xE5, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x9C, 0x53, 0x08, 0x0A, 0xD7, 0xA3, 0x1E, 0x52, 0xE6, 0x75, 0x41, 0xBA, 0x81, 0x3F, 0x44, 0x21, 0xB0, 0x57, 0x42, 0x2E, 0x88, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0x93, 0x01, 0x68, 0x91, 0xED, 0x6C, 0x52, 0xE6, 0x75, 0x41, 0x81, 0xFD, 0x0E, 0x44, 0x6A, 0xBC, 0x55, 0x42, 0x4E, 0x7F, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0x33, 0x02, 0x21, 0xB0, 0x72, 0x30, 0x52, 0xE6, 0x75, 0x41, 0xA4, 0xB8, 0x12, 0x44, 0x3F, 0xB5, 0x56, 0x42, 0x2E, 0x80, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0x93, 0x02, 0xBE, 0x9F, 0x1A, 0x31, 0x52, 0xE6, 0x75, 0x41, 0x96, 0xDB, 0x10, 0x44, 0x5A, 0x64, 0x61, 0x42, 0x11, 0x7F, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x9C, 0x53, 0x08, 0x17, 0xD9, 0xCE, 0xDB, 0xB3, 0x91, 0x78, 0x41, 0x00, 0xFA, 0x2D, 0xC5, 0xA6, 0x1B, 0x43, 0x42, 0x91, 0xEC, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x93, 0x01, 0xE9, 0x26, 0x31, 0x34, 0xB4, 0x91, 0x78, 0x41, 0x42, 0xEA, 0x01, 0xC5, 0x12, 0x83, 0x46, 0x42, 0x16, 0xE8, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x33, 0x02, 0x2B, 0x87, 0x16, 0xE7, 0xB3, 0x91, 0x78, 0x41, 0x10, 0x4E, 0x05, 0xC5, 0x60, 0x65, 0x44, 0x42, 0x16, 0xE8, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x93, 0x02, 0x5E, 0xBA, 0x49, 0xFE, 0xB3, 0x91, 0x78, 0x41, 0x02, 0x9D, 0x03, 0xC5, 0xB8, 0x1E, 0x51, 0x42, 0xE3, 0xE7, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x9D, 0x53, 0x08, 0x1F, 0x85, 0xEB, 0x05, 0xDA, 0x78, 0x77, 0x41, 0xCB, 0x25, 0xFA, 0xC4, 0x8B, 0x6C, 0x47, 0x42, 0x92, 0x29, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x3D, 0x93, 0x01, 0x17, 0xD9, 0xCE, 0x45, 0xDA, 0x78, 0x77, 0x41, 0xFC, 0xCD, 0xBA, 0xC4, 0xCF, 0xF7, 0x4C, 0x42, 0xBB, 0x24, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x3D, 0x33, 0x02, 0x68, 0x91, 0xED, 0x12, 0xDA, 0x78, 0x77, 0x41, 0x48, 0xAD, 0xBF, 0xC4, 0x42, 0x60, 0x4E, 0x42, 0x4A, 0x25, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x3D, 0x93, 0x02, 0xC9, 0x76, 0xBE, 0x0D, 0xDA, 0x78, 0x77, 0x41, 0xDD, 0x3C, 0xBD, 0xC4, 0x3D, 0x8A, 0x58, 0x42, 0xBA, 0x24, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x9D, 0x53, 0x08, 0xAC, 0x1C, 0x5A, 0x74, 0xE1, 0xD7, 0x78, 0x41, 0xAE, 0x2F, 0x0F, 0x45, 0xD9, 0xCE, 0x36, 0x42, 0xE9, 0x16, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x3D, 0x93, 0x01, 0x5A, 0x64, 0x3B, 0xC7, 0xE1, 0xD7, 0x78, 0x41, 0x0A, 0xDB, 0xD5, 0x44, 0x85, 0xEB, 0x3F, 0x42, 0x96, 0x06, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x3D, 0x33, 0x02, 0x46, 0xB6, 0xF3, 0xA3, 0xE1, 0xD7, 0x78, 0x41, 0x89, 0x6D, 0xDB, 0x44, 0x6D, 0xE7, 0x46, 0x42, 0x0C, 0x06, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x3D, 0x93, 0x02, 0x44, 0x8B, 0x6C, 0x99, 0xE1, 0xD7, 0x78, 0x41, 0x4C, 0xA3, 0xD8, 0x44, 0x2B, 0x07, 0x4F, 0x42, 0x83, 0x04, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x9D, 0x53, 0x08, 0x7F, 0x6A, 0xBC, 0x2A, 0xCF, 0x02, 0x7B, 0x41, 0x44, 0x03, 0x13, 0x44, 0x23, 0x5B, 0x38, 0x42, 0x60, 0xF1, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x3D, 0x93, 0x01, 0xE9, 0x26, 0x31, 0x84, 0xCF, 0x02, 0x7B, 0x41, 0xBA, 0x89, 0xDB, 0x43, 0x77, 0x3E, 0x3F, 0x42, 0xA6, 0xAB, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x3D, 0x33, 0x02, 0x0C, 0x02, 0x2B, 0x55, 0xCF, 0x02, 0x7B, 0x41, 0xDB, 0x39, 0xE1, 0x43, 0x7D, 0x3F, 0x42, 0x42, 0xCF, 0xA7, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x3D, 0x93, 0x02, 0x60, 0xE5, 0xD0, 0x4A, 0xCF, 0x02, 0x7B, 0x41, 0x46, 0x66, 0xDE, 0x43, 0xE5, 0xD0, 0x4B, 0x42, 0x3F, 0xA5, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x9D, 0x53, 0x08, 0x54, 0xE3, 0xA5, 0xF1, 0x6B, 0x40, 0x79, 0x41, 0x75, 0xC1, 0x25, 0x45, 0x73, 0x68, 0x3C, 0x42, 0x25, 0xD6, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x3D, 0x93, 0x01, 0x5C, 0x8F, 0xC2, 0x35, 0x6C, 0x40, 0x79, 0x41, 0x7B, 0x90, 0xF7, 0x44, 0x29, 0xDC, 0x42, 0x42, 0xB4, 0xB0, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x3D, 0x33, 0x02, 0x1B, 0x2F, 0xDD, 0xFE, 0x6B, 0x40, 0x79, 0x41, 0xA2, 0x05, 0xFE, 0x44, 0x00, 0x00, 0x43, 0x42, 0xE3, 0xB3, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x3D, 0x93, 0x02, 0x6F, 0x12, 0x83, 0xF4, 0x6B, 0x40, 0x79, 0x41, 0x9E, 0xCB, 0xFA, 0x44, 0xCF, 0x77, 0x4D, 0x42, 0x17, 0xAF, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x9D, 0x53, 0x08, 0x0C, 0x02, 0x2B, 0xE1, 0x4E, 0xD9, 0x76, 0x41, 0x44, 0xBB, 0xED, 0xC3, 0x35, 0x5E, 0x58, 0x42, 0x4A, 0x1D, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x3D, 0x93, 0x01, 0xD3, 0x4D, 0x62, 0x1C, 0x4F, 0xD9, 0x76, 0x41, 0x48, 0x81, 0xB1, 0xC3, 0x17, 0x59, 0x55, 0x42, 0x2C, 0x14, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x3D, 0x33, 0x02, 0xD9, 0xCE, 0xF7, 0xE3, 0x4E, 0xD9, 0x76, 0x41, 0x96, 0x23, 0xB6, 0xC3, 0xD7, 0x23, 0x56, 0x42, 0xF8, 0x14, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x3D, 0x93, 0x02, 0x02, 0x2B, 0x87, 0xE4, 0x4E, 0xD9, 0x76, 0x41, 0xF2, 0xD2, 0xB3, 0xC3, 0x6D, 0xE7, 0x60, 0x42, 0xC5, 0x13, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x81, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x81, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xC1, 0x82, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xC1, 0x82, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x8C, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0xE4, 0x9E, 0x14, 0x08, 0x60, 0xE5, 0xD0, 0xC0, 0xFB, 0xC7, 0x74, 0x41, 0x85, 0x33, 0x68, 0xC4, 0xF0, 0x27, 0x50, 0x42, 0x46, 0x4C, 0x66, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0xE4, 0x3E, 0x34, 0x41, 0x39, 0xB4, 0xC8, 0xCA, 0xFB, 0xC7, 0x74, 0x41, 0x6D, 0xFF, 0x2E, 0xC4, 0x21, 0x30, 0x52, 0x42, 0x16, 0x37, 0x66, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x04, 0x9F, 0x14, 0x48, 0x56, 0x0E, 0x2D, 0xE7, 0x49, 0x72, 0x83, 0x41, 0x27, 0x9D, 0xAF, 0xC4, 0x2B, 0x87, 0x24, 0x42, 0xC7, 0x06, 0x65, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x04, 0x3F, 0x34, 0x41, 0xBE, 0x9F, 0x1A, 0xE8, 0x49, 0x72, 0x83, 0x41, 0x1D, 0x5A, 0x84, 0xC4, 0x91, 0x6D, 0x34, 0x42, 0x91, 0x8E, 0x7C, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x24, 0x9F, 0x14, 0x58, 0xDD, 0x24, 0x06, 0x9F, 0xB6, 0x04, 0x77, 0x41, 0x66, 0x68, 0x30, 0xC5, 0x3B, 0x5F, 0x3C, 0x42, 0x4C, 0x6E, 0x85, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x24, 0x3F, 0x34, 0x41, 0x31, 0x08, 0xAC, 0xFE, 0xB6, 0x04, 0x77, 0x41, 0xDB, 0xF1, 0x04, 0xC5, 0x39, 0xB4, 0x4A, 0x42, 0xA9, 0x63, 0x85, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x44, 0x9F, 0x14, 0x18, 0xDF, 0x4F, 0x8D, 0x53, 0x95, 0x7C, 0x83, 0x41, 0x10, 0xB8, 0x6F, 0xC4, 0x79, 0xE9, 0x1B, 0x42, 0x3D, 0x5C, 0x47, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x44, 0x9F, 0x34, 0x10, 0x39, 0xB4, 0xC8, 0x62, 0x95, 0x7C, 0x83, 0x41, 0xA6, 0x63, 0x39, 0xC4, 0xF0, 0x27, 0x1F, 0x42, 0x99, 0x5B, 0x47, 0x46, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x61, 0x83, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x61, 0x83, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x5A, 0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x84, 0x9F, 0x14, 0x48, 0x6F, 0x12, 0x83, 0xEA, 0x87, 0x4D, 0x78, 0x41, 0x17, 0xE3, 0x32, 0xC5, 0x1D, 0x5A, 0x37, 0x42, 0x4C, 0xB2, 0xA5, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x84, 0x3F, 0x34, 0x41, 0x33, 0x33, 0x33, 0x29, 0x88, 0x4D, 0x78, 0x41, 0x7B, 0xD0, 0x06, 0xC5, 0x23, 0x5B, 0x3B, 0x42, 0xA9, 0xA7, 0xA5, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xA1, 0x83, 0x84, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x1C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xA1, 0x83, 0x84, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x1C, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0xC4, 0x9F, 0x14, 0x48, 0xCF, 0xF7, 0x53, 0xF9, 0xF3, 0x3D, 0x78, 0x41, 0xAC, 0x1C, 0x25, 0x42, 0xB6, 0x73, 0x32, 0x42, 0x31, 0xD9, 0xAB, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0xC4, 0x3F, 0x34, 0x41, 0xDF, 0x4F, 0x8D, 0xD1, 0xF4, 0x3D, 0x78, 0x41, 0x62, 0x10, 0xF9, 0x41, 0x73, 0xE8, 0x47, 0x42, 0xA2, 0xAE, 0xAB, 0x45, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x00, 0xE4, 0x9F, 0x04, 0x48, 0x5C, 0x8F, 0xC2, 0x6D, 0xD3, 0x80, 0x7D, 0x41, 0xBC, 0xC8, 0x04, 0xC5, 0xF0, 0x27, 0x44, 0x42, 0x16, 0xC5, 0x8D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x00, 0xE9, 0x23, 0x24, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD5, 0x1C, 0xC8, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x02, 0x80, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xEE, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x24, 0x9C, 0x14, 0x08, 0x62, 0x10, 0x58, 0x17, 0xF2, 0x73, 0x77, 0x41, 0x79, 0xA5, 0xA2, 0x44, 0x6D, 0x67, 0x39, 0x42, 0x91, 0xA8, 0x86, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x24, 0x9C, 0x34, 0x00, 0x75, 0x93, 0x18, 0xE2, 0xF1, 0x73, 0x77, 0x41, 0x83, 0x88, 0x7B, 0x44, 0x33, 0xB3, 0x45, 0x42, 0x4A, 0xA7, 0x86, 0x45, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x44, 0x9C, 0x14, 0x48, 0xF6, 0x28, 0x5C, 0x2F, 0x30, 0x0F, 0x76, 0x41, 0x71, 0x3D, 0xAB, 0x42, 0x33, 0x33, 0x44, 0x42, 0x99, 0xC4, 0x07, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x44, 0x3C, 0x34, 0x41, 0x25, 0x06, 0x81, 0x97, 0x30, 0x0F, 0x76, 0x41, 0xB2, 0x1D, 0x81, 0x42, 0x77, 0xBE, 0x50, 0x42, 0x99, 0xAE, 0x07, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x61, 0x80, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x84, 0x9C, 0x14, 0x48, 0x00, 0x00, 0x00, 0x1E, 0x84, 0x37, 0x78, 0x41, 0x39, 0xBC, 0x1B, 0x45, 0x89, 0x41, 0x2F, 0x42, 0x1B, 0x51, 0x2E, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x84, 0x3C, 0x34, 0x41, 0xB6, 0xF3, 0xFD, 0xD2, 0x85, 0x37, 0x78, 0x41, 0xE3, 0xB9, 0xEA, 0x44, 0x7B, 0x14, 0x49, 0x42, 0x00, 0xD4, 0x0C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0xA4, 0x9C, 0x14, 0x18, 0xFE, 0xD4, 0x78, 0x9B, 0x2C, 0x80, 0x75, 0x41, 0xEC, 0x55, 0xF0, 0x44, 0x9A, 0x19, 0x4C, 0x42, 0x8D, 0x38, 0xE7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0xA4, 0x3C, 0x34, 0x41, 0x62, 0x10, 0x58, 0xB1, 0x2C, 0x80, 0x75, 0x41, 0xFC, 0x1D, 0xB5, 0x44, 0xF4, 0x7D, 0x49, 0x42, 0xA4, 0x0E, 0xE7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0xC1, 0x80, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x9C, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0xE4, 0x9C, 0x14, 0x48, 0x0E, 0x2D, 0xB2, 0xF3, 0xB2, 0xE8, 0x78, 0x41, 0x3B, 0x23, 0x21, 0x45, 0x3F, 0xB5, 0x23, 0x42, 0x3D, 0xE2, 0xAE, 0x44, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x34, 0x41, 0x7B, 0x14, 0xAE, 0x91, 0xB3, 0xE8, 0x78, 0x41, 0xDF, 0xD7, 0xF2, 0x44, 0x1F, 0x85, 0x36, 0x42, 0x8F, 0x3A, 0xAE, 0x44, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x9D, 0x14, 0x08, 0x23, 0xDB, 0xF9, 0xC5, 0xD4, 0x3D, 0x83, 0x41, 0xE3, 0x55, 0x4B, 0xC4, 0x3F, 0x35, 0xF4, 0x41, 0xA8, 0x25, 0x0B, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x9D, 0x34, 0x10, 0x00, 0x00, 0x00, 0xB6, 0xD4, 0x3D, 0x83, 0x41, 0x58, 0x29, 0x1D, 0xC4, 0xCD, 0xCC, 0x2E, 0x42, 0x9C, 0x53, 0x2C, 0x46, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x21, 0x81, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x22, 0x81, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x93, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x44, 0x9D, 0x14, 0x08, 0xE7, 0xFB, 0xA9, 0x76, 0x88, 0x80, 0x83, 0x41, 0x48, 0xE1, 0x5E, 0xC2, 0xF8, 0xD3, 0x16, 0x42, 0xF4, 0x95, 0x0F, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x44, 0x9D, 0x34, 0x10, 0x7F, 0x6A, 0xBC, 0x64, 0x88, 0x80, 0x83, 0x41, 0x8F, 0xC2, 0x2B, 0xC2, 0x9A, 0x19, 0x20, 0x42, 0x35, 0x87, 0xB5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x81, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x61, 0x81, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xB6, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x84, 0x09, 0x67, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0xBF, 0x9C, 0xC3, 0x5E, 0x3A, 0x14, 0x42, 0x59, 0x1F, 0xBF, 0x49, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x09, 0x67, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0x35, 0x80, 0x42, 0x68, 0x11, 0x2C, 0x42, 0xDA, 0x6E, 0x23, 0x49, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01, 0x67, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xE1, 0x01, 0x67, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB6, 0xF3, 0x8A, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x67, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0xB3, 0x00, 0x57 };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 73;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_ABB_TRACKSTAT)
{
   unsigned char aucLog[] = "<TRACKSTAT COM1 0 52.5 FINESTEERING 2211 417770.000 02000000 457c 32768\r\n<     SOL_COMPUTED SINGLE 5.0 149 \r\n<          5 0 1810bc04 23701425.492 3789.992 46.788 2078.974 1.085 GOOD 0.244 \r\n<          5 0 11305c0b 23701430.037 2953.246 45.620 2072.914 0.000 OBSL2 0.000 \r\n<          29 0 0810bc24 23082310.768 2981.262 47.273 3828.954 -0.032 GOOD 0.247 \r\n<          29 0 01305c2b 23082313.700 2323.065 44.880 3823.754 0.000 OBSL2 0.000 \r\n<          19 0 0810bc44 24714583.651 -3486.828 44.074 21948.975 0.266 GOOD 0.242 \r\n<          19 0 01305c4b 24714585.894 -2717.014 41.115 21943.754 0.000 OBSL2 0.000 \r\n<          6 0 1810bc64 22530869.360 -3009.560 46.137 15678.695 -0.700 GOOD 0.248 \r\n<          6 0 11305c6b 22530873.571 -2345.113 45.456 15671.915 0.000 OBSL2 0.000 \r\n<          31 0 1810bc84 24133460.335 940.801 45.484 2898.935 -0.039 GOOD 0.245 \r\n<          31 0 11305c8b 24133463.193 733.093 45.643 2893.415 0.000 OBSL2 0.000 \r\n<          2 0 1810bca4 20963526.269 -712.720 52.205 11538.955 -0.451 GOOD 0.249 \r\n<          2 0 11305cab 20963524.462 -555.367 51.430 11533.754 0.000 OBSL2 0.000 \r\n<          20 0 1810bcc4 21742080.142 2309.293 50.589 5698.986 -0.339 GOOD 0.248 \r\n<          20 0 11305ccb 21742080.656 1799.452 46.530 5692.415 0.000 OBSL2 0.000 \r\n<          25 0 0810bce4 20665522.673 870.692 50.806 8086.716 -1.284 GOOD 0.248 \r\n<          25 0 01305ceb 20665526.038 678.462 48.855 8080.517 0.000 OBSL2 0.000 \r\n<          11 0 1810bd04 20362089.745 -601.177 51.571 11278.894 1.094 GOOD 0.249 \r\n<          11 0 11305d0b 20362090.656 -468.450 51.328 11272.415 0.000 OBSL2 0.000 \r\n<          0 0 0000a120 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a04120 0.000 -0.001 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0000a140 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a04140 0.000 -0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0000a160 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a04160 0.000 -0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0000a180 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a04180 0.000 -0.005 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0000a1a0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a041a0 0.000 -0.009 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0000a1c0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a041c0 0.000 -0.008 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0000a1e0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a041e0 0.000 -0.007 0.000 0.000 0.000 NA 0.000 \r\n<          194 0 1815be04 43595011.956 105.359 40.180 2967.754 1.259 GOOD 0.083 \r\n<          194 0 02359e0b 43595018.364 82.101 45.378 2964.394 0.000 OBSL2 0.000 \r\n<          0 0 0005a220 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 02258220 0.000 -0.001 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0005a240 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 02258240 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 0005a260 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 02258260 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00022280 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 000222a0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 000222c0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 000222e0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          59 4 08119f04 19784649.048 -1697.276 40.617 11270.271 -0.406 GOOD 0.013 \r\n<          59 4 00b13f0b 19784654.114 -1320.105 37.078 11267.843 0.000 OBSL2 0.000 \r\n<          60 10 18019f24 20676119.555 2426.357 47.661 5537.208 0.000 NOIONOCORR 0.000 \r\n<          60 10 00a12329 0.000 1887.167 0.000 0.000 0.000 NA 0.000 \r\n<          58 11 08119f44 23007031.853 -3585.721 42.540 17527.469 -1.027 GOOD 0.024 \r\n<          58 11 00b13f4b 23007037.627 -2788.897 43.873 17525.518 0.000 OBSL2 0.000 \r\n<          43 3 08019f64 22109153.041 653.979 34.488 2875.611 0.000 NOIONOCORR 0.000 \r\n<          43 3 00a1236a 0.000 508.650 0.000 0.000 0.000 NA 0.000 \r\n<          49 6 08119f84 21145952.567 -3665.791 49.608 13987.188 0.206 GOOD 0.033 \r\n<          49 6 10b13f8b 21145960.005 -2851.174 46.300 13984.918 0.000 OBSL2 0.000 \r\n<          51 0 18119fa4 22949361.716 3724.046 48.048 1598.788 5.125 GOOD 0.031 \r\n<          51 0 00b13fab 22949370.526 2896.484 45.398 1596.718 0.000 OBSL2 0.000 \r\n<          50 5 18119fc4 19987988.748 -30.141 38.296 6750.071 0.488 GOOD 0.020 \r\n<          50 5 10b13fcb 19987994.167 -23.443 43.197 6747.661 0.000 OBSL2 0.000 \r\n<          42 8 18119fe4 24110164.746 -2951.318 46.765 7697.488 -2.593 GOOD 0.024 \r\n<          42 8 10b13feb 24110172.476 -2295.476 41.549 7695.458 0.000 OBSL2 0.000 \r\n<          0 0 00018000 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a12000 0.000 -0.015 0.000 0.000 0.000 NA 0.000 \r\n<          44 12 18119c24 23028956.405 3594.785 45.326 1587.448 -2.517 GOOD 0.033 \r\n<          44 12 10b13c2b 23028963.538 2795.949 40.845 1585.059 0.000 OBSL2 0.000 \r\n<          0 0 00018040 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a12040 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00018060 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a12060 0.000 0.001 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00018080 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a12080 0.000 -0.001 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 000180a0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00a120a0 0.000 -0.003 0.000 0.000 0.000 NA 0.000 \r\n<          16 0 08149cc4 40527641.107 -891.176 42.389 12619.062 0.015 GOOD 0.165 \r\n<          16 0 00349cc4 40527646.740 -688.872 40.549 12618.900 0.000 OBSB2 0.000 \r\n<          16 0 016420c9 0.000 -689.114 0.000 0.000 0.000 NA 0.000 \r\n<          46 0 08149ce4 26203507.728 2528.383 40.344 998.850 0.000 NOIONOCORR 0.000 \r\n<          46 0 002480e9 0.000 1955.106 0.000 0.000 0.000 NA 0.000 \r\n<          46 0 51743ce4 26203516.234 1954.975 45.261 998.249 0.000 OBSB2 0.000 \r\n<          51 0 08048101 0.000 834.000 0.000 0.000 0.000 NA 0.000 \r\n<          51 0 08048101 0.000 -4166.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 01642100 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          9 0 08149d24 41114161.025 -63.950 39.061 3429.762 0.063 GOOD 0.164 \r\n<          9 0 00349d24 41114162.707 -49.152 40.579 6268.900 0.000 OBSB2 0.000 \r\n<          9 0 01642129 0.000 -49.450 0.000 0.000 0.000 NA 0.000 \r\n<          18 0 08048142 0.000 2191.000 0.000 0.000 0.000 NA 0.000 \r\n<          18 0 08048141 0.000 -3000.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 01642140 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 00048160 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          38 0 08048162 0.000 -4864.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 01642160 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          43 0 08149d84 21976230.668 -767.926 53.000 14708.749 0.000 NOIONOCORR 0.000 \r\n<          43 0 00248189 0.000 -593.809 0.000 0.000 0.000 NA 0.000 \r\n<          43 0 41743d84 21976231.759 -593.806 51.238 13013.732 0.000 OBSB2 0.000 \r\n<          34 0 18149da4 24778973.970 -2726.753 45.887 20888.650 0.000 NOIONOCORR 0.000 \r\n<          34 0 002481a9 0.000 -2108.499 0.000 0.000 0.000 NA 0.000 \r\n<          34 0 51743da4 24778976.771 -2108.527 42.384 20888.250 0.000 OBSB2 0.000 \r\n<          0 0 000481c0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          10 0 080481c1 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 016421c0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          1 0 088481e1 0.000 834.000 0.000 0.000 0.000 NA 0.000 \r\n<          1 0 088481e1 0.000 -4166.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 016421e0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          37 0 18149e04 23644809.748 -209.827 51.046 8888.749 0.000 NOIONOCORR 0.000 \r\n<          37 0 00248209 0.000 -162.252 0.000 0.000 0.000 NA 0.000 \r\n<          37 0 51743e04 23644814.609 -162.173 47.800 8888.589 0.000 OBSB2 0.000 \r\n<          21 0 08149e24 24936225.322 -66.255 47.182 6239.049 0.000 NOIONOCORR 0.000 \r\n<          21 0 00248229 0.000 -51.232 0.000 0.000 0.000 NA 0.000 \r\n<          21 0 41743e24 24936224.946 -51.236 44.955 6237.648 0.000 OBSB2 0.000 \r\n<          55 0 08048241 0.000 11000.000 0.000 0.000 0.000 NA 0.000 \r\n<          55 0 08048241 0.000 -5000.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 01642240 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          23 0 08149e64 24335820.858 -2828.899 49.305 16609.148 0.000 NOIONOCORR 0.000 \r\n<          23 0 00248269 0.000 -2187.484 0.000 0.000 0.000 NA 0.000 \r\n<          23 0 41743e64 24335817.841 -2187.432 47.603 16608.648 0.000 OBSB2 0.000 \r\n<          6 0 08149e84 40542914.308 -698.666 37.528 8544.843 -0.444 GOOD 0.164 \r\n<          6 0 10349e84 40542914.850 -540.299 43.901 10378.661 0.000 OBSB2 0.000 \r\n<          6 0 01642289 0.000 -540.253 0.000 0.000 0.000 NA 0.000 \r\n<          27 0 080482a1 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          27 0 080482a2 0.000 -6363.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 016422a0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          28 0 08149ec4 23032309.590 2093.708 50.562 6769.050 0.000 NOIONOCORR 0.000 \r\n<          28 0 002482c9 0.000 1618.988 0.000 0.000 0.000 NA 0.000 \r\n<          28 0 41743ec4 23032311.068 1619.068 48.490 5230.733 0.000 OBSB2 0.000 \r\n<          11 0 18149ee4 22903570.657 -2050.778 48.744 18098.982 1.808 GOOD 0.166 \r\n<          11 0 10349ee4 22903569.907 -1585.855 52.205 18098.830 0.000 OBSB2 0.000 \r\n<          11 0 016422e9 0.000 -1585.792 0.000 0.000 0.000 NA 0.000 \r\n<          48 0 08048302 0.000 1990.000 0.000 0.000 0.000 NA 0.000 \r\n<          48 0 08048301 0.000 -2000.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 01642300 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          39 0 18149f24 40712703.926 -1342.518 42.649 15938.900 0.000 NOIONOCORR 0.000 \r\n<          39 0 00248329 0.000 -1038.120 0.000 0.000 0.000 NA 0.000 \r\n<          39 0 41743f24 40712706.354 -1038.160 42.958 13271.623 0.000 OBSB2 0.000 \r\n<          26 0 08048341 0.000 9000.000 0.000 0.000 0.000 NA 0.000 \r\n<          26 0 08048341 0.000 -7000.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 01642340 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          42 0 08149f64 25060321.026 2422.533 46.916 3378.669 0.000 NOIONOCORR 0.000 \r\n<          42 0 00248369 0.000 1873.256 0.000 0.000 0.000 NA 0.000 \r\n<          42 0 51743f64 25060334.513 1873.301 46.947 3378.249 0.000 OBSB2 0.000 \r\n<          14 0 08149f84 25485562.721 3126.919 44.053 1548.970 -1.473 GOOD 0.163 \r\n<          14 0 00349f84 25485565.097 2418.014 47.552 1548.810 0.000 OBSB2 0.000 \r\n<          14 0 01642389 0.000 2417.932 0.000 0.000 0.000 NA 0.000 \r\n<          58 0 08049fa4 29895620.607 806.953 51.603 11229.342 0.000 BADHEALTH 0.000 \r\n<          58 0 002483a9 0.000 623.987 0.000 0.000 0.000 NA 0.000 \r\n<          58 0 016423a9 0.000 623.987 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 826703c0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 826703e0 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 82670000 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 82670020 0.000 0.000 0.000 0.000 0.000 NA 0.000 \r\n<          0 0 82670040 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n";
   uint8_t aucLogToConvert[] = "#TRACKSTATA,COM1,0,52.5,FINESTEERING,2211,417770.000,02000000,457c,32768;SOL_COMPUTED,SINGLE,5.0,149,5,0,1810bc04,23701425.492,3789.992,46.788,2078.974,1.085,GOOD,0.244,5,0,11305c0b,23701430.037,2953.246,45.620,2072.914,0.000,OBSL2,0.000,29,0,0810bc24,23082310.768,2981.262,47.273,3828.954,-0.032,GOOD,0.247,29,0,01305c2b,23082313.700,2323.065,44.880,3823.754,0.000,OBSL2,0.000,19,0,0810bc44,24714583.651,-3486.828,44.074,21948.975,0.266,GOOD,0.242,19,0,01305c4b,24714585.894,-2717.014,41.115,21943.754,0.000,OBSL2,0.000,6,0,1810bc64,22530869.360,-3009.560,46.137,15678.695,-0.700,GOOD,0.248,6,0,11305c6b,22530873.571,-2345.113,45.456,15671.915,0.000,OBSL2,0.000,31,0,1810bc84,24133460.335,940.801,45.484,2898.935,-0.039,GOOD,0.245,31,0,11305c8b,24133463.193,733.093,45.643,2893.415,0.000,OBSL2,0.000,2,0,1810bca4,20963526.269,-712.720,52.205,11538.955,-0.451,GOOD,0.249,2,0,11305cab,20963524.462,-555.367,51.430,11533.754,0.000,OBSL2,0.000,20,0,1810bcc4,21742080.142,2309.293,50.589,5698.986,-0.339,GOOD,0.248,20,0,11305ccb,21742080.656,1799.452,46.530,5692.415,0.000,OBSL2,0.000,25,0,0810bce4,20665522.673,870.692,50.806,8086.716,-1.284,GOOD,0.248,25,0,01305ceb,20665526.038,678.462,48.855,8080.517,0.000,OBSL2,0.000,11,0,1810bd04,20362089.745,-601.177,51.571,11278.894,1.094,GOOD,0.249,11,0,11305d0b,20362090.656,-468.450,51.328,11272.415,0.000,OBSL2,0.000,0,0,0000a120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04120,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,0000a140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04140,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a160,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04160,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04180,0.000,-0.005,0.000,0.000,0.000,NA,0.000,0,0,0000a1a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a041a0,0.000,-0.009,0.000,0.000,0.000,NA,0.000,0,0,0000a1c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a041c0,0.000,-0.008,0.000,0.000,0.000,NA,0.000,0,0,0000a1e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a041e0,0.000,-0.007,0.000,0.000,0.000,NA,0.000,194,0,1815be04,43595011.956,105.359,40.180,2967.754,1.259,GOOD,0.083,194,0,02359e0b,43595018.364,82.101,45.378,2964.394,0.000,OBSL2,0.000,0,0,0005a220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258220,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,0005a240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00022280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000222a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000222c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000222e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,59,4,08119f04,19784649.048,-1697.276,40.617,11270.271,-0.406,GOOD,0.013,59,4,00b13f0b,19784654.114,-1320.105,37.078,11267.843,0.000,OBSL2,0.000,60,10,18019f24,20676119.555,2426.357,47.661,5537.208,0.000,NOIONOCORR,0.000,60,10,00a12329,0.000,1887.167,0.000,0.000,0.000,NA,0.000,58,11,08119f44,23007031.853,-3585.721,42.540,17527.469,-1.027,GOOD,0.024,58,11,00b13f4b,23007037.627,-2788.897,43.873,17525.518,0.000,OBSL2,0.000,43,3,08019f64,22109153.041,653.979,34.488,2875.611,0.000,NOIONOCORR,0.000,43,3,00a1236a,0.000,508.650,0.000,0.000,0.000,NA,0.000,49,6,08119f84,21145952.567,-3665.791,49.608,13987.188,0.206,GOOD,0.033,49,6,10b13f8b,21145960.005,-2851.174,46.300,13984.918,0.000,OBSL2,0.000,51,0,18119fa4,22949361.716,3724.046,48.048,1598.788,5.125,GOOD,0.031,51,0,00b13fab,22949370.526,2896.484,45.398,1596.718,0.000,OBSL2,0.000,50,5,18119fc4,19987988.748,-30.141,38.296,6750.071,0.488,GOOD,0.020,50,5,10b13fcb,19987994.167,-23.443,43.197,6747.661,0.000,OBSL2,0.000,42,8,18119fe4,24110164.746,-2951.318,46.765,7697.488,-2.593,GOOD,0.024,42,8,10b13feb,24110172.476,-2295.476,41.549,7695.458,0.000,OBSL2,0.000,0,0,00018000,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12000,0.000,-0.015,0.000,0.000,0.000,NA,0.000,44,12,18119c24,23028956.405,3594.785,45.326,1587.448,-2.517,GOOD,0.033,44,12,10b13c2b,23028963.538,2795.949,40.845,1585.059,0.000,OBSL2,0.000,0,0,00018040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00018060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12060,0.000,0.001,0.000,0.000,0.000,NA,0.000,0,0,00018080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12080,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,000180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a120a0,0.000,-0.003,0.000,0.000,0.000,NA,0.000,16,0,08149cc4,40527641.107,-891.176,42.389,12619.062,0.015,GOOD,0.165,16,0,00349cc4,40527646.740,-688.872,40.549,12618.900,0.000,OBSB2,0.000,16,0,016420c9,0.000,-689.114,0.000,0.000,0.000,NA,0.000,46,0,08149ce4,26203507.728,2528.383,40.344,998.850,0.000,NOIONOCORR,0.000,46,0,002480e9,0.000,1955.106,0.000,0.000,0.000,NA,0.000,46,0,51743ce4,26203516.234,1954.975,45.261,998.249,0.000,OBSB2,0.000,51,0,08048101,0.000,834.000,0.000,0.000,0.000,NA,0.000,51,0,08048101,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,0,0,01642100,0.000,0.000,0.000,0.000,0.000,NA,0.000,9,0,08149d24,41114161.025,-63.950,39.061,3429.762,0.063,GOOD,0.164,9,0,00349d24,41114162.707,-49.152,40.579,6268.900,0.000,OBSB2,0.000,9,0,01642129,0.000,-49.450,0.000,0.000,0.000,NA,0.000,18,0,08048142,0.000,2191.000,0.000,0.000,0.000,NA,0.000,18,0,08048141,0.000,-3000.000,0.000,0.000,0.000,NA,0.000,0,0,01642140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00048160,0.000,0.000,0.000,0.000,0.000,NA,0.000,38,0,08048162,0.000,-4864.000,0.000,0.000,0.000,NA,0.000,0,0,01642160,0.000,0.000,0.000,0.000,0.000,NA,0.000,43,0,08149d84,21976230.668,-767.926,53.000,14708.749,0.000,NOIONOCORR,0.000,43,0,00248189,0.000,-593.809,0.000,0.000,0.000,NA,0.000,43,0,41743d84,21976231.759,-593.806,51.238,13013.732,0.000,OBSB2,0.000,34,0,18149da4,24778973.970,-2726.753,45.887,20888.650,0.000,NOIONOCORR,0.000,34,0,002481a9,0.000,-2108.499,0.000,0.000,0.000,NA,0.000,34,0,51743da4,24778976.771,-2108.527,42.384,20888.250,0.000,OBSB2,0.000,0,0,000481c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,10,0,080481c1,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,016421c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,1,0,088481e1,0.000,834.000,0.000,0.000,0.000,NA,0.000,1,0,088481e1,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,0,0,016421e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,37,0,18149e04,23644809.748,-209.827,51.046,8888.749,0.000,NOIONOCORR,0.000,37,0,00248209,0.000,-162.252,0.000,0.000,0.000,NA,0.000,37,0,51743e04,23644814.609,-162.173,47.800,8888.589,0.000,OBSB2,0.000,21,0,08149e24,24936225.322,-66.255,47.182,6239.049,0.000,NOIONOCORR,0.000,21,0,00248229,0.000,-51.232,0.000,0.000,0.000,NA,0.000,21,0,41743e24,24936224.946,-51.236,44.955,6237.648,0.000,OBSB2,0.000,55,0,08048241,0.000,11000.000,0.000,0.000,0.000,NA,0.000,55,0,08048241,0.000,-5000.000,0.000,0.000,0.000,NA,0.000,0,0,01642240,0.000,0.000,0.000,0.000,0.000,NA,0.000,23,0,08149e64,24335820.858,-2828.899,49.305,16609.148,0.000,NOIONOCORR,0.000,23,0,00248269,0.000,-2187.484,0.000,0.000,0.000,NA,0.000,23,0,41743e64,24335817.841,-2187.432,47.603,16608.648,0.000,OBSB2,0.000,6,0,08149e84,40542914.308,-698.666,37.528,8544.843,-0.444,GOOD,0.164,6,0,10349e84,40542914.850,-540.299,43.901,10378.661,0.000,OBSB2,0.000,6,0,01642289,0.000,-540.253,0.000,0.000,0.000,NA,0.000,27,0,080482a1,0.000,0.000,0.000,0.000,0.000,NA,0.000,27,0,080482a2,0.000,-6363.000,0.000,0.000,0.000,NA,0.000,0,0,016422a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,28,0,08149ec4,23032309.590,2093.708,50.562,6769.050,0.000,NOIONOCORR,0.000,28,0,002482c9,0.000,1618.988,0.000,0.000,0.000,NA,0.000,28,0,41743ec4,23032311.068,1619.068,48.490,5230.733,0.000,OBSB2,0.000,11,0,18149ee4,22903570.657,-2050.778,48.744,18098.982,1.808,GOOD,0.166,11,0,10349ee4,22903569.907,-1585.855,52.205,18098.830,0.000,OBSB2,0.000,11,0,016422e9,0.000,-1585.792,0.000,0.000,0.000,NA,0.000,48,0,08048302,0.000,1990.000,0.000,0.000,0.000,NA,0.000,48,0,08048301,0.000,-2000.000,0.000,0.000,0.000,NA,0.000,0,0,01642300,0.000,0.000,0.000,0.000,0.000,NA,0.000,39,0,18149f24,40712703.926,-1342.518,42.649,15938.900,0.000,NOIONOCORR,0.000,39,0,00248329,0.000,-1038.120,0.000,0.000,0.000,NA,0.000,39,0,41743f24,40712706.354,-1038.160,42.958,13271.623,0.000,OBSB2,0.000,26,0,08048341,0.000,9000.000,0.000,0.000,0.000,NA,0.000,26,0,08048341,0.000,-7000.000,0.000,0.000,0.000,NA,0.000,0,0,01642340,0.000,0.000,0.000,0.000,0.000,NA,0.000,42,0,08149f64,25060321.026,2422.533,46.916,3378.669,0.000,NOIONOCORR,0.000,42,0,00248369,0.000,1873.256,0.000,0.000,0.000,NA,0.000,42,0,51743f64,25060334.513,1873.301,46.947,3378.249,0.000,OBSB2,0.000,14,0,08149f84,25485562.721,3126.919,44.053,1548.970,-1.473,GOOD,0.163,14,0,00349f84,25485565.097,2418.014,47.552,1548.810,0.000,OBSB2,0.000,14,0,01642389,0.000,2417.932,0.000,0.000,0.000,NA,0.000,58,0,08049fa4,29895620.607,806.953,51.603,11229.342,0.000,BADHEALTH,0.000,58,0,002483a9,0.000,623.987,0.000,0.000,0.000,NA,0.000,58,0,016423a9,0.000,623.987,0.000,0.000,0.000,NA,0.000,0,0,826703c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,826703e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,82670000,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,82670020,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,82670040,0.000,0.000,0.000,0.000,0.000,NA,0.000*506ac68d";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 73;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ABBREV_ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_BIN_LOGLIST)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x05, 0x00, 0x00, 0x20, 0xC4, 0x00, 0x00, 0x00, 0x74, 0xB4, 0x7B, 0x08, 0x50, 0x7B, 0x11, 0x14, 0x00, 0x00, 0x01, 0x02, 0x0C, 0xC0, 0x78, 0x3F, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x05, 0x00, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x1E, 0x00, 0x00, 0x61, 0x07, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x9A, 0x99, 0x99, 0x99, 0x99, 0x99, 0xA9, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x1E, 0x00, 0x00, 0x62, 0x07, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF6, 0x1A, 0x12, 0x29 };
   uint8_t aucLogToConvert[] = "#LOGLISTA,COM1,0,58.0,FINESTEERING,2171,336690.000,02010000,c00c,16248;6,COM1,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,COM1,LOGLISTB,ONTIME,10.000000,0.000000,NOHOLD,COM1,LOGLISTA,ONTIME,10.000000,0.000000,NOHOLD,COM2,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,CCOM1,INSPVACMPB,ONTIME,0.050000,0.000000,HOLD,CCOM1,INSPVASDCMPB,ONTIME,1.000000,0.000000,HOLD*543ff79b\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::BINARY, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_ABB_LOGLIST)
{
   unsigned char aucLog[] = "<LOGLIST COM1 0 58.0 FINESTEERING 2171 336690.000 02010000 c00c 16248\r\n<     6 \r\n<          COM1 RXSTATUSEVENTA ONNEW 0.000000 0.000000 HOLD \r\n<          COM1 LOGLISTB ONTIME 10.000000 0.000000 NOHOLD \r\n<          COM1 LOGLISTA ONTIME 10.000000 0.000000 NOHOLD \r\n<          COM2 RXSTATUSEVENTA ONNEW 0.000000 0.000000 HOLD \r\n<          CCOM1 INSPVACMPB ONTIME 0.050000 0.000000 HOLD \r\n<          CCOM1 INSPVASDCMPB ONTIME 1.000000 0.000000 HOLD\r\n";
   uint8_t aucLogToConvert[] = "#LOGLISTA,COM1,0,58.0,FINESTEERING,2171,336690.000,02010000,c00c,16248;6,COM1,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,COM1,LOGLISTB,ONTIME,10.000000,0.000000,NOHOLD,COM1,LOGLISTA,ONTIME,10.000000,0.000000,NOHOLD,COM2,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,CCOM1,INSPVACMPB,ONTIME,0.050000,0.000000,HOLD,CCOM1,INSPVASDCMPB,ONTIME,1.000000,0.000000,HOLD*543ff79b\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 71;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ABBREV_ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_BIN_BESTSATS)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0xAA, 0x04, 0x00, 0x20, 0x84, 0x02, 0x00, 0x00, 0x6E, 0xB4, 0x7B, 0x08, 0xA0, 0xE9, 0xB7, 0x13, 0x00, 0x00, 0x01, 0x02, 0x05, 0xBE, 0x78, 0x3F, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x8A, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x17, 0x00, 0x03, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0E, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x65, 0x23, 0x06 };
   uint8_t aucLogToConvert[] = "#BESTSATSA,COM1,0,55.0,FINESTEERING,2171,330820.000,02010000,be05,16248;40,GPS,10,GOOD,00000007,GPS,29,GOOD,00000003,GPS,13,GOOD,00000003,GPS,15,GOOD,00000003,GPS,16,GOOD,00000003,GPS,18,GOOD,00000007,GPS,27,GOOD,00000007,GPS,5,GOOD,00000003,GPS,26,GOOD,00000007,GPS,23,GOOD,00000007,QZSS,194,SUPPLEMENTARY,00000007,SBAS,133,NOTUSED,00000000,SBAS,138,NOTUSED,00000000,SBAS,131,NOTUSED,00000000,GLONASS,22-3,GOOD,00000003,GLONASS,15,GOOD,00000003,GLONASS,23+3,SUPPLEMENTARY,00000001,GLONASS,1+1,GOOD,00000003,GLONASS,7+5,GOOD,00000003,GLONASS,14-7,GOOD,00000003,GLONASS,24+2,GOOD,00000003,GLONASS,8+6,GOOD,00000003,GALILEO,5,GOOD,0000000f,GALILEO,4,GOOD,0000000f,GALILEO,27,GOOD,0000000f,GALILEO,11,GOOD,0000000f,GALILEO,30,GOOD,0000000f,GALILEO,36,GOOD,0000000f,GALILEO,9,GOOD,0000000f,BEIDOU,44,LOCKEDOUT,00000000,BEIDOU,35,LOCKEDOUT,00000000,BEIDOU,7,NOEPHEMERIS,00000000,BEIDOU,29,SUPPLEMENTARY,00000001,BEIDOU,40,NOEPHEMERIS,00000000,BEIDOU,32,LOCKEDOUT,00000000,BEIDOU,22,ELEVATIONERROR,00000000,BEIDOU,56,NOEPHEMERIS,00000000,BEIDOU,19,SUPPLEMENTARY,00000001,BEIDOU,20,SUPPLEMENTARY,00000001,BEIDOU,57,NOEPHEMERIS,00000000*5c85690f\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::BINARY, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_ABB_BESTSATS)
{
   unsigned char aucLog[] = "<BESTSATS COM1 0 51.0 FINESTEERING 2211 419330.000 02000000 be05 32768\r\n<     34 \r\n<          GPS 5 GOOD 00000003 \r\n<          GPS 29 GOOD 00000003 \r\n<          GPS 9 GOOD 00000003 \r\n<          GPS 6 GOOD 00000003 \r\n<          GPS 31 GOOD 00000003 \r\n<          GPS 2 GOOD 00000003 \r\n<          GPS 20 GOOD 00000003 \r\n<          GPS 25 GOOD 00000003 \r\n<          GPS 11 GOOD 00000003 \r\n<          QZSS 194 GOOD 00000003 \r\n<          GLONASS 22-3 GOOD 00000003 \r\n<          GLONASS 23+3 NOIONOCORR 00000000 \r\n<          GLONASS 21+4 GOOD 00000003 \r\n<          GLONASS 6-4 NOIONOCORR 00000000 \r\n<          GLONASS 12-1 GOOD 00000003 \r\n<          GLONASS 14-7 GOOD 00000003 \r\n<          GLONASS 13-2 GOOD 00000003 \r\n<          GLONASS 24+2 GOOD 00000003 \r\n<          GLONASS 7+5 GOOD 00000003 \r\n<          BEIDOU 16 GOOD 00000003 \r\n<          BEIDOU 46 NOIONOCORR 00000000 \r\n<          BEIDOU 9 GOOD 00000003 \r\n<          BEIDOU 43 NOIONOCORR 00000000 \r\n<          BEIDOU 34 NOIONOCORR 00000000 \r\n<          BEIDOU 33 NOIONOCORR 00000000 \r\n<          BEIDOU 37 NOIONOCORR 00000000 \r\n<          BEIDOU 21 NOIONOCORR 00000000 \r\n<          BEIDOU 23 NOIONOCORR 00000000 \r\n<          BEIDOU 6 GOOD 00000003 \r\n<          BEIDOU 28 NOIONOCORR 00000000 \r\n<          BEIDOU 11 GOOD 00000003 \r\n<          BEIDOU 42 NOIONOCORR 00000000 \r\n<          BEIDOU 14 GOOD 00000003 \r\n<          BEIDOU 58 BADHEALTH 00000000\r\n";
   uint8_t aucLogToConvert[] = "#BESTSATSA,COM1,0,51.0,FINESTEERING,2211,419330.000,02000000,be05,32768;34,GPS,5,GOOD,00000003,GPS,29,GOOD,00000003,GPS,9,GOOD,00000003,GPS,6,GOOD,00000003,GPS,31,GOOD,00000003,GPS,2,GOOD,00000003,GPS,20,GOOD,00000003,GPS,25,GOOD,00000003,GPS,11,GOOD,00000003,QZSS,194,GOOD,00000003,GLONASS,22-3,GOOD,00000003,GLONASS,23+3,NOIONOCORR,00000000,GLONASS,21+4,GOOD,00000003,GLONASS,6-4,NOIONOCORR,00000000,GLONASS,12-1,GOOD,00000003,GLONASS,14-7,GOOD,00000003,GLONASS,13-2,GOOD,00000003,GLONASS,24+2,GOOD,00000003,GLONASS,7+5,GOOD,00000003,BEIDOU,16,GOOD,00000003,BEIDOU,46,NOIONOCORR,00000000,BEIDOU,9,GOOD,00000003,BEIDOU,43,NOIONOCORR,00000000,BEIDOU,34,NOIONOCORR,00000000,BEIDOU,33,NOIONOCORR,00000000,BEIDOU,37,NOIONOCORR,00000000,BEIDOU,21,NOIONOCORR,00000000,BEIDOU,23,NOIONOCORR,00000000,BEIDOU,6,GOOD,00000003,BEIDOU,28,NOIONOCORR,00000000,BEIDOU,11,GOOD,00000003,BEIDOU,42,NOIONOCORR,00000000,BEIDOU,14,GOOD,00000003,BEIDOU,58,BADHEALTH,00000000*156e7202\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 72;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ABBREV_ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_BIN_RAWGPSSUBFRAME)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x19, 0x00, 0x00, 0x20, 0x30, 0x00, 0x00, 0x00, 0x66, 0xB4, 0x7B, 0x08, 0x30, 0xD3, 0x1F, 0x14, 0x00, 0x00, 0x01, 0x02, 0x57, 0x04, 0x78, 0x3F, 0x06, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x8B, 0x01, 0xEC, 0x6D, 0xE7, 0xA4, 0x1E, 0xD0, 0x00, 0x2D, 0x80, 0x24, 0x86, 0x30, 0xFB, 0xE7, 0xAA, 0x3A, 0x04, 0xA4, 0xEA, 0x0C, 0x52, 0x9E, 0x00, 0x00, 0x13, 0x12, 0x15, 0x66, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x97, 0x0C, 0x92, 0xAB };
   uint8_t aucLogToConvert[] = "#RAWGPSSUBFRAMEA,COM1,0,51.0,FINESTEERING,2171,337630.000,02010000,0457,16248;6,21,1,8b01ec6de7a41ed0002d80248630fbe7aa3a04a4ea0c529e000013121566,6*91fc6cdd\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::BINARY, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_ABB_RAWGPSSUBFRAME)
{
   unsigned char aucLog[] = "<RAWGPSSUBFRAME COM1 0 53.5 FINESTEERING 2211 419460.000 02000000 0457 32768\r\n<     12 20 4 8b028c888ab35d11dd7b18d5fd4200a10d36427feb5cc4cbc33fcabcffe6 12\r\n";
   uint8_t aucLogToConvert[] = "#RAWGPSSUBFRAMEA,COM1,0,53.5,FINESTEERING,2211,419460.000,02000000,0457,32768;12,20,4,8b028c888ab35d11dd7b18d5fd4200a10d36427feb5cc4cbc33fcabcffe6,12*86299402\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 78;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ABBREV_ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

TEST_F(DecodeEncodeTest, BINARY_BESTUTM)
{
   uint8_t aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0xD6, 0x02, 0x00, 0xA0, 0x50, 0x00, 0x00, 0x00, 0x91, 0xB4, 0xBD, 0x08, 0x18, 0xCD, 0x0F, 0x10, 0x00, 0x00, 0x00, 0x1A, 0x0A, 0xA4, 0x6F, 0x42, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0xE3, 0xCA, 0x9A, 0x89, 0xD6, 0xA1, 0x55, 0x41, 0xD0, 0xB2, 0xEA, 0x2C, 0x9C, 0x98, 0x25, 0x41, 0x00, 0x60, 0x16, 0xCD, 0x30, 0x29, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x9B, 0x5A, 0x42, 0x3F, 0x37, 0x8E, 0x16, 0x3F, 0x79, 0x4A, 0xB7, 0x3F, 0x31, 0x33, 0x31, 0x00, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x0A, 0x0A, 0x0A, 0x00, 0x06, 0x00, 0x03, 0x93, 0xF1, 0x5F, 0x62 };
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog);
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   MetaDataStruct stExpectedMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::BINARY;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedMessageData, &stExpectedMetaData, false));
}

TEST_F(DecodeEncodeTest, CONVERSION_ASC_TO_ABB_FIELDS_AFTER_ARRAY)
{
   unsigned char aucLog[] = "<SBAS26 COM1 0 29.5 SATTIME 2234 401104.000 03000020 ec70 16860\r\n<     132 6 5 15 \r\n<          51 13 \r\n<          23 12 \r\n<          7 12 \r\n<          3 12 \r\n<          0 13 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<          0 0 \r\n<     3 0\r\n";
   uint8_t aucLogToConvert[] = "#SBAS26A,COM1,0,29.5,SATTIME,2234,401104.000,03000020,ec70,16860;132,6,5,15,51,13,23,12,7,12,3,12,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0*f98d2da0\r\n";
   MessageDataStruct stExpectedMessageData;
   stExpectedMessageData.pucMessage = aucLog;
   stExpectedMessageData.uiMessageLength = sizeof(aucLog) - 1;
   stExpectedMessageData.pucMessageHeader = aucLog;
   stExpectedMessageData.uiMessageHeaderLength = 65;
   stExpectedMessageData.pucMessageBody = aucLog + stExpectedMessageData.uiMessageHeaderLength;
   stExpectedMessageData.uiMessageBodyLength = stExpectedMessageData.uiMessageLength - stExpectedMessageData.uiMessageHeaderLength;

   ASSERT_EQ(DecodeEncodeTest::SUCCESS, TestConversion(ENCODEFORMAT::ABBREV_ASCII, aucLogToConvert, &stExpectedMessageData, nullptr, false));
}

// -------------------------------------------------------------------------------------------------------
// To FLATTENED_BINARY Conversion Decode/Encode Assertion Functions.
// -------------------------------------------------------------------------------------------------------
void ASSERT_HEADER_EQ(unsigned char* pucHeader1_, unsigned char* pucHeader2_)
{
   auto* pstHeader1 = reinterpret_cast<OEM4BinaryHeader*>(pucHeader1_);
   auto* pstHeader2 = reinterpret_cast<OEM4BinaryHeader*>(pucHeader2_);

   ASSERT_EQ(pstHeader1->ucSync1, pstHeader2->ucSync1);
   ASSERT_EQ(pstHeader1->ucSync2, pstHeader2->ucSync2);
   ASSERT_EQ(pstHeader1->ucSync3, pstHeader2->ucSync3);
   ASSERT_EQ(pstHeader1->ucHeaderLength, pstHeader2->ucHeaderLength);
   ASSERT_EQ(pstHeader1->usMsgNumber, pstHeader2->usMsgNumber);
   ASSERT_EQ(pstHeader1->ucMsgType, pstHeader2->ucMsgType);
   ASSERT_EQ(pstHeader1->ucPort, pstHeader2->ucPort);
   ASSERT_EQ(pstHeader1->usLength, pstHeader2->usLength);
   ASSERT_EQ(pstHeader1->usSequenceNumber, pstHeader2->usSequenceNumber);
   ASSERT_EQ(pstHeader1->ucIdleTime, pstHeader2->ucIdleTime);
   ASSERT_EQ(pstHeader1->ucTimeStatus, pstHeader2->ucTimeStatus);
   ASSERT_EQ(pstHeader1->usWeekNo, pstHeader2->usWeekNo);
   ASSERT_EQ(pstHeader1->uiWeekMSec, pstHeader2->uiWeekMSec);
   ASSERT_EQ(pstHeader1->uiStatus, pstHeader2->uiStatus);
   ASSERT_EQ(pstHeader1->usMsgDefCRC, pstHeader2->usMsgDefCRC);
   ASSERT_EQ(pstHeader1->usReceiverSWVersion, pstHeader2->usReceiverSWVersion);
}

void ASSERT_SHORT_HEADER_EQ(unsigned char* pucShortHeader_, unsigned char* pucHeader_)
{
   auto* pstShortHeader = reinterpret_cast<OEM4BinaryShortHeader*>(pucShortHeader_);
   auto* pstHeader      = reinterpret_cast<OEM4BinaryHeader*>(pucHeader_);

   ASSERT_EQ(pstShortHeader->ucSync1, pstHeader->ucSync1);
   ASSERT_EQ(pstShortHeader->ucSync2, pstHeader->ucSync2);
   ASSERT_EQ(pstShortHeader->ucSync3, pstHeader->ucSync3 + 1);
   ASSERT_EQ(pstShortHeader->usMessageId, pstHeader->usMsgNumber + 1);
   ASSERT_EQ(pstShortHeader->ucLength, pstHeader->usLength);
   ASSERT_EQ(pstShortHeader->usWeekNo, pstHeader->usWeekNo);
   ASSERT_EQ(pstShortHeader->uiWeekMSec, pstHeader->uiWeekMSec);
}

void ASSERT_BESTSATS_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_)
{
   auto* pstMessage1_ = reinterpret_cast<BESTSATS*>(pucMessage1_);
   auto* pstMessage2_ = reinterpret_cast<BESTSATS*>(pucMessage2_);

   ASSERT_EQ(pstMessage1_->satellite_entries_arraylength, pstMessage2_->satellite_entries_arraylength);

   for (uint32_t i = 0; i < pstMessage1_->satellite_entries_arraylength; i++)
   {
      ASSERT_EQ(pstMessage1_->satellite_entries[i].system_type, pstMessage2_->satellite_entries[i].system_type);
      ASSERT_EQ(pstMessage1_->satellite_entries[i].id.usPrnOrSlot, pstMessage2_->satellite_entries[i].id.usPrnOrSlot);
      ASSERT_EQ(pstMessage1_->satellite_entries[i].id.sFrequencyChannel, pstMessage2_->satellite_entries[i].id.sFrequencyChannel);
      ASSERT_EQ(pstMessage1_->satellite_entries[i].status, pstMessage2_->satellite_entries[i].status);
      ASSERT_EQ(pstMessage1_->satellite_entries[i].status_mask, pstMessage2_->satellite_entries[i].status_mask);
   }
}

void ASSERT_BESTPOS_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_)
{
   auto* pstMessage1 = reinterpret_cast<BESTPOS*>(pucMessage1_);
   auto* pstMessage2 = reinterpret_cast<BESTPOS*>(pucMessage2_);

   ASSERT_EQ(pstMessage1->solution_status, pstMessage2->solution_status);
   ASSERT_EQ(pstMessage1->position_type, pstMessage2->position_type);
   ASSERT_NEAR(pstMessage1->latitude, pstMessage2->latitude, 1e-11);
   ASSERT_NEAR(pstMessage1->longitude, pstMessage2->longitude, 1e-11);
   ASSERT_NEAR(pstMessage1->orthometric_height, pstMessage2->orthometric_height, 1e-4);
   ASSERT_FLOAT_EQ(pstMessage1->undulation, pstMessage2->undulation);
   ASSERT_EQ(pstMessage1->datum_id, pstMessage2->datum_id);
   ASSERT_NEAR(pstMessage1->latitude_std_dev, pstMessage2->latitude_std_dev, 1e-4);
   ASSERT_NEAR(pstMessage1->longitude_std_dev, pstMessage2->longitude_std_dev, 1e-4);
   ASSERT_NEAR(pstMessage1->height_std_dev, pstMessage2->height_std_dev, 1e-4);
   ASSERT_STREQ(reinterpret_cast<char*>(pstMessage1->base_id), reinterpret_cast<char*>(pstMessage2->base_id));
   ASSERT_FLOAT_EQ(pstMessage1->diff_age, pstMessage2->diff_age);
   ASSERT_FLOAT_EQ(pstMessage1->solution_age, pstMessage2->solution_age);
   ASSERT_EQ(pstMessage1->num_svs, pstMessage2->num_svs);
   ASSERT_EQ(pstMessage1->num_soln_svs, pstMessage2->num_soln_svs);
   ASSERT_EQ(pstMessage1->num_soln_L1_svs, pstMessage2->num_soln_L1_svs);
   ASSERT_EQ(pstMessage1->num_soln_multi_svs, pstMessage2->num_soln_multi_svs);
   ASSERT_EQ(pstMessage1->extended_solution_status2, pstMessage2->extended_solution_status2);
   ASSERT_EQ(pstMessage1->ext_sol_stat, pstMessage2->ext_sol_stat);
   ASSERT_EQ(pstMessage1->gal_and_bds_mask, pstMessage2->gal_and_bds_mask);
   ASSERT_EQ(pstMessage1->gps_and_glo_mask, pstMessage2->gps_and_glo_mask);
}

void ASSERT_LOGLIST_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_)
{
   auto* pstMessage1_ = reinterpret_cast<LOGLIST*>(pucMessage1_);
   auto* pstMessage2_ = reinterpret_cast<LOGLIST*>(pucMessage2_);

   ASSERT_EQ(pstMessage1_->log_list_arraylength, pstMessage2_->log_list_arraylength);

   for (uint32_t i = 0; i < pstMessage1_->log_list_arraylength; i++)
   {
      ASSERT_EQ(pstMessage1_->log_list[i].log_port_address, pstMessage2_->log_list[i].log_port_address);
      ASSERT_EQ(pstMessage1_->log_list[i].message_id, pstMessage2_->log_list[i].message_id);
      ASSERT_EQ(pstMessage1_->log_list[i].trigger, pstMessage2_->log_list[i].trigger);
      ASSERT_NEAR(pstMessage1_->log_list[i].on_time, pstMessage2_->log_list[i].on_time, 1e-3);
      ASSERT_EQ(pstMessage1_->log_list[i].offset, pstMessage2_->log_list[i].offset);
      ASSERT_EQ(pstMessage1_->log_list[i].hold, pstMessage2_->log_list[i].hold);
   }
}

void ASSERT_RAWGPSSUBFRAME_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_)
{
   auto* pstMessage1_ = reinterpret_cast<RAWGPSSUBFRAME*>(pucMessage1_);
   auto* pstMessage2_ = reinterpret_cast<RAWGPSSUBFRAME*>(pucMessage2_);

   ASSERT_EQ(pstMessage1_->frame_decoder_number, pstMessage2_->frame_decoder_number);
   ASSERT_EQ(pstMessage1_->satellite_id, pstMessage2_->satellite_id);
   ASSERT_EQ(pstMessage1_->sub_frame_id, pstMessage2_->sub_frame_id);

   for (uint64_t i = 0; i < sizeof(pstMessage2_->raw_sub_frame_data); i++)
   {
      ASSERT_EQ(pstMessage1_->raw_sub_frame_data[i], pstMessage2_->raw_sub_frame_data[i]);
   }

   ASSERT_EQ(pstMessage1_->signal_channel_number, pstMessage2_->signal_channel_number);
}

void ASSERT_TRACKSTAT_EQ(unsigned char* pucMessage1_, unsigned char* pucMessage2_)
{
   auto* pstMessage1_ = reinterpret_cast<TRACKSTAT*>(pucMessage1_);
   auto* pstMessage2_ = reinterpret_cast<TRACKSTAT*>(pucMessage2_);

   ASSERT_EQ(pstMessage1_->position_status, pstMessage2_->position_status);
   ASSERT_EQ(pstMessage1_->position_type, pstMessage2_->position_type);
   ASSERT_EQ(pstMessage1_->tracking_elevation_cutoff, pstMessage2_->tracking_elevation_cutoff);
   ASSERT_EQ(pstMessage1_->chan_status_arraylength, pstMessage2_->chan_status_arraylength);

   for (uint32_t i = 0; i < pstMessage1_->chan_status_arraylength; i++)
   {
      ASSERT_EQ(pstMessage1_->chan_status[i].prn, pstMessage2_->chan_status[i].prn);
      ASSERT_EQ(pstMessage1_->chan_status[i].freq, pstMessage2_->chan_status[i].freq);
      ASSERT_EQ(pstMessage1_->chan_status[i].channel_status, pstMessage2_->chan_status[i].channel_status);
      ASSERT_NEAR(pstMessage1_->chan_status[i].psr, pstMessage2_->chan_status[i].psr, 1e-3);
      ASSERT_NEAR(pstMessage1_->chan_status[i].doppler, pstMessage2_->chan_status[i].doppler, 1e-3);
      ASSERT_NEAR(pstMessage1_->chan_status[i].C_No, pstMessage2_->chan_status[i].C_No, 1e-3);
      ASSERT_NEAR(pstMessage1_->chan_status[i].lock_time, pstMessage2_->chan_status[i].lock_time, 1e-3);
      ASSERT_NEAR(pstMessage1_->chan_status[i].psr_residual, pstMessage2_->chan_status[i].psr_residual, 1e-3);
      ASSERT_EQ(pstMessage1_->chan_status[i].psr_range_reject, pstMessage2_->chan_status[i].psr_range_reject);
      ASSERT_NEAR(pstMessage1_->chan_status[i].psr_filter_weighting, pstMessage2_->chan_status[i].psr_filter_weighting, 1e-3);
   }
}

// -------------------------------------------------------------------------------------------------------
// To FLATTENED_BINARY Conversion Decode/Encode Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_HEADER_SHORT_ASC_TO_ASC)
{
   unsigned char aucLog1[] = "%INSPVASA,2174,397710.000;2174,397710.000000000,51.15044562951,-114.03068302183,1079.6234,0.0020,0.0018,-0.0026,0.728127738,0.536322202,-0.000000000,WAITING_AZIMUTH*e632993c\r\n";
   unsigned char aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   unsigned char aucLog2[] = "#INSPVAA,COM1,0,52.5,FINESTEERING,2174,397710.000,02000000,18bc,16649;2174,397710.000000000,51.15044562951,-114.03068302183,1079.6234,0.0020,0.0018,-0.0026,0.728127738,0.536322202,-0.000000000,WAITING_AZIMUTH*259ccd57";
   unsigned char aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_SHORT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_HEADER_SHORT_BIN_TO_BIN)
{
   // INSPVAS
   uint8_t aucLog1[] = { 0xAA, 0x44, 0x13, 0x58, 0xFC, 0x01, 0x7E, 0x08, 0xB0, 0x92, 0xB4, 0x17, 0x7E, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x46, 0x18, 0x41, 0x8C, 0x48, 0x69, 0xCD, 0x41, 0x93, 0x49, 0x40, 0x24, 0xD4, 0xEB, 0xB5, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x60, 0xF6, 0x55, 0x7E, 0xDE, 0x90, 0x40, 0xA1, 0x23, 0x99, 0x4C, 0x9F, 0x31, 0x60, 0x3F, 0x94, 0xFB, 0xF1, 0x3D, 0x25, 0x88, 0x5D, 0x3F, 0xF5, 0x27, 0x2C, 0xF6, 0xA2, 0x75, 0x65, 0xBF, 0x15, 0xA3, 0xE2, 0x8A, 0xD2, 0x4C, 0xE7, 0x3F, 0x32, 0xBD, 0xB3, 0x2D, 0x8D, 0x29, 0xE1, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0A, 0x00, 0x00, 0x00, 0x5A, 0xDA, 0x26, 0x64 };
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   // INSPVA
   uint8_t aucLog2[] = { 0xAA, 0x44, 0x12, 0x1C, 0xFB, 0x01, 0x00, 0x20, 0x58, 0x00, 0x00, 0x00, 0x69, 0xB4, 0x7E, 0x08, 0xB0, 0x92, 0xB4, 0x17, 0x00, 0x00, 0x00, 0x02, 0xBC, 0x18, 0x09, 0x41, 0x7E, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x46, 0x18, 0x41, 0x8C, 0x48, 0x69, 0xCD, 0x41, 0x93, 0x49, 0x40, 0x24, 0xD4, 0xEB, 0xB5, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x60, 0xF6, 0x55, 0x7E, 0xDE, 0x90, 0x40, 0xA1, 0x23, 0x99, 0x4C, 0x9F, 0x31, 0x60, 0x3F, 0x94, 0xFB, 0xF1, 0x3D, 0x25, 0x88, 0x5D, 0x3F, 0xF5, 0x27, 0x2C, 0xF6, 0xA2, 0x75, 0x65, 0xBF, 0x15, 0xA3, 0xE2, 0x8A, 0xD2, 0x4C, 0xE7, 0x3F, 0x32, 0xBD, 0xB3, 0x2D, 0x8D, 0x29, 0xE1, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0A, 0x00, 0x00, 0x00, 0xCB, 0x85, 0x94, 0xA8 };
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_SHORT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_BESTSATS_ABB_AND_ASC)
{
   uint8_t aucLog1[] = "<BESTSATS COM1 0 51.0 FINESTEERING 2211 419330.000 02000000 be05 32768\r\n<     34\r\n<          GPS 5 GOOD 00000003\r\n<          GPS 29 GOOD 00000003\r\n<          GPS 9 GOOD 00000003\r\n<          GPS 6 GOOD 00000003\r\n<          GPS 31 GOOD 00000003\r\n<          GPS 2 GOOD 00000003\r\n<          GPS 20 GOOD 00000003\r\n<          GPS 25 GOOD 00000003\r\n<          GPS 11 GOOD 00000003\r\n<          QZSS 194 GOOD 00000003\r\n<          GLONASS 22-3 GOOD 00000003\r\n<          GLONASS 23+3 NOIONOCORR 00000000\r\n<          GLONASS 21+4 GOOD 00000003\r\n<          GLONASS 6-4 NOIONOCORR 00000000\r\n<          GLONASS 12-1 GOOD 00000003\r\n<          GLONASS 14-7 GOOD 00000003\r\n<          GLONASS 13-2 GOOD 00000003\r\n<          GLONASS 24+2 GOOD 00000003\r\n<          GLONASS 7+5 GOOD 00000003\r\n<          BEIDOU 16 GOOD 00000003\r\n<          BEIDOU 46 NOIONOCORR 00000000\r\n<          BEIDOU 9 GOOD 00000003\r\n<          BEIDOU 43 NOIONOCORR 00000000\r\n<          BEIDOU 34 NOIONOCORR 00000000\r\n<          BEIDOU 33 NOIONOCORR 00000000\r\n<          BEIDOU 37 NOIONOCORR 00000000\r\n<          BEIDOU 21 NOIONOCORR 00000000\r\n<          BEIDOU 23 NOIONOCORR 00000000\r\n<          BEIDOU 6 GOOD 00000003\r\n<          BEIDOU 28 NOIONOCORR 00000000\r\n<          BEIDOU 11 GOOD 00000003\r\n<          BEIDOU 42 NOIONOCORR 00000000\r\n<          BEIDOU 14 GOOD 00000003\r\n<          BEIDOU 58 BADHEALTH 00000000\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = "#BESTSATSA,COM1,0,51.0,FINESTEERING,2211,419330.000,02000000,be05,32768;34,GPS,5,GOOD,00000003,GPS,29,GOOD,00000003,GPS,9,GOOD,00000003,GPS,6,GOOD,00000003,GPS,31,GOOD,00000003,GPS,2,GOOD,00000003,GPS,20,GOOD,00000003,GPS,25,GOOD,00000003,GPS,11,GOOD,00000003,QZSS,194,GOOD,00000003,GLONASS,22-3,GOOD,00000003,GLONASS,23+3,NOIONOCORR,00000000,GLONASS,21+4,GOOD,00000003,GLONASS,6-4,NOIONOCORR,00000000,GLONASS,12-1,GOOD,00000003,GLONASS,14-7,GOOD,00000003,GLONASS,13-2,GOOD,00000003,GLONASS,24+2,GOOD,00000003,GLONASS,7+5,GOOD,00000003,BEIDOU,16,GOOD,00000003,BEIDOU,46,NOIONOCORR,00000000,BEIDOU,9,GOOD,00000003,BEIDOU,43,NOIONOCORR,00000000,BEIDOU,34,NOIONOCORR,00000000,BEIDOU,33,NOIONOCORR,00000000,BEIDOU,37,NOIONOCORR,00000000,BEIDOU,21,NOIONOCORR,00000000,BEIDOU,23,NOIONOCORR,00000000,BEIDOU,6,GOOD,00000003,BEIDOU,28,NOIONOCORR,00000000,BEIDOU,11,GOOD,00000003,BEIDOU,42,NOIONOCORR,00000000,BEIDOU,14,GOOD,00000003,BEIDOU,58,BADHEALTH,00000000*156e7202\r\n";
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_BESTSATS_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_BESTSATS_ASC_AND_BIN)
{
   uint8_t aucLog1[] = "#BESTPOSA,COM1,0,58.5,FINESTEERING,2171,232010.000,02010000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043714161,-114.03068190724,1097.1381,-17.0000,WGS84,0.5977,0.5586,0.9357,\"131\",6.000,0.000,41,33,33,26,00,0b,1f,37*f1136906\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x75, 0xB4, 0x7B, 0x08, 0x10, 0x31, 0xD4, 0x0D, 0x00, 0x00, 0x01, 0x02, 0xF6, 0xB1, 0x78, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0xE1, 0xA7, 0x35, 0x86, 0x41, 0x93, 0x49, 0x40, 0x33, 0x0A, 0x3F, 0xB1, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x50, 0x4B, 0x62, 0x8D, 0x24, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xCA, 0xFF, 0x18, 0x3F, 0xEE, 0xFF, 0x0E, 0x3F, 0x41, 0x87, 0x6F, 0x3F, 0x31, 0x33, 0x31, 0x00, 0x00, 0x00, 0xC0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x29, 0x21, 0x21, 0x1A, 0x00, 0x0B, 0x1F, 0x37, 0x69, 0xCE, 0x4E, 0x06 };
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_BESTSATS_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_BESTPOS_ABB_AND_ASC)
{
   uint8_t aucLog1[] = "<BESTPOS COM1 0 54.0 FINESTEERING 2211 407360.000 02000000 cdba 32768\r\n<     SOL_COMPUTED SINGLE 51.15042853573 -114.03067810577 1095.6360 -17.0000 WGS84 0.9594 1.1258 1.9065 \"\" 0.000 0.000 31 21 21 21 00 06 30 33\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = "#BESTPOSA,COM1,0,54.0,FINESTEERING,2211,407360.000,02000000,cdba,32768;SOL_COMPUTED,SINGLE,51.15042853573,-114.03067810577,1095.6360,-17.0000,WGS84,0.9594,1.1258,1.9065,\"\",0.000,0.000,31,21,21,21,00,06,30,33*2a42e34b\r\n";
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_BESTPOS_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_BESTPOS_ASC_AND_BIN)
{
   uint8_t aucLog1[] = "#BESTPOSA,COM1,0,58.5,FINESTEERING,2171,232010.000,02010000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043714161,-114.03068190724,1097.1381,-17.0000,WGS84,0.5977,0.5586,0.9357,\"131\",6.000,0.000,41,33,33,26,00,0b,1f,37*f1136906\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x75, 0xB4, 0x7B, 0x08, 0x10, 0x31, 0xD4, 0x0D, 0x00, 0x00, 0x01, 0x02, 0xF6, 0xB1, 0x78, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0xE1, 0xA7, 0x35, 0x86, 0x41, 0x93, 0x49, 0x40, 0x33, 0x0A, 0x3F, 0xB1, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x50, 0x4B, 0x62, 0x8D, 0x24, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xCA, 0xFF, 0x18, 0x3F, 0xEE, 0xFF, 0x0E, 0x3F, 0x41, 0x87, 0x6F, 0x3F, 0x31, 0x33, 0x31, 0x00, 0x00, 0x00, 0xC0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x29, 0x21, 0x21, 0x1A, 0x00, 0x0B, 0x1F, 0x37, 0x69, 0xCE, 0x4E, 0x06 };
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_BESTPOS_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_LOGLIST_ABB_AND_ASC)
{
   uint8_t aucLog1[] = "<LOGLIST COM1 0 54.5 UNKNOWN 0 588960.000 024d4020 c00c 32768\r\n<     6\r\n<          COM1 RXSTATUSEVENTA ONNEW 0.000000 0.000000 HOLD\r\n<          COM1 DEBUGBUFFER ONCHANGED 0.000000 0.000000 NOHOLD\r\n<          COM1 LOGLIST ONTIME 10.000000 0.000000 NOHOLD\r\n<          COM1 LOGLISTA ONTIME 10.000000 0.000000 NOHOLD\r\n<          CCOM1 FILTINSPVACMPB ONTIME 0.050000 0.000000 HOLD\r\n<          CCOM1 INSPVASDCMPB ONTIME 1.000000 0.000000 HOLD\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = "#LOGLISTA,COM1,0,54.5,UNKNOWN,0,588960.000,024d4020,c00c,32768;6,COM1,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,COM1,DEBUGBUFFER,ONCHANGED,0.000000,0.000000,NOHOLD,COM1,LOGLIST,ONTIME,10.000000,0.000000,NOHOLD,COM1,LOGLISTA,ONTIME,10.000000,0.000000,NOHOLD,CCOM1,FILTINSPVACMPB,ONTIME,0.050000,0.000000,HOLD,CCOM1,INSPVASDCMPB,ONTIME,1.000000,0.000000,HOLD*2ca1f24f";
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_LOGLIST_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_LOGLIST_ASC_AND_BIN)
{
   uint8_t aucLog1[] = "#LOGLISTA,COM1,0,58.0,FINESTEERING,2171,336690.000,02010000,c00c,16248;6,COM1,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,COM1,LOGLISTB,ONTIME,10.000000,0.000000,NOHOLD,COM1,LOGLISTA,ONTIME,10.000000,0.000000,NOHOLD,COM2,RXSTATUSEVENTA,ONNEW,0.000000,0.000000,HOLD,CCOM1,INSPVACMPB,ONTIME,0.050000,0.000000,HOLD,CCOM1,INSPVASDCMPB,ONTIME,1.000000,0.000000,HOLD*543ff79b\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = { 0xAA, 0x44, 0x12, 0x1C, 0x05, 0x00, 0x00, 0x20, 0xC4, 0x00, 0x00, 0x00, 0x74, 0xB4, 0x7B, 0x08, 0x50, 0x7B, 0x11, 0x14, 0x00, 0x00, 0x01, 0x02, 0x0C, 0xC0, 0x78, 0x3F, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x05, 0x00, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x1E, 0x00, 0x00, 0x61, 0x07, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x9B, 0x99, 0x99, 0x99, 0x99, 0x99, 0xA9, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x1E, 0x00, 0x00, 0x62, 0x07, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFB, 0xE6, 0xDE, 0x40 };
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_LOGLIST_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_RAWGPSSUBFRAME_ABB_AND_ASC)
{
   uint8_t aucLog1[] = "<RAWGPSSUBFRAME COM1 0 53.5 FINESTEERING 2211 419460.000 02000000 0457 32768\r\n<     12 20 4 8b028c888ab35d11dd7b18d5fd4200a10d36427feb5cc4cbc33fcabcffe6 12\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = "#RAWGPSSUBFRAMEA,COM1,0,53.5,FINESTEERING,2211,419460.000,02000000,0457,32768;12,20,4,8b028c888ab35d11dd7b18d5fd4200a10d36427feb5cc4cbc33fcabcffe6,12*86299402\r\n";
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_RAWGPSSUBFRAME_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_RAWGPSSUBFRAME_ASC_AND_BIN)
{
   uint8_t aucLog1[] = "#RAWGPSSUBFRAMEA,COM1,0,51.0,FINESTEERING,2171,337630.000,02010000,0457,16248;6,21,1,8b01ec6de7a41ed0002d80248630fbe7aa3a04a4ea0c529e000013121566,6*91fc6cdd\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = { 0xAA, 0x44, 0x12, 0x1C, 0x19, 0x00, 0x00, 0x20, 0x30, 0x00, 0x00, 0x00, 0x66, 0xB4, 0x7B, 0x08, 0x30, 0xD3, 0x1F, 0x14, 0x00, 0x00, 0x01, 0x02, 0x57, 0x04, 0x78, 0x3F, 0x06, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x8B, 0x01, 0xEC, 0x6D, 0xE7, 0xA4, 0x1E, 0xD0, 0x00, 0x2D, 0x80, 0x24, 0x86, 0x30, 0xFB, 0xE7, 0xAA, 0x3A, 0x04, 0xA4, 0xEA, 0x0C, 0x52, 0x9E, 0x00, 0x00, 0x13, 0x12, 0x15, 0x66, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x97, 0x0C, 0x92, 0xAB };
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_RAWGPSSUBFRAME_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_TRACKSTAT_ABB_AND_ASC)
{
   uint8_t aucLog1[] = "<TRACKSTAT COM1 0 52.5 FINESTEERING 2211 417770.000 02000000 457c 32768\r\n<     SOL_COMPUTED SINGLE 5.0 149\r\n<          5 0 1810bc04 23701425.492 3789.992 46.788 2078.974 1.085 GOOD 0.244\r\n<          5 0 11305c0b 23701430.037 2953.246 45.620 2072.914 0.000 OBSL2 0.000\r\n<          29 0 0810bc24 23082310.768 2981.262 47.273 3828.954 -0.032 GOOD 0.247\r\n<          29 0 01305c2b 23082313.700 2323.065 44.880 3823.754 0.000 OBSL2 0.000\r\n<          19 0 0810bc44 24714583.651 -3486.828 44.074 21948.975 0.266 GOOD 0.242\r\n<          19 0 01305c4b 24714585.894 -2717.014 41.115 21943.754 0.000 OBSL2 0.000\r\n<          6 0 1810bc64 22530869.360 -3009.560 46.137 15678.695 -0.700 GOOD 0.248\r\n<          6 0 11305c6b 22530873.571 -2345.113 45.456 15671.915 0.000 OBSL2 0.000\r\n<          31 0 1810bc84 24133460.335 940.801 45.484 2898.935 -0.039 GOOD 0.245\r\n<          31 0 11305c8b 24133463.193 733.093 45.643 2893.415 0.000 OBSL2 0.000\r\n<          2 0 1810bca4 20963526.269 -712.720 52.205 11538.955 -0.451 GOOD 0.249\r\n<          2 0 11305cab 20963524.462 -555.367 51.430 11533.754 0.000 OBSL2 0.000\r\n<          20 0 1810bcc4 21742080.142 2309.293 50.589 5698.986 -0.339 GOOD 0.248\r\n<          20 0 11305ccb 21742080.656 1799.452 46.530 5692.415 0.000 OBSL2 0.000\r\n<          25 0 0810bce4 20665522.673 870.692 50.806 8086.716 -1.284 GOOD 0.248\r\n<          25 0 01305ceb 20665526.038 678.462 48.855 8080.517 0.000 OBSL2 0.000\r\n<          11 0 1810bd04 20362089.745 -601.177 51.571 11278.894 1.094 GOOD 0.249\r\n<          11 0 11305d0b 20362090.656 -468.450 51.328 11272.415 0.000 OBSL2 0.000\r\n<          0 0 0000a120 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a04120 0.000 -0.001 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0000a140 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a04140 0.000 -0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0000a160 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a04160 0.000 -0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0000a180 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a04180 0.000 -0.005 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0000a1a0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a041a0 0.000 -0.009 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0000a1c0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a041c0 0.000 -0.008 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0000a1e0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a041e0 0.000 -0.007 0.000 0.000 0.000 NA 0.000\r\n<          194 0 1815be04 43595011.956 105.359 40.180 2967.754 1.259 GOOD 0.083\r\n<          194 0 02359e0b 43595018.364 82.101 45.378 2964.394 0.000 OBSL2 0.000\r\n<          0 0 0005a220 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 02258220 0.000 -0.001 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0005a240 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 02258240 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 0005a260 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 02258260 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00022280 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 000222a0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 000222c0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 000222e0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          59 4 08119f04 19784649.048 -1697.276 40.617 11270.271 -0.406 GOOD 0.013\r\n<          59 4 00b13f0b 19784654.114 -1320.105 37.078 11267.843 0.000 OBSL2 0.000\r\n<          60 10 18019f24 20676119.555 2426.357 47.661 5537.208 0.000 NOIONOCORR 0.000\r\n<          60 10 00a12329 0.000 1887.167 0.000 0.000 0.000 NA 0.000\r\n<          58 11 08119f44 23007031.853 -3585.721 42.540 17527.469 -1.027 GOOD 0.024\r\n<          58 11 00b13f4b 23007037.627 -2788.897 43.873 17525.518 0.000 OBSL2 0.000\r\n<          43 3 08019f64 22109153.041 653.979 34.488 2875.611 0.000 NOIONOCORR 0.000\r\n<          43 3 00a1236a 0.000 508.650 0.000 0.000 0.000 NA 0.000\r\n<          49 6 08119f84 21145952.567 -3665.791 49.608 13987.188 0.206 GOOD 0.033\r\n<          49 6 10b13f8b 21145960.005 -2851.174 46.300 13984.918 0.000 OBSL2 0.000\r\n<          51 0 18119fa4 22949361.716 3724.046 48.048 1598.788 5.125 GOOD 0.031\r\n<          51 0 00b13fab 22949370.526 2896.484 45.398 1596.718 0.000 OBSL2 0.000\r\n<          50 5 18119fc4 19987988.748 -30.141 38.296 6750.071 0.488 GOOD 0.020\r\n<          50 5 10b13fcb 19987994.167 -23.443 43.197 6747.661 0.000 OBSL2 0.000\r\n<          42 8 18119fe4 24110164.746 -2951.318 46.765 7697.488 -2.593 GOOD 0.024\r\n<          42 8 10b13feb 24110172.476 -2295.476 41.549 7695.458 0.000 OBSL2 0.000\r\n<          0 0 00018000 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a12000 0.000 -0.015 0.000 0.000 0.000 NA 0.000\r\n<          44 12 18119c24 23028956.405 3594.785 45.326 1587.448 -2.517 GOOD 0.033\r\n<          44 12 10b13c2b 23028963.538 2795.949 40.845 1585.059 0.000 OBSL2 0.000\r\n<          0 0 00018040 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a12040 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00018060 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a12060 0.000 0.001 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00018080 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a12080 0.000 -0.001 0.000 0.000 0.000 NA 0.000\r\n<          0 0 000180a0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00a120a0 0.000 -0.003 0.000 0.000 0.000 NA 0.000\r\n<          16 0 08149cc4 40527641.107 -891.176 42.389 12619.062 0.015 GOOD 0.165\r\n<          16 0 00349cc4 40527646.740 -688.872 40.549 12618.900 0.000 OBSB2 0.000\r\n<          16 0 016420c9 0.000 -689.114 0.000 0.000 0.000 NA 0.000\r\n<          46 0 08149ce4 26203507.728 2528.383 40.344 998.850 0.000 NOIONOCORR 0.000\r\n<          46 0 002480e9 0.000 1955.106 0.000 0.000 0.000 NA 0.000\r\n<          46 0 51743ce4 26203516.234 1954.975 45.261 998.249 0.000 OBSB2 0.000\r\n<          51 0 08048101 0.000 834.000 0.000 0.000 0.000 NA 0.000\r\n<          51 0 08048101 0.000 -4166.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 01642100 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          9 0 08149d24 41114161.025 -63.950 39.061 3429.762 0.063 GOOD 0.164\r\n<          9 0 00349d24 41114162.707 -49.152 40.579 6268.900 0.000 OBSB2 0.000\r\n<          9 0 01642129 0.000 -49.450 0.000 0.000 0.000 NA 0.000\r\n<          18 0 08048142 0.000 2191.000 0.000 0.000 0.000 NA 0.000\r\n<          18 0 08048141 0.000 -3000.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 01642140 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 00048160 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          38 0 08048162 0.000 -4864.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 01642160 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          43 0 08149d84 21976230.668 -767.926 53.000 14708.749 0.000 NOIONOCORR 0.000\r\n<          43 0 00248189 0.000 -593.809 0.000 0.000 0.000 NA 0.000\r\n<          43 0 41743d84 21976231.759 -593.806 51.238 13013.732 0.000 OBSB2 0.000\r\n<          34 0 18149da4 24778973.970 -2726.753 45.887 20888.650 0.000 NOIONOCORR 0.000\r\n<          34 0 002481a9 0.000 -2108.499 0.000 0.000 0.000 NA 0.000\r\n<          34 0 51743da4 24778976.771 -2108.527 42.384 20888.250 0.000 OBSB2 0.000\r\n<          0 0 000481c0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          10 0 080481c1 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 016421c0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          1 0 088481e1 0.000 834.000 0.000 0.000 0.000 NA 0.000\r\n<          1 0 088481e1 0.000 -4166.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 016421e0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          37 0 18149e04 23644809.748 -209.827 51.046 8888.749 0.000 NOIONOCORR 0.000\r\n<          37 0 00248209 0.000 -162.252 0.000 0.000 0.000 NA 0.000\r\n<          37 0 51743e04 23644814.609 -162.173 47.800 8888.589 0.000 OBSB2 0.000\r\n<          21 0 08149e24 24936225.322 -66.255 47.182 6239.049 0.000 NOIONOCORR 0.000\r\n<          21 0 00248229 0.000 -51.232 0.000 0.000 0.000 NA 0.000\r\n<          21 0 41743e24 24936224.946 -51.236 44.955 6237.648 0.000 OBSB2 0.000\r\n<          55 0 08048241 0.000 11000.000 0.000 0.000 0.000 NA 0.000\r\n<          55 0 08048241 0.000 -5000.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 01642240 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          23 0 08149e64 24335820.858 -2828.899 49.305 16609.148 0.000 NOIONOCORR 0.000\r\n<          23 0 00248269 0.000 -2187.484 0.000 0.000 0.000 NA 0.000\r\n<          23 0 41743e64 24335817.841 -2187.432 47.603 16608.648 0.000 OBSB2 0.000\r\n<          6 0 08149e84 40542914.308 -698.666 37.528 8544.843 -0.444 GOOD 0.164\r\n<          6 0 10349e84 40542914.850 -540.299 43.901 10378.661 0.000 OBSB2 0.000\r\n<          6 0 01642289 0.000 -540.253 0.000 0.000 0.000 NA 0.000\r\n<          27 0 080482a1 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          27 0 080482a2 0.000 -6363.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 016422a0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          28 0 08149ec4 23032309.590 2093.708 50.562 6769.050 0.000 NOIONOCORR 0.000\r\n<          28 0 002482c9 0.000 1618.988 0.000 0.000 0.000 NA 0.000\r\n<          28 0 41743ec4 23032311.068 1619.068 48.490 5230.733 0.000 OBSB2 0.000\r\n<          11 0 18149ee4 22903570.657 -2050.778 48.744 18098.982 1.808 GOOD 0.166\r\n<          11 0 10349ee4 22903569.907 -1585.855 52.205 18098.830 0.000 OBSB2 0.000\r\n<          11 0 016422e9 0.000 -1585.792 0.000 0.000 0.000 NA 0.000\r\n<          48 0 08048302 0.000 1990.000 0.000 0.000 0.000 NA 0.000\r\n<          48 0 08048301 0.000 -2000.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 01642300 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          39 0 18149f24 40712703.926 -1342.518 42.649 15938.900 0.000 NOIONOCORR 0.000\r\n<          39 0 00248329 0.000 -1038.120 0.000 0.000 0.000 NA 0.000\r\n<          39 0 41743f24 40712706.354 -1038.160 42.958 13271.623 0.000 OBSB2 0.000\r\n<          26 0 08048341 0.000 9000.000 0.000 0.000 0.000 NA 0.000\r\n<          26 0 08048341 0.000 -7000.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 01642340 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          42 0 08149f64 25060321.026 2422.533 46.916 3378.669 0.000 NOIONOCORR 0.000\r\n<          42 0 00248369 0.000 1873.256 0.000 0.000 0.000 NA 0.000\r\n<          42 0 51743f64 25060334.513 1873.301 46.947 3378.249 0.000 OBSB2 0.000\r\n<          14 0 08149f84 25485562.721 3126.919 44.053 1548.970 -1.473 GOOD 0.163\r\n<          14 0 00349f84 25485565.097 2418.014 47.552 1548.810 0.000 OBSB2 0.000\r\n<          14 0 01642389 0.000 2417.932 0.000 0.000 0.000 NA 0.000\r\n<          58 0 08049fa4 29895620.607 806.953 51.603 11229.342 0.000 BADHEALTH 0.000\r\n<          58 0 002483a9 0.000 623.987 0.000 0.000 0.000 NA 0.000\r\n<          58 0 016423a9 0.000 623.987 0.000 0.000 0.000 NA 0.000\r\n<          0 0 826703c0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 826703e0 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 82670000 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 82670020 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n<          0 0 82670040 0.000 0.000 0.000 0.000 0.000 NA 0.000\r\n[\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = "#TRACKSTATA,COM1,0,52.5,FINESTEERING,2211,417770.000,02000000,457c,32768;SOL_COMPUTED,SINGLE,5.0,149,5,0,1810bc04,23701425.492,3789.992,46.788,2078.974,1.085,GOOD,0.244,5,0,11305c0b,23701430.037,2953.246,45.620,2072.914,0.000,OBSL2,0.000,29,0,0810bc24,23082310.768,2981.262,47.273,3828.954,-0.032,GOOD,0.247,29,0,01305c2b,23082313.700,2323.065,44.880,3823.754,0.000,OBSL2,0.000,19,0,0810bc44,24714583.651,-3486.828,44.074,21948.975,0.266,GOOD,0.242,19,0,01305c4b,24714585.894,-2717.014,41.115,21943.754,0.000,OBSL2,0.000,6,0,1810bc64,22530869.360,-3009.560,46.137,15678.695,-0.700,GOOD,0.248,6,0,11305c6b,22530873.571,-2345.113,45.456,15671.915,0.000,OBSL2,0.000,31,0,1810bc84,24133460.335,940.801,45.484,2898.935,-0.039,GOOD,0.245,31,0,11305c8b,24133463.193,733.093,45.643,2893.415,0.000,OBSL2,0.000,2,0,1810bca4,20963526.269,-712.720,52.205,11538.955,-0.451,GOOD,0.249,2,0,11305cab,20963524.462,-555.367,51.430,11533.754,0.000,OBSL2,0.000,20,0,1810bcc4,21742080.142,2309.293,50.589,5698.986,-0.339,GOOD,0.248,20,0,11305ccb,21742080.656,1799.452,46.530,5692.415,0.000,OBSL2,0.000,25,0,0810bce4,20665522.673,870.692,50.806,8086.716,-1.284,GOOD,0.248,25,0,01305ceb,20665526.038,678.462,48.855,8080.517,0.000,OBSL2,0.000,11,0,1810bd04,20362089.745,-601.177,51.571,11278.894,1.094,GOOD,0.249,11,0,11305d0b,20362090.656,-468.450,51.328,11272.415,0.000,OBSL2,0.000,0,0,0000a120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04120,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,0000a140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04140,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a160,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04160,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a04180,0.000,-0.005,0.000,0.000,0.000,NA,0.000,0,0,0000a1a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a041a0,0.000,-0.009,0.000,0.000,0.000,NA,0.000,0,0,0000a1c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a041c0,0.000,-0.008,0.000,0.000,0.000,NA,0.000,0,0,0000a1e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a041e0,0.000,-0.007,0.000,0.000,0.000,NA,0.000,194,0,1815be04,43595011.956,105.359,40.180,2967.754,1.259,GOOD,0.083,194,0,02359e0b,43595018.364,82.101,45.378,2964.394,0.000,OBSL2,0.000,0,0,0005a220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258220,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,0005a240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00022280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000222a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000222c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000222e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,59,4,08119f04,19784649.048,-1697.276,40.617,11270.271,-0.406,GOOD,0.013,59,4,00b13f0b,19784654.114,-1320.105,37.078,11267.843,0.000,OBSL2,0.000,60,10,18019f24,20676119.555,2426.357,47.661,5537.208,0.000,NOIONOCORR,0.000,60,10,00a12329,0.000,1887.167,0.000,0.000,0.000,NA,0.000,58,11,08119f44,23007031.853,-3585.721,42.540,17527.469,-1.027,GOOD,0.024,58,11,00b13f4b,23007037.627,-2788.897,43.873,17525.518,0.000,OBSL2,0.000,43,3,08019f64,22109153.041,653.979,34.488,2875.611,0.000,NOIONOCORR,0.000,43,3,00a1236a,0.000,508.650,0.000,0.000,0.000,NA,0.000,49,6,08119f84,21145952.567,-3665.791,49.608,13987.188,0.206,GOOD,0.033,49,6,10b13f8b,21145960.005,-2851.174,46.300,13984.918,0.000,OBSL2,0.000,51,0,18119fa4,22949361.716,3724.046,48.048,1598.788,5.125,GOOD,0.031,51,0,00b13fab,22949370.526,2896.484,45.398,1596.718,0.000,OBSL2,0.000,50,5,18119fc4,19987988.748,-30.141,38.296,6750.071,0.488,GOOD,0.020,50,5,10b13fcb,19987994.167,-23.443,43.197,6747.661,0.000,OBSL2,0.000,42,8,18119fe4,24110164.746,-2951.318,46.765,7697.488,-2.593,GOOD,0.024,42,8,10b13feb,24110172.476,-2295.476,41.549,7695.458,0.000,OBSL2,0.000,0,0,00018000,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12000,0.000,-0.015,0.000,0.000,0.000,NA,0.000,44,12,18119c24,23028956.405,3594.785,45.326,1587.448,-2.517,GOOD,0.033,44,12,10b13c2b,23028963.538,2795.949,40.845,1585.059,0.000,OBSL2,0.000,0,0,00018040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00018060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12060,0.000,0.001,0.000,0.000,0.000,NA,0.000,0,0,00018080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12080,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,000180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a120a0,0.000,-0.003,0.000,0.000,0.000,NA,0.000,16,0,08149cc4,40527641.107,-891.176,42.389,12619.062,0.015,GOOD,0.165,16,0,00349cc4,40527646.740,-688.872,40.549,12618.900,0.000,OBSB2,0.000,16,0,016420c9,0.000,-689.114,0.000,0.000,0.000,NA,0.000,46,0,08149ce4,26203507.728,2528.383,40.344,998.850,0.000,NOIONOCORR,0.000,46,0,002480e9,0.000,1955.106,0.000,0.000,0.000,NA,0.000,46,0,51743ce4,26203516.234,1954.975,45.261,998.249,0.000,OBSB2,0.000,51,0,08048101,0.000,834.000,0.000,0.000,0.000,NA,0.000,51,0,08048101,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,0,0,01642100,0.000,0.000,0.000,0.000,0.000,NA,0.000,9,0,08149d24,41114161.025,-63.950,39.061,3429.762,0.063,GOOD,0.164,9,0,00349d24,41114162.707,-49.152,40.579,6268.900,0.000,OBSB2,0.000,9,0,01642129,0.000,-49.450,0.000,0.000,0.000,NA,0.000,18,0,08048142,0.000,2191.000,0.000,0.000,0.000,NA,0.000,18,0,08048141,0.000,-3000.000,0.000,0.000,0.000,NA,0.000,0,0,01642140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00048160,0.000,0.000,0.000,0.000,0.000,NA,0.000,38,0,08048162,0.000,-4864.000,0.000,0.000,0.000,NA,0.000,0,0,01642160,0.000,0.000,0.000,0.000,0.000,NA,0.000,43,0,08149d84,21976230.668,-767.926,53.000,14708.749,0.000,NOIONOCORR,0.000,43,0,00248189,0.000,-593.809,0.000,0.000,0.000,NA,0.000,43,0,41743d84,21976231.759,-593.806,51.238,13013.732,0.000,OBSB2,0.000,34,0,18149da4,24778973.970,-2726.753,45.887,20888.650,0.000,NOIONOCORR,0.000,34,0,002481a9,0.000,-2108.499,0.000,0.000,0.000,NA,0.000,34,0,51743da4,24778976.771,-2108.527,42.384,20888.250,0.000,OBSB2,0.000,0,0,000481c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,10,0,080481c1,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,016421c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,1,0,088481e1,0.000,834.000,0.000,0.000,0.000,NA,0.000,1,0,088481e1,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,0,0,016421e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,37,0,18149e04,23644809.748,-209.827,51.046,8888.749,0.000,NOIONOCORR,0.000,37,0,00248209,0.000,-162.252,0.000,0.000,0.000,NA,0.000,37,0,51743e04,23644814.609,-162.173,47.800,8888.589,0.000,OBSB2,0.000,21,0,08149e24,24936225.322,-66.255,47.182,6239.049,0.000,NOIONOCORR,0.000,21,0,00248229,0.000,-51.232,0.000,0.000,0.000,NA,0.000,21,0,41743e24,24936224.946,-51.236,44.955,6237.648,0.000,OBSB2,0.000,55,0,08048241,0.000,11000.000,0.000,0.000,0.000,NA,0.000,55,0,08048241,0.000,-5000.000,0.000,0.000,0.000,NA,0.000,0,0,01642240,0.000,0.000,0.000,0.000,0.000,NA,0.000,23,0,08149e64,24335820.858,-2828.899,49.305,16609.148,0.000,NOIONOCORR,0.000,23,0,00248269,0.000,-2187.484,0.000,0.000,0.000,NA,0.000,23,0,41743e64,24335817.841,-2187.432,47.603,16608.648,0.000,OBSB2,0.000,6,0,08149e84,40542914.308,-698.666,37.528,8544.843,-0.444,GOOD,0.164,6,0,10349e84,40542914.850,-540.299,43.901,10378.661,0.000,OBSB2,0.000,6,0,01642289,0.000,-540.253,0.000,0.000,0.000,NA,0.000,27,0,080482a1,0.000,0.000,0.000,0.000,0.000,NA,0.000,27,0,080482a2,0.000,-6363.000,0.000,0.000,0.000,NA,0.000,0,0,016422a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,28,0,08149ec4,23032309.590,2093.708,50.562,6769.050,0.000,NOIONOCORR,0.000,28,0,002482c9,0.000,1618.988,0.000,0.000,0.000,NA,0.000,28,0,41743ec4,23032311.068,1619.068,48.490,5230.733,0.000,OBSB2,0.000,11,0,18149ee4,22903570.657,-2050.778,48.744,18098.982,1.808,GOOD,0.166,11,0,10349ee4,22903569.907,-1585.855,52.205,18098.830,0.000,OBSB2,0.000,11,0,016422e9,0.000,-1585.792,0.000,0.000,0.000,NA,0.000,48,0,08048302,0.000,1990.000,0.000,0.000,0.000,NA,0.000,48,0,08048301,0.000,-2000.000,0.000,0.000,0.000,NA,0.000,0,0,01642300,0.000,0.000,0.000,0.000,0.000,NA,0.000,39,0,18149f24,40712703.926,-1342.518,42.649,15938.900,0.000,NOIONOCORR,0.000,39,0,00248329,0.000,-1038.120,0.000,0.000,0.000,NA,0.000,39,0,41743f24,40712706.354,-1038.160,42.958,13271.623,0.000,OBSB2,0.000,26,0,08048341,0.000,9000.000,0.000,0.000,0.000,NA,0.000,26,0,08048341,0.000,-7000.000,0.000,0.000,0.000,NA,0.000,0,0,01642340,0.000,0.000,0.000,0.000,0.000,NA,0.000,42,0,08149f64,25060321.026,2422.533,46.916,3378.669,0.000,NOIONOCORR,0.000,42,0,00248369,0.000,1873.256,0.000,0.000,0.000,NA,0.000,42,0,51743f64,25060334.513,1873.301,46.947,3378.249,0.000,OBSB2,0.000,14,0,08149f84,25485562.721,3126.919,44.053,1548.970,-1.473,GOOD,0.163,14,0,00349f84,25485565.097,2418.014,47.552,1548.810,0.000,OBSB2,0.000,14,0,01642389,0.000,2417.932,0.000,0.000,0.000,NA,0.000,58,0,08049fa4,29895620.607,806.953,51.603,11229.342,0.000,BADHEALTH,0.000,58,0,002483a9,0.000,623.987,0.000,0.000,0.000,NA,0.000,58,0,016423a9,0.000,623.987,0.000,0.000,0.000,NA,0.000,0,0,826703c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,826703e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,82670000,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,82670020,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,82670040,0.000,0.000,0.000,0.000,0.000,NA,0.000*506ac68d";
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_TRACKSTAT_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

TEST_F(DecodeEncodeTest, CONVERSION_TO_FLAT_BIN_TRACKSTAT_ASC_AND_BIN)
{
   uint8_t aucLog1[] = "#TRACKSTATA,COM1,0,49.0,FINESTEERING,2171,398640.000,02010000,457c,16248;SOL_COMPUTED,WAAS,5.0,235,2,0,1810bc04,20896145.694,-217.249,53.850,10448.934,-0.465,GOOD,0.980,2,0,11303c0b,20896140.914,-169.285,51.590,10442.755,0.000,OBSL2,0.000,0,0,02208000,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02000,0.000,0.000,0.000,0.000,0.000,NA,0.000,20,0,1810bc24,22707310.192,3070.568,46.454,3868.615,0.120,GOOD,0.949,20,0,11303c2b,22707308.600,2392.650,43.722,3861.915,0.000,OBSL2,0.000,0,0,02208020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02020,0.000,0.000,0.000,0.000,0.000,NA,0.000,29,0,1810bc44,23905330.434,3168.755,43.740,2368.533,0.177,GOOD,0.427,29,0,11303c4b,23905329.129,2469.161,43.685,2362.015,0.000,OBSL2,0.000,29,0,02309c4b,23905329.780,2469.161,44.261,2365.375,0.000,OBSL2,0.000,0,0,01c02040,0.000,0.000,0.000,0.000,0.000,NA,0.000,6,0,1810bc64,21527085.528,-2321.828,49.094,13738.735,0.347,GOOD,0.978,6,0,11303c6b,21527086.727,-1809.217,49.051,13733.415,0.000,OBSL2,0.000,6,0,02309c6b,21527087.354,-1809.217,51.680,13735.296,0.000,OBSL2,0.000,6,0,01d03c64,21527088.559,-1733.929,53.706,13736.814,0.000,OBSL5,0.000,31,0,1810bc84,24673727.891,1980.320,41.565,1078.815,-0.418,GOOD,0.405,31,0,11303c8b,24673726.284,1543.107,41.202,1073.516,0.000,OBSL2,0.000,31,0,02309c8b,24673726.867,1543.109,43.518,1075.395,0.000,OBSL2,0.000,0,0,01c02080,0.000,0.000,0.000,0.000,0.000,NA,0.000,19,0,0810bca4,23786785.522,-3358.760,45.789,20368.574,-0.101,GOOD,0.956,19,0,01303cab,23786782.184,-2617.217,41.773,20362.254,0.000,OBSL2,0.000,0,0,022080a0,0.000,-0.004,0.000,0.000,0.000,NA,0.000,0,0,01c020a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,24,0,1810bcc4,25183575.384,-3468.042,42.976,17183.717,-0.029,GOOD,0.870,24,0,11303ccb,25183578.439,-2702.372,42.858,17177.516,0.000,OBSL2,0.000,24,0,02309ccb,25183578.702,-2702.372,47.048,17180.277,0.000,OBSL2,0.000,24,0,01d03cc4,25183579.739,-2589.781,49.028,17181.816,0.000,OBSL5,0.000,25,0,0810bce4,20975024.988,1660.168,50.335,6938.935,0.209,GOOD,0.971,25,0,01303ceb,20975025.604,1293.638,48.657,6932.756,0.000,OBSL2,0.000,25,0,02309ceb,20975026.214,1293.638,50.565,6935.815,0.000,OBSL2,0.000,25,0,01d03ce4,20975027.955,1239.682,54.122,6937.255,0.000,OBSL5,0.000,0,0,0000a100,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02100,0.000,-0.010,0.000,0.000,0.000,NA,0.000,0,0,02208100,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02100,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02120,0.000,-0.004,0.000,0.000,0.000,NA,0.000,0,0,02208120,0.000,-0.005,0.000,0.000,0.000,NA,0.000,0,0,01c02120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02140,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,02208140,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02140,0.000,0.000,0.000,0.000,0.000,NA,0.000,12,0,0810bd64,20240881.497,-528.198,52.945,11628.935,0.015,GOOD,0.980,12,0,01303d6b,20240878.785,-411.583,51.730,11622.916,0.000,OBSL2,0.000,12,0,02309d6b,20240879.335,-411.583,51.580,11625.614,0.000,OBSL2,0.000,0,0,01c02160,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02180,0.000,0.005,0.000,0.000,0.000,NA,0.000,0,0,02208180,0.000,0.005,0.000,0.000,0.000,NA,0.000,0,0,01c02180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022081a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021c0,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,022081c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022081e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,194,0,0815be04,43427077.798,119.358,43.060,4358.514,0.000,NODIFFCORR,0.000,194,0,02359e0b,43427078.197,93.006,43.803,4355.394,0.000,OBSL2,0.000,194,0,01d53e04,43427081.326,89.146,45.901,4356.833,0.000,OBSL5,0.000,0,0,0005a220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258260,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c52260,0.000,0.000,0.000,0.000,0.000,NA,0.000,133,0,58023e84,38625837.440,1.458,47.925,322076.969,0.000,LOCKEDOUT,0.000,138,0,58023ea4,38494663.415,1.903,48.045,1565693.625,0.000,LOCKEDOUT,0.000,144,0,080222c1,0.000,166.000,0.000,0.000,0.000,NA,0.000,131,0,58023ee4,38480032.645,-0.167,47.337,1565698.250,0.000,LOCKEDOUT,0.000,50,5,08018301,0.000,4097.000,0.000,0.000,0.000,NA,0.000,0,0,00a12300,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,00218300,0.000,-0.000,0.000,0.000,0.000,NA,0.000,49,6,08119f24,20180413.902,1424.752,48.590,6337.980,0.000,NODIFFCORR,0.000,49,6,00b13f2b,20180418.677,1108.141,48.456,6333.898,0.000,OBSL2,0.000,49,6,10319f2b,20180418.453,1108.141,49.174,6334.818,0.000,OBSL2,0.000,41,13,18119f44,23433009.567,-2006.451,41.675,6327.127,0.000,NODIFFCORR,0.000,41,13,10b13f4b,23433011.946,-1560.574,40.645,6321.898,0.000,OBSL2,0.000,41,13,00319f4b,23433012.447,-1560.574,41.008,6322.738,0.000,OBSL2,0.000,58,11,18119f64,19414951.522,-723.284,48.739,10947.098,0.000,NODIFFCORR,0.000,58,11,00b13f6b,19414952.430,-562.555,46.876,10941.897,0.000,OBSL2,0.000,58,11,10319f6b,19414953.159,-562.555,46.973,10942.738,0.000,OBSL2,0.000,59,4,18119f84,20729590.409,2535.796,36.096,14.960,0.000,NODIFFCORR,0.000,59,4,10b13f8b,20729594.281,1972.287,35.054,14.960,0.000,OBSL2,0.000,59,4,00319f8b,20729594.515,1972.286,35.695,14.960,0.000,OBSL2,0.000,57,9,18119fa4,22767196.378,-3658.212,33.092,14159.973,0.000,NODIFFCORR,0.000,57,9,10b13fab,22767200.843,-2845.278,41.861,14155.907,0.000,OBSL2,0.000,57,9,00319fab,22767202.552,-2845.278,41.492,14156.826,0.000,OBSL2,0.000,0,0,000183c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a123c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,002183c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,42,8,08119fe4,22393503.679,1361.899,43.938,3595.797,0.000,NODIFFCORR,0.000,42,8,10b13feb,22393507.436,1059.255,43.777,3590.039,0.000,OBSL2,0.000,42,8,10319feb,22393507.759,1059.256,44.162,3590.958,0.000,OBSL2,0.000,43,3,08018001,0.000,4037.000,0.000,0.000,0.000,NA,0.000,0,0,00a12000,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00218000,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00018020,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00218020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00018040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00018060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12060,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00218060,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,00018080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12080,0.000,-0.007,0.000,0.000,0.000,NA,0.000,0,0,00218080,0.000,-0.007,0.000,0.000,0.000,NA,0.000,0,0,000180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a120a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,002180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,26,0,08539cc4,22963489.915,766.027,53.922,10914.045,0.000,NODIFFCORR,0.000,26,0,01933cc4,22963494.808,571.961,53.434,10911.826,0.000,OBSE5,0.000,26,0,02333cc4,22963491.028,586.885,53.677,10912.045,0.000,OBSE5,0.000,26,0,02933cc4,22963491.069,579.431,56.348,10911.767,0.000,OBSE5,0.000,21,0,08539ce4,25762621.738,-2783.625,48.777,24054.283,0.000,NODIFFCORR,0.000,21,0,01933ce4,25762627.262,-2078.641,49.628,24052.043,0.000,OBSE5,0.000,21,0,02333ce4,25762622.443,-2132.879,49.099,24052.043,0.000,OBSE5,0.000,21,0,02933ce4,25762623.893,-2105.813,52.280,24051.943,0.000,OBSE5,0.000,13,0,08539d04,24612256.370,-2001.181,49.856,17556.785,0.000,NODIFFCORR,0.000,13,0,01933d04,24612260.363,-1494.437,51.242,17554.365,0.000,OBSE5,0.000,13,0,02333d04,24612257.183,-1533.415,51.594,17554.645,0.000,OBSE5,0.000,13,0,02933d04,24612256.859,-1513.902,54.135,17554.363,0.000,OBSE5,0.000,31,0,08539d24,26050071.272,2290.980,45.702,4770.864,0.000,NODIFFCORR,0.000,31,0,01933d24,26050076.452,1710.845,47.980,4768.823,0.000,OBSE5,0.000,31,0,02333d24,26050074.247,1755.423,49.726,4768.756,0.000,OBSE5,0.000,31,0,02933d24,26050073.589,1733.103,51.757,4768.564,0.000,OBSE5,0.000,3,0,08539d44,28323058.671,588.051,46.089,1063.543,0.000,NODIFFCORR,0.000,3,0,01933d44,28323064.262,439.076,47.811,1061.364,0.000,OBSE5,0.000,3,0,02333d44,28323061.323,450.452,48.562,1061.244,0.000,OBSE5,0.000,3,0,02933d44,28323060.676,444.799,50.954,1061.164,0.000,OBSE5,0.000,33,0,08539d64,26478271.103,2652.091,47.102,3645.384,0.000,NODIFFCORR,0.000,33,0,01933d64,26478275.360,1980.515,48.715,3643.044,0.000,OBSE5,0.000,33,0,02333d64,26478271.929,2032.176,48.750,3643.243,0.000,OBSE5,0.000,33,0,02933d64,26478271.282,2006.363,51.367,3642.943,0.000,OBSE5,0.000,0,0,00438180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,1,0,08539da4,23958766.073,-475.463,54.092,14535.322,0.000,NODIFFCORR,0.000,1,0,01933da4,23958769.774,-355.010,53.337,14533.043,0.000,OBSE5,0.000,1,0,02333da4,23958766.248,-364.278,53.535,14533.242,0.000,OBSE5,0.000,1,0,02933da4,23958766.283,-359.648,56.226,14532.942,0.000,OBSE5,0.000,0,0,004381c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004382a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,17,0,080482c1,0.000,1000.000,0.000,0.000,0.000,NA,0.000,17,0,080482c1,0.000,-4500.000,0.000,0.000,0.000,NA,0.000,28,0,08149ee4,21790652.051,-928.805,52.039,14739.068,0.000,NODIFFCORR,0.000,28,0,41343ee4,21790652.674,-699.991,52.547,14733.771,0.000,NA,0.000,39,0,48149f04,40782140.897,-1404.911,41.132,14657.694,0.000,NODIFFCORR,0.000,39,0,41343f04,40782141.013,-1058.816,45.107,16163.642,0.000,NA,0.000,37,0,58149f24,24136553.939,-2822.525,47.093,17079.148,0.000,NODIFFCORR,0.000,37,0,41343f24,24136559.917,-2127.116,50.676,17073.830,0.000,NA,0.000,16,0,18149f44,40866474.444,-958.876,38.978,12759.060,0.000,NODIFFCORR,0.000,16,0,10349f44,40866476.348,-741.557,39.789,12758.899,0.000,OBSB2,0.000,53,0,08048361,0.000,2000.000,0.000,0.000,0.000,NA,0.000,53,0,08048361,0.000,-14000.000,0.000,0.000,0.000,NA,0.000,43,0,48149f84,25483390.657,-2862.193,45.838,21209.148,0.000,NODIFFCORR,0.000,43,0,41343f84,25483394.575,-2157.030,46.839,21203.830,0.000,NA,0.000,2,0,088483a1,0.000,2500.000,0.000,0.000,0.000,NA,0.000,2,0,088483a1,0.000,-2500.000,0.000,0.000,0.000,NA,0.000,42,0,48149fc4,25419583.583,41.278,44.613,5499.149,0.000,NODIFFCORR,0.000,42,0,41343fc4,25419597.097,31.133,49.977,5493.829,0.000,NA,0.000,58,0,48049fe4,30936374.860,-2124.546,49.039,18146.543,0.000,NODIFFCORR,0.000,58,0,012423e9,0.000,-1600.901,0.000,0.000,0.000,NA,0.000,0,0,00048000,0.000,0.000,0.000,0.000,0.000,NA,0.000,18,0,08048002,0.000,-7620.000,0.000,0.000,0.000,NA,0.000,14,0,08149c24,24592161.459,1301.171,46.351,4309.071,0.000,NODIFFCORR,0.000,14,0,00349c24,24592158.131,1006.133,49.425,4308.911,0.000,OBSB2,0.000,46,0,48149c44,23130882.960,85.620,49.050,8689.149,0.000,NODIFFCORR,0.000,46,0,41343c44,23130889.469,64.558,52.186,8683.649,0.000,NA,0.000,0,0,00048060,0.000,0.000,0.000,0.000,0.000,NA,0.000,57,0,08048061,0.000,0.000,0.000,0.000,0.000,NA,0.000,33,0,48149c84,25393217.875,2491.764,43.814,2789.069,0.000,NODIFFCORR,0.000,33,0,41343c84,25393245.187,1877.809,50.270,2253.250,0.000,NA,0.000,27,0,18149ca4,22545097.717,1922.685,51.025,7399.069,0.000,NODIFFCORR,0.000,27,0,41343ca4,22545099.084,1448.937,50.373,7393.830,0.000,NA,0.000,0,0,000480c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,29,0,080480c1,0.000,-5000.000,0.000,0.000,0.000,NA,0.000,36,0,48149ce4,26118959.231,2578.202,40.927,1399.070,0.000,NODIFFCORR,0.000,36,0,41343ce4,26118969.105,1942.746,45.630,1393.830,0.000,NA,0.000,6,0,08149d04,40352408.747,-813.342,30.526,8905.414,0.000,NODIFFCORR,0.000,6,0,10349d04,40352406.750,-628.646,43.700,11028.902,0.000,OBSB2,0.000,51,0,08048121,0.000,2000.000,0.000,0.000,0.000,NA,0.000,51,0,08048122,0.000,-4714.000,0.000,0.000,0.000,NA,0.000,9,0,08149d44,40898830.833,-55.720,37.707,2297.372,0.000,NODIFFCORR,0.000,9,0,10349d44,40898828.592,-42.940,40.025,5808.901,0.000,OBSB2,0.000,0,0,00048160,0.000,0.000,0.000,0.000,0.000,NA,0.000,49,0,08048161,0.000,-5832.000,0.000,0.000,0.000,NA,0.000,6,0,0a670984,0.000,-313.494,37.057,1565675.125,0.000,NA,0.000,1,0,0a6709a4,0.000,64.105,43.017,669421.625,0.000,NA,0.000,0,0,026701c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,3,0,0a6701e1,0.000,-277.904,0.000,0.000,0.000,NA,0.000,0,0,02670200,0.000,0.000,0.000,0.000,0.000,NA,0.000*bc28a409\r\n";
   uint8_t aucLog1EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData1;
   MetaDataStruct stMetaData1;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog1, aucLog1EncodeBuffer, sizeof(aucLog1EncodeBuffer), stMetaData1, stMessageData1));

   uint8_t aucLog2[] = { 0xAA, 0x44, 0x12, 0x1C, 0x53, 0x00, 0x00, 0x20, 0xC8, 0x24, 0x00, 0x00, 0x62, 0xB4, 0x7B, 0x08, 0x80, 0xC3, 0xC2, 0x17, 0x00, 0x00, 0x01, 0x02, 0x7C, 0x45, 0x78, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0xEB, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0xBC, 0x10, 0x18, 0x43, 0x09, 0x1A, 0x1B, 0x99, 0xED, 0x73, 0x41, 0xDB, 0x3F, 0x59, 0xC3, 0x10, 0x66, 0x57, 0x42, 0xBC, 0x43, 0x23, 0x46, 0xC3, 0xFA, 0xED, 0xBE, 0x00, 0x00, 0x00, 0x00, 0x63, 0xD0, 0x7A, 0x3F, 0x02, 0x00, 0x00, 0x00, 0x0B, 0x3C, 0x30, 0x11, 0xF9, 0x5F, 0x9E, 0xCE, 0x98, 0xED, 0x73, 0x41, 0x11, 0x49, 0x29, 0xC3, 0xB4, 0x5B, 0x4E, 0x42, 0x05, 0x2B, 0x23, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0xD6, 0x05, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x24, 0xBC, 0x10, 0x18, 0xF2, 0xB2, 0x12, 0xE3, 0xC6, 0xA7, 0x75, 0x41, 0x15, 0xE9, 0x3F, 0x45, 0x65, 0xD1, 0x39, 0x42, 0xD6, 0xC9, 0x71, 0x45, 0xF9, 0xD9, 0xF4, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x1D, 0xEE, 0x72, 0x3F, 0x14, 0x00, 0x00, 0x00, 0x2B, 0x3C, 0x30, 0x11, 0x3F, 0xD4, 0x97, 0xC9, 0xC6, 0xA7, 0x75, 0x41, 0x68, 0x8A, 0x15, 0x45, 0xDE, 0xE2, 0x2E, 0x42, 0xA5, 0x5E, 0x71, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xFA, 0x1F, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x44, 0xBC, 0x10, 0x18, 0x24, 0x11, 0xF0, 0x26, 0x43, 0xCC, 0x76, 0x41, 0x15, 0x0C, 0x46, 0x45, 0x18, 0xF6, 0x2E, 0x42, 0x88, 0x08, 0x14, 0x45, 0x83, 0x1F, 0x35, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x7D, 0xDA, 0x3E, 0x1D, 0x00, 0x00, 0x00, 0x4B, 0x3C, 0x30, 0x11, 0x0F, 0xCD, 0x0E, 0x12, 0x43, 0xCC, 0x76, 0x41, 0x94, 0x52, 0x1A, 0x45, 0xB7, 0xBD, 0x2E, 0x42, 0x3C, 0xA0, 0x13, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x4B, 0x9C, 0x30, 0x02, 0xDB, 0x22, 0x7B, 0x1C, 0x43, 0xCC, 0x76, 0x41, 0x95, 0x52, 0x1A, 0x45, 0x06, 0x0B, 0x31, 0x42, 0xFE, 0xD5, 0x13, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x64, 0xBC, 0x10, 0x18, 0x64, 0xAE, 0x72, 0xD8, 0xA2, 0x87, 0x74, 0x41, 0x3E, 0x1D, 0x11, 0xC5, 0xEB, 0x5F, 0x44, 0x42, 0xF1, 0xAA, 0x56, 0x46, 0x3E, 0xE2, 0xB1, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x7D, 0x7B, 0x7A, 0x3F, 0x06, 0x00, 0x00, 0x00, 0x6B, 0x3C, 0x30, 0x11, 0x63, 0x06, 0xA0, 0xEB, 0xA2, 0x87, 0x74, 0x41, 0xEF, 0x26, 0xE2, 0xC4, 0x8D, 0x34, 0x44, 0x42, 0xA9, 0x95, 0x56, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x6B, 0x9C, 0x30, 0x02, 0x21, 0x5A, 0xAB, 0xF5, 0xA2, 0x87, 0x74, 0x41, 0xF1, 0x26, 0xE2, 0xC4, 0x09, 0xB8, 0x4E, 0x42, 0x2F, 0x9D, 0x56, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x64, 0x3C, 0xD0, 0x01, 0xF0, 0xB5, 0xF3, 0x08, 0xA3, 0x87, 0x74, 0x41, 0xBB, 0xBD, 0xD8, 0xC4, 0x3E, 0xD3, 0x56, 0x42, 0x42, 0xA3, 0x56, 0x46, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x84, 0xBC, 0x10, 0x18, 0x8B, 0xFA, 0x3F, 0xFE, 0xDB, 0x87, 0x77, 0x41, 0x3D, 0x8A, 0xF7, 0x44, 0xC0, 0x42, 0x26, 0x42, 0x17, 0xDA, 0x86, 0x44, 0x85, 0xFC, 0xD5, 0xBE, 0x00, 0x00, 0x00, 0x00, 0xB2, 0x26, 0xCF, 0x3E, 0x1F, 0x00, 0x00, 0x00, 0x8B, 0x3C, 0x30, 0x11, 0xCB, 0x2A, 0x8A, 0xE4, 0xDB, 0x87, 0x77, 0x41, 0x6E, 0xE3, 0xC0, 0x44, 0xB0, 0xCE, 0x24, 0x42, 0x82, 0x30, 0x86, 0x44, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x8B, 0x9C, 0x30, 0x02, 0x53, 0xFC, 0xE0, 0xED, 0xDB, 0x87, 0x77, 0x41, 0x7A, 0xE3, 0xC0, 0x44, 0x44, 0x12, 0x2E, 0x42, 0xA2, 0x6C, 0x86, 0x44, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0xA4, 0xBC, 0x10, 0x08, 0x15, 0x1D, 0x5B, 0x18, 0x52, 0xAF, 0x76, 0x41, 0x28, 0xEC, 0x51, 0xC5, 0x04, 0x28, 0x37, 0x42, 0x26, 0x21, 0x9F, 0x46, 0x4F, 0xC4, 0xCE, 0xBD, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xDA, 0x74, 0x3F, 0x13, 0x00, 0x00, 0x00, 0xAB, 0x3C, 0x30, 0x01, 0x8D, 0x1E, 0xF3, 0xE2, 0x51, 0xAF, 0x76, 0x41, 0x78, 0x93, 0x23, 0xC5, 0x9C, 0x17, 0x27, 0x42, 0x82, 0x14, 0x9F, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x80, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x21, 0x8A, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x20, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xC4, 0xBC, 0x10, 0x18, 0xA8, 0xDD, 0x25, 0x76, 0x55, 0x04, 0x78, 0x41, 0xAD, 0xC0, 0x58, 0xC5, 0x81, 0xE7, 0x2B, 0x42, 0x6F, 0x3F, 0x86, 0x46, 0xE5, 0xDD, 0xED, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x98, 0xA0, 0x5E, 0x3F, 0x18, 0x00, 0x00, 0x00, 0xCB, 0x3C, 0x30, 0x11, 0x31, 0x38, 0x04, 0xA7, 0x55, 0x04, 0x78, 0x41, 0xF5, 0xE5, 0x28, 0xC5, 0x8F, 0x6E, 0x2B, 0x42, 0x08, 0x33, 0x86, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xCB, 0x9C, 0x30, 0x02, 0xC0, 0xD0, 0x3C, 0xAB, 0x55, 0x04, 0x78, 0x41, 0xF4, 0xE5, 0x28, 0xC5, 0xC4, 0x30, 0x3C, 0x42, 0x8E, 0x38, 0x86, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0xD0, 0x01, 0xF3, 0x10, 0xD3, 0xBB, 0x55, 0x04, 0x78, 0x41, 0x7D, 0xDC, 0x21, 0xC5, 0x18, 0x1D, 0x44, 0x42, 0xA2, 0x3B, 0x86, 0x46, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xE4, 0xBC, 0x10, 0x08, 0x4A, 0x15, 0xCF, 0x0F, 0xDB, 0x00, 0x74, 0x41, 0x63, 0x85, 0xCF, 0x44, 0xEF, 0x56, 0x49, 0x42, 0x7B, 0xD7, 0xD8, 0x45, 0x3C, 0x4A, 0x56, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x86, 0x78, 0x3F, 0x19, 0x00, 0x00, 0x00, 0xEB, 0x3C, 0x30, 0x01, 0x2D, 0x85, 0xAA, 0x19, 0xDB, 0x00, 0x74, 0x41, 0x67, 0xB4, 0xA1, 0x44, 0x31, 0xA1, 0x42, 0x42, 0x0C, 0xA6, 0xD8, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xEB, 0x9C, 0x30, 0x02, 0x40, 0xF1, 0x6A, 0x23, 0xDB, 0x00, 0x74, 0x41, 0x68, 0xB4, 0xA1, 0x44, 0x10, 0x42, 0x4A, 0x42, 0x85, 0xBE, 0xD8, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0xD0, 0x01, 0x1D, 0xE0, 0x46, 0x3F, 0xDB, 0x00, 0x74, 0x41, 0xD0, 0xF5, 0x9A, 0x44, 0x3A, 0x7D, 0x58, 0x42, 0x0B, 0xCA, 0xD8, 0x45, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0xE6, 0x21, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCA, 0xD0, 0xFF, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x3F, 0x89, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x7F, 0x9F, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0x73, 0x47, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xC5, 0xAF, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x64, 0xBD, 0x10, 0x08, 0x38, 0xFA, 0xF2, 0x17, 0x9F, 0x4D, 0x73, 0x41, 0xA6, 0x0C, 0x04, 0xC4, 0xC7, 0xC7, 0x53, 0x42, 0xBD, 0xB3, 0x35, 0x46, 0x02, 0x6A, 0x6E, 0x3C, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x7B, 0x3F, 0x0C, 0x00, 0x00, 0x00, 0x6B, 0x3D, 0x30, 0x01, 0xF0, 0xB6, 0x8D, 0xEC, 0x9E, 0x4D, 0x73, 0x41, 0x9D, 0xCA, 0xCD, 0xC3, 0xE5, 0xEB, 0x4E, 0x42, 0xAA, 0x9B, 0x35, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x6B, 0x9D, 0x30, 0x02, 0xAF, 0x00, 0x5B, 0xF5, 0x9E, 0x4D, 0x73, 0x41, 0x98, 0xCA, 0xCD, 0xC3, 0xA7, 0x51, 0x4E, 0x42, 0x75, 0xA6, 0x35, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x6A, 0xA7, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA4, 0x10, 0xB2, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0xAF, 0x1B, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x81, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x04, 0xBE, 0x15, 0x08, 0xF4, 0xC0, 0x61, 0x2E, 0x28, 0xB5, 0x84, 0x41, 0x19, 0xB7, 0xEE, 0x42, 0x94, 0x3D, 0x2C, 0x42, 0x1D, 0x34, 0x88, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x0B, 0x9E, 0x35, 0x02, 0x53, 0xF4, 0x92, 0x31, 0x28, 0xB5, 0x84, 0x41, 0xF3, 0x02, 0xBA, 0x42, 0xFD, 0x35, 0x2F, 0x42, 0x26, 0x1B, 0x88, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00, 0x04, 0x3E, 0xD5, 0x01, 0x2B, 0x3D, 0x9C, 0x4A, 0x28, 0xB5, 0x84, 0x41, 0xA5, 0x4A, 0xB2, 0x42, 0xC4, 0x9A, 0x37, 0x42, 0xAB, 0x26, 0x88, 0x45, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xA2, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x82, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0xC5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xA2, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x82, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0xC5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0xA2, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x82, 0x25, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x9A, 0x08, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0xC5, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x84, 0x3E, 0x02, 0x58, 0xC5, 0xD8, 0x84, 0x6B, 0x11, 0x6B, 0x82, 0x41, 0xD7, 0x90, 0xBA, 0x3F, 0x01, 0xB3, 0x3F, 0x42, 0x9F, 0x43, 0x9D, 0x48, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8A, 0x00, 0x00, 0x00, 0xA4, 0x3E, 0x02, 0x58, 0xD3, 0x6A, 0x51, 0x3B, 0x0E, 0x5B, 0x82, 0x41, 0x9A, 0xA3, 0xF3, 0x3F, 0x96, 0x2D, 0x40, 0x42, 0xED, 0x1F, 0xBF, 0x49, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0xC1, 0x22, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0xE4, 0x3E, 0x02, 0x58, 0xA3, 0x31, 0x28, 0x05, 0x45, 0x59, 0x82, 0x41, 0xB7, 0x64, 0x2B, 0xBE, 0xD6, 0x58, 0x3D, 0x42, 0x12, 0x20, 0xBF, 0x49, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x05, 0x00, 0x01, 0x83, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8E, 0x93, 0xD0, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA8, 0x9F, 0xA9, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x06, 0x00, 0x24, 0x9F, 0x11, 0x08, 0x4F, 0x7F, 0x70, 0xDE, 0xDB, 0x3E, 0x73, 0x41, 0x11, 0x18, 0xB2, 0x44, 0x4E, 0x5C, 0x42, 0x42, 0xD7, 0x0F, 0xC6, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x06, 0x00, 0x2B, 0x3F, 0xB1, 0x00, 0x7E, 0x99, 0xD3, 0x2A, 0xDC, 0x3E, 0x73, 0x41, 0x80, 0x84, 0x8A, 0x44, 0xEA, 0xD2, 0x41, 0x42, 0x30, 0xEF, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x06, 0x00, 0x2B, 0x9F, 0x31, 0x10, 0x88, 0x8C, 0x3F, 0x27, 0xDC, 0x3E, 0x73, 0x41, 0x81, 0x84, 0x8A, 0x44, 0x3D, 0xB2, 0x44, 0x42, 0x8C, 0xF6, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x0D, 0x00, 0x44, 0x9F, 0x11, 0x18, 0x43, 0x60, 0x11, 0x19, 0xF3, 0x58, 0x76, 0x41, 0x6E, 0xCE, 0xFA, 0xC4, 0x52, 0xB3, 0x26, 0x42, 0x05, 0xB9, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x0D, 0x00, 0x4B, 0x3F, 0xB1, 0x10, 0xA6, 0xD7, 0x24, 0x3F, 0xF3, 0x58, 0x76, 0x41, 0x5D, 0x12, 0xC3, 0xC4, 0xD7, 0x94, 0x22, 0x42, 0x2F, 0x8F, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x0D, 0x00, 0x4B, 0x9F, 0x31, 0x00, 0xCB, 0x84, 0x27, 0x47, 0xF3, 0x58, 0x76, 0x41, 0x61, 0x12, 0xC3, 0xC4, 0x07, 0x08, 0x24, 0x42, 0xE7, 0x95, 0xC5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0B, 0x00, 0x64, 0x9F, 0x11, 0x18, 0x7A, 0x79, 0x59, 0x78, 0xFA, 0x83, 0x72, 0x41, 0x2A, 0xD2, 0x34, 0xC4, 0x88, 0xF4, 0x42, 0x42, 0x64, 0x0C, 0x2B, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0B, 0x00, 0x6B, 0x3F, 0xB1, 0x00, 0xA9, 0x29, 0xE2, 0x86, 0xFA, 0x83, 0x72, 0x41, 0x88, 0xA3, 0x0C, 0xC4, 0x63, 0x81, 0x3B, 0x42, 0x97, 0xF7, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0B, 0x00, 0x6B, 0x9F, 0x31, 0x10, 0xBD, 0x14, 0x8A, 0x92, 0xFA, 0x83, 0x72, 0x41, 0x87, 0xA3, 0x0C, 0xC4, 0xEA, 0xE3, 0x3B, 0x42, 0xF4, 0xFA, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x84, 0x9F, 0x11, 0x18, 0x30, 0xF5, 0x89, 0x66, 0xEF, 0xC4, 0x73, 0x41, 0xBE, 0x7C, 0x1E, 0x45, 0x1B, 0x62, 0x10, 0x42, 0x3B, 0x5D, 0x6F, 0x41, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x8B, 0x3F, 0xB1, 0x10, 0x5C, 0x9D, 0x80, 0xA4, 0xEF, 0xC4, 0x73, 0x41, 0x2E, 0x89, 0xF6, 0x44, 0x5F, 0x37, 0x0C, 0x42, 0x3B, 0x5D, 0x6F, 0x41, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x04, 0x00, 0x8B, 0x9F, 0x31, 0x00, 0x65, 0x6A, 0x3E, 0xA8, 0xEF, 0xC4, 0x73, 0x41, 0x2B, 0x89, 0xF6, 0x44, 0x53, 0xC7, 0x0E, 0x42, 0x3B, 0x5D, 0x6F, 0x41, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x09, 0x00, 0xA4, 0x9F, 0x11, 0x18, 0x52, 0xF7, 0x0B, 0xC6, 0x65, 0xB6, 0x75, 0x41, 0x64, 0xA3, 0x64, 0xC5, 0x92, 0x5E, 0x04, 0x42, 0xE4, 0x3F, 0x5D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x09, 0x00, 0xAB, 0x3F, 0xB1, 0x10, 0xF7, 0xD2, 0x7B, 0x0D, 0x66, 0xB6, 0x75, 0x41, 0x72, 0xD4, 0x31, 0xC5, 0x2C, 0x71, 0x27, 0x42, 0xA1, 0x2F, 0x5D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x09, 0x00, 0xAB, 0x9F, 0x31, 0x00, 0x78, 0x23, 0xD3, 0x28, 0x66, 0xB6, 0x75, 0x41, 0x71, 0xD4, 0x31, 0xC5, 0xE4, 0xF7, 0x25, 0x42, 0x4E, 0x33, 0x5D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x23, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x83, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x08, 0x00, 0xE4, 0x9F, 0x11, 0x08, 0xC8, 0x62, 0xDE, 0xFA, 0x29, 0x5B, 0x75, 0x41, 0xC5, 0x3C, 0xAA, 0x44, 0x50, 0xC0, 0x2F, 0x42, 0xC0, 0xBC, 0x60, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x08, 0x00, 0xEB, 0x3F, 0xB1, 0x10, 0x7F, 0xD3, 0xF9, 0x36, 0x2A, 0x5B, 0x75, 0x41, 0x2D, 0x68, 0x84, 0x44, 0xF3, 0x1B, 0x2F, 0x42, 0x9E, 0x60, 0x60, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x08, 0x00, 0xEB, 0x9F, 0x31, 0x10, 0xBC, 0xC8, 0x26, 0x3C, 0x2A, 0x5B, 0x75, 0x41, 0x2E, 0x68, 0x84, 0x44, 0xD5, 0xA5, 0x30, 0x42, 0x56, 0x6F, 0x60, 0x45, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x03, 0x00, 0x01, 0x80, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x7C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0x37, 0x14, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD6, 0xC4, 0x17, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x09, 0x03, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x26, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB1, 0xB1, 0x19, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x25, 0xDB, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xE2, 0xDC, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71, 0xFD, 0xF2, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x20, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x80, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x9C, 0x53, 0x08, 0x66, 0x34, 0xA5, 0x1E, 0x52, 0xE6, 0x75, 0x41, 0xBC, 0x81, 0x3F, 0x44, 0x38, 0xB0, 0x57, 0x42, 0x2E, 0x88, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0x93, 0x01, 0x3C, 0x93, 0xEE, 0x6C, 0x52, 0xE6, 0x75, 0x41, 0x80, 0xFD, 0x0E, 0x44, 0x36, 0xBC, 0x55, 0x42, 0x4E, 0x7F, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0x33, 0x02, 0x2B, 0x95, 0x74, 0x30, 0x52, 0xE6, 0x75, 0x41, 0xA4, 0xB8, 0x12, 0x44, 0x6C, 0xB5, 0x56, 0x42, 0x2E, 0x80, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xC4, 0x3C, 0x93, 0x02, 0x1E, 0x5B, 0x1A, 0x31, 0x52, 0xE6, 0x75, 0x41, 0x94, 0xDB, 0x10, 0x44, 0x7D, 0x64, 0x61, 0x42, 0x11, 0x7F, 0x2A, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x9C, 0x53, 0x08, 0x5E, 0x77, 0xD0, 0xDB, 0xB3, 0x91, 0x78, 0x41, 0x02, 0xFA, 0x2D, 0xC5, 0x53, 0x1B, 0x43, 0x42, 0x91, 0xEC, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x93, 0x01, 0xD9, 0xAB, 0x32, 0x34, 0xB4, 0x91, 0x78, 0x41, 0x43, 0xEA, 0x01, 0xC5, 0xBF, 0x82, 0x46, 0x42, 0x16, 0xE8, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x33, 0x02, 0x64, 0x60, 0x17, 0xE7, 0xB3, 0x91, 0x78, 0x41, 0x0F, 0x4E, 0x05, 0xC5, 0x9F, 0x65, 0x44, 0x42, 0x16, 0xE8, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x93, 0x02, 0x06, 0xBE, 0x49, 0xFE, 0xB3, 0x91, 0x78, 0x41, 0x03, 0x9D, 0x03, 0xC5, 0xF0, 0x1E, 0x51, 0x42, 0xE3, 0xE7, 0xBB, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x9D, 0x53, 0x08, 0x80, 0x6F, 0xED, 0x05, 0xDA, 0x78, 0x77, 0x41, 0xC8, 0x25, 0xFA, 0xC4, 0xB0, 0x6C, 0x47, 0x42, 0x92, 0x29, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x3D, 0x93, 0x01, 0xBD, 0x45, 0xCF, 0x45, 0xDA, 0x78, 0x77, 0x41, 0xFF, 0xCD, 0xBA, 0xC4, 0xE1, 0xF7, 0x4C, 0x42, 0xBB, 0x24, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x3D, 0x33, 0x02, 0x62, 0x68, 0xEE, 0x12, 0xDA, 0x78, 0x77, 0x41, 0x4A, 0xAD, 0xBF, 0xC4, 0x93, 0x60, 0x4E, 0x42, 0x4A, 0x25, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x04, 0x3D, 0x93, 0x02, 0xCC, 0x38, 0xC0, 0x0D, 0xDA, 0x78, 0x77, 0x41, 0xDB, 0x3C, 0xBD, 0xC4, 0x18, 0x8A, 0x58, 0x42, 0xBA, 0x24, 0x89, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x9D, 0x53, 0x08, 0x02, 0x70, 0x5B, 0x74, 0xE1, 0xD7, 0x78, 0x41, 0xAD, 0x2F, 0x0F, 0x45, 0x7A, 0xCE, 0x36, 0x42, 0xE9, 0x16, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x3D, 0x93, 0x01, 0x64, 0xBA, 0x3A, 0xC7, 0xE1, 0xD7, 0x78, 0x41, 0x0E, 0xDB, 0xD5, 0x44, 0x6C, 0xEB, 0x3F, 0x42, 0x96, 0x06, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x3D, 0x33, 0x02, 0x11, 0x28, 0xF3, 0xA3, 0xE1, 0xD7, 0x78, 0x41, 0x88, 0x6D, 0xDB, 0x44, 0xA2, 0xE7, 0x46, 0x42, 0x0D, 0x06, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x24, 0x3D, 0x93, 0x02, 0xD0, 0x08, 0x6D, 0x99, 0xE1, 0xD7, 0x78, 0x41, 0x49, 0xA3, 0xD8, 0x44, 0xED, 0x06, 0x4F, 0x42, 0x83, 0x04, 0x95, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x9D, 0x53, 0x08, 0xC6, 0x32, 0xBD, 0x2A, 0xCF, 0x02, 0x7B, 0x41, 0x45, 0x03, 0x13, 0x44, 0xDA, 0x5A, 0x38, 0x42, 0x64, 0xF1, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x3D, 0x93, 0x01, 0x1A, 0x02, 0x30, 0x84, 0xCF, 0x02, 0x7B, 0x41, 0xAB, 0x89, 0xDB, 0x43, 0x43, 0x3E, 0x3F, 0x42, 0xA6, 0xAB, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x3D, 0x33, 0x02, 0xAA, 0xA4, 0x2B, 0x55, 0xCF, 0x02, 0x7B, 0x41, 0xE4, 0x39, 0xE1, 0x43, 0x22, 0x3F, 0x42, 0x42, 0xCC, 0xA7, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x44, 0x3D, 0x93, 0x02, 0x7C, 0x45, 0xCF, 0x4A, 0xCF, 0x02, 0x7B, 0x41, 0x3E, 0x66, 0xDE, 0x43, 0xFC, 0xD0, 0x4B, 0x42, 0x3D, 0xA5, 0x84, 0x44, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x9D, 0x53, 0x08, 0x54, 0x51, 0xA7, 0xF1, 0x6B, 0x40, 0x79, 0x41, 0x76, 0xC1, 0x25, 0x45, 0x6B, 0x68, 0x3C, 0x42, 0x25, 0xD6, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x3D, 0x93, 0x01, 0x20, 0xC0, 0xC1, 0x35, 0x6C, 0x40, 0x79, 0x41, 0x77, 0x90, 0xF7, 0x44, 0x25, 0xDC, 0x42, 0x42, 0xB4, 0xB0, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x3D, 0x33, 0x02, 0x21, 0xDD, 0xDB, 0xFE, 0x6B, 0x40, 0x79, 0x41, 0xA2, 0x05, 0xFE, 0x44, 0x44, 0x00, 0x43, 0x42, 0xE3, 0xB3, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x64, 0x3D, 0x93, 0x02, 0xA9, 0xF2, 0x81, 0xF4, 0x6B, 0x40, 0x79, 0x41, 0xA0, 0xCB, 0xFA, 0x44, 0xF0, 0x77, 0x4D, 0x42, 0x17, 0xAF, 0x63, 0x45, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x81, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x21, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x9D, 0x53, 0x08, 0x7C, 0x2E, 0x2C, 0xE1, 0x4E, 0xD9, 0x76, 0x41, 0x45, 0xBB, 0xED, 0xC3, 0x6D, 0x5E, 0x58, 0x42, 0x4A, 0x1D, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x3D, 0x93, 0x01, 0xC9, 0xFE, 0x61, 0x1C, 0x4F, 0xD9, 0x76, 0x41, 0x4C, 0x81, 0xB1, 0xC3, 0x20, 0x59, 0x55, 0x42, 0x2C, 0x14, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x3D, 0x33, 0x02, 0x33, 0xD9, 0xF6, 0xE3, 0x4E, 0xD9, 0x76, 0x41, 0x9A, 0x23, 0xB6, 0xC3, 0xBC, 0x23, 0x56, 0x42, 0xF8, 0x14, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x3D, 0x93, 0x02, 0xC1, 0x84, 0x86, 0xE4, 0x4E, 0xD9, 0x76, 0x41, 0xEF, 0xD2, 0xB3, 0xC3, 0x58, 0xE7, 0x60, 0x42, 0xC5, 0x13, 0x63, 0x46, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x81, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x21, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x81, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x21, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x82, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x22, 0x83, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x22, 0x23, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x22, 0x83, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xC1, 0x82, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xC1, 0x82, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x8C, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0xE4, 0x9E, 0x14, 0x08, 0xAD, 0x0A, 0xD0, 0xC0, 0xFB, 0xC7, 0x74, 0x41, 0x8A, 0x33, 0x68, 0xC4, 0x8E, 0x27, 0x50, 0x42, 0x46, 0x4C, 0x66, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0xE4, 0x3E, 0x34, 0x41, 0x17, 0xF3, 0xC8, 0xCA, 0xFB, 0xC7, 0x74, 0x41, 0x6B, 0xFF, 0x2E, 0xC4, 0x4A, 0x30, 0x52, 0x42, 0x15, 0x37, 0x66, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x04, 0x9F, 0x14, 0x48, 0x9D, 0xBD, 0x2C, 0xE7, 0x49, 0x72, 0x83, 0x41, 0x25, 0x9D, 0xAF, 0xC4, 0x62, 0x87, 0x24, 0x42, 0xC7, 0x06, 0x65, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x04, 0x3F, 0x34, 0x41, 0x85, 0xF6, 0x1A, 0xE8, 0x49, 0x72, 0x83, 0x41, 0x1B, 0x5A, 0x84, 0xC4, 0xBA, 0x6D, 0x34, 0x42, 0x91, 0x8E, 0x7C, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x24, 0x9F, 0x14, 0x58, 0x8C, 0x21, 0x04, 0x9F, 0xB6, 0x04, 0x77, 0x41, 0x67, 0x68, 0x30, 0xC5, 0x44, 0x5F, 0x3C, 0x42, 0x4C, 0x6E, 0x85, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x24, 0x3F, 0x34, 0x41, 0xD5, 0x22, 0xAD, 0xFE, 0xB6, 0x04, 0x77, 0x41, 0xDC, 0xF1, 0x04, 0xC5, 0xA4, 0xB4, 0x4A, 0x42, 0xA9, 0x63, 0x85, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x44, 0x9F, 0x14, 0x18, 0xA7, 0x39, 0x8E, 0x53, 0x95, 0x7C, 0x83, 0x41, 0x18, 0xB8, 0x6F, 0xC4, 0xF0, 0xE9, 0x1B, 0x42, 0x3D, 0x5C, 0x47, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x44, 0x9F, 0x34, 0x10, 0x2A, 0x5A, 0xC8, 0x62, 0x95, 0x7C, 0x83, 0x41, 0xAD, 0x63, 0x39, 0xC4, 0x0E, 0x28, 0x1F, 0x42, 0x99, 0x5B, 0x47, 0x46, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x61, 0x83, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x61, 0x83, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x5A, 0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x84, 0x9F, 0x14, 0x48, 0x19, 0x47, 0x83, 0xEA, 0x87, 0x4D, 0x78, 0x41, 0x15, 0xE3, 0x32, 0xC5, 0xE8, 0x59, 0x37, 0x42, 0x4C, 0xB2, 0xA5, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x84, 0x3F, 0x34, 0x41, 0x8B, 0x20, 0x32, 0x29, 0x88, 0x4D, 0x78, 0x41, 0x7C, 0xD0, 0x06, 0xC5, 0x9D, 0x5B, 0x3B, 0x42, 0xA9, 0xA7, 0xA5, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xA1, 0x83, 0x84, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x1C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xA1, 0x83, 0x84, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x1C, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0xC4, 0x9F, 0x14, 0x48, 0xDB, 0x79, 0x55, 0xF9, 0xF3, 0x3D, 0x78, 0x41, 0xD4, 0x1C, 0x25, 0x42, 0x04, 0x74, 0x32, 0x42, 0x32, 0xD9, 0xAB, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A, 0x00, 0x00, 0x00, 0xC4, 0x3F, 0x34, 0x41, 0x6E, 0x51, 0x8B, 0xD1, 0xF4, 0x3D, 0x78, 0x41, 0xBE, 0x10, 0xF9, 0x41, 0x9B, 0xE8, 0x47, 0x42, 0xA2, 0xAE, 0xAB, 0x45, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x00, 0xE4, 0x9F, 0x04, 0x48, 0xA3, 0x7E, 0xC1, 0x6D, 0xD3, 0x80, 0x7D, 0x41, 0xBB, 0xC8, 0x04, 0xC5, 0xFF, 0x27, 0x44, 0x42, 0x16, 0xC5, 0x8D, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x00, 0xE9, 0x23, 0x24, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD9, 0x1C, 0xC8, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x02, 0x80, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xEE, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x24, 0x9C, 0x14, 0x08, 0x4B, 0x98, 0x57, 0x17, 0xF2, 0x73, 0x77, 0x41, 0x77, 0xA5, 0xA2, 0x44, 0x15, 0x67, 0x39, 0x42, 0x91, 0xA8, 0x86, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x24, 0x9C, 0x34, 0x00, 0x20, 0xE5, 0x16, 0xE2, 0xF1, 0x73, 0x77, 0x41, 0x8B, 0x88, 0x7B, 0x44, 0xEA, 0xB2, 0x45, 0x42, 0x49, 0xA7, 0x86, 0x45, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x44, 0x9C, 0x14, 0x48, 0xF5, 0x8C, 0x5D, 0x2F, 0x30, 0x0F, 0x76, 0x41, 0x59, 0x3D, 0xAB, 0x42, 0x2F, 0x33, 0x44, 0x42, 0x99, 0xC4, 0x07, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x44, 0x3C, 0x34, 0x41, 0xD7, 0x57, 0x80, 0x97, 0x30, 0x0F, 0x76, 0x41, 0xAE, 0x1D, 0x81, 0x42, 0x85, 0xBE, 0x50, 0x42, 0x99, 0xAE, 0x07, 0x46, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x61, 0x80, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x84, 0x9C, 0x14, 0x48, 0x4E, 0xB0, 0x00, 0x1E, 0x84, 0x37, 0x78, 0x41, 0x3B, 0xBC, 0x1B, 0x45, 0xF9, 0x41, 0x2F, 0x42, 0x1C, 0x51, 0x2E, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x84, 0x3C, 0x34, 0x41, 0x4C, 0xD6, 0xFF, 0xD2, 0x85, 0x37, 0x78, 0x41, 0xE0, 0xB9, 0xEA, 0x44, 0x9C, 0x14, 0x49, 0x42, 0x02, 0xD4, 0x0C, 0x45, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0xA4, 0x9C, 0x14, 0x18, 0x50, 0x86, 0x78, 0x9B, 0x2C, 0x80, 0x75, 0x41, 0xEA, 0x55, 0xF0, 0x44, 0x9E, 0x19, 0x4C, 0x42, 0x8E, 0x38, 0xE7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0xA4, 0x3C, 0x34, 0x41, 0xEB, 0xF9, 0x57, 0xB1, 0x2C, 0x80, 0x75, 0x41, 0xFA, 0x1D, 0xB5, 0x44, 0xE5, 0x7D, 0x49, 0x42, 0xA4, 0x0E, 0xE7, 0x45, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0xC1, 0x80, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x9C, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0xE4, 0x9C, 0x14, 0x48, 0x0B, 0xE0, 0xB0, 0xF3, 0xB2, 0xE8, 0x78, 0x41, 0x3A, 0x23, 0x21, 0x45, 0xF3, 0xB4, 0x23, 0x42, 0x3A, 0xE2, 0xAE, 0x44, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0xE4, 0x3C, 0x34, 0x41, 0x7D, 0xDC, 0xAD, 0x91, 0xB3, 0xE8, 0x78, 0x41, 0xE1, 0xD7, 0xF2, 0x44, 0xC0, 0x84, 0x36, 0x42, 0x8C, 0x3A, 0xAE, 0x44, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x9D, 0x14, 0x08, 0xAD, 0xB8, 0xFA, 0xC5, 0xD4, 0x3D, 0x83, 0x41, 0xDD, 0x55, 0x4B, 0xC4, 0x24, 0x36, 0xF4, 0x41, 0xA8, 0x25, 0x0B, 0x46, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x9D, 0x34, 0x10, 0x68, 0xCE, 0xFF, 0xB5, 0xD4, 0x3D, 0x83, 0x41, 0x56, 0x29, 0x1D, 0xC4, 0x4D, 0xCD, 0x2E, 0x42, 0x9C, 0x53, 0x2C, 0x46, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x21, 0x81, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x22, 0x81, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x93, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x44, 0x9D, 0x14, 0x08, 0x07, 0x28, 0xAA, 0x76, 0x88, 0x80, 0x83, 0x41, 0xDA, 0xE0, 0x5E, 0xC2, 0xB0, 0xD3, 0x16, 0x42, 0xF2, 0x95, 0x0F, 0x45, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x44, 0x9D, 0x34, 0x10, 0x8E, 0xB1, 0xBC, 0x64, 0x88, 0x80, 0x83, 0x41, 0x73, 0xC2, 0x2B, 0xC2, 0xCD, 0x19, 0x20, 0x42, 0x35, 0x87, 0xB5, 0x45, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x81, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x61, 0x81, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xB6, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x84, 0x09, 0x67, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0xBF, 0x9C, 0xC3, 0x0E, 0x3A, 0x14, 0x42, 0x59, 0x1F, 0xBF, 0x49, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA4, 0x09, 0x67, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD3, 0x35, 0x80, 0x42, 0x4D, 0x11, 0x2C, 0x42, 0xDA, 0x6E, 0x23, 0x49, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x01, 0x67, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xE1, 0x01, 0x67, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB4, 0xF3, 0x8A, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x67, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0xDB, 0x04, 0x50 };
   uint8_t aucLog2EncodeBuffer[MAX_BINARY_MESSAGE_LENGTH];
   MessageDataStruct stMessageData2;
   MetaDataStruct stMetaData2;
   ASSERT_EQ(DecodeEncodeTest::SUCCESS, DecodeEncode(ENCODEFORMAT::FLATTENED_BINARY, aucLog2, aucLog2EncodeBuffer, sizeof(aucLog2EncodeBuffer), stMetaData2, stMessageData2));

   ASSERT_HEADER_EQ(stMessageData1.pucMessageHeader, stMessageData2.pucMessageHeader);
   ASSERT_TRACKSTAT_EQ(stMessageData1.pucMessageBody, stMessageData2.pucMessageBody);
}

// -------------------------------------------------------------------------------------------------------
// Command Encoding Unit Tests
// -------------------------------------------------------------------------------------------------------
class CommandEncodeTest : public ::testing::Test
{

protected:
   static JsonReader* pclMyJsonDb;
   static Commander* pclMyCommander;

   // Per-test-suite setup
   static void SetUpTestSuite()
   {
      try
      {
         pclMyJsonDb = new JsonReader();
         pclMyJsonDb->LoadFile(*TEST_DB_PATH);
         pclMyCommander = new Commander(pclMyJsonDb);
      }
      catch (JsonReaderFailure& e)
      {
         printf("%s\n", e.what());

         if (pclMyJsonDb)
         {
            delete pclMyJsonDb;
            pclMyJsonDb = nullptr;
         }

         if (pclMyCommander)
         {
            delete pclMyCommander;
            pclMyCommander = nullptr;
         }
      }
   }

   // Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if (pclMyJsonDb)
      {
         delete pclMyJsonDb;
         pclMyJsonDb = nullptr;
      }

      if (pclMyCommander)
      {
         pclMyCommander->ShutdownLogger();
         delete pclMyCommander;
         pclMyCommander = nullptr;
      }
   }

public:

   STATUS TestCommandConversion(char* pcCommandToEncode_, uint32_t uiCommandToEncodeLength_, char* pcEncodedCommandBuffer_, uint32_t uiEncodedCommandBufferSize_, ENCODEFORMAT eFormat_)
   {
      return pclMyCommander->Encode(pcCommandToEncode_, uiCommandToEncodeLength_, pcEncodedCommandBuffer_, uiEncodedCommandBufferSize_, eFormat_);
   }

private:
};
JsonReader* CommandEncodeTest::pclMyJsonDb = nullptr;
Commander* CommandEncodeTest::pclMyCommander = nullptr;

// -------------------------------------------------------------------------------------------------------
// Logger Command Encoding Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(CommandEncodeTest, LOGGER)
{
   spdlog::level::level_enum eLevel = spdlog::level::off;

   ASSERT_NE(spdlog::get("novatel_commander"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_commander = pclMyCommander->GetLogger();
   pclMyCommander->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_commander->level(), eLevel);
}

// -------------------------------------------------------------------------------------------------------
// ASCII Command Encoding Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(CommandEncodeTest, COMMAND_ENCODE_ASCII_CONFIGCODE)
{
   char aucExpectedCommand[] = "#CONFIGCODEA,THISPORT,0,0.0,UNKNOWN,0,0.000,00000000,dbc9,0;ERASE_TABLE,\"WJ4HDW\",\"GM5Z99\",\"T2M7DP\",\"KG2T8T\",\"KF7GKR\",\"TABLECLEAR\"*69419dec\r\n";
   char aucCommandToEncode[] = "CONFIGCODE ERASE_TABLE \"WJ4HDW\" \"GM5Z99\" \"T2M7DP\" \"KG2T8T\" \"KF7GKR\" \"TABLECLEAR\"";
   char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];

   ASSERT_EQ(STATUS::SUCCESS, TestCommandConversion(aucCommandToEncode, sizeof(aucCommandToEncode), acEncodeBuffer, sizeof(acEncodeBuffer), ENCODEFORMAT::ASCII));
   ASSERT_EQ(0, memcmp(acEncodeBuffer, aucExpectedCommand, sizeof(aucExpectedCommand)));
}

TEST_F(CommandEncodeTest, COMMAND_ENCODE_ASCII_INSTHRESHOLDS)
{
   char aucExpectedCommand[] = "#INSTHRESHOLDSA,THISPORT,0,0.0,UNKNOWN,0,0.000,00000000,48a5,0;LOW,0.000000000,0.000000000,0.000000000*3989c2ac\r\n";
   char aucCommandToEncode[] = "INSTHRESHOLDS LOW 0.0 0.0 0.0";
   char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];

   ASSERT_EQ(STATUS::SUCCESS, TestCommandConversion(aucCommandToEncode, sizeof(aucCommandToEncode), acEncodeBuffer, sizeof(acEncodeBuffer), ENCODEFORMAT::ASCII));
   ASSERT_EQ(0, memcmp(acEncodeBuffer, aucExpectedCommand, sizeof(aucExpectedCommand)));
}

// -------------------------------------------------------------------------------------------------------
// Binary Command Encoding Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(CommandEncodeTest, COMMAND_ENCODE_BINARY_CONFIGCODE)
{
   unsigned char aucExpectedCommand[] = { 0xAA, 0x44, 0x12, 0x1C, 0x11, 0x04, 0x00, 0xC0, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC9, 0xDB, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x57, 0x4A, 0x34, 0x48, 0x44, 0x57, 0x00, 0x00, 0x47, 0x4D, 0x35, 0x5A, 0x39, 0x39, 0x00, 0x00, 0x54, 0x32, 0x4D, 0x37, 0x44, 0x50, 0x00, 0x00, 0x4B, 0x47, 0x32, 0x54, 0x38, 0x54, 0x00, 0x00, 0x4B, 0x46, 0x37, 0x47, 0x4B, 0x52, 0x00, 0x00, 0x54, 0x41, 0x42, 0x4C, 0x45, 0x43, 0x4C, 0x45, 0x41, 0x52, 0x00, 0x00, 0x06, 0xF3, 0x54, 0x45 };
   char aucCommandToEncode[] = "CONFIGCODE ERASE_TABLE \"WJ4HDW\" \"GM5Z99\" \"T2M7DP\" \"KG2T8T\" \"KF7GKR\" \"TABLECLEAR\"";
   char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];

   ASSERT_EQ(STATUS::SUCCESS, TestCommandConversion(aucCommandToEncode, sizeof(aucCommandToEncode), acEncodeBuffer, sizeof(acEncodeBuffer), ENCODEFORMAT::BINARY));
   ASSERT_EQ(0, memcmp(acEncodeBuffer, aucExpectedCommand, sizeof(aucExpectedCommand)));
}

TEST_F(CommandEncodeTest, COMMAND_ENCODE_BINARY_LOG_PARTIAL)
{
   char aucCommandToEncode[] = "LOG THISPORT BESTPOSA ONCE";
   char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];

   ASSERT_EQ(STATUS::MALFORMED_INPUT, TestCommandConversion(aucCommandToEncode, sizeof(aucCommandToEncode), acEncodeBuffer, sizeof(acEncodeBuffer), ENCODEFORMAT::BINARY));
}

TEST_F(CommandEncodeTest, COMMAND_ENCODE_BINARY_UALCONTROL)
{
   unsigned char aucExpectedCommand[] = { 0xAA, 0x44, 0x12, 0x1C, 0x5B, 0x06, 0x00, 0xC0, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x49, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0xFF, 0xF8, 0x3A, 0xA7 };
   char aucCommandToEncode[] = "UALCONTROL ENABLE 2.0 1.0";
   char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];

   ASSERT_EQ(STATUS::SUCCESS, TestCommandConversion(aucCommandToEncode, sizeof(aucCommandToEncode), acEncodeBuffer, sizeof(acEncodeBuffer), ENCODEFORMAT::BINARY));
   ASSERT_EQ(0, memcmp(acEncodeBuffer, aucExpectedCommand, sizeof(aucExpectedCommand)));
}


// -------------------------------------------------------------------------------------------------------
// Decode/Encode Benchmark Unit Tests
// -------------------------------------------------------------------------------------------------------
class BenchmarkTest : public ::testing::Test
{

protected:
   static JsonReader* pclMyJsonDb;
   static HeaderDecoder* pclMyHeaderDecoder;
   static MessageDecoder* pclMyMessageDecoder;
   static Encoder* pclMyEncoder;
   static constexpr unsigned int uiMaxCount = 1000;

   // Per-test-suite setup
   static void SetUpTestSuite()
   {
      try
      {
         pclMyJsonDb = new JsonReader();
         pclMyJsonDb->LoadFile(*TEST_DB_PATH);
         pclMyHeaderDecoder = new HeaderDecoder(pclMyJsonDb);
         pclMyMessageDecoder = new MessageDecoder(pclMyJsonDb);
         pclMyEncoder = new Encoder(pclMyJsonDb);
      }
      catch (JsonReaderFailure& e)
      {
         printf("%s\n", e.what());

         if (pclMyJsonDb)
         {
            delete pclMyJsonDb;
            pclMyJsonDb = nullptr;
         }

         if (pclMyHeaderDecoder)
         {
            delete pclMyHeaderDecoder;
            pclMyHeaderDecoder = nullptr;
         }

         if (pclMyMessageDecoder)
         {
            delete pclMyMessageDecoder;
            pclMyMessageDecoder = nullptr;
         }

         if (pclMyEncoder)
         {
            delete pclMyEncoder;
            pclMyEncoder = nullptr;
         }
      }
   }

   // Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if (pclMyJsonDb)
      {
         delete pclMyJsonDb;
         pclMyJsonDb = nullptr;
      }

      if (pclMyHeaderDecoder)
      {
         delete pclMyHeaderDecoder;
         pclMyHeaderDecoder = nullptr;
      }

      if (pclMyMessageDecoder)
      {
         delete pclMyMessageDecoder;
         pclMyMessageDecoder = nullptr;
      }

      if (pclMyEncoder)
      {
         delete pclMyEncoder;
         pclMyEncoder = nullptr;
      }
   }
};
JsonReader* BenchmarkTest::pclMyJsonDb = nullptr;
HeaderDecoder* BenchmarkTest::pclMyHeaderDecoder = nullptr;
MessageDecoder* BenchmarkTest::pclMyMessageDecoder = nullptr;
Encoder* BenchmarkTest::pclMyEncoder = nullptr;

TEST_F(BenchmarkTest, BENCHMARK_BINARY_TO_BINARY_BESTPOS)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34, 0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0, 0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E, 0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9 };

   unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = aucEncodeBuffer;

   IntermediateHeader stHeader;
   IntermediateMessage stMessage;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   unsigned char* pucLogPtr = nullptr;
   bool bFailedOnce = false;
   uint32_t uiCount = 0;

   auto start = std::chrono::system_clock::now();
   while (uiCount < uiMaxCount)
   {
      pucLogPtr = aucLog;
      if (STATUS::SUCCESS != pclMyHeaderDecoder->Decode(pucLogPtr, stHeader, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      pucLogPtr += stMetaData.uiHeaderLength;
      if (STATUS::SUCCESS != pclMyMessageDecoder->Decode(pucLogPtr, stMessage, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      if (STATUS::SUCCESS != pclMyEncoder->Encode(&pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData, stMetaData, ENCODEFORMAT::BINARY))
      {
         bFailedOnce = true;
         break;
      }
      uiCount++;
   }
   std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
   printf("TIME ELAPSED: %lf seconds.\nLPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(uiCount) / elapsed_seconds.count()));
   ASSERT_FALSE(bFailedOnce);
}

TEST_F(BenchmarkTest, BENCHMARK_ASCII_TO_ASCII_BESTPOS)
{
   unsigned char aucLog[] = "#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n";

   unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = aucEncodeBuffer;

   IntermediateHeader stHeader;
   IntermediateMessage stMessage;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   unsigned char* pucLogPtr = nullptr;
   bool bFailedOnce = false;
   uint32_t uiCount = 0;

   auto start = std::chrono::system_clock::now();
   while (uiCount < uiMaxCount)
   {
      pucLogPtr = aucLog;
      if (STATUS::SUCCESS != pclMyHeaderDecoder->Decode(pucLogPtr, stHeader, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      pucLogPtr += stMetaData.uiHeaderLength;
      if (STATUS::SUCCESS != pclMyMessageDecoder->Decode(pucLogPtr, stMessage, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      if (STATUS::SUCCESS != pclMyEncoder->Encode(&pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData, stMetaData, ENCODEFORMAT::ASCII))
      {
         bFailedOnce = true;
         break;
      }
      uiCount++;
   }
   std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
   printf("TIME ELAPSED: %lf seconds.\nLPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(uiCount) / elapsed_seconds.count()));
   ASSERT_FALSE(bFailedOnce);
}

TEST_F(BenchmarkTest, BENCHMARK_ASCII_TO_BINARY_BESTPOS)
{
   unsigned char aucLog[] = "#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n";

   unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = aucEncodeBuffer;

   IntermediateHeader stHeader;
   IntermediateMessage stMessage;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   unsigned char* pucLogPtr = nullptr;
   bool bFailedOnce = false;
   uint32_t uiCount = 0;

   auto start = std::chrono::system_clock::now();
   while (uiCount < uiMaxCount)
   {
      pucLogPtr = aucLog;
      if (STATUS::SUCCESS != pclMyHeaderDecoder->Decode(pucLogPtr, stHeader, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      pucLogPtr += stMetaData.uiHeaderLength;
      if (STATUS::SUCCESS != pclMyMessageDecoder->Decode(pucLogPtr, stMessage, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      if (STATUS::SUCCESS != pclMyEncoder->Encode(&pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData, stMetaData, ENCODEFORMAT::BINARY))
      {
         bFailedOnce = true;
         break;
      }
      uiCount++;
   }
   std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
   printf("TIME ELAPSED: %lf seconds.\nLPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(uiCount) / elapsed_seconds.count()));
   ASSERT_FALSE(bFailedOnce);
}

TEST_F(BenchmarkTest, BENCHMARK_BINARY_TO_ASCII_BESTPOS)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34, 0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0, 0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E, 0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9 };

   unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = aucEncodeBuffer;

   IntermediateHeader stHeader;
   IntermediateMessage stMessage;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   unsigned char* pucLogPtr = nullptr;
   bool bFailedOnce = false;
   uint32_t uiCount = 0;

   auto start = std::chrono::system_clock::now();
   while (uiCount < uiMaxCount)
   {
      pucLogPtr = aucLog;
      if (STATUS::SUCCESS != pclMyHeaderDecoder->Decode(pucLogPtr, stHeader, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      pucLogPtr += stMetaData.uiHeaderLength;
      if (STATUS::SUCCESS != pclMyMessageDecoder->Decode(pucLogPtr, stMessage, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      if (STATUS::SUCCESS != pclMyEncoder->Encode(&pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData, stMetaData, ENCODEFORMAT::ASCII))
      {
         bFailedOnce = true;
         break;
      }
      uiCount++;
   }
   std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
   printf("TIME ELAPSED: %lf seconds.\nLPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(uiCount) / elapsed_seconds.count()));
   ASSERT_FALSE(bFailedOnce);
}

TEST_F(BenchmarkTest, BENCHMARK_ASCII_TO_FLAT_BINARY_BESTPOS)
{
   unsigned char aucLog[] = "#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n";

   unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = aucEncodeBuffer;

   IntermediateHeader stHeader;
   IntermediateMessage stMessage;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   unsigned char* pucLogPtr = nullptr;
   bool bFailedOnce = false;
   uint32_t uiCount = 0;

   auto start = std::chrono::system_clock::now();
   while (uiCount < uiMaxCount)
   {
      pucLogPtr = aucLog;
      if (STATUS::SUCCESS != pclMyHeaderDecoder->Decode(pucLogPtr, stHeader, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      pucLogPtr += stMetaData.uiHeaderLength;
      if (STATUS::SUCCESS != pclMyMessageDecoder->Decode(pucLogPtr, stMessage, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      if (STATUS::SUCCESS != pclMyEncoder->Encode(&pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData, stMetaData, ENCODEFORMAT::FLATTENED_BINARY))
      {
         bFailedOnce = true;
         break;
      }
      uiCount++;
   }
   std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
   printf("TIME ELAPSED: %lf seconds.\nLPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(uiCount) / elapsed_seconds.count()));
   ASSERT_FALSE(bFailedOnce);
}

TEST_F(BenchmarkTest, BENCHMARK_BINARY_TO_FLAT_BINARY_BESTPOS)
{
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34, 0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0, 0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E, 0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9 };

   unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = aucEncodeBuffer;

   IntermediateHeader stHeader;
   IntermediateMessage stMessage;

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   unsigned char* pucLogPtr = nullptr;
   bool bFailedOnce = false;
   uint32_t uiCount = 0;

   auto start = std::chrono::system_clock::now();
   while (uiCount < uiMaxCount)
   {
      pucLogPtr = aucLog;
      if (STATUS::SUCCESS != pclMyHeaderDecoder->Decode(pucLogPtr, stHeader, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      pucLogPtr += stMetaData.uiHeaderLength;
      if (STATUS::SUCCESS != pclMyMessageDecoder->Decode(pucLogPtr, stMessage, stMetaData))
      {
         bFailedOnce = true;
         break;
      }

      if (STATUS::SUCCESS != pclMyEncoder->Encode(&pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData, stMetaData, ENCODEFORMAT::FLATTENED_BINARY))
      {
         bFailedOnce = true;
         break;
      }
      uiCount++;
   }
   std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
   printf("TIME ELAPSED: %lf seconds.\nLPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(uiCount) / elapsed_seconds.count()));
   ASSERT_FALSE(bFailedOnce);
}

// -------------------------------------------------------------------------------------------------------
// Filter Unit Tests
// -------------------------------------------------------------------------------------------------------
class FilterTest : public ::testing::Test
{

protected:
   static JsonReader* pclMyJsonDb;
   static HeaderDecoder* pclMyHeaderDecoder;
   static Filter* pclMyFilter;

   // Per-test-suite setup
   static void SetUpTestSuite()
   {
      try
      {
         pclMyJsonDb = new JsonReader();
         pclMyJsonDb->LoadFile(*TEST_DB_PATH);
         pclMyHeaderDecoder = new HeaderDecoder(pclMyJsonDb);
         pclMyFilter = new Filter();
      }
      catch (JsonReaderFailure& e)
      {
         printf("%s\n", e.what());

         if (pclMyJsonDb)
         {
            delete pclMyJsonDb;
            pclMyJsonDb = nullptr;
         }

         if (pclMyHeaderDecoder)
         {
            delete pclMyHeaderDecoder;
            pclMyHeaderDecoder = nullptr;
         }

         if (pclMyFilter)
         {
            delete pclMyFilter;
            pclMyFilter = nullptr;
         }
      }
   }

   // Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if (pclMyJsonDb)
      {
         delete pclMyJsonDb;
         pclMyJsonDb = nullptr;
      }

      if (pclMyHeaderDecoder)
      {
         delete pclMyHeaderDecoder;
         pclMyHeaderDecoder = nullptr;
      }

      if (pclMyFilter)
      {
         pclMyFilter->ShutdownLogger();
         delete pclMyFilter;
         pclMyFilter = nullptr;
      }
   }

   void SetUp() override
   {
      pclMyFilter->ClearFilters();
   }

public:
   bool TestFilter(unsigned char* pucMessage_)
   {
      MetaDataStruct stMetaData;
      IntermediateHeader stHeader;
      const STATUS eStatus = pclMyHeaderDecoder->Decode(pucMessage_, stHeader, stMetaData);
      if (STATUS::SUCCESS != eStatus && STATUS::UNSUPPORTED != eStatus)
      {
         printf("HeaderDecoder Failed!\n");
         return false;
      }

      return pclMyFilter->DoFiltering(stMetaData);
   }
};
JsonReader* FilterTest::pclMyJsonDb = nullptr;
HeaderDecoder* FilterTest::pclMyHeaderDecoder = nullptr;
Filter* FilterTest::pclMyFilter = nullptr;

TEST_F(FilterTest, LOGGER)
{
   spdlog::level::level_enum eLevel = spdlog::level::off;

   ASSERT_NE(spdlog::get("novatel_filter"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_filter = pclMyFilter->GetLogger();
   pclMyFilter->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_filter->level(), eLevel);
}

TEST_F(FilterTest, NONE)
{
   const char* log = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313698.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log))));
}

TEST_F(FilterTest, TIME_RANGE)
{
   const char* early_bestpos_log = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324433.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043040645,-114.03067196833,1097.5312,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*df69e14a\r\n";
   const char* bestpos_log1      = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324434.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log2      = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log3      = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324436.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log4      = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324437.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* late_bestpos_log  = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324438.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043044465,-114.03067191095,1097.5169,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*668e4c66\r\n";

   pclMyFilter->SetIncludeLowerTimeBound(2180, 324434);
   pclMyFilter->SetIncludeUpperTimeBound(2180, 324437);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(early_bestpos_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log1))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log2))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log3))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log4))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(late_bestpos_log))));

   pclMyFilter->InvertTimeFilter(true);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(early_bestpos_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log3))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log4))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(late_bestpos_log))));
}

TEST_F(FilterTest, ABBREV_ASCII_NAME_FILTER)
{
   const char* bestpos_log = "<BESTPOS USB3 0 47.0 FINESTEERING 2236 97396.000 03000000 cdba 16809\r\n<     SOL_COMPUTED WAAS 17.44306670742 78.37412051449 649.8287 -76.8000 WGS84 1.1040 1.1218 2.5477 \"128\" 5.000 0.000 61 11 11 11 00 06 00 03\r\n";
   const char* bestvel_log = "<BESTVEL USB3 0 47.0 FINESTEERING 2236 97396.000 03000000 10a2 16809\r\n<     SOL_COMPUTED WAAS 0.000 5.000 0.0154 23.182594 0.0384 0\r\n";

   pclMyFilter->IncludeMessageName("BESTPOS");
   pclMyFilter->InvertMessageNameFilter(true);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestvel_log))));
}

TEST_F(FilterTest, SHORT_ABBREV_ASCII_NAME_FILTER)
{
   const char* corrimus_log = "<CORRIMUS 2221 285950.000\r\n<     37 0.0000000000000000 0.0000000000000000 0.0000000000000000 0.0000000000000000 0.0000000000000000 0.0000000000000000 8.066 0\r\n";
   const char* insatts_log  = "<INSATTS 2221 285963.000\r\n<     2221 285963.000000000 -0.980733135 0.769729364 -0.000000000 WAITING_AZIMUTH\r\n";

   pclMyFilter->IncludeMessageName("INSATTS");

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(corrimus_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(insatts_log))));
}

TEST_F(FilterTest, FINE_TIME_STATUS)
{
   const char* coarseadjusting_log = "#BESTPOSA,COM1,0,8.5,COARSEADJUSTING,2180,313691.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15045261052,-114.03068986465,1097.3728,-17.0000,WGS84,1.5831,1.2508,3.3114,\"\",0.000,0.000,23,19,19,0,00,02,11,11*7473f577\r\n";
   const char* coarsesteering_log  = "#BESTPOSA,COM1,0,8.5,COARSESTEERING,2180,313692.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15045261052,-114.03068986465,1097.3728,-17.0000,WGS84,1.5831,1.2508,3.3114,\"\",0.000,0.000,23,19,19,0,00,02,11,11*7473f577\r\n";
   const char* fineadjusting_log   = "#BESTPOSA,COM1,0,8.5,FINEADJUSTING,2180,313693.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15045261052,-114.03068986465,1097.3728,-17.0000,WGS84,1.5831,1.2508,3.3114,\"\",0.000,0.000,23,19,19,0,00,02,11,11*7473f577\r\n";
   const char* finesteering_log    = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313694.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";

   pclMyFilter->IncludeTimeStatus(TIME_STATUS::FINEADJUSTING);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(coarseadjusting_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(coarsesteering_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(fineadjusting_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(finesteering_log))));

   pclMyFilter->InvertTimeStatusFilter(true);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(coarseadjusting_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(coarsesteering_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(fineadjusting_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(finesteering_log))));
}

TEST_F(FilterTest, MULTIPLE_TIME_STATUS)
{
   const char* coarseadjusting_log = "#BESTPOSA,COM1,0,8.5,COARSEADJUSTING,2180,313691.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15045261052,-114.03068986465,1097.3728,-17.0000,WGS84,1.5831,1.2508,3.3114,\"\",0.000,0.000,23,19,19,0,00,02,11,11*7473f577\r\n";
   const char* coarsesteering_log  = "#BESTPOSA,COM1,0,8.5,COARSESTEERING,2180,313692.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15045261052,-114.03068986465,1097.3728,-17.0000,WGS84,1.5831,1.2508,3.3114,\"\",0.000,0.000,23,19,19,0,00,02,11,11*7473f577\r\n";
   const char* fineadjusting_log   = "#BESTPOSA,COM1,0,8.5,FINEADJUSTING,2180,313693.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15045261052,-114.03068986465,1097.3728,-17.0000,WGS84,1.5831,1.2508,3.3114,\"\",0.000,0.000,23,19,19,0,00,02,11,11*7473f577\r\n";
   const char* finesteering_log    = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313694.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   std::vector<TIME_STATUS> time_statuses = { TIME_STATUS::COARSEADJUSTING, TIME_STATUS::COARSESTEERING };

   pclMyFilter->IncludeTimeStatus(time_statuses);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(coarseadjusting_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(coarsesteering_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(fineadjusting_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(finesteering_log))));
}

TEST_F(FilterTest, START_TIME)
{
   const char* early_bestpos_log1 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324433.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043040645,-114.03067196833,1097.5312,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*df69e14a\r\n";
   const char* early_bestpos_log2 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324434.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043040645,-114.03067196833,1097.5312,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*df69e14a\r\n";
   const char* bestpos_log1       = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log2       = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324436.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";

   pclMyFilter->SetIncludeLowerTimeBound(2180, 324435);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(early_bestpos_log1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(early_bestpos_log2))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log1))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log2))));
}

TEST_F(FilterTest, STOP_TIME)
{
   const char* bestpos_log1      = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324434.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043040645,-114.03067196833,1097.5312,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*df69e14a\r\n";
   const char* bestpos_log2      = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043040645,-114.03067196833,1097.5312,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*df69e14a\r\n";
   const char* late_bestpos_log1 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324436.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* late_bestpos_log2 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324437.000,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";

   pclMyFilter->SetIncludeUpperTimeBound(2180, 324435);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log1))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(late_bestpos_log1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(late_bestpos_log2))));
}

TEST_F(FilterTest, MESSAGE_FORMAT)
{
   const char* ascii_bestpos_log = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313698.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* shortascii_rawimus_log = "%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n";
   unsigned char binary_bestpos_log[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA4, 0xB4, 0xAC, 0x07, 0xD8, 0x16, 0x6D, 0x08, 0x08, 0x40, 0x00, 0x02, 0xF6, 0xB1, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xD7, 0x03, 0xB0, 0x4C, 0xE5, 0x8E, 0x49, 0x40, 0x52, 0xC4, 0x26, 0xD1, 0x72, 0x82, 0x5C, 0xC0, 0x29, 0xCB, 0x10, 0xC7, 0x7A, 0xA2, 0x90, 0x40, 0x33, 0x33, 0x87, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xFA, 0x7E, 0xBA, 0x3F, 0x3F, 0x57, 0x83, 0x3F, 0xA9, 0xA4, 0x0A, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x16, 0x16, 0x16, 0x00, 0x06, 0x39, 0x33, 0x23, 0xC4, 0x89, 0x7A };
   unsigned char shortbinary_rawimu_log[] = { 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA };

   std::vector<std::tuple<std::string, HEADERFORMAT, MEASUREMENT_SOURCE>> filterlist = {
      {"BESTPOS", HEADERFORMAT::ASCII, MEASUREMENT_SOURCE::PRIMARY},
      {"RAWIMUSX", HEADERFORMAT::SHORT_ASCII, MEASUREMENT_SOURCE::PRIMARY}
   };
   pclMyFilter->IncludeMessageName(filterlist);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(ascii_bestpos_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(shortascii_rawimus_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(binary_bestpos_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(shortbinary_rawimu_log))));
}

TEST_F(FilterTest, MESSAGE_ID)
{
   const char* bestpos_log    = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313698.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* version_log    = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   const char* range_log      = "#RANGEA,COM1,0,49.0,FINESTEERING,2167,159740.000,02000000,5103,16248;115,20,0,23291390.821,0.051,-122397109.320305,0.011,-3214.414,44.0,16498.975,1810bc04,20,0,23291389.349,0.184,-95374376.486282,0.013,-2504.740,41.1,16492.754,11303c0b,29,0,20705108.295,0.022,-108806118.750397,0.005,-1620.698,51.2,14908.774,0810bc44,29,0,20705106.528,0.061,-84783996.294254,0.006,-1262.882,50.7,14902.754,01303c4b,29,0,20705107.069,0.025,-84783990.539554,0.005,-1262.882,51.3,14905.534,02309c4b,13,0,22941454.717,0.042,-120558183.531157,0.009,2111.590,45.7,3984.555,1810bc64,13,0,22941453.693,0.180,-93941456.741696,0.011,1645.396,41.2,3978.255,11303c6b,15,0,22752775.227,0.046,-119566687.005913,0.012,3112.730,44.8,3628.275,0810bc84,15,0,22752775.420,0.118,-93168858.308908,0.012,2425.506,44.9,3622.754,01303c8b,15,0,22752775.932,0.043,-93168851.550483,0.012,2425.505,46.7,3625.014,02309c8b,16,0,23437165.563,0.058,-123163154.592686,0.010,1667.743,42.8,3318.855,1810bca4,16,0,23437164.832,0.179,-95971289.711375,0.012,1299.540,41.3,3313.415,11303cab,18,0,20870226.835,0.024,-109673843.056839,0.005,1782.527,50.4,7988.975,1810bcc4,18,0,20870225.520,0.080,-85460161.951054,0.006,1388.983,48.3,7983.255,11303ccb,18,0,20870225.982,0.024,-85460166.200662,0.006,1388.983,51.9,7985.814,02309ccb,18,0,20870231.341,0.004,-81899348.012827,0.003,1331.096,55.4,7987.255,01d03cc4,5,0,20977730.840,0.024,-110238771.341810,0.006,-1850.683,50.6,12588.896,0810bd04,5,0,20977729.403,0.070,-85900356.288455,0.006,-1442.091,49.4,12583.755,01303d0b,5,0,20977730.127,0.030,-85900355.538492,0.006,-1442.091,49.8,12585.716,02309d0b,26,0,22753733.201,0.042,-119571694.561007,0.008,-494.798,45.7,7318.775,1810bd24,26,0,22753735.163,0.118,-93172767.333088,0.010,-385.557,44.9,7312.755,11303d2b,26,0,22753735.387,0.043,-93172769.582418,0.009,-385.557,46.7,7315.375,02309d2b,26,0,22753735.481,0.008,-89290576.088766,0.005,-369.472,50.1,7316.815,01d03d24,23,0,23067782.934,0.040,-121222050.181679,0.009,3453.274,46.0,3078.894,0810bd44,23,0,23067783.254,0.142,-94458759.273215,0.010,2690.865,43.3,3073.754,01303d4b,23,0,23067783.763,0.040,-94458764.522108,0.009,2690.865,47.3,3076.395,02309d4b,23,0,23067789.450,0.007,-90523004.360543,0.004,2578.883,51.2,3077.834,01d03d44,194,0,43027813.095,0.065,-226112681.899748,0.013,43.499,41.9,17178.695,1815be04,194,0,43027815.196,0.059,-176191709.875251,0.014,33.896,44.0,17173.014,02359e0b,194,0,43027817.865,0.014,-168850394.176443,0.007,32.406,45.4,17177.053,01d53e04,131,0,38480107.260,0.124,-202214296.438902,0.009,-0.922,46.2,292335.531,48023e84,133,0,38618703.555,0.119,-202942631.161186,0.007,0.421,46.7,916697.188,58023ec4,138,0,38495561.597,0.116,-202295515.714333,0.008,-4.752,46.8,292343.625,48023ee4,45,13,20655319.254,0.111,-110608334.938276,0.006,-1928.119,46.3,9728.839,18119f04,45,13,20655320.731,0.021,-86028727.119001,0.006,-1499.649,45.9,9724.239,10b13f0b,45,13,20655321.099,0.092,-86028721.367030,0.006,-1499.649,46.1,9725.238,10319f0b,53,6,23361335.550,0.284,-124792043.406215,0.017,1741.893,38.1,444.840,08119f24,53,6,23361340.271,0.098,-97060514.793435,0.017,1354.807,32.6,444.741,00b13f2b,53,6,23361339.423,0.393,-97060517.036654,0.018,1354.806,33.5,444.801,10319f2b,60,10,20724752.466,0.106,-110863493.957380,0.007,-2492.451,46.7,16549.037,18019f44,39,3,23534282.253,0.169,-125583452.109842,0.012,4608.280,42.6,557.668,08119f84,39,3,23534291.023,0.027,-97676038.550992,0.013,3584.223,43.8,552.119,10b13f8b,39,3,23534290.639,0.108,-97676048.806539,0.013,3584.223,44.7,552.959,10319f8b,61,9,19285134.504,0.086,-103126338.171372,0.005,228.766,48.6,11128.199,08119fa4,61,9,19285138.043,0.020,-80209402.132964,0.005,177.929,46.3,11124.118,00b13fab,61,9,19285138.376,0.084,-80209411.390794,0.005,177.929,46.9,11125.037,00319fab,52,7,22348227.548,0.137,-119422164.171132,0.008,-1798.230,44.4,7458.668,08119fc4,52,7,22348232.044,0.025,-92883929.564420,0.008,-1398.625,44.4,7453.898,00b13fcb,52,7,22348232.124,0.104,-92883930.822797,0.008,-1398.624,45.0,7455.038,10319fcb,54,11,21518220.426,0.169,-115148393.440041,0.010,3262.249,42.6,3877.098,18119fe4,54,11,21518225.678,0.025,-89559888.534930,0.010,2537.306,44.6,3871.818,00b13feb,54,11,21518226.376,0.107,-89559882.794247,0.010,2537.307,44.8,3872.818,10319feb,51,0,23917426.780,0.130,-127493324.706900,0.008,-3976.867,44.9,13028.379,08119c04,51,0,23917434.944,0.031,-99161492.405944,0.010,-3093.121,42.6,13024.238,10b13c0b,51,0,23917434.780,0.126,-99161488.657552,0.010,-3093.121,43.4,13025.178,00319c0b,38,8,19851538.779,0.107,-106117893.493769,0.007,1849.414,46.6,6208.818,08119c24,38,8,19851544.763,0.031,-82536182.471767,0.007,1438.434,42.6,6204.118,00b13c2b,38,8,19851543.771,0.124,-82536181.722576,0.007,1438.434,43.6,6205.038,00319c2b,25,0,27861125.116,0.078,-146411169.405727,0.011,-3136.592,43.2,21188.543,08539cc4,25,0,27861133.366,0.009,-109333028.194067,0.005,-2342.203,49.0,21186.443,01933cc4,25,0,27861129.463,0.011,-112185182.897162,0.006,-2403.344,47.0,21186.443,02333cc4,25,0,27861129.580,0.007,-110759098.611107,0.006,-2372.787,50.8,21186.164,02933cc4,4,0,25274631.488,0.038,-132819124.897734,0.006,997.361,49.6,7638.783,08539ce4,4,0,25274635.181,0.007,-99183140.380658,0.004,744.803,50.8,7636.565,01933ce4,4,0,25274631.890,0.007,-101770517.169783,0.004,764.254,50.9,7636.444,02333ce4,4,0,25274631.708,0.005,-100476824.840813,0.004,754.545,53.6,7636.363,02933ce4,12,0,26373649.887,0.092,-138594449.111813,0.012,-2565.281,41.8,26740.730,08539d04,12,0,26373653.619,0.019,-103495853.823161,0.008,-1915.449,42.6,26738.648,01933d04,12,0,26373650.081,0.023,-106195738.067164,0.011,-1965.500,41.1,26738.449,02333d04,12,0,26373650.251,0.015,-104845791.009488,0.010,-1940.442,44.6,26738.371,02933d04,11,0,22137124.256,0.039,-116331408.305147,0.015,-1200.216,49.2,19415.590,08539d24,11,0,22137125.344,0.008,-86870883.829203,0.011,-896.289,49.8,19413.172,01933d24,11,0,22137122.146,0.008,-89137066.170706,0.012,-919.719,49.6,19413.248,02333d24,11,0,22137121.891,0.006,-88003971.568373,0.011,-908.028,52.4,19413.172,02933d24,30,0,25928558.680,0.072,-136255508.290211,0.010,743.664,43.9,3960.112,08539d44,30,0,25928564.638,0.011,-101749279.487957,0.006,555.328,47.5,4752.748,01933d44,30,0,25928561.460,0.010,-104403592.595320,0.005,569.759,48.1,4753.047,02333d44,30,0,25928561.332,0.008,-103076425.609137,0.006,562.539,50.5,4752.767,02933d44,2,0,25889111.981,0.043,-136048218.157560,0.006,-1792.931,48.4,12654.424,08539d64,2,0,25889117.006,0.009,-101594476.864922,0.005,-1338.866,48.7,12652.444,01933d64,2,0,25889114.168,0.009,-104244753.680674,0.004,-1373.765,49.5,12651.978,02333d64,2,0,25889113.739,0.007,-102919609.843844,0.005,-1356.370,51.8,12651.943,02933d64,19,0,27039623.380,0.118,-142094196.888887,0.015,-1878.632,39.7,11125.104,08539d84,19,0,27039628.887,0.020,-106109319.847355,0.010,-1402.842,41.9,11123.043,01933d84,19,0,27039625.153,0.024,-108877382.476710,0.011,-1439.341,40.6,11122.757,02333d84,19,0,27039625.337,0.016,-107493348.232960,0.010,-1421.103,44.1,11122.765,02933d84,36,0,23927504.603,0.030,-125739945.419298,0.005,1241.596,51.7,11037.264,08539da4,36,0,23927510.217,0.006,-93896767.646843,0.004,927.156,52.9,11035.164,01933da4,36,0,23927507.273,0.006,-96346233.181780,0.004,951.361,53.2,11035.376,02333da4,36,0,23927507.057,0.004,-95121494.979676,0.004,939.285,55.8,11031.874,02933da4,9,0,24890379.004,0.046,-130799846.144936,0.007,3052.621,47.8,2955.889,08539dc4,9,0,24890384.304,0.009,-97675250.055577,0.005,2279.540,49.1,2953.828,01933dc4,9,0,24890381.065,0.008,-100223286.825938,0.004,2338.979,50.0,2953.762,02333dc4,9,0,24890381.366,0.006,-98949262.506583,0.005,2309.297,52.2,2949.700,02933dc4,23,0,26593863.945,0.036,-138481231.000933,0.010,-48.553,44.1,2628.888,08149ec4,23,0,26593862.563,0.010,-104360035.310223,0.005,-36.590,48.2,2623.647,41343ec4,34,0,23330414.273,0.017,-121487628.857801,0.005,2280.558,50.6,6539.069,58149ee4,34,0,23330415.641,0.008,-91553618.939049,0.005,1718.641,50.1,6533.770,41343ee4,35,0,24822913.452,0.024,-129259432.414616,0.007,-2925.143,47.4,23499.049,58149f04,35,0,24822915.980,0.012,-97410461.716286,0.006,-2204.368,46.7,23493.830,41343f04,11,0,24964039.739,0.052,-129994328.361984,0.014,2939.333,40.8,2708.970,18149f24,11,0,24964038.060,0.022,-100519869.959755,0.006,2272.851,48.3,2708.810,00349f24,19,0,23905947.282,0.033,-124484578.888819,0.009,-2342.726,44.8,13489.051,18149f44,19,0,23905949.046,0.008,-93812119.376225,0.005,-1765.479,50.1,13483.831,41343f44,21,0,24577306.170,0.027,-127980528.823414,0.008,3242.344,46.7,3439.068,18149f84,21,0,24577307.993,0.008,-96446682.849511,0.005,2443.502,49.7,3433.828,41343f84,22,0,22438270.920,0.015,-116842012.781567,0.005,729.096,51.5,8979.049,18149fa4,22,0,22438269.274,0.005,-88052653.428423,0.003,549.506,54.1,8973.770,41343fa4,44,0,21553538.984,0.014,-112234979.640419,0.005,-679.127,52.1,15439.131,48149ca4,44,0,21553540.824,0.005,-84580779.869687,0.003,-511.716,53.6,15433.829,41343ca4,57,0,26771391.610,0.021,-139405685.309616,0.007,-2069.940,48.5,20196.455,48049d04,12,0,21542689.063,0.021,-112178498.767984,0.006,952.964,48.8,11229.051,18149d24,12,0,21542686.409,0.013,-86743545.297369,0.004,736.976,52.6,11228.890,00349d24,25,0,26603375.741,0.069,-138530755.415895,0.019,-2155.462,38.4,9789.050,18149d44,25,0,26603380.238,0.015,-104397363.013083,0.007,-1624.205,44.7,9783.829,41343d44*5e9785bd\r\n";
   const char* gloalmanac_log = "#GLOALMANACA,COM1,0,54.0,SATTIME,2167,159108.000,02000000,ba83,16248;24,2167,115150.343,1,1,1,0,39532.343750000,1.316556988,0.022644193,0.000380516,-0.200759736,-2655.375000000,-0.000305176,-0.000080109,2167,80546.843,2,-4,1,0,4928.843750000,-2.415465471,0.030652651,0.001904488,-2.240858310,-2655.851562500,0.000305176,-0.000484467,2167,85554.000,3,5,1,0,9936.000000000,-2.786838624,0.027761457,0.001849174,-2.155051259,-2655.845703125,0.000244141,-0.000038147,2167,90494.343,4,6,1,0,14876.343750000,3.138599593,0.033250232,0.000991821,-1.632539054,-2655.865234375,0.000061035,-0.000095367,2167,95426.781,5,1,1,0,19808.781250000,2.781340861,0.033516881,0.000567436,-2.318803708,-2655.912109375,0.000000000,-0.000072479,2167,101118.375,6,-4,1,0,25500.375000000,2.337855630,0.022338595,0.000492096,2.475749118,-2655.802734375,-0.000183105,-0.000198364,2167,106019.281,7,5,1,0,30401.281250000,2.004915886,0.027902272,0.001599312,-2.137026985,-2655.806640625,-0.000122070,0.000041962,2167,110772.718,8,6,1,0,35154.718750000,1.658347082,0.028034098,0.002008438,-1.757079119,-2655.869140625,-0.000183105,0.000057220,2167,114259.031,9,-2,1,0,38641.031250000,-2.800770285,0.013374395,0.001688004,-2.688301331,-2655.992187500,-0.000915527,-0.000000000,2167,78451.250,10,-7,1,0,2833.250000000,-0.189087101,0.025562352,0.001439095,0.043239083,-2656.169921875,-0.001342773,0.000072479,2167,83619.250,11,0,1,1,8001.250000000,-0.568264981,0.030221219,0.000588417,-2.044029400,-2656.169921875,-0.001342773,0.000022888,2167,88863.437,12,-1,1,0,13245.437500000,-0.938955033,0.026368291,0.001175880,-1.256138518,-2655.986328125,-0.001159668,-0.000244141,2167,93781.406,13,-2,1,0,18163.406250000,-1.308018227,0.025406557,0.000337601,1.744136156,-2656.201171875,-0.001037598,0.000057220,2167,99049.875,14,-7,1,0,23431.875000000,-1.683226333,0.021385849,0.000715256,-2.112099797,-2656.009765625,-0.001098633,-0.000064850,2167,104050.250,15,0,1,0,28432.250000000,-2.043945510,0.025130920,0.000899315,-1.639250219,-2656.019531250,-0.001037598,-0.000099182,2167,109475.187,16,-1,1,0,33857.187500000,-2.465775247,0.018401777,0.002746582,0.205936921,-2655.822265625,-0.000854492,0.000015259,2167,112381.000,17,4,1,0,36763.000000000,-0.550378525,0.044683183,0.000854492,-3.118007699,-2655.976562500,0.001098633,-0.000438690,2167,76649.656,18,-3,1,0,1031.656250000,2.061364581,0.049192247,0.001056671,-0.229426002,-2656.011718750,0.001037598,-0.000083923,2167,81216.375,19,3,1,0,5598.375000000,1.753316072,0.053257895,0.000308990,2.031661680,-2656.169921875,0.000915527,0.000148773,2167,86932.187,20,2,1,0,11314.187500000,1.338137581,0.053485596,0.000810623,0.016106798,-2656.033203125,0.001037598,0.000049591,2167,92471.875,21,4,1,0,16853.875000000,0.905492081,0.048149620,0.000671387,2.711982159,-2655.875000000,0.001098633,0.000225067,2167,97225.437,22,-3,1,0,21607.437500000,0.566332524,0.051370380,0.002092361,0.380906604,-2656.091796875,0.001159668,0.000122070,2167,103403.781,23,3,1,0,27785.781250000,0.114991634,0.051142680,0.000539780,1.610679827,-2656.626953125,0.001098633,0.000007629,2167,107403.343,24,2,1,0,31785.343750000,-0.171967635,0.033052492,0.000456810,-2.399433574,-2656.039062500,0.001037598,-0.000049591*6dee109c\r\n";

   pclMyFilter->IncludeMessageId(42, HEADERFORMAT::ASCII, MEASUREMENT_SOURCE::PRIMARY);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(version_log))));    // Version ID: 37
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log))));     // Bestpos ID: 42
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(range_log))));      // Range ID: 43
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(gloalmanac_log)))); // Gloalmanac ID: 718
}

TEST_F(FilterTest, MULTIPLE_MESSAGE_ID)
{
   const char* bestpos_log    = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313698.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* version_log    = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   const char* range_log      = "#RANGEA,COM1,0,49.0,FINESTEERING,2167,159740.000,02000000,5103,16248;115,20,0,23291390.821,0.051,-122397109.320305,0.011,-3214.414,44.0,16498.975,1810bc04,20,0,23291389.349,0.184,-95374376.486282,0.013,-2504.740,41.1,16492.754,11303c0b,29,0,20705108.295,0.022,-108806118.750397,0.005,-1620.698,51.2,14908.774,0810bc44,29,0,20705106.528,0.061,-84783996.294254,0.006,-1262.882,50.7,14902.754,01303c4b,29,0,20705107.069,0.025,-84783990.539554,0.005,-1262.882,51.3,14905.534,02309c4b,13,0,22941454.717,0.042,-120558183.531157,0.009,2111.590,45.7,3984.555,1810bc64,13,0,22941453.693,0.180,-93941456.741696,0.011,1645.396,41.2,3978.255,11303c6b,15,0,22752775.227,0.046,-119566687.005913,0.012,3112.730,44.8,3628.275,0810bc84,15,0,22752775.420,0.118,-93168858.308908,0.012,2425.506,44.9,3622.754,01303c8b,15,0,22752775.932,0.043,-93168851.550483,0.012,2425.505,46.7,3625.014,02309c8b,16,0,23437165.563,0.058,-123163154.592686,0.010,1667.743,42.8,3318.855,1810bca4,16,0,23437164.832,0.179,-95971289.711375,0.012,1299.540,41.3,3313.415,11303cab,18,0,20870226.835,0.024,-109673843.056839,0.005,1782.527,50.4,7988.975,1810bcc4,18,0,20870225.520,0.080,-85460161.951054,0.006,1388.983,48.3,7983.255,11303ccb,18,0,20870225.982,0.024,-85460166.200662,0.006,1388.983,51.9,7985.814,02309ccb,18,0,20870231.341,0.004,-81899348.012827,0.003,1331.096,55.4,7987.255,01d03cc4,5,0,20977730.840,0.024,-110238771.341810,0.006,-1850.683,50.6,12588.896,0810bd04,5,0,20977729.403,0.070,-85900356.288455,0.006,-1442.091,49.4,12583.755,01303d0b,5,0,20977730.127,0.030,-85900355.538492,0.006,-1442.091,49.8,12585.716,02309d0b,26,0,22753733.201,0.042,-119571694.561007,0.008,-494.798,45.7,7318.775,1810bd24,26,0,22753735.163,0.118,-93172767.333088,0.010,-385.557,44.9,7312.755,11303d2b,26,0,22753735.387,0.043,-93172769.582418,0.009,-385.557,46.7,7315.375,02309d2b,26,0,22753735.481,0.008,-89290576.088766,0.005,-369.472,50.1,7316.815,01d03d24,23,0,23067782.934,0.040,-121222050.181679,0.009,3453.274,46.0,3078.894,0810bd44,23,0,23067783.254,0.142,-94458759.273215,0.010,2690.865,43.3,3073.754,01303d4b,23,0,23067783.763,0.040,-94458764.522108,0.009,2690.865,47.3,3076.395,02309d4b,23,0,23067789.450,0.007,-90523004.360543,0.004,2578.883,51.2,3077.834,01d03d44,194,0,43027813.095,0.065,-226112681.899748,0.013,43.499,41.9,17178.695,1815be04,194,0,43027815.196,0.059,-176191709.875251,0.014,33.896,44.0,17173.014,02359e0b,194,0,43027817.865,0.014,-168850394.176443,0.007,32.406,45.4,17177.053,01d53e04,131,0,38480107.260,0.124,-202214296.438902,0.009,-0.922,46.2,292335.531,48023e84,133,0,38618703.555,0.119,-202942631.161186,0.007,0.421,46.7,916697.188,58023ec4,138,0,38495561.597,0.116,-202295515.714333,0.008,-4.752,46.8,292343.625,48023ee4,45,13,20655319.254,0.111,-110608334.938276,0.006,-1928.119,46.3,9728.839,18119f04,45,13,20655320.731,0.021,-86028727.119001,0.006,-1499.649,45.9,9724.239,10b13f0b,45,13,20655321.099,0.092,-86028721.367030,0.006,-1499.649,46.1,9725.238,10319f0b,53,6,23361335.550,0.284,-124792043.406215,0.017,1741.893,38.1,444.840,08119f24,53,6,23361340.271,0.098,-97060514.793435,0.017,1354.807,32.6,444.741,00b13f2b,53,6,23361339.423,0.393,-97060517.036654,0.018,1354.806,33.5,444.801,10319f2b,60,10,20724752.466,0.106,-110863493.957380,0.007,-2492.451,46.7,16549.037,18019f44,39,3,23534282.253,0.169,-125583452.109842,0.012,4608.280,42.6,557.668,08119f84,39,3,23534291.023,0.027,-97676038.550992,0.013,3584.223,43.8,552.119,10b13f8b,39,3,23534290.639,0.108,-97676048.806539,0.013,3584.223,44.7,552.959,10319f8b,61,9,19285134.504,0.086,-103126338.171372,0.005,228.766,48.6,11128.199,08119fa4,61,9,19285138.043,0.020,-80209402.132964,0.005,177.929,46.3,11124.118,00b13fab,61,9,19285138.376,0.084,-80209411.390794,0.005,177.929,46.9,11125.037,00319fab,52,7,22348227.548,0.137,-119422164.171132,0.008,-1798.230,44.4,7458.668,08119fc4,52,7,22348232.044,0.025,-92883929.564420,0.008,-1398.625,44.4,7453.898,00b13fcb,52,7,22348232.124,0.104,-92883930.822797,0.008,-1398.624,45.0,7455.038,10319fcb,54,11,21518220.426,0.169,-115148393.440041,0.010,3262.249,42.6,3877.098,18119fe4,54,11,21518225.678,0.025,-89559888.534930,0.010,2537.306,44.6,3871.818,00b13feb,54,11,21518226.376,0.107,-89559882.794247,0.010,2537.307,44.8,3872.818,10319feb,51,0,23917426.780,0.130,-127493324.706900,0.008,-3976.867,44.9,13028.379,08119c04,51,0,23917434.944,0.031,-99161492.405944,0.010,-3093.121,42.6,13024.238,10b13c0b,51,0,23917434.780,0.126,-99161488.657552,0.010,-3093.121,43.4,13025.178,00319c0b,38,8,19851538.779,0.107,-106117893.493769,0.007,1849.414,46.6,6208.818,08119c24,38,8,19851544.763,0.031,-82536182.471767,0.007,1438.434,42.6,6204.118,00b13c2b,38,8,19851543.771,0.124,-82536181.722576,0.007,1438.434,43.6,6205.038,00319c2b,25,0,27861125.116,0.078,-146411169.405727,0.011,-3136.592,43.2,21188.543,08539cc4,25,0,27861133.366,0.009,-109333028.194067,0.005,-2342.203,49.0,21186.443,01933cc4,25,0,27861129.463,0.011,-112185182.897162,0.006,-2403.344,47.0,21186.443,02333cc4,25,0,27861129.580,0.007,-110759098.611107,0.006,-2372.787,50.8,21186.164,02933cc4,4,0,25274631.488,0.038,-132819124.897734,0.006,997.361,49.6,7638.783,08539ce4,4,0,25274635.181,0.007,-99183140.380658,0.004,744.803,50.8,7636.565,01933ce4,4,0,25274631.890,0.007,-101770517.169783,0.004,764.254,50.9,7636.444,02333ce4,4,0,25274631.708,0.005,-100476824.840813,0.004,754.545,53.6,7636.363,02933ce4,12,0,26373649.887,0.092,-138594449.111813,0.012,-2565.281,41.8,26740.730,08539d04,12,0,26373653.619,0.019,-103495853.823161,0.008,-1915.449,42.6,26738.648,01933d04,12,0,26373650.081,0.023,-106195738.067164,0.011,-1965.500,41.1,26738.449,02333d04,12,0,26373650.251,0.015,-104845791.009488,0.010,-1940.442,44.6,26738.371,02933d04,11,0,22137124.256,0.039,-116331408.305147,0.015,-1200.216,49.2,19415.590,08539d24,11,0,22137125.344,0.008,-86870883.829203,0.011,-896.289,49.8,19413.172,01933d24,11,0,22137122.146,0.008,-89137066.170706,0.012,-919.719,49.6,19413.248,02333d24,11,0,22137121.891,0.006,-88003971.568373,0.011,-908.028,52.4,19413.172,02933d24,30,0,25928558.680,0.072,-136255508.290211,0.010,743.664,43.9,3960.112,08539d44,30,0,25928564.638,0.011,-101749279.487957,0.006,555.328,47.5,4752.748,01933d44,30,0,25928561.460,0.010,-104403592.595320,0.005,569.759,48.1,4753.047,02333d44,30,0,25928561.332,0.008,-103076425.609137,0.006,562.539,50.5,4752.767,02933d44,2,0,25889111.981,0.043,-136048218.157560,0.006,-1792.931,48.4,12654.424,08539d64,2,0,25889117.006,0.009,-101594476.864922,0.005,-1338.866,48.7,12652.444,01933d64,2,0,25889114.168,0.009,-104244753.680674,0.004,-1373.765,49.5,12651.978,02333d64,2,0,25889113.739,0.007,-102919609.843844,0.005,-1356.370,51.8,12651.943,02933d64,19,0,27039623.380,0.118,-142094196.888887,0.015,-1878.632,39.7,11125.104,08539d84,19,0,27039628.887,0.020,-106109319.847355,0.010,-1402.842,41.9,11123.043,01933d84,19,0,27039625.153,0.024,-108877382.476710,0.011,-1439.341,40.6,11122.757,02333d84,19,0,27039625.337,0.016,-107493348.232960,0.010,-1421.103,44.1,11122.765,02933d84,36,0,23927504.603,0.030,-125739945.419298,0.005,1241.596,51.7,11037.264,08539da4,36,0,23927510.217,0.006,-93896767.646843,0.004,927.156,52.9,11035.164,01933da4,36,0,23927507.273,0.006,-96346233.181780,0.004,951.361,53.2,11035.376,02333da4,36,0,23927507.057,0.004,-95121494.979676,0.004,939.285,55.8,11031.874,02933da4,9,0,24890379.004,0.046,-130799846.144936,0.007,3052.621,47.8,2955.889,08539dc4,9,0,24890384.304,0.009,-97675250.055577,0.005,2279.540,49.1,2953.828,01933dc4,9,0,24890381.065,0.008,-100223286.825938,0.004,2338.979,50.0,2953.762,02333dc4,9,0,24890381.366,0.006,-98949262.506583,0.005,2309.297,52.2,2949.700,02933dc4,23,0,26593863.945,0.036,-138481231.000933,0.010,-48.553,44.1,2628.888,08149ec4,23,0,26593862.563,0.010,-104360035.310223,0.005,-36.590,48.2,2623.647,41343ec4,34,0,23330414.273,0.017,-121487628.857801,0.005,2280.558,50.6,6539.069,58149ee4,34,0,23330415.641,0.008,-91553618.939049,0.005,1718.641,50.1,6533.770,41343ee4,35,0,24822913.452,0.024,-129259432.414616,0.007,-2925.143,47.4,23499.049,58149f04,35,0,24822915.980,0.012,-97410461.716286,0.006,-2204.368,46.7,23493.830,41343f04,11,0,24964039.739,0.052,-129994328.361984,0.014,2939.333,40.8,2708.970,18149f24,11,0,24964038.060,0.022,-100519869.959755,0.006,2272.851,48.3,2708.810,00349f24,19,0,23905947.282,0.033,-124484578.888819,0.009,-2342.726,44.8,13489.051,18149f44,19,0,23905949.046,0.008,-93812119.376225,0.005,-1765.479,50.1,13483.831,41343f44,21,0,24577306.170,0.027,-127980528.823414,0.008,3242.344,46.7,3439.068,18149f84,21,0,24577307.993,0.008,-96446682.849511,0.005,2443.502,49.7,3433.828,41343f84,22,0,22438270.920,0.015,-116842012.781567,0.005,729.096,51.5,8979.049,18149fa4,22,0,22438269.274,0.005,-88052653.428423,0.003,549.506,54.1,8973.770,41343fa4,44,0,21553538.984,0.014,-112234979.640419,0.005,-679.127,52.1,15439.131,48149ca4,44,0,21553540.824,0.005,-84580779.869687,0.003,-511.716,53.6,15433.829,41343ca4,57,0,26771391.610,0.021,-139405685.309616,0.007,-2069.940,48.5,20196.455,48049d04,12,0,21542689.063,0.021,-112178498.767984,0.006,952.964,48.8,11229.051,18149d24,12,0,21542686.409,0.013,-86743545.297369,0.004,736.976,52.6,11228.890,00349d24,25,0,26603375.741,0.069,-138530755.415895,0.019,-2155.462,38.4,9789.050,18149d44,25,0,26603380.238,0.015,-104397363.013083,0.007,-1624.205,44.7,9783.829,41343d44*5e9785bd\r\n";
   const char* gloalmanac_log = "#GLOALMANACA,COM1,0,54.0,SATTIME,2167,159108.000,02000000,ba83,16248;24,2167,115150.343,1,1,1,0,39532.343750000,1.316556988,0.022644193,0.000380516,-0.200759736,-2655.375000000,-0.000305176,-0.000080109,2167,80546.843,2,-4,1,0,4928.843750000,-2.415465471,0.030652651,0.001904488,-2.240858310,-2655.851562500,0.000305176,-0.000484467,2167,85554.000,3,5,1,0,9936.000000000,-2.786838624,0.027761457,0.001849174,-2.155051259,-2655.845703125,0.000244141,-0.000038147,2167,90494.343,4,6,1,0,14876.343750000,3.138599593,0.033250232,0.000991821,-1.632539054,-2655.865234375,0.000061035,-0.000095367,2167,95426.781,5,1,1,0,19808.781250000,2.781340861,0.033516881,0.000567436,-2.318803708,-2655.912109375,0.000000000,-0.000072479,2167,101118.375,6,-4,1,0,25500.375000000,2.337855630,0.022338595,0.000492096,2.475749118,-2655.802734375,-0.000183105,-0.000198364,2167,106019.281,7,5,1,0,30401.281250000,2.004915886,0.027902272,0.001599312,-2.137026985,-2655.806640625,-0.000122070,0.000041962,2167,110772.718,8,6,1,0,35154.718750000,1.658347082,0.028034098,0.002008438,-1.757079119,-2655.869140625,-0.000183105,0.000057220,2167,114259.031,9,-2,1,0,38641.031250000,-2.800770285,0.013374395,0.001688004,-2.688301331,-2655.992187500,-0.000915527,-0.000000000,2167,78451.250,10,-7,1,0,2833.250000000,-0.189087101,0.025562352,0.001439095,0.043239083,-2656.169921875,-0.001342773,0.000072479,2167,83619.250,11,0,1,1,8001.250000000,-0.568264981,0.030221219,0.000588417,-2.044029400,-2656.169921875,-0.001342773,0.000022888,2167,88863.437,12,-1,1,0,13245.437500000,-0.938955033,0.026368291,0.001175880,-1.256138518,-2655.986328125,-0.001159668,-0.000244141,2167,93781.406,13,-2,1,0,18163.406250000,-1.308018227,0.025406557,0.000337601,1.744136156,-2656.201171875,-0.001037598,0.000057220,2167,99049.875,14,-7,1,0,23431.875000000,-1.683226333,0.021385849,0.000715256,-2.112099797,-2656.009765625,-0.001098633,-0.000064850,2167,104050.250,15,0,1,0,28432.250000000,-2.043945510,0.025130920,0.000899315,-1.639250219,-2656.019531250,-0.001037598,-0.000099182,2167,109475.187,16,-1,1,0,33857.187500000,-2.465775247,0.018401777,0.002746582,0.205936921,-2655.822265625,-0.000854492,0.000015259,2167,112381.000,17,4,1,0,36763.000000000,-0.550378525,0.044683183,0.000854492,-3.118007699,-2655.976562500,0.001098633,-0.000438690,2167,76649.656,18,-3,1,0,1031.656250000,2.061364581,0.049192247,0.001056671,-0.229426002,-2656.011718750,0.001037598,-0.000083923,2167,81216.375,19,3,1,0,5598.375000000,1.753316072,0.053257895,0.000308990,2.031661680,-2656.169921875,0.000915527,0.000148773,2167,86932.187,20,2,1,0,11314.187500000,1.338137581,0.053485596,0.000810623,0.016106798,-2656.033203125,0.001037598,0.000049591,2167,92471.875,21,4,1,0,16853.875000000,0.905492081,0.048149620,0.000671387,2.711982159,-2655.875000000,0.001098633,0.000225067,2167,97225.437,22,-3,1,0,21607.437500000,0.566332524,0.051370380,0.002092361,0.380906604,-2656.091796875,0.001159668,0.000122070,2167,103403.781,23,3,1,0,27785.781250000,0.114991634,0.051142680,0.000539780,1.610679827,-2656.626953125,0.001098633,0.000007629,2167,107403.343,24,2,1,0,31785.343750000,-0.171967635,0.033052492,0.000456810,-2.399433574,-2656.039062500,0.001037598,-0.000049591*6dee109c\r\n";
   std::vector<std::tuple<uint32_t, HEADERFORMAT, MEASUREMENT_SOURCE>> message_ids = {
      {43, HEADERFORMAT::ASCII, MEASUREMENT_SOURCE::PRIMARY },
      {718, HEADERFORMAT::ASCII, MEASUREMENT_SOURCE::PRIMARY },
   };

   pclMyFilter->IncludeMessageId(message_ids);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(version_log))));   // Version ID: 37
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log))));   // Bestpos ID: 42
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(range_log))));      // Range ID: 43
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(gloalmanac_log)))); // Gloalmanac ID: 718
}

TEST_F(FilterTest, MESSAGE_NAME)
{
   const char* bestpos_log    = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313698.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* version_log    = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   const char* range_log      = "#RANGEA,COM1,0,49.0,FINESTEERING,2167,159740.000,02000000,5103,16248;115,20,0,23291390.821,0.051,-122397109.320305,0.011,-3214.414,44.0,16498.975,1810bc04,20,0,23291389.349,0.184,-95374376.486282,0.013,-2504.740,41.1,16492.754,11303c0b,29,0,20705108.295,0.022,-108806118.750397,0.005,-1620.698,51.2,14908.774,0810bc44,29,0,20705106.528,0.061,-84783996.294254,0.006,-1262.882,50.7,14902.754,01303c4b,29,0,20705107.069,0.025,-84783990.539554,0.005,-1262.882,51.3,14905.534,02309c4b,13,0,22941454.717,0.042,-120558183.531157,0.009,2111.590,45.7,3984.555,1810bc64,13,0,22941453.693,0.180,-93941456.741696,0.011,1645.396,41.2,3978.255,11303c6b,15,0,22752775.227,0.046,-119566687.005913,0.012,3112.730,44.8,3628.275,0810bc84,15,0,22752775.420,0.118,-93168858.308908,0.012,2425.506,44.9,3622.754,01303c8b,15,0,22752775.932,0.043,-93168851.550483,0.012,2425.505,46.7,3625.014,02309c8b,16,0,23437165.563,0.058,-123163154.592686,0.010,1667.743,42.8,3318.855,1810bca4,16,0,23437164.832,0.179,-95971289.711375,0.012,1299.540,41.3,3313.415,11303cab,18,0,20870226.835,0.024,-109673843.056839,0.005,1782.527,50.4,7988.975,1810bcc4,18,0,20870225.520,0.080,-85460161.951054,0.006,1388.983,48.3,7983.255,11303ccb,18,0,20870225.982,0.024,-85460166.200662,0.006,1388.983,51.9,7985.814,02309ccb,18,0,20870231.341,0.004,-81899348.012827,0.003,1331.096,55.4,7987.255,01d03cc4,5,0,20977730.840,0.024,-110238771.341810,0.006,-1850.683,50.6,12588.896,0810bd04,5,0,20977729.403,0.070,-85900356.288455,0.006,-1442.091,49.4,12583.755,01303d0b,5,0,20977730.127,0.030,-85900355.538492,0.006,-1442.091,49.8,12585.716,02309d0b,26,0,22753733.201,0.042,-119571694.561007,0.008,-494.798,45.7,7318.775,1810bd24,26,0,22753735.163,0.118,-93172767.333088,0.010,-385.557,44.9,7312.755,11303d2b,26,0,22753735.387,0.043,-93172769.582418,0.009,-385.557,46.7,7315.375,02309d2b,26,0,22753735.481,0.008,-89290576.088766,0.005,-369.472,50.1,7316.815,01d03d24,23,0,23067782.934,0.040,-121222050.181679,0.009,3453.274,46.0,3078.894,0810bd44,23,0,23067783.254,0.142,-94458759.273215,0.010,2690.865,43.3,3073.754,01303d4b,23,0,23067783.763,0.040,-94458764.522108,0.009,2690.865,47.3,3076.395,02309d4b,23,0,23067789.450,0.007,-90523004.360543,0.004,2578.883,51.2,3077.834,01d03d44,194,0,43027813.095,0.065,-226112681.899748,0.013,43.499,41.9,17178.695,1815be04,194,0,43027815.196,0.059,-176191709.875251,0.014,33.896,44.0,17173.014,02359e0b,194,0,43027817.865,0.014,-168850394.176443,0.007,32.406,45.4,17177.053,01d53e04,131,0,38480107.260,0.124,-202214296.438902,0.009,-0.922,46.2,292335.531,48023e84,133,0,38618703.555,0.119,-202942631.161186,0.007,0.421,46.7,916697.188,58023ec4,138,0,38495561.597,0.116,-202295515.714333,0.008,-4.752,46.8,292343.625,48023ee4,45,13,20655319.254,0.111,-110608334.938276,0.006,-1928.119,46.3,9728.839,18119f04,45,13,20655320.731,0.021,-86028727.119001,0.006,-1499.649,45.9,9724.239,10b13f0b,45,13,20655321.099,0.092,-86028721.367030,0.006,-1499.649,46.1,9725.238,10319f0b,53,6,23361335.550,0.284,-124792043.406215,0.017,1741.893,38.1,444.840,08119f24,53,6,23361340.271,0.098,-97060514.793435,0.017,1354.807,32.6,444.741,00b13f2b,53,6,23361339.423,0.393,-97060517.036654,0.018,1354.806,33.5,444.801,10319f2b,60,10,20724752.466,0.106,-110863493.957380,0.007,-2492.451,46.7,16549.037,18019f44,39,3,23534282.253,0.169,-125583452.109842,0.012,4608.280,42.6,557.668,08119f84,39,3,23534291.023,0.027,-97676038.550992,0.013,3584.223,43.8,552.119,10b13f8b,39,3,23534290.639,0.108,-97676048.806539,0.013,3584.223,44.7,552.959,10319f8b,61,9,19285134.504,0.086,-103126338.171372,0.005,228.766,48.6,11128.199,08119fa4,61,9,19285138.043,0.020,-80209402.132964,0.005,177.929,46.3,11124.118,00b13fab,61,9,19285138.376,0.084,-80209411.390794,0.005,177.929,46.9,11125.037,00319fab,52,7,22348227.548,0.137,-119422164.171132,0.008,-1798.230,44.4,7458.668,08119fc4,52,7,22348232.044,0.025,-92883929.564420,0.008,-1398.625,44.4,7453.898,00b13fcb,52,7,22348232.124,0.104,-92883930.822797,0.008,-1398.624,45.0,7455.038,10319fcb,54,11,21518220.426,0.169,-115148393.440041,0.010,3262.249,42.6,3877.098,18119fe4,54,11,21518225.678,0.025,-89559888.534930,0.010,2537.306,44.6,3871.818,00b13feb,54,11,21518226.376,0.107,-89559882.794247,0.010,2537.307,44.8,3872.818,10319feb,51,0,23917426.780,0.130,-127493324.706900,0.008,-3976.867,44.9,13028.379,08119c04,51,0,23917434.944,0.031,-99161492.405944,0.010,-3093.121,42.6,13024.238,10b13c0b,51,0,23917434.780,0.126,-99161488.657552,0.010,-3093.121,43.4,13025.178,00319c0b,38,8,19851538.779,0.107,-106117893.493769,0.007,1849.414,46.6,6208.818,08119c24,38,8,19851544.763,0.031,-82536182.471767,0.007,1438.434,42.6,6204.118,00b13c2b,38,8,19851543.771,0.124,-82536181.722576,0.007,1438.434,43.6,6205.038,00319c2b,25,0,27861125.116,0.078,-146411169.405727,0.011,-3136.592,43.2,21188.543,08539cc4,25,0,27861133.366,0.009,-109333028.194067,0.005,-2342.203,49.0,21186.443,01933cc4,25,0,27861129.463,0.011,-112185182.897162,0.006,-2403.344,47.0,21186.443,02333cc4,25,0,27861129.580,0.007,-110759098.611107,0.006,-2372.787,50.8,21186.164,02933cc4,4,0,25274631.488,0.038,-132819124.897734,0.006,997.361,49.6,7638.783,08539ce4,4,0,25274635.181,0.007,-99183140.380658,0.004,744.803,50.8,7636.565,01933ce4,4,0,25274631.890,0.007,-101770517.169783,0.004,764.254,50.9,7636.444,02333ce4,4,0,25274631.708,0.005,-100476824.840813,0.004,754.545,53.6,7636.363,02933ce4,12,0,26373649.887,0.092,-138594449.111813,0.012,-2565.281,41.8,26740.730,08539d04,12,0,26373653.619,0.019,-103495853.823161,0.008,-1915.449,42.6,26738.648,01933d04,12,0,26373650.081,0.023,-106195738.067164,0.011,-1965.500,41.1,26738.449,02333d04,12,0,26373650.251,0.015,-104845791.009488,0.010,-1940.442,44.6,26738.371,02933d04,11,0,22137124.256,0.039,-116331408.305147,0.015,-1200.216,49.2,19415.590,08539d24,11,0,22137125.344,0.008,-86870883.829203,0.011,-896.289,49.8,19413.172,01933d24,11,0,22137122.146,0.008,-89137066.170706,0.012,-919.719,49.6,19413.248,02333d24,11,0,22137121.891,0.006,-88003971.568373,0.011,-908.028,52.4,19413.172,02933d24,30,0,25928558.680,0.072,-136255508.290211,0.010,743.664,43.9,3960.112,08539d44,30,0,25928564.638,0.011,-101749279.487957,0.006,555.328,47.5,4752.748,01933d44,30,0,25928561.460,0.010,-104403592.595320,0.005,569.759,48.1,4753.047,02333d44,30,0,25928561.332,0.008,-103076425.609137,0.006,562.539,50.5,4752.767,02933d44,2,0,25889111.981,0.043,-136048218.157560,0.006,-1792.931,48.4,12654.424,08539d64,2,0,25889117.006,0.009,-101594476.864922,0.005,-1338.866,48.7,12652.444,01933d64,2,0,25889114.168,0.009,-104244753.680674,0.004,-1373.765,49.5,12651.978,02333d64,2,0,25889113.739,0.007,-102919609.843844,0.005,-1356.370,51.8,12651.943,02933d64,19,0,27039623.380,0.118,-142094196.888887,0.015,-1878.632,39.7,11125.104,08539d84,19,0,27039628.887,0.020,-106109319.847355,0.010,-1402.842,41.9,11123.043,01933d84,19,0,27039625.153,0.024,-108877382.476710,0.011,-1439.341,40.6,11122.757,02333d84,19,0,27039625.337,0.016,-107493348.232960,0.010,-1421.103,44.1,11122.765,02933d84,36,0,23927504.603,0.030,-125739945.419298,0.005,1241.596,51.7,11037.264,08539da4,36,0,23927510.217,0.006,-93896767.646843,0.004,927.156,52.9,11035.164,01933da4,36,0,23927507.273,0.006,-96346233.181780,0.004,951.361,53.2,11035.376,02333da4,36,0,23927507.057,0.004,-95121494.979676,0.004,939.285,55.8,11031.874,02933da4,9,0,24890379.004,0.046,-130799846.144936,0.007,3052.621,47.8,2955.889,08539dc4,9,0,24890384.304,0.009,-97675250.055577,0.005,2279.540,49.1,2953.828,01933dc4,9,0,24890381.065,0.008,-100223286.825938,0.004,2338.979,50.0,2953.762,02333dc4,9,0,24890381.366,0.006,-98949262.506583,0.005,2309.297,52.2,2949.700,02933dc4,23,0,26593863.945,0.036,-138481231.000933,0.010,-48.553,44.1,2628.888,08149ec4,23,0,26593862.563,0.010,-104360035.310223,0.005,-36.590,48.2,2623.647,41343ec4,34,0,23330414.273,0.017,-121487628.857801,0.005,2280.558,50.6,6539.069,58149ee4,34,0,23330415.641,0.008,-91553618.939049,0.005,1718.641,50.1,6533.770,41343ee4,35,0,24822913.452,0.024,-129259432.414616,0.007,-2925.143,47.4,23499.049,58149f04,35,0,24822915.980,0.012,-97410461.716286,0.006,-2204.368,46.7,23493.830,41343f04,11,0,24964039.739,0.052,-129994328.361984,0.014,2939.333,40.8,2708.970,18149f24,11,0,24964038.060,0.022,-100519869.959755,0.006,2272.851,48.3,2708.810,00349f24,19,0,23905947.282,0.033,-124484578.888819,0.009,-2342.726,44.8,13489.051,18149f44,19,0,23905949.046,0.008,-93812119.376225,0.005,-1765.479,50.1,13483.831,41343f44,21,0,24577306.170,0.027,-127980528.823414,0.008,3242.344,46.7,3439.068,18149f84,21,0,24577307.993,0.008,-96446682.849511,0.005,2443.502,49.7,3433.828,41343f84,22,0,22438270.920,0.015,-116842012.781567,0.005,729.096,51.5,8979.049,18149fa4,22,0,22438269.274,0.005,-88052653.428423,0.003,549.506,54.1,8973.770,41343fa4,44,0,21553538.984,0.014,-112234979.640419,0.005,-679.127,52.1,15439.131,48149ca4,44,0,21553540.824,0.005,-84580779.869687,0.003,-511.716,53.6,15433.829,41343ca4,57,0,26771391.610,0.021,-139405685.309616,0.007,-2069.940,48.5,20196.455,48049d04,12,0,21542689.063,0.021,-112178498.767984,0.006,952.964,48.8,11229.051,18149d24,12,0,21542686.409,0.013,-86743545.297369,0.004,736.976,52.6,11228.890,00349d24,25,0,26603375.741,0.069,-138530755.415895,0.019,-2155.462,38.4,9789.050,18149d44,25,0,26603380.238,0.015,-104397363.013083,0.007,-1624.205,44.7,9783.829,41343d44*5e9785bd\r\n";
   const char* gloalmanac_log = "#GLOALMANACA,COM1,0,54.0,SATTIME,2167,159108.000,02000000,ba83,16248;24,2167,115150.343,1,1,1,0,39532.343750000,1.316556988,0.022644193,0.000380516,-0.200759736,-2655.375000000,-0.000305176,-0.000080109,2167,80546.843,2,-4,1,0,4928.843750000,-2.415465471,0.030652651,0.001904488,-2.240858310,-2655.851562500,0.000305176,-0.000484467,2167,85554.000,3,5,1,0,9936.000000000,-2.786838624,0.027761457,0.001849174,-2.155051259,-2655.845703125,0.000244141,-0.000038147,2167,90494.343,4,6,1,0,14876.343750000,3.138599593,0.033250232,0.000991821,-1.632539054,-2655.865234375,0.000061035,-0.000095367,2167,95426.781,5,1,1,0,19808.781250000,2.781340861,0.033516881,0.000567436,-2.318803708,-2655.912109375,0.000000000,-0.000072479,2167,101118.375,6,-4,1,0,25500.375000000,2.337855630,0.022338595,0.000492096,2.475749118,-2655.802734375,-0.000183105,-0.000198364,2167,106019.281,7,5,1,0,30401.281250000,2.004915886,0.027902272,0.001599312,-2.137026985,-2655.806640625,-0.000122070,0.000041962,2167,110772.718,8,6,1,0,35154.718750000,1.658347082,0.028034098,0.002008438,-1.757079119,-2655.869140625,-0.000183105,0.000057220,2167,114259.031,9,-2,1,0,38641.031250000,-2.800770285,0.013374395,0.001688004,-2.688301331,-2655.992187500,-0.000915527,-0.000000000,2167,78451.250,10,-7,1,0,2833.250000000,-0.189087101,0.025562352,0.001439095,0.043239083,-2656.169921875,-0.001342773,0.000072479,2167,83619.250,11,0,1,1,8001.250000000,-0.568264981,0.030221219,0.000588417,-2.044029400,-2656.169921875,-0.001342773,0.000022888,2167,88863.437,12,-1,1,0,13245.437500000,-0.938955033,0.026368291,0.001175880,-1.256138518,-2655.986328125,-0.001159668,-0.000244141,2167,93781.406,13,-2,1,0,18163.406250000,-1.308018227,0.025406557,0.000337601,1.744136156,-2656.201171875,-0.001037598,0.000057220,2167,99049.875,14,-7,1,0,23431.875000000,-1.683226333,0.021385849,0.000715256,-2.112099797,-2656.009765625,-0.001098633,-0.000064850,2167,104050.250,15,0,1,0,28432.250000000,-2.043945510,0.025130920,0.000899315,-1.639250219,-2656.019531250,-0.001037598,-0.000099182,2167,109475.187,16,-1,1,0,33857.187500000,-2.465775247,0.018401777,0.002746582,0.205936921,-2655.822265625,-0.000854492,0.000015259,2167,112381.000,17,4,1,0,36763.000000000,-0.550378525,0.044683183,0.000854492,-3.118007699,-2655.976562500,0.001098633,-0.000438690,2167,76649.656,18,-3,1,0,1031.656250000,2.061364581,0.049192247,0.001056671,-0.229426002,-2656.011718750,0.001037598,-0.000083923,2167,81216.375,19,3,1,0,5598.375000000,1.753316072,0.053257895,0.000308990,2.031661680,-2656.169921875,0.000915527,0.000148773,2167,86932.187,20,2,1,0,11314.187500000,1.338137581,0.053485596,0.000810623,0.016106798,-2656.033203125,0.001037598,0.000049591,2167,92471.875,21,4,1,0,16853.875000000,0.905492081,0.048149620,0.000671387,2.711982159,-2655.875000000,0.001098633,0.000225067,2167,97225.437,22,-3,1,0,21607.437500000,0.566332524,0.051370380,0.002092361,0.380906604,-2656.091796875,0.001159668,0.000122070,2167,103403.781,23,3,1,0,27785.781250000,0.114991634,0.051142680,0.000539780,1.610679827,-2656.626953125,0.001098633,0.000007629,2167,107403.343,24,2,1,0,31785.343750000,-0.171967635,0.033052492,0.000456810,-2.399433574,-2656.039062500,0.001037598,-0.000049591*6dee109c\r\n";

   pclMyFilter->IncludeMessageName("BESTPOS");
   pclMyFilter->IncludeMessageName("RANGE");

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(range_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(version_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(gloalmanac_log))));

   pclMyFilter->InvertMessageNameFilter(true);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(range_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(version_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(gloalmanac_log))));
}

TEST_F(FilterTest, MEASUREMENT_SOURCE_SECONDARY)
{
   const char* rawwaasframe_0_log = "#RAWWAASFRAMEA,COM2,0,77.5,SATTIME,1747,411899.000,00000020,58e4,11526;62,138,9,c6243a0581b555352c4056aae0103cf03daff2e00057ff7fdff8010180,62*b026c677\r\n";
   const char* rawwaasframe_1_log = "#RAWWAASFRAMEA_1,COM2,0,77.5,SATTIME,1747,411899.000,00000020,58e4,11526;62,138,9,c6243a0581b555352c4056aae0103cf03daff2e00057ff7fdff8010180,62*b026c677\r\n";
   const char* range_0_log        = "#RANGEA,COM1,0,49.0,FINESTEERING,2167,159740.000,02000000,5103,16248;115,20,0,23291390.821,0.051,-122397109.320305,0.011,-3214.414,44.0,16498.975,1810bc04,20,0,23291389.349,0.184,-95374376.486282,0.013,-2504.740,41.1,16492.754,11303c0b,29,0,20705108.295,0.022,-108806118.750397,0.005,-1620.698,51.2,14908.774,0810bc44,29,0,20705106.528,0.061,-84783996.294254,0.006,-1262.882,50.7,14902.754,01303c4b,29,0,20705107.069,0.025,-84783990.539554,0.005,-1262.882,51.3,14905.534,02309c4b,13,0,22941454.717,0.042,-120558183.531157,0.009,2111.590,45.7,3984.555,1810bc64,13,0,22941453.693,0.180,-93941456.741696,0.011,1645.396,41.2,3978.255,11303c6b,15,0,22752775.227,0.046,-119566687.005913,0.012,3112.730,44.8,3628.275,0810bc84,15,0,22752775.420,0.118,-93168858.308908,0.012,2425.506,44.9,3622.754,01303c8b,15,0,22752775.932,0.043,-93168851.550483,0.012,2425.505,46.7,3625.014,02309c8b,16,0,23437165.563,0.058,-123163154.592686,0.010,1667.743,42.8,3318.855,1810bca4,16,0,23437164.832,0.179,-95971289.711375,0.012,1299.540,41.3,3313.415,11303cab,18,0,20870226.835,0.024,-109673843.056839,0.005,1782.527,50.4,7988.975,1810bcc4,18,0,20870225.520,0.080,-85460161.951054,0.006,1388.983,48.3,7983.255,11303ccb,18,0,20870225.982,0.024,-85460166.200662,0.006,1388.983,51.9,7985.814,02309ccb,18,0,20870231.341,0.004,-81899348.012827,0.003,1331.096,55.4,7987.255,01d03cc4,5,0,20977730.840,0.024,-110238771.341810,0.006,-1850.683,50.6,12588.896,0810bd04,5,0,20977729.403,0.070,-85900356.288455,0.006,-1442.091,49.4,12583.755,01303d0b,5,0,20977730.127,0.030,-85900355.538492,0.006,-1442.091,49.8,12585.716,02309d0b,26,0,22753733.201,0.042,-119571694.561007,0.008,-494.798,45.7,7318.775,1810bd24,26,0,22753735.163,0.118,-93172767.333088,0.010,-385.557,44.9,7312.755,11303d2b,26,0,22753735.387,0.043,-93172769.582418,0.009,-385.557,46.7,7315.375,02309d2b,26,0,22753735.481,0.008,-89290576.088766,0.005,-369.472,50.1,7316.815,01d03d24,23,0,23067782.934,0.040,-121222050.181679,0.009,3453.274,46.0,3078.894,0810bd44,23,0,23067783.254,0.142,-94458759.273215,0.010,2690.865,43.3,3073.754,01303d4b,23,0,23067783.763,0.040,-94458764.522108,0.009,2690.865,47.3,3076.395,02309d4b,23,0,23067789.450,0.007,-90523004.360543,0.004,2578.883,51.2,3077.834,01d03d44,194,0,43027813.095,0.065,-226112681.899748,0.013,43.499,41.9,17178.695,1815be04,194,0,43027815.196,0.059,-176191709.875251,0.014,33.896,44.0,17173.014,02359e0b,194,0,43027817.865,0.014,-168850394.176443,0.007,32.406,45.4,17177.053,01d53e04,131,0,38480107.260,0.124,-202214296.438902,0.009,-0.922,46.2,292335.531,48023e84,133,0,38618703.555,0.119,-202942631.161186,0.007,0.421,46.7,916697.188,58023ec4,138,0,38495561.597,0.116,-202295515.714333,0.008,-4.752,46.8,292343.625,48023ee4,45,13,20655319.254,0.111,-110608334.938276,0.006,-1928.119,46.3,9728.839,18119f04,45,13,20655320.731,0.021,-86028727.119001,0.006,-1499.649,45.9,9724.239,10b13f0b,45,13,20655321.099,0.092,-86028721.367030,0.006,-1499.649,46.1,9725.238,10319f0b,53,6,23361335.550,0.284,-124792043.406215,0.017,1741.893,38.1,444.840,08119f24,53,6,23361340.271,0.098,-97060514.793435,0.017,1354.807,32.6,444.741,00b13f2b,53,6,23361339.423,0.393,-97060517.036654,0.018,1354.806,33.5,444.801,10319f2b,60,10,20724752.466,0.106,-110863493.957380,0.007,-2492.451,46.7,16549.037,18019f44,39,3,23534282.253,0.169,-125583452.109842,0.012,4608.280,42.6,557.668,08119f84,39,3,23534291.023,0.027,-97676038.550992,0.013,3584.223,43.8,552.119,10b13f8b,39,3,23534290.639,0.108,-97676048.806539,0.013,3584.223,44.7,552.959,10319f8b,61,9,19285134.504,0.086,-103126338.171372,0.005,228.766,48.6,11128.199,08119fa4,61,9,19285138.043,0.020,-80209402.132964,0.005,177.929,46.3,11124.118,00b13fab,61,9,19285138.376,0.084,-80209411.390794,0.005,177.929,46.9,11125.037,00319fab,52,7,22348227.548,0.137,-119422164.171132,0.008,-1798.230,44.4,7458.668,08119fc4,52,7,22348232.044,0.025,-92883929.564420,0.008,-1398.625,44.4,7453.898,00b13fcb,52,7,22348232.124,0.104,-92883930.822797,0.008,-1398.624,45.0,7455.038,10319fcb,54,11,21518220.426,0.169,-115148393.440041,0.010,3262.249,42.6,3877.098,18119fe4,54,11,21518225.678,0.025,-89559888.534930,0.010,2537.306,44.6,3871.818,00b13feb,54,11,21518226.376,0.107,-89559882.794247,0.010,2537.307,44.8,3872.818,10319feb,51,0,23917426.780,0.130,-127493324.706900,0.008,-3976.867,44.9,13028.379,08119c04,51,0,23917434.944,0.031,-99161492.405944,0.010,-3093.121,42.6,13024.238,10b13c0b,51,0,23917434.780,0.126,-99161488.657552,0.010,-3093.121,43.4,13025.178,00319c0b,38,8,19851538.779,0.107,-106117893.493769,0.007,1849.414,46.6,6208.818,08119c24,38,8,19851544.763,0.031,-82536182.471767,0.007,1438.434,42.6,6204.118,00b13c2b,38,8,19851543.771,0.124,-82536181.722576,0.007,1438.434,43.6,6205.038,00319c2b,25,0,27861125.116,0.078,-146411169.405727,0.011,-3136.592,43.2,21188.543,08539cc4,25,0,27861133.366,0.009,-109333028.194067,0.005,-2342.203,49.0,21186.443,01933cc4,25,0,27861129.463,0.011,-112185182.897162,0.006,-2403.344,47.0,21186.443,02333cc4,25,0,27861129.580,0.007,-110759098.611107,0.006,-2372.787,50.8,21186.164,02933cc4,4,0,25274631.488,0.038,-132819124.897734,0.006,997.361,49.6,7638.783,08539ce4,4,0,25274635.181,0.007,-99183140.380658,0.004,744.803,50.8,7636.565,01933ce4,4,0,25274631.890,0.007,-101770517.169783,0.004,764.254,50.9,7636.444,02333ce4,4,0,25274631.708,0.005,-100476824.840813,0.004,754.545,53.6,7636.363,02933ce4,12,0,26373649.887,0.092,-138594449.111813,0.012,-2565.281,41.8,26740.730,08539d04,12,0,26373653.619,0.019,-103495853.823161,0.008,-1915.449,42.6,26738.648,01933d04,12,0,26373650.081,0.023,-106195738.067164,0.011,-1965.500,41.1,26738.449,02333d04,12,0,26373650.251,0.015,-104845791.009488,0.010,-1940.442,44.6,26738.371,02933d04,11,0,22137124.256,0.039,-116331408.305147,0.015,-1200.216,49.2,19415.590,08539d24,11,0,22137125.344,0.008,-86870883.829203,0.011,-896.289,49.8,19413.172,01933d24,11,0,22137122.146,0.008,-89137066.170706,0.012,-919.719,49.6,19413.248,02333d24,11,0,22137121.891,0.006,-88003971.568373,0.011,-908.028,52.4,19413.172,02933d24,30,0,25928558.680,0.072,-136255508.290211,0.010,743.664,43.9,3960.112,08539d44,30,0,25928564.638,0.011,-101749279.487957,0.006,555.328,47.5,4752.748,01933d44,30,0,25928561.460,0.010,-104403592.595320,0.005,569.759,48.1,4753.047,02333d44,30,0,25928561.332,0.008,-103076425.609137,0.006,562.539,50.5,4752.767,02933d44,2,0,25889111.981,0.043,-136048218.157560,0.006,-1792.931,48.4,12654.424,08539d64,2,0,25889117.006,0.009,-101594476.864922,0.005,-1338.866,48.7,12652.444,01933d64,2,0,25889114.168,0.009,-104244753.680674,0.004,-1373.765,49.5,12651.978,02333d64,2,0,25889113.739,0.007,-102919609.843844,0.005,-1356.370,51.8,12651.943,02933d64,19,0,27039623.380,0.118,-142094196.888887,0.015,-1878.632,39.7,11125.104,08539d84,19,0,27039628.887,0.020,-106109319.847355,0.010,-1402.842,41.9,11123.043,01933d84,19,0,27039625.153,0.024,-108877382.476710,0.011,-1439.341,40.6,11122.757,02333d84,19,0,27039625.337,0.016,-107493348.232960,0.010,-1421.103,44.1,11122.765,02933d84,36,0,23927504.603,0.030,-125739945.419298,0.005,1241.596,51.7,11037.264,08539da4,36,0,23927510.217,0.006,-93896767.646843,0.004,927.156,52.9,11035.164,01933da4,36,0,23927507.273,0.006,-96346233.181780,0.004,951.361,53.2,11035.376,02333da4,36,0,23927507.057,0.004,-95121494.979676,0.004,939.285,55.8,11031.874,02933da4,9,0,24890379.004,0.046,-130799846.144936,0.007,3052.621,47.8,2955.889,08539dc4,9,0,24890384.304,0.009,-97675250.055577,0.005,2279.540,49.1,2953.828,01933dc4,9,0,24890381.065,0.008,-100223286.825938,0.004,2338.979,50.0,2953.762,02333dc4,9,0,24890381.366,0.006,-98949262.506583,0.005,2309.297,52.2,2949.700,02933dc4,23,0,26593863.945,0.036,-138481231.000933,0.010,-48.553,44.1,2628.888,08149ec4,23,0,26593862.563,0.010,-104360035.310223,0.005,-36.590,48.2,2623.647,41343ec4,34,0,23330414.273,0.017,-121487628.857801,0.005,2280.558,50.6,6539.069,58149ee4,34,0,23330415.641,0.008,-91553618.939049,0.005,1718.641,50.1,6533.770,41343ee4,35,0,24822913.452,0.024,-129259432.414616,0.007,-2925.143,47.4,23499.049,58149f04,35,0,24822915.980,0.012,-97410461.716286,0.006,-2204.368,46.7,23493.830,41343f04,11,0,24964039.739,0.052,-129994328.361984,0.014,2939.333,40.8,2708.970,18149f24,11,0,24964038.060,0.022,-100519869.959755,0.006,2272.851,48.3,2708.810,00349f24,19,0,23905947.282,0.033,-124484578.888819,0.009,-2342.726,44.8,13489.051,18149f44,19,0,23905949.046,0.008,-93812119.376225,0.005,-1765.479,50.1,13483.831,41343f44,21,0,24577306.170,0.027,-127980528.823414,0.008,3242.344,46.7,3439.068,18149f84,21,0,24577307.993,0.008,-96446682.849511,0.005,2443.502,49.7,3433.828,41343f84,22,0,22438270.920,0.015,-116842012.781567,0.005,729.096,51.5,8979.049,18149fa4,22,0,22438269.274,0.005,-88052653.428423,0.003,549.506,54.1,8973.770,41343fa4,44,0,21553538.984,0.014,-112234979.640419,0.005,-679.127,52.1,15439.131,48149ca4,44,0,21553540.824,0.005,-84580779.869687,0.003,-511.716,53.6,15433.829,41343ca4,57,0,26771391.610,0.021,-139405685.309616,0.007,-2069.940,48.5,20196.455,48049d04,12,0,21542689.063,0.021,-112178498.767984,0.006,952.964,48.8,11229.051,18149d24,12,0,21542686.409,0.013,-86743545.297369,0.004,736.976,52.6,11228.890,00349d24,25,0,26603375.741,0.069,-138530755.415895,0.019,-2155.462,38.4,9789.050,18149d44,25,0,26603380.238,0.015,-104397363.013083,0.007,-1624.205,44.7,9783.829,41343d44*5e9785bd\r\n";
   const char* range_1_log        = "#RANGEA_1,COM1,0,49.0,FINESTEERING,2167,159740.000,02000000,5103,16248;115,20,0,23291390.821,0.051,-122397109.320305,0.011,-3214.414,44.0,16498.975,1810bc04,20,0,23291389.349,0.184,-95374376.486282,0.013,-2504.740,41.1,16492.754,11303c0b,29,0,20705108.295,0.022,-108806118.750397,0.005,-1620.698,51.2,14908.774,0810bc44,29,0,20705106.528,0.061,-84783996.294254,0.006,-1262.882,50.7,14902.754,01303c4b,29,0,20705107.069,0.025,-84783990.539554,0.005,-1262.882,51.3,14905.534,02309c4b,13,0,22941454.717,0.042,-120558183.531157,0.009,2111.590,45.7,3984.555,1810bc64,13,0,22941453.693,0.180,-93941456.741696,0.011,1645.396,41.2,3978.255,11303c6b,15,0,22752775.227,0.046,-119566687.005913,0.012,3112.730,44.8,3628.275,0810bc84,15,0,22752775.420,0.118,-93168858.308908,0.012,2425.506,44.9,3622.754,01303c8b,15,0,22752775.932,0.043,-93168851.550483,0.012,2425.505,46.7,3625.014,02309c8b,16,0,23437165.563,0.058,-123163154.592686,0.010,1667.743,42.8,3318.855,1810bca4,16,0,23437164.832,0.179,-95971289.711375,0.012,1299.540,41.3,3313.415,11303cab,18,0,20870226.835,0.024,-109673843.056839,0.005,1782.527,50.4,7988.975,1810bcc4,18,0,20870225.520,0.080,-85460161.951054,0.006,1388.983,48.3,7983.255,11303ccb,18,0,20870225.982,0.024,-85460166.200662,0.006,1388.983,51.9,7985.814,02309ccb,18,0,20870231.341,0.004,-81899348.012827,0.003,1331.096,55.4,7987.255,01d03cc4,5,0,20977730.840,0.024,-110238771.341810,0.006,-1850.683,50.6,12588.896,0810bd04,5,0,20977729.403,0.070,-85900356.288455,0.006,-1442.091,49.4,12583.755,01303d0b,5,0,20977730.127,0.030,-85900355.538492,0.006,-1442.091,49.8,12585.716,02309d0b,26,0,22753733.201,0.042,-119571694.561007,0.008,-494.798,45.7,7318.775,1810bd24,26,0,22753735.163,0.118,-93172767.333088,0.010,-385.557,44.9,7312.755,11303d2b,26,0,22753735.387,0.043,-93172769.582418,0.009,-385.557,46.7,7315.375,02309d2b,26,0,22753735.481,0.008,-89290576.088766,0.005,-369.472,50.1,7316.815,01d03d24,23,0,23067782.934,0.040,-121222050.181679,0.009,3453.274,46.0,3078.894,0810bd44,23,0,23067783.254,0.142,-94458759.273215,0.010,2690.865,43.3,3073.754,01303d4b,23,0,23067783.763,0.040,-94458764.522108,0.009,2690.865,47.3,3076.395,02309d4b,23,0,23067789.450,0.007,-90523004.360543,0.004,2578.883,51.2,3077.834,01d03d44,194,0,43027813.095,0.065,-226112681.899748,0.013,43.499,41.9,17178.695,1815be04,194,0,43027815.196,0.059,-176191709.875251,0.014,33.896,44.0,17173.014,02359e0b,194,0,43027817.865,0.014,-168850394.176443,0.007,32.406,45.4,17177.053,01d53e04,131,0,38480107.260,0.124,-202214296.438902,0.009,-0.922,46.2,292335.531,48023e84,133,0,38618703.555,0.119,-202942631.161186,0.007,0.421,46.7,916697.188,58023ec4,138,0,38495561.597,0.116,-202295515.714333,0.008,-4.752,46.8,292343.625,48023ee4,45,13,20655319.254,0.111,-110608334.938276,0.006,-1928.119,46.3,9728.839,18119f04,45,13,20655320.731,0.021,-86028727.119001,0.006,-1499.649,45.9,9724.239,10b13f0b,45,13,20655321.099,0.092,-86028721.367030,0.006,-1499.649,46.1,9725.238,10319f0b,53,6,23361335.550,0.284,-124792043.406215,0.017,1741.893,38.1,444.840,08119f24,53,6,23361340.271,0.098,-97060514.793435,0.017,1354.807,32.6,444.741,00b13f2b,53,6,23361339.423,0.393,-97060517.036654,0.018,1354.806,33.5,444.801,10319f2b,60,10,20724752.466,0.106,-110863493.957380,0.007,-2492.451,46.7,16549.037,18019f44,39,3,23534282.253,0.169,-125583452.109842,0.012,4608.280,42.6,557.668,08119f84,39,3,23534291.023,0.027,-97676038.550992,0.013,3584.223,43.8,552.119,10b13f8b,39,3,23534290.639,0.108,-97676048.806539,0.013,3584.223,44.7,552.959,10319f8b,61,9,19285134.504,0.086,-103126338.171372,0.005,228.766,48.6,11128.199,08119fa4,61,9,19285138.043,0.020,-80209402.132964,0.005,177.929,46.3,11124.118,00b13fab,61,9,19285138.376,0.084,-80209411.390794,0.005,177.929,46.9,11125.037,00319fab,52,7,22348227.548,0.137,-119422164.171132,0.008,-1798.230,44.4,7458.668,08119fc4,52,7,22348232.044,0.025,-92883929.564420,0.008,-1398.625,44.4,7453.898,00b13fcb,52,7,22348232.124,0.104,-92883930.822797,0.008,-1398.624,45.0,7455.038,10319fcb,54,11,21518220.426,0.169,-115148393.440041,0.010,3262.249,42.6,3877.098,18119fe4,54,11,21518225.678,0.025,-89559888.534930,0.010,2537.306,44.6,3871.818,00b13feb,54,11,21518226.376,0.107,-89559882.794247,0.010,2537.307,44.8,3872.818,10319feb,51,0,23917426.780,0.130,-127493324.706900,0.008,-3976.867,44.9,13028.379,08119c04,51,0,23917434.944,0.031,-99161492.405944,0.010,-3093.121,42.6,13024.238,10b13c0b,51,0,23917434.780,0.126,-99161488.657552,0.010,-3093.121,43.4,13025.178,00319c0b,38,8,19851538.779,0.107,-106117893.493769,0.007,1849.414,46.6,6208.818,08119c24,38,8,19851544.763,0.031,-82536182.471767,0.007,1438.434,42.6,6204.118,00b13c2b,38,8,19851543.771,0.124,-82536181.722576,0.007,1438.434,43.6,6205.038,00319c2b,25,0,27861125.116,0.078,-146411169.405727,0.011,-3136.592,43.2,21188.543,08539cc4,25,0,27861133.366,0.009,-109333028.194067,0.005,-2342.203,49.0,21186.443,01933cc4,25,0,27861129.463,0.011,-112185182.897162,0.006,-2403.344,47.0,21186.443,02333cc4,25,0,27861129.580,0.007,-110759098.611107,0.006,-2372.787,50.8,21186.164,02933cc4,4,0,25274631.488,0.038,-132819124.897734,0.006,997.361,49.6,7638.783,08539ce4,4,0,25274635.181,0.007,-99183140.380658,0.004,744.803,50.8,7636.565,01933ce4,4,0,25274631.890,0.007,-101770517.169783,0.004,764.254,50.9,7636.444,02333ce4,4,0,25274631.708,0.005,-100476824.840813,0.004,754.545,53.6,7636.363,02933ce4,12,0,26373649.887,0.092,-138594449.111813,0.012,-2565.281,41.8,26740.730,08539d04,12,0,26373653.619,0.019,-103495853.823161,0.008,-1915.449,42.6,26738.648,01933d04,12,0,26373650.081,0.023,-106195738.067164,0.011,-1965.500,41.1,26738.449,02333d04,12,0,26373650.251,0.015,-104845791.009488,0.010,-1940.442,44.6,26738.371,02933d04,11,0,22137124.256,0.039,-116331408.305147,0.015,-1200.216,49.2,19415.590,08539d24,11,0,22137125.344,0.008,-86870883.829203,0.011,-896.289,49.8,19413.172,01933d24,11,0,22137122.146,0.008,-89137066.170706,0.012,-919.719,49.6,19413.248,02333d24,11,0,22137121.891,0.006,-88003971.568373,0.011,-908.028,52.4,19413.172,02933d24,30,0,25928558.680,0.072,-136255508.290211,0.010,743.664,43.9,3960.112,08539d44,30,0,25928564.638,0.011,-101749279.487957,0.006,555.328,47.5,4752.748,01933d44,30,0,25928561.460,0.010,-104403592.595320,0.005,569.759,48.1,4753.047,02333d44,30,0,25928561.332,0.008,-103076425.609137,0.006,562.539,50.5,4752.767,02933d44,2,0,25889111.981,0.043,-136048218.157560,0.006,-1792.931,48.4,12654.424,08539d64,2,0,25889117.006,0.009,-101594476.864922,0.005,-1338.866,48.7,12652.444,01933d64,2,0,25889114.168,0.009,-104244753.680674,0.004,-1373.765,49.5,12651.978,02333d64,2,0,25889113.739,0.007,-102919609.843844,0.005,-1356.370,51.8,12651.943,02933d64,19,0,27039623.380,0.118,-142094196.888887,0.015,-1878.632,39.7,11125.104,08539d84,19,0,27039628.887,0.020,-106109319.847355,0.010,-1402.842,41.9,11123.043,01933d84,19,0,27039625.153,0.024,-108877382.476710,0.011,-1439.341,40.6,11122.757,02333d84,19,0,27039625.337,0.016,-107493348.232960,0.010,-1421.103,44.1,11122.765,02933d84,36,0,23927504.603,0.030,-125739945.419298,0.005,1241.596,51.7,11037.264,08539da4,36,0,23927510.217,0.006,-93896767.646843,0.004,927.156,52.9,11035.164,01933da4,36,0,23927507.273,0.006,-96346233.181780,0.004,951.361,53.2,11035.376,02333da4,36,0,23927507.057,0.004,-95121494.979676,0.004,939.285,55.8,11031.874,02933da4,9,0,24890379.004,0.046,-130799846.144936,0.007,3052.621,47.8,2955.889,08539dc4,9,0,24890384.304,0.009,-97675250.055577,0.005,2279.540,49.1,2953.828,01933dc4,9,0,24890381.065,0.008,-100223286.825938,0.004,2338.979,50.0,2953.762,02333dc4,9,0,24890381.366,0.006,-98949262.506583,0.005,2309.297,52.2,2949.700,02933dc4,23,0,26593863.945,0.036,-138481231.000933,0.010,-48.553,44.1,2628.888,08149ec4,23,0,26593862.563,0.010,-104360035.310223,0.005,-36.590,48.2,2623.647,41343ec4,34,0,23330414.273,0.017,-121487628.857801,0.005,2280.558,50.6,6539.069,58149ee4,34,0,23330415.641,0.008,-91553618.939049,0.005,1718.641,50.1,6533.770,41343ee4,35,0,24822913.452,0.024,-129259432.414616,0.007,-2925.143,47.4,23499.049,58149f04,35,0,24822915.980,0.012,-97410461.716286,0.006,-2204.368,46.7,23493.830,41343f04,11,0,24964039.739,0.052,-129994328.361984,0.014,2939.333,40.8,2708.970,18149f24,11,0,24964038.060,0.022,-100519869.959755,0.006,2272.851,48.3,2708.810,00349f24,19,0,23905947.282,0.033,-124484578.888819,0.009,-2342.726,44.8,13489.051,18149f44,19,0,23905949.046,0.008,-93812119.376225,0.005,-1765.479,50.1,13483.831,41343f44,21,0,24577306.170,0.027,-127980528.823414,0.008,3242.344,46.7,3439.068,18149f84,21,0,24577307.993,0.008,-96446682.849511,0.005,2443.502,49.7,3433.828,41343f84,22,0,22438270.920,0.015,-116842012.781567,0.005,729.096,51.5,8979.049,18149fa4,22,0,22438269.274,0.005,-88052653.428423,0.003,549.506,54.1,8973.770,41343fa4,44,0,21553538.984,0.014,-112234979.640419,0.005,-679.127,52.1,15439.131,48149ca4,44,0,21553540.824,0.005,-84580779.869687,0.003,-511.716,53.6,15433.829,41343ca4,57,0,26771391.610,0.021,-139405685.309616,0.007,-2069.940,48.5,20196.455,48049d04,12,0,21542689.063,0.021,-112178498.767984,0.006,952.964,48.8,11229.051,18149d24,12,0,21542686.409,0.013,-86743545.297369,0.004,736.976,52.6,11228.890,00349d24,25,0,26603375.741,0.069,-138530755.415895,0.019,-2155.462,38.4,9789.050,18149d44,25,0,26603380.238,0.015,-104397363.013083,0.007,-1624.205,44.7,9783.829,41343d44*5e9785bd\r\n";

   pclMyFilter->IncludeMessageId(43, HEADERFORMAT::ASCII, MEASUREMENT_SOURCE::SECONDARY);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(range_1_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(rawwaasframe_1_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(range_0_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(rawwaasframe_0_log))));
}

TEST_F(FilterTest, DECIMATION)
{
   const char* bestpos_log_10Hz1 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.100,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_10Hz2 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.200,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_10Hz3 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.300,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_10Hz4 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.400,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_10Hz5 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.500,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_20Hz1 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.150,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_20Hz2 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.250,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_20Hz3 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.350,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_20Hz4 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.450,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";
   const char* bestpos_log_20Hz5 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,324435.550,02000020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043043561,-114.03067194872,1097.5245,-17.0000,WGS84,0.9659,0.6980,1.7027,\"\",0.000,0.000,36,32,32,32,00,06,39,33*3bed5c1b\r\n";

   pclMyFilter->SetIncludeDecimation(0.1);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz1))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz2))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz3))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz4))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz5))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz3))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz4))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz5))));

   pclMyFilter->InvertDecimationFilter(true);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz3))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz4))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_10Hz5))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz1))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz2))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz3))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz4))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestpos_log_20Hz5))));
}

TEST_F(FilterTest, MIX_1)
{
   const char* bestposa_log_1 = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313698.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* bestposa_log_2 = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313700.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* bestposa_log_3 = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2179,313702.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* bestposa_log_4 = "#BESTPOSA,COM1,0,8.0,COARSESTEERING,2180,313700.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* versiona_log_1 = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   const char* versiona_log_2 = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   const char* trackstata_log = "#TRACKSTATA,COM1,0,58.0,FINESTEERING,2180,313700.000,02000000,457c,16248;SOL_COMPUTED,WAAS,5.0,235,2,0,0810bc04,20999784.925,770.496,49.041,8473.355,0.228,GOOD,0.975,2,0,01303c0b,20999781.972,600.387,49.021,8466.896,0.000,OBSL2,0.000,0,0,02208000,0.000,-0.004,0.000,0.000,0.000,NA,0.000,0,0,01c02000,0.000,0.000,0.000,0.000,0.000,NA,0.000,20,0,0810bc24,24120644.940,3512.403,42.138,1624.974,0.464,GOOD,0.588,20,0,01303c2b,24120645.042,2736.937,39.553,1619.755,0.000,OBSL2,0.000,0,0,02208020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02020,0.000,0.000,0.000,0.000,0.000,NA,0.000,6,0,0810bc44,20727107.371,-1161.109,50.325,11454.975,-0.695,GOOD,0.979,6,0,01303c4b,20727108.785,-904.761,50.213,11448.915,0.000,OBSL2,0.000,6,0,02309c4b,20727109.344,-904.761,52.568,11451.815,0.000,OBSL2,0.000,6,0,01d03c44,20727110.520,-867.070,55.259,11453.455,0.000,OBSL5,0.000,29,0,0810bc64,25296813.545,3338.614,43.675,114.534,-0.170,GOOD,0.206,29,0,01303c6b,25296814.118,2601.518,39.636,109.254,0.000,OBSL2,0.000,29,0,02309c6b,25296814.580,2601.517,40.637,111.114,0.000,OBSL2,0.000,0,0,01c02060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02080,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,02208080,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02080,0.000,0.000,0.000,0.000,0.000,NA,0.000,19,0,0810bca4,22493227.199,-3020.625,44.911,18244.973,0.411,GOOD,0.970,19,0,01303cab,22493225.215,-2353.736,44.957,18239.754,0.000,OBSL2,0.000,0,0,022080a0,0.000,-0.006,0.000,0.000,0.000,NA,0.000,0,0,01c020a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,24,0,0810bcc4,23856706.090,-3347.685,43.417,15187.116,-0.358,GOOD,0.957,24,0,01303ccb,23856708.306,-2608.588,43.207,15181.256,0.000,OBSL2,0.000,24,0,02309ccb,23856708.614,-2608.588,46.741,15183.815,0.000,OBSL2,0.000,24,0,01d03cc4,23856711.245,-2499.840,50.038,15185.256,0.000,OBSL5,0.000,25,0,1810bce4,21953295.423,2746.317,46.205,4664.936,0.322,GOOD,0.622,25,0,11303ceb,21953296.482,2139.988,45.623,4658.756,0.000,OBSL2,0.000,25,0,02309ceb,21953296.899,2139.988,47.584,4661.796,0.000,OBSL2,0.000,25,0,01d03ce4,21953298.590,2050.845,51.711,4662.976,0.000,OBSL5,0.000,0,0,0000a100,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02100,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,02208100,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02100,0.000,0.000,0.000,0.000,0.000,NA,0.000,17,0,1810bd24,24833573.179,-3002.286,43.809,21504.975,-0.219,GOOD,0.903,17,0,11303d2b,24833573.345,-2339.444,42.894,21499.256,0.000,OBSL2,0.000,17,0,02309d2b,24833573.677,-2339.444,44.238,21501.717,0.000,OBSL2,0.000,0,0,01c02120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02140,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,02208140,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02140,0.000,0.000,0.000,0.000,0.000,NA,0.000,12,0,0810bd64,20275478.792,742.751,50.336,9634.855,0.166,GOOD,0.977,12,0,01303d6b,20275477.189,578.767,50.042,9629.756,0.000,OBSL2,0.000,12,0,02309d6b,20275477.555,578.767,51.012,9631.516,0.000,OBSL2,0.000,0,0,01c02160,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02180,0.000,0.002,0.000,0.000,0.000,NA,0.000,0,0,02208180,0.000,0.003,0.000,0.000,0.000,NA,0.000,0,0,01c02180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021a0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,022081a0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021c0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,022081c0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021e0,0.000,0.001,0.000,0.000,0.000,NA,0.000,0,0,022081e0,0.000,0.003,0.000,0.000,0.000,NA,0.000,0,0,01c021e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,194,0,0815be04,43478223.927,63.042,38.698,2382.214,0.000,NODIFFCORR,0.000,194,0,02359e0b,43478226.941,49.122,44.508,2378.714,0.000,OBSL2,0.000,194,0,01d53e04,43478228.121,47.080,43.958,2380.253,0.000,OBSL5,0.000,0,0,0005a220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258240,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c52240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52260,0.000,0.000,0.000,0.000,0.000,NA,0.000,131,0,48023e84,38480992.384,-0.167,45.356,471155.406,0.000,LOCKEDOUT,0.000,135,0,58023ea4,38553658.881,3.771,44.648,4.449,0.000,NODIFFCORR,0.000,133,0,58023ec4,38624746.161,1.065,45.618,471153.219,0.000,LOCKEDOUT,0.000,138,0,48023ee4,38493033.873,0.953,45.833,898498.250,0.000,LOCKEDOUT,0.000,55,4,18119f04,21580157.377,3208.835,44.921,3584.798,0.000,NODIFFCORR,0.000,55,4,00b13f0b,21580163.823,2495.762,45.078,3580.119,0.000,OBSL2,0.000,55,4,10319f0b,21580163.635,2495.762,45.682,3581.038,0.000,OBSL2,0.000,45,13,08119f24,23088997.031,-313.758,44.105,4273.538,0.000,NODIFFCORR,0.000,45,13,00b13f2b,23088998.989,-244.036,42.927,4267.818,0.000,OBSL2,0.000,45,13,00319f2b,23088999.269,-244.036,43.297,4268.818,0.000,OBSL2,0.000,54,11,18119f44,19120160.469,178.235,50.805,9344.977,0.000,NODIFFCORR,0.000,54,11,00b13f4b,19120162.255,138.627,46.584,9339.897,0.000,OBSL2,0.000,54,11,00319f4b,19120162.559,138.627,47.049,9340.818,0.000,OBSL2,0.000,0,0,00018360,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12360,0.000,0.004,0.000,0.000,0.000,NA,0.000,0,0,00218360,0.000,0.004,0.000,0.000,0.000,NA,0.000,0,0,00018380,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12380,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218380,0.000,0.000,0.000,0.000,0.000,NA,0.000,53,6,18119fa4,21330036.443,3045.661,43.167,3862.756,0.000,NODIFFCORR,0.000,53,6,00b13fab,21330040.203,2368.849,41.759,3858.039,0.000,OBSL2,0.000,53,6,00319fab,21330039.119,2368.850,42.691,3859.038,0.000,OBSL2,0.000,38,8,18119fc4,22996582.245,2427.724,41.817,2014.338,0.000,NODIFFCORR,0.000,38,8,10b13fcb,22996590.440,1888.231,35.968,2010.119,0.000,OBSL2,0.000,38,8,10319fcb,22996589.454,1888.230,36.755,2011.038,0.000,OBSL2,0.000,52,7,08119fe4,19520740.266,-1275.394,50.736,10712.179,0.000,NODIFFCORR,0.000,52,7,00b13feb,19520744.583,-991.974,47.931,10708.038,0.000,OBSL2,0.000,52,7,10319feb,19520744.527,-991.974,48.251,10709.038,0.000,OBSL2,0.000,51,0,18119c04,22302364.417,-4314.112,43.692,16603.602,0.000,NODIFFCORR,0.000,51,0,00b13c0b,22302371.827,-3355.424,45.975,16603.580,0.000,OBSL2,0.000,51,0,00319c0b,22302371.325,-3355.424,46.904,16603.502,0.000,OBSL2,0.000,61,9,08119c24,21163674.206,-3198.898,47.898,14680.979,0.000,NODIFFCORR,0.000,61,9,10b13c2b,21163677.196,-2488.033,44.960,14675.897,0.000,OBSL2,0.000,61,9,00319c2b,21163677.300,-2488.033,45.628,14676.737,0.000,OBSL2,0.000,0,0,00018040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00018060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12060,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,00218060,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,00018080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a120a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,002180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004380c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,21,0,08539ce4,25416828.004,2077.626,46.584,6337.363,0.000,NODIFFCORR,0.000,21,0,01933ce4,25416833.286,1551.460,49.589,6335.164,0.000,OBSE5,0.000,21,0,02333ce4,25416829.717,1591.910,50.226,6335.176,0.000,OBSE5,0.000,21,0,02933ce4,25416829.814,1571.722,52.198,6334.944,0.000,OBSE5,0.000,27,0,08539d04,23510780.996,-707.419,51.721,16182.524,0.000,NODIFFCORR,0.000,27,0,01933d04,23510785.247,-528.262,53.239,16180.444,0.000,OBSE5,0.000,27,0,02333d04,23510781.458,-542.015,53.731,16180.243,0.000,OBSE5,0.000,27,0,02933d04,23510781.960,-535.149,55.822,16180.165,0.000,OBSE5,0.000,0,0,00438120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832120,0.000,0.000,0.000,0.000,0.000,NA,0.000,15,0,08539d44,23034423.020,183.445,51.283,11971.245,0.000,NODIFFCORR,0.000,15,0,01933d44,23034428.761,136.945,53.293,11969.243,0.000,OBSE5,0.000,15,0,02333d44,23034425.379,140.546,53.897,11969.245,0.000,OBSE5,0.000,15,0,02933d44,23034425.436,138.742,55.909,11968.946,0.000,OBSE5,0.000,13,0,08539d64,25488681.795,2565.988,46.632,4828.445,0.000,NODIFFCORR,0.000,13,0,01933d64,25488687.213,1916.182,47.753,4826.243,0.000,OBSE5,0.000,13,0,02333d64,25488683.967,1966.148,50.045,4826.243,0.000,OBSE5,0.000,13,0,02933d64,25488684.398,1941.169,51.348,4826.165,0.000,OBSE5,0.000,0,0,00438180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,30,0,08539de4,25532715.149,-2938.485,46.289,26421.467,0.000,NODIFFCORR,0.000,30,0,01933de4,25532721.371,-2194.317,49.285,26419.447,0.000,OBSE5,0.000,30,0,02333de4,25532718.174,-2251.520,50.681,26419.447,0.000,OBSE5,0.000,30,0,02933de4,25532717.843,-2222.952,52.291,26419.166,0.000,OBSE5,0.000,0,0,00438200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004382a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,41,0,48149ec4,26228546.068,2731.326,43.047,1244.968,0.000,NODIFFCORR,0.000,41,0,41343ec4,26228560.733,2058.212,46.309,1239.648,0.000,NA,0.000,27,0,08149ee4,21470141.903,-686.571,51.408,13695.229,0.000,NODIFFCORR,0.000,27,0,41343ee4,21470143.417,-517.430,52.724,13690.050,0.000,NA,0.000,6,0,08149f04,40334269.953,-663.889,38.200,12755.121,0.000,NODIFFCORR,0.000,6,0,00349f04,40334265.525,-513.549,39.333,12754.961,0.000,OBSB2,0.000,16,0,08149f24,40591561.211,-689.953,40.783,11755.120,0.000,NODIFFCORR,0.000,16,0,00349f24,40591562.100,-533.388,39.928,11754.960,0.000,OBSB2,0.000,39,0,58149f44,40402963.125,-730.398,41.019,11015.042,0.000,NODIFFCORR,0.000,39,0,41343f44,40402964.083,-550.456,43.408,11009.821,0.000,NA,0.000,30,0,18149f64,22847646.673,2123.913,50.266,6625.051,0.000,NODIFFCORR,0.000,30,0,41343f64,22847649.151,1600.605,49.656,6619.991,0.000,NA,0.000,7,0,08048381,0.000,2500.000,0.000,0.000,0.000,NA,0.000,7,0,08048381,0.000,-2500.000,0.000,0.000,0.000,NA,0.000,33,0,48149fa4,25666349.147,776.929,42.271,3835.148,0.000,NODIFFCORR,0.000,33,0,41343fa4,25666377.385,585.535,48.361,3697.589,0.000,NA,0.000,46,0,48149fc4,23048323.129,-2333.170,49.345,15915.131,0.000,NODIFFCORR,0.000,46,0,41343fc4,23048329.413,-1758.350,52.408,15909.830,0.000,NA,0.000,18,0,080483e1,0.000,4000.000,0.000,0.000,0.000,NA,0.000,18,0,080483e1,0.000,-500.000,0.000,0.000,0.000,NA,0.000,45,0,48149c04,26221109.945,2965.644,44.864,435.050,0.000,NODIFFCORR,0.000,45,0,41343c04,26221119.956,2234.910,47.292,429.831,0.000,NA,0.000,36,0,58149c24,23277715.056,700.443,48.907,8015.069,0.000,NODIFFCORR,0.000,36,0,41343c24,23277723.101,527.848,51.167,8009.829,0.000,NA,0.000,52,0,08048041,0.000,1667.000,0.000,0.000,0.000,NA,0.000,52,0,08048041,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,49,0,08048061,0.000,5832.000,0.000,0.000,0.000,NA,0.000,49,0,08048061,0.000,-4999.000,0.000,0.000,0.000,NA,0.000,47,0,08048081,0.000,1000.000,0.000,0.000,0.000,NA,0.000,47,0,08048081,0.000,-500.000,0.000,0.000,0.000,NA,0.000,58,0,48049ca4,34894393.899,-3079.127,30.345,47.772,0.000,NODIFFCORR,0.000,58,0,012420a9,0.000,-2321.139,0.000,0.000,0.000,NA,0.000,14,0,08149cc4,25730238.361,-588.324,38.191,4795.070,0.000,NODIFFCORR,0.000,14,0,00349cc4,25730237.379,-454.787,44.427,4794.910,0.000,OBSB2,0.000,28,0,08149ce4,24802536.288,-2833.581,46.004,19865.129,0.000,NODIFFCORR,0.000,28,0,41343ce4,24802537.579,-2135.389,46.897,19859.650,0.000,NA,0.000,48,0,08048101,0.000,16000.000,0.000,0.000,0.000,NA,0.000,0,0,00248100,0.000,0.000,0.000,0.000,0.000,NA,0.000,9,0,08149d24,40753569.155,222.237,37.682,1784.493,0.000,NODIFFCORR,0.000,9,0,00349d24,40753568.209,171.813,41.501,4664.961,0.000,OBSB2,0.000,3,0,08848141,0.000,6000.000,0.000,0.000,0.000,NA,0.000,3,0,08848141,0.000,-11000.000,0.000,0.000,0.000,NA,0.000,1,0,08848161,0.000,4999.000,0.000,0.000,0.000,NA,0.000,1,0,08848161,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,6,0,0a670984,0.000,-301.833,36.924,1734607.250,0.000,NA,0.000,1,0,0a6709a4,0.000,83.304,43.782,558002.188,0.000,NA,0.000,0,0,026701c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,3,0,0a6701e1,0.000,419.842,0.000,0.000,0.000,NA,0.000,0,0,02670200,0.000,0.000,0.000,0.000,0.000,NA,0.000*c8963f70\r\n";
   unsigned const char bestposb_log[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA4, 0xB4, 0xAC, 0x07, 0xD8, 0x16, 0x6D, 0x08, 0x08, 0x40, 0x00, 0x02, 0xF6, 0xB1, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xD7, 0x03, 0xB0, 0x4C, 0xE5, 0x8E, 0x49, 0x40, 0x52, 0xC4, 0x26, 0xD1, 0x72, 0x82, 0x5C, 0xC0, 0x29, 0xCB, 0x10, 0xC7, 0x7A, 0xA2, 0x90, 0x40, 0x33, 0x33, 0x87, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xFA, 0x7E, 0xBA, 0x3F, 0x3F, 0x57, 0x83, 0x3F, 0xA9, 0xA4, 0x0A, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x16, 0x16, 0x16, 0x00, 0x06, 0x39, 0x33, 0x23, 0xC4, 0x89, 0x7A };
   unsigned const char sourcetableb_log[] = { 0xAA, 0x44, 0x12, 0x1C, 0x40, 0x05, 0x00, 0x20, 0x68, 0x00, 0x15, 0x00, 0x80, 0xB4, 0x74, 0x08, 0x00, 0x5B, 0x88, 0x0D, 0x20, 0x80, 0x00, 0x02, 0xDD, 0x71, 0x00, 0x80, 0x68, 0x65, 0x72, 0x61, 0x2E, 0x6E, 0x6F, 0x76, 0x61, 0x74, 0x65, 0x6C, 0x2E, 0x63, 0x6F, 0x6D, 0x3A, 0x32, 0x31, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x54, 0x52, 0x3B, 0x48, 0x79, 0x64, 0x65, 0x72, 0x61, 0x62, 0x61, 0x64, 0x5F, 0x4C, 0x42, 0x32, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x53, 0x4E, 0x49, 0x50, 0x3B, 0x58, 0x58, 0x58, 0x3B, 0x30, 0x2E, 0x30, 0x30, 0x3B, 0x30, 0x2E, 0x30, 0x30, 0x3B, 0x30, 0x3B, 0x30, 0x3B, 0x73, 0x4E, 0x54, 0x52, 0x49, 0x50, 0x3B, 0x6E, 0x6F, 0x6E, 0x65, 0x3B, 0x4E, 0x3B, 0x4E, 0x3B, 0x30, 0x3B, 0x6E, 0x6F, 0x6E, 0x65, 0x3B, 0x00, 0x00, 0x00, 0xB9, 0x6E, 0x19, 0x2E };
   const char* nmea_gpalm_log = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n";

   pclMyFilter->IncludeMessageId(42, HEADERFORMAT::ASCII); // Filter for BESTPOS (ASCII)
   pclMyFilter->IncludeTimeStatus(TIME_STATUS::FINESTEERING); // Filter for FINESTEERING
   pclMyFilter->SetIncludeLowerTimeBound(2180, 313699);
   // We only want WEEK 2180 and GPS Second 313700
   pclMyFilter->SetIncludeUpperTimeBound(2180, 313701);
   pclMyFilter->IncludeNMEAMessages(true);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_1))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_3))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_4))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(versiona_log_1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(versiona_log_2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(trackstata_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposb_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(sourcetableb_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(nmea_gpalm_log))));
}

TEST_F(FilterTest, MIX_1_INVERTED)
{
   const char* bestposa_log_1 = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313698.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* bestposa_log_2 = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2180,313700.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* bestposa_log_3 = "#BESTPOSA,COM1,0,8.0,FINESTEERING,2179,313702.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* bestposa_log_4 = "#BESTPOSA,COM1,0,8.0,COARSESTEERING,2180,313700.000,024000a0,cdba,32768;SOL_COMPUTED,SINGLE,51.15045046450,-114.03068725072,1097.2706,-17.0000,WGS84,1.3811,1.1629,3.1178,\"\",0.000,0.000,24,22,22,0,00,02,11,11*c64c3d4a\r\n";
   const char* versiona_log_1 = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   const char* versiona_log_2 = "#VERSIONA,COM1,0,55.5,FINESTEERING,2167,254938.857,02000000,3681,16248;8,GPSCARD,\"FFNBYNTMNP1\",\"BMHR15470120X\",\"OEM719N-0.00C\",\"OM7CR0707RN0000\",\"OM7BR0000RBG000\",\"2020/Apr/09\",\"13:40:45\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\",DEFAULT_CONFIG,\"\",\"\",\"\",\"EZDCD0707RN0001\",\"\",\"2020/Apr/09\",\"13:41:07\",APPLICATION,\"\",\"\",\"\",\"EZAPR0707RN0000\",\"\",\"2020/Apr/09\",\"13:41:00\",PACKAGE,\"\",\"\",\"\",\"EZPKR0103RN0000\",\"\",\"2020/Apr/09\",\"13:41:14\",ENCLOSURE,\"\",\"NMJC14520001W\",\"0.0.0.H\",\"\",\"\",\"\",\"\",IMUCARD,\"Epson G320N 125\",\"E0000114\",\"G320PDGN\",\"2302\",\"\",\"\",\"\",RADIO,\"M3-R4\",\"1843000570\",\"SPL0020d12\",\"V07.34.2.5.1.11\",\"\",\"\",\"\"*4b995016\r\n";
   const char* trackstata_log = "#TRACKSTATA,COM1,0,58.0,COARSESTEERING,2180,313700.000,02000000,457c,16248;SOL_COMPUTED,WAAS,5.0,235,2,0,0810bc04,20999784.925,770.496,49.041,8473.355,0.228,GOOD,0.975,2,0,01303c0b,20999781.972,600.387,49.021,8466.896,0.000,OBSL2,0.000,0,0,02208000,0.000,-0.004,0.000,0.000,0.000,NA,0.000,0,0,01c02000,0.000,0.000,0.000,0.000,0.000,NA,0.000,20,0,0810bc24,24120644.940,3512.403,42.138,1624.974,0.464,GOOD,0.588,20,0,01303c2b,24120645.042,2736.937,39.553,1619.755,0.000,OBSL2,0.000,0,0,02208020,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02020,0.000,0.000,0.000,0.000,0.000,NA,0.000,6,0,0810bc44,20727107.371,-1161.109,50.325,11454.975,-0.695,GOOD,0.979,6,0,01303c4b,20727108.785,-904.761,50.213,11448.915,0.000,OBSL2,0.000,6,0,02309c4b,20727109.344,-904.761,52.568,11451.815,0.000,OBSL2,0.000,6,0,01d03c44,20727110.520,-867.070,55.259,11453.455,0.000,OBSL5,0.000,29,0,0810bc64,25296813.545,3338.614,43.675,114.534,-0.170,GOOD,0.206,29,0,01303c6b,25296814.118,2601.518,39.636,109.254,0.000,OBSL2,0.000,29,0,02309c6b,25296814.580,2601.517,40.637,111.114,0.000,OBSL2,0.000,0,0,01c02060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02080,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,02208080,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c02080,0.000,0.000,0.000,0.000,0.000,NA,0.000,19,0,0810bca4,22493227.199,-3020.625,44.911,18244.973,0.411,GOOD,0.970,19,0,01303cab,22493225.215,-2353.736,44.957,18239.754,0.000,OBSL2,0.000,0,0,022080a0,0.000,-0.006,0.000,0.000,0.000,NA,0.000,0,0,01c020a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,24,0,0810bcc4,23856706.090,-3347.685,43.417,15187.116,-0.358,GOOD,0.957,24,0,01303ccb,23856708.306,-2608.588,43.207,15181.256,0.000,OBSL2,0.000,24,0,02309ccb,23856708.614,-2608.588,46.741,15183.815,0.000,OBSL2,0.000,24,0,01d03cc4,23856711.245,-2499.840,50.038,15185.256,0.000,OBSL5,0.000,25,0,1810bce4,21953295.423,2746.317,46.205,4664.936,0.322,GOOD,0.622,25,0,11303ceb,21953296.482,2139.988,45.623,4658.756,0.000,OBSL2,0.000,25,0,02309ceb,21953296.899,2139.988,47.584,4661.796,0.000,OBSL2,0.000,25,0,01d03ce4,21953298.590,2050.845,51.711,4662.976,0.000,OBSL5,0.000,0,0,0000a100,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02100,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,02208100,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02100,0.000,0.000,0.000,0.000,0.000,NA,0.000,17,0,1810bd24,24833573.179,-3002.286,43.809,21504.975,-0.219,GOOD,0.903,17,0,11303d2b,24833573.345,-2339.444,42.894,21499.256,0.000,OBSL2,0.000,17,0,02309d2b,24833573.677,-2339.444,44.238,21501.717,0.000,OBSL2,0.000,0,0,01c02120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a140,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02140,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,02208140,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,01c02140,0.000,0.000,0.000,0.000,0.000,NA,0.000,12,0,0810bd64,20275478.792,742.751,50.336,9634.855,0.166,GOOD,0.977,12,0,01303d6b,20275477.189,578.767,50.042,9629.756,0.000,OBSL2,0.000,12,0,02309d6b,20275477.555,578.767,51.012,9631.516,0.000,OBSL2,0.000,0,0,01c02160,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a02180,0.000,0.002,0.000,0.000,0.000,NA,0.000,0,0,02208180,0.000,0.003,0.000,0.000,0.000,NA,0.000,0,0,01c02180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021a0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,022081a0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021c0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,022081c0,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,01c021c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0000a1e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a021e0,0.000,0.001,0.000,0.000,0.000,NA,0.000,0,0,022081e0,0.000,0.003,0.000,0.000,0.000,NA,0.000,0,0,01c021e0,0.000,0.000,0.000,0.000,0.000,NA,0.000,194,0,0815be04,43478223.927,63.042,38.698,2382.214,0.000,NODIFFCORR,0.000,194,0,02359e0b,43478226.941,49.122,44.508,2378.714,0.000,OBSL2,0.000,194,0,01d53e04,43478228.121,47.080,43.958,2380.253,0.000,OBSL5,0.000,0,0,0005a220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258240,0.000,-0.002,0.000,0.000,0.000,NA,0.000,0,0,01c52240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,0005a260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02258260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01c52260,0.000,0.000,0.000,0.000,0.000,NA,0.000,131,0,48023e84,38480992.384,-0.167,45.356,471155.406,0.000,LOCKEDOUT,0.000,135,0,58023ea4,38553658.881,3.771,44.648,4.449,0.000,NODIFFCORR,0.000,133,0,58023ec4,38624746.161,1.065,45.618,471153.219,0.000,LOCKEDOUT,0.000,138,0,48023ee4,38493033.873,0.953,45.833,898498.250,0.000,LOCKEDOUT,0.000,55,4,18119f04,21580157.377,3208.835,44.921,3584.798,0.000,NODIFFCORR,0.000,55,4,00b13f0b,21580163.823,2495.762,45.078,3580.119,0.000,OBSL2,0.000,55,4,10319f0b,21580163.635,2495.762,45.682,3581.038,0.000,OBSL2,0.000,45,13,08119f24,23088997.031,-313.758,44.105,4273.538,0.000,NODIFFCORR,0.000,45,13,00b13f2b,23088998.989,-244.036,42.927,4267.818,0.000,OBSL2,0.000,45,13,00319f2b,23088999.269,-244.036,43.297,4268.818,0.000,OBSL2,0.000,54,11,18119f44,19120160.469,178.235,50.805,9344.977,0.000,NODIFFCORR,0.000,54,11,00b13f4b,19120162.255,138.627,46.584,9339.897,0.000,OBSL2,0.000,54,11,00319f4b,19120162.559,138.627,47.049,9340.818,0.000,OBSL2,0.000,0,0,00018360,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12360,0.000,0.004,0.000,0.000,0.000,NA,0.000,0,0,00218360,0.000,0.004,0.000,0.000,0.000,NA,0.000,0,0,00018380,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12380,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218380,0.000,0.000,0.000,0.000,0.000,NA,0.000,53,6,18119fa4,21330036.443,3045.661,43.167,3862.756,0.000,NODIFFCORR,0.000,53,6,00b13fab,21330040.203,2368.849,41.759,3858.039,0.000,OBSL2,0.000,53,6,00319fab,21330039.119,2368.850,42.691,3859.038,0.000,OBSL2,0.000,38,8,18119fc4,22996582.245,2427.724,41.817,2014.338,0.000,NODIFFCORR,0.000,38,8,10b13fcb,22996590.440,1888.231,35.968,2010.119,0.000,OBSL2,0.000,38,8,10319fcb,22996589.454,1888.230,36.755,2011.038,0.000,OBSL2,0.000,52,7,08119fe4,19520740.266,-1275.394,50.736,10712.179,0.000,NODIFFCORR,0.000,52,7,00b13feb,19520744.583,-991.974,47.931,10708.038,0.000,OBSL2,0.000,52,7,10319feb,19520744.527,-991.974,48.251,10709.038,0.000,OBSL2,0.000,51,0,18119c04,22302364.417,-4314.112,43.692,16603.602,0.000,NODIFFCORR,0.000,51,0,00b13c0b,22302371.827,-3355.424,45.975,16603.580,0.000,OBSL2,0.000,51,0,00319c0b,22302371.325,-3355.424,46.904,16603.502,0.000,OBSL2,0.000,61,9,08119c24,21163674.206,-3198.898,47.898,14680.979,0.000,NODIFFCORR,0.000,61,9,10b13c2b,21163677.196,-2488.033,44.960,14675.897,0.000,OBSL2,0.000,61,9,00319c2b,21163677.300,-2488.033,45.628,14676.737,0.000,OBSL2,0.000,0,0,00018040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218040,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00018060,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12060,0.000,-0.000,0.000,0.000,0.000,NA,0.000,0,0,00218060,0.000,-0.001,0.000,0.000,0.000,NA,0.000,0,0,00018080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a12080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00218080,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,000180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00a120a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,002180a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004380c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028320c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,21,0,08539ce4,25416828.004,2077.626,46.584,6337.363,0.000,NODIFFCORR,0.000,21,0,01933ce4,25416833.286,1551.460,49.589,6335.164,0.000,OBSE5,0.000,21,0,02333ce4,25416829.717,1591.910,50.226,6335.176,0.000,OBSE5,0.000,21,0,02933ce4,25416829.814,1571.722,52.198,6334.944,0.000,OBSE5,0.000,27,0,08539d04,23510780.996,-707.419,51.721,16182.524,0.000,NODIFFCORR,0.000,27,0,01933d04,23510785.247,-528.262,53.239,16180.444,0.000,OBSE5,0.000,27,0,02333d04,23510781.458,-542.015,53.731,16180.243,0.000,OBSE5,0.000,27,0,02933d04,23510781.960,-535.149,55.822,16180.165,0.000,OBSE5,0.000,0,0,00438120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232120,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832120,0.000,0.000,0.000,0.000,0.000,NA,0.000,15,0,08539d44,23034423.020,183.445,51.283,11971.245,0.000,NODIFFCORR,0.000,15,0,01933d44,23034428.761,136.945,53.293,11969.243,0.000,OBSE5,0.000,15,0,02333d44,23034425.379,140.546,53.897,11969.245,0.000,OBSE5,0.000,15,0,02933d44,23034425.436,138.742,55.909,11968.946,0.000,OBSE5,0.000,13,0,08539d64,25488681.795,2565.988,46.632,4828.445,0.000,NODIFFCORR,0.000,13,0,01933d64,25488687.213,1916.182,47.753,4826.243,0.000,OBSE5,0.000,13,0,02333d64,25488683.967,1966.148,50.045,4826.243,0.000,OBSE5,0.000,13,0,02933d64,25488684.398,1941.169,51.348,4826.165,0.000,OBSE5,0.000,0,0,00438180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832180,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004381c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028321c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,30,0,08539de4,25532715.149,-2938.485,46.289,26421.467,0.000,NODIFFCORR,0.000,30,0,01933de4,25532721.371,-2194.317,49.285,26419.447,0.000,OBSE5,0.000,30,0,02333de4,25532718.174,-2251.520,50.681,26419.447,0.000,OBSE5,0.000,30,0,02933de4,25532717.843,-2222.952,52.291,26419.166,0.000,OBSE5,0.000,0,0,00438200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832200,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832220,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832240,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832260,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,00438280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,01832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02232280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,02832280,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,004382a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,018322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,022322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,0,0,028322a0,0.000,0.000,0.000,0.000,0.000,NA,0.000,41,0,48149ec4,26228546.068,2731.326,43.047,1244.968,0.000,NODIFFCORR,0.000,41,0,41343ec4,26228560.733,2058.212,46.309,1239.648,0.000,NA,0.000,27,0,08149ee4,21470141.903,-686.571,51.408,13695.229,0.000,NODIFFCORR,0.000,27,0,41343ee4,21470143.417,-517.430,52.724,13690.050,0.000,NA,0.000,6,0,08149f04,40334269.953,-663.889,38.200,12755.121,0.000,NODIFFCORR,0.000,6,0,00349f04,40334265.525,-513.549,39.333,12754.961,0.000,OBSB2,0.000,16,0,08149f24,40591561.211,-689.953,40.783,11755.120,0.000,NODIFFCORR,0.000,16,0,00349f24,40591562.100,-533.388,39.928,11754.960,0.000,OBSB2,0.000,39,0,58149f44,40402963.125,-730.398,41.019,11015.042,0.000,NODIFFCORR,0.000,39,0,41343f44,40402964.083,-550.456,43.408,11009.821,0.000,NA,0.000,30,0,18149f64,22847646.673,2123.913,50.266,6625.051,0.000,NODIFFCORR,0.000,30,0,41343f64,22847649.151,1600.605,49.656,6619.991,0.000,NA,0.000,7,0,08048381,0.000,2500.000,0.000,0.000,0.000,NA,0.000,7,0,08048381,0.000,-2500.000,0.000,0.000,0.000,NA,0.000,33,0,48149fa4,25666349.147,776.929,42.271,3835.148,0.000,NODIFFCORR,0.000,33,0,41343fa4,25666377.385,585.535,48.361,3697.589,0.000,NA,0.000,46,0,48149fc4,23048323.129,-2333.170,49.345,15915.131,0.000,NODIFFCORR,0.000,46,0,41343fc4,23048329.413,-1758.350,52.408,15909.830,0.000,NA,0.000,18,0,080483e1,0.000,4000.000,0.000,0.000,0.000,NA,0.000,18,0,080483e1,0.000,-500.000,0.000,0.000,0.000,NA,0.000,45,0,48149c04,26221109.945,2965.644,44.864,435.050,0.000,NODIFFCORR,0.000,45,0,41343c04,26221119.956,2234.910,47.292,429.831,0.000,NA,0.000,36,0,58149c24,23277715.056,700.443,48.907,8015.069,0.000,NODIFFCORR,0.000,36,0,41343c24,23277723.101,527.848,51.167,8009.829,0.000,NA,0.000,52,0,08048041,0.000,1667.000,0.000,0.000,0.000,NA,0.000,52,0,08048041,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,49,0,08048061,0.000,5832.000,0.000,0.000,0.000,NA,0.000,49,0,08048061,0.000,-4999.000,0.000,0.000,0.000,NA,0.000,47,0,08048081,0.000,1000.000,0.000,0.000,0.000,NA,0.000,47,0,08048081,0.000,-500.000,0.000,0.000,0.000,NA,0.000,58,0,48049ca4,34894393.899,-3079.127,30.345,47.772,0.000,NODIFFCORR,0.000,58,0,012420a9,0.000,-2321.139,0.000,0.000,0.000,NA,0.000,14,0,08149cc4,25730238.361,-588.324,38.191,4795.070,0.000,NODIFFCORR,0.000,14,0,00349cc4,25730237.379,-454.787,44.427,4794.910,0.000,OBSB2,0.000,28,0,08149ce4,24802536.288,-2833.581,46.004,19865.129,0.000,NODIFFCORR,0.000,28,0,41343ce4,24802537.579,-2135.389,46.897,19859.650,0.000,NA,0.000,48,0,08048101,0.000,16000.000,0.000,0.000,0.000,NA,0.000,0,0,00248100,0.000,0.000,0.000,0.000,0.000,NA,0.000,9,0,08149d24,40753569.155,222.237,37.682,1784.493,0.000,NODIFFCORR,0.000,9,0,00349d24,40753568.209,171.813,41.501,4664.961,0.000,OBSB2,0.000,3,0,08848141,0.000,6000.000,0.000,0.000,0.000,NA,0.000,3,0,08848141,0.000,-11000.000,0.000,0.000,0.000,NA,0.000,1,0,08848161,0.000,4999.000,0.000,0.000,0.000,NA,0.000,1,0,08848161,0.000,-4166.000,0.000,0.000,0.000,NA,0.000,6,0,0a670984,0.000,-301.833,36.924,1734607.250,0.000,NA,0.000,1,0,0a6709a4,0.000,83.304,43.782,558002.188,0.000,NA,0.000,0,0,026701c0,0.000,0.000,0.000,0.000,0.000,NA,0.000,3,0,0a6701e1,0.000,419.842,0.000,0.000,0.000,NA,0.000,0,0,02670200,0.000,0.000,0.000,0.000,0.000,NA,0.000*c8963f70\r\n";
   unsigned const char bestposb_log[] = { 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA4, 0xB4, 0xAC, 0x07, 0xD8, 0x16, 0x6D, 0x08, 0x08, 0x40, 0x00, 0x02, 0xF6, 0xB1, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xD7, 0x03, 0xB0, 0x4C, 0xE5, 0x8E, 0x49, 0x40, 0x52, 0xC4, 0x26, 0xD1, 0x72, 0x82, 0x5C, 0xC0, 0x29, 0xCB, 0x10, 0xC7, 0x7A, 0xA2, 0x90, 0x40, 0x33, 0x33, 0x87, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0xFA, 0x7E, 0xBA, 0x3F, 0x3F, 0x57, 0x83, 0x3F, 0xA9, 0xA4, 0x0A, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x16, 0x16, 0x16, 0x00, 0x06, 0x39, 0x33, 0x23, 0xC4, 0x89, 0x7A };
   unsigned const char sourcetableb_log[] = { 0xAA, 0x44, 0x12, 0x1C, 0x40, 0x05, 0x00, 0x20, 0x68, 0x00, 0x15, 0x00, 0x80, 0xB4, 0x74, 0x08, 0x00, 0x5B, 0x88, 0x0D, 0x20, 0x80, 0x00, 0x02, 0xDD, 0x71, 0x00, 0x80, 0x68, 0x65, 0x72, 0x61, 0x2E, 0x6E, 0x6F, 0x76, 0x61, 0x74, 0x65, 0x6C, 0x2E, 0x63, 0x6F, 0x6D, 0x3A, 0x32, 0x31, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x54, 0x52, 0x3B, 0x48, 0x79, 0x64, 0x65, 0x72, 0x61, 0x62, 0x61, 0x64, 0x5F, 0x4C, 0x42, 0x32, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x53, 0x4E, 0x49, 0x50, 0x3B, 0x58, 0x58, 0x58, 0x3B, 0x30, 0x2E, 0x30, 0x30, 0x3B, 0x30, 0x2E, 0x30, 0x30, 0x3B, 0x30, 0x3B, 0x30, 0x3B, 0x73, 0x4E, 0x54, 0x52, 0x49, 0x50, 0x3B, 0x6E, 0x6F, 0x6E, 0x65, 0x3B, 0x4E, 0x3B, 0x4E, 0x3B, 0x30, 0x3B, 0x6E, 0x6F, 0x6E, 0x65, 0x3B, 0x00, 0x00, 0x00, 0xB9, 0x6E, 0x19, 0x2E };
   const char* nmea_gpalm_log = "$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n";

   pclMyFilter->IncludeMessageId(42, HEADERFORMAT::ALL);   // Filter for BESTPOS (ASCII)
   pclMyFilter->IncludeMessageId(37, HEADERFORMAT::ASCII); // Filter for VERSION (ASCII)
   pclMyFilter->IncludeMessageId(83, HEADERFORMAT::ASCII); // Filter for TRACKSTAT (ASCII)
   pclMyFilter->InvertMessageIdFilter(true);
   pclMyFilter->IncludeNMEAMessages(false);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_3))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposa_log_4))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(versiona_log_1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(versiona_log_2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(trackstata_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bestposb_log))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(sourcetableb_log))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(nmea_gpalm_log))));
}

TEST_F(FilterTest, MIX_2)
{
   const char* log_to_filter1 = "#TIMEA,COM1,0,6.0,UNKNOWN,0,25.000,024c0020,9924,32768;INVALID,0.000000000,0.000333564,0.00000000000,0,0,0,0,0,0,INVALID*6fe3e59a\r\n";
   const char* log_to_filter2 = "#RANGEA,COM1,0,5.5,UNKNOWN,0,25.000,024c00a0,5103,32768;0*943a8919\r\n";
   const char* log_to_filter3 = "#BESTPOSA,COM1,0,6.0,UNKNOWN,0,25.000,024c0020,cdba,32768;INSUFFICIENT_OBS,NONE,0.00000000000,0.00000000000,0.0000,0.0000,WGS84,0.0000,0.0000,0.0000,\"\",0.000,0.000,0,0,0,0,00,00,00,00*fb4efeaa\r\n";
   const char* log_to_filter4 = "#TIMEA,COM1,0,6.5,UNKNOWN,0,26.000,024c0020,9924,32768;INVALID,0.000000000,0.000333564,0.00000000000,0,0,0,0,0,0,INVALID*ca3c9c2c\r\n";
   const char* log_to_filter5 = "#BESTPOSA,COM1,0,6.5,UNKNOWN,0,26.000,024c0020,cdba,32768;INSUFFICIENT_OBS,NONE,0.00000000000,0.00000000000,0.0000,0.0000,WGS84,0.0000,0.0000,0.0000,\"\",0.000,0.000,0,0,0,0,00,00,00,00*f5a77b4d\r\n";
   const char* log_to_filter6 = "#VERSIONA,COM1,0,5.5,COARSESTEERING,2180,407587.500,024c00a0,3681,32768;2,GPSCARD,\"FFNRNNCBN\",\"BMGX15290006Y\",\"OEM729-0.00H\",\"OM7MGMNJCDND6EB\",\"OM7BR0000ABG001\",\"2021/Sep/13\",\"15:28:07\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\"*55a19852\r\n";
   const char* log_to_filter7 = "#RANGEA,COM1,0,6.5,COARSESTEERING,2180,407587.500,024c0020,5103,32768;3,8,0,22086479.072,0.079,-116065230.826912,0.008,827.920,48.3,20.465,0800bca4,32,0,22250341.055,0.070,-116926330.596180,0.007,3298.934,49.5,19.924,0800bce4,15,0,23310073.938,0.111,-122495264.853699,0.012,-3571.021,45.5,19.861,0800bda4*d32c11da\r\n";
   const char* log_to_filter8 = "#BESTPOSA,COM1,0,7.5,COARSESTEERING,2180,407587.500,024c0020,cdba,32768;INSUFFICIENT_OBS,NONE,0.00000000000,0.00000000000,0.0000,0.0000,WGS84,0.0000,0.0000,0.0000,\"\",0.000,0.000,0,0,0,0,00,00,00,00*2075f7c6\r\n";
   const char* log_to_filter9 = "#TIMEA,COM1,0,7.5,COARSESTEERING,2180,407588.000,024c0020,9924,32768;INVALID,0.000000000,0.000333564,-17.99999999838,2021,10,21,17,12,50000,VALID*6fec570c\r\n";
   const char* log_to_filter10 = "#BESTPOSA,COM1,0,8.0,COARSESTEERING,2180,407588.000,024c0020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043021293,-114.03066584294,1096.9485,0.0000,WGS84,34.6695,23.1483,74.2061,\"\",0.000,0.000,8,8,8,0,00,00,01,01*04e05542\r\n";
   const char* log_to_filter11 = "#TIMEA,COM1,0,8.0,COARSESTEERING,2180,407589.000,024c0020,9924,32768;ITERATING,-0.001480222,0.000000136,-17.99999999838,2021,10,21,17,12,51001,VALID*3271a4db\r\n";
   const char* log_to_filter12 = "#BESTPOSA,COM1,0,8.0,COARSESTEERING,2180,407589.000,024c0020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044969214,-114.03067885944,1097.8348,0.0000,WGS84,21.5220,15.8619,35.9534,\"\",0.000,0.000,18,18,18,0,00,00,11,11*9baaa764\r\n";
   const char* log_to_filter13 = "#VERSIONA,COM1,0,5.5,COARSESTEERING,2180,407590.000,024c0020,3681,32768;2,GPSCARD,\"FFNRNNCBN\",\"BMGX15290006Y\",\"OEM729-0.00H\",\"OM7MGMNJCDND6EB\",\"OM7BR0000ABG001\",\"2021/Sep/13\",\"15:28:07\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\"*0c5c14a1\r\n";
   const char* log_to_filter14 = "#TIMEA,COM1,0,5.5,COARSESTEERING,2180,407590.000,02400020,9924,32768;ITERATING,-0.001480245,5.445525581e-08,-17.99999999838,2021,10,21,17,12,52001,VALID*0804ea28\r\n";
   const char* log_to_filter15 = "#RANGEA,COM1,0,5.5,COARSESTEERING,2180,407590.000,024c0020,5103,32768;41,23,0,20296030.375,0.055,-106656359.351969,0.006,-1522.264,51.0,22.611,0810bc24,23,0,20296034.364,2.872,-79645997.562468,0.007,-1136.719,55.7,0.830,01d03c24,8,0,22086085.415,0.074,-116063162.380520,0.007,825.378,48.4,22.965,0810bca4,8,0,22086087.843,0.095,-86670551.251795,0.048,616.253,53.7,1.216,01d03ca4,32,0,22248771.717,0.066,-116918083.707594,0.007,3297.228,49.5,22.424,0810bce4,32,0,22248773.901,0.100,-87308967.147285,0.039,2462.156,51.8,1.214,01d03ce4,24,0,22264715.219,0.105,-117001867.872692,0.009,1428.997,45.6,22.284,0810bd04,24,0,22264719.059,0.105,-87371538.460072,0.072,1067.142,50.2,1.145,01d03d04,21,0,24669644.466,0.188,-129639857.490297,0.018,3093.623,40.2,23.416,0800bd24,15,0,23311772.856,0.104,-122504192.563972,0.010,-3572.507,45.6,22.361,0800bda4,39,3,23195458.666,0.174,-123775410.587280,0.008,4001.722,46.6,25.110,08119f64,39,3,23195462.989,0.847,-96269782.463670,0.008,3112.451,42.9,0.641,0031976b,46,5,22953717.926,0.189,-122571572.431876,0.008,1696.585,45.9,25.092,08119fa4,46,5,22953719.900,1.160,-95333453.473848,0.008,1319.566,39.8,0.563,003197ab,38,8,18999930.055,0.143,-101565513.986405,0.006,1405.812,48.4,25.005,18119c04,38,8,18999932.172,0.697,-78995409.137779,0.006,1093.409,45.4,0.555,0031940b,61,9,18934081.726,0.093,-101249042.682301,0.005,817.900,51.8,26.435,18119c24,61,9,18934084.342,0.367,-78749266.465596,0.005,636.145,49.3,1.033,0031942b,60,10,19648944.071,0.097,-105108600.620312,0.006,-2737.993,51.7,25.173,08019c44,54,11,21935268.469,0.220,-117380052.385700,0.008,4060.476,44.6,24.925,08119c64,54,11,21935271.786,0.533,-91295609.074792,0.008,3158.148,46.1,1.217,0031946b,44,12,23261389.160,0.307,-124520037.179708,0.010,-3982.786,41.8,24.051,08119c84,44,12,23261389.788,0.868,-96848920.409648,0.010,-3097.722,42.6,0.562,0031948b,45,13,19511152.838,0.114,-104481336.951767,0.005,-2167.651,50.0,26.455,08119ca4,45,13,19511153.255,0.554,-81263263.963896,0.005,-1685.951,48.8,0.633,003194ab,4,0,23262018.929,0.052,-122242734.095053,0.006,554.113,53.5,23.261,08539d24,4,0,23262022.476,0.094,-91285170.722265,0.048,413.790,54.7,1.240,01933d24,4,0,23262019.108,0.031,-93666511.360582,0.076,424.600,55.8,1.240,02333d24,4,0,23262020.382,1.260,-92475840.210258,0.004,419.179,57.5,0.941,02933d24,9,0,23898656.058,0.091,-125588285.025036,0.007,2841.674,50.1,16.799,08439dc4,11,0,21319648.513,0.110,-112035509.001801,0.015,-461.787,50.2,11.107,08539e04,11,0,21319649.335,3.312,-83662883.158997,0.012,-344.930,50.9,0.827,01933e04,11,0,21319645.998,2.960,-85845379.482821,0.129,-353.888,51.5,0.760,02333e04,11,0,21319647.463,3.898,-84754130.488752,0.009,-349.423,53.6,0.767,02933e04,35,0,22877372.394,0.054,-119128481.299582,0.005,1633.597,51.0,23.542,08049ee4,32,0,23880326.384,0.079,-124351124.940607,0.007,-2624.954,47.9,22.659,18049f44,23,0,25929790.141,0.218,-135023220.319811,0.010,1613.334,44.5,12.772,18049c84,19,0,22897003.727,0.057,-119230706.390177,0.006,2562.076,50.2,24.902,18049d04,20,0,21333636.698,0.042,-111089843.666973,0.005,2.533,53.2,23.627,18049d24,25,0,26367558.065,0.173,-137302792.936779,0.011,-661.552,42.4,17.590,18049e24,29,0,21811517.639,0.046,-113578295.384806,0.006,-1531.856,52.3,23.606,08049e44*f528c67b\r\n";
   const char* log_to_filter16 = "#BESTPOSA,COM1,0,5.5,COARSESTEERING,2180,407590.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044625442,-114.03068674478,1098.5042,-17.0000,WGS84,1.9497,1.2176,3.6853,\"\",0.000,0.000,24,18,18,0,00,02,11,11*2660ffae\r\n";
   const char* log_to_filter17 = "#TIMEA,COM1,0,5.5,COARSESTEERING,2180,407591.000,02400020,9924,32768;ITERATING,-0.001480327,5.208111551e-09,-17.99999999838,2021,10,21,17,12,53001,VALID*c46d1791\r\n";
   const char* log_to_filter18 = "#BESTPOSA,COM1,0,5.5,COARSESTEERING,2180,407591.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044507847,-114.03068659595,1098.3277,-17.0000,WGS84,1.5509,1.0997,2.5066,\"\",0.000,0.000,24,24,24,0,00,02,11,11*309fb715\r\n";
   const char* log_to_filter19 = "#TIMEA,COM1,0,7.0,COARSESTEERING,2180,407592.000,02400020,9924,32768;ITERATING,-0.001480349,3.184623638e-09,-17.99999999838,2021,10,21,17,12,54001,VALID*b64126e3\r\n";
   const char* log_to_filter20 = "#BESTPOSA,COM1,0,7.0,COARSESTEERING,2180,407592.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044507358,-114.03068646694,1098.2902,-17.0000,WGS84,1.5509,1.0992,2.5061,\"\",0.000,0.000,24,24,24,0,00,02,11,11*91248d43\r\n";
   const char* log_to_filter21 = "#TIMEA,COM1,0,6.5,COARSESTEERING,2180,407593.000,02400020,9924,32768;ITERATING,-0.001480363,2.501886198e-09,-17.99999999838,2021,10,21,17,12,55001,VALID*b5cf0420\r\n";
   const char* log_to_filter22 = "#BESTPOSA,COM1,0,6.5,COARSESTEERING,2180,407593.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044498855,-114.03068624941,1098.2669,-17.0000,WGS84,1.5510,1.0991,2.5060,\"\",0.000,0.000,24,24,24,0,00,02,11,11*d3b81bb5\r\n";
   const char* log_to_filter23 = "#TIMEA,COM1,0,6.5,COARSESTEERING,2180,407594.000,02400020,9924,32768;ITERATING,-0.001480371,2.131839123e-09,-17.99999999838,2021,10,21,17,12,56001,VALID*741b4855\r\n";
   const char* log_to_filter24 = "#BESTPOSA,COM1,0,6.5,COARSESTEERING,2180,407594.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044476808,-114.03068625223,1098.2658,-17.0000,WGS84,1.5511,1.0989,2.5060,\"\",0.000,0.000,27,24,24,0,00,02,11,11*c3b290f9\r\n";
   const char* log_to_filter25 = "#TIMEA,COM1,0,7.5,COARSESTEERING,2180,407595.000,02400020,9924,32768;ITERATING,-0.001480373,1.892068473e-09,-17.99999999838,2021,10,21,17,12,57001,VALID*dd7d6731\r\n";
   const char* log_to_filter26 = "#RANGEA,COM1,0,5.5,COARSESTEERING,2180,407595.000,02400020,5103,32768;77,23,0,20297500.868,0.052,-106664086.632315,0.006,-1562.684,50.7,27.611,0810bc24,23,0,20297500.632,0.265,-83114871.442232,0.005,-1217.676,50.4,1.250,01305c2b,23,0,20297501.045,0.140,-83114872.694162,0.007,-1217.677,53.0,4.512,02309c2b,23,0,20297505.319,0.067,-79651767.930981,0.004,-1166.878,55.9,5.830,01d03c24,8,0,22085323.035,0.069,-116059155.718779,0.008,783.431,48.2,27.965,0810bca4,8,0,22085325.492,0.358,-90435713.372734,0.009,610.466,47.5,2.745,01305cab,8,0,22085325.995,0.198,-90435715.632605,0.008,610.465,49.8,4.785,02309cab,8,0,22085325.442,0.066,-86667559.258654,0.006,585.051,53.8,6.216,01d03ca4,32,0,22245656.498,0.061,-116901713.393351,0.007,3257.008,49.3,27.424,0810bce4,32,0,22245659.125,0.360,-91092255.293990,0.011,2537.928,47.4,2.405,01305ceb,32,0,22245659.231,0.201,-91092248.543947,0.007,2537.927,49.7,4.783,02309ceb,32,0,22245658.803,0.068,-87296742.558874,0.006,2432.279,52.0,6.214,01d03ce4,24,0,22263378.104,0.098,-116994841.931118,0.009,1387.328,45.2,27.284,0810bd04,24,0,22263381.064,0.464,-91164823.200386,0.014,1081.034,45.3,2.405,01305d0b,24,0,22263381.254,0.244,-91164817.453840,0.010,1081.036,48.0,4.784,02309d0b,24,0,22263382.083,0.071,-87366291.822250,0.008,1036.050,50.4,6.145,01d03d04,21,0,24666722.444,0.168,-129624501.758570,0.015,3054.951,40.4,28.416,0810bd24,21,0,24666722.744,0.790,-101006106.505545,0.019,2380.481,40.6,2.497,01305d2b,18,0,21981503.954,0.067,-115513581.695168,0.007,-3112.239,48.3,28.105,0800bd44,15,0,23315193.939,0.100,-122522170.201077,0.012,-3612.525,45.0,27.361,0810bda4,15,0,23315194.577,0.472,-95471823.159175,0.011,-2814.954,45.1,2.741,01305dab,15,0,23315194.509,0.340,-95471821.421129,0.012,-2814.955,45.1,4.682,02309dab,10,0,20058554.660,0.034,-105408414.831429,0.006,676.520,54.2,28.112,1800bdc4,27,0,21420640.151,0.053,-112566222.962255,0.007,-1305.266,50.4,28.067,0810bde4,27,0,21420642.131,5.656,-84059200.042173,0.044,-974.588,53.6,0.647,01d03de4,39,3,23191730.713,0.167,-123755517.439732,0.008,3961.597,46.2,30.110,08119f64,39,3,23191737.435,0.130,-96254312.753414,0.009,3081.246,42.7,4.721,10b13f6b,39,3,23191737.125,0.527,-96254310.011505,0.008,3081.244,43.5,5.641,00319f6b,46,5,22952152.164,0.171,-122563211.547077,0.008,1653.720,46.0,30.092,08119fa4,46,5,22952155.579,0.197,-95326955.817892,0.009,1286.228,39.0,4.722,00b13fab,46,5,22952155.755,0.808,-95326950.071980,0.009,1286.228,39.8,5.563,10319fab,38,8,18998637.906,0.130,-101558607.040641,0.006,1363.216,48.4,30.005,18119c04,38,8,18998643.997,0.103,-78990046.826712,0.006,1060.280,44.6,4.656,10b13c0b,38,8,18998642.764,0.407,-78990037.073071,0.006,1060.279,45.8,5.555,00319c0b,61,9,18933340.219,0.085,-101245077.461238,0.005,774.337,51.9,31.435,18119c24,61,9,18933343.261,0.061,-78746182.652719,0.005,602.263,48.8,5.036,00b13c2b,61,9,18933343.113,0.259,-78746181.906229,0.005,602.261,49.3,6.033,10319c2b,60,10,19651526.114,0.088,-105122412.778005,0.005,-2780.830,51.8,30.173,08019c44,54,11,21931496.436,0.190,-117359867.373328,0.008,4019.855,45.1,29.925,08119c64,54,11,21931499.657,0.092,-91279908.365263,0.008,3126.554,45.3,5.066,10b13c6b,54,11,21931500.087,0.378,-91279909.619295,0.008,3126.552,45.9,6.217,00319c6b,44,12,23265130.397,0.268,-124540065.478237,0.010,-4022.401,42.2,29.051,08119c84,44,12,23265132.939,0.134,-96864504.724735,0.011,-3128.535,42.4,4.642,00b13c8b,44,12,23265134.440,0.590,-96864497.977821,0.011,-3128.535,42.5,5.562,00319c8b,45,13,19513199.233,0.104,-104492295.524177,0.005,-2209.578,50.1,31.455,08119ca4,45,13,19513200.373,0.065,-81271790.545637,0.006,-1718.560,48.7,4.714,00b13cab,45,13,19513200.665,0.281,-81271786.796770,0.006,-1718.560,48.9,5.633,10319cab,4,0,23261513.970,0.049,-122240080.567102,0.006,513.442,53.2,28.261,08539d24,4,0,23261517.679,0.065,-91283189.196609,0.006,383.352,54.8,6.240,01933d24,4,0,23261514.144,0.017,-93664478.142199,0.010,393.414,55.8,6.240,02333d24,4,0,23261515.437,0.065,-92473832.837046,0.004,388.382,57.7,5.941,02933d24,9,0,23895974.018,0.082,-125574190.676455,0.006,2802.065,49.9,21.799,08539dc4,9,0,23895979.211,0.083,-93772951.560656,0.014,2092.436,48.8,4.634,01933dc4,9,0,23895976.481,0.032,-96219193.923071,0.019,2147.062,51.9,4.635,02333dc4,9,0,23895977.631,0.077,-94996067.408674,0.005,2119.773,52.7,4.359,02933dc4,11,0,21320110.232,0.094,-112037935.358484,0.016,-502.672,50.0,16.107,08539e04,11,0,21320111.176,0.072,-83664695.046743,0.011,-375.390,51.1,5.827,01933e04,11,0,21320107.412,0.029,-85847238.635632,0.020,-385.203,51.6,5.760,02333e04,11,0,21320109.153,0.068,-84755966.011379,0.011,-380.314,53.8,5.767,02933e04,30,0,24900309.826,0.291,-129662447.050890,0.008,-3516.229,45.6,9.155,18049ec4,35,0,22875826.226,0.048,-119120430.228863,0.005,1592.785,51.2,28.542,08149ee4,35,0,22875828.809,5.656,-89769665.177532,0.080,1200.265,50.0,0.641,01343ee4,35,0,22875825.944,0.090,-92111471.486866,0.020,1231.668,48.2,4.042,01743ee4,32,0,23882868.520,0.071,-124364362.318052,0.007,-2663.985,47.9,27.659,18149f44,32,0,23882875.101,5.656,-93721531.198517,0.062,-2007.573,51.6,0.639,01343f44,32,0,23882871.851,0.081,-96166426.626690,0.009,-2059.995,49.1,4.819,01743f44,23,0,25928263.061,0.131,-135015269.286980,0.008,1572.973,44.8,17.772,18149c84,23,0,25928258.755,0.105,-104402354.741234,0.016,1216.402,44.3,4.448,01743c84,19,0,22894565.383,0.052,-119218009.616373,0.006,2522.539,50.3,29.902,18149d04,19,0,22894567.405,5.656,-89843199.600471,0.127,1901.049,51.0,0.642,01343d04,19,0,22894564.496,0.084,-92186924.132240,0.013,1950.624,48.8,4.638,11743d04,20,0,21333656.505,0.038,-111089946.855726,0.005,-37.780,53.3,28.627,18149d24,20,0,21333652.378,0.076,-85901777.681990,0.014,-29.197,53.2,4.441,11743d24,25,0,26368215.794,0.150,-137306218.129744,0.011,-702.371,42.3,22.590,18149e24,25,0,26368215.690,0.109,-106173876.909266,0.013,-543.206,42.7,5.247,11743e24,29,0,21813011.021,0.042,-113586071.992727,0.005,-1572.769,52.4,28.606,08149e44,29,0,21813008.843,0.080,-87831948.349440,0.015,-1216.090,50.9,4.438,01743e44*0475a421\r\n";
   const char* log_to_filter27 = "#BESTPOSA,COM1,0,7.5,COARSESTEERING,2180,407595.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044243058,-114.03068360219,1097.7004,-17.0000,WGS84,1.3792,0.9846,2.2407,\"\",0.000,0.000,28,27,27,0,00,02,11,11*9a57a6f4\r\n";
   const char* log_to_filter28 = "#TIMEA,COM1,0,7.5,FINESTEERING,2180,407596.000,02400020,9924,32768;ITERATING,-0.001480373,1.694616069e-09,-17.99999999838,2021,10,21,17,12,58001,VALID*ca78e438\r\n";
   const char* log_to_filter29 = "#BESTPOSA,COM1,0,7.5,FINESTEERING,2180,407596.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044308398,-114.03068382954,1097.7873,-17.0000,WGS84,1.1918,0.9784,2.0965,\"\",0.000,0.000,29,28,28,0,00,02,11,11*81ec1d55\r\n";
   const char* log_to_filter30 = "#TIMEA,COM1,0,7.5,FINESTEERING,2180,407597.000,02400020,9924,32768;ITERATING,-1.056282464e-08,4.980118958e-09,-17.99999999838,2021,10,21,17,12,59000,VALID*86a8f91f\r\n";
   const char* log_to_filter31 = "#BESTPOSA,COM1,0,7.5,FINESTEERING,2180,407597.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044349643,-114.03068418925,1097.7073,-17.0000,WGS84,1.1581,0.9671,2.0529,\"\",0.000,0.000,30,29,29,0,00,02,11,11*4f210b38\r\n";
   const char* log_to_filter32 = "#TIMEA,COM1,0,7.5,FINESTEERING,2180,407598.000,02400020,9924,32768;ITERATING,-8.720389968e-09,2.851254543e-09,-17.99999999838,2021,10,21,17,13,0,VALID*6046ea6b\r\n";
   const char* log_to_filter33 = "#BESTPOSA,COM1,0,7.5,FINESTEERING,2180,407598.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044380692,-114.03068422806,1097.7573,-17.0000,WGS84,1.1254,0.9541,2.0500,\"\",0.000,0.000,30,30,30,0,00,02,11,11*6dea55c5\r\n";
   const char* log_to_filter34 = "#TIMEA,COM1,0,8.5,FINESTEERING,2180,407599.000,02400020,9924,32768;ITERATING,-5.706091940e-09,2.208897498e-09,-17.99999999838,2021,10,21,17,13,1000,VALID*19f9590d\r\n";
   const char* log_to_filter35 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,407599.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044360617,-114.03068408017,1097.7294,-17.0000,WGS84,1.1247,0.9540,2.0496,\"\",0.000,0.000,31,30,30,0,00,02,11,11*09f5c77f\r\n";
   const char* log_to_filter36 = "#VERSIONA,COM1,0,5.5,FINESTEERING,2180,407600.000,02400020,3681,32768;2,GPSCARD,\"FFNRNNCBN\",\"BMGX15290006Y\",\"OEM729-0.00H\",\"OM7MGMNJCDND6EB\",\"OM7BR0000ABG001\",\"2021/Sep/13\",\"15:28:07\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\"*6ebd192f\r\n";
   const char* log_to_filter37 = "#TIMEA,COM1,0,8.5,FINESTEERING,2180,407600.000,02400020,9924,32768;ITERATING,-2.698749387e-09,1.871232507e-09,-17.99999999838,2021,10,21,17,13,2000,VALID*500b310b\r\n";
   const char* filtered_log = "#RANGEA,COM1,0,7.5,FINESTEERING,2180,407600.000,02400020,5103,32768;97,23,0,20742792.951,0.046,-109004112.049799,0.006,-1569.674,51.1,32.610,0810bc24,23,0,20742792.750,0.186,-84938267.863909,0.006,-1223.123,50.0,6.249,01305c2b,23,0,20742792.847,0.096,-84938269.116295,0.006,-1223.124,53.0,9.510,02309c2b,23,0,20742797.345,0.042,-81399189.508091,0.004,-1172.077,55.9,10.829,01d03c24,8,0,22528384.243,0.061,-118387457.924629,0.008,775.156,48.5,32.964,0810bca4,8,0,22528386.777,0.241,-92249974.832392,0.009,604.018,46.9,7.744,01305cab,8,0,22528387.307,0.137,-92249977.089224,0.009,604.017,49.9,9.784,02309cab,8,0,22528386.573,0.041,-88406226.493011,0.004,578.790,53.7,11.215,01d03ca4,32,0,22686363.945,0.054,-119217646.474789,0.007,3250.487,49.6,32.423,0810bce4,32,0,22686366.736,0.229,-92896878.471411,0.009,2532.846,47.3,7.403,01305ceb,32,0,22686366.916,0.139,-92896871.724675,0.008,2532.847,49.7,9.781,02309ceb,32,0,22686366.199,0.043,-89026173.111600,0.005,2427.340,51.9,11.213,01d03ce4,24,0,22705864.682,0.086,-119320124.813728,0.010,1378.939,45.6,32.283,0810bd04,24,0,22705867.730,0.304,-92976731.939616,0.014,1074.499,44.9,7.403,01305d0b,24,0,22705867.765,0.170,-92976726.195712,0.011,1074.498,47.9,9.782,02309d0b,24,0,22705868.751,0.045,-89102704.360517,0.006,1029.886,50.4,11.143,01d03d04,21,0,25107621.799,0.157,-131941443.240112,0.015,3049.215,40.2,33.414,0810bd24,21,0,25107622.272,0.511,-102811515.463824,0.019,2376.015,40.4,7.495,01305d2b,18,0,22428269.766,0.060,-117861351.677555,0.007,-3118.904,48.7,33.104,0810bd44,18,0,22428270.582,0.341,-91840018.377299,0.009,-2430.314,48.0,1.744,01305d4b,18,0,22428270.851,0.197,-91840020.623335,0.008,-2430.314,50.7,4.004,02309d4b,18,0,22428276.184,0.070,-88013371.028066,0.008,-2328.945,53.6,5.444,01d03d44,15,0,23762435.332,0.086,-124872439.418394,0.010,-3618.623,45.6,32.360,0810bda4,15,0,23762436.149,0.288,-97303201.768388,0.010,-2819.707,45.4,7.739,01305dab,15,0,23762436.478,0.224,-97303200.020764,0.011,-2819.708,45.6,9.680,02309dab,10,0,20501716.740,0.030,-107737247.457800,0.006,669.502,54.6,33.111,1810bdc4,10,0,20501718.526,0.218,-83951109.550164,0.012,521.690,52.1,1.410,11305dcb,10,0,20501718.446,0.137,-83951105.806965,0.006,521.690,53.2,4.610,02309dcb,10,0,20501717.497,0.069,-80453142.024519,0.012,500.008,55.8,5.451,01d03dc4,27,0,21865687.995,0.047,-114904964.957329,0.009,-1313.509,50.8,33.066,0810bde4,27,0,21865689.816,0.277,-89536343.796655,0.012,-1023.514,49.9,1.406,01305deb,27,0,21865690.201,0.179,-89536345.047585,0.008,-1023.513,51.6,3.906,02309deb,27,0,21865690.220,0.069,-85805663.218944,0.005,-980.794,53.7,5.646,01d03de4,195,0,43535938.853,0.997,-228782904.394478,0.050,97.965,41.7,1.894,0805be64,39,3,23631824.987,0.161,-126103944.379482,0.008,3955.602,45.9,35.108,08119f64,39,3,23631831.178,0.095,-98080867.076891,0.010,3076.584,42.1,9.719,10b13f6b,39,3,23631830.647,0.419,-98080864.331959,0.009,3076.583,42.7,10.640,00319f6b,46,5,23394410.562,0.168,-124924846.921444,0.008,1645.097,45.5,35.091,08119fa4,46,5,23394413.840,0.138,-97163783.339546,0.008,1279.521,38.9,9.721,00b13fab,46,5,23394414.425,0.589,-97163777.588993,0.008,1279.521,39.8,10.561,10319fab,38,8,19441169.444,0.127,-103924192.060146,0.007,1354.658,47.9,35.004,18119c04,38,8,19441175.548,0.072,-80829946.284019,0.007,1053.623,44.6,9.654,10b13c0b,38,8,19441174.425,0.295,-80829936.531229,0.007,1053.623,45.8,10.554,00319c0b,61,9,19376423.087,0.084,-103614440.682349,0.005,765.021,51.4,36.434,18119c24,61,9,19376426.033,0.044,-80589020.713352,0.005,595.017,48.7,10.035,00b13c2b,61,9,19376426.215,0.193,-80589019.965771,0.005,595.016,49.3,11.032,10319c2b,60,10,20097930.916,0.086,-107510377.111368,0.006,-2789.332,51.3,35.172,08019c44,54,11,22371546.971,0.192,-119714667.286963,0.009,4013.269,44.3,34.924,08119c64,54,11,22371550.593,0.065,-93111419.410790,0.009,3121.432,45.3,10.064,10b13c6b,54,11,22371550.486,0.275,-93111420.666705,0.009,3121.432,46.1,11.216,00319c6b,44,12,23712690.875,0.292,-126935892.950525,0.011,-4027.379,40.8,34.049,08119c84,44,12,23712694.012,0.097,-98727926.080471,0.012,-3132.409,42.0,9.640,00b13c8b,44,12,23712694.402,0.452,-98727919.338653,0.012,-3132.408,42.0,10.560,00319c8b,45,13,19959067.661,0.102,-106879900.540449,0.005,-2217.328,49.6,36.453,08119ca4,45,13,19959068.752,0.045,-83128816.666560,0.006,-1724.589,48.7,9.712,00b13cab,45,13,19959068.964,0.205,-83128812.915366,0.005,-1724.589,48.9,10.632,10319cab,36,0,25125640.606,0.171,-132036132.059647,0.007,2122.474,50.4,4.515,08539cc4,36,0,25125647.090,0.096,-98598432.570003,0.038,1584.968,51.3,2.253,01933cc4,36,0,25125643.494,0.042,-101170552.953031,0.043,1626.410,52.7,2.255,02333cc4,36,0,25125644.280,0.089,-99884485.431022,0.006,1605.671,54.1,2.175,02933cc4,4,0,23704831.386,0.043,-124569729.390211,0.006,506.173,53.5,33.260,08539d24,4,0,23704835.118,0.041,-93022862.014097,0.004,378.062,54.9,11.238,01933d24,4,0,23704831.592,0.013,-95449533.729992,0.004,387.896,55.7,11.239,02333d24,4,0,23704832.864,0.040,-94236197.040798,0.004,382.983,57.7,10.939,02933d24,9,0,24337113.933,0.071,-127892396.521105,0.006,2796.031,50.2,26.798,08539dc4,9,0,24337119.145,0.055,-95504079.306541,0.006,2087.980,48.9,9.633,01933dc4,9,0,24337116.399,0.022,-97995481.519361,0.005,2142.473,51.7,9.634,02333dc4,9,0,24337117.533,0.050,-96749775.081781,0.005,2115.195,52.8,9.358,02933dc4,11,0,21764394.000,0.078,-114372662.230968,0.016,-509.639,50.4,21.106,08539e04,11,0,21764394.874,0.046,-85408159.916046,0.011,-380.564,51.2,10.826,01933e04,11,0,21764391.156,0.022,-87636185.192937,0.012,-390.407,51.4,10.758,02333e04,11,0,21764392.912,0.043,-86522171.723775,0.011,-385.475,53.8,10.766,02933e04,30,0,25347490.171,0.186,-131991032.051733,0.010,-3521.784,44.8,14.154,18149ec4,30,0,25347493.223,0.106,-102063849.002439,0.011,-2723.240,44.4,4.254,01743ec4,35,0,23318103.142,0.045,-121423482.169649,0.006,1585.411,51.0,33.541,08149ee4,35,0,23318106.047,0.074,-91505254.770271,0.008,1194.859,50.2,5.640,01343ee4,35,0,23318102.856,0.060,-93892337.335754,0.006,1225.981,47.9,9.041,01743ee4,32,0,24329230.899,0.066,-126688688.281939,0.008,-2669.839,47.8,32.658,18149f44,32,0,24329238.229,0.071,-95473152.992887,0.007,-2011.999,51.9,5.638,01343f44,32,0,24329234.245,0.054,-97963742.895374,0.005,-2064.471,49.0,9.818,01743f44,10,0,41200419.719,0.220,-214541396.438501,0.014,307.525,40.8,16.261,18149fa4,10,0,41200417.817,0.457,-165897011.707388,0.015,237.583,43.8,4.441,003497a4,23,0,26370558.823,0.115,-137318419.659615,0.010,1565.900,44.6,22.771,18149c84,23,0,26370558.170,0.084,-103483735.258005,0.014,1180.124,47.7,4.991,01343c84,23,0,26370554.485,0.071,-106183296.719354,0.008,1210.904,44.2,9.447,01743c84,19,0,23335949.237,0.049,-121516411.434889,0.006,2516.239,50.2,34.901,18149d04,19,0,23335951.484,0.072,-91575284.852926,0.011,1896.299,51.3,5.641,01343d04,19,0,23335948.398,0.056,-93964194.214218,0.006,1945.745,48.6,9.637,11743d04,20,0,21777498.539,0.035,-113401148.742895,0.005,-44.823,53.3,33.626,18149d24,20,0,21777497.588,0.071,-85459578.394430,0.015,-33.735,55.6,5.045,01343d24,20,0,21777494.410,0.051,-87688945.587293,0.004,-34.668,53.0,9.440,11743d24,25,0,26812696.158,0.132,-139620743.984209,0.012,-710.154,42.6,27.589,18149e24,25,0,26812698.725,0.104,-105218789.152803,0.012,-535.063,43.7,4.989,01343e24,25,0,26812696.208,0.075,-107963615.107809,0.009,-549.023,42.7,10.246,11743e24,29,0,22258326.689,0.039,-115904947.604645,0.006,-1580.344,52.3,33.604,08149e44,29,0,22258328.914,0.071,-87346461.870361,0.006,-1190.900,53.8,5.245,01343e44,29,0,22258324.535,0.053,-89625050.062736,0.005,-1221.969,50.8,9.436,01743e44*aced3074\r\n";

   pclMyFilter->IncludeMessageId(43, HEADERFORMAT::ASCII); // Filter for RANGE
   pclMyFilter->IncludeTimeStatus(TIME_STATUS::FINESTEERING); // Filter for FINESTEERING
   pclMyFilter->SetIncludeLowerTimeBound(2180, 407595);

   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter3))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter4))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter5))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter6))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter7))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter8))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter9))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter10))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter11))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter12))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter13))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter14))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter15))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter16))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter17))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter18))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter19))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter20))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter21))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter22))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter23))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter24))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter25))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter26))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter27))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter28))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter29))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter30))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter31))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter32))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter33))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter34))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter35))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter36))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter37))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(filtered_log))));
}

TEST_F(FilterTest, MIX_2_INVERTED)
{
   const char* log_to_filter1 = "#TIMEA,COM1,0,6.0,UNKNOWN,0,25.000,024c0020,9924,32768;INVALID,0.000000000,0.000333564,0.00000000000,0,0,0,0,0,0,INVALID*6fe3e59a\r\n";
   const char* filtered_log_1 = "#RANGEA,COM1,0,5.5,UNKNOWN,0,25.000,024c00a0,5103,32768;0*943a8919\r\n";
   const char* log_to_filter2 = "#BESTPOSA,COM1,0,6.0,UNKNOWN,0,25.000,024c0020,cdba,32768;INSUFFICIENT_OBS,NONE,0.00000000000,0.00000000000,0.0000,0.0000,WGS84,0.0000,0.0000,0.0000,\"\",0.000,0.000,0,0,0,0,00,00,00,00*fb4efeaa\r\n";
   const char* log_to_filter3 = "#TIMEA,COM1,0,6.5,UNKNOWN,0,26.000,024c0020,9924,32768;INVALID,0.000000000,0.000333564,0.00000000000,0,0,0,0,0,0,INVALID*ca3c9c2c\r\n";
   const char* log_to_filter4 = "#BESTPOSA,COM1,0,6.5,UNKNOWN,0,26.000,024c0020,cdba,32768;INSUFFICIENT_OBS,NONE,0.00000000000,0.00000000000,0.0000,0.0000,WGS84,0.0000,0.0000,0.0000,\"\",0.000,0.000,0,0,0,0,00,00,00,00*f5a77b4d\r\n";
   const char* log_to_filter5 = "#VERSIONA,COM1,0,5.5,COARSESTEERING,2180,407587.500,024c00a0,3681,32768;2,GPSCARD,\"FFNRNNCBN\",\"BMGX15290006Y\",\"OEM729-0.00H\",\"OM7MGMNJCDND6EB\",\"OM7BR0000ABG001\",\"2021/Sep/13\",\"15:28:07\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\"*55a19852\r\n";
   const char* filtered_log_2 = "#RANGEA,COM1,0,6.5,COARSESTEERING,2180,407587.500,024c0020,5103,32768;3,8,0,22086479.072,0.079,-116065230.826912,0.008,827.920,48.3,20.465,0800bca4,32,0,22250341.055,0.070,-116926330.596180,0.007,3298.934,49.5,19.924,0800bce4,15,0,23310073.938,0.111,-122495264.853699,0.012,-3571.021,45.5,19.861,0800bda4*d32c11da\r\n";
   const char* log_to_filter6 = "#BESTPOSA,COM1,0,7.5,COARSESTEERING,2180,407587.500,024c0020,cdba,32768;INSUFFICIENT_OBS,NONE,0.00000000000,0.00000000000,0.0000,0.0000,WGS84,0.0000,0.0000,0.0000,\"\",0.000,0.000,0,0,0,0,00,00,00,00*2075f7c6\r\n";
   const char* log_to_filter7 = "#TIMEA,COM1,0,7.5,COARSESTEERING,2180,407588.000,024c0020,9924,32768;INVALID,0.000000000,0.000333564,-17.99999999838,2021,10,21,17,12,50000,VALID*6fec570c\r\n";
   const char* log_to_filter8 = "#BESTPOSA,COM1,0,8.0,COARSESTEERING,2180,407588.000,024c0020,cdba,32768;SOL_COMPUTED,SINGLE,51.15043021293,-114.03066584294,1096.9485,0.0000,WGS84,34.6695,23.1483,74.2061,\"\",0.000,0.000,8,8,8,0,00,00,01,01*04e05542\r\n";
   const char* log_to_filter9 = "#TIMEA,COM1,0,8.0,COARSESTEERING,2180,407589.000,024c0020,9924,32768;ITERATING,-0.001480222,0.000000136,-17.99999999838,2021,10,21,17,12,51001,VALID*3271a4db\r\n";
   const char* log_to_filter10 = "#BESTPOSA,COM1,0,8.0,COARSESTEERING,2180,407589.000,024c0020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044969214,-114.03067885944,1097.8348,0.0000,WGS84,21.5220,15.8619,35.9534,\"\",0.000,0.000,18,18,18,0,00,00,11,11*9baaa764\r\n";
   const char* log_to_filter11 = "#VERSIONA,COM1,0,5.5,COARSESTEERING,2180,407590.000,024c0020,3681,32768;2,GPSCARD,\"FFNRNNCBN\",\"BMGX15290006Y\",\"OEM729-0.00H\",\"OM7MGMNJCDND6EB\",\"OM7BR0000ABG001\",\"2021/Sep/13\",\"15:28:07\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\"*0c5c14a1\r\n";
   const char* log_to_filter12 = "#TIMEA,COM1,0,5.5,COARSESTEERING,2180,407590.000,02400020,9924,32768;ITERATING,-0.001480245,5.445525581e-08,-17.99999999838,2021,10,21,17,12,52001,VALID*0804ea28\r\n";
   const char* filtered_log_3 = "#RANGEA,COM1,0,5.5,COARSESTEERING,2180,407590.000,024c0020,5103,32768;41,23,0,20296030.375,0.055,-106656359.351969,0.006,-1522.264,51.0,22.611,0810bc24,23,0,20296034.364,2.872,-79645997.562468,0.007,-1136.719,55.7,0.830,01d03c24,8,0,22086085.415,0.074,-116063162.380520,0.007,825.378,48.4,22.965,0810bca4,8,0,22086087.843,0.095,-86670551.251795,0.048,616.253,53.7,1.216,01d03ca4,32,0,22248771.717,0.066,-116918083.707594,0.007,3297.228,49.5,22.424,0810bce4,32,0,22248773.901,0.100,-87308967.147285,0.039,2462.156,51.8,1.214,01d03ce4,24,0,22264715.219,0.105,-117001867.872692,0.009,1428.997,45.6,22.284,0810bd04,24,0,22264719.059,0.105,-87371538.460072,0.072,1067.142,50.2,1.145,01d03d04,21,0,24669644.466,0.188,-129639857.490297,0.018,3093.623,40.2,23.416,0800bd24,15,0,23311772.856,0.104,-122504192.563972,0.010,-3572.507,45.6,22.361,0800bda4,39,3,23195458.666,0.174,-123775410.587280,0.008,4001.722,46.6,25.110,08119f64,39,3,23195462.989,0.847,-96269782.463670,0.008,3112.451,42.9,0.641,0031976b,46,5,22953717.926,0.189,-122571572.431876,0.008,1696.585,45.9,25.092,08119fa4,46,5,22953719.900,1.160,-95333453.473848,0.008,1319.566,39.8,0.563,003197ab,38,8,18999930.055,0.143,-101565513.986405,0.006,1405.812,48.4,25.005,18119c04,38,8,18999932.172,0.697,-78995409.137779,0.006,1093.409,45.4,0.555,0031940b,61,9,18934081.726,0.093,-101249042.682301,0.005,817.900,51.8,26.435,18119c24,61,9,18934084.342,0.367,-78749266.465596,0.005,636.145,49.3,1.033,0031942b,60,10,19648944.071,0.097,-105108600.620312,0.006,-2737.993,51.7,25.173,08019c44,54,11,21935268.469,0.220,-117380052.385700,0.008,4060.476,44.6,24.925,08119c64,54,11,21935271.786,0.533,-91295609.074792,0.008,3158.148,46.1,1.217,0031946b,44,12,23261389.160,0.307,-124520037.179708,0.010,-3982.786,41.8,24.051,08119c84,44,12,23261389.788,0.868,-96848920.409648,0.010,-3097.722,42.6,0.562,0031948b,45,13,19511152.838,0.114,-104481336.951767,0.005,-2167.651,50.0,26.455,08119ca4,45,13,19511153.255,0.554,-81263263.963896,0.005,-1685.951,48.8,0.633,003194ab,4,0,23262018.929,0.052,-122242734.095053,0.006,554.113,53.5,23.261,08539d24,4,0,23262022.476,0.094,-91285170.722265,0.048,413.790,54.7,1.240,01933d24,4,0,23262019.108,0.031,-93666511.360582,0.076,424.600,55.8,1.240,02333d24,4,0,23262020.382,1.260,-92475840.210258,0.004,419.179,57.5,0.941,02933d24,9,0,23898656.058,0.091,-125588285.025036,0.007,2841.674,50.1,16.799,08439dc4,11,0,21319648.513,0.110,-112035509.001801,0.015,-461.787,50.2,11.107,08539e04,11,0,21319649.335,3.312,-83662883.158997,0.012,-344.930,50.9,0.827,01933e04,11,0,21319645.998,2.960,-85845379.482821,0.129,-353.888,51.5,0.760,02333e04,11,0,21319647.463,3.898,-84754130.488752,0.009,-349.423,53.6,0.767,02933e04,35,0,22877372.394,0.054,-119128481.299582,0.005,1633.597,51.0,23.542,08049ee4,32,0,23880326.384,0.079,-124351124.940607,0.007,-2624.954,47.9,22.659,18049f44,23,0,25929790.141,0.218,-135023220.319811,0.010,1613.334,44.5,12.772,18049c84,19,0,22897003.727,0.057,-119230706.390177,0.006,2562.076,50.2,24.902,18049d04,20,0,21333636.698,0.042,-111089843.666973,0.005,2.533,53.2,23.627,18049d24,25,0,26367558.065,0.173,-137302792.936779,0.011,-661.552,42.4,17.590,18049e24,29,0,21811517.639,0.046,-113578295.384806,0.006,-1531.856,52.3,23.606,08049e44*f528c67b\r\n";
   const char* log_to_filter13 = "#BESTPOSA,COM1,0,5.5,COARSESTEERING,2180,407590.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044625442,-114.03068674478,1098.5042,-17.0000,WGS84,1.9497,1.2176,3.6853,\"\",0.000,0.000,24,18,18,0,00,02,11,11*2660ffae\r\n";
   const char* log_to_filter14 = "#TIMEA,COM1,0,5.5,COARSESTEERING,2180,407591.000,02400020,9924,32768;ITERATING,-0.001480327,5.208111551e-09,-17.99999999838,2021,10,21,17,12,53001,VALID*c46d1791\r\n";
   const char* log_to_filter15 = "#BESTPOSA,COM1,0,5.5,COARSESTEERING,2180,407591.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044507847,-114.03068659595,1098.3277,-17.0000,WGS84,1.5509,1.0997,2.5066,\"\",0.000,0.000,24,24,24,0,00,02,11,11*309fb715\r\n";
   const char* log_to_filter16 = "#TIMEA,COM1,0,7.0,COARSESTEERING,2180,407592.000,02400020,9924,32768;ITERATING,-0.001480349,3.184623638e-09,-17.99999999838,2021,10,21,17,12,54001,VALID*b64126e3\r\n";
   const char* log_to_filter17 = "#BESTPOSA,COM1,0,7.0,COARSESTEERING,2180,407592.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044507358,-114.03068646694,1098.2902,-17.0000,WGS84,1.5509,1.0992,2.5061,\"\",0.000,0.000,24,24,24,0,00,02,11,11*91248d43\r\n";
   const char* log_to_filter18 = "#TIMEA,COM1,0,6.5,COARSESTEERING,2180,407593.000,02400020,9924,32768;ITERATING,-0.001480363,2.501886198e-09,-17.99999999838,2021,10,21,17,12,55001,VALID*b5cf0420\r\n";
   const char* log_to_filter19 = "#BESTPOSA,COM1,0,6.5,COARSESTEERING,2180,407593.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044498855,-114.03068624941,1098.2669,-17.0000,WGS84,1.5510,1.0991,2.5060,\"\",0.000,0.000,24,24,24,0,00,02,11,11*d3b81bb5\r\n";
   const char* log_to_filter20 = "#TIMEA,COM1,0,6.5,COARSESTEERING,2180,407594.000,02400020,9924,32768;ITERATING,-0.001480371,2.131839123e-09,-17.99999999838,2021,10,21,17,12,56001,VALID*741b4855\r\n";
   const char* log_to_filter21 = "#BESTPOSA,COM1,0,6.5,COARSESTEERING,2180,407594.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044476808,-114.03068625223,1098.2658,-17.0000,WGS84,1.5511,1.0989,2.5060,\"\",0.000,0.000,27,24,24,0,00,02,11,11*c3b290f9\r\n";
   const char* log_to_filter22 = "#TIMEA,COM1,0,7.5,COARSESTEERING,2180,407595.000,02400020,9924,32768;ITERATING,-0.001480373,1.892068473e-09,-17.99999999838,2021,10,21,17,12,57001,VALID*dd7d6731\r\n";
   const char* filtered_log_4 = "#RANGEA,COM1,0,5.5,COARSESTEERING,2180,407595.000,02400020,5103,32768;77,23,0,20297500.868,0.052,-106664086.632315,0.006,-1562.684,50.7,27.611,0810bc24,23,0,20297500.632,0.265,-83114871.442232,0.005,-1217.676,50.4,1.250,01305c2b,23,0,20297501.045,0.140,-83114872.694162,0.007,-1217.677,53.0,4.512,02309c2b,23,0,20297505.319,0.067,-79651767.930981,0.004,-1166.878,55.9,5.830,01d03c24,8,0,22085323.035,0.069,-116059155.718779,0.008,783.431,48.2,27.965,0810bca4,8,0,22085325.492,0.358,-90435713.372734,0.009,610.466,47.5,2.745,01305cab,8,0,22085325.995,0.198,-90435715.632605,0.008,610.465,49.8,4.785,02309cab,8,0,22085325.442,0.066,-86667559.258654,0.006,585.051,53.8,6.216,01d03ca4,32,0,22245656.498,0.061,-116901713.393351,0.007,3257.008,49.3,27.424,0810bce4,32,0,22245659.125,0.360,-91092255.293990,0.011,2537.928,47.4,2.405,01305ceb,32,0,22245659.231,0.201,-91092248.543947,0.007,2537.927,49.7,4.783,02309ceb,32,0,22245658.803,0.068,-87296742.558874,0.006,2432.279,52.0,6.214,01d03ce4,24,0,22263378.104,0.098,-116994841.931118,0.009,1387.328,45.2,27.284,0810bd04,24,0,22263381.064,0.464,-91164823.200386,0.014,1081.034,45.3,2.405,01305d0b,24,0,22263381.254,0.244,-91164817.453840,0.010,1081.036,48.0,4.784,02309d0b,24,0,22263382.083,0.071,-87366291.822250,0.008,1036.050,50.4,6.145,01d03d04,21,0,24666722.444,0.168,-129624501.758570,0.015,3054.951,40.4,28.416,0810bd24,21,0,24666722.744,0.790,-101006106.505545,0.019,2380.481,40.6,2.497,01305d2b,18,0,21981503.954,0.067,-115513581.695168,0.007,-3112.239,48.3,28.105,0800bd44,15,0,23315193.939,0.100,-122522170.201077,0.012,-3612.525,45.0,27.361,0810bda4,15,0,23315194.577,0.472,-95471823.159175,0.011,-2814.954,45.1,2.741,01305dab,15,0,23315194.509,0.340,-95471821.421129,0.012,-2814.955,45.1,4.682,02309dab,10,0,20058554.660,0.034,-105408414.831429,0.006,676.520,54.2,28.112,1800bdc4,27,0,21420640.151,0.053,-112566222.962255,0.007,-1305.266,50.4,28.067,0810bde4,27,0,21420642.131,5.656,-84059200.042173,0.044,-974.588,53.6,0.647,01d03de4,39,3,23191730.713,0.167,-123755517.439732,0.008,3961.597,46.2,30.110,08119f64,39,3,23191737.435,0.130,-96254312.753414,0.009,3081.246,42.7,4.721,10b13f6b,39,3,23191737.125,0.527,-96254310.011505,0.008,3081.244,43.5,5.641,00319f6b,46,5,22952152.164,0.171,-122563211.547077,0.008,1653.720,46.0,30.092,08119fa4,46,5,22952155.579,0.197,-95326955.817892,0.009,1286.228,39.0,4.722,00b13fab,46,5,22952155.755,0.808,-95326950.071980,0.009,1286.228,39.8,5.563,10319fab,38,8,18998637.906,0.130,-101558607.040641,0.006,1363.216,48.4,30.005,18119c04,38,8,18998643.997,0.103,-78990046.826712,0.006,1060.280,44.6,4.656,10b13c0b,38,8,18998642.764,0.407,-78990037.073071,0.006,1060.279,45.8,5.555,00319c0b,61,9,18933340.219,0.085,-101245077.461238,0.005,774.337,51.9,31.435,18119c24,61,9,18933343.261,0.061,-78746182.652719,0.005,602.263,48.8,5.036,00b13c2b,61,9,18933343.113,0.259,-78746181.906229,0.005,602.261,49.3,6.033,10319c2b,60,10,19651526.114,0.088,-105122412.778005,0.005,-2780.830,51.8,30.173,08019c44,54,11,21931496.436,0.190,-117359867.373328,0.008,4019.855,45.1,29.925,08119c64,54,11,21931499.657,0.092,-91279908.365263,0.008,3126.554,45.3,5.066,10b13c6b,54,11,21931500.087,0.378,-91279909.619295,0.008,3126.552,45.9,6.217,00319c6b,44,12,23265130.397,0.268,-124540065.478237,0.010,-4022.401,42.2,29.051,08119c84,44,12,23265132.939,0.134,-96864504.724735,0.011,-3128.535,42.4,4.642,00b13c8b,44,12,23265134.440,0.590,-96864497.977821,0.011,-3128.535,42.5,5.562,00319c8b,45,13,19513199.233,0.104,-104492295.524177,0.005,-2209.578,50.1,31.455,08119ca4,45,13,19513200.373,0.065,-81271790.545637,0.006,-1718.560,48.7,4.714,00b13cab,45,13,19513200.665,0.281,-81271786.796770,0.006,-1718.560,48.9,5.633,10319cab,4,0,23261513.970,0.049,-122240080.567102,0.006,513.442,53.2,28.261,08539d24,4,0,23261517.679,0.065,-91283189.196609,0.006,383.352,54.8,6.240,01933d24,4,0,23261514.144,0.017,-93664478.142199,0.010,393.414,55.8,6.240,02333d24,4,0,23261515.437,0.065,-92473832.837046,0.004,388.382,57.7,5.941,02933d24,9,0,23895974.018,0.082,-125574190.676455,0.006,2802.065,49.9,21.799,08539dc4,9,0,23895979.211,0.083,-93772951.560656,0.014,2092.436,48.8,4.634,01933dc4,9,0,23895976.481,0.032,-96219193.923071,0.019,2147.062,51.9,4.635,02333dc4,9,0,23895977.631,0.077,-94996067.408674,0.005,2119.773,52.7,4.359,02933dc4,11,0,21320110.232,0.094,-112037935.358484,0.016,-502.672,50.0,16.107,08539e04,11,0,21320111.176,0.072,-83664695.046743,0.011,-375.390,51.1,5.827,01933e04,11,0,21320107.412,0.029,-85847238.635632,0.020,-385.203,51.6,5.760,02333e04,11,0,21320109.153,0.068,-84755966.011379,0.011,-380.314,53.8,5.767,02933e04,30,0,24900309.826,0.291,-129662447.050890,0.008,-3516.229,45.6,9.155,18049ec4,35,0,22875826.226,0.048,-119120430.228863,0.005,1592.785,51.2,28.542,08149ee4,35,0,22875828.809,5.656,-89769665.177532,0.080,1200.265,50.0,0.641,01343ee4,35,0,22875825.944,0.090,-92111471.486866,0.020,1231.668,48.2,4.042,01743ee4,32,0,23882868.520,0.071,-124364362.318052,0.007,-2663.985,47.9,27.659,18149f44,32,0,23882875.101,5.656,-93721531.198517,0.062,-2007.573,51.6,0.639,01343f44,32,0,23882871.851,0.081,-96166426.626690,0.009,-2059.995,49.1,4.819,01743f44,23,0,25928263.061,0.131,-135015269.286980,0.008,1572.973,44.8,17.772,18149c84,23,0,25928258.755,0.105,-104402354.741234,0.016,1216.402,44.3,4.448,01743c84,19,0,22894565.383,0.052,-119218009.616373,0.006,2522.539,50.3,29.902,18149d04,19,0,22894567.405,5.656,-89843199.600471,0.127,1901.049,51.0,0.642,01343d04,19,0,22894564.496,0.084,-92186924.132240,0.013,1950.624,48.8,4.638,11743d04,20,0,21333656.505,0.038,-111089946.855726,0.005,-37.780,53.3,28.627,18149d24,20,0,21333652.378,0.076,-85901777.681990,0.014,-29.197,53.2,4.441,11743d24,25,0,26368215.794,0.150,-137306218.129744,0.011,-702.371,42.3,22.590,18149e24,25,0,26368215.690,0.109,-106173876.909266,0.013,-543.206,42.7,5.247,11743e24,29,0,21813011.021,0.042,-113586071.992727,0.005,-1572.769,52.4,28.606,08149e44,29,0,21813008.843,0.080,-87831948.349440,0.015,-1216.090,50.9,4.438,01743e44*0475a421\r\n";
   const char* log_to_filter23 = "#BESTPOSA,COM1,0,7.5,COARSESTEERING,2180,407595.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044243058,-114.03068360219,1097.7004,-17.0000,WGS84,1.3792,0.9846,2.2407,\"\",0.000,0.000,28,27,27,0,00,02,11,11*9a57a6f4\r\n";
   const char* log_to_filter24 = "#TIMEA,COM1,0,7.5,FINESTEERING,2180,407596.000,02400020,9924,32768;ITERATING,-0.001480373,1.694616069e-09,-17.99999999838,2021,10,21,17,12,58001,VALID*ca78e438\r\n";
   const char* log_to_filter25 = "#BESTPOSA,COM1,0,7.5,FINESTEERING,2180,407596.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044308398,-114.03068382954,1097.7873,-17.0000,WGS84,1.1918,0.9784,2.0965,\"\",0.000,0.000,29,28,28,0,00,02,11,11*81ec1d55\r\n";
   const char* log_to_filter26 = "#TIMEA,COM1,0,7.5,FINESTEERING,2180,407597.000,02400020,9924,32768;ITERATING,-1.056282464e-08,4.980118958e-09,-17.99999999838,2021,10,21,17,12,59000,VALID*86a8f91f\r\n";
   const char* log_to_filter27 = "#BESTPOSA,COM1,0,7.5,FINESTEERING,2180,407597.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044349643,-114.03068418925,1097.7073,-17.0000,WGS84,1.1581,0.9671,2.0529,\"\",0.000,0.000,30,29,29,0,00,02,11,11*4f210b38\r\n";
   const char* log_to_filter28 = "#TIMEA,COM1,0,7.5,FINESTEERING,2180,407598.000,02400020,9924,32768;ITERATING,-8.720389968e-09,2.851254543e-09,-17.99999999838,2021,10,21,17,13,0,VALID*6046ea6b\r\n";
   const char* log_to_filter29 = "#BESTPOSA,COM1,0,7.5,FINESTEERING,2180,407598.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044380692,-114.03068422806,1097.7573,-17.0000,WGS84,1.1254,0.9541,2.0500,\"\",0.000,0.000,30,30,30,0,00,02,11,11*6dea55c5\r\n";
   const char* log_to_filter30 = "#TIMEA,COM1,0,8.5,FINESTEERING,2180,407599.000,02400020,9924,32768;ITERATING,-5.706091940e-09,2.208897498e-09,-17.99999999838,2021,10,21,17,13,1000,VALID*19f9590d\r\n";
   const char* log_to_filter31 = "#BESTPOSA,COM1,0,8.5,FINESTEERING,2180,407599.000,02400020,cdba,32768;SOL_COMPUTED,SINGLE,51.15044360617,-114.03068408017,1097.7294,-17.0000,WGS84,1.1247,0.9540,2.0496,\"\",0.000,0.000,31,30,30,0,00,02,11,11*09f5c77f\r\n";
   const char* log_to_filter32 = "#VERSIONA,COM1,0,5.5,FINESTEERING,2180,407600.000,02400020,3681,32768;2,GPSCARD,\"FFNRNNCBN\",\"BMGX15290006Y\",\"OEM729-0.00H\",\"OM7MGMNJCDND6EB\",\"OM7BR0000ABG001\",\"2021/Sep/13\",\"15:28:07\",OEM7FPGA,\"\",\"\",\"\",\"OMV070001RN0000\",\"\",\"\",\"\"*6ebd192f\r\n";
   const char* log_to_filter33 = "#TIMEA,COM1,0,8.5,FINESTEERING,2180,407600.000,02400020,9924,32768;ITERATING,-2.698749387e-09,1.871232507e-09,-17.99999999838,2021,10,21,17,13,2000,VALID*500b310b\r\n";
   const char* filtered_log_5 = "#RANGEA,COM1,0,7.5,FINESTEERING,2180,407600.000,02400020,5103,32768;97,23,0,20742792.951,0.046,-109004112.049799,0.006,-1569.674,51.1,32.610,0810bc24,23,0,20742792.750,0.186,-84938267.863909,0.006,-1223.123,50.0,6.249,01305c2b,23,0,20742792.847,0.096,-84938269.116295,0.006,-1223.124,53.0,9.510,02309c2b,23,0,20742797.345,0.042,-81399189.508091,0.004,-1172.077,55.9,10.829,01d03c24,8,0,22528384.243,0.061,-118387457.924629,0.008,775.156,48.5,32.964,0810bca4,8,0,22528386.777,0.241,-92249974.832392,0.009,604.018,46.9,7.744,01305cab,8,0,22528387.307,0.137,-92249977.089224,0.009,604.017,49.9,9.784,02309cab,8,0,22528386.573,0.041,-88406226.493011,0.004,578.790,53.7,11.215,01d03ca4,32,0,22686363.945,0.054,-119217646.474789,0.007,3250.487,49.6,32.423,0810bce4,32,0,22686366.736,0.229,-92896878.471411,0.009,2532.846,47.3,7.403,01305ceb,32,0,22686366.916,0.139,-92896871.724675,0.008,2532.847,49.7,9.781,02309ceb,32,0,22686366.199,0.043,-89026173.111600,0.005,2427.340,51.9,11.213,01d03ce4,24,0,22705864.682,0.086,-119320124.813728,0.010,1378.939,45.6,32.283,0810bd04,24,0,22705867.730,0.304,-92976731.939616,0.014,1074.499,44.9,7.403,01305d0b,24,0,22705867.765,0.170,-92976726.195712,0.011,1074.498,47.9,9.782,02309d0b,24,0,22705868.751,0.045,-89102704.360517,0.006,1029.886,50.4,11.143,01d03d04,21,0,25107621.799,0.157,-131941443.240112,0.015,3049.215,40.2,33.414,0810bd24,21,0,25107622.272,0.511,-102811515.463824,0.019,2376.015,40.4,7.495,01305d2b,18,0,22428269.766,0.060,-117861351.677555,0.007,-3118.904,48.7,33.104,0810bd44,18,0,22428270.582,0.341,-91840018.377299,0.009,-2430.314,48.0,1.744,01305d4b,18,0,22428270.851,0.197,-91840020.623335,0.008,-2430.314,50.7,4.004,02309d4b,18,0,22428276.184,0.070,-88013371.028066,0.008,-2328.945,53.6,5.444,01d03d44,15,0,23762435.332,0.086,-124872439.418394,0.010,-3618.623,45.6,32.360,0810bda4,15,0,23762436.149,0.288,-97303201.768388,0.010,-2819.707,45.4,7.739,01305dab,15,0,23762436.478,0.224,-97303200.020764,0.011,-2819.708,45.6,9.680,02309dab,10,0,20501716.740,0.030,-107737247.457800,0.006,669.502,54.6,33.111,1810bdc4,10,0,20501718.526,0.218,-83951109.550164,0.012,521.690,52.1,1.410,11305dcb,10,0,20501718.446,0.137,-83951105.806965,0.006,521.690,53.2,4.610,02309dcb,10,0,20501717.497,0.069,-80453142.024519,0.012,500.008,55.8,5.451,01d03dc4,27,0,21865687.995,0.047,-114904964.957329,0.009,-1313.509,50.8,33.066,0810bde4,27,0,21865689.816,0.277,-89536343.796655,0.012,-1023.514,49.9,1.406,01305deb,27,0,21865690.201,0.179,-89536345.047585,0.008,-1023.513,51.6,3.906,02309deb,27,0,21865690.220,0.069,-85805663.218944,0.005,-980.794,53.7,5.646,01d03de4,195,0,43535938.853,0.997,-228782904.394478,0.050,97.965,41.7,1.894,0805be64,39,3,23631824.987,0.161,-126103944.379482,0.008,3955.602,45.9,35.108,08119f64,39,3,23631831.178,0.095,-98080867.076891,0.010,3076.584,42.1,9.719,10b13f6b,39,3,23631830.647,0.419,-98080864.331959,0.009,3076.583,42.7,10.640,00319f6b,46,5,23394410.562,0.168,-124924846.921444,0.008,1645.097,45.5,35.091,08119fa4,46,5,23394413.840,0.138,-97163783.339546,0.008,1279.521,38.9,9.721,00b13fab,46,5,23394414.425,0.589,-97163777.588993,0.008,1279.521,39.8,10.561,10319fab,38,8,19441169.444,0.127,-103924192.060146,0.007,1354.658,47.9,35.004,18119c04,38,8,19441175.548,0.072,-80829946.284019,0.007,1053.623,44.6,9.654,10b13c0b,38,8,19441174.425,0.295,-80829936.531229,0.007,1053.623,45.8,10.554,00319c0b,61,9,19376423.087,0.084,-103614440.682349,0.005,765.021,51.4,36.434,18119c24,61,9,19376426.033,0.044,-80589020.713352,0.005,595.017,48.7,10.035,00b13c2b,61,9,19376426.215,0.193,-80589019.965771,0.005,595.016,49.3,11.032,10319c2b,60,10,20097930.916,0.086,-107510377.111368,0.006,-2789.332,51.3,35.172,08019c44,54,11,22371546.971,0.192,-119714667.286963,0.009,4013.269,44.3,34.924,08119c64,54,11,22371550.593,0.065,-93111419.410790,0.009,3121.432,45.3,10.064,10b13c6b,54,11,22371550.486,0.275,-93111420.666705,0.009,3121.432,46.1,11.216,00319c6b,44,12,23712690.875,0.292,-126935892.950525,0.011,-4027.379,40.8,34.049,08119c84,44,12,23712694.012,0.097,-98727926.080471,0.012,-3132.409,42.0,9.640,00b13c8b,44,12,23712694.402,0.452,-98727919.338653,0.012,-3132.408,42.0,10.560,00319c8b,45,13,19959067.661,0.102,-106879900.540449,0.005,-2217.328,49.6,36.453,08119ca4,45,13,19959068.752,0.045,-83128816.666560,0.006,-1724.589,48.7,9.712,00b13cab,45,13,19959068.964,0.205,-83128812.915366,0.005,-1724.589,48.9,10.632,10319cab,36,0,25125640.606,0.171,-132036132.059647,0.007,2122.474,50.4,4.515,08539cc4,36,0,25125647.090,0.096,-98598432.570003,0.038,1584.968,51.3,2.253,01933cc4,36,0,25125643.494,0.042,-101170552.953031,0.043,1626.410,52.7,2.255,02333cc4,36,0,25125644.280,0.089,-99884485.431022,0.006,1605.671,54.1,2.175,02933cc4,4,0,23704831.386,0.043,-124569729.390211,0.006,506.173,53.5,33.260,08539d24,4,0,23704835.118,0.041,-93022862.014097,0.004,378.062,54.9,11.238,01933d24,4,0,23704831.592,0.013,-95449533.729992,0.004,387.896,55.7,11.239,02333d24,4,0,23704832.864,0.040,-94236197.040798,0.004,382.983,57.7,10.939,02933d24,9,0,24337113.933,0.071,-127892396.521105,0.006,2796.031,50.2,26.798,08539dc4,9,0,24337119.145,0.055,-95504079.306541,0.006,2087.980,48.9,9.633,01933dc4,9,0,24337116.399,0.022,-97995481.519361,0.005,2142.473,51.7,9.634,02333dc4,9,0,24337117.533,0.050,-96749775.081781,0.005,2115.195,52.8,9.358,02933dc4,11,0,21764394.000,0.078,-114372662.230968,0.016,-509.639,50.4,21.106,08539e04,11,0,21764394.874,0.046,-85408159.916046,0.011,-380.564,51.2,10.826,01933e04,11,0,21764391.156,0.022,-87636185.192937,0.012,-390.407,51.4,10.758,02333e04,11,0,21764392.912,0.043,-86522171.723775,0.011,-385.475,53.8,10.766,02933e04,30,0,25347490.171,0.186,-131991032.051733,0.010,-3521.784,44.8,14.154,18149ec4,30,0,25347493.223,0.106,-102063849.002439,0.011,-2723.240,44.4,4.254,01743ec4,35,0,23318103.142,0.045,-121423482.169649,0.006,1585.411,51.0,33.541,08149ee4,35,0,23318106.047,0.074,-91505254.770271,0.008,1194.859,50.2,5.640,01343ee4,35,0,23318102.856,0.060,-93892337.335754,0.006,1225.981,47.9,9.041,01743ee4,32,0,24329230.899,0.066,-126688688.281939,0.008,-2669.839,47.8,32.658,18149f44,32,0,24329238.229,0.071,-95473152.992887,0.007,-2011.999,51.9,5.638,01343f44,32,0,24329234.245,0.054,-97963742.895374,0.005,-2064.471,49.0,9.818,01743f44,10,0,41200419.719,0.220,-214541396.438501,0.014,307.525,40.8,16.261,18149fa4,10,0,41200417.817,0.457,-165897011.707388,0.015,237.583,43.8,4.441,003497a4,23,0,26370558.823,0.115,-137318419.659615,0.010,1565.900,44.6,22.771,18149c84,23,0,26370558.170,0.084,-103483735.258005,0.014,1180.124,47.7,4.991,01343c84,23,0,26370554.485,0.071,-106183296.719354,0.008,1210.904,44.2,9.447,01743c84,19,0,23335949.237,0.049,-121516411.434889,0.006,2516.239,50.2,34.901,18149d04,19,0,23335951.484,0.072,-91575284.852926,0.011,1896.299,51.3,5.641,01343d04,19,0,23335948.398,0.056,-93964194.214218,0.006,1945.745,48.6,9.637,11743d04,20,0,21777498.539,0.035,-113401148.742895,0.005,-44.823,53.3,33.626,18149d24,20,0,21777497.588,0.071,-85459578.394430,0.015,-33.735,55.6,5.045,01343d24,20,0,21777494.410,0.051,-87688945.587293,0.004,-34.668,53.0,9.440,11743d24,25,0,26812696.158,0.132,-139620743.984209,0.012,-710.154,42.6,27.589,18149e24,25,0,26812698.725,0.104,-105218789.152803,0.012,-535.063,43.7,4.989,01343e24,25,0,26812696.208,0.075,-107963615.107809,0.009,-549.023,42.7,10.246,11743e24,29,0,22258326.689,0.039,-115904947.604645,0.006,-1580.344,52.3,33.604,08149e44,29,0,22258328.914,0.071,-87346461.870361,0.006,-1190.900,53.8,5.245,01343e44,29,0,22258324.535,0.053,-89625050.062736,0.005,-1221.969,50.8,9.436,01743e44*aced3074\r\n";

   pclMyFilter->IncludeMessageId(43); // Filter for RANGE
   pclMyFilter->InvertMessageIdFilter(true);

   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter1))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter2))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter3))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter4))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter5))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter6))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter7))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter8))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter9))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter10))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter11))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter12))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter13))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter14))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter15))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter16))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter17))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter18))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter19))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter20))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter21))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter22))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter23))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter24))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter25))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter26))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter27))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter28))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter29))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter30))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter31))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter32))));
   ASSERT_TRUE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(log_to_filter33))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(filtered_log_1))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(filtered_log_2))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(filtered_log_3))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(filtered_log_4))));
   ASSERT_FALSE(TestFilter(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(filtered_log_5))));
}

// -------------------------------------------------------------------------------------------------------
// FileParser Unit Tests
// -------------------------------------------------------------------------------------------------------
class FileParserTest : public ::testing::Test
{

protected:
   static FileParser* pclFp;

   // Per-test-suite setup
   static void SetUpTestSuite()
   {
      try
      {
         pclFp = new FileParser(*TEST_DB_PATH);
      }
      catch (JsonReaderFailure& e)
      {
         printf("%s\n", e.what());

         if (pclFp)
         {
            delete pclFp;
            pclFp = nullptr;
         }
      }
   }

   // Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if (pclFp)
      {
         pclFp->ShutdownLogger();
         delete pclFp;
         pclFp = nullptr;
      }
   }
};
FileParser* FileParserTest::pclFp = nullptr;

TEST_F(FileParserTest, LOGGER)
{
   spdlog::level::level_enum eLevel = spdlog::level::off;

   // FileParser logger
   ASSERT_NE(spdlog::get("novatel_fileparser"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_fileparser = pclFp->GetLogger();
   pclFp->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_fileparser->level(), eLevel);

   // Parser logger
   ASSERT_NE(spdlog::get("novatel_parser"), nullptr);
   ASSERT_NO_THROW(pclFp->EnableFramerDecoderLogging(eLevel, "novatel_parser.log"));
}

TEST_F(FileParserTest, FILEPARSER_INSTANTIATION)
{
   ASSERT_NO_THROW(FileParser fp1 = FileParser());
   ASSERT_NO_THROW(FileParser fp2 = FileParser(*TEST_DB_PATH));

   std::string sTEST_DB_PATH = *TEST_DB_PATH;
   const std::u32string usTEST_DB_PATH(sTEST_DB_PATH.begin(), sTEST_DB_PATH.end());
   ASSERT_NO_THROW(FileParser fp3 = FileParser(usTEST_DB_PATH));

   JsonReader* jsonDb = new JsonReader();
   jsonDb->LoadFile(*TEST_DB_PATH);
   ASSERT_NO_THROW(FileParser fp4 = FileParser(jsonDb));
}

TEST_F(FileParserTest, LOAD_JSON_DB_STRING)
{
   JsonReader pclMyJsonDb;
   pclMyJsonDb.LoadFile<std::string>(*TEST_DB_PATH);
   ASSERT_NO_THROW(pclFp->LoadJsonDb(&pclMyJsonDb));
   ASSERT_NO_THROW(pclFp->LoadJsonDb(nullptr));
}

TEST_F(FileParserTest, LOAD_JSON_DB_U32STRING)
{
   JsonReader pclMyJsonDb;
   std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
   std::u32string u32str = converter.from_bytes(*TEST_DB_PATH);
   pclMyJsonDb.LoadFile<std::u32string>(u32str);
   ASSERT_NO_THROW(pclFp->LoadJsonDb(&pclMyJsonDb));
   ASSERT_NO_THROW(pclFp->LoadJsonDb(nullptr));
}

TEST_F(FileParserTest, LOAD_JSON_DB_CHAR_ARRAY)
{
   JsonReader pclMyJsonDb;
   pclMyJsonDb.LoadFile<char*>(const_cast<char*>(TEST_DB_PATH->c_str()));
   ASSERT_NO_THROW(pclFp->LoadJsonDb(&pclMyJsonDb));
   ASSERT_NO_THROW(pclFp->LoadJsonDb(nullptr));
}

TEST_F(FileParserTest, RANGE_CMP)
{
   pclFp->SetDecompressRangeCmp(true);
   ASSERT_TRUE(pclFp->GetDecompressRangeCmp());
   pclFp->SetDecompressRangeCmp(false);
   ASSERT_FALSE(pclFp->GetDecompressRangeCmp());
}

TEST_F(FileParserTest, UNKNOWN_BYTES)
{
   pclFp->SetReturnUnknownBytes(true);
   ASSERT_TRUE(pclFp->GetReturnUnknownBytes());
   pclFp->SetReturnUnknownBytes(false);
   ASSERT_FALSE(pclFp->GetReturnUnknownBytes());
}

TEST_F(FileParserTest, PARSE_FILE_WITH_FILTER)
{
   // Reset the FileParser with the database because a previous test assigns it to the nullptr
   pclFp = new FileParser(*TEST_DB_PATH);
   Filter* clFilter = new Filter();
   clFilter->SetLoggerLevel(spdlog::level::debug);
   pclFp->SetFilter(clFilter);
   ASSERT_EQ(pclFp->GetFilter(), clFilter);

   std::filesystem::path test_gps_file = std::filesystem::path(*TEST_RESOURCE_PATH) / "BESTUTMBIN.GPS";
   InputFileStream clInputFileStream = InputFileStream(test_gps_file.string().c_str());
   ASSERT_TRUE(pclFp->SetStream(&clInputFileStream));

   MetaDataStruct stMetaData;
   MessageDataStruct stMessageData;

   int numSuccess = 0;
   uint32_t uiExpectedMetaDataLength[2] = { 213, 195 };
   double dExpectedMilliseconds[2] = { 270605000, 172189053 };
   uint32_t uiExpectedMessageLength[2] = { 213, 195 };

   STATUS eStatus = STATUS::UNKNOWN;
   pclFp->SetEncodeFormat(ENCODEFORMAT::ASCII);
   ASSERT_EQ(pclFp->GetEncodeFormat(), ENCODEFORMAT::ASCII);

   while (eStatus != STATUS::STREAM_EMPTY)
   {
      eStatus = pclFp->Read(stMessageData, stMetaData);
      if (eStatus == STATUS::SUCCESS)
      {
         ASSERT_EQ(stMetaData.uiLength, uiExpectedMetaDataLength[numSuccess]);
         ASSERT_DOUBLE_EQ(stMetaData.dMilliseconds, dExpectedMilliseconds[numSuccess]);
         ASSERT_EQ(stMessageData.uiMessageLength, uiExpectedMessageLength[numSuccess]);
         numSuccess++;
      }
   }
   ASSERT_EQ(pclFp->GetPercentRead(), 100U);
   ASSERT_EQ(numSuccess, 2);
}

TEST_F(FileParserTest, RESET)
{
   pclFp = new FileParser();
   ASSERT_NO_THROW(pclFp->GetInternalBuffer(););
   ASSERT_TRUE(pclFp->Reset());
}

// -------------------------------------------------------------------------------------------------------
// Novatel Types Unit Tests
// -------------------------------------------------------------------------------------------------------
class NovatelTypesTest : public::testing::Test
{

protected:
   class DecoderTester : public MessageDecoder
   {
   public:
      DecoderTester(JsonReader* pclJsonDb_) : MessageDecoder(pclJsonDb_) {}

      STATUS TestDecodeAscii(const std::vector<BaseField*> MsgDefFields_, const char** ppcLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_)
      {
         return DecodeAscii(MsgDefFields_, const_cast<char**>(ppcLogBuf_), vIntermediateFormat_);
      }

      STATUS TestDecodeBinary(const std::vector<BaseField*> MsgDefFields_, unsigned char** ppucLogBuf_, std::vector<FieldContainer>& vIntermediateFormat_)
      {
         uint16_t MsgDefFieldsSize = 0;
         for (BaseField* field : MsgDefFields_)
         {
            MsgDefFieldsSize += field->dataType.length;
         }
         return DecodeBinary(MsgDefFields_, ppucLogBuf_, vIntermediateFormat_, MsgDefFieldsSize);
      }
   };
   class EncoderTester : public Encoder
   {
   public:
      EncoderTester(JsonReader* pclJsonDb_) : Encoder(pclJsonDb_) {}

      bool TestEncodeBinaryBody(const IntermediateMessage& stIntermediateMessage_, unsigned char** ppcOutBuf_, uint32_t uiBytes)
      {
         return Encoder::EncodeBinaryBody(stIntermediateMessage_, ppcOutBuf_, uiBytes, false);
      }
   };
public:
   JsonReader* pclMyJsonDb;
   DecoderTester* pclMyDecoderTester;
   EncoderTester* pclMyEncoderTester;
   std::vector<BaseField*> MsgDefFields_;
   std::string sMinJsonDb;

   NovatelTypesTest() {
      sMinJsonDb = "{ \
                        \"enums\": [ \
                           { \
                              \"name\": \"Responses\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           }, \
                           { \
                              \"name\": \"Commands\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           }, \
                           { \
                              \"name\": \"PortAddress\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           }, \
                           { \
                              \"name\": \"GPSTimeStatus\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           } \
                        ], \
                        \"messages\": [] \
                  }";
   }

   virtual void SetUp()
   {
      try
      {
         pclMyJsonDb = new JsonReader();
         //pclMyJsonDb->LoadFile(*TEST_DB_PATH);
         pclMyJsonDb->ParseJson(sMinJsonDb);
         pclMyDecoderTester = new DecoderTester(pclMyJsonDb);
         pclMyEncoderTester = new EncoderTester(pclMyJsonDb);
      }
      catch (JsonReaderFailure& e)
      {
         printf("%s\n", e.what());

         if (pclMyJsonDb)
         {
            delete pclMyJsonDb;
            pclMyJsonDb = nullptr;
         }
         if (pclMyDecoderTester)
         {
            delete pclMyDecoderTester;
            pclMyDecoderTester = nullptr;
         }
         MsgDefFields_.clear();
      }
   }
   virtual void TearDown()
   {
      if (pclMyJsonDb)
      {
         delete pclMyJsonDb;
         pclMyJsonDb = nullptr;
      }
      if (pclMyDecoderTester)
      {
         pclMyDecoderTester->ShutdownLogger();
         delete pclMyDecoderTester;
         pclMyDecoderTester = nullptr;
      }
      MsgDefFields_.clear();
   }
   void CreateBaseField(std::string szName_, FIELD_TYPE eFieldType_, CONVERSION_STRING eConversionStripped_, uint16_t usLength_, DATA_TYPE_NAME eDataTypeName_)
   {
      BaseField* stField = new BaseField();
      stField->name = szName_;
      stField->type = eFieldType_;
      stField->conversionStripped = eConversionStripped_;
      stField->dataType.length = usLength_;
      stField->dataType.name = eDataTypeName_;
      MsgDefFields_.emplace_back(stField);
   }
   void CreateEnumField(std::string name, std::string description, int32_t value)
   {
      EnumField* stField = new EnumField();
      EnumDefinition* enumDef = new EnumDefinition();
      EnumDataType* enumDT = new EnumDataType();
      enumDT->name = name;
      enumDT->description = description;
      enumDT->value = value;
      enumDef->enumerators.push_back(*enumDT);
      stField->enumDef = enumDef;
      stField->type = FIELD_TYPE::ENUM;
      MsgDefFields_.emplace_back(stField);
   }
};

TEST_F(NovatelTypesTest, LOGGER)
{
   spdlog::level::level_enum eLevel = spdlog::level::off;

   ASSERT_NE(spdlog::get("novatel_message_decoder"), nullptr);
   std::shared_ptr<spdlog::logger> novatel_message_decoder = pclMyDecoderTester->GetLogger();
   pclMyDecoderTester->SetLoggerLevel(eLevel);
   ASSERT_EQ(novatel_message_decoder->level(), eLevel);
}

TEST_F(NovatelTypesTest, FIELD_CONTAINER_ERROR_ON_COPY)
{
   FieldContainer fc = FieldContainer(3, new BaseField());
   ASSERT_THROW(FieldContainer fc2(fc);, std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_CHAR_BYTE_VALID)
{
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(9);

   const char* testInput = "-129,-128,-127,-1,0,1,127,128,129";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[0].field_value), 127);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[1].field_value), -128);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[2].field_value), -127);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[3].field_value), -1);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[4].field_value), 0);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[5].field_value), 1);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[6].field_value), 127);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[7].field_value), -128);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[8].field_value), -127);
}

TEST_F(NovatelTypesTest, ASCI_UCHAR_BYTE_VALID)
{
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB, 1, DATA_TYPE_NAME::UCHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(9);

   const char* testInput = "-256,-255,-254,-1,0,1,254,255,256";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[0].field_value), 0);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[1].field_value), 1);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[2].field_value), 2);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[3].field_value), 255);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[4].field_value), 0);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[5].field_value), 1);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[6].field_value), 254);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[7].field_value), 255);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[8].field_value), 0);
}

TEST_F(NovatelTypesTest, ASCII_UCHAR_BYTE_INVALID)
{
   CreateBaseField("INT_1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B, 1, DATA_TYPE_NAME::CHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "0.1";
   pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[0].field_value), 0);
}

TEST_F(NovatelTypesTest, ASCII_CHAR_VALID)
{
   CreateBaseField("CHAR", FIELD_TYPE::SIMPLE, CONVERSION_STRING::c, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("CHAR", FIELD_TYPE::SIMPLE, CONVERSION_STRING::c, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("CHAR", FIELD_TYPE::SIMPLE, CONVERSION_STRING::c, 1, DATA_TYPE_NAME::CHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(3);

   const char* testInput = "#,A,;";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[0].field_value), '#');
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[1].field_value), 'A');
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[2].field_value), ';');
}

TEST_F(NovatelTypesTest, ASCII_CHAR_INVALID)
{
   CreateBaseField("CHAR", FIELD_TYPE::SIMPLE, CONVERSION_STRING::c, 2, DATA_TYPE_NAME::CHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_UCHAR_VALID)
{
   CreateBaseField("uint8_t", FIELD_TYPE::SIMPLE, CONVERSION_STRING::uc, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("uint8_t", FIELD_TYPE::SIMPLE, CONVERSION_STRING::uc, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("uint8_t", FIELD_TYPE::SIMPLE, CONVERSION_STRING::uc, 1, DATA_TYPE_NAME::UCHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(3);

   const char* testInput = "#,A,;";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[0].field_value), '#');
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[1].field_value), 'A');
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[2].field_value), ';');
}

TEST_F(NovatelTypesTest, ASCII_UCHAR_INVALID)
{
   CreateBaseField("uint8_t", FIELD_TYPE::SIMPLE, CONVERSION_STRING::uc, 2, DATA_TYPE_NAME::UCHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_INT_VALID)
{
   CreateBaseField("INT_2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 2, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 2, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 2, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 2, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 2, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 2, DATA_TYPE_NAME::UCHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(6);

   const char* testInput = "-32769,-32768,-32767,32767,32768,32769";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[0].field_value), 32767);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[1].field_value), -32768);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[2].field_value), -32767);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[3].field_value), 32767);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[4].field_value), -32768);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[5].field_value), -32767);
}

TEST_F(NovatelTypesTest, ASCII_INT_INVALID)
{
   CreateBaseField("INT_2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 3, DATA_TYPE_NAME::UCHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_FLOAT_VALID)
{
   CreateBaseField("Und", FIELD_TYPE::SIMPLE, CONVERSION_STRING::f, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("LatStd", FIELD_TYPE::SIMPLE, CONVERSION_STRING::f, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("LongStd", FIELD_TYPE::SIMPLE, CONVERSION_STRING::f, 4, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(3);

   const char* testInput = "51.11636937989,-114.03825348307,0";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_NEAR(std::get<float>(vIntermediateFormat_[0].field_value),   51.11636937989f, std::numeric_limits<float>::epsilon());
   ASSERT_NEAR(std::get<float>(vIntermediateFormat_[1].field_value), -114.03825348307f, std::numeric_limits<float>::epsilon());
   ASSERT_NEAR(std::get<float>(vIntermediateFormat_[2].field_value),                 0, std::numeric_limits<float>::epsilon());
}

TEST_F(NovatelTypesTest, ASCII_FLOAT_INVALID)
{
   CreateBaseField("Und", FIELD_TYPE::SIMPLE, CONVERSION_STRING::f, 5, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_DOUBLE_VALID)
{
   CreateBaseField("Lat", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Long", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Ht", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf, 8, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(7);

   const char* testInput = "51.11636937989,-114.03825348307,0,1.7e+308,-1.7e+308,1.7e+309,-1.7e+309";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);
   constexpr double inf = std::numeric_limits<double>::infinity();

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[0].field_value), 51.11636937989);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[1].field_value), -114.03825348307);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[2].field_value), 0);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[3].field_value), 1.7e+308);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[4].field_value), -1.7e+308);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[5].field_value), inf);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[6].field_value), -inf);
}

TEST_F(NovatelTypesTest, ASCII_DOUBLE_INVALID)
{
   CreateBaseField("Lat", FIELD_TYPE::SIMPLE, CONVERSION_STRING::f, 9, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_BOOL_VALID)
{
   CreateBaseField("B_True", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 4, DATA_TYPE_NAME::BOOL);
   CreateBaseField("B_False", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 4, DATA_TYPE_NAME::BOOL);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(2);

   const char* testInput = "TRUE,FALSE";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_TRUE(std::get<bool>(vIntermediateFormat_[0].field_value));
   ASSERT_FALSE(std::get<bool>(vIntermediateFormat_[1].field_value));
}

TEST_F(NovatelTypesTest, ASCII_BOOL_INVALID)
{
   CreateBaseField("B_True", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 4, DATA_TYPE_NAME::BOOL);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "True";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_UINT_VALID)
{
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   const char* testInput = "-1,0,4294967294,4294967295";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[0].field_value), 4294967295U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[1].field_value), 0U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[2].field_value), 4294967294U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[3].field_value), 4294967295U);
}

TEST_F(NovatelTypesTest, ASCII_GPSTIME_MSEC_VALID)
{
   CreateBaseField("Sec1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec3", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   const char* testInput = "-1.000,0.000,604800.000,4294967295.000";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   // If GPSTIME exceeds 4,294,967.295 (seconds) the conversion to milliseconds is wrong
   // But the limit should be 604,800 (seconds) as that's the number of seconds in a GPS reference week
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[0].field_value), 4294966296U);  // 4,294,967,295 + 1 - 1,000 = 4,294,966,296
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[1].field_value), 0U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[2].field_value), 604800000U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[3].field_value), 4294966296U);
}

TEST_F(NovatelTypesTest, ASCII_SCIENTIFIC_NOTATION_FLOAT_VALID)
{
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 4, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   const char* testInput = "-1.0,0.0,1.175494351e-38,3.402823466e+38";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_NEAR(std::get<float>(vIntermediateFormat_[0].field_value),               -1, std::numeric_limits<float>::epsilon());
   ASSERT_NEAR(std::get<float>(vIntermediateFormat_[1].field_value),                0, std::numeric_limits<float>::epsilon());
   ASSERT_NEAR(std::get<float>(vIntermediateFormat_[2].field_value), 1.175494351e-38f, std::numeric_limits<float>::epsilon());
   ASSERT_NEAR(std::get<float>(vIntermediateFormat_[3].field_value), 3.402823466e+38f, std::numeric_limits<float>::epsilon());
}

TEST_F(NovatelTypesTest, ASCII_SCIENTIFIC_NOTATION_FLOAT_INVALID)
{
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 5, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "-1.0";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, ASCII_SCIENTIFIC_NOTATION_DOUBLE_VALID)
{
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 8, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   const char* testInput = "-1.0,0.0,2.2250738585072014e-308,1.7976931348623158e+308";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[0].field_value), -1);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[1].field_value), 0);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[2].field_value), 2.2250738585072014e-308);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[3].field_value), 1.7976931348623158e+308);
}

TEST_F(NovatelTypesTest, ASCII_ULONG_VALID)
{
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 1, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 1, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 1, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lu, 2, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lu, 2, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lu, 2, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::hu, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::hu, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::hu, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::llu, 8, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::llu, 8, DATA_TYPE_NAME::ULONG);
   CreateBaseField("rx_chars", FIELD_TYPE::SIMPLE, CONVERSION_STRING::llu, 8, DATA_TYPE_NAME::ULONG);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(12);

   const char* testInput = "-1,0,255,-1,0,65535,-1,0,4294967295,-1,0,18446744073709551615";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[0].field_value), 255);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[1].field_value), 0);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[2].field_value), 255);
   ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat_[3].field_value), 65535);
   ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat_[4].field_value), 0);
   ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat_[5].field_value), 65535);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[6].field_value), 4294967295U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[7].field_value), 0U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[8].field_value), 4294967295U);
   ASSERT_EQ(std::get<uint64_t>(vIntermediateFormat_[9].field_value), ULLONG_MAX);
   ASSERT_EQ(std::get<uint64_t>(vIntermediateFormat_[10].field_value), 0ULL);
   ASSERT_EQ(std::get<uint64_t>(vIntermediateFormat_[11].field_value), ULLONG_MAX);
}

TEST_F(NovatelTypesTest, ASCII_ENUM_VALID)
{
   CreateEnumField("UNKNOWN", "Unknown or unspecified type", 20);
   CreateEnumField("APPROXIMATE", "Approximate time", 60);
   CreateEnumField("SATTIME", "", 200);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(3);

   const char* testInput = "UNKNOWN,APPROXIMATE,SATTIME";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<int32_t>(vIntermediateFormat_[0].field_value), 20);
   ASSERT_EQ(std::get<int32_t>(vIntermediateFormat_[1].field_value), 60);
   ASSERT_EQ(std::get<int32_t>(vIntermediateFormat_[2].field_value), 200);
}

TEST_F(NovatelTypesTest, ASCII_STRING_VALID)
{
   CreateBaseField("MESSAGE", FIELD_TYPE::STRING, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "#RAWEPHEMA,COM1,100";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<std::string>(vIntermediateFormat_[0].field_value), "RAWEPHEMA,COM1,100");
}

TEST_F(NovatelTypesTest, ASCII_EMPTY_STRING_VALID)
{
   CreateBaseField("MESSAGE", FIELD_TYPE::STRING, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "\"\"";
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<std::string>(vIntermediateFormat_[0].field_value), "");
}

TEST_F(NovatelTypesTest, ASCII_TYPE_INVALID)
{
   CreateBaseField("", FIELD_TYPE::UNKNOWN, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(1);

   const char* testInput = "";

   ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, BINARY_BOOL_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::BOOL);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::BOOL);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(2);

   bool input[] = { 1, 0 };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_TRUE(std::get<bool>(vIntermediateFormat_[0].field_value));
   ASSERT_FALSE(std::get<bool>(vIntermediateFormat_[1].field_value));
}

TEST_F(NovatelTypesTest, BINARY_HEXBYTE_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::HEXBYTE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::HEXBYTE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::HEXBYTE);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(3);

   uint8_t input[] = { 0x00, 0x01, 0xFF };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[0].field_value), 0);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[1].field_value), 1);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[2].field_value), UCHAR_MAX);
}

TEST_F(NovatelTypesTest, BINARY_uint8_t_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(5);

   uint8_t input[] = { 0x23, 0x41, 0x3B, 0x00, 0xFF };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[0].field_value), '#');
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[1].field_value), 'A');
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[2].field_value), ';');
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[3].field_value), 0);
   ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat_[4].field_value), UCHAR_MAX);
}

TEST_F(NovatelTypesTest, BINARY_USHORT_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   uint8_t input[] = { 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0xFF, 0xFF };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat_[0].field_value), 0);
   ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat_[1].field_value), 1);
   ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat_[2].field_value), 16);
   ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat_[3].field_value), USHRT_MAX);
}
TEST_F(NovatelTypesTest, BINARY_SHORT_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   uint8_t input[] = { 0x00, 0x80, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x7F };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[0].field_value), SHRT_MIN);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[1].field_value), -1);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[2].field_value), -0);
   ASSERT_EQ(std::get<int16_t>(vIntermediateFormat_[3].field_value), SHRT_MAX);
}
TEST_F(NovatelTypesTest, BINARY_INT_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   uint8_t input[] = {
      0x00, 0x00, 0x00, 0x80,
      0x00, 0x00, 0xFF, 0xFF,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0x7F
   };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<int32_t>(vIntermediateFormat_[0].field_value), INT_MIN);
   ASSERT_EQ(std::get<int32_t>(vIntermediateFormat_[1].field_value), -65536);
   ASSERT_EQ(std::get<int32_t>(vIntermediateFormat_[2].field_value), 0);
   ASSERT_EQ(std::get<int32_t>(vIntermediateFormat_[3].field_value), INT_MAX);
}

TEST_F(NovatelTypesTest, BINARY_UINT_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   uint8_t input[] = {
      0x00, 0x00, 0x00, 0x80,
      0xFF, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF
   };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[0].field_value), 2147483648U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[1].field_value), 65535U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[2].field_value), 0U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[3].field_value), UINT_MAX);
}

TEST_F(NovatelTypesTest, BINARY_ULONG_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   uint8_t input[] = {
      0x00, 0x00, 0x00, 0x80,
      0xFF, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF
   };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[0].field_value), 2147483648U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[1].field_value), 65535U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[2].field_value), 0U);
   ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat_[3].field_value), UINT_MAX);
}

TEST_F(NovatelTypesTest, BINARY_CHAR_BYTE_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   uint8_t input[] = { 0x80, 0xFF, 0x00, 0x7F };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[0].field_value), CHAR_MIN);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[1].field_value), -1);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[2].field_value), 0);
   ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[3].field_value), CHAR_MAX);
}

TEST_F(NovatelTypesTest, BINARY_FLOAT_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(2);

   uint8_t input[] = { 0x9A, 0x99, 0x99, 0x3F, 0xCD, 0xCC, 0xBC, 0xC0 };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_FLOAT_EQ(std::get<float>(vIntermediateFormat_[0].field_value), 1.2f);
   ASSERT_FLOAT_EQ(std::get<float>(vIntermediateFormat_[1].field_value), -5.9f);
}

TEST_F(NovatelTypesTest, BINARY_DOUBLE_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> vIntermediateFormat_;
   vIntermediateFormat_.reserve(4);

   uint8_t input[] = {
      0x12, 0x71, 0x1C, 0x31, 0xE5, 0x8E, 0x49, 0x40,
      0x99, 0xAF, 0xBC, 0xBE, 0x72, 0x82, 0x5C, 0xC0,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x7F
   };
   unsigned char* testInput = reinterpret_cast<unsigned char*>(input);
   STATUS stDecoderStatus = pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_);
   constexpr double inf = std::numeric_limits<double>::infinity();

   ASSERT_EQ(stDecoderStatus, STATUS::SUCCESS);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[0].field_value), 51.11636937989);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[1].field_value), -114.03825348307);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[2].field_value), 0);
   ASSERT_DOUBLE_EQ(std::get<double>(vIntermediateFormat_[3].field_value), inf);
}

TEST_F(NovatelTypesTest, BINARY_SIMPLE_TYPE_INVALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> vIntermediateFormat_;

   unsigned char* testInput = nullptr;

   ASSERT_THROW(pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, BINARY_TYPE_INVALID)
{
   CreateBaseField("", FIELD_TYPE::UNKNOWN, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> vIntermediateFormat_;

   unsigned char* testInput = nullptr;

   ASSERT_THROW(pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(NovatelTypesTest, SIMPLE_FIELD_WIDTH_VALID)
{
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d,   4, DATA_TYPE_NAME::BOOL);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::XB,  1, DATA_TYPE_NAME::HEXBYTE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UB,  1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::B,   1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::hu,  2, DATA_TYPE_NAME::USHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::hd,  2, DATA_TYPE_NAME::SHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u,   4, DATA_TYPE_NAME::UINT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lu,  4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d,   4, DATA_TYPE_NAME::INT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::ld,  4, DATA_TYPE_NAME::LONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::llu, 8, DATA_TYPE_NAME::ULONGLONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lld, 8, DATA_TYPE_NAME::LONGLONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::f,   4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::lf,  8, DATA_TYPE_NAME::DOUBLE);

   IntermediateMessage vIntermediateFormat;
   vIntermediateFormat.reserve(MsgDefFields_.size());

   const char* testInput = "TRUE,0x63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03";
   unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* pucEncodeBuffer = aucEncodeBuffer;

   ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(MsgDefFields_, &testInput, vIntermediateFormat));
   ASSERT_TRUE(pclMyEncoderTester->TestEncodeBinaryBody(vIntermediateFormat, &pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH));

   vIntermediateFormat.clear();
   pucEncodeBuffer = aucEncodeBuffer;
   pclMyDecoderTester->TestDecodeBinary(MsgDefFields_, &pucEncodeBuffer, vIntermediateFormat);

   size_t sz = 0;

   ASSERT_EQ  (std::get<bool    >(vIntermediateFormat.at(sz++).field_value), true);
   ASSERT_EQ  (std::get<uint8_t >(vIntermediateFormat.at(sz++).field_value), 99);
   ASSERT_EQ  (std::get<uint8_t >(vIntermediateFormat.at(sz++).field_value), 227);
   ASSERT_EQ  (std::get<int8_t  >(vIntermediateFormat.at(sz++).field_value), 56);
   ASSERT_EQ  (std::get<uint16_t>(vIntermediateFormat.at(sz++).field_value), 2734);
   ASSERT_EQ  (std::get<int16_t >(vIntermediateFormat.at(sz++).field_value), -3842);
   ASSERT_EQ  (std::get<uint32_t>(vIntermediateFormat.at(sz++).field_value), 38283U);
   ASSERT_EQ  (std::get<uint32_t>(vIntermediateFormat.at(sz++).field_value), 54244U);
   ASSERT_EQ  (std::get<int32_t >(vIntermediateFormat.at(sz++).field_value), -4359);
   ASSERT_EQ  (std::get<int32_t >(vIntermediateFormat.at(sz++).field_value), 5293);
   ASSERT_EQ  (std::get<uint64_t>(vIntermediateFormat.at(sz++).field_value), 79338432ULL);
   ASSERT_EQ  (std::get<int64_t >(vIntermediateFormat.at(sz++).field_value), -289834LL);
   ASSERT_NEAR(std::get<float   >(vIntermediateFormat.at(sz++).field_value), 2.54, 0.001);
   ASSERT_NEAR(std::get<double  >(vIntermediateFormat.at(sz++).field_value), 5.44061788e+03, 0.000001);
}
