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
import novatel_edie.oem as oem
import pytest
from novatel_edie import STATUS, ENCODE_FORMAT


@pytest.fixture(scope="function")
def parser():
    return oem.Parser()


@pytest.fixture(scope="module")
def test_gps_file(decoders_test_resources):
    return decoders_test_resources / "BESTUTMBIN.GPS"


@pytest.mark.skip(reason="Slow and redundant")
def test_parser_instantiation(json_db, json_db_path):
    parser = oem.Parser()
    parser.load_json_db(json_db)
    oem.Parser(json_db_path)
    oem.Parser(json_db)


def test_range_cmp(parser):
    parser.decompress_range_cmp = True
    assert parser.decompress_range_cmp
    parser.decompress_range_cmp = False
    assert not parser.decompress_range_cmp


def test_unknown_bytes(parser):
    parser.return_unknown_bytes = True
    assert parser.return_unknown_bytes
    parser.return_unknown_bytes = False
    assert not parser.return_unknown_bytes


def test_parse_file_with_filter(parser, test_gps_file):
    parser.filter = oem.Filter()
    msgs = []
    with test_gps_file.open("rb") as f:
        while chunk := f.read(32):
            parser.write(chunk)
            msgs.extend([msg for msg in parser if isinstance(msg, oem.Message)])

    assert len(msgs) == 2

    assert msgs[0].header.milliseconds == pytest.approx(270605000)
    assert len(msgs[0].to_ascii().message) == 213

    assert msgs[1].header.milliseconds == pytest.approx(172189053)
    assert len(msgs[1].to_ascii().message) == 195
    assert parser.flush(return_flushed_bytes=True) == b""


@pytest.mark.parametrize("ignore_responses", [True, False])
@pytest.mark.parametrize("response_str, context", [("OK", b"\r\n<OK\r\nfdfa")])
def test_parse_abbrev_ascii_resp(response_str, context, ignore_responses, parser):
    # Arrange
    parser.ignore_abbreviated_ascii_responses = ignore_responses
    permutations = [(context[:i], context[i:]) for i in range(len(context) + 1)]
    msg_sets = []

    # Act
    for part1, part2 in permutations:
        parser.write(part1)
        msgs = [msg for msg in parser]
        parser.write(part2)
        new_msgs = [msg for msg in parser]
        msgs.extend(new_msgs)
        msg_sets.append(msgs)

    # Assert
    for i, msgs in enumerate(msg_sets):
        responses = [msg for msg in msgs if isinstance(msg, oem.Response)]
        try:
            if ignore_responses:
                assert len(responses) == 0
            else:
                assert responses[0].response_string == response_str
        except AssertionError as e:
            raise AssertionError(
                f"Failure at permutation {permutations[i]}: {e}"
            ) from e


def test_write_max_num_bytes(parser: oem.Parser):
    """Tests that data with length matching available space can be written."""
    # Arrange
    data = b"a" * parser.available_space
    # Act
    bytes_written = parser.write(data)
    # Assert
    assert bytes_written == len(data)


def test_write_exceeding_max_num_bytes(parser: oem.Parser):
    """Tests that data exceeding available space is not fully written.

    Whether data is partially written is not defined in the spec.
    """
    # Arrange
    data = b"a" * (parser.available_space + 1)
    # Act
    bytes_written = parser.write(data)
    # Assert
    assert bytes_written <= parser.available_space


def test_nmea_parsed_as_unknown_bytes(parser: oem.Parser):
    """Tests that unrecognized NMEA sentences are returned as UnknownBytes with NMEA reason."""
    # Arrange
    parser.return_unknown_bytes = True
    parser.write(b"$GPHDT,265.1253,T*01\r\n")

    # Act
    msgs = list(parser)

    # Assert
    assert len(msgs) == 1
    assert isinstance(msgs[0], ne.UnknownBytes)
    assert msgs[0].reason == ne.UNKNOWN_REASON.NMEA
    assert msgs[0].data == b"$GPHDT,265.1253,T*01\r\n"


BESTPOS_ASCII = (
    b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;"
    b"SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,"
    b'WGS84,1.3648,1.1806,3.1112,"",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n'
)
BESTPOS_ID = 42


def test_message_counts_empty_before_parsing(parser: oem.Parser):
    """Tests that a new Parser has no message counts."""
    assert parser.message_counts == {}


def test_message_counts_one_key_per_message(parser: oem.Parser):
    """Tests that each decoded message increments one count."""
    # Arrange
    parser.write(BESTPOS_ASCII * 3)
    # Act
    msgs = [msg for msg in parser if isinstance(msg, oem.Message)]
    # Assert
    assert len(msgs) == 3
    assert parser.message_counts == {(BESTPOS_ID, ne.DECODE_FORMAT.ASCII, 0): 3}


def test_message_counts_key_holds_the_format(parser: oem.Parser):
    """Tests that one message in two formats gives two counts."""
    # Arrange
    abbrev_ascii = (
        b"<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
        b"<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 "
        b'-17.0000 WGS84 0.9038 0.8534 1.7480 "" 0.000 0.000 35 30 30 30 00 06 39 33\r\n'
    )
    # An abbreviated ASCII message has no terminating delimiter. The framer knows
    # the message is complete only when data follows it, or when the Parser is
    # flushed. The trailing prompt is that following data.
    parser.write(BESTPOS_ASCII + abbrev_ascii + b"[COM1]")
    # Act
    list(parser)
    # Assert
    assert parser.message_counts == {
        (BESTPOS_ID, ne.DECODE_FORMAT.ASCII, 0): 1,
        (BESTPOS_ID, ne.DECODE_FORMAT.ABB_ASCII, 0): 1,
    }


def test_message_counts_keys_match_filter_message_ids(parser: oem.Parser):
    """Tests that a count key can be used directly against Filter.message_ids."""
    # Arrange
    filter = oem.Filter()
    filter.add_message_id(BESTPOS_ID, ne.DECODE_FORMAT.ASCII, 0)
    parser.write(BESTPOS_ASCII)
    # Act
    list(parser)
    # Assert
    assert set(parser.message_counts) == set(filter.message_ids)


def test_reset_message_counts(parser: oem.Parser):
    """Tests that resetting the counts sets them all back to 0."""
    # Arrange
    parser.write(BESTPOS_ASCII)
    list(parser)
    assert parser.message_counts
    # Act
    parser.reset_message_counts()
    # Assert
    assert parser.message_counts == {}
    # Counting starts again from 0.
    parser.write(BESTPOS_ASCII)
    list(parser)
    assert parser.message_counts == {(BESTPOS_ID, ne.DECODE_FORMAT.ASCII, 0): 1}
