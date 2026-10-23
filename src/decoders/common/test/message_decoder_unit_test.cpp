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

#include <algorithm>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "novatel_edie/common/test_utils/logger_registry_test.hpp"
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

        STATUS TestDecodeAscii(const FieldInfo& FieldInfo_, const char** ppcLogBuf_, CompositeField& vIntermediateFormat_)
        {
            return DecodeAscii<false>(FieldInfo_, ppcLogBuf_, vIntermediateFormat_);
        }

        STATUS TestDecodeBinary(const FieldInfo& FieldInfo_, const unsigned char** ppucLogBuf_, CompositeField& vIntermediateFormat_)
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
                CompositeField vIntermediateFormat_(DataTypeSize(D), 0);

                // there is an issue here in that some data types can have multiple conversion strings or sizes
                // associated with them. In order to fix this, we may want these DataType functions to return a
                // vector so we can iterate through every possible valid combination of a basefield
                auto stMessageDataType = std::make_shared<const BaseField>("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), D);
                const char* tempStr = vstrTestInput[sz].c_str();
                DecodeAsciiField(*stMessageDataType, &tempStr, vstrTestInput[sz].length(), vIntermediateFormat_);

                const T value = std::get<T>(vIntermediateFormat_.GetFieldValueVariant(*stMessageDataType));
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
            CompositeField vIntermediateFormat(DataTypeSize(D) + 2, 0);

            auto stMessageDataType = std::make_shared<const BaseField>("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), DataTypeSize(D) + 2, D);
            const char* tempStr = strTestInput.data();
            ASSERT_THROW(MessageDecoderBase::DecodeAsciiField(*stMessageDataType, &tempStr, strTestInput.size(), vIntermediateFormat),
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
                CompositeField vIntermediateFormat_(DataTypeSize(D), 0);

                // there is an issue here in that some data types can have multiple conversion strings or sizes
                // associated with them. In order to fix this, we may want these DataType functions to return a
                // vector so we can iterate through every possible valid combination of a basefield
                auto stMessageDataType = std::make_shared<const BaseField>("", FIELD_TYPE::SIMPLE, DataTypeConversion(D), D);
                // there should be a better way to do this
                const uint8_t* pucTestInput = vvucTestInput[sz].data();
                DecodeBinaryField(*stMessageDataType, &pucTestInput, vIntermediateFormat_);

                const T value = std::get<T>(vIntermediateFormat_.GetFieldValueVariant(*stMessageDataType));
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
                        \"messages\": [], \
                        \"meta\": { \
                           \"version\": \"1.0.0\" \
                        } \
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
    SKIP_IF_STATIC_SPDLOG_REGISTRY();
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

TEST_F(MessageDecoderTypesTest, ASCII_CHAR_BYTE_INVALID_INPUT)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("CHAR_1", FIELD_TYPE::SIMPLE, "%c", DATA_TYPE::CHAR));
    CompositeField vIntermediateFormat_(1, 0);

    const auto* testInput = "4";
    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat_));
    ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_.GetFieldValueVariant(*MsgDefFields[0])), static_cast<int8_t>('4'));
}

TEST_F(MessageDecoderTypesTest, ASCII_BOOL_VALID_INPUT)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("B_True", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::BOOL));
    CompositeField vIntermediateFormat_(4, 0);

    const auto* testInput = "TRUE";
    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat_));
    ASSERT_TRUE(std::get<bool>(vIntermediateFormat_.GetFieldValueVariant(*MsgDefFields[0])));
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
    e0->dataType.name = DATA_TYPE::INT;
    e0->index = 0;
    e0->enumDef = enumDef;

    auto e1 = std::make_shared<EnumField>(*e0);
    e1->name = "e1";
    e0->dataType.length = 4;
    e0->dataType.name = DATA_TYPE::INT;
    e1->index = 4;

    auto e2 = std::make_shared<EnumField>(*e0);
    e2->name = "e2";
    e0->dataType.length = 4;
    e0->dataType.name = DATA_TYPE::INT;
    e2->index = 8;

    MsgDefFields.emplace_back(e0);
    MsgDefFields.emplace_back(e1);
    MsgDefFields.emplace_back(e2);

    CompositeField vIntermediateFormat(12, 0);

    const auto* testInput = "UNKNOWN,APPROXIMATE,SATTIME";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat), STATUS::SUCCESS);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*e0)), 20U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*e1)), 60U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*e2)), 200U);
}

TEST_F(MessageDecoderTypesTest, ASCII_STRING_VALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("MESSAGE", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN));
    CompositeField vIntermediateFormat(0, 1);

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
    MsgDefFields.emplace_back(std::make_shared<BaseField>("", FIELD_TYPE::UNKNOWN, "%d", DATA_TYPE::UNKNOWN));
    CompositeField vIntermediateFormat_;

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
    MsgDefFields.emplace_back(std::make_shared<BaseField>("", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::UNKNOWN));
    CompositeField vIntermediateFormat_(1, 0);

    const unsigned char* testInput = nullptr;

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_THROW(pclMyDecoderTester->TestDecodeBinary(fieldInfo, &testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, BINARY_TYPE_INVALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("", FIELD_TYPE::UNKNOWN, "%d", DATA_TYPE::UNKNOWN));
    CompositeField vIntermediateFormat_(1, 0);

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
        auto field = std::make_shared<BaseField>("", FIELD_TYPE::SIMPLE, std::move(conversion), dt);
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

    CompositeField vIntermediateFormat(offset, 0);

    const auto* testInput = "TRUE,63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, &testInput, vIntermediateFormat));
    ASSERT_TRUE(std::get<bool>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[0])));
    ASSERT_EQ(std::get<uint8_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[1])), 0x63U);
    ASSERT_EQ(std::get<int16_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[2])), 227);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[3])), 56U);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[4])), 2734U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[5])), -3842);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[6])), 38283);
    ASSERT_EQ(std::get<uint64_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[7])), 54244ULL);
    ASSERT_EQ(std::get<int64_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[8])), -4359LL);
    ASSERT_EQ(std::get<uint16_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[9])), 5293);
    ASSERT_EQ(std::get<uint32_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[10])), 79338432U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[11])), -289834);
    ASSERT_NEAR(std::get<float>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[12])), 2.54F, 0.001F);
    ASSERT_NEAR(std::get<double>(vIntermediateFormat.GetFieldValueVariant(*MsgDefFields[13])), 5.44061788e3, 0.001);
}

