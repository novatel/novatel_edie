################################################################################
#
# COPYRIGHT NovAtel Inc, 2022. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
################################################################################
#                            DESCRIPTION
#
#! \file proprietarytest.hpp
#! \brief Unit tests for proprietary NovAtel logs.
################################################################################

#-----------------------------------------------------------------------
# Includes
#-----------------------------------------------------------------------
#include "paths.hpp"

#include "hw_interface/stream_interface/inputfilestream.hpp"
#include "decoders/novatel/framer.hpp"
#include <gtest/gtest.h>

class ProprietaryFramerTest : public ::testing::Test
{

protected:
   static Framer* pclMyFramer;
   static InputFileStream* pclMyIFS;
   static unsigned char* pucMyTestFrameBuffer;

   # Per-test-suite setup
   static void SetUpTestSuite()
   {
      pclMyFramer = new Framer();
      pclMyFramer->SetReportUnknownBytes(true);
      pclMyFramer->SetPayloadOnly(false);
      pucMyTestFrameBuffer = new unsigned char[131071]; # 128kB
   }

   # Per-test-suite teardown
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

   # Per-test setup
   void SetUp()
   {
      FlushFramer();
   }

   # Per-test teardown
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
         cout << "MetaData.eFormat (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eFormat) << ", got " << static_cast<uint32_t>(pstTestMD_->eFormat) << ")\n";
         bResult = false;
      }
      if (pstTestMD_->eMeasurementSource != pstExpectedMetaData_->eMeasurementSource)
      {
         cout << "MetaData.eMeasurementSource (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eMeasurementSource) << ", got " << static_cast<uint32_t>(pstTestMD_->eMeasurementSource) << ")\n";
         bResult = false;
      }
      if (pstTestMD_->eTimeStatus != pstExpectedMetaData_->eTimeStatus)
      {
         cout << "MetaData.eTimeStatus (expected " << static_cast<uint32_t>(pstExpectedMetaData_->eTimeStatus) << ", got " << static_cast<uint32_t>(pstTestMD_->eTimeStatus) << ")\n";
         bResult = false;
      }
      if (pstTestMD_->bResponse != pstExpectedMetaData_->bResponse)
      {
         cout << "MetaData.bResponse (expected " << static_cast<uint32_t>(pstExpectedMetaData_->bResponse) << ", got " << static_cast<uint32_t>(pstTestMD_->bResponse) << ")\n";
         bResult = false;
      }
      if (pstTestMD_->usWeek != pstExpectedMetaData_->usWeek)
      {
         cout << "MetaData.usWeek (expected " << pstExpectedMetaData_->usWeek << ", got " << pstTestMD_->usWeek << ")\n";
         bResult = false;
      }
      if (pstTestMD_->dMilliseconds != pstExpectedMetaData_->dMilliseconds)
      {
         cout << "MetaData.dMilliseconds (expected " << pstExpectedMetaData_->dMilliseconds << ", got " << pstTestMD_->dMilliseconds << ")\n";
         bResult = false;
      }
      if (pstTestMD_->uiBinaryMsgLength != pstExpectedMetaData_->uiBinaryMsgLength)
      {
         cout << "MetaData.uiBinaryMsgLength (expected " << pstExpectedMetaData_->uiBinaryMsgLength << ", got " << pstTestMD_->uiBinaryMsgLength << ")\n";
         bResult = false;
      }
      if (pstTestMD_->uiLength != pstExpectedMetaData_->uiLength)
      {
         cout << "MetaData.uiLength (expected " << pstExpectedMetaData_->uiLength << ", got " << pstTestMD_->uiLength << ")\n";
         bResult = false;
      }
      if (pstTestMD_->uiHeaderLength != pstExpectedMetaData_->uiHeaderLength)
      {
         cout << "MetaData.uiHeaderLength (expected " << pstExpectedMetaData_->uiHeaderLength << ", got " << pstTestMD_->uiHeaderLength << ")\n";
         bResult = false;
      }
      if (pstTestMD_->usMessageID != pstExpectedMetaData_->usMessageID)
      {
         cout << "MetaData.usMessageID (expected " << pstExpectedMetaData_->usMessageID << ", got " << pstTestMD_->usMessageID << ")\n";
         bResult = false;
      }
      if (pstTestMD_->uiMessageCRC != pstExpectedMetaData_->uiMessageCRC)
      {
         cout << "MetaData.uiMessageCRC (expected " << pstExpectedMetaData_->uiMessageCRC << ", got " << pstTestMD_->uiMessageCRC << ")\n";
         bResult = false;
      }
      if (pstTestMD_->MessageName() != pstExpectedMetaData_->MessageName())
      {
         cout << "MetaData.acMessageName (expected " << pstExpectedMetaData_->MessageName() << ", got " << pstTestMD_->MessageName() << ")\n";
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
Framer* ProprietaryFramerTest::pclMyFramer = nullptr;
InputFileStream* ProprietaryFramerTest::pclMyIFS = nullptr;
unsigned char* ProprietaryFramerTest::pucMyTestFrameBuffer = nullptr;

# -------------------------------------------------------------------------------------------------------
# Proprietary Binary Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_COMPLETE)
{
   # "GARBAGE_DATA<binary bestpos log>"
   uint8_t aucData[] = { 0x47, 0x41, 0x52, 0x42, 0x41, 0x47, 0x45, 0x5F, 0x44, 0x41, 0x54, 0x41, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 12;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   stExpectedMetaData.uiLength = 76;
   stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_INCOMPLETE)
{
   # "<incomplete binary bestpos log>"
   uint8_t aucData[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 59;
   stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
   ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_SYNC_ERROR)
{
   WriteFileStreamToFramer("proprietary_binary_sync_error.BIN");

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = MAX_BINARY_MESSAGE_LENGTH;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_BAD_CRC)
{
   # "<encrypted binary bestpos log>"
   uint8_t aucData[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xFF };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 30; # Unknown bytes up to 0x24 ('$') should be returned (NMEA sync was found mid-log)
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
   ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_RUN_ON_CRC)
{
   # "<encrypted binary bestpos log>FF"
   uint8_t aucData[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC, 0xFF };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 76;
   stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_INADEQUATE_BUFFER)
{
   # "<encrypted binary bestpos log>"
   uint8_t aucData[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC };
   WriteBytesToFramer(aucData, sizeof(aucData));

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.uiLength = 76;
   stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
   ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 38, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));

   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, 76, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_BYTE_BY_BYTE)
{
   # "<binary bestpos log>"
   uint8_t aucData[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC };
   uint32_t uiLogSize = sizeof(aucData);
   uint32_t uiRemainingBytes = uiLogSize;

   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

   while (true)
   {
      WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
      uiRemainingBytes--;
      stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

      if (stExpectedMetaData.uiLength == OEM4_BINARY_SYNC_LENGTH - 1)
      {
         stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
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

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_SEGMENTED)
{
   # "<binary bestpos log>"
   uint8_t aucData[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC };
   uint32_t uiLogSize = sizeof(aucData);
   uint32_t uiBytesWritten = 0;
   MetaDataStruct stExpectedMetaData, stTestMetaData;
   stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;

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

   WriteBytesToFramer(&aucData[uiBytesWritten], 44);
   uiBytesWritten += 44;
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

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_TRICK)
{
   # "<binary syncs><binary sync + half header><binary sync byte 1><binary bestpos log>"
   uint8_t aucData[] = { 0xAA, 0x45, 0x12, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0xAA, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC };
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

   stExpectedMetaData.uiLength = 76;
   stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
   ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer, MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
   ASSERT_TRUE(CompareMetaData(&stTestMetaData, &stExpectedMetaData));
}
