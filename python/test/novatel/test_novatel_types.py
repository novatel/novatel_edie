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
import pytest
from novatel_edie import STATUS, FIELD_TYPE, CONVERSION_STRING, DATA_TYPE
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
LONG_MIN = -2147483647
LONG_MAX = 2147483647
ULONG_MAX = 4294967295
LLONG_MIN = -9223372036854775807
LLONG_MAX = 9223372036854775807
ULLONG_MAX = 18446744073709551615


class TestHelper:

    def __init__(self):
        json_db = ne.JsonReader()
        json_db.parse_json(min_json_db)
        self.message_decoder = ne.MessageDecoder(json_db)
        self.encoder = ne.Encoder(json_db)
        self.msg_def_fields = []

    def create_base_field(self, name, field_type, conversion_stripped, length, DATA_TYPE):
        field = ne.BaseField()
        field.name = name
        field.type = field_type
        field.conversion_stripped = conversion_stripped
        field.data_type.length = length
        field.data_type.name = DATA_TYPE
        self.msg_def_fields.append(field)

    def create_enum_field(self, name, description, value):
        dt = ne.EnumDataType()
        dt.name = name
        dt.description = description
        dt.value = value
        field = ne.EnumField()
        field.name = name
        field.type = FIELD_TYPE.ENUM
        field.enum_def = ne.EnumDefinition()
        field.enum_def.enumerators.append(dt)
        self.msg_def_fields.append(field)

    def test_decode_ascii(self, msg_def_fields, data):
        return self.message_decoder._decode_ascii(msg_def_fields, data)

    def test_decode_binary(self, msg_def_fields, data):
        fields_size = sum(field.data_type.length for field in msg_def_fields)
        return self.message_decoder._decode_binary(msg_def_fields, data, fields_size)


@pytest.fixture(scope="function")
def helper():
    return TestHelper()


def test_FIELD_CONTAINER_ERROR_ON_COPY(helper):
    fc = ne.FieldContainer(3, ne.BaseField())
    with pytest.raises(Exception):
        ne.FieldContainer(fc)


def test_ASCII_CHAR_BYTE_VALID(helper):
    helper.create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_5", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_6", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_7", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_8", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("INT_9", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)

    input = b"-129,-128,-127,-1,0,1,127,128,129"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == STATUS.SUCCESS
    assert intermediate_format.INT_1 == 127
    assert intermediate_format.INT_2 == -128
    assert intermediate_format.INT_3 == -127
    assert intermediate_format.INT_4 == -1
    assert intermediate_format.INT_5 == 0
    assert intermediate_format.INT_6 == 1
    assert intermediate_format.INT_7 == 127
    assert intermediate_format.INT_8 == -128
    assert intermediate_format.INT_9 == -127


def test_ASCI_UCHAR_BYTE_VALID(helper):
    helper.create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_5", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_6", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_7", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_8", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_9", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)

    input = b"-256,-255,-254,-1,0,1,254,255,256"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == STATUS.SUCCESS
    assert intermediate_format.INT_1 == 0
    assert intermediate_format.INT_2 == 1
    assert intermediate_format.INT_3 == 2
    assert intermediate_format.INT_4 == 255
    assert intermediate_format.INT_5 == 0
    assert intermediate_format.INT_6 == 1
    assert intermediate_format.INT_7 == 254
    assert intermediate_format.INT_8 == 255
    assert intermediate_format.INT_9 == 0


def test_ASCII_UCHAR_BYTE_INVALID(helper):
    helper.create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    input = b"0.1"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)
    assert intermediate_format.INT_1 == 0


