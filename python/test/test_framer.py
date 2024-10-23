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
#                                     DESCRIPTION
#
# \file novateltest.hpp
# \brief Unit tests for OEM helper.framer, HeaderDecoder, MessageDecoder,
# Encoder and Filter.
################################################################################

import novatel_edie as ne
import pytest
from novatel_edie import HEADER_FORMAT, STATUS


class Helper:
    def __init__(self, test_resources_path):
        self.test_resources = test_resources_path
        self.framer = ne.Framer()
        self.framer.set_report_unknown_bytes(True)
        self.framer.set_payload_only(False)

    def test_framer(self, expected_header_format, expected_status, length, buffer_length=None):
        expected_meta_data = ne.MetaData()
        if buffer_length is None:
            status, frame, test_meta_data = self.framer.get_frame()
        else:
            test_meta_data = ne.MetaData()
            status, frame = self.framer.get_frame(test_meta_data, buffer_length)
        assert status == expected_status
        if length is not None:
            expected_meta_data.length = length
        expected_meta_data.format = expected_header_format
        compare_metadata(test_meta_data, expected_meta_data, ignore_length=length is None)

    def write_file_to_framer(self, filename):
        data = (self.test_resources / filename).read_bytes()
        bytes_written = self.framer.write(data)
        assert bytes_written == len(data)

    def write_bytes_to_framer(self, data):
        assert self.framer.write(data) == len(data)


@pytest.fixture(scope="function")
def helper(decoders_test_resources):
    return Helper(decoders_test_resources)


def compare_metadata(test_md, expected_md, ignore_length=False):
    assert test_md.format == expected_md.format, "MetaData format mismatch"
    assert test_md.measurement_source == expected_md.measurement_source, "MetaData measurement_source mismatch"
    assert test_md.time_status == expected_md.time_status, "MetaData time_status mismatch"
    assert test_md.response == expected_md.response, "MetaData response mismatch"
    assert test_md.week == expected_md.week, "MetaData week mismatch"
    assert test_md.milliseconds == expected_md.milliseconds, "MetaData milliseconds mismatch"
    assert test_md.binary_msg_length == expected_md.binary_msg_length, "MetaData binary_msg_length mismatch"
    if not ignore_length:
        assert test_md.length == expected_md.length, "MetaData length mismatch"
    assert test_md.header_length == expected_md.header_length, "MetaData header_length mismatch"
    assert test_md.message_id == expected_md.message_id, "MetaData message_id mismatch"
    assert test_md.message_crc == expected_md.message_crc, "MetaData message_crc mismatch"
    assert test_md.message_name == expected_md.message_name, "MetaData message_name mismatch"


# -------------------------------------------------------------------------------------------------------
# Logger Framer Unit Tests
#  -------------------------------------------------------------------------------------------------------
def test_LOGGER():
    name = "novatel_framer"
    level = ne.LogLevel.OFF
    logger = ne.Framer().logger
    logger.set_level(level)
    assert logger.name == name
    assert logger.level == level
    assert ne.Logging.get(name) is not None


# -------------------------------------------------------------------------------------------------------
# ASCII Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_ASCII_COMPLETE(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ASCII, STATUS.SUCCESS, len(data))


def test_ASCII_INCOMPLETE(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ASCII, STATUS.INCOMPLETE, len(data))


def test_ASCII_SYNC_ERROR(helper):
    helper.write_file_to_framer("ascii_sync_error.ASC")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, ne.MAX_ASCII_MESSAGE_LENGTH)


def test_ASCII_BAD_CRC(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*ffffffff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, len(data))


def test_ASCII_RUN_ON_CRC(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35ff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ASCII, STATUS.INCOMPLETE, len(data))


def test_ASCII_INADEQUATE_BUFFER(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ASCII, STATUS.BUFFER_FULL, len(data), buffer_length=len(data) - 1)
    helper.test_framer(HEADER_FORMAT.ASCII, STATUS.SUCCESS, len(data), buffer_length=len(data))