TEST(MessageDecoderContainerTypesTest, FixedFieldRegionSimpleFieldRoundTripAndBounds)
{
    FixedFieldRegion fields(sizeof(uint32_t));
    BaseField simpleField("simple", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    simpleField.index = 0;

    const uint32_t inValue = 0xAABBCCDDU;
    fields.SetFieldValue(0, inValue);
    EXPECT_EQ(LoadFixedField<uint32_t>(fields, simpleField), inValue);

    EXPECT_THROW(fields.SetFieldValue(sizeof(uint16_t), inValue), std::runtime_error);
}

TEST(MessageDecoderContainerTypesTest, FixedFieldRegionArrayElementAccess)
{
    FixedFieldRegion fields(3 * sizeof(uint32_t));
    ArrayField arrayField("arr", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%u", DATA_TYPE::UINT, 3);
    arrayField.index = 0;

    const uint32_t values[3] = {10U, 20U, 30U};
    fields.SetFieldValue(0, values, 3);

    EXPECT_EQ(LoadFixedFieldElement<uint32_t>(fields, 0, arrayField), values[0]);
    EXPECT_EQ(LoadFixedFieldElement<uint32_t>(fields, 1, arrayField), values[1]);
    EXPECT_EQ(LoadFixedFieldElement<uint32_t>(fields, 2, arrayField), values[2]);
    EXPECT_THROW((void)LoadFixedFieldElement<uint32_t>(fields, 3, arrayField), std::runtime_error);
}

TEST(MessageDecoderContainerTypesTest, FixedStorageRequiresValidSchemaAndWholeRecords)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({valueField});
    EXPECT_THROW((void)FieldArray(std::vector<std::byte>{}, nullptr), std::runtime_error);
    EXPECT_THROW((void)FieldArray(std::vector<std::byte>(1), fieldInfo), std::runtime_error);
    EXPECT_THROW((void)FieldArray(std::vector<std::byte>{}, std::make_shared<const FieldInfo>()), std::runtime_error);
    auto stringField = std::make_shared<BaseField>("text", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);
    const auto variableInfo = BuildFieldInfo({valueField, stringField});
    EXPECT_THROW((void)FieldArray(std::vector<std::byte>{}, variableInfo), std::runtime_error);
    const FieldArray empty(std::vector<std::byte>{}, fieldInfo);
    EXPECT_TRUE(empty.empty());
    EXPECT_TRUE(empty.ForEachRecord([](const CompositeField&) { ADD_FAILURE(); return false; }));
}

TEST(MessageDecoderContainerTypesTest, FixedStorageWritesReplacesAndVisitsRecords)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({valueField});
    FieldArray records(std::vector<std::byte>(2 * fieldInfo->fixedFieldBytes), fieldInfo);
    const uint32_t values[] = {11U, 22U};
    records[0].SetFieldValue(*valueField, values[0]);
    records[1].SetFieldValue(*valueField, values[1]);
    ASSERT_EQ(records.size(), 2U);
    EXPECT_THROW(records[2].SetFieldValue(*valueField, values[0]), std::runtime_error);

    CompositeField replacement(fieldInfo);
    replacement.SetFieldValue(*valueField, 33U);
    const auto previous = records[1].ReplaceRecord(replacement);
    EXPECT_EQ(previous.GetFieldValue<uint32_t>(*valueField), 22U);
    EXPECT_EQ(records[0].GetFieldValue<uint32_t>(*valueField), 11U);
    EXPECT_EQ(records[1].GetFieldValue<uint32_t>(*valueField), 33U);
    EXPECT_THROW((void)records[2].ReplaceRecord(replacement), std::out_of_range);
    EXPECT_THROW((void)records[0].ReplaceRecord(CompositeField(0, 1)), std::runtime_error);

    std::vector<uint32_t> visited;
    EXPECT_TRUE(records.ForEachRecord([&](const CompositeField& record) {
        visited.push_back(record.GetFieldValue<uint32_t>(*valueField));
        return true;
    }));
    EXPECT_EQ(visited, (std::vector<uint32_t>{11U, 33U}));
    size_t visits = 0;
    EXPECT_FALSE(records.ForEachRecord([&](const CompositeField&) { ++visits; return false; }));
    EXPECT_EQ(visits, 1U);
}

