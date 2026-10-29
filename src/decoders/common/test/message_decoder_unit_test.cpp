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
        DecoderTester(MessageDatabase::Ptr pclMessageDb_, std::string family_ = "OEM")
            : MessageDecoderBase(std::move(family_), std::move(pclMessageDb_))
        {
        }

        using MessageDecoderBase::DecodeAsciiField;

        STATUS TestDecodeAscii(const FieldInfo& FieldInfo_, std::string_view& input_, CompositeField& vIntermediateFormat_, bool abbreviated_ = false)
        {
            return abbreviated_ ? DecodeAscii<true>(FieldInfo_, input_, vIntermediateFormat_)
                                : DecodeAscii<false>(FieldInfo_, input_, vIntermediateFormat_);
        }

        STATUS TestDecodeBinary(const FieldInfo& FieldInfo_, const unsigned char** ppucLogBuf_, CompositeField& vIntermediateFormat_,
                                uint32_t length_ = 0)
        {
            return DecodeBinary(FieldInfo_, ppucLogBuf_, vIntermediateFormat_, length_);
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
                DecodeAsciiField(*stMessageDataType, vstrTestInput[sz], vIntermediateFormat_);

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
            ASSERT_THROW(MessageDecoderBase::DecodeAsciiField(*stMessageDataType, strTestInput, vIntermediateFormat), std::runtime_error);
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

    std::string_view testInput = "4";
    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, testInput, vIntermediateFormat_));
    ASSERT_EQ(std::get<int8_t>(vIntermediateFormat_.GetFieldValueVariant(*MsgDefFields[0])), static_cast<int8_t>('4'));
}

TEST_F(MessageDecoderTypesTest, ASCII_BOOL_VALID_INPUT)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("B_True", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::BOOL));
    CompositeField vIntermediateFormat_(4, 0);

    std::string_view testInput = "TRUE";
    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, testInput, vIntermediateFormat_));
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

    std::string_view testInput = "UNKNOWN,APPROXIMATE,SATTIME";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(fieldInfo, testInput, vIntermediateFormat), STATUS::SUCCESS);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*e0)), 20U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*e1)), 60U);
    ASSERT_EQ(std::get<int32_t>(vIntermediateFormat.GetFieldValueVariant(*e2)), 200U);
}

TEST_F(MessageDecoderTypesTest, ASCII_STRING_VALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("MESSAGE", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN));
    CompositeField vIntermediateFormat(0, 1);

    std::vector<std::string_view> testInputs = {"SOL_COMPUTED,WAAS"};
    std::vector<const char*> testTargets = {"SOL_COMPUTED","WAAS"};

    for (size_t sz = 0; sz < testInputs.size(); ++sz)
    {
        FieldInfo fieldInfo;
        std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
        fieldInfo.messageOrderedFields = constFields;
        fieldInfo.varFieldCount = 1;
        ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(fieldInfo, testInputs[sz], vIntermediateFormat), STATUS::SUCCESS);
        ASSERT_EQ(std::get<std::string>(vIntermediateFormat.GetVarFields()[0]), testTargets[sz]);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_TYPE_INVALID)
{
    MsgDefFields.emplace_back(std::make_shared<BaseField>("", FIELD_TYPE::UNKNOWN, "%d", DATA_TYPE::UNKNOWN));
    CompositeField vIntermediateFormat_;

    std::string_view testInput = "garbage";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_THROW(pclMyDecoderTester->TestDecodeAscii(fieldInfo, testInput, vIntermediateFormat_), std::runtime_error);
}

