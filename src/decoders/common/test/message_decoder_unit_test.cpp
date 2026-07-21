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

TEST(MessageDecoderContainerTypesTest, FlatFieldArrayIteratorBuildsExpectedMessageBodyPerField)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("i16", FIELD_TYPE::SIMPLE, "%hd", DATA_TYPE::SHORT);

    const auto fieldInfo = BuildFieldInfo({f0, f1});
    FlatFieldArray fieldArray(2, fieldInfo);

    fieldArray.SetFieldValue<uint32_t>(0, *f0, 11U);
    fieldArray.SetFieldValue<int16_t>(0, *f1, static_cast<int16_t>(-7));
    fieldArray.SetFieldValue<uint32_t>(1, *f0, 22U);
    fieldArray.SetFieldValue<int16_t>(1, *f1, static_cast<int16_t>(9));

    auto it = fieldArray.begin();
    const FixedRecordView row0 = *it;
    EXPECT_EQ(row0.GetFieldValue<uint32_t>(*f0), 11U);
    EXPECT_EQ(row0.GetFieldValue<int16_t>(*f1), static_cast<int16_t>(-7));

    ++it;
    const FixedRecordView row1 = *it;
    EXPECT_EQ(row1.GetFieldValue<uint32_t>(*f0), 22U);
    EXPECT_EQ(row1.GetFieldValue<int16_t>(*f1), static_cast<int16_t>(9));
}

TEST(MessageDecoderContainerTypesTest, FlatFieldArrayIteratorEndDereferenceThrows)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto fieldInfo = BuildFieldInfo({f0});
    FlatFieldArray fieldArray(1, fieldInfo);

    auto it = fieldArray.end();
    EXPECT_THROW(*it, std::runtime_error);
}

TEST(MessageDecoderContainerTypesTest, MessageBodyIteratorTraversesFieldValuesInDefinitionOrder)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("str", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);

    auto f0Copy = std::make_shared<BaseField>(*f0);
    auto f1Copy = std::make_shared<BaseField>(*f1);
    auto faFieldInfo = BuildFieldInfo({f0Copy, f1Copy});
    auto f2 = std::make_shared<FieldArrayField>("fa", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 2, faFieldInfo);

    const auto fieldInfo = BuildFieldInfo({f0, f1, f2});
    CompositeField body(fieldInfo);
    body.SetFieldValue(*f0, 1234U);
    body.SetFieldValue(*f1, std::string("ok"));
    CompositeField nestedBody(faFieldInfo);
    nestedBody.SetFieldValue(*f0Copy, 5678U);
    nestedBody.SetFieldValue(*f1Copy, std::string("nested"));
    body.SetFieldValue(*f2, CompositeFieldArray{nestedBody});

    auto it = body.begin();
    auto fieldValue = *it;
    EXPECT_EQ(fieldValue.first, f0);
    EXPECT_EQ(std::get<uint32_t>(fieldValue.second), 1234U);
    ++it;
    fieldValue = *it;
    EXPECT_EQ(fieldValue.first, f1);
    EXPECT_EQ(std::get<std::string>(fieldValue.second), "ok");
    ++it;
    fieldValue = *it;
    EXPECT_EQ(fieldValue.first, f2);
    const auto nestedArray = std::get<CompositeFieldArray>(fieldValue.second);
    EXPECT_EQ(nestedArray.size(), 1U);
    const auto& nestedBodyValue = nestedArray[0];
    EXPECT_EQ(nestedBodyValue.GetFieldValue<uint32_t>(*f0Copy), 5678U);
    EXPECT_EQ(nestedBodyValue.GetFieldValue<std::string>(*f1Copy), "nested");
    ++it;
    EXPECT_EQ(it, body.end());
}

TEST(MessageDecoderContainerTypesTest, DecodeFromMessageDatabaseAndIterateMessageAndFieldArray)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("str", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);

    auto f0Nested = std::make_shared<BaseField>(*f0);
    auto f1Nested = std::make_shared<BaseField>(*f1);
    const auto faFieldInfo = BuildFieldInfo({f0Nested, f1Nested});
    auto f2 = std::make_shared<FieldArrayField>("fa", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 2, faFieldInfo);

    const auto fieldInfo = BuildFieldInfo({f0, f1, f2});
    constexpr uint32_t kMsgCrc = 0x11223344U;
    auto msgDef = std::make_shared<MessageDefinition>();
    msgDef->logID = 777U;
    msgDef->name = "TESTMSG";
    msgDef->fieldInfo.emplace(kMsgCrc, fieldInfo);
    msgDef->latestMessageCrc = kMsgCrc;

    auto db = std::make_shared<MessageDatabase>(std::vector<MessageDefinition::ConstPtr>{msgDef}, std::vector<EnumDefinition::ConstPtr>{});
    MessageDecoderBase decoder("OEM", db);

    const std::string payload = "1234,ok,1,5678,nested";
    MetaDataBase meta;
    meta.eFormat = HEADER_FORMAT::ASCII;
    meta.usMessageId = static_cast<uint16_t>(msgDef->logID);
    meta.uiMessageCrc = kMsgCrc;
    meta.uiHeaderLength = 0;
    meta.uiLength = static_cast<uint32_t>(payload.size());
    meta.messageName = msgDef->name;

    CompositeField decoded;
    const STATUS status = decoder.Decode(reinterpret_cast<const unsigned char*>(payload.data()), decoded, meta);
    ASSERT_EQ(status, STATUS::SUCCESS);

    auto it = decoded.begin();
    auto fieldValue = *it;
    EXPECT_EQ(fieldValue.first, f0);
    EXPECT_EQ(std::get<uint32_t>(fieldValue.second), 1234U);
    ++it;
    fieldValue = *it;
    EXPECT_EQ(fieldValue.first, f1);
    EXPECT_EQ(std::get<std::string>(fieldValue.second), "ok");
    ++it;

    fieldValue = *it;
    EXPECT_EQ(fieldValue.first, f2);
    const auto nestedArray = std::get<CompositeFieldArray>(fieldValue.second);
    ASSERT_EQ(nestedArray.size(), 1U);

    const CompositeField& nestedBody = nestedArray.front();
    auto nestedIt = nestedBody.begin();
    auto nestedFieldValue = *nestedIt;
    EXPECT_EQ(nestedFieldValue.first->name, f0Nested->name);
    EXPECT_EQ(std::get<uint32_t>(nestedFieldValue.second), 5678U);
    ++nestedIt;
    nestedFieldValue = *nestedIt;
    EXPECT_EQ(nestedFieldValue.first->name, f1Nested->name);
    EXPECT_EQ(std::get<std::string>(nestedFieldValue.second), "nested");
    ++nestedIt;
    EXPECT_EQ(nestedIt, nestedBody.end());

    ++it;
    EXPECT_EQ(it, decoded.end());
}

TEST(MessageDecoderContainerTypesTest, MessageBodyIteratorRequiresFieldInfo)
{
    CompositeField body;
    EXPECT_THROW(body.begin(), std::runtime_error);
    EXPECT_THROW(body.end(), std::runtime_error);
}
