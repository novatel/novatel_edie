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
        self.framer.report_unknown_bytes = True
        self.framer.payload_only = False

    def test_framer_errors(self, expected_error, buffer_size=ne.MAX_MESSAGE_LENGTH):
        pass
        with pytest.raises(expected_error):
            self.framer.get_frame(buffer_size)

    def test_framer(self, expected_header_format, length, buffer_size=ne.MAX_MESSAGE_LENGTH, response=False):
        # Arrange
        expected_meta_data = ne.MetaData()
        if length is not None:
            expected_meta_data.length = length
        expected_meta_data.response = response
        expected_meta_data.format = expected_header_format
        # Act
        _, test_meta_data = self.framer.get_frame(buffer_size)

        # Assert
        compare_metadata(test_meta_data, expected_meta_data, ignore_length=length is None)

    def get_file_contents(self, filename):
        return (self.test_resources / filename).read_bytes()

    def write_bytes_to_framer(self, data):
        assert self.framer.write(data) == len(data)

    def write_file_to_framer(self, filename):
        data = self.get_file_contents(filename)
        self.write_bytes_to_framer(data)


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
# ASCII Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_ascii_complete(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ASCII, len(data))


def test_ascii_incomplete(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1"
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.IncompleteException)


def test_ascii_sync_error(helper):
    file = "ascii_sync_error.ASC"
    data = helper.get_file_contents(file)
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1, buffer_size=ne.MAX_ASCII_MESSAGE_LENGTH)


def test_ascii_bad_crc(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*ffffffff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1, buffer_size=len(data))


def test_ascii_run_on_crc(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35ff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.IncompleteException)


def test_ascii_inadequate_buffer(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    helper.write_bytes_to_framer(data)

    helper.test_framer_errors(ne.BufferFullException, len(data) - 1)
    helper.test_framer(HEADER_FORMAT.ASCII, len(data), len(data))


def test_ascii_byte_by_byte(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    log_size = len(data)
    remaining_bytes = log_size
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        # We have to process the CRC all at the same time, so we can't test byte-by-byte
        # within it
        if remaining_bytes >= ne.OEM4_ASCII_CRC_LENGTH + 2:  # CRC + CRLF
            helper.test_framer_errors(ne.IncompleteException)
        if not remaining_bytes:
            break
    helper.test_framer(HEADER_FORMAT.ASCII, log_size)


def test_ascii_segmented(helper):
    data = b"#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_ASCII_SYNC_LENGTH])
    bytes_written += ne.OEM4_ASCII_SYNC_LENGTH
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:70])
    bytes_written += 70
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:135])
    bytes_written += 135
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:1])
    bytes_written += 1
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_ASCII_CRC_LENGTH + 2])
    bytes_written += ne.OEM4_ASCII_CRC_LENGTH + 2
    helper.test_framer(HEADER_FORMAT.ASCII, bytes_written)

    assert bytes_written == len(data)


def test_ascii_trick(helper):
    data = b"#TEST;*ffffffff\r\n#;*\r\n#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 16)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 4)
    helper.test_framer(HEADER_FORMAT.ASCII, 217)


def test_abbrev_ascii_segmented(helper):
    data = b"<RAWIMUX ICOM7 0 68.5 FINESTEERING 2222 136132.845 02040120 0dc5 16860\r\n< 04 41 2222 136132.844765 edb7fe00 327412165 - 7829932 13988218 - 498546 213188 - 987039\r\n[COM1]"
    log_size = len(data) - 6  # Remove the [ICOM] from the log size
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:1])  # Sync Byte
    bytes_written += 1
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:69])  # Header with no CRLF
    bytes_written += 69
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:1])  # CR
    bytes_written += 1
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:1])  # LF
    bytes_written += 1
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:89])  # Body
    bytes_written += 89
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:6 + 2])  # CRLF + [COM1]
    bytes_written += 2  # Ignore the [COM1]
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, log_size)
    assert log_size == bytes_written


# -------------------------------------------------------------------------------------------------------
# Binary Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_binary_complete(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.BINARY, len(data))


def test_binary_incomplete(helper):
    # "<incomplete binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00])
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.IncompleteException)


def test_binary_buffer_full(helper):
    # "<incomplete binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00])
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.BufferFullException, len(data) - 1)