TEST(MessageDecoderContainerTypesTest, RecordViewSetFieldValueForBothStorageTypes)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto arrayField = std::make_shared<ArrayField>("array", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%u", DATA_TYPE::UINT, 2);
    auto textField = std::make_shared<ArrayField>("text", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%s", DATA_TYPE::UCHAR, 4);
    const auto fieldInfo = BuildFieldInfo({valueField, arrayField, textField});
    FieldArray arrays[] = {FieldArray({CompositeField(fieldInfo), CompositeField(fieldInfo)}, fieldInfo),
                          FieldArray(std::vector<std::byte>(2 * fieldInfo->fixedFieldBytes), fieldInfo)};
    for (auto& array : arrays)
    {
        const auto row = array[1];
        row.SetFieldValue(*valueField, 11U);
        EXPECT_EQ(row.GetFieldValue<uint32_t>(*valueField), 11U);
        row.SetFieldValue(*valueField, 22U);
        EXPECT_EQ(row.GetFieldValue<uint32_t>(*valueField), 22U);

        const uint32_t values[] = {33U, 44U};
        row.SetFieldValue(*arrayField, values, 2);
        EXPECT_EQ(row.GetFieldValue<uint32_t>(*arrayField, 1), 44U);
        row.SetFieldValue(*arrayField, std::vector<uint32_t>{55U, 66U});
        EXPECT_EQ(row.GetFieldValue<uint32_t>(*arrayField), 55U);
        const auto buffer = row.GetFieldValue<TypedBuffer<uint32_t>>(*arrayField);
        array[0].SetFieldValue(*arrayField, buffer);
        EXPECT_EQ(array[0].GetFieldValue<uint32_t>(*arrayField, 1), 66U);

        const std::string text = "abc";
        row.SetFieldValue(*textField, text);
        EXPECT_EQ(row.GetFieldValue<uint8_t>(*textField, 1), static_cast<uint8_t>('b'));
        row.SetFieldValue(*textField, std::string("xyz"));
        EXPECT_EQ(row.GetFieldValue<uint8_t>(*textField, 1), static_cast<uint8_t>('y'));
        EXPECT_EQ(array[0].GetFieldValue<uint32_t>(*valueField), 0U);
    }
}

TEST(MessageDecoderContainerTypesTest, PointerSetFieldValueRejectsCountLargerThanField)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto arrayField = std::make_shared<ArrayField>("array", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%u", DATA_TYPE::UINT, 2);
    auto nextField = std::make_shared<BaseField>("next", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({valueField, arrayField, nextField});
    const uint32_t values[] = {1U, 2U, 3U};
    const uint64_t wide = 4U;

    // Each write is too large for its field and would overwrite the field after it
    CompositeField body(fieldInfo);
    EXPECT_THROW(body.SetFieldValue(*valueField, values, 2), std::runtime_error);
    EXPECT_THROW(body.SetFieldValue(*arrayField, values, 3), std::runtime_error);
    EXPECT_THROW(body.SetFieldValue(*valueField, &wide, 1), std::runtime_error);
    EXPECT_EQ(body.GetFieldValue<uint32_t>(*arrayField), 0U);
    EXPECT_EQ(body.GetFieldValue<uint32_t>(*nextField), 0U);

    FieldArray arrays[] = {FieldArray({CompositeField(fieldInfo)}, fieldInfo),
                           FieldArray(std::vector<std::byte>(fieldInfo->fixedFieldBytes), fieldInfo)};
    for (auto& array : arrays)
    {
        EXPECT_THROW(array[0].SetFieldValue(*valueField, values, 2), std::runtime_error);
        EXPECT_THROW(array[0].SetFieldValue(*arrayField, values, 3), std::runtime_error);
        EXPECT_EQ(array[0].GetFieldValue<uint32_t>(*arrayField), 0U);
        EXPECT_EQ(array[0].GetFieldValue<uint32_t>(*nextField), 0U);
    }
}

TEST(MessageDecoderContainerTypesTest, RecordViewSetFieldValueVariableFields)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto arrayField = std::make_shared<ArrayField>("array", FIELD_TYPE::VARIABLE_LENGTH_ARRAY, "%u", DATA_TYPE::UINT, 3);
    auto textField = std::make_shared<BaseField>("text", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);
    const auto fieldInfo = BuildFieldInfo({valueField, arrayField, textField});
    FieldArray records({CompositeField(fieldInfo)}, fieldInfo);
    const auto row = records[0];
    const uint32_t values[] = {11U, 22U};
    row.SetFieldValue(*arrayField, values, 2);
    EXPECT_EQ(row.GetFieldValue<std::vector<uint32_t>>(*arrayField), (std::vector<uint32_t>{11U, 22U}));
    const std::vector<uint32_t> vectorValues{33U, 44U};
    row.SetFieldValue(*arrayField, vectorValues);
    EXPECT_EQ(row.GetFieldValue<uint32_t>(*arrayField, 1), 44U);
    row.SetFieldValue(*arrayField, std::vector<uint32_t>{45U});
    EXPECT_EQ(row.GetFieldValue<uint32_t>(*arrayField), 45U);
    row.SetFieldValue(*arrayField, std::vector<uint32_t>{55U});
    EXPECT_EQ(row.GetFieldValue<uint32_t>(*arrayField), 55U);
    row.SetFieldValue(*arrayField, std::vector<uint32_t>{66U});
    EXPECT_EQ(row.GetFieldValue<uint32_t>(*arrayField), 66U);
    const std::string text = "abc";
    row.SetFieldValue(*textField, text);
    EXPECT_EQ(row.GetFieldValue<std::string>(*textField), text);
    row.SetFieldValue(*textField, std::string("moved"));
    EXPECT_EQ(row.GetFieldValue<std::string>(*textField), "moved");
    row.SetFieldValue(*textField, std::string("xyz"));
    EXPECT_EQ(row.GetFieldValue<std::string>(*textField), "xyz");
    EXPECT_THROW(row.SetFieldValue(*textField, values, 2), std::runtime_error);

    const auto fixedInfo = BuildFieldInfo({valueField});
    FieldArray flat(std::vector<std::byte>(fixedInfo->fixedFieldBytes), fixedInfo);
    EXPECT_THROW(flat[0].SetFieldValue(*arrayField, values, 2), std::runtime_error);
    EXPECT_THROW(flat[0].SetFieldValue(*textField, text), std::runtime_error);
}

TEST(MessageDecoderContainerTypesTest, WriteFieldToBufferCompositeHandler)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto recordInfo = BuildFieldInfo({valueField});
    auto arrayField = std::make_shared<FieldArrayField>("records", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 2, recordInfo);
    CompositeField body(BuildFieldInfo({arrayField}));
    CompositeField first(recordInfo), second(recordInfo);
    first.SetFieldValue(*valueField, 11U);
    second.SetFieldValue(*valueField, 22U);
    body.SetFieldValue(*arrayField, FieldArray({first, second}, recordInfo));

    std::vector<std::byte> buffer(12);
    EXPECT_THROW((void)body.WriteFieldToBuffer(*arrayField, buffer.data(), buffer.size()), std::runtime_error);
    size_t calls = 0;
    const auto handler = [&](const CompositeField& record, std::byte* destination,
                                                               size_t capacity) -> std::optional<size_t> {
        ++calls;
        const auto written = record.WriteFieldToBuffer(*valueField, destination, capacity);
        return written == 0 ? std::nullopt : std::optional<size_t>(written);
    };
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, buffer.data(), buffer.size(), handler), 8U);
    EXPECT_EQ(calls, 2U);
    EXPECT_EQ(LoadValueFromBuffer<uint32_t>(buffer.data()), 11U);
    EXPECT_EQ(LoadValueFromBuffer<uint32_t>(buffer.data() + 4), 22U);

    auto* cursor = reinterpret_cast<unsigned char*>(buffer.data());
    auto* const start = cursor;
    uint32_t capacity = static_cast<uint32_t>(buffer.size());
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, &cursor, capacity, handler), 8U);
    EXPECT_EQ(cursor, start + 8);
    EXPECT_EQ(capacity, 4U);
    cursor = start;
    capacity = 4;
    calls = 0;
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, &cursor, capacity, handler), 0U);
    EXPECT_EQ(cursor, start);
    EXPECT_EQ(capacity, 4U);
    EXPECT_EQ(calls, 2U);

    const auto failing = [&](const CompositeField&, std::byte*, size_t) -> std::optional<size_t> {
        ++calls;
        return std::nullopt;
    };
    calls = 0;
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, buffer.data(), buffer.size(), failing), 0U);
    EXPECT_EQ(calls, 1U);
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, buffer.data(), buffer.size(),
                                     [](const CompositeField&, std::byte*, size_t available) { return available + 1; }), 0U);
    calls = 0;
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, buffer.data(), buffer.size(),
                                     [&](const CompositeField&, std::byte*, size_t) { ++calls; return 0; }), 0U);
    EXPECT_EQ(calls, 2U);

    FieldArray flat(std::vector<std::byte>(8), recordInfo);
    const uint32_t values[] = {33U, 44U};
    flat[0].SetFieldValue(*valueField, values[0]);
    flat[1].SetFieldValue(*valueField, values[1]);
    body.SetFieldValue(*arrayField, std::move(flat));
    calls = 0;
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, buffer.data(), buffer.size(), failing), 8U);
    EXPECT_EQ(calls, 0U);
    EXPECT_EQ(LoadValueFromBuffer<uint32_t>(buffer.data() + 4), 44U);
    EXPECT_EQ(first.WriteFieldToBuffer(*valueField, buffer.data(), buffer.size(), failing), 4U);
    EXPECT_EQ(calls, 0U);
    body.SetFieldValue(*arrayField, FieldArray(std::vector<CompositeField>{}, recordInfo));
    EXPECT_EQ(body.WriteFieldToBuffer(*arrayField, buffer.data(), buffer.size(), failing), 0U);
    EXPECT_EQ(calls, 0U);
}

