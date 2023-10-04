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

import novatel_edie as ne
from novatel_edie import STATUS, FIELD_TYPE, CONVERSION_STRING, DATA_TYPE_NAME
import pytest
from pytest import approx

# -------------------------------------------------------------------------------------------------------
# Novatel Types Unit Tests
# -------------------------------------------------------------------------------------------------------


min_json_db = """
{
  "enums": [
    {
      "name": "Responses",
      "_id": "0",
      "enumerators": []
    },
    {
      "name": "Commands",
      "_id": "0",
      "enumerators": []
    },
    {
      "name": "PortAddress",
      "_id": "0",
      "enumerators": []
    },
    {
      "name": "GPSTimeStatus",
      "_id": "0",
      "enumerators": []
    }
  ],
  "messages": []
}
"""

UCHAR_MAX = 255
CHAR_MIN = -127
CHAR_MAX = 128
SHRT_MIN = -32767
SHRT_MAX = 32767
USHRT_MAX = 65535
INT_MIN = -32767
INT_MAX = 32767
UINT_MAX = 65535
LONG_MIN = -2147483647
LONG_MAX = 2147483647
ULONG_MAX = 4294967295
LLONG_MIN = -9223372036854775807
LLONG_MAX = 9223372036854775807
ULLONG_MAX = 18446744073709551615


@pytest.fixture(scope="module")
def json_db():
   json_db = ne.JsonReader()
   json_db.parse_json(min_json_db)
   return json_db

@pytest.fixture(scope="function")
def message_decoder(json_db):
   return ne.MessageDecoder(json_db)

@pytest.fixture(scope="function")
def encoder(json_db):
   return ne.Encoder(json_db)


def create_base_field(name_, field_type_, conversion_stripped_, length_, data_type_name_):
   field = ne.BaseField()
   field.name = name_
   field.type = field_type_
   field.conversion_stripped = conversion_stripped_
   field.data_type.length = length_
   field.data_type.name = data_type_name_
   MsgDefFields_.append(field)


def create_enum_field(name, description, value):
   field = ne.EnumField()
   definition = ne.EnumDefinition()
   d_t = ne.EnumDataType()
   d_t.name = name
   d_t.description = description
   d_t.value = value
   definition.enumerators.push_back(*d_t)
   field.enum_def = definition
   field.type = FIELD_TYPE.ENUM
   MsgDefFields_.append(field)


def test_FIELD_CONTAINER_ERROR_ON_COPY(decoder_tester):
   fc = FieldContainer(3, ne.BaseField())
   with pytest.raises(Exception):
      FieldContainer fc2(fc)

def test_ASCII_CHAR_BYTE_VALID(decoder_tester):
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   intermediate_format_ = [None] * 9

   input = "-129,-128,-127,-1,0,1,127,128,129"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== STATUS.SUCCESS
   assert int(intermediate_format_[0].field_value) == 127
   assert int(intermediate_format_[1].field_value) == -128
   assert int(intermediate_format_[2].field_value) == -127
   assert int(intermediate_format_[3].field_value) == -1
   assert int(intermediate_format_[4].field_value) == 0
   assert int(intermediate_format_[5].field_value) == 1
   assert int(intermediate_format_[6].field_value) == 127
   assert int(intermediate_format_[7].field_value) == -128
   assert int(intermediate_format_[8].field_value) == -127

def test_ASCI_UCHAR_BYTE_VALID(decoder_tester):
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE_NAME.UCHAR)
   intermediate_format_ = [None] * 9

   input = "-256,-255,-254,-1,0,1,254,255,256"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== STATUS.SUCCESS
   assert int(intermediate_format_[0].field_value) == 0
   assert int(intermediate_format_[1].field_value) == 1
   assert int(intermediate_format_[2].field_value) == 2
   assert int(intermediate_format_[3].field_value) == 255
   assert int(intermediate_format_[4].field_value) == 0
   assert int(intermediate_format_[5].field_value) == 1
   assert int(intermediate_format_[6].field_value) == 254
   assert int(intermediate_format_[7].field_value) == 255
   assert int(intermediate_format_[8].field_value) == 0

