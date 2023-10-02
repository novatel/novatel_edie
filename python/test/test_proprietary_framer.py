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

class ProprietaryFramerTest : public .testing.Test
{

protected:
   static Framer* my_framer
   static InputFileStream* myIFS
   static unsigned char* my_test_frame_buffer
   # Per-test-suite setup
   static void SetUpTestSuite()
   {
      my_framer = ne.Framer()
      my_framer.set_report_unknown_bytes(true)
      my_framer.set_payload_only(false)
      my_test_frame_buffer = ne.unsigned char[131071]; # 128kB
   }

   # Per-test-suite teardown
   static void TearDownTestSuite()
   {
      if (my_framer)
      {
         my_framer.shutdown_logger()
         delete my_framer
         my_framer = nullptr
      }

      if (myIFS)
      {
         delete myIFS
         myIFS = nullptr
      }

      if (my_test_frame_buffer)
      {
         delete[] my_test_frame_buffer
         my_test_frame_buffer = nullptr
      }
   }

   # Per-test setup
   void SetUp()
   {
      FlushFramer()
   }

   # Per-test teardown
   void TearDown()
   {
      FlushFramer()
   }

public:
   void WriteFileStreamToFramer(std.string filename_)
   {
      myIFS = ne.InputFileStream((Path(*TEST_RESOURCE_PATH) / filename_).string().c_str())
      StreamReadStatus read_status
      ReadDataStructure read_data
      read_data.data = ne.char[MAX_ASCII_MESSAGE_LENGTH]
      read_data.data_size = MAX_ASCII_MESSAGE_LENGTH
      uint32_t bytes_written = 0
      while (!read_status.eOS)
      {
         read_status = myIFS.read_data(read_data)
         bytes_written = my_framer.write(reinterpret_cast<unsigned char*>(read_data.data), read_status.current_stream_read)
         assert bytes_written == read_status.current_stream_read
      }

      delete[] read_data.data
      read_data.data = nullptr
      delete myIFS
      myIFS = nullptr
   }

   void WriteBytesToFramer(unsigned char* bytes_, uint32_t num_bytes_)
   {
      ASSERT_EQ(my_framer.write(bytes_, num_bytes_), num_bytes_)
   }

   bool CompareMetaData(MetaDataStruct* testMD_, MetaDataStruct* expected_meta_data_)
   {
      bool result = true
      if (testMD_.e_format != expected_meta_data_.e_format)
      {
         cout << "MetaData.format (expected " << static_cast<uint32_t>(expected_meta_data_.e_format) << ", got " << static_cast<uint32_t>(testMD_.e_format) << ")\n"
         result = false
      }
      if (testMD_.e_measurement_source != expected_meta_data_.e_measurement_source)
      {
         cout << "MetaData.measurement_source (expected " << static_cast<uint32_t>(expected_meta_data_.e_measurement_source) << ", got " << static_cast<uint32_t>(testMD_.e_measurement_source) << ")\n"
         result = false
      }
      if (testMD_.e_time_status != expected_meta_data_.e_time_status)
      {
         cout << "MetaData.time_status (expected " << static_cast<uint32_t>(expected_meta_data_.e_time_status) << ", got " << static_cast<uint32_t>(testMD_.e_time_status) << ")\n"
         result = false
      }
      if (testMD_.b_response != expected_meta_data_.b_response)
      {
         cout << "MetaData.response (expected " << static_cast<uint32_t>(expected_meta_data_.b_response) << ", got " << static_cast<uint32_t>(testMD_.b_response) << ")\n"
         result = false
      }
      if (testMD_.us_week != expected_meta_data_.us_week)
      {
         cout << "MetaData.week (expected " << expected_meta_data_.us_week << ", got " << testMD_.us_week << ")\n"
         result = false
      }
      if (testMD_.d_milliseconds != expected_meta_data_.d_milliseconds)
      {
         cout << "MetaData.milliseconds (expected " << expected_meta_data_.d_milliseconds << ", got " << testMD_.d_milliseconds << ")\n"
         result = false
      }
      if (testMD_.ui_binary_msg_length != expected_meta_data_.ui_binary_msg_length)
      {
         cout << "MetaData.binary_msg_length (expected " << expected_meta_data_.ui_binary_msg_length << ", got " << testMD_.ui_binary_msg_length << ")\n"
         result = false
      }
      if (testMD_.ui_length != expected_meta_data_.ui_length)
      {
         cout << "MetaData.length (expected " << expected_meta_data_.ui_length << ", got " << testMD_.ui_length << ")\n"
         result = false
      }
      if (testMD_.ui_header_length != expected_meta_data_.ui_header_length)
      {
         cout << "MetaData.header_length (expected " << expected_meta_data_.ui_header_length << ", got " << testMD_.ui_header_length << ")\n"
         result = false
      }
      if (testMD_.us_message_i_d != expected_meta_data_.us_message_i_d)
      {
         cout << "MetaData.messageID (expected " << expected_meta_data_.us_message_i_d << ", got " << testMD_.us_message_i_d << ")\n"
         result = false
      }
      if (testMD_.ui_message_c_r_c != expected_meta_data_.ui_message_c_r_c)
      {
         cout << "MetaData.messageCRC (expected " << expected_meta_data_.ui_message_c_r_c << ", got " << testMD_.ui_message_c_r_c << ")\n"
         result = false
      }
      if (testMD_.message_name() != expected_meta_data_.message_name())
      {
         cout << "MetaData.message_name (expected " << expected_meta_data_.message_name() << ", got " << testMD_.message_name() << ")\n"
         result = false
      }
      return result
   }

