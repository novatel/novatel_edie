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
#include <novatel_edie/decoders/common/json_db_reader.hpp>
#include <novatel_edie/decoders/oem/encoder.hpp>
#include <novatel_edie/decoders/oem/message_decoder.hpp>
#include <novatel_edie/decoders/oem/rangecmp/range_decompressor.hpp>

#include "novatel_edie/decoders/oem/header_decoder.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

static void LoadJson(benchmark::State& state)
{
    for ([[maybe_unused]] auto _ : state) { (void)LoadJsonDbFile(std::getenv("TEST_DATABASE_PATH")); }
}

BENCHMARK(LoadJson);

template <size_t N> static void DecodeLog(benchmark::State& state, const unsigned char (&data)[N])
{
    MessageDatabase::Ptr clJsonDb = LoadJsonDbFile(std::getenv("TEST_DATABASE_PATH"));
    const HeaderDecoder headerDecoder(clJsonDb);
    const MessageDecoder messageDecoder(clJsonDb);

    for ([[maybe_unused]] auto _ : state)
    {
        const unsigned char* dataPtr = data;
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

    MessageDatabase::Ptr clJsonDb = LoadJsonDbFile(std::getenv("TEST_DATABASE_PATH"));
    const HeaderDecoder headerDecoder(clJsonDb);
    const MessageDecoder messageDecoder(clJsonDb);
    const Encoder encoder(clJsonDb);

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

static void DecompressRangeCmp(benchmark::State& state, uint32_t id, const char* compressedData)
{
    MessageDatabase::Ptr clJsonDb = LoadJsonDbFile(std::getenv("TEST_DATABASE_PATH"));
    RangeDecompressor rangeDecompressor(clJsonDb);

    char aucCompressionBuffer[MAX_ASCII_MESSAGE_LENGTH];
    strcpy(aucCompressionBuffer, compressedData);

    MetaDataStruct stMetaData;
    stMetaData.usMessageId = static_cast<uint16_t>(id);
    stMetaData.uiLength = static_cast<uint32_t>(strlen(compressedData));

    for ([[maybe_unused]] auto _ : state)
    {
        auto* bufferPtr = reinterpret_cast<unsigned char*>(aucCompressionBuffer);
        (void)rangeDecompressor.Decompress(bufferPtr, sizeof(aucCompressionBuffer), stMetaData);
    }

    state.counters["logs_per_second"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// clang-format off
constexpr std::string_view rangecmpLog = "#RANGECMPA,COM1,0,77.5,FINESTEERING,2195,512277.000,02000020,9691,16696;105,04dc10084831f31f25ab020b129a79c45207c2966a030000,0b5c30012705f6df3dab020b8cd140dd50070d962a030000,0bdc30022705f6ef32ab020b4ade40dd520767966a030000,24dc100868910e901ca70a0b17583abf5213c27261030000,2b5c3001095a0bf023a70a0b74ed29d94013187201030000,44dc1018fbbeff3fc6c7d50a1fedf5e1520f81fca2030000,4b5c301156cdff7fd0c7d50a019a3af4500fd1fb02030000,4bdc300256cdff7fd0c7d50ac29f3af4520f27fc42030000,64dc10088e7cff5fbaeca5095a288ea9310e02dee5030000,6b5c30019399ff4fc8eca5098caec18f300e48dde5030000,6bdc30029399ff3fbfeca5094cadc18f300eaddde5030000,64dcd001b59dff3fe4eca50998d06ec4100ecfdde5030000,84dc10089881f15f268df30bde143ea66308bff8e7020000,8b5c3001c8b4f4af538df30b5767f4e18008f8f7e7020000,8bdc3002c8b4f47f4a8df30b175bf4e1820851f887030000,84dcd0011d2df5ff4a8df30b64d534a3100886f8e7030000,a4dc101808a5f6ff13393d0a51132cc6201e0286e8030000,ab5c3011cab5f80f30393d0ab9cd50c2201e5885c8030000,abdc3002cab5f89f27393d0a77ce50c2201e9785e8030000,a4dcd0018503f99f2b393d0aa81638fa101ec885e8030000,e4dc10080190fb9ff79c450a0a32a9c0310d42d1e4030000,eb5c3001cf8afc9fff9c450a007b05be300d98d024030000,04dd1008b4defc0f14022c0b817551a94215c96743030000,0b5d30019c8ffd2f1f022c0ba86517c850150d67e3020000,24dd1008b59808900210230afdaa5ad7211142e2e2030000,2b5d3001d6b206500d10230a1c18b4cf201198e1a2030000,2bdd3002d6b206e00310230adb09b4cf2111e4e1e2030000,44dd1018e8140720225fbb0a99724ef34201022d82030000,4b5d3011a78405e04c5fbb0a108ebe8140013d2c42030000,4bdd3002a78405503e5fbb0acf86be8142019a2ca2030000,44ddd001c44905103f5fbb0a328d56bc1001c82ce2030000,04de15186d0f000018bd73140859c09063c17214e9020000,0bde3502050c00f01fbd7314b3ec108864c1071469020000,04ded5016f0b00d042bd7314ecd1baf730c1351429030000,049f1118ba5cf14f8dc7fc0a1eba82a8522669a0e5220000,0b3fb110739df44fdcc7fc0a41c32cca502666a0c5210000,0b9f3110739df4efd6c7fc0a82c92cca532666a0e5210000,249f111817ef0e5070d9320a5da299ad203a6ab2812f0000,2b3fb100859d0bd098d9320aed18b0b1203a24b2a12f0000,2b9f3100859d0be09ad9320a2c0eb0b1203a41b2a12f0000,449f11082e9c0bd0147d730b8d6770d63028e82460330000,4b3fb100ad0709103b7d730b115c578a7028a42440330000,4b9f3100ad0709f03b7d730b525a578a7028bf2440330000,649f11086bacfdcf666dab091393588983395d5543260000,6b3fb110c530feaf996dab096abe0bf980391755a3270000,6b9f3110c530fe8fa36dab09a9ca0bf980393555a3270000,a49f01184f22f65ffc27ec09394471e3202f870c88030000,c49f1118be52ffbf2ffa6a0a1cf40f8d202739ba820f0000,cb3fb1103f79ffef6efa6a0a0b9561982027d972420f0000,cb9f31003e79ff2f6cfa6a0a489561983027e072420f0000,e49f110842e9efbf7a986d0b723dc1dbf738730c20290000,eb3fb1008a7cf39fb1986d0b2db3798ef0386d0c402a0000,eb9f3100897cf3afb4986d0b6cb2798ef3386d0c402a0000,049c1118e8b30de08dcae40a16cad8b931313c81601b0000,0b3cb1105fa80a30cccae40a0789a8d730313a81401b0000,0b9c31005fa80aa0cacae40a4588a8d730313a81601b0000,c4dc5308775808a0aa68460c8fc3d0ef3115340dc2030000,c4dc9301523b0660d168460c0ed28ffa101515f4c2030000,c4dc3302f66406b0c268460c64be59d2101515f4e2030000,c43c9302255006b0c668460c06c374e6101510f4e2030000,e4dc5308ade9ff2f97ce1f0b733355b1201b7563e7030000,e4dc930162efffcfbace1f0b3987128b101b3563e7030000,e4dc3302f4eeff8fa9ce1f0b2988a1e6101b3563e7030000,e43c930228efffdfadce1f0bfd03daf8101bdc62e7030000,24dd53083451f7cf792b550ccca91ee6311e283fcb030000,24dd93012884f95fa82b550c39a252f3201ee13ecb030000,24dd3302d458f90f972b550c9735ecca101eee3eeb030000,243d93027a6ef94f9d2b550cb4751fdf101edf3eeb030000,44dd5308a27e0860a14e9f0d337b428d4204f11520030000,44dd9301f4570680cc4e9f0d69c761d13004ae1560030000,44dd33024e820630bc4e9f0d70ddc1a42004ae15a0030000,443d9302236d06e0c44e9f0db9da11bb2004a515e0030000,84dd53087da30940168e2c0ca008cc80310f2743a2030000,84dd93019e320750508e2c0c31333e87200fe142c2030000,84dd3302a86207603d8e2c0cecd45cdf100fdf42e2030000,843d9302b14a0710418e2c0cdc8c4df3200fbd42e2030000,c49e1408a4c1090035316c0b45ae9e9020180ca6c2030000,c4de34014d5a07103d316c0bfdbc9ae5101842a5e2030000,c43e7401728b07c021316c0b9e832fc02018aea5c2030000,e49e14188d14f6bf98a43e0bc1c644ae302d228f8a030000,e4de34016586f83ff2a43e0b5691f2fb102d628eea030000,e43e74116754f86fd8a43e0b7c8a1cd7202de88eca030000,049f14081575f66f16d34d0c654cc1fd710e27bc85020000,04df34101e9ff84f1fd34d0c92e99ece210e02bc85030000,449f14082a9a00c08d0c290a85a9f4e2201a2b7fe6030000,44df3401277400f0ab0c290a4c2e1d84101a667ee6030000,443f7411387700e0920c290a8889d4e2101aee7ee6030000,649f1418cfdd0d0064c6260c2ada2b97302c39ad60030000,64df340127730a50afc6260c9163148a302c75ac20030000,643f7401fcb80ae082c6260c09f045e2302cfbac00030000,849f1418fecd035075c6c10c771548b260293767e0020000,84df340101de0290f9c6c10cf6820cbe20296c6680030000,843f741128f102e0d7c6c10c044f42943029086720030000,a49f14182f8ef48f35b0d20cd72146a7402ab97127030000,a4df34011160f72fdeb0d20cc5fdc0b5302aee7027030000,a43f74116f26f74fc4b0d20ca565bf8b402a7b7107030000,c49f1418e055fa5fe2d15a0cfdcc4bf5302171e283030000,c4df340124bbfb7fcfd25a0c4c2d8df04021d5ddc3020000,c43f7411b19efbcfaed25a0ca92e14c8602182c863020000,049c1418c477fa3ff755080b2da69dd1201d76aea5030000,04dc3401b4d4fb1f1156080bd2b39596101db5ade5030000,043c7411e4b8fbbff255080bff9c71f2201d42aea5030000,249c14085ccf0600d0c6bc0a8827cc822023b57fe3030000,24dc3401c6210580e6c6bc0ae4e8a5bb1023f57ee3030000,243c741110440560cac6bc0aa848799820237b7fe3030000*41fc2e65\r\n";
constexpr std::string_view rangecmp2Log = "#RANGECMP2A,COM1,0,56.0,FINESTEERING,2171,404649.000,02010000,1fe3,16248;1870,000200c8ba5b859afb2fe1ffff6b3f0651e830813d00e4ffff43bac60a006c803d0001140034b7f884a8ff2fe1ffff6b3fa428a83c82f0ffe4ffff439c4404c8cb82f0ff021d00043bfd04720330e1ffff6b3f2628086b811200e4ffff439ca605283f811200e5ffff095d860f50b081120003060020dbf8854ef94fe1ffff6b954a513855800a00e4ffff43d56a798813800a00e5ffff09782a88a836800a00e7ffff031ca4a8706980f7ff041f001822d685d8fc3fe1ffff6b5b483218a2003b00e4ffff43f1280ee054003b00e5ffff09b268154897003b00050900ac57ef85effe4fe1ffff6b948c0a705680f7ffe4ffff43d44c1ea87900f7ffe5ffff095bac23987d00f7ffe7ffff031fa249f0148116000612001813cb059e0640e1ffff6b59480fb0da802d00e4ffff43f38a07183e812d00e5ffff09966a12c0f3002e00e7ffff031b2669187782190007190048e81385abfb4fe1ffff2b3e6639208800eaffe4ffff039b4649586400eaffe5ffff095ee651583900eaffe7ffff031f827020ac00e0ff080500f8ce12059b0430e1ffff6b3f842c5829820c00e4ffff439c040b50e5820c00e5ffff095da414788b820c00091a00d4c6dd85140640e1ffff6b92ae0b289300ccffe4ffff43f30e35f0db80cbffe5ffff0978ce38a89100ccffe7ffff031c643a885081c8ff0b0c00e88f7105f0f83fe1ffff2b5c4686e805011c00e4ffff03b82669c03e801b00e5ffff097a866f70a0801b0010c270b8074e8a660030e1ffff2b78e840084080edffe3ffff0978884af01500edffe4ffff0319e671088f80f4ff14852054613589010010e1ffff63bba60ab02200c7ff158a208c6a2d89000010e1ffff63bc0880503f00260017832000972c89000010e1ffff63bb885f2007000000180d15640900851f0030e1ffff290fcd0f18f900deffe4ffff43564e4e70b001deffe3ffff49d30e4cf0a401deff190c168cd722052af93fe1ffff29b9a619283300f4ffe4ffff031b066e00bf80f3ffe3ffff499b266988b380f3ff1a171a60005285370610e1ffff69d7660410220114001b151be8a3298543fa3fe1ffff69d72608885800e2ffe4ffff033a4635788a00e2ffe3ffff499a663e306000e2ff1c16146892a3046bff3fe1ffff6911cd11d03300e8ffe4ffff43714c55482f01e8ffe3ffff09f12c5cf85101e8ff1d071c9c3942853f0730e1ffff69d6c60f705e01e8ffe4ffff0339463a98cf82e8ffe3ffff499ae641682083e8ff1e0e10fc64a785d90630e1ffff29f3ca0e1021801a00e4ffff4337aa7fe833811a00e3ffff09b8ca7610fa801a001f05188c42a9854ef93fe1ffff29f1ea06585080dbffe4ffff4372ea46280f01dbffe3ffff49f20c50504101dbff2006137c4000059e0010e1ffff690e3904080400c5ff261a5064418705fbfd4fe1ffff293f0406908b80ecffe2ffff031f6264f8e601e6ffe3ffff031fc22ec85801ebffe4ffff031fe22ae05681e8ff270c50ec595586230540e1ffff29950a02c04b801900e2ffff031ac6496035812200e3ffff031924168079001f00e4ffff031ca6110086802400280d50488d8506ebfa4fe1ffff29980839600500c9ffe2ffff031a668fd80681d2ffe3ffff0319066b801300bcffe4ffff031c8654401b80c1ff291f5034b8e385a5ff4fe1ffff295f640c683700e0ffe2ffff031f225b802581daffe3ffff031fe21d906b00d9ffe4ffff031f4221609780d8ff2b2150f8eac105a70240e1ffff293f641468ef802500e2ffff031f8240905c022000e3ffff031f82042888812900e4ffff031f6207e0a80124002c0850309a0206250040e1ffff2979e80eb8b9003100e2ffff031b044018e7013200e3ffff031c441dd036812300e4ffff031ea413e0650128002d0150f8db2f068afa4fe1ffff297ce63c0043001b00e2ffff031fe29948fa801a00e3ffff031e84589847801a00e4ffff031f045e883f001a002e0750dc257686740440e1ffff297a680f881f81d4ffe2ffff031e047970f102c2ffe3ffff031ea249f85e02bfffe4ffff031f244390fe81bfff2f1850d08c82065f0440e1ffff2998a803488b00e4ffe2ffff031b664f981202ccffe3ffff031bc40f201201cdffe4ffff031d4418602001ccff362d6040494c060f0420e1ffff6958e80fe837003d00f4ffff031ca4acd845823000371c60983fad8543fb2fe1ffff293a860f388f00cdfff4ffff031ee234b8c680ccff3b1e60ccf2a885dc0420e1ffff293b6606800701effff4ffff031e6446402f02edff3f3a607ca3168851fa1fe1ffff2957a8589829000700410e60701bf60529fc2fe1ffff2976e82a880101ebffe3ffff093ce40148e480e3ff422e607085ec05acfa2fe1ffff293a06614007002600f4ffff031e42e7b01981240044216008d2be85f6fe2fe1ffff293b060f0053813e00f4ffff031f22bf8111863b00451b6048481885190020e1ffff691f0204d06a001800f4ffff031f621b986e810f0047246044cfe4053cff2fe1ffff293bc60ea00a001700f4ffff031d249748ad8219004b29600c07b9859e0420e1ffff293b460e98a9011c00f4ffff031fe2de305f850700*2b134683\r\n";
constexpr std::string_view rangecmp4Log = "#RANGECMP4A,COM1,0,81.5,FINESTEERING,1921,228459.000,00000020,fb0e,32768;627,630032090851000000009200dbbf7d8306f822d0a3b2bc897f0010d350428cf31228ea9f7300040050ff5e641cb7c7463d2a00b6a4644f6e5ee2a0fe530a00fe1f829dcfe4cf30d52abaf37f94e01621cd8d8c04a0bafcaf00e43b0761690064e7bfe90f11ce8710a4eb2b573202607403fc28e647c6fe9f550118007a9d839c2680ebfedff6876be81150411adbc972feef4686c483f30a09f01773ff0b0050d8b8a843f41576b94100440e1e4f59ace54fffca2700fc1f62e14720f4facba64affbf9c52ff39ce4b3eef9f14fd0f00244387d00d80fefabfeb0fb3cf456ae97542d410fc9ffab7f601e73580e5efdaff0f00a0b33991fc072ccbaa99ff134efa9fd0dc684bfc61f0fffeff60b020000000008004c0ff3fa0b2f724f7e1eee889e9fb9f3977c0437391ab135877fe0b00301edf93f4bd63c62850fdbf8527e6e5cd438e3a208400e0ff43bb6f5fc2101c75b058daff375c5ea4378f51940022eeffff0fe1c97dcda81887c83a63007c9d5a7ed65ce6f901427bffff3f9c04f735db1d55294a3bfc5f35ccc66df318c412181400140060eedbd7285feaf6a653f9bf9fc7fe27cd653633c0b5fcffff03197b4f8228d4e59d0cfbffa731b2f73b07e9b68078f47f0000a9be7dcdcc51898da269fe839b6191ab9cc67701f21000fc3f0001a1000000008002c03fb4362793b9bfeb657dfcffe6badabb9a4375b77f5bff1fed87bce64454a98ae16c14ff4fec6f7a48f3206b03e8040138fbd0023d225492cd7679a4ffa5623b08810e42bf05fce17fa41f9a9ccfc8e2626231edf2ff208a1225ce6150204067febfef030100000000000028000ca9cc8728bb3306e68af97f921cfce3e632f0d1cf8300c8f701*6de99eb7\r\n";
constexpr std::string_view rangecmp5Log = "#RANGECMP5A,COM1,0,57.0,FINESTEERING,2307,333380.000,02000020,4f34,32768;973,6300240cad01000000001200ffff0f21ed0d4262ecdd5a68ac007bc81a68685f10a3178439000600e00efe106edd3d8ba9557d300db1007ff085908681b46d0200001cbf27831539493f7dc4122e83797ff0fd185b25a0e960e8ff8728f843744735da26aa27c118bff217c1174210035c1506feff50eabe142b746e9285e8dfbfadad8244f7a49cf200451fff00003ade13a394922b9f84630344fa4550f17e98e80bc807150c00c088f3720a36f54bd7e01840ac14f3c79b13346e06bd04fff87f48696f92d4e038dafa17e36303754147fba2e89160293c7f00003bee8ba105cd2256bcea7e05fc2c98714fccf611b06bf70f00e0ca7d39083a268b6a9f27d05a1cfa2beec9992502696800ff3f040e1c000000008004c0ff07a8b6bd3191744aeceabd0b709f9f01e9ed4f11e58203e0ffc33f9811f844d4c2dd82a5b132c03ebf137cbc37687407fc2dfe73005082d91f047a48ce90a2ca020f7ee41fdffe984518302e01ec010068667f50c8a7efb6722100400e22fe517bd496ade07b0520fe0feb9dfd317b554bc72ac6ffbfd835073fed5161748242737fe23f9014f8448ca8995d2466e00117090c2cc12fe192052481023800a0a2de172170816695f85209897abd2f004441a2000000a000f0ff0fa9edcb19860b6e696dd3faa55e2ae86c5fccd11cdc7cdbfbfd21ee3d2112cb71556b339340343bfcd5ef4770af811c2743fb3f74b03f6b178e3fa749dbff975a5fa083fd49c04ae041f5cffe870af66735b0ae7c33192f40a9bdebd7b62fa5220a3ee8fecefe90f79e9047965589c5fe1fb0b407ff0ef823d0a2007f962009001adf1322db01f0b33855ffc0693150ff7e88c315301ff7afff43e97b42b06add4856cebca00cfd022ae08f90070399ea03e87f685f7f96663acd331379fcf31f4e417afb5276c3c0bedbfff80fc0c83860840000a08090ad6ddbb490debe9c071a58e755c621c0ef2f7d587a3f4819345f8df25624549b7eff337c11f81d2051c580fe0f47ed4b82e02b8798d13c808ff1f05f704f8acd14006dfde50120a87d41d6b86b528cac0a1097d8fc6bed8d2143025ff1bf2b004cb72fa7f100d68b8155fe2b641a20aebd310a46006de92f01804bf84224515c75294e1c4034680eacbf1f61a8004857002d00f0fe7e0cb5368751655a18f0c7cbfec7f7449864004a7bc0fd1f16db1774f0eb81c1c478f79de166b0dcbe94b42dd80ec7bffe43f5fb31fe1274c1950df7404eccf6dbdd93d10d03dfcd84fe7fe077a89ac7ca35c4eaabffe3a6340134b62fa86f605054221d011eefd77fa7bd3921270006fd57ff876bf842f027bfeba735d3418b6e02acc01f61420342e6094b00*3b6fb3a3\r\n";
// clang-format on;

static void DecompressRangeCmp(benchmark::State& state) { DecompressRangeCmp(state, RANGECMP_MSG_ID, rangecmpLog.data()); }
static void DecompressRangeCmp2(benchmark::State& state) { DecompressRangeCmp(state, RANGECMP2_MSG_ID, rangecmp2Log.data()); }
static void DecompressRangeCmp4(benchmark::State& state) { DecompressRangeCmp(state, RANGECMP4_MSG_ID, rangecmp4Log.data()); }
static void DecompressRangeCmp5(benchmark::State& state) { DecompressRangeCmp(state, RANGECMP5_MSG_ID, rangecmp5Log.data()); }

BENCHMARK(DecompressRangeCmp);
BENCHMARK(DecompressRangeCmp2);
BENCHMARK(DecompressRangeCmp4);
BENCHMARK(DecompressRangeCmp5);

int main(int argc, char** argv)
{
    Logger::InitLogger();

    std::filesystem::path pathSourceFile = __FILE__;
    std::filesystem::path pathRepoDir = pathSourceFile.parent_path().parent_path();
    std::filesystem::path pathDatabaseFile = pathRepoDir / "database" / "database.json";

    std::string strDatabaseVar = pathDatabaseFile.string();

#ifdef _WIN32
    if (_putenv_s("TEST_DATABASE_PATH", strDatabaseVar.c_str()) != 0) { throw std::runtime_error("Failed to set db path."); }
#else
    if (setenv("TEST_DATABASE_PATH", strDatabaseVar.c_str(), 1) != 0) { throw std::runtime_error("Failed to set db path."); }
#endif

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
