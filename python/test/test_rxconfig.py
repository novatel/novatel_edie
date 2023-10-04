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

class RxConfigTest : public .testing.Test {

protected:
   static JsonReader* my_json_db
   static RxConfigHandler* my_rx_config_handler
   # Per-test-suite setup
   static void SetUpTestSuite():
      my_json_db = ne.JsonReader()
      my_json_db.load_file(json_db_path)
      my_rx_config_handler = ne.RxConfigHandler(my_json_db)

   # Per-test-suite teardown
   static void TearDownTestSuite():
      if(my_json_db):
         my_json_db = nullptr

      if(my_rx_config_handler):
         my_rx_config_handler.shutdown_logger()
         my_rx_config_handler = nullptr

   # Per-test setup
   void SetUp():
      my_rx_config_handler.flush()

   # Per-test teardown
   void TearDown():
      my_rx_config_handler.flush()

   bool CompareMessageData(MessageDataStruct* test_message_data_, MessageDataStruct* expected_message_data_):
      bool result = True
      if test_message_data_.ui_message_header_length != expected_message_data_.ui_message_header_length:
         print(f"MessageData.message_header_length (expected {int(expected_message_data_.ui_message_header_length)}, got {int(test_message_data_.ui_message_header_length)})")
         result = False
      if test_message_data_.ui_message_body_length != expected_message_data_.ui_message_body_length:
         print(f"MessageData.message_body_length (expected {int(expected_message_data_.ui_message_body_length)}, got {int(test_message_data_.ui_message_body_length)})")
         result = False
      if test_message_data_.ui_message_length != expected_message_data_.ui_message_length:
         print(f"MessageData.message_length (expected {int(expected_message_data_.ui_message_length)}, got {int(test_message_data_.ui_message_length)})")
         result = False
      if test_message_data_.puc_message_header != expected_message_data_.puc_message_header:
         print("MessageData.message_header contents do not match")
         result = False
      if test_message_data_.puc_message_body != expected_message_data_.puc_message_body:
         print("MessageData.message_body contents do not match")
         result = False
      if test_message_data_.puc_message != expected_message_data_.puc_message:
         print("MessageData.message contents do not match")
         result = False
      return result

public:

   void WriteBytesToHandler(unsigned char* bytes_, uint32_t num_bytes_):
      ASSERT_EQ(my_rx_config_handler.write(bytes_, num_bytes_), num_bytes_)

   bool TestSameFormatCompare(ENCODEFORMAT format_, MessageDataStruct* expected_rx_config_message_data_, MessageDataStruct* expected_embedded_message_data_):
      MetaData test_rx_config_meta_data
      MetaData test_embedded_meta_data
      MessageDataStruct test_rx_config_message_data
      MessageDataStruct test_embedded_message_data
      # CompareMessageData
      const STATUS status = my_rx_config_handler.convert(test_rx_config_message_data, test_rx_config_meta_data, test_embedded_message_data, test_embedded_meta_data, format_)
      if(status != STATUS.SUCCESS):
         print("Convert failed with code %d\n" % (int(status)))
         return False

      if(not CompareMessageData(&test_rx_config_message_data, expected_rx_config_message_data_) ||
         not CompareMessageData(&test_embedded_message_data, expected_embedded_message_data_)):
         return False