   void FlushFramer()
   {
      uint32_t bytes = 0
      do
      {
         bytes = my_framer.flush(my_test_frame_buffer, MAX_ASCII_MESSAGE_LENGTH)
      } while (bytes > 0)
   }
}
Framer* ProprietaryFramerTest.my_framer = nullptr
InputFileStream* ProprietaryFramerTest.myIFS = nullptr
unsigned char* ProprietaryFramerTest.my_test_frame_buffer = nullptr
# -------------------------------------------------------------------------------------------------------
# Proprietary Binary Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_PROPRIETARY_BINARY_COMPLETE():
{
   # "GARBAGE_DATA<binary bestpos log>"
   uint8_t data[] = { 0x47, 0x41, 0x52, 0x42, 0x41, 0x47, 0x45, 0x5F, 0x44, 0x41, 0x54, 0x41, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC }
   WriteBytesToFramer(data, sizeof(data))
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.length = 12
   expected_meta_data.format = HEADERFORMAT.UNKNOWN
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.UNKNOWN
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   expected_meta_data.length = 76
   expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.SUCCESS
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}

def test_PROPRIETARY_BINARY_INCOMPLETE():
{
   # "<incomplete binary bestpos log>"
   uint8_t data[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A }
   WriteBytesToFramer(data, sizeof(data))
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.length = 59
   expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.INCOMPLETE
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}

def test_PROPRIETARY_BINARY_SYNC_ERROR():
{
   WriteFileStreamToFramer("proprietary_binary_sync_error.BIN")
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.length = MAX_BINARY_MESSAGE_LENGTH
   expected_meta_data.format = HEADERFORMAT.UNKNOWN
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.UNKNOWN
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}

def test_PROPRIETARY_BINARY_BAD_CRC():
{
   # "<encrypted binary bestpos log>"
   uint8_t data[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xFF }
   WriteBytesToFramer(data, sizeof(data))
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.length = 30; # Unknown bytes up to 0x24 ('$') should be returned (NMEA sync was found mid-log)
   expected_meta_data.format = HEADERFORMAT.UNKNOWN
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.UNKNOWN
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}

def test_PROPRIETARY_BINARY_RUN_ON_CRC():
{
   # "<encrypted binary bestpos log>FF"
   uint8_t data[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC, 0xFF }
   WriteBytesToFramer(data, sizeof(data))
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.length = 76
   expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.SUCCESS
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}

def test_PROPRIETARY_BINARY_INADEQUATE_BUFFER():
{
   # "<encrypted binary bestpos log>"
   uint8_t data[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC }
   WriteBytesToFramer(data, sizeof(data))
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.length = 76
   expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
   assert my_framer.get_frame(my_test_frame_buffer, 38, test_meta_data) == STATUS.BUFFER_FULL
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   assert my_framer.get_frame(my_test_frame_buffer, 76, test_meta_data) == STATUS.SUCCESS
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}

def test_PROPRIETARY_BINARY_BYTE_BY_BYTE():
{
   # "<binary bestpos log>"
   uint8_t data[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC }
   uint32_t log_size = sizeof(data)
   uint32_t remaining_bytes = log_size
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.format = HEADERFORMAT.UNKNOWN
   while (true)
   {
      WriteBytesToFramer(&data[log_size - remaining_bytes], 1)
      remaining_bytes--
      expected_meta_data.length = log_size - remaining_bytes
      if (expected_meta_data.length == OEM4_BINARY_SYNC_LENGTH - 1)
      {
         expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
      }

      if (remaining_bytes > 0)
      {
         assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.INCOMPLETE
         assert CompareMetaData(&test_meta_data, &expected_meta_data)
      }
      else
      {
         break
      }
   }
   expected_meta_data.length = log_size
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.SUCCESS
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}

def test_PROPRIETARY_BINARY_SEGMENTED():
{
   # "<binary bestpos log>"
   uint8_t data[] = { 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC }
   uint32_t log_size = sizeof(data)
   uint32_t bytes_written = 0
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
   WriteBytesToFramer(&data[bytes_written], OEM4_BINARY_SYNC_LENGTH)
   bytes_written += OEM4_BINARY_SYNC_LENGTH
   expected_meta_data.length = bytes_written
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.INCOMPLETE
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   WriteBytesToFramer(&data[bytes_written], (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH))
   bytes_written += (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH)
   expected_meta_data.length = bytes_written
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.INCOMPLETE
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   WriteBytesToFramer(&data[bytes_written], 44)
   bytes_written += 44
   expected_meta_data.length = bytes_written
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.INCOMPLETE
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   WriteBytesToFramer(&data[bytes_written], OEM4_BINARY_CRC_LENGTH)
   bytes_written += OEM4_BINARY_CRC_LENGTH
   expected_meta_data.length = bytes_written
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.SUCCESS
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
   assert log_size == bytes_written
}

def test_PROPRIETARY_BINARY_TRICK():
{
   # "<binary syncs><binary sync + half header><binary sync byte 1><binary bestpos log>"
   uint8_t data[] = { 0xAA, 0x45, 0x12, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0xAA, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC }
   uint32_t log_size = sizeof(data)
   MetaDataStruct expected_meta_data, test_meta_data
   expected_meta_data.format = HEADERFORMAT.UNKNOWN
   WriteBytesToFramer(data, log_size)
   expected_meta_data.length = 3
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.UNKNOWN
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   expected_meta_data.length = 15
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.UNKNOWN
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   expected_meta_data.length = 1
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.UNKNOWN
   assert CompareMetaData(&test_meta_data, &expected_meta_data)

   expected_meta_data.length = 76
   expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
   assert my_framer.get_frame(my_test_frame_buffer, MAX_BINARY_MESSAGE_LENGTH, test_meta_data) == STATUS.SUCCESS
   assert CompareMetaData(&test_meta_data, &expected_meta_data)
}