def test_ASCII_BYTE_BY_BYTE(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    log_size = len(data)
    remaining_bytes = log_size
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.ASCII
    test_meta_data = ne.MetaData()
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        # We have to process the CRC all at the same time, so we can't test byte-by-byte
        # within it
        if remaining_bytes >= ne.OEM4_ASCII_CRC_LENGTH + 2:  # CRC + CRLF
            status, frame = helper.framer.get_frame(test_meta_data)
            assert status == STATUS.INCOMPLETE
            compare_metadata(test_meta_data, expected_meta_data)

        if not remaining_bytes:
            break
    expected_meta_data.length = log_size
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)


def test_ASCII_SEGMENTED(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    bytes_written = 0
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.ASCII
    test_meta_data = ne.MetaData()
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_ASCII_SYNC_LENGTH])
    bytes_written += ne.OEM4_ASCII_SYNC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:70])
    bytes_written += 70
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:135])
    bytes_written += 135
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:1])
    bytes_written += 1
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_ASCII_CRC_LENGTH + 2])
    bytes_written += ne.OEM4_ASCII_CRC_LENGTH + 2
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)

    assert bytes_written == len(data)


def test_ASCII_TRICK(helper):
    data = b"#TEST;*ffffffff\r\n#;*\r\n#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 17)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 5)
    helper.test_framer(HEADER_FORMAT.ASCII, STATUS.SUCCESS, 217)


def test_ABBREV_ASCII_SEGMENTED(helper):
    data = b"<RAWIMUX ICOM7 0 68.5 FINESTEERING 2222 136132.845 02040120 0dc5 16860\r\n< 04 41 2222 136132.844765 edb7fe00 327412165 - 7829932 13988218 - 498546 213188 - 987039\r\n[COM1]"
    log_size = len(data) - 6  # Remove the [ICOM] from the log size
    bytes_written = 0
    expected_frame_data = ne.MetaData()
    test_meta_data = ne.MetaData()
    helper.write_bytes_to_framer(data[bytes_written:][:1])  # Sync Byte
    bytes_written += 1
    expected_frame_data.length = bytes_written
    expected_frame_data.format = HEADER_FORMAT.ABB_ASCII
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_frame_data)

    helper.write_bytes_to_framer(data[bytes_written:][:69])  # Header with no CRLF
    bytes_written += 69
    expected_frame_data.length = bytes_written
    expected_frame_data.format = HEADER_FORMAT.ABB_ASCII
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_frame_data)

    helper.write_bytes_to_framer(data[bytes_written:][:1])  # CR
    bytes_written += 1
    expected_frame_data.length = bytes_written
    expected_frame_data.format = HEADER_FORMAT.ABB_ASCII
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_frame_data)

    helper.write_bytes_to_framer(data[bytes_written:][:1])  # LF
    bytes_written += 1
    expected_frame_data.length = bytes_written - 2  # helper.framer is going to step back 2 bytes to keep alignment with the CR
    # so no extra bytes to detect. Odd quirk with abbv ascii framing.
    expected_frame_data.format = HEADER_FORMAT.ABB_ASCII
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_frame_data)

    helper.write_bytes_to_framer(data[bytes_written:][:89])  # Body
    bytes_written += 89
    expected_frame_data.length = bytes_written
    expected_frame_data.format = HEADER_FORMAT.ABB_ASCII
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_frame_data)

    helper.write_bytes_to_framer(data[bytes_written:][:6 + 2])  # CRLF + [COM1]
    bytes_written += 2  # Ignore the [COM1]
    expected_frame_data.length = bytes_written
    expected_frame_data.format = HEADER_FORMAT.ABB_ASCII
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_frame_data)
    assert log_size == bytes_written


# -------------------------------------------------------------------------------------------------------
# Binary Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_BINARY_COMPLETE(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.BINARY, STATUS.SUCCESS, len(data))


def test_BINARY_INCOMPLETE(helper):
    # "<incomplete binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.BINARY, STATUS.INCOMPLETE, len(data))


