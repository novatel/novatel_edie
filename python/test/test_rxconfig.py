################################################################################
#
# COPYRIGHT Novatel Inc, 2021. All rights reserved.
#
# No part of this software may be reproduced or modified in any form
# or by any means - electronic, mechanical, photocopying, recording,
# or otherwise - without the prior written consent of NovAtel Inc.
#
################################################################################
#                            DESCRIPTION
#
#! \file rxconfigtest.hpp
#! \brief Unit Tests for RxConfigHandler.
################################################################################

#-----------------------------------------------------------------------
# Includes
#-----------------------------------------------------------------------
#include "paths.hpp"

#include "common/common.hpp"
#include "common/jsonreader.hpp"
#include "decoders/novatel/filter.hpp"
#include "decoders/novatel/framer.hpp"
#include "decoders/novatel/header_decoder.hpp"
#include "decoders/novatel/message_decoder.hpp"
#include "decoders/novatel/encoder.hpp"
#include "decoders/novatel/rxconfig/rxconfig_handler.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>

class RxConfigTest : public ::testing::Test {

protected:
   static JsonReader* pclMyJsonDb;
   static RxConfigHandler* pclMyRxConfigHandler;

   # Per-test-suite setup
   static void SetUpTestSuite()
   {
      pclMyJsonDb = new JsonReader();
      pclMyJsonDb->LoadFile(*TEST_DB_PATH);

      pclMyRxConfigHandler = new RxConfigHandler(pclMyJsonDb);
   }

   # Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if(pclMyJsonDb)
      {
         delete pclMyJsonDb;
         pclMyJsonDb = nullptr;
      }

      if(pclMyRxConfigHandler)
      {
         pclMyRxConfigHandler->ShutdownLogger();
         delete pclMyRxConfigHandler;
         pclMyRxConfigHandler = nullptr;
      }
   }

   # Per-test setup
   void SetUp()
   {
      pclMyRxConfigHandler->Flush();
   }

   # Per-test teardown
   void TearDown()
   {
      pclMyRxConfigHandler->Flush();
   }

   bool CompareMessageData(MessageDataStruct* pstTestMessageData_, MessageDataStruct* pstExpectedMessageData_)
   {
      bool bResult = true;
      if (pstTestMessageData_->uiMessageHeaderLength != pstExpectedMessageData_->uiMessageHeaderLength)
      {
         cout << "MessageData.uiMessageHeaderLength (expected " << static_cast<uint32_t>(pstExpectedMessageData_->uiMessageHeaderLength) << ", got " << static_cast<uint32_t>(pstTestMessageData_->uiMessageHeaderLength) << ")\n";
         bResult = false;
      }
      if (pstTestMessageData_->uiMessageBodyLength != pstExpectedMessageData_->uiMessageBodyLength)
      {
         cout << "MessageData.uiMessageBodyLength (expected " << static_cast<uint32_t>(pstExpectedMessageData_->uiMessageBodyLength) << ", got " << static_cast<uint32_t>(pstTestMessageData_->uiMessageBodyLength) << ")\n";
         bResult = false;
      }
      if (pstTestMessageData_->uiMessageLength != pstExpectedMessageData_->uiMessageLength)
      {
         cout << "MessageData.uiMessageLength (expected " << static_cast<uint32_t>(pstExpectedMessageData_->uiMessageLength) << ", got " << static_cast<uint32_t>(pstTestMessageData_->uiMessageLength) << ")\n";
         bResult = false;
      }
      if (0 != memcmp(pstTestMessageData_->pucMessageHeader, pstExpectedMessageData_->pucMessageHeader, pstTestMessageData_->uiMessageHeaderLength))
      {
         cout << "MessageData.pucMessageHeader contents do not match\n";
         bResult = false;
      }
      if (0 != memcmp(pstTestMessageData_->pucMessageBody, pstExpectedMessageData_->pucMessageBody, pstTestMessageData_->uiMessageBodyLength))
      {
         cout << "MessageData.pucMessageBody contents do not match\n";
         bResult = false;
      }
      if (0 != memcmp(pstTestMessageData_->pucMessage, pstExpectedMessageData_->pucMessage, pstTestMessageData_->uiMessageLength))
      {
         cout << "MessageData.pucMessage contents do not match\n";
         bResult = false;
      }
      return bResult;
   }