def test_binary_sync_error(helper):
    helper.write_file_to_framer("binary_sync_error.BIN")
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = ne.MAX_BINARY_MESSAGE_LENGTH
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 3, buffer_size=ne.MAX_BINARY_MESSAGE_LENGTH)


def test_binary_bad_crc(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 57)


def test_binary_run_on_crc(helper):
    # "<binary BESTPOS log>FF"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89, 0xFF, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.BINARY, 104)


def test_binary_inadequate_buffer(helper):
    # "<binary BESTPOS log>"
    data = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
         0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
         0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
         0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89])
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.BufferFullException, len(data) - 1)
    helper.test_framer(HEADER_FORMAT.BINARY, len(data))


def test_binary_byte_by_byte(helper):
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
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        if expected_meta_data.length == ne.OEM4_BINARY_SYNC_LENGTH:
            expected_meta_data.format = HEADER_FORMAT.BINARY

        if remaining_bytes > 0:
            helper.test_framer_errors(ne.IncompleteException)
        else:
            break
    expected_meta_data.length = log_size
    helper.test_framer(HEADER_FORMAT.BINARY, log_size)


def test_binary_segmented(helper):
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
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_SYNC_LENGTH
    expected_meta_data.length = bytes_written
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:(ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH)])
    bytes_written += (ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH)
    expected_meta_data.length = bytes_written
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:72])
    bytes_written += 72
    expected_meta_data.length = bytes_written
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_CRC_LENGTH])
    bytes_written += ne.OEM4_BINARY_CRC_LENGTH
    expected_meta_data.length = bytes_written
    helper.test_framer(HEADER_FORMAT.BINARY, len(data))


def test_binary_trick(helper):
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
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 3)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 15)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.BINARY, 104)


# -------------------------------------------------------------------------------------------------------
# Short ASCII Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_short_ascii_complete(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, len(data))


def test_short_ascii_incomplete(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215"
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.IncompleteException)


def test_short_ascii_sync_error(helper):
    helper.write_file_to_framer("short_ascii_sync_error.ASC")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1, buffer_size=ne.MAX_SHORT_ASCII_MESSAGE_LENGTH)


def test_short_ascii_bad_crc(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*ffffffff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1, buffer_size=len(data))


def test_short_ascii_run_on_crc(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7bff\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.IncompleteException)


def test_short_ascii_inadequate_buffer(helper):
    # "<binary BESTPOS log>"
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.BufferFullException, len(data) - 1)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, len(data), len(data))


def test_short_ascii_byte_by_byte(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    log_size = len(data)
    remaining_bytes = log_size
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.SHORT_ASCII
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        # We have to process the CRC all at the same time, so we can't test byte-by-byte
        # within it
        if remaining_bytes >= ne.OEM4_ASCII_CRC_LENGTH + 2:  # CRC + CRLF
            helper.test_framer_errors(ne.IncompleteException)

        if not remaining_bytes:
            break
    expected_meta_data.length = log_size
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, log_size)


def test_short_ascii_segmented(helper):
    data = b"%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_SHORT_ASCII_SYNC_LENGTH])
    bytes_written += ne.OEM4_SHORT_ASCII_SYNC_LENGTH
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:26])
    bytes_written += 26
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:82])
    bytes_written += 82
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:1])
    bytes_written += 1
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_ASCII_CRC_LENGTH + 2])
    bytes_written += ne.OEM4_ASCII_CRC_LENGTH + 2
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, bytes_written)

    assert bytes_written == len(data)


def test_short_ascii_trick(helper):
    data = b"%;*\r\n%%**\r\n%RAWIMUSXA,1692,484620.664;00,11,1692,484620.664389000,00801503,43110635,-817242,-202184,-215194,-41188,-9895*a5db8c7b\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 4)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 5)
    helper.test_framer(HEADER_FORMAT.SHORT_ASCII, 120)


# -------------------------------------------------------------------------------------------------------
# Short Binary Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_short_binary_complete(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, len(data))


def test_short_binary_incomplete(helper):
    # "<incomplete short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87])
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.IncompleteException)


def test_short_binary_buffer_full(helper):
    # "<incomplete short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87])
    helper.write_bytes_to_framer(data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 34
    expected_meta_data.format = HEADER_FORMAT.SHORT_BINARY
    helper.test_framer_errors(ne.BufferFullException, len(data) - 1)


def test_short_binary_sync_error(helper):
    helper.write_file_to_framer("short_binary_sync_error.BIN")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, None)


