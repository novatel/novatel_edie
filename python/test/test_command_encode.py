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

from pathlib import Path

import novatel_edie as ne
from novatel_edie import HEADERFORMAT, STATUS
import pytest

# -------------------------------------------------------------------------------------------------------
# Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------
class CommandEncodeTest : public .testing.Test

protected:
static JsonReader* my_json_db
static Commander* my_commander
# Per-test-suite setup
static void SetUpTestSuite():
try:
    my_json_db = ne.JsonReader()
    my_json_db.load_file(*TEST_DB_PATH)
    my_commander = ne.Commander(my_json_db)
catch (JsonReaderFailure e):
printf("%s\n", e.what())
if (my_json_db)
    {
        my_json_db = nullptr
}

if (my_commander)
    {
        my_commander = nullptr
}

# Per-test-suite teardown
static void TearDownTestSuite():
if my_json_db:
    my_json_db = nullptr

if my_commander:
    my_commander.shutdown_logger()
    my_commander = nullptr

public:

STATUS TestCommandConversion(char* command_to_encode_, uint32_t command_to_encode_length_, char* encoded_command_buffer_, uint32_t encoded_command_buffer_size_, ENCODEFORMAT format_):
return my_commander.encode(command_to_encode_, command_to_encode_length_, encoded_command_buffer_, encoded_command_buffer_size_, format_)

private:
JsonReader* CommandEncodeTest.my_json_db = nullptr
Commander* CommandEncodeTest.my_commander = nullptr
# -------------------------------------------------------------------------------------------------------
# Logger Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_LOGGER():
    level = spdlog.level.off
    assert spdlog.get("novatel_commander") != nullptr
    std.shared_ptr<spdlog.logger> novatel_commander = my_commander.get_logger()
    my_commander.set_logger_level(level)
    assert novatel_commander.level() == level

# -------------------------------------------------------------------------------------------------------
# ASCII Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_COMMAND_ENCODE_ASCII_CONFIGCODE():
    char expected_command[] = "#CONFIGCODEA,THISPORT,0,0.0,UNKNOWN,0,0.000,00000000,dbc9,0;ERASE_TABLE,\"WJ4HDW\",\"GM5Z99\",\"T2M7DP\",\"KG2T8T\",\"KF7GKR\",\"TABLECLEAR\"*69419dec\r\n"
    char command_to_encode[] = "CONFIGCODE ERASE_TABLE \"WJ4HDW\" \"GM5Z99\" \"T2M7DP\" \"KG2T8T\" \"KF7GKR\" \"TABLECLEAR\""
    char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    assert TestCommandConversion(command_to_encode, len(command_to_encode), encode_buffer, len(encode_buffer), ENCODEFORMAT.ASCII) == STATUS.SUCCESS
    assert memcmp(encode_buffer, expected_command, len(expected_command)) == 0

def test_COMMAND_ENCODE_ASCII_INSTHRESHOLDS():
    char expected_command[] = "#INSTHRESHOLDSA,THISPORT,0,0.0,UNKNOWN,0,0.000,00000000,48a5,0;LOW,0.000000000,0.000000000,0.000000000*3989c2ac\r\n"
    char command_to_encode[] = "INSTHRESHOLDS LOW 0.0 0.0 0.0"
    char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    assert TestCommandConversion(command_to_encode, len(command_to_encode), encode_buffer, len(encode_buffer), ENCODEFORMAT.ASCII) == STATUS.SUCCESS
    assert memcmp(encode_buffer, expected_command, len(expected_command)) == 0

# -------------------------------------------------------------------------------------------------------
# Binary Command Encoding Unit Tests
# -------------------------------------------------------------------------------------------------------
def test_COMMAND_ENCODE_BINARY_CONFIGCODE():
    unsigned char expected_command[]  = [ 0xAA, 0x44, 0x12, 0x1C, 0x11, 0x04, 0x00, 0xC0, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC9, 0xDB, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x57, 0x4A, 0x34, 0x48, 0x44, 0x57, 0x00, 0x00, 0x47, 0x4D, 0x35, 0x5A, 0x39, 0x39, 0x00, 0x00, 0x54, 0x32, 0x4D, 0x37, 0x44, 0x50, 0x00, 0x00, 0x4B, 0x47, 0x32, 0x54, 0x38, 0x54, 0x00, 0x00, 0x4B, 0x46, 0x37, 0x47, 0x4B, 0x52, 0x00, 0x00, 0x54, 0x41, 0x42, 0x4C, 0x45, 0x43, 0x4C, 0x45, 0x41, 0x52, 0x00, 0x00, 0x06, 0xF3, 0x54, 0x45 ]
    char command_to_encode[] = "CONFIGCODE ERASE_TABLE \"WJ4HDW\" \"GM5Z99\" \"T2M7DP\" \"KG2T8T\" \"KF7GKR\" \"TABLECLEAR\""
    char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    assert TestCommandConversion(command_to_encode, len(command_to_encode), encode_buffer, len(encode_buffer), ENCODEFORMAT.BINARY) == STATUS.SUCCESS
    assert memcmp(encode_buffer, expected_command, len(expected_command)) == 0

def test_COMMAND_ENCODE_BINARY_LOG_PARTIAL():
    char command_to_encode[] = "LOG THISPORT BESTPOSA ONCE"
    char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    assert TestCommandConversion(command_to_encode, len(command_to_encode), encode_buffer, len(encode_buffer), ENCODEFORMAT.BINARY) == STATUS.MALFORMED_INPUT

def test_COMMAND_ENCODE_BINARY_UALCONTROL():
    unsigned char expected_command[]  = [ 0xAA, 0x44, 0x12, 0x1C, 0x5B, 0x06, 0x00, 0xC0, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x49, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0xFF, 0xF8, 0x3A, 0xA7 ]
    char command_to_encode[] = "UALCONTROL ENABLE 2.0 1.0"
    char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    assert TestCommandConversion(command_to_encode, len(command_to_encode), encode_buffer, len(encode_buffer), ENCODEFORMAT.BINARY) == STATUS.SUCCESS
    assert memcmp(encode_buffer, expected_command, len(expected_command)) == 0

