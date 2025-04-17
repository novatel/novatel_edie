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
from novatel_edie import STATUS, ENCODE_FORMAT


@pytest.fixture(scope="function")
def parser():
    return ne.Parser()

@pytest.fixture(scope="module")
def test_gps_file(decoders_test_resources):
    return decoders_test_resources / "BESTUTMBIN.GPS"


@pytest.mark.skip(reason="Slow and redundant")
def test_parser_instantiation(json_db, json_db_path):
    parser = ne.Parser()
    parser.load_json_db(json_db)
    ne.Parser(json_db_path)
    ne.Parser(json_db)


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
    parser.filter = ne.Filter()
    msgs = []
    with test_gps_file.open("rb") as f:
        while chunk := f.read(32):
            parser.write(chunk)
            msgs.extend([msg for msg in parser if isinstance(msg, ne.Message)])


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
        responses = [msg for msg in msgs if isinstance(msg, ne.Response)]
        try:
            if ignore_responses:
                assert len(responses) == 0
            else:
                assert responses[0].response_str == response_str
        except AssertionError as e:
            raise AssertionError(
                f"Failure at permutation {permutations[i]}: {e}") from e