TEST_F(MessageDecoderTypesTest, ASCII_VALIDATE_NUMERIC_TOKENS)
{
    BaseField field("number", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    CompositeField decoded(sizeof(uint32_t), 0);
    const char input[] = {' ', '4', '2', '9'};
    pclMyDecoderTester->DecodeAsciiField(field, std::string_view(input, 3), decoded);
    EXPECT_EQ(decoded.GetFieldValue<uint32_t>(field), 42U);
    for (const std::string_view invalid : {"", "   ", "12junk", "4294967296"})
    {
        EXPECT_THROW(pclMyDecoderTester->DecodeAsciiField(field, invalid, decoded), std::runtime_error);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_QUOTED_DELIMITERS_AND_EMPTY_STRINGS)
{
    auto text = std::make_shared<BaseField>("text", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);
    auto number = std::make_shared<BaseField>("number", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto info = BuildFieldInfo({text, number});
    for (const std::string_view encoded : {"\"A*,B\",42*crc", "\"\",42*crc", ",42*crc"})
    {
        CompositeField decoded(info);
        auto input = encoded;
        ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::SUCCESS);
        const auto expected = encoded.front() == '"' ? encoded.substr(1, encoded.find('"', 1) - 1) : encoded.substr(0, encoded.find(','));
        EXPECT_EQ(decoded.GetFieldValue<std::string>(*text), expected);
        EXPECT_EQ(decoded.GetFieldValue<uint32_t>(*number), 42U);
    }
    for (const std::string_view encoded : {"\"unterminated,42", "\"text\"junk,42"})
    {
        CompositeField decoded(info);
        auto input = encoded;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::MALFORMED_INPUT);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_VALIDATE_HEX_ARRAY)
{
    auto bytes = std::make_shared<ArrayField>("bytes", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%Z", DATA_TYPE::UCHAR, 3);
    auto number = std::make_shared<BaseField>("number", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto info = BuildFieldInfo({bytes, number});
    CompositeField decoded(info);
    std::string_view input = "0041ff,42*crc";
    ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::SUCCESS);
    const auto values = decoded.GetFieldValue<TypedBuffer<uint8_t>>(*bytes);
    EXPECT_EQ(values[0], 0);
    EXPECT_EQ(values[1], 65);
    EXPECT_EQ(values[2], 255);
    EXPECT_EQ(decoded.GetFieldValue<uint32_t>(*number), 42U);
    for (const std::string_view encoded : {"00", "00410G,42", "0041ffjunk,42"})
    {
        input = encoded;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::MALFORMED_INPUT);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_QUOTED_ARRAY_ESCAPES_AND_PADDING)
{
    auto text = std::make_shared<ArrayField>("text", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%s", DATA_TYPE::CHAR, 4);
    const auto info = BuildFieldInfo({text});
    CompositeField decoded(info);
    for (const std::string_view encoded : {"\"A\\x42\\\\\"*crc", "\"\"*crc", "\"ABCD\"*crc"})
    {
        auto input = encoded;
        ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::SUCCESS);
        const auto values = decoded.GetFieldValue<TypedBuffer<int8_t>>(*text);
        EXPECT_EQ(values[0], encoded[1] == '"' ? 0 : 'A');
        EXPECT_EQ(values[3], encoded == "\"ABCD\"*crc" ? 'D' : 0);
        if (encoded[2] == '\\')
        {
            EXPECT_EQ(values[1], 'B');
            EXPECT_EQ(values[2], '\\');
        }
    }
    for (const std::string_view encoded : {"\"A", "\"ABCDE\"", "\"\\x0\"", "\"\\x0G\"", "\"\\xGG\"", "\"AB\"junk"})
    {
        auto input = encoded;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::MALFORMED_INPUT);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_EMPTY_VARIABLE_ARRAY_PRESERVES_NEXT_FIELD)
{
    for (const std::string_view conversion : {"%u", "%Z", "%P"})
    {
        auto bytes = std::make_shared<ArrayField>("bytes", FIELD_TYPE::VARIABLE_LENGTH_ARRAY, std::string(conversion), DATA_TYPE::UCHAR, 8);
        auto number = std::make_shared<BaseField>("number", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
        const auto info = BuildFieldInfo({bytes, number});
        CompositeField decoded(info);
        std::string_view input = "0,42*crc";
        ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::SUCCESS);
        EXPECT_TRUE(decoded.GetFieldValue<std::vector<uint8_t>>(*bytes).empty());
        EXPECT_EQ(decoded.GetFieldValue<uint32_t>(*number), 42U);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_MISSING_FIELDS)
{
    auto first = std::make_shared<BaseField>("first", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto second = std::make_shared<BaseField>("second", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto info = BuildFieldInfo({first, second});
    for (const std::string_view encoded : {"1*crc", "1", "1,*crc", "1,", "1,,2*crc"})
    {
        CompositeField decoded(info);
        auto input = encoded;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::MALFORMED_INPUT);
    }
}

TEST_F(MessageDecoderTypesTest, ASCII_ARRAY_DELIMITERS)
{
    auto bytes = std::make_shared<ArrayField>("bytes", FIELD_TYPE::VARIABLE_LENGTH_ARRAY, "%u", DATA_TYPE::UCHAR, 2);
    const auto info = BuildFieldInfo({bytes});
    for (const std::string_view encoded : {"2,65*25crc", "2,65"})
    {
        CompositeField decoded(info);
        auto input = encoded;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded), STATUS::MALFORMED_INPUT);
    }
}

TEST_F(MessageDecoderTypesTest, ABBREV_ASCII_NUMERIC_FIELD_BOUNDS)
{
    auto number = std::make_shared<BaseField>("number", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    const auto info = BuildFieldInfo({number});
    CompositeField decoded(info);
    std::string_view input = "< \r\n";
    EXPECT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded, true), STATUS::MALFORMED_INPUT);
    const char bounded[] = {'<', ' ', '4', '2', '9'};
    input = std::string_view(bounded, 4);
    ASSERT_EQ(pclMyDecoderTester->TestDecodeAscii(*info, input, decoded, true), STATUS::SUCCESS);
    EXPECT_EQ(decoded.GetFieldValue<uint32_t>(*number), 42U);
    EXPECT_TRUE(input.empty());
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

TEST_F(MessageDecoderTypesTest, BINARY_ARRAY_LENGTH_BOUNDS)
{
    auto prefix = std::make_shared<BaseField>("prefix", FIELD_TYPE::SIMPLE, "%uc", DATA_TYPE::UCHAR);
    auto bytes = std::make_shared<ArrayField>("bytes", FIELD_TYPE::VARIABLE_LENGTH_ARRAY, "%uc", DATA_TYPE::UCHAR, 4);
    bytes->arrayLengthFieldSize = 4;
    const auto info = BuildFieldInfo({prefix, bytes});
    MessageDatabase::RegisterAlignmentFunction("ARRAY_BOUNDS_TEST", [](size_t size, uintptr_t start, uintptr_t current) {
        return (size - (current - start) % size) % size;
    });
    DecoderTester decoder(nullptr, "ARRAY_BOUNDS_TEST");
    const unsigned char data[8] = {};

    for (uint32_t length = 2; length < sizeof(data); ++length)
    {
        CompositeField decoded(info);
        const auto* input = data;
        EXPECT_THROW(decoder.TestDecodeBinary(*info, &input, decoded, length), std::runtime_error);
        EXPECT_EQ(input, data + 1);
    }

    CompositeField decoded(info);
    const auto* input = data;
    EXPECT_EQ(decoder.TestDecodeBinary(*info, &input, decoded, sizeof(data)), STATUS::SUCCESS);
    EXPECT_EQ(input, data + sizeof(data));

    bytes->arrayLengthFieldSize = 0;
    input = data;
    EXPECT_THROW(decoder.TestDecodeBinary(*info, &input, decoded, sizeof(data)), std::runtime_error);
    EXPECT_EQ(input, data + 1);
}

TEST_F(MessageDecoderTypesTest, BINARY_REFERENCED_ARRAY_LENGTH)
{
    auto count = std::make_shared<BaseField>("count", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto bytes = std::make_shared<ArrayField>("bytes", FIELD_TYPE::VARIABLE_LENGTH_ARRAY, "%uc", DATA_TYPE::UCHAR, 4);
    bytes->arrayLengthRef = "count";
    const auto info = BuildFieldInfo({count, bytes});
    CompositeField decoded(info);
    decoded.SetFieldValue<true>(count->index, uint32_t{0});
    FieldInfo arrayOnly;
    arrayOnly.messageOrderedFields = {bytes};
    const unsigned char data[] = {0};
    const auto* input = data;

    EXPECT_EQ(pclMyDecoderTester->TestDecodeBinary(arrayOnly, &input, decoded, 0), STATUS::SUCCESS);
    EXPECT_EQ(input, data);
}

TEST_F(MessageDecoderTypesTest, BINARY_ARRAY_PAYLOAD_BOUNDS)
{
    for (const auto type : {FIELD_TYPE::FIXED_LENGTH_ARRAY, FIELD_TYPE::VARIABLE_LENGTH_ARRAY})
    {
        auto values = std::make_shared<ArrayField>("values", type, "%hu", DATA_TYPE::USHORT, 2);
        values->arrayLengthFieldSize = 1;
        const auto info = BuildFieldInfo({values});
        const unsigned char data[] = {2, 1, 0, 2, 0};
        const auto* start = data + (type == FIELD_TYPE::FIXED_LENGTH_ARRAY ? 1 : 0);
        const uint32_t fullLength = type == FIELD_TYPE::FIXED_LENGTH_ARRAY ? 4 : 5;
        for (uint32_t length = type == FIELD_TYPE::FIXED_LENGTH_ARRAY ? 0 : 1; length < fullLength; ++length)
        {
            CompositeField decoded(info);
            const auto* input = start;
            EXPECT_THROW(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, length), std::runtime_error);
        }
        CompositeField decoded(info);
        const auto* input = start;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, fullLength), STATUS::SUCCESS);
        EXPECT_EQ(input, start + fullLength);
    }
}

TEST_F(MessageDecoderTypesTest, BINARY_STRING_BOUNDS)
{
    auto text = std::make_shared<BaseField>("text", FIELD_TYPE::STRING, "%s", DATA_TYPE::CHAR);
    const auto info = BuildFieldInfo({text});
    const unsigned char data[] = {'h', 'i', 0};
    for (uint32_t length = 0; length < sizeof(data); ++length)
    {
        CompositeField decoded(info);
        const auto* input = data;
        EXPECT_THROW(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, length), std::runtime_error);
        EXPECT_EQ(input, data);
    }
    CompositeField decoded(info);
    const auto* input = data;
    EXPECT_EQ(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, sizeof(data)), STATUS::SUCCESS);
    EXPECT_EQ(decoded.GetFieldValue<std::string>(*text), "hi");
}

