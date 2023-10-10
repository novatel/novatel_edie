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
from novatel_edie import STATUS, ENCODEFORMAT
import pytest

# -------------------------------------------------------------------------------------------------------
# Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------

@pytest.fixture(scope="function")
def commander():
    return ne.Commander()

# -------------------------------------------------------------------------------------------------------
# Logger Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_LOGGER():
    name = "novatel_commander"
    assert ne.Logger.get(name) is None
    level = ne.LogLevel.OFF
    logger = ne.Commander().logger
    logger.set_level(level)
    assert logger.name == name
    assert logger.level == level

# -------------------------------------------------------------------------------------------------------
# ASCII Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_COMMAND_ENCODE_ASCII_CONFIGCODE(commander):
    expected_command = b"#CONFIGCODEA,THISPORT,0,0.0,UNKNOWN,0,0.000,00000000,dbc9,0;ERASE_TABLE,\"WJ4HDW\",\"GM5Z99\",\"T2M7DP\",\"KG2T8T\",\"KF7GKR\",\"TABLECLEAR\"*69419dec\r\n"
    command_to_encode = b"CONFIGCODE ERASE_TABLE \"WJ4HDW\" \"GM5Z99\" \"T2M7DP\" \"KG2T8T\" \"KF7GKR\" \"TABLECLEAR\""
    status, encoded_command = commander.encode(command_to_encode, ENCODEFORMAT.ASCII)
    assert status == STATUS.SUCCESS
    assert encoded_command == expected_command

def test_COMMAND_ENCODE_ASCII_INSTHRESHOLDS(commander):
    expected_command = b"#INSTHRESHOLDSA,THISPORT,0,0.0,UNKNOWN,0,0.000,00000000,48a5,0;LOW,0.000000000,0.000000000,0.000000000*3989c2ac\r\n"
    command_to_encode = b"INSTHRESHOLDS LOW 0.0 0.0 0.0"
    status, encoded_command = commander.encode(command_to_encode, ENCODEFORMAT.ASCII)
    assert status == STATUS.SUCCESS
    assert encoded_command == expected_command

# -------------------------------------------------------------------------------------------------------
# Binary Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_COMMAND_ENCODE_BINARY_CONFIGCODE(commander):
    expected_command = bytes([0xAA, 0x44, 0x12, 0x1C, 0x11, 0x04, 0x00, 0xC0, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC9, 0xDB, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x57, 0x4A, 0x34, 0x48, 0x44, 0x57, 0x00, 0x00, 0x47, 0x4D, 0x35, 0x5A, 0x39, 0x39, 0x00, 0x00, 0x54, 0x32, 0x4D, 0x37, 0x44, 0x50, 0x00, 0x00, 0x4B, 0x47, 0x32, 0x54, 0x38, 0x54, 0x00, 0x00, 0x4B, 0x46, 0x37, 0x47, 0x4B, 0x52, 0x00, 0x00, 0x54, 0x41, 0x42, 0x4C, 0x45, 0x43, 0x4C, 0x45, 0x41, 0x52, 0x00, 0x00, 0x06, 0xF3, 0x54, 0x45])
    command_to_encode = b"CONFIGCODE ERASE_TABLE \"WJ4HDW\" \"GM5Z99\" \"T2M7DP\" \"KG2T8T\" \"KF7GKR\" \"TABLECLEAR\""
    status, encoded_command = commander.encode(command_to_encode, ENCODEFORMAT.BINARY)
    assert status == STATUS.SUCCESS
    assert encoded_command == expected_command

def test_COMMAND_ENCODE_BINARY_LOG_PARTIAL(commander):
    command_to_encode = b"LOG THISPORT BESTPOSA ONCE"
    status, encoded_command = commander.encode(command_to_encode, ENCODEFORMAT.BINARY)
    assert status == STATUS.MALFORMED_INPUT

def test_COMMAND_ENCODE_BINARY_UALCONTROL(commander):
    expected_command = bytes([0xAA, 0x44, 0x12, 0x1C, 0x5B, 0x06, 0x00, 0xC0, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x49, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0xFF, 0xF8, 0x3A, 0xA7])
    command_to_encode = b"UALCONTROL ENABLE 2.0 1.0"
    status, encoded_command = commander.encode(command_to_encode, ENCODEFORMAT.BINARY)
    assert status == STATUS.SUCCESS
    assert encoded_command == expected_command