public:

   void WriteBytesToHandler(unsigned char* pucBytes_, uint32_t uiNumBytes_)
   {
      ASSERT_EQ(pclMyRxConfigHandler->Write(pucBytes_, uiNumBytes_), uiNumBytes_);
   }

   bool TestSameFormatCompare(ENCODEFORMAT eFormat_, MessageDataStruct* pstExpectedRxConfigMessageData_, MessageDataStruct* pstExpectedEmbeddedMessageData_)
   {
      MetaDataStruct stTestRxConfigMetaData;
      MetaDataStruct stTestEmbeddedMetaData;
      MessageDataStruct stTestRxConfigMessageData;
      MessageDataStruct stTestEmbeddedMessageData;

      # CompareMessageData
      const STATUS eStatus = pclMyRxConfigHandler->Convert(stTestRxConfigMessageData, stTestRxConfigMetaData, stTestEmbeddedMessageData, stTestEmbeddedMetaData, eFormat_);
      if(eStatus != STATUS::SUCCESS)
      {
         printf("Convert failed with code %d\n", static_cast<uint32_t>(eStatus));
         return false;
      }

      if(!CompareMessageData(&stTestRxConfigMessageData, pstExpectedRxConfigMessageData_) ||
         !CompareMessageData(&stTestEmbeddedMessageData, pstExpectedEmbeddedMessageData_))
      {
         return false;
      }

      return true;
   }
};
JsonReader* RxConfigTest::pclMyJsonDb = nullptr;
RxConfigHandler* RxConfigTest::pclMyRxConfigHandler = nullptr;

# -------------------------------------------------------------------------------------------------------
# Logger Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
TEST_F(RxConfigTest, LOGGER)
{
   spdlog::level::level_enum eLevel = spdlog::level::off;

   ASSERT_NE(spdlog::get("rxconfig_handler"), nullptr);
   std::shared_ptr<spdlog::logger> rxconfig_handler = pclMyRxConfigHandler->GetLogger();
   pclMyRxConfigHandler->SetLoggerLevel(eLevel);
   ASSERT_EQ(rxconfig_handler->level(), eLevel);
}

# -------------------------------------------------------------------------------------------------------
# Round-trip unit tests.
# -------------------------------------------------------------------------------------------------------
TEST_F(RxConfigTest, RXCONFIG_ROUNDTRIP_ASCII)
{
   unsigned char aucLog[] = "#RXCONFIGA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;#INTERFACEMODEA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;COM1,NOVATEL,NOVATEL,ON*ca0f5c51*71be1427\r\n";
   MessageDataStruct stExpectedRxConfigMessageData;
   stExpectedRxConfigMessageData.pucMessage = &aucLog[0];
   stExpectedRxConfigMessageData.uiMessageLength = 192;
   stExpectedRxConfigMessageData.pucMessageHeader = &aucLog[0];
   stExpectedRxConfigMessageData.uiMessageHeaderLength = 72;
   stExpectedRxConfigMessageData.pucMessageBody = &aucLog[72];
   stExpectedRxConfigMessageData.uiMessageBodyLength = 120;

   MessageDataStruct stExpectedEmbeddedMessageData;
   stExpectedEmbeddedMessageData.pucMessage = &aucLog[72];
   stExpectedEmbeddedMessageData.uiMessageLength = 109;
   stExpectedEmbeddedMessageData.pucMessageHeader = &aucLog[72];
   stExpectedEmbeddedMessageData.uiMessageHeaderLength = 77;
   stExpectedEmbeddedMessageData.pucMessageBody = &aucLog[149];
   stExpectedEmbeddedMessageData.uiMessageBodyLength = 32;

   WriteBytesToHandler(aucLog, sizeof(aucLog));
   ASSERT_TRUE(TestSameFormatCompare(ENCODEFORMAT::ASCII, &stExpectedRxConfigMessageData, &stExpectedEmbeddedMessageData));
}