TEST(MessageDecoderContainerTypesTest, FieldArrayIteratorOperationsForBothStorageTypes)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({valueField});
    const FieldArray emptyArrays[] = {FieldArray(std::vector<CompositeField>{}, fieldInfo),
                                     FieldArray(std::vector<std::byte>{}, fieldInfo)};
    for (const auto& array : emptyArrays)
    {
        EXPECT_EQ(array.begin(), array.end());
        EXPECT_EQ(std::distance(array.begin(), array.end()), 0);
        for (const auto& record : array)
        {
            (void)record;
            ADD_FAILURE() << "Empty FieldArray must not yield records";
        }
    }

    CompositeField first(fieldInfo), second(fieldInfo);
    first.SetFieldValue(*valueField, 11U);
    second.SetFieldValue(*valueField, 22U);
    FieldArray arrays[] = {FieldArray({first, second}, fieldInfo),
                          FieldArray(std::vector<std::byte>(2 * fieldInfo->fixedFieldBytes), fieldInfo)};
    const uint32_t values[] = {11U, 22U};
    arrays[1][0].SetFieldValue(*valueField, values[0]);
    arrays[1][1].SetFieldValue(*valueField, values[1]);
    for (const auto& array : arrays)
    {
        auto current = array.begin();
        const auto copy = current;
        const auto firstRecord = *current;
        EXPECT_EQ(current, copy);
        EXPECT_EQ(current++, copy);
        EXPECT_NE(current, copy);
        EXPECT_EQ((*copy).GetFieldValue<uint32_t>(*valueField), 11U);
        EXPECT_EQ((*current).GetFieldValue<uint32_t>(*valueField), 22U);
        EXPECT_EQ(firstRecord.GetFieldValue<uint32_t>(*valueField), 11U);
        EXPECT_EQ(&(++current), &current);
        EXPECT_EQ(current, array.end());

        std::vector<uint32_t> visited;
        for (const auto& record : array) { visited.push_back(record.GetFieldValue<uint32_t>(*valueField)); }
        EXPECT_EQ(visited, (std::vector<uint32_t>{11U, 22U}));
    }
}

