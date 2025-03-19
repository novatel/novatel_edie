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
import pytest
from novatel_edie import STATUS, ENCODE_FORMAT


@pytest.fixture(scope="function")
def fp(test_gps_file):
    return ne.FileParser(test_gps_file)


@pytest.fixture(scope="module")
def test_gps_file(decoders_test_resources):
    return decoders_test_resources / "BESTUTMBIN.GPS"

@pytest.mark.skip(reason="Logging is still under development")
def test_logger(test_gps_file):
    # FileParser logger
    level = ne.LogLevel.OFF
    file_parser = ne.FileParser(test_gps_file)
    logger = file_parser.logger
    logger.set_level(level)
    assert logger.name == "novatel_file_parser"
    assert logger.level == level
    # Parser logger
    file_parser.enable_framer_decoder_logging(level, "novatel_parser.log")


@pytest.mark.skip(reason="Slow and redundant")
def test_fileparser_instantiation(json_db, json_db_path):
    fp = ne.FileParser()
    fp.load_json_db(json_db)
    ne.FileParser(json_db_path)
    ne.FileParser(json_db)


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
    fp.filter = ne.Filter()
    fp.filter.logger.set_level(ne.LogLevel.DEBUG)
    msgs = []
    while True:
        try:
            msg = fp.read()
            if isinstance(msg, ne.Message):
                msgs.append(msg)
        except ne.StreamEmptyException:
            break
    assert len(msgs) == 2

    assert msgs[0].header.milliseconds == pytest.approx(270605000)
    assert len(msgs[0].to_ascii().message) == 213

    assert msgs[1].header.milliseconds == pytest.approx(172189053)
    assert len(msgs[1].to_ascii().message) == 195


def test_file_parser_iterator(fp):
    fp.filter = ne.Filter()
    fp.filter.logger.set_level(ne.LogLevel.DEBUG)
    msgs = [msg for msg in fp if isinstance(msg, ne.Message)]
    assert len(msgs) == 2

    assert msgs[0].header.milliseconds == pytest.approx(270605000)
    assert len(msgs[0].to_ascii().message) == 213

    assert msgs[1].header.milliseconds == pytest.approx(172189053)
    assert len(msgs[1].to_ascii().message) == 195


def test_reset(fp):
    assert len(fp.internal_buffer) > 0
    assert fp.reset()
