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
# \file test_command_decode.py
# \brief Unit tests for the CommandDecoder.
################################################################################

import novatel_edie as ne
import novatel_edie.oem as oem
from novatel_edie.oem import enums, messages
import pytest

@pytest.fixture(scope="function")
def command_decoder():
    return oem.CommandDecoder()

def test_decode_log(command_decoder):
    command = command_decoder.decode(b"LOG BESTXYZA ONCE")
    assert isinstance(command, messages.LOG)
    assert command.header.message_id == 1
    assert command.log_port_address == enums.PortAddress.THISPORT
    assert command.trigger == enums.LogTrigger.ONCE
    assert command.hold == enums.Hold.NOHOLD

def test_decode_log_payload(command_decoder):
    command = command_decoder.decode(b"LOG COM1 BESTXYZA ONCE")
    assert command.header.message_id == 1
    assert command.message_id & 0xFFFF == 241
    assert command.log_port_address == enums.PortAddress.COM1

def test_decode_to_ascii(command_decoder):
    command = command_decoder.decode(b"ETHCONFIG ETHA MDI")
    assert b";ETHA,AUTO,AUTO,MDI,AUTO*" in command.to_ascii().message

def test_decode_to_binary(command_decoder):
    command = command_decoder.decode(b"LOG THISPORT BESTPOSA ONCE\r\n")
    assert command.to_binary().message[:3] == bytes([0xAA, 0x44, 0x12])

@pytest.mark.parametrize("command", [b"ECHO CM1 ON", b"ECHO COM1 ON ON", b"LOG", b"SEND COM1"])
def test_decode_malformed(command_decoder, command):
    with pytest.raises(ne.MalformedInputException):
        command_decoder.decode(command)

def test_decode_unknown_command(command_decoder):
    with pytest.raises(ne.NoDefinitionException):
        command_decoder.decode(b"NOTACOMMAND ON")
