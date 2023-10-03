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
# \file novateltest.hpp
# \brief Unit tests for OEM Framer, HeaderDecoder, MessageDecoder,
# Encoder and Filter.
################################################################################

from pathlib import Path

import novatel_edie as ne
from novatel_edie import HEADERFORMAT, STATUS
import pytest

# -------------------------------------------------------------------------------------------------------
# FileParser Unit Tests
# -------------------------------------------------------------------------------------------------------
class FileParserTest : public .testing.Test

protected:
   static FileParser* fp;

   # Per-test-suite setup
   static void SetUpTestSuite():
      try:
         fp = ne.FileParser(*TEST_DB_PATH);
      catch (JsonReaderFailure e):
         printf("%s\n", e.what());

         if (fp)
         {
            fp = nullptr;
         }

   # Per-test-suite teardown
   static void TearDownTestSuite():
      if fp:
         fp.shutdown_logger();
         fp = nullptr;;
FileParser* FileParserTest.fp = nullptr;

def test_LOGGER():
   level = spdlog.level.off;

   # FileParser logger
   assert spdlog.get("novatel_fileparser") != nullptr
   std.shared_ptr<spdlog.logger> novatel_fileparser = fp.get_logger();
   fp.set_logger_level(level);
   assert novatel_fileparser.level() == level

   # Parser logger
   assert spdlog.get("novatel_parser") != nullptr
   ASSERT_NO_THROW(fp.enable_framer_decoder_logging(level, "novatel_parser.log"));

def test_FILEPARSER_INSTANTIATION():
   ASSERT_NO_THROW(FileParser fp1 = FileParser());
   ASSERT_NO_THROW(FileParser fp2 = FileParser(*TEST_DB_PATH));

   std.string tEST_DB_PATH = *TEST_DB_PATH;
   const std.u32string tEST_DB_PATH(tEST_DB_PATH.begin(), tEST_DB_PATH.end());
   ASSERT_NO_THROW(FileParser fp3 = FileParser(tEST_DB_PATH));

   JsonReader* db = ne.JsonReader();
   db.load_file(*TEST_DB_PATH);
   ASSERT_NO_THROW(FileParser fp4 = FileParser(db));

def test_LOAD_JSON_DB_STRING():
   JsonReader my_json_db;
   my_json_db.LoadFile<std.string>(*TEST_DB_PATH);
   ASSERT_NO_THROW(fp.load_json_db(my_json_db));
   ASSERT_NO_THROW(fp.load_json_db(nullptr));

def test_LOAD_JSON_DB_U32STRING():
   JsonReader my_json_db;
   std.wstring_convert<std.codecvt_utf8<char32_t>, char32_t> converter;
   std.u32string u32str = converter.from_bytes(*TEST_DB_PATH);
   my_json_db.LoadFile<std.u32string>(u32str);
   ASSERT_NO_THROW(fp.load_json_db(my_json_db));
   ASSERT_NO_THROW(fp.load_json_db(nullptr));

def test_LOAD_JSON_DB_CHAR_ARRAY():
   JsonReader my_json_db;
   my_json_db.LoadFile<char*>(const_cast<char*>(TEST_DB_PATH.c_str()));
   ASSERT_NO_THROW(fp.load_json_db(my_json_db));
   ASSERT_NO_THROW(fp.load_json_db(nullptr));

def test_RANGE_CMP():
   fp.set_decompress_range_cmp(True);
   assert fp.get_decompress_range_cmp()
   fp.set_decompress_range_cmp(False);
   assert not fp.get_decompress_range_cmp()

def test_UNKNOWN_BYTES():
   fp.set_return_unknown_bytes(True);
   assert fp.get_return_unknown_bytes()
   fp.set_return_unknown_bytes(False);
   assert not fp.get_return_unknown_bytes()

def test_PARSE_FILE_WITH_FILTER():
   # Reset the FileParser with the database because a previous test assigns it to the nullptr
   fp = ne.FileParser(*TEST_DB_PATH);
   Filter* filter = ne.Filter();
   filter.set_logger_level(spdlog.level.debug);
   fp.set_filter(filter);
   assert fp.get_filter() == filter

   Path test_gps_file = Path(*TEST_RESOURCE_PATH) / "BESTUTMBIN.GPS";
   InputFileStream input_file_stream = InputFileStream(test_gps_file.string().c_str());
   assert fp.set_stream(input_file_stream)

   MetaData meta_data;
   MessageDataStruct message_data;

   int success = 0;
   uint32_t expected_meta_data_length[2]  = [ 213, 195 ];
   double expected_milliseconds[2]  = [ 270605000, 172189053 ];
   uint32_t expected_message_length[2]  = [ 213, 195 ];

   STATUS status = STATUS.UNKNOWN;
   fp.set_encode_format(ENCODEFORMAT.ASCII);
   assert fp.get_encode_format() == ENCODEFORMAT.ASCII

   while status != STATUS.STREAM_EMPTY:
      status = fp.read(message_data, meta_data);
      if status == STATUS.SUCCESS:
         assert meta_data.length == expected_meta_data_length[success]
         ASSERT_DOUBLE_EQ(meta_data.milliseconds, expected_milliseconds[success]);
         assert message_data.message_length == expected_message_length[success]
         success++;
   assert fp.get_percent_read() == 100U
   assert success == 2

def test_RESET():
   fp = ne.FileParser();
   ASSERT_NO_THROW(fp.get_internal_buffer(););
   assert fp.reset()
