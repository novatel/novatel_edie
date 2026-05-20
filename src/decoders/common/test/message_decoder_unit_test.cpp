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

#include <utility>

#include <gtest/gtest.h>

#include "novatel_edie/decoders/common/json_db_reader.hpp"
#include "novatel_edie/decoders/common/message_decoder.hpp"

using namespace novatel::edie;

class MessageDecoderTypesTest : public ::testing::Test
{
  protected:
    class DecoderTester : public MessageDecoderBase
    {
      public:
        DecoderTester(MessageDatabase::Ptr pclMessageDb_) : MessageDecoderBase("OEM", std::move(pclMessageDb_)) {}

        STATUS TestDecodeAscii(const FieldInfo& FieldInfo_, const char** ppcLogBuf_, MessageBody& vIntermediateFormat_)
        {
            return DecodeAscii<false>(FieldInfo_, ppcLogBuf_, vIntermediateFormat_);
        }

        STATUS TestDecodeBinary(const FieldInfo& FieldInfo_, const unsigned char** ppucLogBuf_, MessageBody& vIntermediateFormat_)
        {
            return DecodeBinary(FieldInfo_, ppucLogBuf_, vIntermediateFormat_, 0); // 0 for message length since it's not used in these tests
        }

        template <typename T, DATA_TYPE D> void ValidSimpleASCIIHelper(std::vector<std::string> vstrTestInput, std::vector<T> vTargets)
        {
            // this test expects the first two values in the test input to be the min and max, respectively
            // these values must be omitted from the vTargets argument when the function is called
            if constexpr (D != DATA_TYPE::UCHAR && D != DATA_TYPE::CHAR)
            {
                vTargets.insert(vTargets.begin(), std::numeric_limits<T>::max());
                vTargets.insert(vTargets.begin(), std::numeric_limits<T>::min());
            }

            for (size_t sz = 0; sz < vstrTestInput.size(); ++sz)
            {
                MessageBody vIntermediateFormat_(DataTypeSize(D), 0);

                // there is an issue here in that some data types can have multiple conversion strings or sizes
                // associated with them. In order to fix this, we may want these DataType functions to return a
                // vector so we can iterate through every possible valid combination of a basefield
                auto stMessageDataType = std::make_shared<const BaseField>("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), DataTypeSize(D), D);
                const char* tempStr = vstrTestInput[sz].c_str();
                DecodeAsciiField(stMessageDataType, &tempStr, vstrTestInput[sz].length(), vIntermediateFormat_);

                const T value = std::get<T>(vIntermediateFormat_.GetFieldValue(*stMessageDataType));
                if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
                {
                    const T tolerance = std::max<T>(std::numeric_limits<T>::epsilon() * static_cast<T>(10),
                                                    std::abs(vTargets[sz]) * std::numeric_limits<T>::epsilon());
                    ASSERT_NEAR(value, vTargets[sz], tolerance);
                }
                else { ASSERT_EQ(value, vTargets[sz]); }
            }
        }

        template <typename T, DATA_TYPE D> void InvalidSizeSimpleASCIIHelper(std::string_view strTestInput)
        {
            MessageBody vIntermediateFormat(DataTypeSize(D) + 2, 0);

            auto stMessageDataType = std::make_shared<const BaseField>("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), DataTypeSize(D) + 2, D);
            const char* tempStr = strTestInput.data();
            ASSERT_THROW(MessageDecoderBase::DecodeAsciiField(stMessageDataType, &tempStr, strTestInput.size(), vIntermediateFormat),
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
                MessageBody vIntermediateFormat_(DataTypeSize(D), 0);

                // there is an issue here in that some data types can have multiple conversion strings or sizes
                // associated with them. In order to fix this, we may want these DataType functions to return a
                // vector so we can iterate through every possible valid combination of a basefield
                auto stMessageDataType = std::make_shared<const BaseField>("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), DataTypeSize(D), D);
                // there should be a better way to do this
                const uint8_t* pucTestInput = vvucTestInput[sz].data();
                DecodeBinaryField(stMessageDataType, &pucTestInput, vIntermediateFormat_);

                const T value = std::get<T>(vIntermediateFormat_.GetFieldValue(*stMessageDataType));
                if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
                {
                    const T tolerance = std::max<T>(std::numeric_limits<T>::epsilon() * static_cast<T>(10),
                                                    std::abs(vTargets[sz]) * std::numeric_limits<T>::epsilon());
                    ASSERT_NEAR(value, vTargets[sz], tolerance);
                }
                else { ASSERT_EQ(value, vTargets[sz]); }
            }
        }
    };

  public:
    MessageDatabase::Ptr pclMyJsonDb;
    std::unique_ptr<DecoderTester> pclMyDecoderTester;
    std::vector<BaseField::Ptr> MsgDefFields;
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
            pclMyJsonDb = ParseJsonDb(sMinJsonDb);
            pclMyDecoderTester = std::make_unique<DecoderTester>(pclMyJsonDb);
        }
        catch (JsonDbReaderFailure& e)
        {
            std::cout << e.what() << '\n';
            MsgDefFields.clear();
        }
    }