def test_BINARY_BUFFER_FULL(helper):
    # "<incomplete binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00])
    helper.write_bytes_to_framer(data)
    test_meta_data = ne.MetaData()
    status, frame = helper.framer.get_frame(test_meta_data, len(data) - 1)
    assert status == STATUS.BUFFER_FULL


def test_BINARY_SYNC_ERROR(helper):
    helper.write_file_to_framer("binary_sync_error.BIN")
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = ne.MAX_BINARY_MESSAGE_LENGTH
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    status, frame, test_meta_data = helper.framer.get_frame()
    assert status == STATUS.UNKNOWN
    compare_metadata(test_meta_data, expected_meta_data)


def test_BINARY_BAD_CRC(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 57)


def test_BINARY_RUN_ON_CRC(helper):
    # "<binary BESTPOS log>FF"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89, 0xFF, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.BINARY, STATUS.SUCCESS, 104)


def test_BINARY_INADEQUATE_BUFFER(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.BINARY, STATUS.BUFFER_FULL, len(data), buffer_length=len(data) - 1)
    helper.test_framer(HEADER_FORMAT.BINARY, STATUS.SUCCESS, len(data), buffer_length=len(data))


def test_BINARY_BYTE_BY_BYTE(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89])
    log_size = len(data)
    remaining_bytes = log_size
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    test_meta_data = ne.MetaData()
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        if expected_meta_data.length == ne.OEM4_BINARY_SYNC_LENGTH:
            expected_meta_data.format = HEADER_FORMAT.BINARY

        if remaining_bytes > 0:
            status, frame = helper.framer.get_frame(test_meta_data)
            assert status == STATUS.INCOMPLETE
            compare_metadata(test_meta_data, expected_meta_data)
        else:
            break
    expected_meta_data.length = log_size
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)


def test_BINARY_SEGMENTED(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89])
    bytes_written = 0
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.BINARY
    test_meta_data = ne.MetaData()
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_SYNC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:(ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH)])
    bytes_written += (ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH)
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:72])
    bytes_written += 72
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_CRC_LENGTH])
    bytes_written += ne.OEM4_BINARY_CRC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)
    assert bytes_written == len(data)


def test_BINARY_TRICK(helper):
    # "<binary syncs><binary sync + half header><binary sync byte 1><binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73,
         0xAA, 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98,
         0x74, 0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
         0x00, 0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00,
         0x10, 0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5,
         0x3F, 0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 3)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 15)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.BINARY, STATUS.SUCCESS, 104)


# -------------------------------------------------------------------------------------------------------
# Short ASCII Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_SHORT_ASCII_COMPLETE(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, STATUS.SUCCESS, len(data))


def test_SHORT_ASCII_INCOMPLETE(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, STATUS.INCOMPLETE, len(data))


def test_SHORT_ASCII_SYNC_ERROR(helper):
    helper.write_file_to_framer("short_ascii_sync_error.ASC")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, ne.MAX_SHORT_ASCII_MESSAGE_LENGTH)


def test_SHORT_ASCII_BAD_CRC(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*ffffffff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, len(data))


def test_SHORT_ASCII_RUN_ON_CRC(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7bff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, STATUS.INCOMPLETE, len(data))


def test_SHORT_ASCII_INADEQUATE_BUFFER(helper):
    # "<binary BESTPOS log>"
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, STATUS.BUFFER_FULL, len(data), buffer_length=len(data) - 1)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, STATUS.SUCCESS, len(data), buffer_length=len(data))


def test_SHORT_ASCII_BYTE_BY_BYTE(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    log_size = len(data)
    remaining_bytes = log_size
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.SHORT_ASCII
    test_meta_data = ne.MetaData()
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        # We have to process the CRC all at the same time, so we can't test byte-by-byte
        # within it
        if remaining_bytes >= ne.OEM4_ASCII_CRC_LENGTH + 2:  # CRC + CRLF
            status, frame = helper.framer.get_frame(test_meta_data)
            assert status == STATUS.INCOMPLETE
            compare_metadata(test_meta_data, expected_meta_data)

        if not remaining_bytes:
            break
    expected_meta_data.length = log_size
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)