TEST(MessageDecoderContainerTypesTest, FixedStorageReplaceRecordRejectsWrongRecordSize)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({valueField});
    FieldArray records(std::vector<std::byte>(2 * fieldInfo->fixedFieldBytes), fieldInfo);

    // A 1-byte record cannot fill a 4-byte row; copying fixedFieldBytes from it reads past its storage
    EXPECT_THROW((void)records[0].ReplaceRecord(CompositeField(1, 0)), std::runtime_error);
}

TEST(MessageDecoderContainerTypesTest, FieldArrayRowIndexOutOfRangeThrowsForBothStorageTypes)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({valueField});
    const FieldArray arrays[] = {FieldArray({CompositeField(fieldInfo), CompositeField(fieldInfo)}, fieldInfo),
                                 FieldArray(std::vector<std::byte>(2 * fieldInfo->fixedFieldBytes), fieldInfo)};
    for (const auto& array : arrays)
    {
        EXPECT_THROW((void)array[2].GetFieldValue<uint32_t>(*valueField), std::out_of_range);
        EXPECT_THROW((void)array.GetFieldValueByName<uint32_t>("value", 2), std::out_of_range);
    }
}

TEST(MessageDecoderContainerTypesTest, CompositeFieldIsNothrowMovable)
{
    // Without nothrow moves, std::vector<CompositeField> copies every record when it grows,
    // and moving a CompositeField (e.g. into a FieldArray) deep-copies its storage.
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<CompositeField>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<CompositeField>);
}

TEST(MessageDecoderContainerTypesTest, ForEachRecordProvidesFieldInfoForBothStorageTypes)
{
    auto valueField = std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({valueField});
    CompositeField record(fieldInfo);
    record.SetFieldValue(*valueField, 7U);
    FieldArray arrays[] = {FieldArray({record}, fieldInfo),
                           FieldArray(std::vector<std::byte>(record.GetFixedFields().data(),
                                                             record.GetFixedFields().data() + fieldInfo->fixedFieldBytes),
                                      fieldInfo)};
    for (auto& array : arrays)
    {
        // Records must behave the same regardless of how the array stores them
        EXPECT_TRUE(array.ForEachRecord([](const CompositeField& r) { return r.GetFieldValueByName<uint32_t>("value") == 7U; }));
        EXPECT_EQ(array[0].ReplaceRecord(record).GetFieldValueByName<uint32_t>("value"), 7U);
    }
}

TEST(MessageDecoderContainerTypesTest, FixedStorageIteratorBuildsExpectedMessageBodyPerField)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("i16", FIELD_TYPE::SIMPLE, "%hd", DATA_TYPE::SHORT);

    const auto fieldInfo = BuildFieldInfo({f0, f1});
    FixedFieldRegion fieldArray(2 * fieldInfo->fixedFieldBytes);

    fieldArray.SetFieldValue<uint32_t>(f0->index, 11U);
    fieldArray.SetFieldValue<int16_t>(f1->index, static_cast<int16_t>(-7));
    fieldArray.SetFieldValue<uint32_t>(fieldInfo->fixedFieldBytes + f0->index, 22U);
    fieldArray.SetFieldValue<int16_t>(fieldInfo->fixedFieldBytes + f1->index, static_cast<int16_t>(9));

    FieldArray wrapped(std::vector<std::byte>(fieldArray.data(), fieldArray.data() + fieldArray.size()), fieldInfo);
    auto it = wrapped.begin();
    const auto row0 = *it;
    EXPECT_EQ(row0.GetFieldValue<uint32_t>(*f0), 11U);
    EXPECT_EQ(row0.GetFieldValue<int16_t>(*f1), static_cast<int16_t>(-7));

    ++it;
    const auto row1 = *it;
    EXPECT_EQ(row1.GetFieldValue<uint32_t>(*f0), 22U);
    EXPECT_EQ(row1.GetFieldValue<int16_t>(*f1), static_cast<int16_t>(9));
}