TEST_F(MessageDecoderTypesTest, BINARY_RESPONSE_BOUNDS)
{
    auto id = std::make_shared<BaseField>("id", FIELD_TYPE::RESPONSE_ID, "%d", DATA_TYPE::INT);
    auto text = std::make_shared<BaseField>("text", FIELD_TYPE::RESPONSE_STR, "%s", DATA_TYPE::CHAR);
    const auto info = BuildFieldInfo({id, text});
    const unsigned char data[] = {1, 0, 0, 0, 'O', 'K', 0xAA};
    for (uint32_t length = 0; length < sizeof(int32_t); ++length)
    {
        CompositeField decoded(info);
        const auto* input = data;
        EXPECT_THROW(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, length), std::runtime_error);
    }
    for (uint32_t length : {4U, 6U})
    {
        CompositeField decoded(info);
        const auto* input = data;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, length), STATUS::SUCCESS);
        EXPECT_EQ(decoded.GetFieldValue<std::string>(*text), length == 4 ? "" : "OK");
        EXPECT_EQ(input, data + length);
    }
}

TEST_F(MessageDecoderTypesTest, BINARY_FIELD_ARRAY_BOUNDS)
{
    for (const bool nested : {false, true})
    {
        auto member = nested ? std::make_shared<BaseField>("text", FIELD_TYPE::STRING, "%s", DATA_TYPE::CHAR)
                             : std::make_shared<BaseField>("value", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
        auto array = std::make_shared<FieldArrayField>("records", FIELD_TYPE::FIELD_ARRAY, "%u", DATA_TYPE::UINT, 1, BuildFieldInfo({member}));
        array->arrayLengthFieldSize = 1;
        const auto info = BuildFieldInfo({array});
        const unsigned char data[] = {1, 'x', 0, 0, 0};
        const uint32_t fullLength = nested ? 3 : 5;
        for (uint32_t length = 1; length < fullLength; ++length)
        {
            CompositeField decoded(info);
            const auto* input = data;
            EXPECT_THROW(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, length), std::runtime_error);
        }
        CompositeField decoded(info);
        const auto* input = data;
        EXPECT_EQ(pclMyDecoderTester->TestDecodeBinary(*info, &input, decoded, fullLength), STATUS::SUCCESS);
        EXPECT_EQ(input, data + fullLength);
    }
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

    std::string_view testInput = "TRUE,63,227,56,2734,-3842,38283,54244,-4359,5293,79338432,-289834,2.54,5.44061788e+03";

    FieldInfo fieldInfo;
    std::vector<BaseField::ConstPtr> constFields(MsgDefFields.begin(), MsgDefFields.end());
    fieldInfo.messageOrderedFields = constFields;
    ASSERT_EQ(STATUS::SUCCESS, pclMyDecoderTester->TestDecodeAscii(fieldInfo, testInput, vIntermediateFormat));
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
    FlatFieldArray fieldArray(2, fieldInfo.get());

    fieldArray.SetFieldValue<uint32_t>(0, *f0, 11U);
    fieldArray.SetFieldValue<int16_t>(0, *f1, static_cast<int16_t>(-7));
    fieldArray.SetFieldValue<uint32_t>(1, *f0, 22U);
    fieldArray.SetFieldValue<int16_t>(1, *f1, static_cast<int16_t>(9));

    auto it = fieldArray.begin();
    const FieldArrayRecordView row0 = *it;
    EXPECT_EQ(row0.GetFieldValue<uint32_t>(*f0), 11U);
    EXPECT_EQ(row0.GetFieldValue<int16_t>(*f1), static_cast<int16_t>(-7));

    ++it;
    const FieldArrayRecordView row1 = *it;
    EXPECT_EQ(row1.GetFieldValue<uint32_t>(*f0), 22U);
    EXPECT_EQ(row1.GetFieldValue<int16_t>(*f1), static_cast<int16_t>(9));
}