def test_short_binary_bad_crc(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, len(data))


def test_short_binary_run_on_crc(helper):
    # "<short binary rawimusx log>FF"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA, 0xFF, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, 56)


def test_short_binary_inadequate_buffer(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA])
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.BufferFullException, len(data) - 1)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, len(data), len(data))


def test_short_binary_byte_by_byte(helper):
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
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        if expected_meta_data.length == ne.OEM4_SHORT_BINARY_SYNC_LENGTH:
            expected_meta_data.format = HEADER_FORMAT.SHORT_BINARY

        if remaining_bytes > 0:
            helper.test_framer_errors(ne.IncompleteException)
        else:
            break
    expected_meta_data.length = log_size
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, log_size)


def test_short_binary_segmented(helper):
    # "<short binary rawimusx log>"
    data = bytes(
        [0xAA, 0x44, 0x13, 0x28, 0xB6, 0x05, 0x9C, 0x06, 0x78, 0xB9, 0xE2, 0x1C, 0x00, 0x0B, 0x9C, 0x06, 0x0B, 0x97,
         0x55, 0xA8, 0x32, 0x94, 0x1D, 0x41, 0x03, 0x15, 0x80, 0x00, 0xEB, 0xD0, 0x91, 0x02, 0xA6, 0x87, 0xF3, 0xFF,
         0x38, 0xEA, 0xFC, 0xFF, 0x66, 0xB7, 0xFC, 0xFF, 0x1C, 0x5F, 0xFF, 0xFF, 0x59, 0xD9, 0xFF, 0xFF, 0x47, 0x5F,
         0xAF, 0xBA])
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_SHORT_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_SHORT_BINARY_SYNC_LENGTH
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(
        data[bytes_written:][:ne.OEM4_SHORT_BINARY_HEADER_LENGTH - ne.OEM4_SHORT_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_SHORT_BINARY_HEADER_LENGTH - ne.OEM4_SHORT_BINARY_SYNC_LENGTH
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:40])
    bytes_written += 40
    helper.test_framer_errors(ne.IncompleteException)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_CRC_LENGTH])
    bytes_written += ne.OEM4_BINARY_CRC_LENGTH
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, bytes_written)


def test_short_binary_trick(helper):
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
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 3)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 10)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.SHORT_BINARY, 56)


# -------------------------------------------------------------------------------------------------------
# Abbreviated ASCII Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_abbrev_ascii_complete(helper):
    data = (b"<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
            b"<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n[COM1]")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, len(data) - 6)


def test_abbrev_ascii_incomplete(helper):
    data = (b"<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
            b"<     SOL_COMPUTED SINGLE 51.15043711386 ")
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.IncompleteException)


def test_abbrev_ascii_buffer_full(helper):
    data = b"<ERROR:Message is invalid for this model\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.BufferFullException, len(data) - 1)


def test_abbrev_ascii_sync_error(helper):
    helper.write_file_to_framer("abbreviated_ascii_sync_error.ASC")
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1, buffer_size=ne.MAX_ASCII_MESSAGE_LENGTH)


def test_abbrev_ascii_inadequate_buffer(helper):
    data = (b"<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
            b"<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n[COM1]")
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(ne.BufferFullException, len(data) - 7)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, len(data) - 6, len(data) - 6)


def test_abbrev_ascii_no_prompt(helper):
    data = (b"<TIME COM1 0 48.5 FINESTEERING 2211 314480.000 02000000 9924 32768\r\n"
            b"<     VALID -1.055585415e-09 7.492303535e-10 -17.99999999958 2022 5 25 15 21 2000 VALID\r\n"
            b"<TIME COM1 0 46.5 FINESTEERING 2211 314490.000 02000000 9924 32768\r\n"
            b"<     VALID 5.035219694e-10 7.564775104e-10 -17.99999999958 2022 5 25 15 21 12000 VALID\r\n")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, 157)
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, 157)


