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
    test_stream = ne.FileStream("filestream_file1.asc")
    assert test_stream.file_name == "filestream_file1.asc"
    with pytest.raises(Exception):
        test_stream = ne.FileStream(None)


def test_constructor_wide_char():
    test_stream = ne.FileStream("filestream不同语言的文件.gps")
    assert test_stream.file_name == "filestream不同语言的文件.gps"


def test_wide_char_open_file(hw_interface_test_resources):
    # try:
    test_stream = ne.FileStream(str(hw_interface_test_resources / "filestream不同语言的文件.gps"))
    test_stream.open(ne.FILEMODES.OUTPUT)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.APPEND)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.INPUT)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.INSERT)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.TRUNCATE)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    # catch (except ne)
    # {
    #    assert std.string(ne.buffer).find("file  cannot open") != std.string.npos
    # }
    # catch (...)
    # {
    #    delete test_stream
    # }

def test_open_file(hw_interface_test_resources):
    # try:
    test_stream = ne.FileStream(str(hw_interface_test_resources / "ne.FileStream_file2.asc"))

    test_stream.open(ne.FILEMODES.OUTPUT)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.APPEND)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.INPUT)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.INSERT)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()

    test_stream.open(ne.FILEMODES.TRUNCATE)
    assert not test_stream.my_file_stream.fail()
    test_stream.close()


    #    delete test_stream
    # }
    # catch (except ne)
    # {
    #    assert std.string(ne.buffer).find("file  cannot open") != std.string.npos
    # }
    # catch (...)
    # {
    #    delete test_stream
    # }

def test_exception(hw_interface_test_resources):
    with pytest.raises(Exception):
        test_stream = ne.FileStream(str(hw_interface_test_resources / "abcd.xyz"))