TEST(MessageDecoderContainerTypesTest, FixedStorageIteratorFind)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("i16", FIELD_TYPE::SIMPLE, "%hd", DATA_TYPE::SHORT);

    const auto fieldInfo = BuildFieldInfo({f0, f1});
    FixedFieldRegion fieldArray(3 * fieldInfo->fixedFieldBytes);

    fieldArray.SetFieldValue<uint32_t>(f0->index, 11U);
    fieldArray.SetFieldValue<int16_t>(f1->index, static_cast<int16_t>(-7));
    fieldArray.SetFieldValue<uint32_t>(fieldInfo->fixedFieldBytes + f0->index, 22U);
    fieldArray.SetFieldValue<int16_t>(fieldInfo->fixedFieldBytes + f1->index, static_cast<int16_t>(9));
    fieldArray.SetFieldValue<uint32_t>((2 * fieldInfo->fixedFieldBytes) + f0->index, 33U);
    fieldArray.SetFieldValue<int16_t>((2 * fieldInfo->fixedFieldBytes) + f1->index, static_cast<int16_t>(15));

    FieldArray wrapped(std::vector<std::byte>(fieldArray.data(), fieldArray.data() + fieldArray.size()), fieldInfo);
    const auto it = std::find_if(wrapped.begin(), wrapped.end(), [&](const auto& row) {
        return row.GetFieldValue<uint32_t>(*f0) == 22U;
    });
    ASSERT_NE(it, wrapped.end());
    EXPECT_EQ(std::distance(wrapped.begin(), it), 1);
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*f0), 22U);
    EXPECT_EQ((*it).GetFieldValue<int16_t>(*f1), static_cast<int16_t>(9));

    const auto missing = std::find_if(wrapped.begin(), wrapped.end(), [&](const auto& row) {
        return row.GetFieldValue<uint32_t>(*f0) == 99U;
    });
    EXPECT_EQ(missing, wrapped.end());

    EXPECT_EQ(std::count_if(wrapped.begin(), wrapped.end(), [&](const auto& row) {
        return row.GetFieldValue<int16_t>(*f1) > 0;
    }), 2);
}

TEST(MessageDecoderContainerTypesTest, FieldArrayWrapperReadsFlatBackedFieldArray)
{
    auto nestedU32 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto nestedI16 = std::make_shared<BaseField>("i16", FIELD_TYPE::SIMPLE, "%hd", DATA_TYPE::SHORT);
    const auto nestedFieldInfo = BuildFieldInfo({nestedU32, nestedI16});

    auto fieldArrayField = std::make_shared<FieldArrayField>("fa", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 2, nestedFieldInfo);
    const auto rootFieldInfo = BuildFieldInfo({fieldArrayField});

    FieldArray stored(std::vector<std::byte>(2 * nestedFieldInfo->fixedFieldBytes), nestedFieldInfo);
    const uint32_t u32Values[] = {11U, 22U};
    const int16_t i16Values[] = {-7, 9};
    for (size_t i = 0; i < 2; ++i)
    {
        stored[i].SetFieldValue(*nestedU32, u32Values[i]);
        stored[i].SetFieldValue(*nestedI16, i16Values[i]);
    }

    CompositeField body(rootFieldInfo);
    body.SetFieldValue(*fieldArrayField, std::move(stored));

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    ASSERT_EQ(wrapped.size(), 2U);
    EXPECT_EQ(wrapped.GetFieldInfo(), nestedFieldInfo);
    EXPECT_EQ(wrapped.GetFieldValue<uint32_t>(*nestedU32, 0), 11U);
    EXPECT_EQ(wrapped.GetFieldValue<int16_t>(*nestedI16, 1), static_cast<int16_t>(9));

    auto it = wrapped.begin();
    const auto row0 = *it;
    EXPECT_EQ(row0.GetFieldValue<uint32_t>(*nestedU32), 11U);
    EXPECT_EQ(row0.GetFieldValue<int16_t>(*nestedI16), static_cast<int16_t>(-7));

    ++it;
    const auto row1 = *it;
    EXPECT_EQ(row1.GetFieldValue<uint32_t>(*nestedU32), 22U);
    EXPECT_EQ(row1.GetFieldValue<int16_t>(*nestedI16), static_cast<int16_t>(9));

    ++it;
    EXPECT_EQ(it, wrapped.end());
}

TEST(MessageDecoderContainerTypesTest, FieldArrayFlatBackedIteratorFind)
{
    auto nestedU32 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto nestedI16 = std::make_shared<BaseField>("i16", FIELD_TYPE::SIMPLE, "%hd", DATA_TYPE::SHORT);
    const auto nestedFieldInfo = BuildFieldInfo({nestedU32, nestedI16});

    auto fieldArrayField = std::make_shared<FieldArrayField>("fa", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 2, nestedFieldInfo);
    const auto rootFieldInfo = BuildFieldInfo({fieldArrayField});

    FieldArray stored(std::vector<std::byte>(2 * nestedFieldInfo->fixedFieldBytes), nestedFieldInfo);
    const uint32_t u32Values[] = {11U, 22U};
    const int16_t i16Values[] = {-7, 9};
    for (size_t i = 0; i < 2; ++i)
    {
        stored[i].SetFieldValue(*nestedU32, u32Values[i]);
        stored[i].SetFieldValue(*nestedI16, i16Values[i]);
    }

    CompositeField body(rootFieldInfo);
    body.SetFieldValue(*fieldArrayField, std::move(stored));

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    const auto it = std::find_if(wrapped.begin(), wrapped.end(), [&](const auto& row) {
        return row.GetFieldValue<uint32_t>(*nestedU32) == 22U;
    });
    ASSERT_NE(it, wrapped.end());
    EXPECT_EQ(std::distance(wrapped.begin(), it), 1);
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*nestedU32), 22U);
    EXPECT_EQ((*it).GetFieldValue<int16_t>(*nestedI16), static_cast<int16_t>(9));
    EXPECT_EQ(std::count_if(wrapped.begin(), wrapped.end(), [&](const auto& row) {
        return row.GetFieldValue<int16_t>(*nestedI16) > 0;
    }), 1);
}