def test_ASCII_CHAR_VALID(helper):
    helper.create_base_field("CHAR_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE.CHAR)
    helper.create_base_field("CHAR_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE.CHAR)
    helper.create_base_field("CHAR_3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 1, DATA_TYPE.CHAR)

    input = b"#,A,;"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == STATUS.SUCCESS
    assert intermediate_format.CHAR_1 == ord('#')
    assert intermediate_format.CHAR_2 == ord('A')
    assert intermediate_format.CHAR_3 == ord(';')


def test_ASCII_CHAR_INVALID(helper):
    helper.create_base_field("CHAR", FIELD_TYPE.SIMPLE, CONVERSION_STRING.c, 2, DATA_TYPE.CHAR)
    input = b""
    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_ASCII_UCHAR_VALID(helper):
    helper.create_base_field("uint8_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("uint8_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("uint8_3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 1, DATA_TYPE.UCHAR)

    input = b"#,A,;"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == STATUS.SUCCESS
    assert intermediate_format.uint8_1 == ord('#')
    assert intermediate_format.uint8_2 == ord('A')
    assert intermediate_format.uint8_3 == ord(';')


def test_ASCII_UCHAR_INVALID(helper):
    helper.create_base_field("uint8_t", FIELD_TYPE.SIMPLE, CONVERSION_STRING.uc, 2, DATA_TYPE.UCHAR)
    input = b""
    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_ASCII_INT_VALID(helper):
    helper.create_base_field("INT_1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_5", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE.UCHAR)
    helper.create_base_field("INT_6", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 2, DATA_TYPE.UCHAR)

    input = b"-32769,-32768,-32767,32767,32768,32769"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == STATUS.SUCCESS
    assert intermediate_format.INT_1 == 32767
    assert intermediate_format.INT_2 == -32768
    assert intermediate_format.INT_3 == -32767
    assert intermediate_format.INT_4 == 32767
    assert intermediate_format.INT_5 == -32768
    assert intermediate_format.INT_6 == -32767


def test_ASCII_INT_INVALID(helper):
    helper.create_base_field("INT_2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 3, DATA_TYPE.UCHAR)
    input = b""
    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_ASCII_FLOAT_VALID(helper):
    helper.create_base_field("Und", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE.FLOAT)
    helper.create_base_field("LatStd", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE.FLOAT)
    helper.create_base_field("LongStd", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE.FLOAT)

    input = b"51.11636937989,-114.03825348307,0"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == STATUS.SUCCESS
    assert intermediate_format.Und == approx(51.11636937989, rel=1e-6)
    assert intermediate_format.LatStd == approx(-114.03825348307, rel=1e-6)
    assert intermediate_format.LongStd == approx(0, rel=1e-6)


def test_ASCII_FLOAT_INVALID(helper):
    helper.create_base_field("Und", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 5, DATA_TYPE.FLOAT)
    input = b""
    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_ASCII_DOUBLE_VALID(helper):
    helper.create_base_field("Lat", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("Long", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("Ht", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("longitude1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("longitude2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("longitude3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("longitude4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)

    input = b"51.11636937989,-114.03825348307,0,1.7e+308,-1.7e+308,1.7e+309,-1.7e+309"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)
    inf = float("inf")

    assert status == STATUS.SUCCESS
    assert intermediate_format.Lat == approx(51.11636937989, rel=1e-15)
    assert intermediate_format.Long == approx(-114.03825348307, rel=1e-15)
    assert intermediate_format.Ht == approx(0, rel=1e-15)
    assert intermediate_format.longitude1 == approx(1.7e+308, rel=1e-15)
    assert intermediate_format.longitude2 == approx(-1.7e+308, rel=1e-15)
    assert intermediate_format.longitude3 == approx(inf, rel=1e-15)
    assert intermediate_format.longitude4 == approx(-inf, rel=1e-15)


def test_ASCII_DOUBLE_INVALID(helper):
    helper.create_base_field("Lat", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 9, DATA_TYPE.DOUBLE)
    input = b""
    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_ASCII_BOOL_VALID(helper):
    helper.create_base_field("B_True", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE.BOOL)
    helper.create_base_field("B_False", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE.BOOL)

    input = b"TRUE,FALSE"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert bool(intermediate_format.B_True)
    assert not bool(intermediate_format.B_False)


def test_ASCII_BOOL_INVALID(helper):
    helper.create_base_field("B_True", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE.BOOL)
    input = b"True"
    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_ASCII_UINT_VALID(helper):
    helper.create_base_field("toe1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE.UINT)
    helper.create_base_field("toe2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE.UINT)
    helper.create_base_field("toe3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE.UINT)
    helper.create_base_field("toe4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE.UINT)

    input = b"-1,0,4294967294,4294967295"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.toe1 == 4294967295
    assert intermediate_format.toe2 == 0
    assert intermediate_format.toe3 == 4294967294
    assert intermediate_format.toe4 == 4294967295


def test_ASCII_GPSTIME_MSEC_VALID(helper):
    helper.create_base_field("Sec1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE.DOUBLE)
    helper.create_base_field("Sec2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE.DOUBLE)
    helper.create_base_field("Sec3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE.DOUBLE)
    helper.create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.T, 4, DATA_TYPE.DOUBLE)

    input = b"-1.000,0.000,604800.000,4294967295.000"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    # If GPSTIME exceeds 4,294,967.295 (seconds) the conversion to milliseconds is wrong
    # But the limit should be 604,800 (seconds) as that's the number of seconds in a GPS reference week
    assert intermediate_format.Sec1 == 4294966296  # 4,294,967,295 + 1 - 1,000 = 4,294,966,296
    assert intermediate_format.Sec2 == 0
    assert intermediate_format.Sec3 == 604800000
    assert intermediate_format.Sec4 == 4294966296


def test_ASCII_SCIENTIFIC_NOTATION_FLOAT_VALID(helper):
    helper.create_base_field("Sec1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 4, DATA_TYPE.FLOAT)
    helper.create_base_field("Sec2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 4, DATA_TYPE.FLOAT)
    helper.create_base_field("Sec3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 4, DATA_TYPE.FLOAT)
    helper.create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 4, DATA_TYPE.FLOAT)

    input = b"-1.0,0.0,1.175494351e-38,3.402823466e+38"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.Sec1 == approx(-1, rel=1e-6)
    assert intermediate_format.Sec2 == approx(0, rel=1e-6)
    assert intermediate_format.Sec3 == approx(1.175494351e-38, rel=1e-6)
    assert intermediate_format.Sec4 == approx(3.402823466e+38, rel=1e-6)


def test_ASCII_SCIENTIFIC_NOTATION_FLOAT_INVALID(helper):
    helper.create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 5, DATA_TYPE.FLOAT)
    input = b"-1.0"
    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_ASCII_SCIENTIFIC_NOTATION_DOUBLE_VALID(helper):
    helper.create_base_field("Sec1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("Sec2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("Sec3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.g, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("Sec4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.e, 8, DATA_TYPE.DOUBLE)

    input = b"-1.0,0.0,2.2250738585072014e-308,1.7976931348623158e+308"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.Sec1 == approx(-1, rel=1e-15)
    assert intermediate_format.Sec2 == approx(0, rel=1e-15)
    assert intermediate_format.Sec3 == approx(2.2250738585072014e-308, rel=1e-15)
    assert intermediate_format.Sec4 == approx(1.7976931348623158e+308, rel=1e-15)


def test_ASCII_ULONG_VALID(helper):
    helper.create_base_field("rx_chars1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 1, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 1, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 1, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu, 2, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars5", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu, 2, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars6", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu, 2, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars7", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu, 4, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars8", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu, 4, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars9", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu, 4, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars10", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars11", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE.ULONG)
    helper.create_base_field("rx_chars12", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE.ULONG)

    input = b"-1,0,255,-1,0,65535,-1,0,4294967295,-1,0,18446744073709551615"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.rx_chars1 == 255
    assert intermediate_format.rx_chars2 == 0
    assert intermediate_format.rx_chars3 == 255
    assert intermediate_format.rx_chars4 == 65535
    assert intermediate_format.rx_chars5 == 0
    assert intermediate_format.rx_chars6 == 65535
    assert intermediate_format.rx_chars7 == 4294967295
    assert intermediate_format.rx_chars8 == 0
    assert intermediate_format.rx_chars9 == 4294967295
    assert intermediate_format.rx_chars10 == ULLONG_MAX
    assert intermediate_format.rx_chars11 == 0
    assert intermediate_format.rx_chars12 == ULLONG_MAX


def test_ASCII_ENUM_VALID(helper):
    helper.create_enum_field("UNKNOWN", "Unknown or unspecified type", 20)
    helper.create_enum_field("APPROXIMATE", "Approximate time", 60)
    helper.create_enum_field("SATTIME", "", 200)

    input = b"UNKNOWN,APPROXIMATE,SATTIME"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.UNKNOWN == 20
    assert intermediate_format.APPROXIMATE == 60
    assert intermediate_format.SATTIME == 200


def test_ASCII_STRING_VALID(helper):
    helper.create_base_field("MESSAGE", FIELD_TYPE.STRING, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UNKNOWN)

    input = b"#RAWEPHEMA,COM1,100"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.MESSAGE == "RAWEPHEMA,COM1,100"


def test_ASCII_EMPTY_STRING_VALID(helper):
    helper.create_base_field("MESSAGE", FIELD_TYPE.STRING, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UNKNOWN)

    input = b"\"\""
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.MESSAGE == ""


def test_ASCII_TYPE_INVALID(helper):
    helper.create_base_field("", FIELD_TYPE.UNKNOWN, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UNKNOWN)

    input = b""

    with pytest.raises(Exception):
        helper.test_decode_ascii(helper.msg_def_fields, input)


def test_BINARY_BOOL_VALID(helper):
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.BOOL)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.BOOL)

    input = bytes([1, 0])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert bool(intermediate_format.field1)
    assert not bool(intermediate_format.field2)


def test_BINARY_HEXBYTE_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.HEXBYTE)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.HEXBYTE)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.HEXBYTE)

    input = bytes([0x00, 0x01, 0xFF])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == 0
    assert intermediate_format.field1 == 1
    assert intermediate_format.field2 == UCHAR_MAX


def test_BINARY_uint8_t_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("field4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UCHAR)

    input = bytes([0x23, 0x41, 0x3B, 0x00, 0xFF])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == ord('#')
    assert intermediate_format.field1 == ord('A')
    assert intermediate_format.field2 == ord(';')
    assert intermediate_format.field3 == 0
    assert intermediate_format.field4 == UCHAR_MAX


def test_BINARY_USHORT_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.USHORT)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.USHORT)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.USHORT)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.USHORT)

    input = bytes([0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0xFF, 0xFF])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == 0
    assert intermediate_format.field1 == 1
    assert intermediate_format.field2 == 16
    assert intermediate_format.field3 == USHRT_MAX


def test_BINARY_SHORT_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.SHORT)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.SHORT)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.SHORT)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 2, DATA_TYPE.SHORT)

    input = bytes([0x00, 0x80, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x7F])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == SHRT_MIN
    assert intermediate_format.field1 == -1
    assert intermediate_format.field2 == -0
    assert intermediate_format.field3 == SHRT_MAX


def test_BINARY_INT_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.INT)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.INT)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.INT)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.INT)

    input = bytes([
        0x00, 0x00, 0x00, 0x80,
        0x00, 0x00, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0x7F
    ])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == LONG_MIN
    assert intermediate_format.field1 == -65536
    assert intermediate_format.field2 == 0
    assert intermediate_format.field3 == LONG_MAX


def test_BINARY_UINT_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.UINT)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.UINT)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.UINT)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.UINT)

    input = bytes([
        0x00, 0x00, 0x00, 0x80,
        0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF
    ])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == 2147483648
    assert intermediate_format.field1 == 65535
    assert intermediate_format.field2 == 0
    assert intermediate_format.field3 == ULONG_MAX