    void TearDown() override
    {
        GetLoggerManager()->Shutdown();
        MsgDefFields.clear();
    }

    /*TODO: this function doesn populate the enum definition maps
    void CreateEnumField(const std::string& name, const std::string& description, int32_t value)
    {
        auto stField = std::make_shared<EnumField>();
        auto enumDef = std::make_shared<EnumDefinition>();
        EnumDataType enumDT;
        enumDT.name = name;
        enumDT.description = description;
        enumDT.value = value;
        enumDef->enumerators.push_back(enumDT);
        stField->enumDef = enumDef;
        stField->type = FIELD_TYPE::ENUM;
        MsgDefFields.emplace_back(stField);
    }*/
};

// TODO: we disable clang-format because of the long strings
// clang-format off

TEST_F(MessageDecoderTypesTest, LOGGER)
{
    spdlog::level::level_enum eLevel = spdlog::level::off;

    std::shared_ptr<spdlog::logger> novatel_message_decoder_logger = pclMyDecoderTester->GetLogger();
    ASSERT_NE(novatel_message_decoder_logger, nullptr);
    pclMyDecoderTester->SetLoggerLevel(eLevel);
    ASSERT_EQ(novatel_message_decoder_logger->level(), eLevel);
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
    pclMyDecoderTester->ValidSimpleASCIIHelper<float, DATA_TYPE::FLOAT>({"1.17549435e-38", "3.40282347e+38", "3279347.4", "-3.14"}, {3279347.4F, -3.14F});
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
    MsgDefFields.emplace_back(std::make_shared<BaseField>("CHAR_1", FIELD_TYPE::SIMPLE, "%c", 1, DATA_TYPE::CHAR));
    MessageBody vIntermediateFormat_(1, 0);

    const auto* testInput = "4";
    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat_));
    ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_.GetFieldValue(*MsgDefFields[0])), static_cast<int8_t>('4'));
}

TEST_F(MessageDecoderTypesTest, ASCII_BOOL_VALID_INPUT)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("B_True", FIELD_TYPE::SIMPLE, "%d", 4, DATA_TYPE::BOOL));
    MessageBody vIntermediateFormat_(4, 0);

    const auto* testInput = "TRUE";
    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat_));
    ASSERT_TRUE(std::get<bool>(vIntermediateFormat_.GetFieldValue(*MsgDefFields[0])));
}

TEST_F(MessageDecoderTypesTest, ASCII_ENUM_VALID)
{
    auto enumDef = std::make_shared<EnumDefinition>();
    enumDef->unknownValue = 0;
    enumDef->nameValue["UNKNOWN"] = 20;
    enumDef->nameValue["APPROXIMATE"] = 60;
    enumDef->nameValue["SATTIME"] = 200;

    auto e0 = std::make_shared<EnumField>();
    e0->name = "e0";
    e0->type = FIELD_TYPE::ENUM;
    e0->dataType.length = 4;
    e0->dataType.name = DATA_TYPE::UINT;
    e0->length = 4;
    e0->index = 0;
    e0->enumDef = enumDef;

    auto e1 = std::make_shared<EnumField>(*e0);
    e1->name = "e1";
    e1->index = 4;

    auto e2 = std::make_shared<EnumField>(*e0);
    e2->name = "e2";
    e2->index = 8;

    MsgDefFields.emplace_back(e0);
    MsgDefFields.emplace_back(e1);
    MsgDefFields.emplace_back(e2);

    MessageBody vIntermediateFormat(12, 0);

    const auto* testInput = "UNKNOWN,APPROXIMATE,SATTIME";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat), STATUS::SUCCESS);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValue(*e0)), 20U);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValue(*e1)), 60U);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValue(*e2)), 200U);
}

