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

import timeit

import novatel_edie as ne
from novatel_edie import STATUS, ENCODEFORMAT
import pytest

# -------------------------------------------------------------------------------------------------------
# Decode/Encode Benchmark Unit Tests
# -------------------------------------------------------------------------------------------------------

max_count = 1000


class Benchmarker:
    def __init__(self):
        self.header_decoder = ne.HeaderDecoder()
        self.message_decoder = ne.MessageDecoder()
        self.encoder = ne.Encoder()

    def run(self, log, encode_format):
        meta_data = ne.MetaData()
        failed_once = False
        count = 0
        start = timeit.default_timer()
        for count in range(max_count):
            status, header = self.header_decoder.decode(log, meta_data)
            if status != STATUS.SUCCESS:
                print("Failed to decode header: ", status)
                failed_once = True
                break
            body = log[meta_data.header_length:]
            status, message = self.message_decoder.decode(body, meta_data)
            if status != STATUS.SUCCESS:
                print("Failed to decode message: ", status)
                failed_once = True
                break
            status, message_data = self.encoder.encode(header, message, meta_data, encode_format)
            if status != STATUS.SUCCESS:
                print("Failed to encode message: ", status)
                failed_once = True
                break
        elapsed_seconds = timeit.default_timer() - start
        print(f"TIME ELAPSED: {elapsed_seconds} seconds.\nPS: {(float(count) / elapsed_seconds)}")
        assert not failed_once


@pytest.fixture(scope="function")
def benchmarker():
    return Benchmarker()


def test_BENCHMARK_BINARY_TO_BINARY_BESTPOS(benchmarker):
    log = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34,
         0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0,
         0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E,
         0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9])
    benchmarker.run(log, ENCODEFORMAT.BINARY)


def test_BENCHMARK_ASCII_TO_ASCII_BESTPOS(benchmarker):
    log = b"#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n"
    benchmarker.run(log, ENCODEFORMAT.ASCII)


def test_BENCHMARK_ASCII_TO_BINARY_BESTPOS(benchmarker):
    log = b"#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n"
    benchmarker.run(log, ENCODEFORMAT.BINARY)


def test_BENCHMARK_BINARY_TO_ASCII_BESTPOS(benchmarker):
    log = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34,
         0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0,
         0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E,
         0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9])
    benchmarker.run(log, ENCODEFORMAT.ASCII)


def test_BENCHMARK_ASCII_TO_FLAT_BINARY_BESTPOS(benchmarker):
    log = b"#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n"
    benchmarker.run(log, ENCODEFORMAT.FLATTENED_BINARY)


def test_BENCHMARK_BINARY_TO_FLAT_BINARY_BESTPOS(benchmarker):
    log = bytes(
        [0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34,
         0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
         0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0,
         0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E,
         0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9])
    benchmarker.run(log, ENCODEFORMAT.FLATTENED_BINARY)
