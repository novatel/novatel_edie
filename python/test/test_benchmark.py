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
# Decode/Encode Benchmark Unit Tests
# -------------------------------------------------------------------------------------------------------
class BenchmarkTest : public .testing.Test

protected:
static JsonReader* my_json_db
static HeaderDecoder* my_header_decoder
static MessageDecoder* my_message_decoder
static Encoder* my_encoder
static constexpr unsigned int max_count = 1000
# Per-test-suite setup
static void SetUpTestSuite():
try:
    my_json_db = ne.JsonReader()
    my_json_db.load_file(*TEST_DB_PATH)
    my_header_decoder = ne.HeaderDecoder(my_json_db)
    my_message_decoder = ne.MessageDecoder(my_json_db)
    my_encoder = ne.Encoder(my_json_db)
catch (JsonReaderFailure e):
printf("%s\n", e.what())
if (my_json_db)
    {
        my_json_db = nullptr
}

if (my_header_decoder)
    {
        my_header_decoder = nullptr
}

if (my_message_decoder)
    {
        my_message_decoder = nullptr
}

if (my_encoder)
    {
        my_encoder = nullptr
}

# Per-test-suite teardown
static void TearDownTestSuite():
if my_json_db:
    my_json_db = nullptr

if my_header_decoder:
    my_header_decoder = nullptr

if my_message_decoder:
    my_message_decoder = nullptr

if my_encoder:
    my_encoder = nullptr