TEST_F(MessageDecoderTypesTest, ASCII_STRING_VALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("MESSAGE", FIELD_TYPE::STRING, "%", 1, DATA_TYPE::UNKNOWN));
    MessageBody vIntermediateFormat(0, 1);

    std::vector<const char*> testInputs = {"SOL_COMPUTED,WAAS"};
    std::vector<const char*> testTargets = {"SOL_COMPUTED","WAAS"};

    for (size_t sz = 0; sz < testInputs.size(); ++sz)
    {
        FieldInfo fieldInfo;
        std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
        fieldInfo.messageOrderedFields = constFields;
        fieldInfo.varFieldCount = 1;
        ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInputs[sz], vIntermediateFormat), STATUS::SUCCESS);
        ASSERT_EQ(std::get<std::string>(vIntermediateFormat.GetVarFields()[0]), testTargets[sz]);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_TYPE_INVALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("", FIELD_TYPE::UNKNOWN, "%d", 1, DATA_TYPE::UNKNOWN));
    MessageBody vIntermediateFormat_;

    const auto* testInput = "garbage";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat_), std::runtime_error);
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
    pclMyDecoderTester->ValidBinaryHelper<float, DATA_TYPE::FLOAT>({{0x00, 0x00, 0x80, 0x00}, {0xFF, 0xFF, 0x7F, 0x7F}, {0x9A, 0x99, 0x99, 0x3F}, {0xCD, 0xCC, 0xBC, 0xC0}}, {1.2F, -5.9F});
    pclMyDecoderTester->ValidBinaryHelper<double, DATA_TYPE::DOUBLE>({{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, {0.0});
}

TEST_F(MessageDecoderTypesTest, BINARY_SIMPLE_TYPE_INVALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("", FIELD_TYPE::SIMPLE, "%", 1, DATA_TYPE::UNKNOWN));
    MessageBody vIntermediateFormat_(1, 0);

    const unsigned char* testInput = nullptr;

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_THROW(pclMyDecoderTester->TestDecodeBinary(fieldInfo, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, BINARY_TYPE_INVALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("", FIELD_TYPE::UNKNOWN, "%", 1, DATA_TYPE::UNKNOWN));
    MessageBody vIntermediateFormat_(1, 0);

    const unsigned char* testInput = nullptr;

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_THROW(pclMyDecoderTester->TestDecodeBinary(fieldInfo, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, SIMPLE_FIELD_WIDTH_VALID)
{
    size_t offset = 0;
    auto addSimpleField = [&](std::string conversion, const size_t length, const DATA_TYPE dt) {
        auto field = std::make_shared<BaseField>("", FIELD_TYPE::SIMPLE, std::move(conversion), length, dt);
        field->index = offset;
        offset += length;
        MsgDefFields.emplace_back(field);
    };

    addSimpleField("%d", 1, DATA_TYPE::BOOL);
    addSimpleField("%XB", 1, DATA_TYPE::HEXBYTE);
    addSimpleField("%hd", 2, DATA_TYPE::SHORT);
    addSimpleField("%u", 4, DATA_TYPE::UINT);
    addSimpleField("%lu", 4, DATA_TYPE::ULONG);
    addSimpleField("%d", 4, DATA_TYPE::INT);
    addSimpleField("%ld", 4, DATA_TYPE::LONG);
    addSimpleField("%llu", 8, DATA_TYPE::ULONGLONG);
    addSimpleField("%lld", 8, DATA_TYPE::LONGLONG);
    addSimpleField("%hu", 2, DATA_TYPE::USHORT);
    addSimpleField("%lu", 4, DATA_TYPE::ULONG);
    addSimpleField("%d", 4, DATA_TYPE::INT);
    addSimpleField("%f", 4, DATA_TYPE::FLOAT);
    addSimpleField("%lf", 8, DATA_TYPE::DOUBLE);

    MessageBody vIntermediateFormat(offset, 0);

    const auto* testInput = "TRUE,63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat));
    ASSERT_TRUE(std::get<bool>(vIntermediateFormat.GetFieldValue(*MsgDefFields[0])));
    ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[1])), 0x63U);
    ASSERT_EQ(std::get<int16_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[2])), 227);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[3])), 56U);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[4])), 2734U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[5])), -3842);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[6])), 38283);
    ASSERT_EQ(std::get<uint64_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[7])), 54244ULL);
    ASSERT_EQ(std::get<int64_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[8])), -4359LL);
    ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[9])), 5293);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[10])), 79338432U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValue(*MsgDefFields[11])), -289834);
    ASSERT_NEAR(std::get<float>(vIntermediateFormat.GetFieldValue(*MsgDefFields[12])), 2.54F, 0.001F);
    ASSERT_NEAR(std::get<double>(vIntermediateFormat.GetFieldValue(*MsgDefFields[13])), 5.44061788e3, 0.001);
}