      return True
JsonReader* RxConfigTest.my_json_db = nullptr
RxConfigHandler* RxConfigTest.my_rx_config_handler = nullptr
# -------------------------------------------------------------------------------------------------------
# Logger Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_LOGGER():
   level = ne.LogLevel.OFF
   assert spdlog.get("rxconfig_handler") != nullptr
   rxconfig_handler = my_rx_config_handler.get_logger()
   my_rx_config_handler.logger.set_level(level)
   assert rxconfig_handler.level() == level

# -------------------------------------------------------------------------------------------------------
# Round-trip unit tests.
# -------------------------------------------------------------------------------------------------------
def test_RXCONFIG_ROUNDTRIP_ASCII():
   unsigned char log[] = "#RXCONFIGA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;#INTERFACEMODEA,COM1,0,54.0,FINESTEERING,2172,155744.316,02010000,f702,16248;COM1,NOVATEL,NOVATEL,ON*ca0f5c51*71be1427\r\n"
   MessageDataStruct expected_rx_config_message_data
   expected_rx_config_message_data.message = &log[0]
   expected_rx_config_message_data.message_length = 192
   expected_rx_config_message_data.message_header = &log[0]
   expected_rx_config_message_data.message_header_length = 72
   expected_rx_config_message_data.message_body = &log[72]
   expected_rx_config_message_data.message_body_length = 120
   MessageDataStruct expected_embedded_message_data
   expected_embedded_message_data.message = &log[72]
   expected_embedded_message_data.message_length = 109
   expected_embedded_message_data.message_header = &log[72]
   expected_embedded_message_data.message_header_length = 77
   expected_embedded_message_data.message_body = &log[149]
   expected_embedded_message_data.message_body_length = 32
   WriteBytesToHandler(log, len(log))
   assert TestSameFormatCompare(ENCODEFORMAT.ASCII, &expected_rx_config_message_data, &expected_embedded_message_data)

def test_RXCONFIG_ROUNDTRIP_ABBREV():
   # NOTE: This RXCONFIG message is NOT what an OEM7 receiver would produce.  The space after the embedded header is added intentionally by RxConfigHandler to make it decodeable.
   unsigned char log[] = "<RXCONFIG COM2 78 33.0 UNKNOWN 0 12.468 024c0000 f702 32768\r\n<     PDPFILTER COM2 78 33.0 UNKNOWN 0 12.468 024c0000 dab4 32768 \r\n<     DISABLE\r\more bytes..."
   MessageDataStruct expected_rx_config_message_data
   expected_rx_config_message_data.message = &log[0]
   expected_rx_config_message_data.message_length = 144
   expected_rx_config_message_data.message_header = &log[0]
   expected_rx_config_message_data.message_header_length = 61
   expected_rx_config_message_data.message_body = &log[61]
   expected_rx_config_message_data.message_body_length = 83
   MessageDataStruct expected_embedded_message_data
   expected_embedded_message_data.message = &log[61]
   expected_embedded_message_data.message_length = 83
   expected_embedded_message_data.message_header = &log[61]
   expected_embedded_message_data.message_header_length = 68
   expected_embedded_message_data.message_body = &log[129]
   expected_embedded_message_data.message_body_length = 15
   WriteBytesToHandler(log, len(log))
   assert TestSameFormatCompare(ENCODEFORMAT.ABBREV_ASCII, &expected_rx_config_message_data, &expected_embedded_message_data)

def test_RXCONFIG_ROUNDTRIP_BINARY():
   # RXCONFIG
   unsigned char log[]  = [ 0xAA, 0x44, 0x12, 0x1C, 0x80, 0x00, 0x00, 0x20, 0x30, 0x00, 0x00, 0x00, 0x65, 0xB4, 0x7C, 0x08, 0x3C, 0x78, 0x48, 0x09, 0x00, 0x00, 0x01, 0x02, 0x02, 0xF7, 0x78, 0x3F, 0xAA, 0x44, 0x12, 0x1C, 0x03, 0x00, 0x00, 0x20, 0x10, 0x00, 0x00, 0x00, 0x65, 0xB4, 0x7C, 0x08, 0x3C, 0x78, 0x48, 0x09, 0x00, 0x00, 0x01, 0x02, 0x02, 0xF7, 0x78, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x67, 0x74, 0xB2, 0xEC, 0x0E, 0xD1, 0xFB, 0x06 ]
   MessageDataStruct expected_rx_config_message_data
   expected_rx_config_message_data.message = &log[0]
   expected_rx_config_message_data.message_length = 80
   expected_rx_config_message_data.message_header = &log[0]
   expected_rx_config_message_data.message_header_length = OEM4_BINARY_HEADER_LENGTH
   expected_rx_config_message_data.message_body = &log[OEM4_BINARY_HEADER_LENGTH]
   expected_rx_config_message_data.message_body_length = 52
   MessageDataStruct expected_embedded_message_data
   expected_embedded_message_data.message = &log[OEM4_BINARY_HEADER_LENGTH]
   expected_embedded_message_data.message_length = 48
   expected_embedded_message_data.message_header = &log[OEM4_BINARY_HEADER_LENGTH]
   expected_embedded_message_data.message_header_length = OEM4_BINARY_HEADER_LENGTH
   expected_embedded_message_data.message_body = &log[OEM4_BINARY_HEADER_LENGTH*2]
   expected_embedded_message_data.message_body_length = 20
   WriteBytesToHandler(log, len(log))
   assert TestSameFormatCompare(ENCODEFORMAT.BINARY, &expected_rx_config_message_data, &expected_embedded_message_data)


# -------------------------------------------------------------------------------------------------------
# Conversion to JSON unit tests.
# -------------------------------------------------------------------------------------------------------
def test_RXCONFIG_CONVERT_ASCII_TO_JSON():
   unsigned char log[] = "#RXCONFIGA,COM2,235,77.0,UNKNOWN,0,0.727,02000020,f702,17002;#ADJUST1PPSA,COM2,235,77.0,UNKNOWN,0,0.727,02000020,f702,17002;OFF*4c2dbb6d*1600a42a\r\n"
   unsigned char json_log[] = """{"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 235,"percent_idle_time": 77.0,"time_status": "UNKNOWN","week": 0,"seconds": 0.727,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "ADJUST1PPS","id": 429,"port": "COM2","sequence_num": 235,"percent_idle_time": 77.0,"time_status": "UNKNOWN","week": 0,"seconds": 0.727,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"mode": "OFF"}}}"""
   MessageDataStruct expected_rx_config_message_data
   expected_rx_config_message_data.message = &json_log[0]
   expected_rx_config_message_data.message_length = 531
   expected_rx_config_message_data.message_header = &json_log[10]
   expected_rx_config_message_data.message_header_length = 229
   expected_rx_config_message_data.message_body = &json_log[247]
   expected_rx_config_message_data.message_body_length = 283
   MessageDataStruct expected_embedded_message_data
   expected_embedded_message_data.message = &json_log[247]
   expected_embedded_message_data.message_length = 283
   expected_embedded_message_data.message_header = &json_log[266]
   expected_embedded_message_data.message_header_length = 231
   expected_embedded_message_data.message_body = &json_log[514]
   expected_embedded_message_data.message_body_length = 15
   WriteBytesToHandler(log, len(log))
   assert TestSameFormatCompare(ENCODEFORMAT.JSON, &expected_rx_config_message_data, &expected_embedded_message_data)

def test_RXCONFIG_CONVERT_ABBREV_TO_JSON():
   unsigned char log[] = "<RXCONFIG COM2 187 78.5 UNKNOWN 0 0.839 02000020 f702 17002\r\n<     SBASECUTOFF COM2 187 78.5 UNKNOWN 0 0.839 02000020 f702 17002 \r\n<     -5.0\r\n[PISSCOM1]"
   unsigned char json_log[] = """{"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 187,"percent_idle_time": 78.5,"time_status": "UNKNOWN","week": 0,"seconds": 0.839,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "SBASECUTOFF","id": 1000,"port": "COM2","sequence_num": 187,"percent_idle_time": 78.5,"time_status": "UNKNOWN","week": 0,"seconds": 0.839,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"elevation_cutoff_angle": -5.0}}}"""
   MessageDataStruct expected_rx_config_message_data
   expected_rx_config_message_data.message = &json_log[0]
   expected_rx_config_message_data.message_length = 550
   expected_rx_config_message_data.message_header = &json_log[10]
   expected_rx_config_message_data.message_header_length = 229
   expected_rx_config_message_data.message_body = &json_log[247]
   expected_rx_config_message_data.message_body_length = 302
   MessageDataStruct expected_embedded_message_data
   expected_embedded_message_data.message = &json_log[247]
   expected_embedded_message_data.message_length = 302
   expected_embedded_message_data.message_header = &json_log[266]
   expected_embedded_message_data.message_header_length = 233
   expected_embedded_message_data.message_body = &json_log[516]
   expected_embedded_message_data.message_body_length = 32
   WriteBytesToHandler(log, len(log))
   assert TestSameFormatCompare(ENCODEFORMAT.JSON, &expected_rx_config_message_data, &expected_embedded_message_data)

def test_RXCONFIG_CONVERT_BINARY_TO_JSON():
   unsigned char log[]  = [0xAA, 0x44, 0x12, 0x1C, 0x80, 0x00, 0x00, 0x40, 0x3C, 0x00, 0x00, 0x00, 0x9C, 0xB4, 0xBB, 0x08, 0x47, 0x74, 0x6A, 0x18, 0x20, 0x00, 0x00, 0x02, 0x02, 0xF7, 0x6A, 0x42, 0xAA, 0x44, 0x12, 0x1C, 0xDE, 0x04, 0x00, 0x40, 0x1C, 0x00, 0x00, 0x00, 0x9C, 0xB4, 0xBB, 0x08, 0x47, 0x74, 0x6A, 0x18, 0x20, 0x00, 0x00, 0x02, 0x02, 0xF7, 0x6A, 0x42, 0x02, 0x00, 0x00, 0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xF4, 0xD7, 0x46, 0x65, 0x5D, 0x3D, 0xED, 0xF6]
   unsigned char json_log[] = """{"header":{"message": "RXCONFIG","id": 128,"port": "COM2","sequence_num": 0,"percent_idle_time": 78.0,"time_status": "FINESTEERING","week": 2235,"seconds": 409629.767,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"body":{"embedded_header":{"message": "SERIALCONFIG","id": 1246,"port": "COM2","sequence_num": 0,"percent_idle_time": 78.0,"time_status": "FINESTEERING","week": 2235,"seconds": 409629.767,"receiver_status": 33554464,"HEADER_reserved1": 63234,"receiver_sw_version": 17002},"embedded_body":{"port": "COM2","baud_rate": 460800,"parity": "N","data_bits": 8,"stop_bits": 1,"hand_shaking": "N","breaks": "ON"}}}"""
   MessageDataStruct expected_rx_config_message_data
   expected_rx_config_message_data.message = &json_log[0]
   expected_rx_config_message_data.message_length = 656
   expected_rx_config_message_data.message_header = &json_log[10]
   expected_rx_config_message_data.message_header_length = 240
   expected_rx_config_message_data.message_body = &json_log[258]
   expected_rx_config_message_data.message_body_length = 397
   MessageDataStruct expected_embedded_message_data
   expected_embedded_message_data.message = &json_log[258]
   expected_embedded_message_data.message_length = 397
   expected_embedded_message_data.message_header = &json_log[277]
   expected_embedded_message_data.message_header_length = 245
   expected_embedded_message_data.message_body = &json_log[539]
   expected_embedded_message_data.message_body_length = 115
   WriteBytesToHandler(log, len(log))
   assert TestSameFormatCompare(ENCODEFORMAT.JSON, &expected_rx_config_message_data, &expected_embedded_message_data)
