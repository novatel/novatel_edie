################################################################################
#
# Copyright (c) 2020 NovAtel Inc.
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

import io
from pathlib import Path

import novatel_edie as ne


def test_read_data(hw_interface_test_resources):
    test_stream = ne.InputFileStream(str(hw_interface_test_resources / "streaminterface_testread.asc"))
    data, status = test_stream.read(20)
    assert data == b"This is a test file."


def test_read_data_wide_char_path(hw_interface_test_resources):
    test_stream = ne.InputFileStream(str(hw_interface_test_resources / "inputfilestream不同语言的文件.gps"))
    data, status = test_stream.read(69)
    assert data == b"#RANGEA,COM1,0,77.5,FINESTEERING,2195,512277.000,02000020,9691,16696;"


# ReadLine
def test_read_line(hw_interface_test_resources):
    test_stream = ne.InputFileStream(str(hw_interface_test_resources / "streaminterface_testread.asc"))
    line, read_status = test_stream.readline()
    assert line == b"This is a test file. it will\r"
    assert len(line) == read_status.current_read
    assert not read_status.eos

    line, read_status = test_stream.readline()
    assert line == b"be used to perform unit test cases\r"
    assert len(line) == read_status.current_read
    assert not read_status.eos

    line, read_status = test_stream.readline()
    assert line == b"for file stream functionalities.\r"
    assert len(line) == read_status.current_read
    assert not read_status.eos

    line, read_status = test_stream.readline()
    assert len(line) == 0
    assert len(line) == read_status.current_read
    assert read_status.eos


# Test Reset Method
def test_reset(hw_interface_test_resources):
    test_stream = ne.InputFileStream(str(hw_interface_test_resources / "streaminterface_testread.asc"))
    data, status = test_stream.read(20)
    assert data == b"This is a test file."

    test_stream.reset()
    data, status = test_stream.read(5)
    assert data == b"This "

    test_stream.reset(2, io.SEEK_SET)
    data, status = test_stream.read(5)
    assert data == b"is is"


# Test File Extension
def test_get_file_extension(hw_interface_test_resources):
    test_stream = ne.InputFileStream(str(hw_interface_test_resources / "streaminterface_testread.asc"))
    assert Path(test_stream.file_name).suffix == ".asc"


def test_get_current_file_stats(hw_interface_test_resources):
    test_stream = ne.InputFileStream(str(hw_interface_test_resources / "streaminterface_testread.asc"))
    data, status = test_stream.read(20)
    assert data == b"This is a test file."

    test_stream.reset()
    data, status = test_stream.read(5)
    assert data == b"This "

    assert 5 == test_stream.current_position
    assert 0 == test_stream.current_offset

    test_stream.reset(7)
    assert 7 == test_stream.current_position
    assert 7 == test_stream.current_offset
