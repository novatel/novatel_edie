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

class EncoderTester : public Encoder:
   public:
      EncoderTester(JsonReader* json_db_) : Encoder(json_db_) {}

      bool TestEncodeBinaryBody(const IntermediateMessage intermediate_message_, unsigned char** out_buf_, uint32_t bytes):
         return Encoder.EncodeBinaryBody(intermediate_message_, out_buf_, bytes, False);;
public:
   JsonReader* my_json_db;
   DecoderTester* my_decoder_tester;
   EncoderTester* my_encoder_tester;
   std.vector<BaseField*> MsgDefFields_;
   std.string min_json_db;

   NovatelTypesTest() {
      min_json_db = "{ \
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

   virtual void SetUp():
      try:
         my_json_db = ne.JsonReader();
         #my_json_db.load_file(*TEST_DB_PATH);
         my_json_db.parse_json(min_json_db);
         my_decoder_tester = ne.DecoderTester(my_json_db);
         my_encoder_tester = ne.EncoderTester(my_json_db);
      catch (JsonReaderFailure e):
         printf("%s\n", e.what());

         if (my_json_db)
         {
            my_json_db = nullptr;
         }
         if (my_decoder_tester)
         {
            my_decoder_tester = nullptr;
         }
         MsgDefFields_.clear();
   virtual void TearDown():
      if my_json_db:
         my_json_db = nullptr;
      if my_decoder_tester:
         my_decoder_tester.shutdown_logger();
         my_decoder_tester = nullptr;
      MsgDefFields_.clear();
   void CreateBaseField(std.string name_, FIELD_TYPE field_type_, CONVERSION_STRING conversion_stripped_, uint16_t length_, DATA_TYPE_NAME data_type_name_):
      BaseField* field = ne.BaseField();
      field.name = name_;
      field.type = field_type_;
      field.conversion_stripped = conversion_stripped_;
      field.data_type.length = length_;
      field.data_type.name = data_type_name_;
      MsgDefFields_.emplace_back(field);
   void CreateEnumField(std.string name, std.string description, int32_t value):
      EnumField* field = ne.EnumField();
      EnumDefinition* def = ne.EnumDefinition();
      EnumDataType* d_t = ne.EnumDataType();
      d_t.name = name;
      d_t.description = description;
      d_t.value = value;
      def.enumerators.push_back(*d_t);
      field.enum_def = def;
      field.type = FIELD_TYPE.ENUM;
      MsgDefFields_.emplace_back(field);;

def test_LOGGER():
   level = spdlog.level.off;

   assert spdlog.get("novatel_message_decoder") != nullptr
   std.shared_ptr<spdlog.logger> novatel_message_decoder = my_decoder_tester.get_logger();
   my_decoder_tester.set_logger_level(level);
   assert novatel_message_decoder.level() == level

def test_FIELD_CONTAINER_ERROR_ON_COPY():
   FieldContainer fc = FieldContainer(3, ne.BaseField());
   ASSERT_THROW(FieldContainer fc2(fc);, std.runtime_error);

def test_ASCII_CHAR_BYTE_VALID():
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(9);

   const char* input = "-129,-128,-127,-1,0,1,127,128,129";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS.SUCCESS
   assert std.get<int8_t>(intermediate_format_[0].field_value) == 127
   assert std.get<int8_t>(intermediate_format_[1].field_value) == -128
   assert std.get<int8_t>(intermediate_format_[2].field_value) == -127
   assert std.get<int8_t>(intermediate_format_[3].field_value) == -1
   assert std.get<int8_t>(intermediate_format_[4].field_value) == 0
   assert std.get<int8_t>(intermediate_format_[5].field_value) == 1
   assert std.get<int8_t>(intermediate_format_[6].field_value) == 127
   assert std.get<int8_t>(intermediate_format_[7].field_value) == -128
   assert std.get<int8_t>(intermediate_format_[8].field_value) == -127

def test_ASCI_UCHAR_BYTE_VALID():
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(9);

   const char* input = "-256,-255,-254,-1,0,1,254,255,256";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS.SUCCESS
   assert std.get<uint8_t>(intermediate_format_[0].field_value) == 0
   assert std.get<uint8_t>(intermediate_format_[1].field_value) == 1
   assert std.get<uint8_t>(intermediate_format_[2].field_value) == 2
   assert std.get<uint8_t>(intermediate_format_[3].field_value) == 255
   assert std.get<uint8_t>(intermediate_format_[4].field_value) == 0
   assert std.get<uint8_t>(intermediate_format_[5].field_value) == 1
   assert std.get<uint8_t>(intermediate_format_[6].field_value) == 254
   assert std.get<uint8_t>(intermediate_format_[7].field_value) == 255
   assert std.get<uint8_t>(intermediate_format_[8].field_value) == 0

def test_ASCII_UCHAR_BYTE_INVALID():
   CreateBaseField("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "0.1";
   my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert std.get<int8_t>(intermediate_format_[0].field_value) == 0

def test_ASCII_CHAR_VALID():
   CreateBaseField("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE_NAME.CHAR);
   CreateBaseField("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE_NAME.CHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(3);

   const char* input = "#,A,;";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS.SUCCESS
   assert std.get<int8_t>(intermediate_format_[0].field_value) == '#'
   assert std.get<int8_t>(intermediate_format_[1].field_value) == 'A'
   assert std.get<int8_t>(intermediate_format_[2].field_value) == ';'

def test_ASCII_CHAR_INVALID():
   CreateBaseField("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 2, DATA_TYPE_NAME.CHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std.runtime_error);

def test_ASCII_UCHAR_VALID():
   CreateBaseField("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE_NAME.UCHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(3);

   const char* input = "#,A,;";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS.SUCCESS
   assert std.get<uint8_t>(intermediate_format_[0].field_value) == '#'
   assert std.get<uint8_t>(intermediate_format_[1].field_value) == 'A'
   assert std.get<uint8_t>(intermediate_format_[2].field_value) == ';'

def test_ASCII_UCHAR_INVALID():
   CreateBaseField("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 2, DATA_TYPE_NAME.UCHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std.runtime_error);

def test_ASCII_INT_VALID():
   CreateBaseField("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR);
   CreateBaseField("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(6);

   const char* input = "-32769,-32768,-32767,32767,32768,32769";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS.SUCCESS
   assert std.get<int16_t>(intermediate_format_[0].field_value) == 32767
   assert std.get<int16_t>(intermediate_format_[1].field_value) == -32768
   assert std.get<int16_t>(intermediate_format_[2].field_value) == -32767
   assert std.get<int16_t>(intermediate_format_[3].field_value) == 32767
   assert std.get<int16_t>(intermediate_format_[4].field_value) == -32768
   assert std.get<int16_t>(intermediate_format_[5].field_value) == -32767

def test_ASCII_INT_INVALID():
   CreateBaseField("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 3, DATA_TYPE_NAME.UCHAR);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std.runtime_error);

def test_ASCII_FLOAT_VALID():
   CreateBaseField("Und", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE_NAME.FLOAT);
   CreateBaseField("LatStd", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE_NAME.FLOAT);
   CreateBaseField("LongStd", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE_NAME.FLOAT);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(3);

   const char* input = "51.11636937989,-114.03825348307,0";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS.SUCCESS
   ASSERT_NEAR(std.get<float>(intermediate_format_[0].field_value),   51.11636937989f, std.numeric_limits<float>.epsilon());
   assert std.get<float>(intermediate_format_[1].field_value) == approx(-114.03825348307f, abs=std.numeric_limits<float>.epsilon())
   ASSERT_NEAR(std.get<float>(intermediate_format_[2].field_value),                 0, std.numeric_limits<float>.epsilon());

def test_ASCII_FLOAT_INVALID():
   CreateBaseField("Und", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 5, DATA_TYPE_NAME.FLOAT);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std.runtime_error);

def test_ASCII_DOUBLE_VALID():
   CreateBaseField("Lat", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE);
   CreateBaseField("Long", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE);
   CreateBaseField("Ht", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE);
   CreateBaseField("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(7);

   const char* input = "51.11636937989,-114.03825348307,0,1.7e+308,-1.7e+308,1.7e+309,-1.7e+309";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);
   constexpr double inf = std.numeric_limits<double>.infinity();

   assert decoder_status == STATUS.SUCCESS
   ASSERT_DOUBLE_EQ(std.get<double>(intermediate_format_[0].field_value), 51.11636937989);
   ASSERT_DOUBLE_EQ(std.get<double>(intermediate_format_[1].field_value), -114.03825348307);
   ASSERT_DOUBLE_EQ(std.get<double>(intermediate_format_[2].field_value), 0);
   ASSERT_DOUBLE_EQ(std.get<double>(intermediate_format_[3].field_value), 1.7e+308);
   ASSERT_DOUBLE_EQ(std.get<double>(intermediate_format_[4].field_value), -1.7e+308);
   ASSERT_DOUBLE_EQ(std.get<double>(intermediate_format_[5].field_value), inf);
   ASSERT_DOUBLE_EQ(std.get<double>(intermediate_format_[6].field_value), -inf);

def test_ASCII_DOUBLE_INVALID():
   CreateBaseField("Lat", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 9, DATA_TYPE_NAME.DOUBLE);
   std.vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std::runtime_error);

def test_ASCII_BOOL_VALID():
   CreateBaseField("B_True", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 4, DATA_TYPE_NAME::BOOL);
   CreateBaseField("B_False", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 4, DATA_TYPE_NAME::BOOL);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(2);

   const char* input = "TRUE,FALSE";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<bool>(intermediate_format_[0].field_value)
   assert not std::get<bool>(intermediate_format_[1].field_value)

def test_ASCII_BOOL_INVALID():
   CreateBaseField("B_True", FIELD_TYPE::SIMPLE, CONVERSION_STRING::d, 4, DATA_TYPE_NAME::BOOL);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "True";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std::runtime_error);

def test_ASCII_UINT_VALID():
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("toe", FIELD_TYPE::SIMPLE, CONVERSION_STRING::u, 4, DATA_TYPE_NAME::UINT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   const char* input = "-1,0,4294967294,4294967295";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<uint32_t>(intermediate_format_[0].field_value) == 4294967295U
   assert std::get<uint32_t>(intermediate_format_[1].field_value) == 0U
   assert std::get<uint32_t>(intermediate_format_[2].field_value) == 4294967294U
   assert std::get<uint32_t>(intermediate_format_[3].field_value) == 4294967295U

def test_ASCII_GPSTIME_MSEC_VALID():
   CreateBaseField("Sec1", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec2", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec3", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::T, 4, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   const char* input = "-1.000,0.000,604800.000,4294967295.000";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   # If GPSTIME exceeds 4,294,967.295 (seconds) the conversion to milliseconds is wrong
   # But the limit should be 604,800 (seconds) as that's the number of seconds in a GPS reference week
   ASSERT_EQ(std::get<uint32_t>(intermediate_format_[0].field_value), 4294966296U);  # 4,294,967,295 + 1 - 1,000 = 4,294,966,296
   assert std::get<uint32_t>(intermediate_format_[1].field_value) == 0U
   assert std::get<uint32_t>(intermediate_format_[2].field_value) == 604800000U
   assert std::get<uint32_t>(intermediate_format_[3].field_value) == 4294966296U

def test_ASCII_SCIENTIFIC_NOTATION_FLOAT_VALID():
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 4, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   const char* input = "-1.0,0.0,1.175494351e-38,3.402823466e+38";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   ASSERT_NEAR(std::get<float>(intermediate_format_[0].field_value),               -1, std::numeric_limits<float>::epsilon());
   ASSERT_NEAR(std::get<float>(intermediate_format_[1].field_value),                0, std::numeric_limits<float>::epsilon());
   assert std::get<float>(intermediate_format_[2].field_value) == approx(1.175494351e-38f, abs=std::numeric_limits<float>::epsilon())
   assert std::get<float>(intermediate_format_[3].field_value) == approx(3.402823466e+38f, abs=std::numeric_limits<float>::epsilon())

def test_ASCII_SCIENTIFIC_NOTATION_FLOAT_INVALID():
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 5, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "-1.0";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std::runtime_error);

def test_ASCII_SCIENTIFIC_NOTATION_DOUBLE_VALID():
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::g, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("Sec4", FIELD_TYPE::SIMPLE, CONVERSION_STRING::e, 8, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   const char* input = "-1.0,0.0,2.2250738585072014e-308,1.7976931348623158e+308";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[0].field_value), -1);
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[1].field_value), 0);
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[2].field_value), 2.2250738585072014e-308);
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[3].field_value), 1.7976931348623158e+308);

def test_ASCII_ULONG_VALID():
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
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(12);

   const char* input = "-1,0,255,-1,0,65535,-1,0,4294967295,-1,0,18446744073709551615";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<uint8_t>(intermediate_format_[0].field_value) == 255
   assert std::get<uint8_t>(intermediate_format_[1].field_value) == 0
   assert std::get<uint8_t>(intermediate_format_[2].field_value) == 255
   assert std::get<uint16_t>(intermediate_format_[3].field_value) == 65535
   assert std::get<uint16_t>(intermediate_format_[4].field_value) == 0
   assert std::get<uint16_t>(intermediate_format_[5].field_value) == 65535
   assert std::get<uint32_t>(intermediate_format_[6].field_value) == 4294967295U
   assert std::get<uint32_t>(intermediate_format_[7].field_value) == 0U
   assert std::get<uint32_t>(intermediate_format_[8].field_value) == 4294967295U
   assert std::get<uint64_t>(intermediate_format_[9].field_value) == ULLONG_MAX
   assert std::get<uint64_t>(intermediate_format_[10].field_value) == 0ULL
   assert std::get<uint64_t>(intermediate_format_[11].field_value) == ULLONG_MAX

def test_ASCII_ENUM_VALID():
   CreateEnumField("UNKNOWN", "Unknown or unspecified type", 20);
   CreateEnumField("APPROXIMATE", "Approximate time", 60);
   CreateEnumField("SATTIME", "", 200);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(3);

   const char* input = "UNKNOWN,APPROXIMATE,SATTIME";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<int32_t>(intermediate_format_[0].field_value) == 20
   assert std::get<int32_t>(intermediate_format_[1].field_value) == 60
   assert std::get<int32_t>(intermediate_format_[2].field_value) == 200

def test_ASCII_STRING_VALID():
   CreateBaseField("MESSAGE", FIELD_TYPE::STRING, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "#RAWEPHEMA,COM1,100";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   ASSERT_EQ(std::get<std::string>(intermediate_format_[0].field_value), "RAWEPHEMA,COM1,100");

def test_ASCII_EMPTY_STRING_VALID():
   CreateBaseField("MESSAGE", FIELD_TYPE::STRING, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "\"\"";
   STATUS decoder_status = my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<std::string>(intermediate_format_[0].field_value) == ""

def test_ASCII_TYPE_INVALID():
   CreateBaseField("", FIELD_TYPE::UNKNOWN, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(1);

   const char* input = "";

   ASSERT_THROW(my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_), std::runtime_error);

def test_BINARY_BOOL_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::BOOL);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::BOOL);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(2);

   bool input[]  = [ 1, 0 ];
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<bool>(intermediate_format_[0].field_value)
   assert not std::get<bool>(intermediate_format_[1].field_value)

def test_BINARY_HEXBYTE_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::HEXBYTE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::HEXBYTE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::HEXBYTE);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(3);

   uint8_t input[]  = [ 0x00, 0x01, 0xFF ];
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<uint8_t>(intermediate_format_[0].field_value) == 0
   assert std::get<uint8_t>(intermediate_format_[1].field_value) == 1
   assert std::get<uint8_t>(intermediate_format_[2].field_value) == UCHAR_MAX