def test_BINARY_ULONG_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.ULONG)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.ULONG)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.ULONG)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.ULONG)

    input = bytes([
        0x00, 0x00, 0x00, 0x80,
        0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF
    ])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == 2147483648
    assert intermediate_format.field1 == 65535
    assert intermediate_format.field2 == 0
    assert intermediate_format.field3 == ULONG_MAX


def test_BINARY_CHAR_BYTE_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.CHAR)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.CHAR)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.CHAR)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.CHAR)

    input = bytes([0x80, 0xFF, 0x00, 0x7F])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == CHAR_MIN
    assert intermediate_format.field1 == -1
    assert intermediate_format.field2 == 0
    assert intermediate_format.field3 == CHAR_MAX


def test_BINARY_FLOAT_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.FLOAT)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 4, DATA_TYPE.FLOAT)

    input = bytes([0x9A, 0x99, 0x99, 0x3F, 0xCD, 0xCC, 0xBC, 0xC0])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == approx(1.2, rel=1e-6)
    assert intermediate_format.field1 == approx(-5.9, rel=1e-6)


def test_BINARY_DOUBLE_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE.DOUBLE)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 8, DATA_TYPE.DOUBLE)

    input = bytes([
        0x12, 0x71, 0x1C, 0x31, 0xE5, 0x8E, 0x49, 0x40,
        0x99, 0xAF, 0xBC, 0xBE, 0x72, 0x82, 0x5C, 0xC0,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x7F
    ])
    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert status == ne.STATUS.SUCCESS
    assert intermediate_format.field0 == approx(51.11636937989, rel=1e-15)
    assert intermediate_format.field1 == approx(-114.03825348307, rel=1e-15)
    assert intermediate_format.field2 == approx(0, rel=1e-15)
    assert intermediate_format.field3 == approx(float("inf"), rel=1e-15)