def test_ASCII_UCHAR_BYTE_INVALID(decoder_tester):
   create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE_NAME.CHAR)
   intermediate_format_ = [None]

   input = "0.1"
   decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert int(intermediate_format_[0].field_value) == 0

def test_ASCII_CHAR_VALID(decoder_tester):
   create_base_field("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE_NAME.CHAR)
   intermediate_format_ = [None, None, None]

   input = "#,A,;"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== STATUS.SUCCESS
   assert int(intermediate_format_[0].field_value) == '#'
   assert int(intermediate_format_[1].field_value) == 'A'
   assert int(intermediate_format_[2].field_value) == ';'

def test_ASCII_CHAR_INVALID(decoder_tester):
   create_base_field("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 2, DATA_TYPE_NAME.CHAR)
   intermediate_format_ = [None]

   input = ""
   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_ASCII_UCHAR_VALID(decoder_tester):
   create_base_field("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE_NAME.UCHAR)
   intermediate_format_ = [None, None, None]

   input = "#,A,;"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== STATUS.SUCCESS
   assert int(intermediate_format_[0].field_value) == '#'
   assert int(intermediate_format_[1].field_value) == 'A'
   assert int(intermediate_format_[2].field_value) == ';'

def test_ASCII_UCHAR_INVALID(decoder_tester):
   create_base_field("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 2, DATA_TYPE_NAME.UCHAR)
   intermediate_format_ = [None]

   input = ""

   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_ASCII_INT_VALID(decoder_tester):
   create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR)
   create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE_NAME.UCHAR)
   intermediate_format_ = [None] * 6

   input = "-32769,-32768,-32767,32767,32768,32769"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== STATUS.SUCCESS
   assert int(intermediate_format_[0].field_value) == 32767
   assert int(intermediate_format_[1].field_value) == -32768
   assert int(intermediate_format_[2].field_value) == -32767
   assert int(intermediate_format_[3].field_value) == 32767
   assert int(intermediate_format_[4].field_value) == -32768
   assert int(intermediate_format_[5].field_value) == -32767

def test_ASCII_INT_INVALID(decoder_tester):
   create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 3, DATA_TYPE_NAME.UCHAR)
   intermediate_format_ = [None]

   input = ""

   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_ASCII_FLOAT_VALID(decoder_tester):
   create_base_field("Und", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE_NAME.FLOAT)
   create_base_field("LatStd", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE_NAME.FLOAT)
   create_base_field("LongStd", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE_NAME.FLOAT)
   intermediate_format_ = [None, None, None]

   input = "51.11636937989,-114.03825348307,0"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== STATUS.SUCCESS
   assert std.get<float>(intermediate_format_[0].field_value) == approx(  51.11636937989f, rel=1e-6)
   assert std.get<float>(intermediate_format_[1].field_value) == approx(-114.03825348307f, abs=std.numeric_limits<float>.epsilon())
   assert std.get<float>(intermediate_format_[2].field_value) == approx(                0, rel=1e-6)

def test_ASCII_FLOAT_INVALID(decoder_tester):
   create_base_field("Und", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 5, DATA_TYPE_NAME.FLOAT)
   intermediate_format_ = [None]

   input = ""

   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_ASCII_DOUBLE_VALID(decoder_tester):
   create_base_field("Lat", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Long", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Ht", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("longitude", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE_NAME.DOUBLE)
   intermediate_format_ = [None] * 7

   input = "51.11636937989,-114.03825348307,0,1.7e+308,-1.7e+308,1.7e+309,-1.7e+309"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)
   constexpr double inf = std.numeric_limits<double>.infinity()

   assert decoder_== STATUS.SUCCESS
   assert std.get<double>(intermediate_format_[0].field_value) == approx(51.11636937989, rel=1e-15)
   assert std.get<double>(intermediate_format_[1].field_value) == approx(-114.03825348307, rel=1e-15)
   assert std.get<double>(intermediate_format_[2].field_value) == approx(0, rel=1e-15)
   assert std.get<double>(intermediate_format_[3].field_value) == approx(1.7e+308, rel=1e-15)
   assert std.get<double>(intermediate_format_[4].field_value) == approx(-1.7e+308, rel=1e-15)
   assert std.get<double>(intermediate_format_[5].field_value) == approx(inf, rel=1e-15)
   assert std.get<double>(intermediate_format_[6].field_value) == approx(-inf, rel=1e-15)

def test_ASCII_DOUBLE_INVALID(decoder_tester):
   create_base_field("Lat", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 9, DATA_TYPE_NAME.DOUBLE)
   intermediate_format_ = [None]

   input = ""

   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_ASCII_BOOL_VALID(decoder_tester):
   create_base_field("B_True", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE_NAME.BOOL)
   create_base_field("B_False", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE_NAME.BOOL)
   intermediate_format_ = [None, None]

   input = "TRUE,FALSE"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<bool>(intermediate_format_[0].field_value)
   assert not std.get<bool>(intermediate_format_[1].field_value)

def test_ASCII_BOOL_INVALID(decoder_tester):
   create_base_field("B_True", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE_NAME.BOOL)
   intermediate_format_ = [None]

   input = "True"

   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_ASCII_UINT_VALID(decoder_tester):
   create_base_field("toe", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE_NAME.UINT)
   create_base_field("toe", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE_NAME.UINT)
   create_base_field("toe", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE_NAME.UINT)
   create_base_field("toe", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE_NAME.UINT)
   intermediate_format_ = [None] * 4

   input = "-1,0,4294967294,4294967295"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<uint32_t>(intermediate_format_[0].field_value) == 4294967295
   assert std.get<uint32_t>(intermediate_format_[1].field_value) == 0
   assert std.get<uint32_t>(intermediate_format_[2].field_value) == 4294967294
   assert std.get<uint32_t>(intermediate_format_[3].field_value) == 4294967295

def test_ASCII_GPSTIME_MSEC_VALID(decoder_tester):
   create_base_field("Sec1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Sec2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Sec3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE_NAME.DOUBLE)
   intermediate_format_ = [None] * 4

   input = "-1.000,0.000,604800.000,4294967295.000"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   # If GPSTIME exceeds 4,294,967.295 (seconds) the conversion to milliseconds is wrong
   # But the limit should be 604,800 (seconds) as that's the number of seconds in a GPS reference week
   assert std.get<uint32_t>(intermediate_format_[0].field_value) == 4294966296  # 4,294,967,295 + 1 - 1,000 = 4,294,966,296
   assert std.get<uint32_t>(intermediate_format_[1].field_value) == 0
   assert std.get<uint32_t>(intermediate_format_[2].field_value) == 604800000
   assert std.get<uint32_t>(intermediate_format_[3].field_value) == 4294966296

def test_ASCII_SCIENTIFIC_NOTATION_FLOAT_VALID(decoder_tester):
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 4, DATA_TYPE_NAME.FLOAT)
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 4, DATA_TYPE_NAME.FLOAT)
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 4, DATA_TYPE_NAME.FLOAT)
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 4, DATA_TYPE_NAME.FLOAT)
   intermediate_format_ = [None] * 4

   input = "-1.0,0.0,1.175494351e-38,3.402823466e+38"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<float>(intermediate_format_[0].field_value) == approx(-1, rel=1e-6)
   assert std.get<float>(intermediate_format_[1].field_value) == approx( 0, rel=1e-6)
   assert std.get<float>(intermediate_format_[2].field_value) == approx(1.175494351e-38, rel=1e-6)
   assert std.get<float>(intermediate_format_[3].field_value) == approx(3.402823466e+38, rel=1e-6)

