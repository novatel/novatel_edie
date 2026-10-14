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
// ! \file json_reader_unit_test.cpp
// ===============================================================================

#include <filesystem>

#include <gtest/gtest.h>

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/json_db_reader.hpp"

using namespace novatel::edie;

class JsonDbReaderTest : public testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override {}
};

// -------------------------------------------------------------------------------------------------------
// JsonDbReader Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(JsonDbReaderTest, JsonDbReaderFailure)
{
    MessageDatabase clJson;
    ASSERT_THROW(LoadJsonDbFile(""), JsonDbReaderFailure);
}

TEST_F(JsonDbReaderTest, AppendEnumerations)
{
    const std::string strId = "2630f62f9ca0a9e4bcccef50ffbaba8f920c4439";

    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendEnumerations(LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")))->EnumDefinitions());

    EnumDefinition::ConstPtr pstEnumDef = clJson->GetEnumDefId(strId);
    ASSERT_NE(pstEnumDef, nullptr);
    ASSERT_EQ(pstEnumDef->name, "Adjust1PPSMode");

    clJson->RemoveEnumeration("Adjust1PPSMode");
    ASSERT_EQ(clJson->GetEnumDefId(strId), nullptr);
}

TEST_F(JsonDbReaderTest, AppendBitMasks)
{
    const std::string strId = "7977f11493b08295e9a5692b992646b1b1438b7d";
    const std::string strName = "ExtendedSolutionStatus";

    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendBitMasks(LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")))->BitMasks());

    BitMaskMap::ConstPtr pstBitMaskDef = clJson->GetBitMaskDefId(strId);
    ASSERT_NE(pstBitMaskDef, nullptr);
    ASSERT_EQ(pstBitMaskDef->name, strName);
    ASSERT_EQ(clJson->GetBitMaskDefName(strName), pstBitMaskDef);
    ASSERT_EQ(pstBitMaskDef->masks.at("pseudorange_inno_correction").bitfield, BitMask::fromRange(1, 4));

    clJson->RemoveBitMask(strName);
    ASSERT_EQ(clJson->GetBitMaskDefId(strId), nullptr);
    ASSERT_EQ(clJson->GetBitMaskDefName(strName), nullptr);
}

TEST_F(JsonDbReaderTest, LoadResolvesBitMaskEnums)
{
    const auto clJson = LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")));
    const BitMaskMap::ConstPtr pstBitMaskDef = clJson->GetBitMaskDefName("ExtendedSolutionStatus");
    ASSERT_NE(pstBitMaskDef, nullptr);

    const BitMaskMapEntry& enumEntry = pstBitMaskDef->masks.at("pseudorange_inno_correction");
    ASSERT_FALSE(enumEntry.enumId.empty());
    ASSERT_NE(enumEntry.enumDef, nullptr);
    ASSERT_EQ(enumEntry.enumDef, clJson->GetEnumDefId(enumEntry.enumId));

    const BitMaskMapEntry& plainEntry = pstBitMaskDef->masks.at("rtk_solution_verified");
    ASSERT_TRUE(plainEntry.enumId.empty());
    ASSERT_EQ(plainEntry.enumDef, nullptr);
}

TEST_F(JsonDbReaderTest, MergeBitMasks)
{
    const std::string strName = "ExtendedSolutionStatus";

    MessageDatabase clJson;
    clJson.Merge(*LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH"))));

    const BitMaskMap::ConstPtr pstBitMaskDef = clJson.GetBitMaskDefName(strName);
    ASSERT_NE(pstBitMaskDef, nullptr);

    const BitMaskMapEntry& enumEntry = pstBitMaskDef->masks.at("pseudorange_inno_correction");
    ASSERT_EQ(enumEntry.enumDef, clJson.GetEnumDefId(enumEntry.enumId));
    ASSERT_NE(enumEntry.enumDef, nullptr);

    const auto pstMsgDef = clJson.GetMsgDef("BESTPOS");
    ASSERT_EQ(pstMsgDef->GetMsgDefFromCrc(pstMsgDef->latestMessageCrc).GetFieldDefByName("ext_sol_stat")->bitMasks, pstBitMaskDef);
}

TEST_F(JsonDbReaderTest, AppendMessages)
{
    constexpr uint32_t uiMsgId = 690;

    auto clJson = std::make_shared<MessageDatabase>();
    clJson->AppendMessages(LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")))->MessageDefinitions());

    MessageDefinition::ConstPtr pstMsgDef = clJson->GetMsgDef(uiMsgId);
    ASSERT_NE(pstMsgDef, nullptr);
    ASSERT_EQ(pstMsgDef->name, "PASSAUX");

    clJson->RemoveMessage(uiMsgId);
    ASSERT_EQ(clJson->GetMsgDef(uiMsgId), nullptr);
}

TEST_F(JsonDbReaderTest, FieldDefaultValues)
{
    MessageDatabase::Ptr clJson = LoadJsonDbFile(std::filesystem::path(std::getenv("TEST_DATABASE_PATH")));

    MessageDefinition::ConstPtr pstMsgDef = clJson->GetMsgDef("LOG");
    ASSERT_NE(pstMsgDef, nullptr);

    const FieldInfo& stFieldInfo = *pstMsgDef->fieldInfo.at(pstMsgDef->latestMessageCrc);
    ASSERT_EQ(stFieldInfo.GetFieldDefByName("trigger")->defaultValue, SimpleValueVariant(int32_t{4})); // ONCE
    ASSERT_EQ(stFieldInfo.GetFieldDefByName("offset")->defaultValue, SimpleValueVariant(0.0));
}

TEST_F(JsonDbReaderTest, FieldDefaultValueTypes)
{
    // A field of the given type with the given default, or no default if strDefault_ is empty
    auto Field = [](const std::string& strName_, const std::string& strType_, const std::string& strDataType_, const std::string& strDefault_,
                    const std::string& strExtra_ = R"("arrayLength": null)") {
        return R"({"name": ")" + strName_ + R"(", "type": ")" + strType_ + R"(", "description": "", "conversionString": null, )" + strExtra_ +
               R"(, "dataType": {"name": ")" + strDataType_ + R"(", "length": 4, "description": ""})" +
               (strDefault_.empty() ? "" : R"(, "defaultValue": )" + strDefault_) + "}";
    };

    const std::string strFields =
        Field("bool", "SIMPLE", "BOOL", "1") + "," + Field("char", "SIMPLE", "CHAR", "-2") + "," + Field("uchar", "SIMPLE", "UCHAR", "5") + "," +
        Field("short", "SIMPLE", "SHORT", "-300") + "," + Field("ushort", "SIMPLE", "USHORT", "300") + "," + Field("long", "SIMPLE", "LONG", "-3") +
        "," + Field("ulong", "SIMPLE", "ULONG", "4000000000") + "," + Field("longlong", "SIMPLE", "LONGLONG", "-5") + "," +
        Field("ulonglong", "SIMPLE", "ULONGLONG", "18446744073709551615") + "," + Field("float", "SIMPLE", "FLOAT", "0.5") + "," +
        Field("double", "SIMPLE", "DOUBLE", "1") + "," + Field("enum", "ENUM", "ENUM", "2", R"("enumID": "e1")") + "," +
        Field("array", "FIXED_LENGTH_ARRAY", "UCHAR", "7", R"("arrayLength": 3)") + "," + Field("null", "SIMPLE", "ULONG", "null") + "," +
        Field("missing", "SIMPLE", "ULONG", "") + "," + Field("negative_ulong", "SIMPLE", "ULONG", "-2147483648") + "," +
        Field("large_uchar", "SIMPLE", "UCHAR", "256") + "," + Field("float_long", "SIMPLE", "LONG", "1.5") + "," +
        Field("string", "STRING", "STRING", "0", R"("arrayLength": 8)");

    MessageDatabase::Ptr clJson =
        ParseJsonDb(R"({"meta": {"version": "2.0.0", "messageFamily": "OEM", "subset": "all"},)"
                    R"("enums": [{"name": "Things", "_id": "e1", "enumerators": [{"name": "TWO", "value": 2, "description": ""}]}],)"
                    R"("messages": [{"name": "DEFAULTS", "_id": "m1", "messageID": 1, "description": "", )"
                    R"("latestMsgDefCrc": "1", "fields": {"1": [)" +
                    strFields + "]}}]}");

    MessageDefinition::ConstPtr pstMsgDef = clJson->GetMsgDef("DEFAULTS");
    ASSERT_NE(pstMsgDef, nullptr);
    const FieldInfo& stFieldInfo = *pstMsgDef->fieldInfo.at(pstMsgDef->latestMessageCrc);
    auto Default = [&](const std::string& strName_) { return stFieldInfo.GetFieldDefByName(strName_)->defaultValue; };

    // Each default is loaded as the type the field decodes to
    ASSERT_EQ(Default("bool"), SimpleValueVariant(true));
    ASSERT_EQ(Default("char"), SimpleValueVariant(int8_t{-2}));
    ASSERT_EQ(Default("uchar"), SimpleValueVariant(uint8_t{5}));
    ASSERT_EQ(Default("short"), SimpleValueVariant(int16_t{-300}));
    ASSERT_EQ(Default("ushort"), SimpleValueVariant(uint16_t{300}));
    ASSERT_EQ(Default("long"), SimpleValueVariant(int32_t{-3}));
    ASSERT_EQ(Default("ulong"), SimpleValueVariant(uint32_t{4000000000}));
    ASSERT_EQ(Default("longlong"), SimpleValueVariant(int64_t{-5}));
    ASSERT_EQ(Default("ulonglong"), SimpleValueVariant(uint64_t{18446744073709551615ULL}));
    ASSERT_EQ(Default("float"), SimpleValueVariant(0.5F));
    ASSERT_EQ(Default("double"), SimpleValueVariant(1.0));
    ASSERT_EQ(Default("enum"), SimpleValueVariant(int32_t{2}));
    ASSERT_EQ(Default("array"), SimpleValueVariant(uint8_t{7}));

    // Defaults that are null, missing, don't fit the field or are on a string are left unset
    ASSERT_EQ(Default("null"), std::nullopt);
    ASSERT_EQ(Default("missing"), std::nullopt);
    ASSERT_EQ(Default("negative_ulong"), std::nullopt);
    ASSERT_EQ(Default("large_uchar"), std::nullopt);
    ASSERT_EQ(Default("float_long"), std::nullopt);
    ASSERT_EQ(Default("string"), std::nullopt);
}
