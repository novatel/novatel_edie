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
// ! \file message_decoder_unit_test.cpp
// ===============================================================================

#include <gtest/gtest.h>

#include "novatel_edie/common/message_decoder.hpp"

using namespace novatel::edie;

class MessageDecoderTypesTest : public ::testing::Test
{
  protected:
    class DecoderTester : public MessageDecoderBase
    {
      public:
        DecoderTester(JsonReader* pclJsonDb_) : MessageDecoderBase(pclJsonDb_) {}

        STATUS TestDecodeAscii(const std::vector<BaseField*> MsgDefFields_, const char** ppcLogBuf_,
                               std::vector<FieldContainer>& vIntermediateFormat_)
        {
            return DecodeAscii<false>(MsgDefFields_, const_cast<char**>(ppcLogBuf_), vIntermediateFormat_);
        }

        STATUS TestDecodeBinary(const std::vector<BaseField*> MsgDefFields_, unsigned char** ppucLogBuf_,
                                std::vector<FieldContainer>& vIntermediateFormat_)
        {
            uint16_t MsgDefFieldsSize = 0;
            for (BaseField* field : MsgDefFields_) { MsgDefFieldsSize += field->dataType.length; }
            return DecodeBinary(MsgDefFields_, ppucLogBuf_, vIntermediateFormat_, MsgDefFieldsSize);
        }

