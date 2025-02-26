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

import novatel_edie as ne
import pytest
from novatel_edie import HEADER_FORMAT, STATUS
from test.test_framer import Helper

@pytest.fixture(scope="function")
def helper(decoders_test_resources):
    return Helper(decoders_test_resources)


def compare_metadata(test_md, expected_md):
    result = True
    if test_md.format != expected_md.format:
        print(f"MetaData.format (expected {expected_md.format}, got {test_md.format})")
        result = False
    if test_md.measurement_source != expected_md.measurement_source:
        print(
            f"MetaData.measurement_source (expected {int(expected_md.measurement_source)}, got {int(test_md.measurement_source)})")
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
def test_proprietary_binary_complete(helper):
    # "GARBAGE_DATA<binary bestpos log>"
    data = bytes(
        [0x47, 0x41, 0x52, 0x42, 0x41, 0x47, 0x45, 0x5F, 0x44, 0x41, 0x54, 0x41, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09,
         0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02,
         0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC,
         0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35,
         0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC])
    helper.write_bytes_to_framer(data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 12
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    _, test_meta_data = helper.framer.get_frame()
    assert compare_metadata(test_meta_data, expected_meta_data)

    expected_meta_data.length = 76
    expected_meta_data.format = HEADER_FORMAT.PROPRIETARY_BINARY
    _, test_meta_data = helper.framer.get_frame()
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_proprietary_binary_incomplete(helper):
    # "<incomplete binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A])
    helper.write_bytes_to_framer(data)
    helper.test_framer_errors(StopIteration)


def test_proprietary_binary_sync_error(helper):
    helper.write_file_to_framer("proprietary_binary_sync_error.BIN")
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = ne.MAX_BINARY_MESSAGE_LENGTH
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    _, test_meta_data = helper.framer.get_frame()
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_proprietary_binary_bad_crc(helper):
    # "<encrypted binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xFF])
    helper.write_bytes_to_framer(data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 30  # Unknown bytes up to 0x24 ('$') should be returned (NMEA sync was found mid-log)
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    _, test_meta_data = helper.framer.get_frame()
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_proprietary_binary_run_on_crc(helper):
    # "<encrypted binary bestpos log>FF"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC, 0xFF])
    helper.write_bytes_to_framer(data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 76
    expected_meta_data.format = HEADER_FORMAT.PROPRIETARY_BINARY
    _, test_meta_data = helper.framer.get_frame()
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_proprietary_binary_inadequate_buffer(helper):
    # "<encrypted binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    helper.write_bytes_to_framer(data)
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 76
    expected_meta_data.format = HEADER_FORMAT.PROPRIETARY_BINARY
    test_meta_data = ne.MetaData()
    helper.test_framer_errors(ne.BufferFullException, buffer_size=38)

    _, test_meta_data = helper.framer.get_frame(76)
    assert compare_metadata(test_meta_data, expected_meta_data)


def test_proprietary_binary_byte_by_byte(helper):
    # "<binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    log_size = len(data)
    remaining_bytes = log_size
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        if remaining_bytes > 0:
            helper.test_framer_errors(StopIteration)
        else:
            break
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, log_size)


def test_proprietary_binary_segmented(helper):
    # "<binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    bytes_written = 0
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_SYNC_LENGTH
    helper.test_framer_errors(StopIteration)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH
    helper.test_framer_errors(StopIteration)

    helper.write_bytes_to_framer(data[bytes_written:][:44])
    bytes_written += 44
    helper.test_framer_errors(StopIteration)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_CRC_LENGTH])
    bytes_written += ne.OEM4_BINARY_CRC_LENGTH
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, bytes_written)
    assert bytes_written == len(data)


def test_proprietary_binary_trick(helper):
    # "<binary syncs><binary sync + half header><binary sync byte 1><binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82,
         0xAA, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8,
         0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E,
         0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF,
         0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91,
         0x27, 0x6F, 0x8E, 0x0B, 0xCC])

    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 3)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 15)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, 1)
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, 76)