def test_BINARY_uint8_t_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UCHAR);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(5);

   uint8_t input[]  = [ 0x23, 0x41, 0x3B, 0x00, 0xFF ];
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<uint8_t>(intermediate_format_[0].field_value) == '#'
   assert std::get<uint8_t>(intermediate_format_[1].field_value) == 'A'
   assert std::get<uint8_t>(intermediate_format_[2].field_value) == ';'
   assert std::get<uint8_t>(intermediate_format_[3].field_value) == 0
   assert std::get<uint8_t>(intermediate_format_[4].field_value) == UCHAR_MAX

def test_BINARY_USHORT_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::USHORT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   uint8_t input[]  = [ 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0xFF, 0xFF ];
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<uint16_t>(intermediate_format_[0].field_value) == 0
   assert std::get<uint16_t>(intermediate_format_[1].field_value) == 1
   assert std::get<uint16_t>(intermediate_format_[2].field_value) == 16
   assert std::get<uint16_t>(intermediate_format_[3].field_value) == USHRT_MAX
def test_BINARY_SHORT_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 2, DATA_TYPE_NAME::SHORT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   uint8_t input[]  = [ 0x00, 0x80, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x7F ];
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<int16_t>(intermediate_format_[0].field_value) == SHRT_MIN
   assert std::get<int16_t>(intermediate_format_[1].field_value) == -1
   assert std::get<int16_t>(intermediate_format_[2].field_value) == -0
   assert std::get<int16_t>(intermediate_format_[3].field_value) == SHRT_MAX