def test_abbrev_ascii_multiline(helper):
    data = (b"<SAVEDSURVEYPOSITIONS COM1 0 55.5 FINESTEERING 2211 324085.143 02000000 ddf2 32768\r\n"
            b"<     2 \r\n"
            b"<          \"MN01\" 51.11600000000 -114.03800000000 1065.0000 \r\n"
            b"<          \"MN02\" 51.11400000000 -114.03700000000 1063.1000\r\n[COM1]")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, len(data) - 6)


def test_abbrev_ascii_response(helper):
    data = b"<ERROR:Message is invalid for this model\r\n"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, len(data), response=True)


def test_abbrev_ascii_swapped(helper):
    data = (b"<     64 60 B1D2 4 e2410e75b821e2664201b02000b022816c36140020001ddde0000000\r\n"
            b"<BDSRAWNAVSUBFRAME ICOM1_29 0 40.5 FINESTEERING 2204 236927.000 02060000 88f3 16807\r\n<GARBAGE")
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 2)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 75)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 84)


def test_abbrev_ascii_empty_array(helper):
    data = b"<RANGE COM1 0 95.5 UNKNOWN 0 170.000 025c0020 5103 16807\r\n<     0 \r\n<         \r\n[COM1]"
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.ABB_ASCII, len(data) - 6)


@pytest.mark.parametrize("response_frame, context", [(b"<OK\r\n", b"\r\n<OK\r\nfdfa")])
def test_parse_abbrev_ascii_resp(response_frame, context, helper):
    # Arrange
    permutations = [(context[:i], context[i:]) for i in range(len(context) + 1)]

    # Act
    for part1, part2 in permutations:
        helper.write_bytes_to_framer(part1)
        data = list(helper.framer)
        helper.write_bytes_to_framer(part2)
        new_data = list(helper.framer)
        data.extend(new_data)

    # Assert
    frames = [datum[0] for datum in data]
    assert response_frame in frames

    md = data[frames.index(response_frame)][1]
    assert md.format == HEADER_FORMAT.ABB_ASCII
    assert md.response is True

# -------------------------------------------------------------------------------------------------------
# JSON Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_json_complete(helper):
    data = b"""{"header": {"message": "BESTSATS","id": 1194,"port": "COM1","sequence_num": 0,"percent_idle_time": 50.0,"time_status": "FINESTEERING","week": 2167,"seconds": 244820.000,"receiver_status": 33554432,"HEADER_reserved1": 48645,"receiver_sw_version": 16248},"body": {"satellite_entries": [{"system_type": "GPS","id": "2","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "20","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "29","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "13","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "16","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "18","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "25","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "5","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "26","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "23","status": "GOOD","status_mask": 7},{"system_type": "QZSS","id": "194","status": "SUPPLEMENTARY","status_mask": 7},{"system_type": "SBAS","id": "131","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "133","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "138","status": "NOTUSED","status_mask": 0},{"system_type": "GLONASS","id": "8+6","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "9-2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "1+1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "24+2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "2-4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "17+4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "16-1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "18-3","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GALILEO","id": "26","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "12","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "19","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "31","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "33","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "8","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "7","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "24","status": "GOOD","status_mask": 15},{"system_type": "BEIDOU","id": "35","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "29","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "20","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "22","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "44","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "57","status": "NOEPHEMERIS","status_mask": 0},{"system_type": "BEIDOU","id": "12","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "24","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "19","status": "SUPPLEMENTARY","status_mask": 1}]}}"""
    helper.write_bytes_to_framer(data)
    helper.framer.frame_json = True
    helper.test_framer(HEADER_FORMAT.JSON, len(data))

    helper.framer.frame_json = False


# -------------------------------------------------------------------------------------------------------
# Edge-case Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_unknown_binary_with_ascii_sync(helper):
    data = b"\x07#\x82"  # '#' is used-to identify binary payload
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)

def test_write_max_num_bytes(helper: Helper):
    """Tests that data with length matching available space can be written."""
    # Arrange
    data = b'a' * helper.framer.available_space
    # Act
    bytes_written = helper.framer.write(data)
    # Assert
    assert bytes_written == len(data)

def test_write_exceeding_max_num_bytes(helper: Helper):
    """Tests that data exceeding available space is not fully written.

    Whether data is partially written is not defined in the spec.
    """
    # Arrange
    data = b'a' * (helper.framer.available_space + 1)
    # Act
    bytes_written = helper.framer.write(data)
    # Assert
    assert bytes_written <= helper.framer.available_space