TEST_F(RxConfigTest, RXCONFIG_ROUNDTRIP_ABBREV)
{
   # NOTE: This RXCONFIG message is NOT what an OEM7 receiver would produce.  The space after the embedded header is added intentionally by RxConfigHandler to make it decodeable.
   unsigned char aucLog[] = "<RXCONFIG COM2 78 33.0 UNKNOWN 0 12.468 024c0000 f702 32768\r\n<     PDPFILTER COM2 78 33.0 UNKNOWN 0 12.468 024c0000 dab4 32768 \r\n<     DISABLE\r\nMore bytes...";

   MessageDataStruct stExpectedRxConfigMessageData;
   stExpectedRxConfigMessageData.pucMessage = &aucLog[0];
   stExpectedRxConfigMessageData.uiMessageLength = 144;
   stExpectedRxConfigMessageData.pucMessageHeader = &aucLog[0];
   stExpectedRxConfigMessageData.uiMessageHeaderLength = 61;
   stExpectedRxConfigMessageData.pucMessageBody = &aucLog[61];
   stExpectedRxConfigMessageData.uiMessageBodyLength = 83;

   MessageDataStruct stExpectedEmbeddedMessageData;
   stExpectedEmbeddedMessageData.pucMessage = &aucLog[61];
   stExpectedEmbeddedMessageData.uiMessageLength = 83;
   stExpectedEmbeddedMessageData.pucMessageHeader = &aucLog[61];
   stExpectedEmbeddedMessageData.uiMessageHeaderLength = 68;
   stExpectedEmbeddedMessageData.pucMessageBody = &aucLog[129];
   stExpectedEmbeddedMessageData.uiMessageBodyLength = 15;

   WriteBytesToHandler(aucLog, sizeof(aucLog));
   ASSERT_TRUE(TestSameFormatCompare(ENCODEFORMAT::ABBREV_ASCII, &stExpectedRxConfigMessageData, &stExpectedEmbeddedMessageData));
}

TEST_F(RxConfigTest, RXCONFIG_ROUNDTRIP_BINARY)
{
   # RXCONFIG
   unsigned char aucLog[] = { 0xAA, 0x44, 0x12, 0x1C, 0x80, 0x00, 0x00, 0x20, 0x30, 0x00, 0x00, 0x00, 0x65, 0xB4, 0x7C, 0x08, 0x3C, 0x78, 0x48, 0x09, 0x00, 0x00, 0x01, 0x02, 0x02, 0xF7, 0x78, 0x3F, 0xAA, 0x44, 0x12, 0x1C, 0x03, 0x00, 0x00, 0x20, 0x10, 0x00, 0x00, 0x00, 0x65, 0xB4, 0x7C, 0x08, 0x3C, 0x78, 0x48, 0x09, 0x00, 0x00, 0x01, 0x02, 0x02, 0xF7, 0x78, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x67, 0x74, 0xB2, 0xEC, 0x0E, 0xD1, 0xFB, 0x06 };
   MessageDataStruct stExpectedRxConfigMessageData;
   stExpectedRxConfigMessageData.pucMessage = &aucLog[0];
   stExpectedRxConfigMessageData.uiMessageLength = 80;
   stExpectedRxConfigMessageData.pucMessageHeader = &aucLog[0];
   stExpectedRxConfigMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedRxConfigMessageData.pucMessageBody = &aucLog[OEM4_BINARY_HEADER_LENGTH];
   stExpectedRxConfigMessageData.uiMessageBodyLength = 52;

   MessageDataStruct stExpectedEmbeddedMessageData;
   stExpectedEmbeddedMessageData.pucMessage = &aucLog[OEM4_BINARY_HEADER_LENGTH];
   stExpectedEmbeddedMessageData.uiMessageLength = 48;
   stExpectedEmbeddedMessageData.pucMessageHeader = &aucLog[OEM4_BINARY_HEADER_LENGTH];
   stExpectedEmbeddedMessageData.uiMessageHeaderLength = OEM4_BINARY_HEADER_LENGTH;
   stExpectedEmbeddedMessageData.pucMessageBody = &aucLog[OEM4_BINARY_HEADER_LENGTH*2];
   stExpectedEmbeddedMessageData.uiMessageBodyLength = 20;

   WriteBytesToHandler(aucLog, sizeof(aucLog));
   ASSERT_TRUE(TestSameFormatCompare(ENCODEFORMAT::BINARY, &stExpectedRxConfigMessageData, &stExpectedEmbeddedMessageData));
}