def test_BINARY_INT_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::INT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   uint8_t input[] = {
      0x00, 0x00, 0x00, 0x80,
      0x00, 0x00, 0xFF, 0xFF,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0x7F;
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<int32_t>(intermediate_format_[0].field_value) == INT_MIN
   assert std::get<int32_t>(intermediate_format_[1].field_value) == -65536
   assert std::get<int32_t>(intermediate_format_[2].field_value) == 0
   assert std::get<int32_t>(intermediate_format_[3].field_value) == INT_MAX

def test_BINARY_UINT_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::UINT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   uint8_t input[] = {
      0x00, 0x00, 0x00, 0x80,
      0xFF, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF;
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<uint32_t>(intermediate_format_[0].field_value) == 2147483648U
   assert std::get<uint32_t>(intermediate_format_[1].field_value) == 65535U
   assert std::get<uint32_t>(intermediate_format_[2].field_value) == 0U
   assert std::get<uint32_t>(intermediate_format_[3].field_value) == UINT_MAX

def test_BINARY_ULONG_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::ULONG);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   uint8_t input[] = {
      0x00, 0x00, 0x00, 0x80,
      0xFF, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF;
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<uint32_t>(intermediate_format_[0].field_value) == 2147483648U
   assert std::get<uint32_t>(intermediate_format_[1].field_value) == 65535U
   assert std::get<uint32_t>(intermediate_format_[2].field_value) == 0U
   assert std::get<uint32_t>(intermediate_format_[3].field_value) == UINT_MAX

def test_BINARY_CHAR_BYTE_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::CHAR);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   uint8_t input[]  = [ 0x80, 0xFF, 0x00, 0x7F ];
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   assert std::get<int8_t>(intermediate_format_[0].field_value) == CHAR_MIN
   assert std::get<int8_t>(intermediate_format_[1].field_value) == -1
   assert std::get<int8_t>(intermediate_format_[2].field_value) == 0
   assert std::get<int8_t>(intermediate_format_[3].field_value) == CHAR_MAX

def test_BINARY_FLOAT_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::FLOAT);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 4, DATA_TYPE_NAME::FLOAT);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(2);

   uint8_t input[]  = [ 0x9A, 0x99, 0x99, 0x3F, 0xCD, 0xCC, 0xBC, 0xC0 ];
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);

   assert decoder_status == STATUS::SUCCESS
   ASSERT_FLOAT_EQ(std::get<float>(intermediate_format_[0].field_value), 1.2f);
   ASSERT_FLOAT_EQ(std::get<float>(intermediate_format_[1].field_value), -5.9f);

