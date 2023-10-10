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
#  DESCRIPTION: FileStream Unit Test.
#
################################################################################

import novatel_edie as ne
import pytest


def test_constructors():
    file = "filestream不同语言的文件.gps"
    test_stream = ne.FileStream(file)
    assert test_stream.file_name == file
    with pytest.raises(Exception):
        ne.FileStream(None)


@pytest.mark.parametrize("mode", [ne.FILEMODES.OUTPUT, ne.FILEMODES.APPEND, ne.FILEMODES.INPUT,
                                  ne.FILEMODES.INSERT, ne.FILEMODES.TRUNCATE])
def test_wide_char_open_file(hw_interface_test_resources, mode):
    test_stream = ne.FileStream(str(hw_interface_test_resources / "filestream不同语言的文件.gps"))
    test_stream.open(mode)
    assert not test_stream.stream_failed
    test_stream.close()


def test_exception(hw_interface_test_resources):
    with pytest.raises(OSError) as excinfo:
        test_stream = ne.FileStream(str(hw_interface_test_resources / "abcd.xyz"))
        test_stream.close()
    assert "close file failed" in str(excinfo.value)
