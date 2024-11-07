// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2022. All rights reserved.                          |
// |                                                                             |
// |  Permission is hereby granted, free of charge, to any person obtaining a    |
// |  copy of this software and associated documentation files (the "Software"), |
// |  to deal in the Software without restriction, including without limitation  |
// |  the rights to use, copy, modify, merge, publish, distribute, sublicense,   |
// |  and/or sell copies of the Software, and to permit persons to whom the      |
// |  Software is furnished to do so, subject to the following conditions:       |
// |                                                                             |
// |  The above copyright notice and this permission notice shall be included    |
// |  in all copies or substantial portions of the Software.                     |
// |                                                                             |
// |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR |
// |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   |
// |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    |
// |  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER |
// |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    |
// |  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        |
// |  DEALINGS IN THE SOFTWARE.                                                  |
// |                                                                             |
// ===============================================================================
// ! \file benchmark.cpp
// ===============================================================================

#include <algorithm>
#include <numeric>
#include <vector>

#include <benchmark/benchmark.h>
#include <novatel_edie/decoders/oem/encoder.hpp>
#include <novatel_edie/decoders/oem/message_decoder.hpp>

#include "novatel_edie/decoders/oem/header_decoder.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

static void LoadJson(benchmark::State& state)
{
    JsonReader jsonReader;

    for ([[maybe_unused]] auto _ : state) { jsonReader.LoadFile(std::getenv("TEST_DATABASE_PATH")); }
}

BENCHMARK(LoadJson);