def test_BINARY_DOUBLE_VALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 8, DATA_TYPE_NAME::DOUBLE);
   std::vector<FieldContainer> intermediate_format_;
   intermediate_format_.reserve(4);

   uint8_t input[] = {
      0x12, 0x71, 0x1C, 0x31, 0xE5, 0x8E, 0x49, 0x40,
      0x99, 0xAF, 0xBC, 0xBE, 0x72, 0x82, 0x5C, 0xC0,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x7F;
   unsigned char* input = reinterpret_cast<unsigned char*>(input);
   STATUS decoder_status = my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_);
   constexpr double inf = std::numeric_limits<double>::infinity();

   assert decoder_status == STATUS::SUCCESS
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[0].field_value), 51.11636937989);
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[1].field_value), -114.03825348307);
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[2].field_value), 0);
   ASSERT_DOUBLE_EQ(std::get<double>(intermediate_format_[3].field_value), inf);

def test_BINARY_SIMPLE_TYPE_INVALID():
   CreateBaseField("", FIELD_TYPE::SIMPLE, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> intermediate_format_;

   unsigned char* input = nullptr;

   ASSERT_THROW(my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_), std::runtime_error);

def test_BINARY_TYPE_INVALID():
   CreateBaseField("", FIELD_TYPE::UNKNOWN, CONVERSION_STRING::UNKNOWN, 1, DATA_TYPE_NAME::UNKNOWN);
   std::vector<FieldContainer> intermediate_format_;

   unsigned char* input = nullptr;

   ASSERT_THROW(my_decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_), std::runtime_error);