# -------------------------------------------------------------------------------------------------------
# Conversion to JSON unit tests.
# -------------------------------------------------------------------------------------------------------
TEST_F(RxConfigTest, RXCONFIG_CONVERT_ASCII_TO_JSON)
{
   unsigned char aucLog[] = "#RXCONFIGA,COM2,235,77.0,UNKNOWN,0,0.727,02000020,f702,17002;#ADJUST1PPSA,COM2,235,77.0,UNKNOWN,0,0.727,02000020,f702,17002;OFF*4c2dbb6d*1600a42a\r\n";
   unsigned char aucJsonLog[] = R"({"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 235,"percent_idle_time": 77.0,"time_status": "UNKNOWN","week": 0,"seconds": 0.727,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "ADJUST1PPS","id": 429,"port": "COM2","sequence_num": 235,"percent_idle_time": 77.0,"time_status": "UNKNOWN","week": 0,"seconds": 0.727,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"mode": "OFF"}}})";
   MessageDataStruct stExpectedRxConfigMessageData;
   stExpectedRxConfigMessageData.pucMessage = &aucJsonLog[0];
   stExpectedRxConfigMessageData.uiMessageLength = 531;
   stExpectedRxConfigMessageData.pucMessageHeader = &aucJsonLog[10];
   stExpectedRxConfigMessageData.uiMessageHeaderLength = 229;
   stExpectedRxConfigMessageData.pucMessageBody = &aucJsonLog[247];
   stExpectedRxConfigMessageData.uiMessageBodyLength = 283;

   MessageDataStruct stExpectedEmbeddedMessageData;
   stExpectedEmbeddedMessageData.pucMessage = &aucJsonLog[247];
   stExpectedEmbeddedMessageData.uiMessageLength = 283;
   stExpectedEmbeddedMessageData.pucMessageHeader = &aucJsonLog[266];
   stExpectedEmbeddedMessageData.uiMessageHeaderLength = 231;
   stExpectedEmbeddedMessageData.pucMessageBody = &aucJsonLog[514];
   stExpectedEmbeddedMessageData.uiMessageBodyLength = 15;

   WriteBytesToHandler(aucLog, sizeof(aucLog));
   ASSERT_TRUE(TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedRxConfigMessageData, &stExpectedEmbeddedMessageData));
}

TEST_F(RxConfigTest, RXCONFIG_CONVERT_ABBREV_TO_JSON)
{
   unsigned char aucLog[] = "<RXCONFIG COM2 187 78.5 UNKNOWN 0 0.839 02000020 f702 17002\r\n<     SBASECUTOFF COM2 187 78.5 UNKNOWN 0 0.839 02000020 f702 17002 \r\n<     -5.0\r\n[PISSCOM1]";
   unsigned char aucJsonLog[] = R"({"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 187,"percent_idle_time": 78.5,"time_status": "UNKNOWN","week": 0,"seconds": 0.839,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "SBASECUTOFF","id": 1000,"port": "COM2","sequence_num": 187,"percent_idle_time": 78.5,"time_status": "UNKNOWN","week": 0,"seconds": 0.839,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"elevation_cutoff_angle": -5.0}}})";
   MessageDataStruct stExpectedRxConfigMessageData;
   stExpectedRxConfigMessageData.pucMessage = &aucJsonLog[0];
   stExpectedRxConfigMessageData.uiMessageLength = 550;
   stExpectedRxConfigMessageData.pucMessageHeader = &aucJsonLog[10];
   stExpectedRxConfigMessageData.uiMessageHeaderLength = 229;
   stExpectedRxConfigMessageData.pucMessageBody = &aucJsonLog[247];
   stExpectedRxConfigMessageData.uiMessageBodyLength = 302;

   MessageDataStruct stExpectedEmbeddedMessageData;
   stExpectedEmbeddedMessageData.pucMessage = &aucJsonLog[247];
   stExpectedEmbeddedMessageData.uiMessageLength = 302;
   stExpectedEmbeddedMessageData.pucMessageHeader = &aucJsonLog[266];
   stExpectedEmbeddedMessageData.uiMessageHeaderLength = 233;
   stExpectedEmbeddedMessageData.pucMessageBody = &aucJsonLog[516];
   stExpectedEmbeddedMessageData.uiMessageBodyLength = 32;

   WriteBytesToHandler(aucLog, sizeof(aucLog));
   ASSERT_TRUE(TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedRxConfigMessageData, &stExpectedEmbeddedMessageData));
}