        template <typename T, DATA_TYPE D> void ValidSimpleASCIIHelper(std::vector<std::string> vstrTestInput, std::vector<T> vTargets)
        {
            // this test expects the virst two values in the test input to be the min and max, respectively
            // these values must be omitted from the vTargets argument when the function is called
            if constexpr (D != DATA_TYPE::UCHAR && D != DATA_TYPE::CHAR)
            {
                vTargets.insert(vTargets.begin(), std::numeric_limits<T>::max());
                vTargets.insert(vTargets.begin(), std::numeric_limits<T>::min());
            }

            for (size_t sz = 0; sz < vstrTestInput.size(); ++sz)
            {
                std::vector<FieldContainer> vIntermediateFormat_;
                vIntermediateFormat_.reserve(1);

                // there is an issue here in that some data types can have multiple conversion strings or sizes
                // associated with them. In order to fix this, we may want these DataType functions to return a
                // vector so we can iterate through every possible valid combination of a basefield
                const auto stMessageDataType = BaseField("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), DataTypeSize(D), D);
                const char* tempStr = vstrTestInput[sz].c_str();
                MessageDecoderBase::DecodeAsciiField(&stMessageDataType, const_cast<char**>(&tempStr), vstrTestInput[sz].length(),
                                                     vIntermediateFormat_);

                if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
                {
                    ASSERT_NEAR(std::get<T>(vIntermediateFormat_[0].fieldValue), vTargets[sz],
                                std::abs(vTargets[sz]) * std::numeric_limits<T>::epsilon());
                }
                else { ASSERT_EQ(std::get<T>(vIntermediateFormat_[0].fieldValue), vTargets[sz]); }
            }
        }

        template <typename T, DATA_TYPE D> void InvalidSizeSimpleASCIIHelper(std::string strTestInput)
        {
            std::vector<FieldContainer> vIntermediateFormat;
            vIntermediateFormat.reserve(1);

            const auto stMessageDataType = BaseField("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), DataTypeSize(D) + 1, D);
            const char* tempStr = strTestInput.c_str();
            ASSERT_THROW(
                MessageDecoderBase::DecodeAsciiField(&stMessageDataType, const_cast<char**>(&tempStr), strTestInput.length(), vIntermediateFormat),
                std::runtime_error);
        }

        template <typename T, DATA_TYPE D> void ValidBinaryHelper(std::vector<std::vector<uint8_t>> vvucTestInput, std::vector<T> vTargets)
        {
            // this test expects the virst two values in the test input to be the min and max, respectively
            // these values must be omitted from the vTargets argument when the function is called
            vTargets.insert(vTargets.begin(), std::numeric_limits<T>::max());
            vTargets.insert(vTargets.begin(), std::numeric_limits<T>::min());

            for (size_t sz = 0; sz < vvucTestInput.size(); ++sz)
            {
                std::vector<FieldContainer> vIntermediateFormat_;
                vIntermediateFormat_.reserve(1);

                // there is an issue here in that some data types can have multiple conversion strings or sizes
                // associated with them. In order to fix this, we may want these DataType functions to return a
                // vector so we can iterate through every possible valid combination of a basefield
                const auto stMessageDataType = BaseField("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), DataTypeSize(D), D);
                // there should be a better way to do this
                uint8_t* pucTestInput = vvucTestInput[sz].data();
                MessageDecoderBase::DecodeBinaryField(&stMessageDataType, &pucTestInput, vIntermediateFormat_);

                if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
                {
                    ASSERT_NEAR(std::get<T>(vIntermediateFormat_[0].fieldValue), vTargets[sz],
                                std::abs(vTargets[sz]) * std::numeric_limits<T>::epsilon());
                }
                else { ASSERT_EQ(std::get<T>(vIntermediateFormat_[0].fieldValue), vTargets[sz]); }
            }
        }
    };

  public:
    std::unique_ptr<JsonReader> pclMyJsonDb;
    std::unique_ptr<DecoderTester> pclMyDecoderTester;
    std::vector<BaseField*> MsgDefFields;
    std::string sMinJsonDb;

    MessageDecoderTypesTest()
    {
        sMinJsonDb = "{ \
                        \"enums\": [ \
                           { \
                              \"name\": \"Responses\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           }, \
                           { \
                              \"name\": \"Commands\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           }, \
                           { \
                              \"name\": \"PortAddress\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           }, \
                           { \
                              \"name\": \"GPSTimeStatus\", \
                              \"_id\": \"0\", \
                              \"enumerators\": [] \
                           } \
                        ], \
                        \"messages\": [] \
                  }";
    }

    void SetUp() override
    {
        try
        {
            pclMyJsonDb = std::make_unique<JsonReader>();
            pclMyJsonDb->ParseJson(sMinJsonDb);
            pclMyDecoderTester = std::make_unique<DecoderTester>(pclMyJsonDb.get());
        }
        catch (JsonReaderFailure& e)
        {
            std::cout << e.what() << std::endl;

            for (auto it : MsgDefFields) { delete it; }

            MsgDefFields.clear();
        }
    }

    void TearDown() override
    {
        pclMyDecoderTester->ShutdownLogger();

        for (auto it : MsgDefFields) { delete it; }

        MsgDefFields.clear();
    }

    void CreateEnumField(std::string name, std::string description, int32_t value)
    {
        auto stField = new EnumField();
        auto enumDef = new EnumDefinition();
        auto enumDT = new EnumDataType();
        enumDT->name = name;
        enumDT->description = description;
        enumDT->value = value;
        enumDef->enumerators.push_back(*enumDT);
        stField->enumDef = enumDef;
        stField->type = FIELD_TYPE::ENUM;
        MsgDefFields.emplace_back(stField);
    }
};

// TODO: we disable clang-format because of the long strings
// clang-format off

TEST_F(MessageDecoderTypesTest, LOGGER)
{
    spdlog::level::level_enum eLevel = spdlog::level::off;

    ASSERT_NE(spdlog::get("message_decoder"), nullptr);
    std::shared_ptr<spdlog::logger> novatel_message_decoder = pclMyDecoderTester->GetLogger();
    pclMyDecoderTester->SetLoggerLevel(eLevel);
    ASSERT_EQ(novatel_message_decoder->level(), eLevel);
}

TEST_F(MessageDecoderTypesTest, FIELD_CONTAINER_ERROR_ON_COPY)
{
    FieldContainer fc(3, new BaseField());
    ASSERT_THROW(FieldContainer fc2(fc);, std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, ASCII_SIMPLE_VALID)
{
    pclMyDecoderTester->ValidSimpleASCIIHelper<bool, DATA_TYPE::BOOL>({"FALSE", "TRUE"}, {});
    pclMyDecoderTester->ValidSimpleASCIIHelper<uint8_t, DATA_TYPE::UCHAR>({"#", "A", ";"}, {'#', 'A', ';'});
    pclMyDecoderTester->ValidSimpleASCIIHelper<int8_t, DATA_TYPE::CHAR>({"#", "A", ";"}, {'#', 'A', ';'});
    pclMyDecoderTester->ValidSimpleASCIIHelper<uint16_t, DATA_TYPE::USHORT>({"0", "65535", "29383"}, {29383});
    pclMyDecoderTester->ValidSimpleASCIIHelper<int16_t, DATA_TYPE::SHORT>({"-32768", "32767", "0"}, {0});
    pclMyDecoderTester->ValidSimpleASCIIHelper<uint32_t, DATA_TYPE::UINT>({"0", "4294967295", "367184312"}, {367184312});
    pclMyDecoderTester->ValidSimpleASCIIHelper<int32_t, DATA_TYPE::INT>({"-2147483648", "2147483647", "0"}, {0});
    pclMyDecoderTester->ValidSimpleASCIIHelper<uint32_t, DATA_TYPE::ULONG>({"0", "4294967295", "76382343"}, {76382343});
    pclMyDecoderTester->ValidSimpleASCIIHelper<int32_t, DATA_TYPE::LONG>({"-2147483648", "2147483647", "0"}, {0});
    pclMyDecoderTester->ValidSimpleASCIIHelper<uint64_t, DATA_TYPE::ULONGLONG>({"0", "18446744073709551615", "9346284632323321"}, {9346284632323321ULL});
    pclMyDecoderTester->ValidSimpleASCIIHelper<int64_t, DATA_TYPE::LONGLONG>({"-9223372036854775808", "9223372036854775807", "0"}, {0LL});
    pclMyDecoderTester->ValidSimpleASCIIHelper<float, DATA_TYPE::FLOAT>({"1.17549435e-38", "3.40282347e+38", "3279347.4", "-3.14"}, {3279347.4f, -3.14f});
    pclMyDecoderTester->ValidSimpleASCIIHelper<double, DATA_TYPE::DOUBLE>({"2.2250738585072014e-308", "1.7976931348623157e+308", "0.0"}, {0.0});
}

TEST_F(MessageDecoderTypesTest, DISABLED_ASCII_SIMPLE_INVALID_SIZE)
{
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<bool, DATA_TYPE::BOOL>("FALSE");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<uint8_t, DATA_TYPE::UCHAR>("#");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<int8_t, DATA_TYPE::CHAR>("#");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<uint8_t, DATA_TYPE::HEXBYTE>("0x00");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<uint16_t, DATA_TYPE::USHORT>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<int16_t, DATA_TYPE::SHORT>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<uint32_t, DATA_TYPE::UINT>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<int32_t, DATA_TYPE::INT>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<uint32_t, DATA_TYPE::ULONG>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<int32_t, DATA_TYPE::LONG>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<uint64_t, DATA_TYPE::ULONGLONG>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<int64_t, DATA_TYPE::LONGLONG>("0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<float, DATA_TYPE::FLOAT>("0.0");
    pclMyDecoderTester->InvalidSizeSimpleASCIIHelper<double, DATA_TYPE::DOUBLE>("0.0");
}

TEST_F(MessageDecoderTypesTest, ASCII_CHAR_BYTE_INVALID_INPUT)
{
    MsgDefFields.emplace_back(new BaseField("CHAR_1", FIELD_TYPE::SIMPLE, "%c", 1, DATA_TYPE::CHAR));
    std::vector<FieldContainer> vIntermediateFormat_;
    vIntermediateFormat_.reserve(1);

    auto testInput = "4";
    pclMyDecoderTester->TestDecodeAscii(MsgDefFields, &testInput, vIntermediateFormat_);

    ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_[0].fieldValue), '4');
}

TEST_F(MessageDecoderTypesTest, ASCII_BOOL_INVALID_INPUT)
{
    MsgDefFields.emplace_back(new BaseField("B_True", FIELD_TYPE::SIMPLE, "%d", 4, DATA_TYPE::BOOL));
    std::vector<FieldContainer> vIntermediateFormat_;
    vIntermediateFormat_.reserve(1);

    auto testInput = "True";
    pclMyDecoderTester->TestDecodeAscii(MsgDefFields, &testInput, vIntermediateFormat_);

    ASSERT_EQ(std::get<bool>(vIntermediateFormat_[0].fieldValue), false);
}

TEST_F(MessageDecoderTypesTest, ASCII_ENUM_VALID)
{
    std::vector<std::pair<std::string, int32_t>> vTestInput = {{"UNKNOWN", 20}, {"APPROXIMATE", 60}, {"SATTIME", 200}};

    for (const auto& input : vTestInput)
    {
        CreateEnumField(input.first, "", input.second);
    }

    std::vector<FieldContainer> vIntermediateFormat;
    vIntermediateFormat.reserve(vTestInput.size());

    auto testInput = "UNKNOWN,APPROXIMATE,SATTIME";

    ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(MsgDefFields, &testInput, vIntermediateFormat), STATUS::SUCCESS);
    ASSERT_EQ(vIntermediateFormat.size(), vTestInput.size());

    for (size_t sz = 0; sz < vTestInput.size(); ++sz)
    {
        ASSERT_EQ(std::get<int32_t>(vIntermediateFormat[sz].fieldValue), vTestInput[sz].second);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_STRING_VALID)
{
    MsgDefFields.emplace_back(new BaseField("MESSAGE", FIELD_TYPE::STRING, "", 1, DATA_TYPE::UNKNOWN));
    std::vector<FieldContainer> vIntermediateFormat;
    vIntermediateFormat.reserve(1);

    std::vector<const char*> testInputs = {"SOL_COMPUTED,WAAS"};
    std::vector<const char*> testTargets = {"SOL_COMPUTED","WAAS"};

    for (size_t sz = 0; sz < testInputs.size(); ++sz)
    {
        ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(MsgDefFields, &testInputs[sz], vIntermediateFormat), STATUS::SUCCESS);
        ASSERT_EQ(std::get<std::string>(vIntermediateFormat[0].fieldValue), testTargets[sz]);

        vIntermediateFormat.clear();
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_TYPE_INVALID)
{
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::UNKNOWN, "%d", 1, DATA_TYPE::UNKNOWN));
    std::vector<FieldContainer> vIntermediateFormat_;
    vIntermediateFormat_.reserve(1);