def test_ASCII_SCIENTIFIC_NOTATION_FLOAT_INVALID(decoder_tester):
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 5, DATA_TYPE_NAME.FLOAT)
   intermediate_format_ = [None] * 1

   input = "-1.0"

   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_ASCII_SCIENTIFIC_NOTATION_DOUBLE_VALID(decoder_tester):
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 8, DATA_TYPE_NAME.DOUBLE)
   intermediate_format_ = [None] * 4

   input = "-1.0,0.0,2.2250738585072014e-308,1.7976931348623158e+308"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<double>(intermediate_format_[0].field_value) == approx(-1, rel=1e-15)
   assert std.get<double>(intermediate_format_[1].field_value) == approx(0, rel=1e-15)
   assert std.get<double>(intermediate_format_[2].field_value) == approx(2.2250738585072014e-308, rel=1e-15)
   assert std.get<double>(intermediate_format_[3].field_value) == approx(1.7976931348623158e+308, rel=1e-15)

def test_ASCII_ULONG_VALID(decoder_tester):
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 1, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 1, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 1, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu, 2, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu, 2, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu, 2, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu, 4, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu, 4, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu, 4, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE_NAME.ULONG)
   create_base_field("rx_chars", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE_NAME.ULONG)
   intermediate_format_ = [None] * 12

   input = "-1,0,255,-1,0,65535,-1,0,4294967295,-1,0,18446744073709551615"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<uint8_t>(intermediate_format_[0].field_value) == 255
   assert std.get<uint8_t>(intermediate_format_[1].field_value) == 0
   assert std.get<uint8_t>(intermediate_format_[2].field_value) == 255
   assert std.get<uint16_t>(intermediate_format_[3].field_value) == 65535
   assert std.get<uint16_t>(intermediate_format_[4].field_value) == 0
   assert std.get<uint16_t>(intermediate_format_[5].field_value) == 65535
   assert std.get<uint32_t>(intermediate_format_[6].field_value) == 4294967295
   assert std.get<uint32_t>(intermediate_format_[7].field_value) == 0
   assert std.get<uint32_t>(intermediate_format_[8].field_value) == 4294967295
   assert std.get<uint64_t>(intermediate_format_[9].field_value) == ULLONG_MAX
   assert std.get<uint64_t>(intermediate_format_[10].field_value) == 0
   assert std.get<uint64_t>(intermediate_format_[11].field_value) == ULLONG_MAX