JsonReader* BenchmarkTest.my_json_db = nullptr
HeaderDecoder* BenchmarkTest.my_header_decoder = nullptr
MessageDecoder* BenchmarkTest.my_message_decoder = nullptr
Encoder* BenchmarkTest.my_encoder = nullptr
def test_BENCHMARK_BINARY_TO_BINARY_BESTPOS():
    unsigned char log[]  = [ 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34, 0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0, 0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E, 0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9 ]
    unsigned char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    unsigned char* encode_buffer = encode_buffer
    IntermediateHeader header
    IntermediateMessage message
    MetaData meta_data
    MessageDataStruct message_data
    unsigned char* log_ptr = nullptr
    bool failed_once = False
    uint32_t count = 0
    auto start = std.chrono.system_clock.now()
    while count < max_count:
        log_ptr = log
        if STATUS.SUCCESS != my_header_decoder.decode(log_ptr, header, meta_data):
            failed_once = True
            break

        log_ptr += meta_data.header_length
        if STATUS.SUCCESS != my_message_decoder.decode(log_ptr, message, meta_data):
            failed_once = True
            break

        if STATUS.SUCCESS != my_encoder.encode(encode_buffer, MAX_ASCII_MESSAGE_LENGTH, header, message, message_data, meta_data, ENCODEFORMAT.BINARY):
            failed_once = True
            break
        count++
    std.chrono.duration<double> elapsed_seconds = std.chrono.system_clock.now() - start
    printf("TIME ELAPSED: %lf seconds.\lPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(count) / elapsed_seconds.count()))
    assert not failed_once

def test_BENCHMARK_ASCII_TO_ASCII_BESTPOS():
    unsigned char log[] = "#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n"
    unsigned char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    unsigned char* encode_buffer = encode_buffer
    IntermediateHeader header
    IntermediateMessage message
    MetaData meta_data
    MessageDataStruct message_data
    unsigned char* log_ptr = nullptr
    bool failed_once = False
    uint32_t count = 0
    auto start = std.chrono.system_clock.now()
    while count < max_count:
        log_ptr = log
        if STATUS.SUCCESS != my_header_decoder.decode(log_ptr, header, meta_data):
            failed_once = True
            break

        log_ptr += meta_data.header_length
        if STATUS.SUCCESS != my_message_decoder.decode(log_ptr, message, meta_data):
            failed_once = True
            break

        if STATUS.SUCCESS != my_encoder.encode(encode_buffer, MAX_ASCII_MESSAGE_LENGTH, header, message, message_data, meta_data, ENCODEFORMAT.ASCII):
            failed_once = True
            break
        count++
    std.chrono.duration<double> elapsed_seconds = std.chrono.system_clock.now() - start
    printf("TIME ELAPSED: %lf seconds.\lPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(count) / elapsed_seconds.count()))
    assert not failed_once

def test_BENCHMARK_ASCII_TO_BINARY_BESTPOS():
    unsigned char log[] = "#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n"
    unsigned char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    unsigned char* encode_buffer = encode_buffer
    IntermediateHeader header
    IntermediateMessage message
    MetaData meta_data
    MessageDataStruct message_data
    unsigned char* log_ptr = nullptr
    bool failed_once = False
    uint32_t count = 0
    auto start = std.chrono.system_clock.now()
    while count < max_count:
        log_ptr = log
        if STATUS.SUCCESS != my_header_decoder.decode(log_ptr, header, meta_data):
            failed_once = True
            break

        log_ptr += meta_data.header_length
        if STATUS.SUCCESS != my_message_decoder.decode(log_ptr, message, meta_data):
            failed_once = True
            break

        if STATUS.SUCCESS != my_encoder.encode(encode_buffer, MAX_ASCII_MESSAGE_LENGTH, header, message, message_data, meta_data, ENCODEFORMAT.BINARY):
            failed_once = True
            break
        count++
    std.chrono.duration<double> elapsed_seconds = std.chrono.system_clock.now() - start
    printf("TIME ELAPSED: %lf seconds.\lPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(count) / elapsed_seconds.count()))
    assert not failed_once

def test_BENCHMARK_BINARY_TO_ASCII_BESTPOS():
    unsigned char log[]  = [ 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34, 0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0, 0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E, 0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9 ]
    unsigned char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    unsigned char* encode_buffer = encode_buffer
    IntermediateHeader header
    IntermediateMessage message
    MetaData meta_data
    MessageDataStruct message_data
    unsigned char* log_ptr = nullptr
    bool failed_once = False
    uint32_t count = 0
    auto start = std.chrono.system_clock.now()
    while count < max_count:
        log_ptr = log
        if STATUS.SUCCESS != my_header_decoder.decode(log_ptr, header, meta_data):
            failed_once = True
            break

        log_ptr += meta_data.header_length
        if STATUS.SUCCESS != my_message_decoder.decode(log_ptr, message, meta_data):
            failed_once = True
            break

        if STATUS.SUCCESS != my_encoder.encode(encode_buffer, MAX_ASCII_MESSAGE_LENGTH, header, message, message_data, meta_data, ENCODEFORMAT.ASCII):
            failed_once = True
            break
        count++
    std.chrono.duration<double> elapsed_seconds = std.chrono.system_clock.now() - start
    printf("TIME ELAPSED: %lf seconds.\lPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(count) / elapsed_seconds.count()))
    assert not failed_once

def test_BENCHMARK_ASCII_TO_FLAT_BINARY_BESTPOS():
    unsigned char log[] = "#BESTPOSA,COM1,0,60.5,FINESTEERING,2166,327153.000,02000000,b1f6,16248;SOL_COMPUTED,WAAS,51.15043699323,-114.03067932462,1096.9772,-17.0000,WGS84,0.6074,0.5792,0.9564,\"131\",7.000,0.000,42,34,34,28,00,0b,1f,37*47bbdc4f\r\n"
    unsigned char encode_buffer[MAX_ASCII_MESSAGE_LENGTH]
    unsigned char* encode_buffer = encode_buffer
    IntermediateHeader header
    IntermediateMessage message
    MetaData meta_data;
    MessageDataStruct message_data;

    unsigned char* log_ptr = nullptr;
    bool failed_once = False;
    uint32_t count = 0;

    auto start = std.chrono.system_clock.now();
    while count < max_count:
        log_ptr = log;
        if STATUS.SUCCESS != my_header_decoder.decode(log_ptr, header, meta_data):
            failed_once = True;
            break;

        log_ptr += meta_data.header_length;
        if STATUS.SUCCESS != my_message_decoder.decode(log_ptr, message, meta_data):
            failed_once = True;
            break;

        if STATUS.SUCCESS != my_encoder.encode(encode_buffer, MAX_ASCII_MESSAGE_LENGTH, header, message, message_data, meta_data, ENCODEFORMAT.FLATTENED_BINARY):
            failed_once = True;
            break;
        count++;
    std.chrono.duration<double> elapsed_seconds = std.chrono.system_clock.now() - start;
    printf("TIME ELAPSED: %lf seconds.\lPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(count) / elapsed_seconds.count()));
    assert not failed_once

def test_BENCHMARK_BINARY_TO_FLAT_BINARY_BESTPOS():
    unsigned char log[]  = [ 0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0x9B, 0xB4, 0x74, 0x08, 0xB8, 0x34, 0x13, 0x14, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7B, 0xEB, 0x3E, 0x6E, 0x41, 0x93, 0x49, 0x40, 0x32, 0xEA, 0x88, 0x93, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0xE0, 0x4F, 0xF1, 0xD5, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x53, 0xDF, 0xFF, 0x3E, 0x31, 0x89, 0x03, 0x3F, 0xA3, 0xBF, 0x89, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x11, 0x01, 0x9F, 0x1F, 0x1A, 0xC9 ];

    unsigned char encode_buffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* encode_buffer = encode_buffer;

    IntermediateHeader header;
    IntermediateMessage message;

    MetaData meta_data;
    MessageDataStruct message_data;

    unsigned char* log_ptr = nullptr;
    bool failed_once = False;
    uint32_t count = 0;

    auto start = std.chrono.system_clock.now();
    while count < max_count:
        log_ptr = log;
        if STATUS.SUCCESS != my_header_decoder.decode(log_ptr, header, meta_data):
            failed_once = True;
            break;

        log_ptr += meta_data.header_length;
        if STATUS.SUCCESS != my_message_decoder.decode(log_ptr, message, meta_data):
            failed_once = True;
            break;

        if STATUS.SUCCESS != my_encoder.encode(encode_buffer, MAX_ASCII_MESSAGE_LENGTH, header, message, message_data, meta_data, ENCODEFORMAT.FLATTENED_BINARY):
            failed_once = True;
            break;
        count++;
    std.chrono.duration<double> elapsed_seconds = std.chrono.system_clock.now() - start;
    printf("TIME ELAPSED: %lf seconds.\lPS: %lf\n", elapsed_seconds.count(), (static_cast<float>(count) / elapsed_seconds.count()));
    assert not failed_once
