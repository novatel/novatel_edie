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
# \brief Unit tests for OEM Framer, Decoder,
# Encoder and Filter.
################################################################################

import novatel_edie as ne
import novatel_edie.oem as oem
import pytest
from novatel_edie import STATUS, ENCODE_FORMAT


@pytest.fixture(scope="function")
def fp(test_gps_file):
    return oem.FileParser(test_gps_file)


@pytest.fixture(scope="module")
def test_gps_file(decoders_test_resources):
    return decoders_test_resources / "BESTUTMBIN.GPS"


@pytest.mark.skip(reason="Slow and redundant")
def test_fileparser_instantiation(json_db, json_db_path):
    fp = oem.FileParser()
    fp.load_json_db(json_db)
    oem.FileParser(json_db_path)
    oem.FileParser(json_db)


def test_range_cmp(fp):
    fp.decompress_range_cmp = True
    assert fp.decompress_range_cmp
    fp.decompress_range_cmp = False
    assert not fp.decompress_range_cmp


def test_unknown_bytes(fp):
    fp.return_unknown_bytes = True
    assert fp.return_unknown_bytes
    fp.return_unknown_bytes = False
    assert not fp.return_unknown_bytes


def test_parse_file_with_filter(fp):
    fp.filter = oem.Filter()
    msgs = []
    while True:
        try:
            msg = fp.read()
            if isinstance(msg, oem.Message):
                msgs.append(msg)
        except ne.StreamEmptyException:
            break
    assert len(msgs) == 2

    assert msgs[0].header.milliseconds == pytest.approx(270605000)
    assert len(msgs[0].to_ascii().message) == 213

    assert msgs[1].header.milliseconds == pytest.approx(172189053)
    assert len(msgs[1].to_ascii().message) == 195


def test_file_parser_iterator(fp):
    fp.filter = oem.Filter()
    msgs = [msg for msg in fp if isinstance(msg, oem.Message)]
    assert len(msgs) == 2

    assert msgs[0].header.milliseconds == pytest.approx(270605000)
    assert len(msgs[0].to_ascii().message) == 213

    assert msgs[1].header.milliseconds == pytest.approx(172189053)
    assert len(msgs[1].to_ascii().message) == 195


# The file holds one VERSION log and one BESTUTM log, both in ASCII format.
# The name of the file says BIN, but the logs inside it are ASCII.
VERSION_ID = 37
BESTUTM_ID = 726


def expected_file_counts():
    return {
        (VERSION_ID, ne.HEADER_FORMAT.ASCII, 0): 1,
        (BESTUTM_ID, ne.HEADER_FORMAT.ASCII, 0): 1,
    }


def read_whole_file(fp):
    """Read every message in the file and return the messages."""
    msgs = []
    while True:
        try:
            msg = fp.read()
            if isinstance(msg, oem.Message):
                msgs.append(msg)
        except ne.StreamEmptyException:
            break
    return msgs


def test_message_counts_empty_before_reading(fp):
    """Tests that a new FileParser has no message counts."""
    assert fp.message_counts == {}


def test_message_counts_cover_the_whole_file(fp):
    """Tests that every message read from the file has a count."""
    # Act
    msgs = read_whole_file(fp)
    # Assert
    assert len(msgs) == 2
    assert fp.message_counts == expected_file_counts()
    assert sum(fp.message_counts.values()) == len(msgs)


def test_reset_clears_message_counts(fp):
    """Tests that a reset rewinds the file and starts new counts."""
    # Arrange
    read_whole_file(fp)
    assert fp.message_counts
    # Act
    fp.reset()
    # Assert
    assert fp.message_counts == {}
    # A second pass over the same file gives the same counts.
    assert len(read_whole_file(fp)) == 2
    assert fp.message_counts == expected_file_counts()


def test_reset_message_counts_keeps_the_file_position(fp):
    """Tests that clearing the counts does not rewind the file."""
    # Arrange
    read_whole_file(fp)
    # Act
    fp.reset_message_counts()
    # Assert
    assert fp.message_counts == {}
    assert read_whole_file(fp) == []
    assert fp.message_counts == {}