def test_ASCII_ENUM_VALID(decoder_tester):
   create_enum_field("UNKNOWN", "Unknown or unspecified type", 20)
   create_enum_field("APPROXIMATE", "Approximate time", 60)
   create_enum_field("SATTIME", "", 200)
   intermediate_format_ = [None] * 3

   input = "UNKNOWN,APPROXIMATE,SATTIME"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<int32_t>(intermediate_format_[0].field_value) == 20
   assert std.get<int32_t>(intermediate_format_[1].field_value) == 60
   assert std.get<int32_t>(intermediate_format_[2].field_value) == 200

def test_ASCII_STRING_VALID(decoder_tester):
   create_base_field("MESSAGE", FIELD_TYPE.STRING, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UNKNOWN)
   intermediate_format_ = [None]

   input = "#RAWEPHEMA,COM1,100"
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<std.string>(intermediate_format_[0].field_value) == "RAWEPHEMA,COM1,100"

def test_ASCII_EMPTY_STRING_VALID(decoder_tester):
   create_base_field("MESSAGE", FIELD_TYPE.STRING, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UNKNOWN)
   intermediate_format_ = [None]

   input = "\"\""
   decoder_= decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<std.string>(intermediate_format_[0].field_value) == ""

def test_ASCII_TYPE_INVALID(decoder_tester):
   create_base_field("", FIELD_TYPE.UNKNOWN, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UNKNOWN)
   intermediate_format_ = [None]

   input = ""

   with pytest.raises(Exception):
      decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format_)

def test_BINARY_BOOL_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.BOOL)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.BOOL)
   intermediate_format_ = [None, None]

   input = [True, False]
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<bool>(intermediate_format_[0].field_value)
   assert not std.get<bool>(intermediate_format_[1].field_value)

