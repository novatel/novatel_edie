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
# \file proprietarytest.hpp
# \brief Unit tests for proprietary NovAtel logs.
################################################################################

from pathlib import Path

import novatel_edie as ne
import pytest
from novatel_edie import HEADERFORMAT, STATUS

PROJECT_ROOT = Path(__file__).parent.parent.parent
TEST_RESOURCE_PATH = PROJECT_ROOT / "src" / "decoders" / "novatel" / "test" / "resources"


@pytest.fixture(scope="function")
def framer():
    framer = ne.Framer()
    framer.set_report_unknown_bytes(True)
    framer.set_payload_only(False)
    return framer


def write_file_to_framer(framer, filename):
    data = (TEST_RESOURCE_PATH / filename).read_bytes()
    bytes_written = framer.write(data)
    assert bytes_written == len(data)


def write_bytes_to_framer(framer, bytes_):
    print(len(bytes_), bytes_)
    assert framer.write(bytes_) == len(bytes_)


def compare_metadata(test_md, expected_md):
    result = True
    if test_md.format != expected_md.format:
        print(f"MetaData.format (expected {expected_md.format}, got {test_md.format})")
        result = False
    if test_md.measurement_source != expected_md.measurement_source:
        print(f"MetaData.measurement_source (expected {int(expected_md.measurement_source)}, got {int(test_md.measurement_source)})")
        result = False
    if test_md.time_status != expected_md.time_status:
        print(f"MetaData.time_status (expected {int(expected_md.time_status)}, got {int(test_md.time_status)})")
        result = False
    if test_md.response != expected_md.response:
        print(f"MetaData.response (expected {int(expected_md.response)}, got {int(test_md.response)})")
        result = False
    if test_md.week != expected_md.week:
        print(f"MetaData.week (expected {expected_md.week}, got {test_md.week})")
        result = False
    if test_md.milliseconds != expected_md.milliseconds:
        print(f"MetaData.milliseconds (expected {expected_md.milliseconds}, got {test_md.milliseconds})")
        result = False
    if test_md.binary_msg_length != expected_md.binary_msg_length:
        print(f"MetaData.binary_msg_length (expected {expected_md.binary_msg_length}, got {test_md.binary_msg_length})")
        result = False
    if test_md.length != expected_md.length:
        print(f"MetaData.length (expected {expected_md.length}, got {test_md.length})")
        result = False
    if test_md.header_length != expected_md.header_length:
        print(f"MetaData.header_length (expected {expected_md.header_length}, got {test_md.header_length})")
        result = False
    if test_md.message_id != expected_md.message_id:
        print(f"MetaData.message_id (expected {expected_md.message_id}, got {test_md.message_id})")
        result = False
    if test_md.message_crc != expected_md.message_crc:
        print(f"MetaData.messageCRC (expected {expected_md.message_crc}, got {test_md.message_crc})")
        result = False
    if test_md.message_name != expected_md.message_name:
        print(f"MetaData.message_name (expected {expected_md.message_name}, got {test_md.message_name})")
        result = False
    return result