TEST(MessageDecoderContainerTypesTest, FlatFieldArrayIteratorFind)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("i16", FIELD_TYPE::SIMPLE, "%hd", DATA_TYPE::SHORT);

    const auto fieldInfo = BuildFieldInfo({f0, f1});
    FlatFieldArray fieldArray(3, fieldInfo.get());

    fieldArray.SetFieldValue<uint32_t>(0, *f0, 11U);
    fieldArray.SetFieldValue<int16_t>(0, *f1, static_cast<int16_t>(-7));
    fieldArray.SetFieldValue<uint32_t>(1, *f0, 22U);
    fieldArray.SetFieldValue<int16_t>(1, *f1, static_cast<int16_t>(9));
    fieldArray.SetFieldValue<uint32_t>(2, *f0, 33U);
    fieldArray.SetFieldValue<int16_t>(2, *f1, static_cast<int16_t>(15));

    const auto it = std::find_if(fieldArray.begin(), fieldArray.end(), [&](const FieldArrayRecordView& row) {
        return row.GetFieldValue<uint32_t>(*f0) == 22U;
    });
    ASSERT_NE(it, fieldArray.end());
    EXPECT_EQ(std::distance(fieldArray.begin(), it), 1);
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*f0), 22U);
    EXPECT_EQ((*it).GetFieldValue<int16_t>(*f1), static_cast<int16_t>(9));

    const auto missing = std::find_if(fieldArray.begin(), fieldArray.end(), [&](const FieldArrayRecordView& row) {
        return row.GetFieldValue<uint32_t>(*f0) == 99U;
    });
    EXPECT_EQ(missing, fieldArray.end());

    EXPECT_EQ(std::count_if(fieldArray.begin(), fieldArray.end(), [&](const FieldArrayRecordView& row) {
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

    FlatFieldArray stored(2, nestedFieldInfo.get());
    stored.SetFieldValue<uint32_t>(0, *nestedU32, 11U);
    stored.SetFieldValue<int16_t>(0, *nestedI16, static_cast<int16_t>(-7));
    stored.SetFieldValue<uint32_t>(1, *nestedU32, 22U);
    stored.SetFieldValue<int16_t>(1, *nestedI16, static_cast<int16_t>(9));

    CompositeField body(rootFieldInfo);
    body.SetFieldValue(*fieldArrayField, stored);

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    ASSERT_EQ(wrapped.size(), 2U);
    EXPECT_EQ(wrapped.GetFieldInfo(), nestedFieldInfo.get());
    EXPECT_EQ(wrapped.GetFieldValue<uint32_t>(*nestedU32, 0), 11U);
    EXPECT_EQ(wrapped.GetFieldValue<int16_t>(*nestedI16, 1), static_cast<int16_t>(9));

    auto it = wrapped.begin();
    const FieldArrayRecordView row0 = *it;
    EXPECT_EQ(row0.GetFieldValue<uint32_t>(*nestedU32), 11U);
    EXPECT_EQ(row0.GetFieldValue<int16_t>(*nestedI16), static_cast<int16_t>(-7));

    ++it;
    const FieldArrayRecordView row1 = *it;
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

    FlatFieldArray stored(2, nestedFieldInfo.get());
    stored.SetFieldValue<uint32_t>(0, *nestedU32, 11U);
    stored.SetFieldValue<int16_t>(0, *nestedI16, static_cast<int16_t>(-7));
    stored.SetFieldValue<uint32_t>(1, *nestedU32, 22U);
    stored.SetFieldValue<int16_t>(1, *nestedI16, static_cast<int16_t>(9));

    CompositeField body(rootFieldInfo);
    body.SetFieldValue(*fieldArrayField, stored);

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    const auto it = std::find_if(wrapped.begin(), wrapped.end(), [&](const FieldArrayRecordView& row) {
        return row.GetFieldValue<uint32_t>(*nestedU32) == 22U;
    });
    ASSERT_NE(it, wrapped.end());
    EXPECT_EQ(std::distance(wrapped.begin(), it), 1);
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*nestedU32), 22U);
    EXPECT_EQ((*it).GetFieldValue<int16_t>(*nestedI16), static_cast<int16_t>(9));
    EXPECT_EQ(std::count_if(wrapped.begin(), wrapped.end(), [&](const FieldArrayRecordView& row) {
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
    body.SetFieldValue(*fieldArrayField, CompositeFieldArray{row0, row1});

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    ASSERT_EQ(wrapped.size(), 2U);
    EXPECT_EQ(wrapped.GetFieldInfo(), nestedFieldInfo.get());
    EXPECT_EQ(wrapped.GetFieldValue<uint32_t>(*nestedU32, 0), 10U);
    EXPECT_EQ(wrapped.GetFieldValue<std::string>(*nestedStr, 1), "twenty");

    const FieldArrayRecordView firstRow = wrapped[0];
    EXPECT_EQ(firstRow.GetFieldValue<uint32_t>(*nestedU32), 10U);
    EXPECT_EQ(firstRow.GetFieldValue<std::string>(*nestedStr), "ten");

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
    body.SetFieldValue(*fieldArrayField, CompositeFieldArray{row0, row1});

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    const auto it = std::find_if(wrapped.begin(), wrapped.end(), [&](const FieldArrayRecordView& row) {
        return row.GetFieldValue<uint32_t>(*nestedU32) == 20U;
    });
    ASSERT_NE(it, wrapped.end());
    EXPECT_EQ(std::distance(wrapped.begin(), it), 1);
    EXPECT_EQ((*it).GetFieldValue<uint32_t>(*nestedU32), 20U);
    EXPECT_EQ((*it).GetFieldValue<std::string>(*nestedStr), "twenty");
    EXPECT_EQ(std::count_if(wrapped.begin(), wrapped.end(), [&](const FieldArrayRecordView& row) {
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
    body.SetFieldValue(*fieldArrayField, CompositeFieldArray{});

    const auto wrapped = body.GetFieldValue<FieldArray>(*fieldArrayField);
    EXPECT_TRUE(wrapped.empty());
    EXPECT_EQ(wrapped.GetFieldInfo(), nestedFieldInfo.get());
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

TEST(MessageDecoderContainerTypesTest, MessageBodyIteratorFind)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<BaseField>("str", FIELD_TYPE::STRING, "%s", DATA_TYPE::UNKNOWN);
    auto f2 = std::make_shared<BaseField>("i16", FIELD_TYPE::SIMPLE, "%hd", DATA_TYPE::SHORT);

    const auto fieldInfo = BuildFieldInfo({f0, f1, f2});
    CompositeField body(fieldInfo);
    body.SetFieldValue(*f0, 1234U);
    body.SetFieldValue(*f1, std::string("ok"));
    body.SetFieldValue(*f2, static_cast<int16_t>(-7));

    using FieldValuePair = std::pair<BaseField::ConstPtr, FieldValueVariant>;

    const auto byName = std::find_if(body.begin(), body.end(), [](const FieldValuePair& fieldValue) { return fieldValue.first->name == "str"; });
    ASSERT_NE(byName, body.end());
    EXPECT_EQ(std::distance(body.begin(), byName), 1);
    EXPECT_EQ(std::get<std::string>((*byName).second), "ok");

    const auto byValue =
        std::find_if(body.begin(), body.end(), [](const FieldValuePair& fieldValue) { return std::holds_alternative<int16_t>(fieldValue.second); });
    ASSERT_NE(byValue, body.end());
    EXPECT_EQ((*byValue).first->name, "i16");
    EXPECT_EQ(std::get<int16_t>((*byValue).second), static_cast<int16_t>(-7));

    const auto missing = std::find_if(body.begin(), body.end(), [](const FieldValuePair& fieldValue) { return fieldValue.first->name == "none"; });
    EXPECT_EQ(missing, body.end());

    EXPECT_EQ(std::distance(body.begin(), body.end()), 3);
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
    meta.eFormat = DECODE_FORMAT::ASCII;
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

TEST(MessageDecoderContainerTypesTest, FlatFieldArrayRecordViewGetFieldValueByName)
{
    auto f0 = std::make_shared<BaseField>("u32", FIELD_TYPE::SIMPLE, "%u", DATA_TYPE::UINT);
    auto f1 = std::make_shared<ArrayField>("arr", FIELD_TYPE::FIXED_LENGTH_ARRAY, "%s", DATA_TYPE::UCHAR, 7);
    const auto fieldInfo = BuildFieldInfo({f0, f1});

    FlatFieldArray ffa(2, fieldInfo.get());
    ffa.SetFieldValue(0, *f0, 10U);
    ffa.SetFieldValue(0, *f1, std::string("ten"));
    ffa.SetFieldValue(1, *f0, 20U);
    ffa.SetFieldValue(1, *f1, std::string("twenty"));

    auto tenStr = std::string("ten");
    auto twentyStr = std::string("twenty");
    EXPECT_EQ(ffa[0].GetFieldValueByName<uint32_t>("u32"), 10);
    const auto arr1 = ffa[0].GetFieldValueByName<TypedBuffer<uint8_t>>("arr");
    for (size_t i = 0; i < tenStr.size(); i++) { EXPECT_EQ(arr1[i], static_cast<uint8_t>(tenStr[i])); }
    EXPECT_EQ(ffa[1].GetFieldValueByName<uint32_t>("u32"), 20);
    const auto arr2 = ffa[1].GetFieldValueByName<TypedBuffer<uint8_t>>("arr");
    for (size_t i = 0; i < twentyStr.size(); i++) { EXPECT_EQ(arr2[i], static_cast<uint8_t>(twentyStr[i])); }
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