TEST(MessageDecoderContainerTypesTest, FieldArrayWrapperReadsCompositeBackedFieldArray)
{
    auto nestedU32 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto nestedStr = std::make_shared<BaseField>("str", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);
    const auto nestedFieldInfo = BuildFieldInfo({nestedU32, nestedStr});

    auto fieldArrayField = std::make_shared<FieldArrayField>("fa", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 4, nestedFieldInfo);
    const auto rootFieldInfo = BuildFieldInfo({fieldArrayField});

    CompositeField row0(nestedFieldInfo);
    row0.SetFieldValue(*nestedU32, 10U);
    row0.SetFieldValue(*nestedStr, std::string("ten"));

    CompositeField row1(nestedFieldInfo);
    row1.SetFieldValue(*nestedU32, 20U);
    row1.SetFieldValue(*nestedStr, std::string("twenty"));

    CompositeField body(rootFieldInfo);
    body.SetFieldValue(*fieldArrayField, FieldArray({row0, row1}, nestedFieldInfo));

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    ASSERT_EQ(wrapped.size(), 2U);
    EXPECT_EQ(wrapped.GetFieldInfo(), nestedFieldInfo);
    EXPECT_EQ(wrapped.GetFieldValue<uint32_t>(*nestedU32, 0), 10U);
    EXPECT_EQ(wrapped.GetFieldValue<std::string>(*nestedStr, 1), "twenty");

    const FieldArray::FieldArrayRecordView firstRow = wrapped[0];
    EXPECT_EQ(firstRow.GetFieldValue<uint32_t>(*nestedU32), 10U);
    EXPECT_EQ(firstRow.GetFieldValue<std::string>(*nestedStr), "ten");
    EXPECT_EQ(firstRow.GetFieldValueByName<std::string>("str"), "ten");
    EXPECT_THROW((void)firstRow.GetFieldValueByName<uint32_t>("missing"), std::runtime_error);

    auto it = wrapped.begin();
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*nestedU32), 10U);
    EXPECT_EQ((*it).GetFieldValue<std::string>(*nestedStr), "ten");
    
    ++it;
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*nestedU32), 20U);
    EXPECT_EQ((*it).GetFieldValue<std::string>(*nestedStr), "twenty");

    ++it;
    EXPECT_EQ(it, wrapped.end());
}

TEST(MessageDecoderContainerTypesTest, FieldArrayCompositeBackedIteratorFind)
{
    auto nestedU32 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto nestedStr = std::make_shared<BaseField>("str", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);
    const auto nestedFieldInfo = BuildFieldInfo({nestedU32, nestedStr});

    auto fieldArrayField = std::make_shared<FieldArrayField>("fa", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 4, nestedFieldInfo);
    const auto rootFieldInfo = BuildFieldInfo({fieldArrayField});

    CompositeField row0(nestedFieldInfo);
    row0.SetFieldValue(*nestedU32, 10U);
    row0.SetFieldValue(*nestedStr, std::string("ten"));

    CompositeField row1(nestedFieldInfo);
    row1.SetFieldValue(*nestedU32, 20U);
    row1.SetFieldValue(*nestedStr, std::string("twenty"));

    CompositeField body(rootFieldInfo);
    body.SetFieldValue(*fieldArrayField, FieldArray({row0, row1}, nestedFieldInfo));

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    const auto it = std::find_if(wrapped.begin(), wrapped.end(), [&](const auto& row) {
        return row.GetFieldValue<uint32_t>(*nestedU32) == 20U;
    });
    ASSERT_NE(it, wrapped.end());
    EXPECT_EQ(std::distance(wrapped.begin(), it), 1);
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*nestedU32), 20U);
    EXPECT_EQ((*it).GetFieldValue<std::string>(*nestedStr), "twenty");
    EXPECT_EQ(std::count_if(wrapped.begin(), wrapped.end(), [&](const auto& row) {
        return row.GetFieldValue<uint32_t>(*nestedU32) > 0U;
    }), 2);
}

TEST(MessageDecoderContainerTypesTest, FieldArrayWrapperPreservesSchemaForEmptyCompositeFieldArray)
{
    auto nestedU32 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto nestedStr = std::make_shared<BaseField>("str", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);
    const auto nestedFieldInfo = BuildFieldInfo({nestedU32, nestedStr});

    auto fieldArrayField = std::make_shared<FieldArrayField>("fa", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 4, nestedFieldInfo);
    const auto rootFieldInfo = BuildFieldInfo({fieldArrayField});

    CompositeField body(rootFieldInfo);
    body.SetFieldValue(*fieldArrayField, FieldArray(std::vector<CompositeField>{}, nestedFieldInfo));

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    EXPECT_TRUE(wrapped.empty());
    EXPECT_EQ(wrapped.GetFieldInfo(), nestedFieldInfo);
}

TEST(MessageDecoderContainerTypesTest, CompositeFieldGetFieldValueByName)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("str", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);

    const auto fieldInfo = BuildFieldInfo({f0, f1});
    CompositeField stMessage(fieldInfo);

    stMessage.SetFieldValue(*f0, uint32_t{10});
    stMessage.SetFieldValue(*f1, std::string("ten"));

    EXPECT_EQ(stMessage.GetFieldValueByName<uint32_t>("u32"), 10);
    EXPECT_EQ(stMessage.GetFieldValueByName<std::string>("str"), "ten");
    EXPECT_THROW((void)stMessage.GetFieldValueByName<uint8_t>("error"), std::runtime_error);
}