# -------------------------------------------------------------------------------------------------------
# Proprietary Binary Framer Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_PROPRIETARY_BINARY_COMPLETE(framer):
    # "GARBAGE_DATA<binary bestpos log>"
    data = bytes(
        [0x47, 0x41, 0x52, 0x42, 0x41, 0x47, 0x45, 0x5F, 0x44, 0x41, 0x54, 0x41, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09,
         0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02,
         0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC,
         0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35,
         0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC])
    write_bytes_to_framer(framer, data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 12
    expected_meta_data.format = HEADERFORMAT.UNKNOWN
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.UNKNOWN
    assert compare_metadata(test_meta_data, expected_meta_data)

    expected_meta_data.length = 76
    expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.SUCCESS
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_PROPRIETARY_BINARY_INCOMPLETE(framer):
    # "<incomplete binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A])
    write_bytes_to_framer(framer, data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 59
    expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.INCOMPLETE
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_PROPRIETARY_BINARY_SYNC_ERROR(framer):
    write_file_to_framer(framer, "proprietary_binary_sync_error.BIN")
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = ne.MAX_BINARY_MESSAGE_LENGTH
    expected_meta_data.format = HEADERFORMAT.UNKNOWN
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.UNKNOWN
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_PROPRIETARY_BINARY_BAD_CRC(framer):
    # "<encrypted binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xFF])
    write_bytes_to_framer(framer, data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 30  # Unknown bytes up to 0x24 ('$') should be returned (NMEA sync was found mid-log)
    expected_meta_data.format = HEADERFORMAT.UNKNOWN
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.UNKNOWN
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_PROPRIETARY_BINARY_RUN_ON_CRC(framer):
    # "<encrypted binary bestpos log>FF"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC, 0xFF])
    write_bytes_to_framer(framer, data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 76
    expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.SUCCESS
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_PROPRIETARY_BINARY_INADEQUATE_BUFFER(framer):
    # "<encrypted binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    write_bytes_to_framer(framer, data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 76
    expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
    test_meta_data = ne.MetaData()
    status, frame = framer.get_frame(test_meta_data, buffer_size=38)
    assert status == STATUS.BUFFER_FULL
    assert compare_metadata(test_meta_data, expected_meta_data)

    status, frame = framer.get_frame(test_meta_data, buffer_size=76)
    assert status == STATUS.SUCCESS
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_PROPRIETARY_BINARY_BYTE_BY_BYTE(framer):
    # "<binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    log_size = len(data)
    remaining_bytes = log_size
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADERFORMAT.UNKNOWN
    test_meta_data = ne.MetaData()
    while True:
        write_bytes_to_framer(framer, data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        expected_meta_data.length = log_size - remaining_bytes
        if expected_meta_data.length == ne.OEM4_BINARY_SYNC_LENGTH - 1:
            expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY

        if remaining_bytes > 0:
            status, frame = framer.get_frame(test_meta_data)
            assert status == STATUS.INCOMPLETE
            assert compare_metadata(test_meta_data, expected_meta_data)
        else:
            break
    expected_meta_data.length = log_size
    status, frame = framer.get_frame(test_meta_data)
    assert status == STATUS.SUCCESS
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_PROPRIETARY_BINARY_SEGMENTED(framer):
    # "<binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    bytes_written = 0
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
    test_meta_data = ne.MetaData()
    write_bytes_to_framer(framer, data[bytes_written:][:ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_SYNC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    assert compare_metadata(test_meta_data, expected_meta_data)

    write_bytes_to_framer(framer, data[bytes_written:][:ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    assert compare_metadata(test_meta_data, expected_meta_data)

    write_bytes_to_framer(framer, data[bytes_written:][:44])
    bytes_written += 44
    expected_meta_data.length = bytes_written
    status, frame = framer.get_frame(test_meta_data)
    assert status == STATUS.INCOMPLETE
    assert compare_metadata(test_meta_data, expected_meta_data)

    write_bytes_to_framer(framer, data[bytes_written:][:ne.OEM4_BINARY_CRC_LENGTH])
    bytes_written += ne.OEM4_BINARY_CRC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = framer.get_frame(test_meta_data)
    assert status == STATUS.SUCCESS
    assert compare_metadata(test_meta_data, expected_meta_data)
    assert bytes_written == len(data)


def test_PROPRIETARY_BINARY_TRICK(framer):
    # "<binary syncs><binary sync + half header><binary sync byte 1><binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82,
         0xAA, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8,
         0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E,
         0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF,
         0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91,
         0x27, 0x6F, 0x8E, 0x0B, 0xCC])
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADERFORMAT.UNKNOWN
    write_bytes_to_framer(framer, data)
    expected_meta_data.length = 3
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.UNKNOWN
    assert compare_metadata(test_meta_data, expected_meta_data)

    expected_meta_data.length = 15
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.UNKNOWN
    assert compare_metadata(test_meta_data, expected_meta_data)

    expected_meta_data.length = 1
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.UNKNOWN
    assert compare_metadata(test_meta_data, expected_meta_data)

    expected_meta_data.length = 76
    expected_meta_data.format = HEADERFORMAT.PROPRIETARY_BINARY
    status, frame, test_meta_data = framer.get_frame()
    assert status == STATUS.SUCCESS
    assert compare_metadata(test_meta_data, expected_meta_data)
