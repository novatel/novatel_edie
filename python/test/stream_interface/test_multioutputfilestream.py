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
#
#  DESCRIPTION: Multi Output File Stream Unit Test.
#
#
################################################################################

from pathlib import Path

import novatel_edie as ne
import pytest


def test_configure_split_by_log():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_split_by_log(True)
    assert test_stream._is_file_split
    test_stream.configure_split_by_log(False)
    assert not test_stream._is_file_split


# Test the ConfigureBaseFileName method.
def test_configure_base_file_name():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_base_file_name("outputfilestream_file13")
    assert "outputfilestream_file13" == test_stream._base_name
    assert "DefaultExt" == test_stream._extension_name

    test_stream.configure_base_file_name("outputfilestream_file13.txt")
    assert "outputfilestream_file13" == test_stream._base_name
    assert "txt" == test_stream._extension_name


def test_configure_base32_string_file_name():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_base_file_name("outputfilestream_不同语言的文件")
    assert "outputfilestream_不同语言的文件" == test_stream._base_name
    assert "DefaultExt" == test_stream._extension_name

    test_stream.configure_base_file_name("outputfilestream_不同语言的文件.txt")
    assert "outputfilestream_不同语言的文件" == test_stream._base_name
    assert "txt" == test_stream._extension_name


# Test the SelectLogFile method.
def test_select32_string_log_file():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_base_file_name("outputfilestream不同_file14.asc")
    test_stream.select_log_file("bestpos")
    file_map = list(test_stream.file_map.items())
    file_name = f"{test_stream._base_name}_bestpos.{test_stream._extension_name}"
    assert file_name == file_map[0][0]
    assert file_name == file_map[0][1].file_name

    test_stream.configure_base_file_name("outputfilestream不同_file15")
    test_stream.set_extension_name("DefaultExt")
    test_stream.select_log_file("bestpos")
    file_name = f"{test_stream._base_name}_bestpos"
    file_map = list(test_stream.file_map.items())
    assert len(file_map) == 2
    assert file_name == file_map[1][0]
    assert file_name == file_map[1][1].file_name


def test_write_data_wide_file():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_split_by_log(True)
    test_stream.configure_base_file_name("Log不同.txt")
    command = b"HELLO"
    len = test_stream.write(command, "BESTPOS", 0, ne.TIME_STATUS.UNKNOWN, 0, 0.0)
    assert len == 5
    assert Path("Log不同_BESTPOS.txt").exists()


def test_configure_split_by_size():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_split_by_size(1)
    assert test_stream._is_file_split
    assert test_stream._file_split_size == 1

    test_stream.configure_base_file_name("Log.txt")
    command = b"HELLO"
    len = test_stream.write(command, "", 1, ne.TIME_STATUS.UNKNOWN, 0, 0.0)
    assert len == 5
    assert Path("Log_Part0.txt").exists()

    test_stream.clear_file_stream_map()
    with pytest.raises(Exception) as excinfo:
        test_stream.configure_split_by_size(0)
    assert str(excinfo.value) == "File Split by Size not valid"


def test_configure_split_by_time():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_split_by_time(0.01)
    test_stream.configure_base_file_name("Log.txt")
    command = b"HELLO"
    len = test_stream.write(command, "", 0, ne.TIME_STATUS.UNKNOWN, 0, 0.01)
    assert len == 5
    assert Path("Log_Part0.txt").exists()

    with pytest.raises(Exception) as excinfo:
        test_stream.configure_split_by_time(0.0)  # 0
    assert str(excinfo.value) == "File Split by time not valid"


def test_select_time_file():
    test_stream = ne.MultiOutputFileStream()
    test_stream.configure_split_by_time(0.01)
    test_stream.select_file_stream("Log.txt")
    test_stream.select_time_file(ne.TIME_STATUS.UNKNOWN, 0, 0.01)
    assert test_stream._time_in_seconds == 0.0
    assert test_stream._start_time_in_seconds == 0.0
    assert test_stream._week == 0
    assert test_stream._start_week == 0

    test_stream.select_time_file(ne.TIME_STATUS.SATTIME, 0, 0.0)
    assert test_stream._time_in_seconds == 0.0
    assert test_stream._start_time_in_seconds == 0.0
    assert test_stream._week == 0
    assert test_stream._start_week == 0