TEST_F(RxConfigTest, RXCONFIG_CONVERT_BINARY_TO_JSON)
{
   unsigned char aucLog[] = {0xAA, 0x44, 0x12, 0x1C, 0x80, 0x00, 0x00, 0x40, 0x3C, 0x00, 0x00, 0x00, 0x9C, 0xB4, 0xBB, 0x08, 0x47, 0x74, 0x6A, 0x18, 0x20, 0x00, 0x00, 0x02, 0x02, 0xF7, 0x6A, 0x42, 0xAA, 0x44, 0x12, 0x1C, 0xDE, 0x04, 0x00, 0x40, 0x1C, 0x00, 0x00, 0x00, 0x9C, 0xB4, 0xBB, 0x08, 0x47, 0x74, 0x6A, 0x18, 0x20, 0x00, 0x00, 0x02, 0x02, 0xF7, 0x6A, 0x42, 0x02, 0x00, 0x00, 0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF4, 0xD7, 0x46, 0x65, 0x5D, 0x3D, 0xED, 0xF6};
   unsigned char aucJsonLog[] = R"({"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 0,"percent_idle_time": 78.0,"time_status": "FINESTEERING","week": 2235,"seconds": 409629.767,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "SERIALCONFIG","id": 1246,"port": "COM2","sequence_num": 0,"percent_idle_time": 78.0,"time_status": "FINESTEERING","week": 2235,"seconds": 409629.767,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"port": "COM2","baud_rate": 460800,"parity": "N","data_bits": 8,"stop_bits": 1,"hand_shaking": "N","breaks": "ON"}}})";
   MessageDataStruct stExpectedRxConfigMessageData;
   stExpectedRxConfigMessageData.pucMessage = &aucJsonLog[0];
   stExpectedRxConfigMessageData.uiMessageLength = 656;
   stExpectedRxConfigMessageData.pucMessageHeader = &aucJsonLog[10];
   stExpectedRxConfigMessageData.uiMessageHeaderLength = 240;
   stExpectedRxConfigMessageData.pucMessageBody = &aucJsonLog[258];
   stExpectedRxConfigMessageData.uiMessageBodyLength = 397;

   MessageDataStruct stExpectedEmbeddedMessageData;
   stExpectedEmbeddedMessageData.pucMessage = &aucJsonLog[258];
   stExpectedEmbeddedMessageData.uiMessageLength = 397;
   stExpectedEmbeddedMessageData.pucMessageHeader = &aucJsonLog[277];
   stExpectedEmbeddedMessageData.uiMessageHeaderLength = 245;
   stExpectedEmbeddedMessageData.pucMessageBody = &aucJsonLog[539];
   stExpectedEmbeddedMessageData.uiMessageBodyLength = 115;

   WriteBytesToHandler(aucLog, sizeof(aucLog));
   ASSERT_TRUE(TestSameFormatCompare(ENCODEFORMAT::JSON, &stExpectedRxConfigMessageData, &stExpectedEmbeddedMessageData));
}