def test_SIMPLE_FIELD_WIDTH_VALID():
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

   IntermediateMessage intermediate_format;
   intermediate_format.reserve(MsgDefFields_.size());

   const char* input = "TRUE,0x63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03";
   unsigned char encode_buffer[MAX_ASCII_MESSAGE_LENGTH];
   unsigned char* encode_buffer = encode_buffer;

   assert my_decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format) == STATUS::SUCCESS
   assert my_encoder_tester.test_encode_binary_body(intermediate_format, encode_buffer, MAX_ASCII_MESSAGE_LENGTH)

   intermediate_format.clear();
   encode_buffer = encode_buffer;
   my_decoder_tester.test_decode_binary(MsgDefFields_, encode_buffer, intermediate_format);

   size_t sz = 0;

   ASSERT_EQ  (std::get<bool    >(intermediate_format.at(sz++).field_value), True);
   ASSERT_EQ  (std::get<uint8_t >(intermediate_format.at(sz++).field_value), 99);
   ASSERT_EQ  (std::get<uint8_t >(intermediate_format.at(sz++).field_value), 227);
   ASSERT_EQ  (std::get<int8_t  >(intermediate_format.at(sz++).field_value), 56);
   ASSERT_EQ  (std::get<uint16_t>(intermediate_format.at(sz++).field_value), 2734);
   ASSERT_EQ  (std::get<int16_t >(intermediate_format.at(sz++).field_value), -3842);
   ASSERT_EQ  (std::get<uint32_t>(intermediate_format.at(sz++).field_value), 38283U);
   ASSERT_EQ  (std::get<uint32_t>(intermediate_format.at(sz++).field_value), 54244U);
   ASSERT_EQ  (std::get<int32_t >(intermediate_format.at(sz++).field_value), -4359);
   ASSERT_EQ  (std::get<int32_t >(intermediate_format.at(sz++).field_value), 5293);
   ASSERT_EQ  (std::get<uint64_t>(intermediate_format.at(sz++).field_value), 79338432ULL);
   ASSERT_EQ  (std::get<int64_t >(intermediate_format.at(sz++).field_value), -289834LL);
   assert std::get<float   >(intermediate_format.at(sz++).field_value) == approx(2.54, abs=0.001)
   assert std::get<double  >(intermediate_format.at(sz++).field_value) == approx(5.44061788e+03, abs=0.000001)