def test_SHORT_ASCII_SEGMENTED(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_SHORT_ASCII_SYNC_LENGTH])
    bytes_written += ne.OEM4_SHORT_ASCII_SYNC_LENGTH
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:26])
    bytes_written += 26
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:82])
    bytes_written += 82
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:1])
    bytes_written += 1
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_ASCII_CRC_LENGTH + 2])
    bytes_written += ne.OEM4_ASCII_CRC_LENGTH + 2
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.SUCCESS, bytes_written)

    assert bytes_written == len(data)


def test_SHORT_ASCII_TRICK(helper):
    data = b"%;*\r\n%%**\r\n%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 5)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 5)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, STATUS.SUCCESS, 120)


# -------------------------------------------------------------------------------------------------------
# Short Binary Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_SHORT_BINARY_COMPLETE(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, STATUS.SUCCESS, len(data))


def test_SHORT_BINARY_INCOMPLETE(helper):
    # "<incomplete short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, STATUS.INCOMPLETE, len(data))


def test_SHORT_BINARY_BUFFER_FULL(helper):
    # "<incomplete short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87])
    helper.write_bytes_to_framer(data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 34
    expected_meta_data.format = HEADER_FORMAT.SHORT_BINARY
    status, frame, test_meta_data = helper.framer.get_frame()
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)


def test_SHORT_BINARY_SYNC_ERROR(helper):
    helper.write_file_to_framer("short_binary_sync_error.BIN")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, None)


def test_SHORT_BINARY_BAD_CRC(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, len(data))


def test_SHORT_BINARY_RUN_ON_CRC(helper):
    # "<short binary rawimusx log>FF"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA, 0xFF, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, STATUS.SUCCESS, 56)


def test_SHORT_BINARY_INADEQUATE_BUFFER(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, STATUS.BUFFER_FULL, len(data), buffer_length=len(data) - 1)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, STATUS.SUCCESS, len(data), buffer_length=len(data))


def test_SHORT_BINARY_BYTE_BY_BYTE(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA])
    log_size = len(data)
    remaining_bytes = log_size
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    test_meta_data = ne.MetaData()
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        if expected_meta_data.length == ne.OEM4_SHORT_BINARY_SYNC_LENGTH:
            expected_meta_data.format = HEADER_FORMAT.SHORT_BINARY

        if remaining_bytes > 0:
            status, frame = helper.framer.get_frame(test_meta_data)
            assert status == STATUS.INCOMPLETE
            compare_metadata(test_meta_data, expected_meta_data)
        else:
            break
    expected_meta_data.length = log_size
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)


def test_SHORT_BINARY_SEGMENTED(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA])
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_SHORT_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_SHORT_BINARY_SYNC_LENGTH
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(
        data[bytes_written:][:ne.OEM4_SHORT_BINARY_HEADER_LENGTH - ne.OEM4_SHORT_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_SHORT_BINARY_HEADER_LENGTH - ne.OEM4_SHORT_BINARY_SYNC_LENGTH
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:40])
    bytes_written += 40
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_CRC_LENGTH])
    bytes_written += ne.OEM4_BINARY_CRC_LENGTH
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.SUCCESS, bytes_written)


def test_SHORT_BINARY_TRICK(helper):
    # "<short binary sync><short binary sync + part header><short binary sync 1><short binary RAWIMUSX log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xAA, 0xAA, 0x44, 0x13, 0x28,
         0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97, 0x55, 0xA8, 0x32, 0x94,
         0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF, 0x38, 0xEA, 0xFC, 0xFF,
         0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F, 0xAF, 0xBA, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 3)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 10)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, STATUS.SUCCESS, 56)


# -------------------------------------------------------------------------------------------------------
# NMEA Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_NMEA_COMPLETE(helper):
    data = b"$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.NMEA, STATUS.SUCCESS, len(data))


