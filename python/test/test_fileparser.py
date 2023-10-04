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
from novatel_edie import STATUS, ENCODEFORMAT
from pytest import approx


# -------------------------------------------------------------------------------------------------------
# Unit Tests
# -------------------------------------------------------------------------------------------------------

@pytest.fixture(scope="function")
def fp(json_db):
    return ne.FileParser(json_db)


def test_FILEPARSER_INSTANTIATION(fp, json_db_path):
    fp1 = ne.FileParser()
    fp2 = ne.FileParser(json_db_path)

    db = ne.JsonReader()
    db.load_file(json_db_path)
    fp4 = ne.FileParser(db)


def test_RANGE_CMP(fp):
    fp.set_decompress_range_cmp(True)
    assert fp.get_decompress_range_cmp()
    fp.set_decompress_range_cmp(False)
    assert not fp.get_decompress_range_cmp()


def test_UNKNOWN_BYTES(fp):
    fp.set_return_unknown_bytes(True)
    assert fp.get_return_unknown_bytes()
    fp.set_return_unknown_bytes(False)
    assert not fp.get_return_unknown_bytes()


def test_PARSE_FILE_WITH_FILTER(fp, json_db_path, decoders_test_resources):
    # Reset the with the database because a previous test assigns it to the nullptr
    fp = ne.ne.FileParser(json_db_path)
    filter = ne.Filter()
    filter.logger.set_level(ne.LogLevel.DEBUG)
    fp.set_filter(filter)
    assert fp.get_filter() == filter

    test_gps_file = decoders_test_resources / "BESTUTMBIN.GPS"
    input_file_stream = ne.InputFileStream(str(test_gps_file))
    assert fp.set_stream(input_file_stream)

    meta_data = ne.MetaData()

    success = 0
    expected_meta_data_length = [213, 195]
    expected_milliseconds = [270605000, 172189053]
    expected_message_length = [213, 195]

    status = STATUS.UNKNOWN
    fp.set_encode_format(ENCODEFORMAT.ASCII)
    assert fp.get_encode_format() == ENCODEFORMAT.ASCII

    while status != STATUS.STREAM_EMPTY:
        status = fp.read(message_data, meta_data)
        if status == STATUS.SUCCESS:
            assert meta_data.length == expected_meta_data_length[success]
            assert meta_data.milliseconds == approx(expected_milliseconds[success])
            assert message_data.message_length == expected_message_length[success]
            success += 1
    assert fp.get_percent_read() == 100
    assert success == 2


def test_RESET(fp):
    fp = ne.FileParser()
    fp.get_internal_buffer()
    assert fp.reset()