TEST(MessageDecoderContainerTypesTest, FixedStorageRecordViewGetFieldValueByName)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<ArrayField>("arr", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%s", DATA_TYPE::UCHAR, 7);
    const auto fieldInfo = BuildFieldInfo({f0, f1});

    FixedFieldRegion fields(2 * fieldInfo->fixedFieldBytes);
    fields.SetFieldValue(f0->index, 10U);
    fields.SetFieldValue(f1->index, std::string("ten"));
    fields.SetFieldValue(fieldInfo->fixedFieldBytes + f0->index, 20U);
    fields.SetFieldValue(fieldInfo->fixedFieldBytes + f1->index, std::string("twenty"));

    FieldArray wrapped(std::vector<std::byte>(fields.data(), fields.data() + fields.size()), fieldInfo);
    auto tenStr = std::string("ten");
    auto twentyStr = std::string("twenty");
    EXPECT_EQ(wrapped[0].GetFieldValueByName<uint32_t>("u32"), 10);
    const auto arr1 = wrapped[0].GetFieldValueByName<TypedBuffer<uint8_t>>("arr");
    for (size_t i = 0; i < tenStr.size(); i++) { EXPECT_EQ(arr1[i], static_cast<uint8_t>(tenStr[i])); }
    EXPECT_EQ(wrapped[1].GetFieldValueByName<uint32_t>("u32"), 20);
    const auto arr2 = wrapped[1].GetFieldValueByName<TypedBuffer<uint8_t>>("arr");
    for (size_t i = 0; i < twentyStr.size(); i++) { EXPECT_EQ(arr2[i], static_cast<uint8_t>(twentyStr[i])); }

    const FieldArray::FieldArrayRecordView row = wrapped[1];
    EXPECT_EQ(row.GetFieldValue<uint8_t>(*f1, 2), static_cast<uint8_t>('e'));
    EXPECT_EQ(row.GetFieldValueByName<uint8_t>("arr", 2), static_cast<uint8_t>('e'));
    EXPECT_THROW((void)row.GetFieldValue<uint8_t>(*f1, 7), std::runtime_error);
    EXPECT_THROW((void)row.GetFieldValueByName<uint32_t>("missing"), std::runtime_error);
    EXPECT_THROW((void)row.GetFieldValue<std::string>(*f0), std::runtime_error);
    const uint32_t replacement = 30U;
    wrapped[1].SetFieldValue(*f0, replacement);
    EXPECT_EQ(row.GetFieldValueByName<uint32_t>("u32"), replacement);
    EXPECT_THROW((void)FieldArray{}[0].GetFieldValueByName<uint32_t>("u32"), std::runtime_error);
}

TEST(MessageDecoderContainerTypesTest, CompositeFieldSetFieldValueFixedLengthArray)
{
    auto charArrField = std::make_shared<ArrayField>("str_arr", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%s", DATA_TYPE::UCHAR, 8);
    auto uintArrField = std::make_shared<ArrayField>("uint_arr", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%u", DATA_TYPE::UINT, 4);

    const auto fieldInfo = BuildFieldInfo({charArrField, uintArrField});
    CompositeField body(fieldInfo);

    // Set FIXED_LENGTH_ARRAY using std::string
    const std::string strVal = "NOVATEL";
    body.SetFieldValue(*charArrField, strVal);
    const auto readBufChar = body.GetFieldValue<TypedBuffer<uint8_t>>(*charArrField);
    ASSERT_EQ(readBufChar.size(), 8U);
    for (size_t i = 0; i < strVal.size(); ++i) { EXPECT_EQ(readBufChar[i], static_cast<uint8_t>(strVal[i])); }

    // Set FIXED_LENGTH_ARRAY using rvalue std::string
    body.SetFieldValue(*charArrField, std::string("TESTING"));
    const auto readBufCharRvalue = body.GetFieldValue<TypedBuffer<uint8_t>>(*charArrField);
    const std::string expectedRvalue = "TESTING";
    for (size_t i = 0; i < expectedRvalue.size(); ++i) { EXPECT_EQ(readBufCharRvalue[i], static_cast<uint8_t>(expectedRvalue[i])); }

    // Set FIXED_LENGTH_ARRAY using std::vector
    const std::vector<uint32_t> vecVal = {100U, 200U, 300U, 400U};
    body.SetFieldValue(*uintArrField, vecVal);
    const auto readBufVec = body.GetFieldValue<TypedBuffer<uint32_t>>(*uintArrField);
    ASSERT_EQ(readBufVec.size(), 4U);
    for (size_t i = 0; i < vecVal.size(); ++i) { EXPECT_EQ(readBufVec[i], vecVal[i]); }

    // Set FIXED_LENGTH_ARRAY using TypedBuffer
    const std::vector<uint32_t> srcVec = {111U, 222U, 333U, 444U};
    TypedBuffer<uint32_t> typedBufVal(reinterpret_cast<const std::byte*>(srcVec.data()), srcVec.size());
    body.SetFieldValue(*uintArrField, typedBufVal);
    const auto readBufTyped = body.GetFieldValue<TypedBuffer<uint32_t>>(*uintArrField);
    ASSERT_EQ(readBufTyped.size(), 4U);
    for (size_t i = 0; i < srcVec.size(); ++i) { EXPECT_EQ(readBufTyped[i], srcVec[i]); }

    // Verify individual element reads
    EXPECT_EQ(body.GetFieldValue<uint32_t>(*uintArrField, 0), 111U);
    EXPECT_EQ(body.GetFieldValue<uint32_t>(*uintArrField, 1), 222U);
    EXPECT_EQ(body.GetFieldValue<uint32_t>(*uintArrField, 2), 333U);
    EXPECT_EQ(body.GetFieldValue<uint32_t>(*uintArrField, 3), 444U);
}