def test_NMEA_INCOMPLETE(helper):
    data = b"$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc4"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.NMEA, STATUS.INCOMPLETE, len(data))


def test_NMEA_SYNC_ERROR(helper):
    helper.write_file_to_framer("nmea_sync_error.txt")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, None)


def test_NMEA_BAD_CRC(helper):
    data = b"$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*11\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, len(data))


def test_NMEA_RUN_ON_CRC(helper):
    data = b"$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29ff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, len(data))


def test_NMEA_INADEQUATE_BUFFER(helper):
    data = b"$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.NMEA, STATUS.BUFFER_FULL, len(data), buffer_length=len(data) - 1)
    helper.test_framer(HEADER_FORMAT.NMEA, STATUS.SUCCESS, len(data), buffer_length=len(data))


def test_NMEA_BYTE_BY_BYTE(helper):
    data = b"$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n"
    log_size = len(data)
    remaining_bytes = log_size
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.NMEA
    test_meta_data = ne.MetaData()
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        if remaining_bytes > 0:
            status, frame = helper.framer.get_frame(test_meta_data)
            assert status == STATUS.INCOMPLETE
            compare_metadata(test_meta_data, expected_meta_data)
        else:
            break
    expected_meta_data.length = log_size
    status, frame = helper.framer.get_frame(test_meta_data)
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)


def test_NMEA_SEGMENTED(helper):
    data = b"$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n"
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:ne.NMEA_SYNC_LENGTH])
    bytes_written += ne.NMEA_SYNC_LENGTH
    helper.test_framer(HEADER_FORMAT.NMEA, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:76])
    bytes_written += 76
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:1])
    bytes_written += 1
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.NMEA_CRC_LENGTH])
    bytes_written += ne.NMEA_CRC_LENGTH
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.INCOMPLETE, bytes_written)

    helper.write_bytes_to_framer(data[bytes_written:][:2])
    bytes_written += 2
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.SUCCESS, bytes_written)

    assert bytes_written == len(data)


def test_NMEA_TRICK(helper):
    data = b"$*ff\r\n$$**\r\n$GPALM,30,01,01,2029,00,4310,7b,145f,fd44,a10ce4,1c5b11,0b399f,2bc421,f80,ffe*29\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 6)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 5)
    helper.test_framer(HEADER_FORMAT.NMEA, STATUS.SUCCESS, 82)


# -------------------------------------------------------------------------------------------------------
# Abbreviated ASCII Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_ABBREV_ASCII_COMPLETE(helper):
    data = (b"<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
            b"<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n[COM1]")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.SUCCESS, len(data) - 6)


def test_ABBREV_ASCII_INCOMPLETE(helper):
    data = (b"<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
            b"<     SOL_COMPUTED SINGLE 51.15043711386 ")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.INCOMPLETE, len(data))


def test_ABBREV_ASCII_BUFFER_FULL(helper):
    data = b"<ERROR:Message is invalid for this model\r\n"
    helper.write_bytes_to_framer(data)
    test_meta_data = ne.MetaData()
    status, frame = helper.framer.get_frame(test_meta_data, len(data) - 1)
    assert status == STATUS.BUFFER_FULL


def test_ABBREV_ASCII_SYNC_ERROR(helper):
    helper.write_file_to_framer("abbreviated_ascii_sync_error.ASC")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, ne.MAX_ASCII_MESSAGE_LENGTH)


def test_ABBREV_ASCII_INADEQUATE_BUFFER(helper):
    data = (b"<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
            b"<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n[COM1]")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.BUFFER_FULL, len(data) - 6, buffer_length=len(data) - 7)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.SUCCESS, len(data) - 6, buffer_length=len(data) - 6)