def test_BINARY_SIMPLE_TYPE_INVALID(helper):
    helper.create_base_field("", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UNKNOWN)
    input = None
    with pytest.raises(Exception):
        helper.test_decode_binary(helper.msg_def_fields, input)


def test_BINARY_TYPE_INVALID(helper):
    helper.create_base_field("", FIELD_TYPE.UNKNOWN, CONVERSION_STRING.UNKNOWN, 1, DATA_TYPE.UNKNOWN)
    with pytest.raises(Exception):
        helper.test_decode_binary(helper.msg_def_fields, input)


def test_SIMPLE_FIELD_WIDTH_VALID(helper):
    helper.create_base_field("field0", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE.BOOL)
    helper.create_base_field("field1", FIELD_TYPE.SIMPLE, CONVERSION_STRING.XB, 1, DATA_TYPE.HEXBYTE)
    helper.create_base_field("field2", FIELD_TYPE.SIMPLE, CONVERSION_STRING.UB, 1, DATA_TYPE.UCHAR)
    helper.create_base_field("field3", FIELD_TYPE.SIMPLE, CONVERSION_STRING.B, 1, DATA_TYPE.CHAR)
    helper.create_base_field("field4", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hu, 2, DATA_TYPE.USHORT)
    helper.create_base_field("field5", FIELD_TYPE.SIMPLE, CONVERSION_STRING.hd, 2, DATA_TYPE.SHORT)
    helper.create_base_field("field6", FIELD_TYPE.SIMPLE, CONVERSION_STRING.u, 4, DATA_TYPE.UINT)
    helper.create_base_field("field7", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lu, 4, DATA_TYPE.ULONG)
    helper.create_base_field("field8", FIELD_TYPE.SIMPLE, CONVERSION_STRING.d, 4, DATA_TYPE.INT)
    helper.create_base_field("field9", FIELD_TYPE.SIMPLE, CONVERSION_STRING.ld, 4, DATA_TYPE.LONG)
    helper.create_base_field("field10", FIELD_TYPE.SIMPLE, CONVERSION_STRING.llu, 8, DATA_TYPE.ULONGLONG)
    helper.create_base_field("field11", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lld, 8, DATA_TYPE.LONGLONG)
    helper.create_base_field("field12", FIELD_TYPE.SIMPLE, CONVERSION_STRING.f, 4, DATA_TYPE.FLOAT)
    helper.create_base_field("field13", FIELD_TYPE.SIMPLE, CONVERSION_STRING.lf, 8, DATA_TYPE.DOUBLE)

    input = b"TRUE,0x63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03"
    status, intermediate_format = helper.test_decode_ascii(helper.msg_def_fields, input)
    assert status == STATUS.SUCCESS
    # assert encoder_tester.test_encode_binary_body(intermediate_format, MAX_ASCII_MESSAGE_LENGTH)

    status, intermediate_format = helper.test_decode_binary(helper.msg_def_fields, input)

    assert intermediate_format.field0 == True  # bool
    assert intermediate_format.field1 == 99  # uint8_t
    assert intermediate_format.field2 == 227  # uint8_t
    assert intermediate_format.field3 == 56  # int8_t
    assert intermediate_format.field4 == 2734  # uint16_t
    assert intermediate_format.field5 == -3842  # int16_t
    assert intermediate_format.field6 == 38283  # uint32_t
    assert intermediate_format.field7 == 54244  # uint32_t
    assert intermediate_format.field8 == -4359  # int32_t
    assert intermediate_format.field9 == 5293  # int32_t
    assert intermediate_format.field10 == 79338432  # uint64_t
    assert intermediate_format.field11 == -289834  # int64_t
    assert intermediate_format.field12 == approx(2.54, abs=0.001)  # float
    assert intermediate_format.field13 == approx(5.44061788e+03, abs=0.000001)  # double