def test_BINARY_HEXBYTE_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.HEXBYTE)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.HEXBYTE)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.HEXBYTE)
   intermediate_format_ = [None, None, None]

   input = bytes([ 0x00, 0x01, 0xFF ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<uint8_t>(intermediate_format_[0].field_value) == 0
   assert std.get<uint8_t>(intermediate_format_[1].field_value) == 1
   assert std.get<uint8_t>(intermediate_format_[2].field_value) == UCHAR_MAX

def test_BINARY_uint8_t_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UCHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UCHAR)
   intermediate_format_ = [None] * 5

   input = bytes([ 0x23, 0x41, 0x3B, 0x00, 0xFF ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<uint8_t>(intermediate_format_[0].field_value) == '#'
   assert std.get<uint8_t>(intermediate_format_[1].field_value) == 'A'
   assert std.get<uint8_t>(intermediate_format_[2].field_value) == ';'
   assert std.get<uint8_t>(intermediate_format_[3].field_value) == 0
   assert std.get<uint8_t>(intermediate_format_[4].field_value) == UCHAR_MAX

def test_BINARY_USHORT_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.USHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.USHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.USHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.USHORT)
   intermediate_format_ = [None] * 4

   input = bytes([ 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0xFF, 0xFF ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<uint16_t>(intermediate_format_[0].field_value) == 0
   assert std.get<uint16_t>(intermediate_format_[1].field_value) == 1
   assert std.get<uint16_t>(intermediate_format_[2].field_value) == 16
   assert std.get<uint16_t>(intermediate_format_[3].field_value) == USHRT_MAX
def test_BINARY_SHORT_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.SHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.SHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.SHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE_NAME.SHORT)
   intermediate_format_ = [None] * 4

   input = bytes([ 0x00, 0x80, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x7F ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<int16_t>(intermediate_format_[0].field_value) == SHRT_MIN
   assert std.get<int16_t>(intermediate_format_[1].field_value) == -1
   assert std.get<int16_t>(intermediate_format_[2].field_value) == -0
   assert std.get<int16_t>(intermediate_format_[3].field_value) == SHRT_MAX
def test_BINARY_INT_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.INT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.INT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.INT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.INT)
   intermediate_format_ = [None] * 4

   input = bytes([
      0x00, 0x00, 0x00, 0x80,
      0x00, 0x00, 0xFF, 0xFF,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0x7F
   ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<int32_t>(intermediate_format_[0].field_value) == INT_MIN
   assert std.get<int32_t>(intermediate_format_[1].field_value) == -65536
   assert std.get<int32_t>(intermediate_format_[2].field_value) == 0
   assert std.get<int32_t>(intermediate_format_[3].field_value) == INT_MAX

def test_BINARY_UINT_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.UINT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.UINT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.UINT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.UINT)
   intermediate_format_ = [None] * 4

   input = bytes([
      0x00, 0x00, 0x00, 0x80,
      0xFF, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF
   ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<uint32_t>(intermediate_format_[0].field_value) == 2147483648
   assert std.get<uint32_t>(intermediate_format_[1].field_value) == 65535
   assert std.get<uint32_t>(intermediate_format_[2].field_value) == 0
   assert std.get<uint32_t>(intermediate_format_[3].field_value) == UINT_MAX

def test_BINARY_ULONG_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.ULONG)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.ULONG)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.ULONG)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.ULONG)
   intermediate_format_ = [None] * 4

   input = bytes([
      0x00, 0x00, 0x00, 0x80,
      0xFF, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0xFF, 0xFF, 0xFF, 0xFF
   ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<uint32_t>(intermediate_format_[0].field_value) == 2147483648
   assert std.get<uint32_t>(intermediate_format_[1].field_value) == 65535
   assert std.get<uint32_t>(intermediate_format_[2].field_value) == 0
   assert std.get<uint32_t>(intermediate_format_[3].field_value) == UINT_MAX

def test_BINARY_CHAR_BYTE_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.CHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.CHAR)
   intermediate_format_ = [None] * 4

   input = bytes([ 0x80, 0xFF, 0x00, 0x7F ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<int8_t>(intermediate_format_[0].field_value) == CHAR_MIN
   assert std.get<int8_t>(intermediate_format_[1].field_value) == -1
   assert std.get<int8_t>(intermediate_format_[2].field_value) == 0
   assert std.get<int8_t>(intermediate_format_[3].field_value) == CHAR_MAX

def test_BINARY_FLOAT_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.FLOAT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE_NAME.FLOAT)
   intermediate_format_ = [None] * 2

   input = bytes([ 0x9A, 0x99, 0x99, 0x3F, 0xCD, 0xCC, 0xBC, 0xC0 ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<float>(intermediate_format_[0].field_value) == approx(1.2, rel=1e-6)
   assert std.get<float>(intermediate_format_[1].field_value) == approx(-5.9, rel=1e-6)

def test_BINARY_DOUBLE_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE_NAME.DOUBLE)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE_NAME.DOUBLE)
   intermediate_format_ = [None] * 4

   input = bytes([
      0x12, 0x71, 0x1C, 0x31, 0xE5, 0x8E, 0x49, 0x40,
      0x99, 0xAF, 0xBC, 0xBE, 0x72, 0x82, 0x5C, 0xC0,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x7F
   ])
   decoder_= decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

   assert decoder_== ne.STATUS.SUCCESS
   assert std.get<double>(intermediate_format_[0].field_value) == approx(51.11636937989, rel=1e-15)
   assert std.get<double>(intermediate_format_[1].field_value) == approx(-114.03825348307, rel=1e-15)
   assert std.get<double>(intermediate_format_[2].field_value) == approx(0, rel=1e-15)
   assert std.get<double>(intermediate_format_[3].field_value) == approx(float("inf"), rel=1e-15)

def test_BINARY_SIMPLE_TYPE_INVALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UNKNOWN)
   intermediate_format_ = []

   input = None

   with pytest.raises(Exception):
      decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

def test_BINARY_TYPE_INVALID(decoder_tester):
   create_base_field("", FIELD_TYPE.UNKNOWN, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE_NAME.UNKNOWN)
   intermediate_format_ = []

   with pytest.raises(Exception):
      decoder_tester.test_decode_binary(MsgDefFields_, input, intermediate_format_)

def test_SIMPLE_FIELD_WIDTH_VALID(decoder_tester):
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d,   4, DATA_TYPE_NAME.BOOL)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.XB,  1, DATA_TYPE_NAME.HEXBYTE)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB,  1, DATA_TYPE_NAME.UCHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B,   1, DATA_TYPE_NAME.CHAR)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu,  2, DATA_TYPE_NAME.USHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hd,  2, DATA_TYPE_NAME.SHORT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u,   4, DATA_TYPE_NAME.UINT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu,  4, DATA_TYPE_NAME.ULONG)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d,   4, DATA_TYPE_NAME.INT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.ld,  4, DATA_TYPE_NAME.LONG)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE_NAME.ULONGLONG)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lld, 8, DATA_TYPE_NAME.LONGLONG)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f,   4, DATA_TYPE_NAME.FLOAT)
   create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf,  8, DATA_TYPE_NAME.DOUBLE)

   intermediate_format = ne.IntermediateMessage()
   intermediate_format.reserve(MsgDefFields_.size())

   input = "TRUE,0x63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03"
   assert decoder_tester.test_decode_ascii(MsgDefFields_, input, intermediate_format) == ne.STATUS.SUCCESS
   assert encoder_tester.test_encode_binary_body(intermediate_format, MAX_ASCII_MESSAGE_LENGTH)

   intermediate_format.clear()
   decoder_tester.test_decode_binary(MsgDefFields_, intermediate_format)

   sz = 0

   assert std.get<bool    >(intermediate_format.at(sz++).field_value) == True
   assert std.get<uint8_t >(intermediate_format.at(sz++).field_value) == 99
   assert std.get<uint8_t >(intermediate_format.at(sz++).field_value) == 227
   assert std.get<int8_t  >(intermediate_format.at(sz++).field_value) == 56
   assert std.get<uint16_t>(intermediate_format.at(sz++).field_value) == 2734
   assert std.get<int16_t >(intermediate_format.at(sz++).field_value) == -3842
   assert std.get<uint32_t>(intermediate_format.at(sz++).field_value) == 38283
   assert std.get<uint32_t>(intermediate_format.at(sz++).field_value) == 54244
   assert std.get<int32_t >(intermediate_format.at(sz++).field_value) == -4359
   assert std.get<int32_t >(intermediate_format.at(sz++).field_value) == 5293
   assert std.get<uint64_t>(intermediate_format.at(sz++).field_value) == 79338432
   assert std.get<int64_t >(intermediate_format.at(sz++).field_value) == -289834
   assert std.get<float   >(intermediate_format.at(sz++).field_value) == approx(2.54, abs=0.001)
   assert std.get<double  >(intermediate_format.at(sz++).field_value) == approx(5.44061788e+03, abs=0.000001)
