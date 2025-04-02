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

@pytest.fixture(autouse=True)
def setup_and_teardown():
    # Setup

    # Test Run
    yield

    # Teardown
    framer_manager_instance = ne.FramerManager.get_instance()
    framer_manager_instance.flush()
    framer_manager_instance.reset_all_framer_states()
    framer_manager_instance.reset_all_metadata_states()

class Helper:
    def __init__(self, test_resources_path):
        self.test_resources = test_resources_path
        framer_manager_instance = ne.FramerManager.get_instance()
        try:
            framer_manager_instance.register_framer("NOVATEL", ne.create_framer(), ne.create_metadata())
        except RuntimeError:
            print("Framer Already Registered")

        self.framer = framer_manager_instance.framer_instance("NOVATEL")
        framer_manager_instance.set_report_unknown_bytes(True)
        framer_manager_instance.set_logger_level(ne.LogLevel.INFO)
        self.framer.set_payload_only(False)

    def write_file_to_framer(self, filename):
        data = (self.test_resources / filename).read_bytes()
        framer_manager_instance = ne.FramerManager.get_instance()
        bytes_written = framer_manager_instance.write(data)
        assert bytes_written == len(data)

    def write_bytes_to_framer(self, data):
        framer_manager_instance = ne.FramerManager.get_instance()
        assert framer_manager_instance.write(data) == len(data)

    def test_framer(self, expected_header_format, expected_status, length, buffer_length=None):
        framer_manager_instance = ne.FramerManager.get_instance()
        expected_meta_data = ne.MetaData()
        if buffer_length is None:
            status, buffer = framer_manager_instance.get_frame(length)
        else:
            status, buffer = framer_manager_instance.get_frame(buffer_length)
        assert status == expected_status
        if length is not None:
            expected_meta_data.length = length
        expected_meta_data.format = expected_header_format
        test_meta_data = framer_manager_instance.active_metadata()
        compare_metadata(test_meta_data, expected_meta_data, ignore_length=length is None)

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
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 12, buffer_length=ne.MAX_BINARY_MESSAGE_LENGTH)
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, STATUS.SUCCESS, 76, buffer_length=ne.MAX_BINARY_MESSAGE_LENGTH)

def test_proprietary_binary_incomplete(helper):
    # "<incomplete binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, STATUS.INCOMPLETE, len(data), buffer_length=ne.MAX_BINARY_MESSAGE_LENGTH)


def test_proprietary_binary_sync_error(helper):
    helper.write_file_to_framer("proprietary_binary_sync_error.BIN")
    
    framer_manager_instance = ne.FramerManager.get_instance()
    status, frame = framer_manager_instance.get_frame(ne.MAX_BINARY_MESSAGE_LENGTH)
    test_meta_data = framer_manager_instance.active_metadata()
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = 3  # number of binary sync bytes
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    assert status == STATUS.UNKNOWN
    compare_metadata(test_meta_data, expected_meta_data)

    status, frame = framer_manager_instance.get_frame(ne.MAX_BINARY_MESSAGE_LENGTH)
    test_meta_data = framer_manager_instance.active_metadata()
    expected_meta_data = ne.MetaData()
    expected_meta_data.length = ne.MAX_BINARY_MESSAGE_LENGTH
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    assert status == STATUS.UNKNOWN
    compare_metadata(test_meta_data, expected_meta_data)


def test_proprietary_binary_bad_crc(helper):
    # "<encrypted binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 3, ne.MAX_BINARY_MESSAGE_LENGTH)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, len(data)-3, ne.MAX_BINARY_MESSAGE_LENGTH)


def test_proprietary_binary_run_on_crc(helper):
    # "<encrypted binary bestpos log>FF"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC, 0xFF])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, STATUS.SUCCESS, 76, ne.MAX_BINARY_MESSAGE_LENGTH)


def test_proprietary_binary_inadequate_buffer(helper):
    # "<encrypted binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, STATUS.BUFFER_FULL, len(data), buffer_length=len(data) - 1)
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, STATUS.SUCCESS, len(data), buffer_length=len(data))


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
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    framer_manager_instance = ne.FramerManager.get_instance()
    while True:
        helper.write_bytes_to_framer(data[log_size - remaining_bytes:][:1])
        remaining_bytes -= 1
        bytes_written = log_size - remaining_bytes
        expected_meta_data.length = bytes_written
        if expected_meta_data.length >= ne.OEM4_BINARY_SYNC_LENGTH - 1:
            expected_meta_data.format = HEADER_FORMAT.PROPRIETARY_BINARY

        if remaining_bytes > 0:
            status, frame = framer_manager_instance.get_frame(ne.MAX_BINARY_MESSAGE_LENGTH)
            assert status == STATUS.INCOMPLETE
            if bytes_written > ne.OEM4_BINARY_SYNC_LENGTH:
                test_meta_data = framer_manager_instance.active_metadata()
                compare_metadata(test_meta_data, expected_meta_data)
        else:
            break
    expected_meta_data.length = log_size
    status, frame = framer_manager_instance.get_frame(bytes_written)
    test_meta_data = framer_manager_instance.active_metadata()
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)


def test_proprietary_binary_segmented(helper):
    # "<binary bestpos log>"
    data = bytes(
        [0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6,
         0xC7, 0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8,
         0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
         0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27,
         0x6F, 0x8E, 0x0B, 0xCC])
    bytes_written = 0
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.PROPRIETARY_BINARY
    framer_manager_instance = ne.FramerManager.get_instance()
    
    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_SYNC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = framer_manager_instance.get_frame(ne.MAX_BINARY_MESSAGE_LENGTH)
    test_meta_data = framer_manager_instance.active_metadata()
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH])
    bytes_written += ne.OEM4_BINARY_HEADER_LENGTH - ne.OEM4_BINARY_SYNC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = framer_manager_instance.get_frame(ne.MAX_BINARY_MESSAGE_LENGTH)
    test_meta_data = framer_manager_instance.active_metadata()
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:44])
    bytes_written += 44
    expected_meta_data.length = bytes_written
    status, frame = framer_manager_instance.get_frame(ne.MAX_BINARY_MESSAGE_LENGTH)
    test_meta_data = framer_manager_instance.active_metadata()
    assert status == STATUS.INCOMPLETE
    compare_metadata(test_meta_data, expected_meta_data)

    helper.write_bytes_to_framer(data[bytes_written:][:ne.OEM4_BINARY_CRC_LENGTH])
    bytes_written += ne.OEM4_BINARY_CRC_LENGTH
    expected_meta_data.length = bytes_written
    status, frame = framer_manager_instance.get_frame(ne.MAX_BINARY_MESSAGE_LENGTH)
    test_meta_data = framer_manager_instance.active_metadata()
    status.raise_on_error()
    compare_metadata(test_meta_data, expected_meta_data)
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
    expected_meta_data = ne.MetaData()
    expected_meta_data.format = HEADER_FORMAT.UNKNOWN
    helper.write_bytes_to_framer(data)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 3, ne.MAX_BINARY_MESSAGE_LENGTH)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 3, ne.MAX_BINARY_MESSAGE_LENGTH)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 12, ne.MAX_BINARY_MESSAGE_LENGTH)
    helper.test_framer(HEADER_FORMAT.UNKNOWN, STATUS.UNKNOWN, 1, ne.MAX_BINARY_MESSAGE_LENGTH)
    helper.test_framer(HEADER_FORMAT.PROPRIETARY_BINARY, STATUS.SUCCESS, 76, ne.MAX_BINARY_MESSAGE_LENGTH)
    