    auto testInput = "garbage";

    ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(MsgDefFields, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, BINARY_VALID)
{
    pclMyDecoderTester->ValidBinaryHelper<bool, DATA_TYPE::BOOL>({{0x00, 0x00, 0x00, 0x00}, {0x01, 0x00, 0x00, 0x00}}, {});
    pclMyDecoderTester->ValidBinaryHelper<uint8_t, DATA_TYPE::UCHAR>({{0x00}, {0xFF}, {0x23}, {0x41}, {0x3B}}, {'#', 'A', ';'});
    pclMyDecoderTester->ValidBinaryHelper<int8_t, DATA_TYPE::CHAR>({{0x80}, {0x7F}, {0x00}}, {0});
    pclMyDecoderTester->ValidBinaryHelper<uint8_t, DATA_TYPE::HEXBYTE>({{0x00}, {0xFF}, {0x01}}, {1});
    pclMyDecoderTester->ValidBinaryHelper<uint16_t, DATA_TYPE::USHORT>({{0x00, 0x00}, {0xFF, 0xFF}, {0x01, 0x00}, {0x10, 0x00}}, {1, 16});
    pclMyDecoderTester->ValidBinaryHelper<int16_t, DATA_TYPE::SHORT>({{0x00, 0x80}, {0xFF, 0x7F}, {0xFF, 0xFF}, {0x00, 0x00}}, {-1, 0});
    pclMyDecoderTester->ValidBinaryHelper<uint32_t, DATA_TYPE::UINT>({{0x00, 0x00, 0x00, 0x00}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0x00, 0x00}},{65535U});
    pclMyDecoderTester->ValidBinaryHelper<int32_t, DATA_TYPE::INT>({{0x00, 0x00, 0x00, 0x80}, {0xFF, 0xFF, 0xFF, 0x7F}, {0x00, 0x00, 0xFF, 0xFF}, {0x00, 0x00, 0x00, 0x00}}, {-65536, 0});
    pclMyDecoderTester->ValidBinaryHelper<uint32_t, DATA_TYPE::ULONG>({{0x00, 0x00, 0x00, 0x00}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0x00, 0x00}}, {65535U});
    pclMyDecoderTester->ValidBinaryHelper<int32_t, DATA_TYPE::LONG>({{0x00, 0x00, 0x00, 0x80}, {0xFF, 0xFF, 0xFF, 0x7F}, {0x00, 0x00, 0xFF, 0xFF}, {0x00, 0x00, 0x00, 0x00}}, {-65536, 0});
    pclMyDecoderTester->ValidBinaryHelper<uint64_t, DATA_TYPE::ULONGLONG>({{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}, {});
    pclMyDecoderTester->ValidBinaryHelper<int64_t, DATA_TYPE::LONGLONG>({{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}}, {});
    pclMyDecoderTester->ValidBinaryHelper<float, DATA_TYPE::FLOAT>({{0x00, 0x00, 0x80, 0x00}, {0xFF, 0xFF, 0x7F, 0x7F}, {0x9A, 0x99, 0x99, 0x3F}, {0xCD, 0xCC, 0xBC, 0xC0}}, {1.2f, -5.9f});
    pclMyDecoderTester->ValidBinaryHelper<double, DATA_TYPE::DOUBLE>({{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, {0.0});
}

TEST_F(MessageDecoderTypesTest, BINARY_SIMPLE_TYPE_INVALID)
{
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "", 1, DATA_TYPE::UNKNOWN));
    std::vector<FieldContainer> vIntermediateFormat_;

    unsigned char* testInput = nullptr;

    ASSERT_THROW(pclMyDecoderTester->TestDecodeBinary(MsgDefFields, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, BINARY_TYPE_INVALID)
{
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::UNKNOWN, "", 1, DATA_TYPE::UNKNOWN));
    std::vector<FieldContainer> vIntermediateFormat_;

    unsigned char* testInput = nullptr;

    ASSERT_THROW(pclMyDecoderTester->TestDecodeBinary(MsgDefFields, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, SIMPLE_FIELD_WIDTH_VALID)
{
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%d", 4, DATA_TYPE::BOOL));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%hu", 2, DATA_TYPE::USHORT));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%hd", 2, DATA_TYPE::SHORT));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%u", 4, DATA_TYPE::UINT));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%lu", 4, DATA_TYPE::ULONG));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%d", 4, DATA_TYPE::INT));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%ld", 4, DATA_TYPE::LONG));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%llu", 8, DATA_TYPE::ULONGLONG));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%lld", 8, DATA_TYPE::LONGLONG));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%f", 4, DATA_TYPE::FLOAT));
    MsgDefFields.emplace_back(new BaseField("", FIELD_TYPE::SIMPLE, "%lf", 8, DATA_TYPE::DOUBLE));

    std::vector<FieldContainer> vIntermediateFormat;
    vIntermediateFormat.reserve(MsgDefFields.size());

    auto testInput = "TRUE,0x63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03";

    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(MsgDefFields, &testInput, vIntermediateFormat));
    // TODO: Keep this here or make a file for testing common encoder?
    //unsigned char aucEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    //unsigned char* pucEncodeBuffer = aucEncodeBuffer;

    //ASSERT_TRUE(pclMyEncoderTester->TestEncodeBinaryBody(vIntermediateFormat, &pucEncodeBuffer, MAX_ASCII_MESSAGE_LENGTH));
    //
    //vIntermediateFormat.clear();
    //pucEncodeBuffer = aucEncodeBuffer;
    //pclMyDecoderTester->TestDecodeBinary(MsgDefFields, &pucEncodeBuffer, vIntermediateFormat);
    //
    //size_t sz = 0;
    //
    //ASSERT_EQ(std::get<bool>(vIntermediateFormat.at(sz++).field_value), true);
    //ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat.at(sz++).field_value), 99);
    //ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat.at(sz++).field_value), 227);
    //ASSERT_EQ(std::get<int8_t>(vIntermediateFormat.at(sz++).field_value), 56);
    //ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat.at(sz++).field_value), 2734);
    //ASSERT_EQ(std::get<int16_t>(vIntermediateFormat.at(sz++).field_value), -3842);
    //ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.at(sz++).field_value), 38283U);
    //ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.at(sz++).field_value), 54244U);
    //ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.at(sz++).field_value), -4359);
    //ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.at(sz++).field_value), 5293);
    //ASSERT_EQ(std::get<uint64_t>(vIntermediateFormat.at(sz++).field_value), 79338432ULL);
    //ASSERT_EQ(std::get<int64_t>(vIntermediateFormat.at(sz++).field_value), -289834LL);
    //ASSERT_NEAR(std::get<float>(vIntermediateFormat.at(sz++).field_value), 2.54, 0.001);
    //ASSERT_NEAR(std::get<double>(vIntermediateFormat.at(sz++).field_value), 5.44061788e+03, 0.000001);
}