template <size_t N> static void DecodeLog(benchmark::State& state, const unsigned char (&data)[N])
{
    JsonReader jsonReader;
    jsonReader.LoadFile(std::getenv("TEST_DATABASE_PATH"));
    const HeaderDecoder headerDecoder(&jsonReader);
    const MessageDecoder messageDecoder(&jsonReader);

    const unsigned char* dataPtr = data;

    for ([[maybe_unused]] auto _ : state)
    {
        MetaDataStruct metaData;
        IntermediateHeader header;
        std::vector<FieldContainer> message;

        (void)headerDecoder.Decode(dataPtr, header, metaData);
        dataPtr += metaData.uiHeaderLength;
        (void)messageDecoder.Decode(dataPtr, message, metaData);
    }

    state.counters["logs_per_second"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

static void DecodeFlattenedBinaryLog(benchmark::State& state)
{
    constexpr unsigned char data[] = {0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
                                      0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                                      0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
                                      0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
                                      0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89};

    DecodeLog(state, data);
}

static void DecodeAsciiLog(benchmark::State& state)
{
    constexpr unsigned char data[] =
        "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17."
        "0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";

    DecodeLog(state, data);
}

static void DecodeAbbrevAsciiLog(benchmark::State& state)
{
    constexpr unsigned char data[] = "<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
                                     "<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 "
                                     "\"\" 0.000 0.000 35 30 30 30 00 06 39 33\r\n[COM1]";
    DecodeLog(state, data);
}

static void DecodeBinaryLog(benchmark::State& state)
{
    constexpr unsigned char data[] = {0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
                                      0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                                      0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
                                      0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
                                      0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89};

    DecodeLog(state, data);
}

static void DecodeJsonLog(benchmark::State& state)
{
    // clang-format off
    constexpr unsigned char data[] = R"({"header": {"message": "BESTSATS","id": 1194,"port": "COM1","sequence_num": 0,"percent_idle_time": 50.0,"time_status": "FINESTEERING","week": 2167,"seconds": 244820.000,"receiver_status": 33554432,"HEADER_reserved1": 48645,"receiver_sw_version": 16248},"body": {"satellite_entries": [{"system_type": "GPS","id": "2","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "20","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "29","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "13","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "16","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "18","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "25","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "5","status": "GOOD","status_mask": 3},{"system_type": "GPS","id": "26","status": "GOOD","status_mask": 7},{"system_type": "GPS","id": "23","status": "GOOD","status_mask": 7},{"system_type": "QZSS","id": "194","status": "SUPPLEMENTARY","status_mask": 7},{"system_type": "SBAS","id": "131","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "133","status": "NOTUSED","status_mask": 0},{"system_type": "SBAS","id": "138","status": "NOTUSED","status_mask": 0},{"system_type": "GLONASS","id": "8+6","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "9-2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "1+1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "24+2","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "2-4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "17+4","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "16-1","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "18-3","status": "GOOD","status_mask": 3},{"system_type": "GLONASS","id": "15","status": "GOOD","status_mask": 3},{"system_type": "GALILEO","id": "26","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "12","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "19","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "31","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "33","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "8","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "GALILEO","id": "7","status": "GOOD","status_mask": 15},{"system_type": "GALILEO","id": "24","status": "GOOD","status_mask": 15},{"system_type": "BEIDOU","id": "35","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "29","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "25","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "20","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "22","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "44","status": "LOCKEDOUT","status_mask": 0},{"system_type": "BEIDOU","id": "57","status": "NOEPHEMERIS","status_mask": 0},{"system_type": "BEIDOU","id": "12","status": "ELEVATIONERROR","status_mask": 0},{"system_type": "BEIDOU","id": "24","status": "SUPPLEMENTARY","status_mask": 1},{"system_type": "BEIDOU","id": "19","status": "SUPPLEMENTARY","status_mask": 1}]}})";
    // clang-format on
    DecodeLog(state, data);
}

BENCHMARK(DecodeFlattenedBinaryLog);
BENCHMARK(DecodeAsciiLog);
BENCHMARK(DecodeAbbrevAsciiLog);
BENCHMARK(DecodeBinaryLog);
BENCHMARK(DecodeJsonLog);

template <ENCODE_FORMAT Format> static void EncodeLog(benchmark::State& state)
{
    constexpr unsigned char data[] = {0xAA, 0x44, 0x12, 0x1C, 0x2A, 0x00, 0x00, 0x20, 0x48, 0x00, 0x00, 0x00, 0xA3, 0xB4, 0x73, 0x08, 0x98, 0x74,
                                      0xA8, 0x13, 0x00, 0x00, 0x00, 0x02, 0xF6, 0xB1, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
                                      0xFC, 0xAB, 0xE1, 0x82, 0x41, 0x93, 0x49, 0x40, 0xBA, 0x32, 0x86, 0x8A, 0xF6, 0x81, 0x5C, 0xC0, 0x00, 0x10,
                                      0xE5, 0xDF, 0x71, 0x23, 0x91, 0x40, 0x00, 0x00, 0x88, 0xC1, 0x3D, 0x00, 0x00, 0x00, 0x24, 0x21, 0xA5, 0x3F,
                                      0xF1, 0x8F, 0x8F, 0x3F, 0x43, 0x74, 0x3C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x15, 0x15, 0x15, 0x00, 0x00, 0x02, 0x11, 0x01, 0x55, 0xCE, 0xC3, 0x89};

    JsonReader jsonReader;
    jsonReader.LoadFile(std::getenv("TEST_DATABASE_PATH"));
    const HeaderDecoder headerDecoder(&jsonReader);
    const MessageDecoder messageDecoder(&jsonReader);
    const Encoder encoder(&jsonReader);

    MetaDataStruct metaData;
    MessageDataStruct messageData;
    IntermediateHeader header;
    std::vector<FieldContainer> message;

    const unsigned char* tempPtr = data;

    (void)headerDecoder.Decode(tempPtr, header, metaData);
    tempPtr += metaData.uiHeaderLength;
    (void)messageDecoder.Decode(tempPtr, message, metaData);

    unsigned char encodeBuffer[MAX_ASCII_MESSAGE_LENGTH];

    for ([[maybe_unused]] auto _ : state)
    {
        unsigned char* bufferPtr = encodeBuffer;
        (void)encoder.Encode(&bufferPtr, sizeof(encodeBuffer), header, message, messageData, metaData, Format); 
    }

    state.counters["logs_per_second"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

static void EncodeFlattenedBinaryLog(benchmark::State& state) { EncodeLog<ENCODE_FORMAT::FLATTENED_BINARY>(state); }
static void EncodeAsciiLog(benchmark::State& state) { EncodeLog<ENCODE_FORMAT::ASCII>(state); }
static void EncodeAbbrevAsciiLog(benchmark::State& state) { EncodeLog<ENCODE_FORMAT::ABBREV_ASCII>(state); }
static void EncodeBinaryLog(benchmark::State& state) { EncodeLog<ENCODE_FORMAT::BINARY>(state); }
static void EncodeJsonLog(benchmark::State& state) { EncodeLog<ENCODE_FORMAT::JSON>(state); }

BENCHMARK(EncodeFlattenedBinaryLog);
BENCHMARK(EncodeAsciiLog);
BENCHMARK(EncodeAbbrevAsciiLog);
BENCHMARK(EncodeBinaryLog);
BENCHMARK(EncodeJsonLog);

int main(int argc, char** argv)
{
    if (argc < 2) { throw std::invalid_argument("1 argument required.\nUsage: <project root> [benchmark options]"); }

    std::string strDatabaseVar = std::string(argv[1]) + "/database/messages_public.json";

#ifdef _WIN32
    if (_putenv_s("TEST_DATABASE_PATH", strDatabaseVar.c_str()) != 0) { throw std::runtime_error("Failed to set db path."); }
#else
    if (setenv("TEST_DATABASE_PATH", strDatabaseVar.c_str(), 1) != 0) { throw std::runtime_error("Failed to set db path."); }
#endif

    Logger::InitLogger();

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