def test_ABBREV_ASCII_NO_PROMPT(helper):
    data = (b"<TIME COM1 0 48.5 FINESTEERING 2211 314480.000 02000000 9924 32768\r\n"
            b"<     VALID -1.055585415e-09 7.492303535e-10 -17.99999999958 2022 5 25 15 21 2000 VALID\r\n"
            b"<TIME COM1 0 46.5 FINESTEERING 2211 314490.000 02000000 9924 32768\r\n"
            b"<     VALID 5.035219694e-10 7.564775104e-10 -17.99999999958 2022 5 25 15 21 12000 VALID\r\n")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.SUCCESS, 157)
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.SUCCESS, 157)


def test_ABBREV_ASCII_MULTILINE(helper):
    data = (b"<SAVEDSURVEYPOSITIONS COM1 0 55.5 FINESTEERING 2211 324085.143 02000000 ddf2 32768\r\n"
            b"<     2 \r\n"
            b"<          \"MN01\" 51.11600000000 -114.03800000000 1065.0000 \r\n"
            b"<          \"MN02\" 51.11400000000 -114.03700000000 1063.1000\r\n[COM1]")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.SUCCESS, len(data) - 6)


def test_ABBREV_ASCII_RESPONSE(helper):
    data = b"<ERROR:Message is invalid for this model\r\n"
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.ABB_ASCII
    helper.write_bytes_to_framer(data)
    expected_meta_data.response = True
    expected_meta_data.length = len(data)
    status, frame, test_meta_data = helper.framer.get_frame()
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)


def test_ABBREV_ASCII_SWAPPED(helper):
    data = (b"<     64 60 B1D2 4 e2410e75b821e2664201b02000b022816c36140020001ddde0000000\r\n"
            b"<BDSRAWNAVSUBFRAME ICOM1_29 0 40.5 FINESTEERING 2204 236927.000 02060000 88f3 16807\r\n<GARBAGE")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 77)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 85)


def test_ABBREV_ASCII_EMPTY_ARRAY(helper):
    data = b"<RANGE COM1 0 95.5 UNKNOWN 0 170.000 025c0020 5103 16807\r\n<     0 \r\n<         \r\n[COM1]"
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.ABB_ASCII
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, STATUS.SUCCESS, len(data) - 6)


# -------------------------------------------------------------------------------------------------------
# JSON Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_JSON_COMPLETE(helper):
    data = b"""{"header": {"message": "BESTSATS","id": 1194,"port": "COM1","sequence_num": 0,"percent_idle_time": 50.0,"time_status": "FINESTEERING","week": 2167,"seconds": 244820.000,"receiver_status": 33554432,"HEADER_reserved1": 48645,"receiver_sw_version": 16248},"body": {"satellite_entries": [{"system_type": "GPS","id": "2","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "20","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "29","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "13","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "16","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "18","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "25","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "5","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "26","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "23","status": "GOOD","status_mask": 7},{"system_type": "QZSS","id": "194","status": "SUPPLEMENTARY","status_mask": 7},{"system_type": "SBAS","id": "131","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "133","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "138","status": "NOTUSED","status_mask": 0},{"system_type": "GLONASS","id": "8+6","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "9-2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "1+1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "24+2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "2-4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "17+4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "16-1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "18-3","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GALILEO","id": "26","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "12","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "19","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "31","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "33","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "8","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "7","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "24","status": "GOOD","status_mask": 15},{"system_type": "BEIDOU","id": "35","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "29","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "20","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "22","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "44","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "57","status": "NOEPHEMERIS","status_mask": 0},{"system_type": "BEIDOU","id": "12","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "24","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "19","status": "SUPPLEMENTARY","status_mask": 1}]}}"""
    helper.write_bytes_to_framer(data)
    helper.framer.set_frame_json(True)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = len(data)
    expected_meta_data.format = HEADER_FORMAT.JSON
    status, frame, test_meta_data = helper.framer.get_frame()
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)

    helper.framer.set_frame_json(False)


# -------------------------------------------------------------------------------------------------------
# Edge-case Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_UNKNOWN_BINARY_WITH_ASCII_SYNC(helper):
    data = b"\x07#\x82"  # '#' is used-to identify binary payload
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 1)
