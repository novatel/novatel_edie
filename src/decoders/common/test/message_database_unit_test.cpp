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
// ! \file message_database_unit_test.cpp
// ===============================================================================

#include <filesystem>

#include <gtest/gtest.h>

#include "novatel_edie/decoders/common/common.hpp"
#include "novatel_edie/decoders/common/message_database.hpp"

using namespace novatel::edie;

class MessageDatabaseTest : public testing::Test
{
  protected:
    std::shared_ptr<BaseField> f0;
    std::shared_ptr<BaseField> f1;
    static constexpr uint32_t kMsgCrc = 0x11223344U;
    static constexpr uint32_t kMsgCrc2 = 0x55667788U;

    void SetUp() override
    {
        MessageDatabase::RegisterAlignmentFunction("OEM", [](const size_t size, const uintptr_t start, const uintptr_t ptr) {
            size_t alignment = std::min(size_t{4}, size);
            size_t offset = (ptr - start) % alignment;
            return offset == 0 ? 0 : alignment - offset;
        });

        f0 = std::make_shared<BaseField>("short", FIELD_TYPE::SIMPLE, "%hu", DATA_TYPE::USHORT);
        f1 = std::make_shared<BaseField>("int", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::INT);
    }

    std::shared_ptr<MessageDefinition> CreateMessageDefinition(uint32_t logID, const std::string& name)
    {
        auto msgDef = std::make_shared<MessageDefinition>();
        msgDef->logID = logID;
        msgDef->name = name;
        const auto fieldInfo = BuildFieldInfo({f0, f1});
        msgDef->fieldInfo.emplace(kMsgCrc, fieldInfo);
        msgDef->latestMessageCrc = kMsgCrc;
        return msgDef;
    }

    static FieldInfo::ConstPtr CreateSimpleFieldInfo()
    {
        auto a = std::make_shared<BaseField>("short", FIELD_TYPE::SIMPLE, "%hu", DATA_TYPE::USHORT);
        auto b = std::make_shared<BaseField>("int", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::INT);
        return BuildFieldInfo({a, b});
    }

    static std::shared_ptr<MessageDefinition> CreateMultiCrcMessageDefinition(uint32_t logID, const std::string& name)
    {
        auto msgDef = std::make_shared<MessageDefinition>();
        msgDef->logID = logID;
        msgDef->name = name;
        msgDef->fieldInfo.emplace(kMsgCrc, CreateSimpleFieldInfo());
        msgDef->fieldInfo.emplace(kMsgCrc2, CreateSimpleFieldInfo());
        msgDef->latestMessageCrc = kMsgCrc2;
        return msgDef;
    }

    static std::shared_ptr<MessageDefinition> CreateComplexMultiCrcMessageDefinition(uint32_t logID, const std::string& name)
    {
        auto msgDef = std::make_shared<MessageDefinition>();
        msgDef->logID = logID;
        msgDef->name = name;
        msgDef->fieldInfo.emplace(kMsgCrc, CreateSimpleFieldInfo());

        auto parentField0 = std::make_shared<BaseField>("rootShort", FIELD_TYPE::SIMPLE, "%hu", DATA_TYPE::USHORT);
        auto parentField1 = std::make_shared<ArrayField>("rootVar", FIELD_TYPE::VARIABLE_LENGTH_ARRAY, "%u", DATA_TYPE::UINT, 10);
        auto parentField2 = std::make_shared<BaseField>("rootInt", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::INT);
        auto nestedField0 = std::make_shared<BaseField>("nestedShort", FIELD_TYPE::SIMPLE, "%hu", DATA_TYPE::USHORT);
        auto nestedField1 = std::make_shared<BaseField>("nestedInt", FIELD_TYPE::SIMPLE, "%d", DATA_TYPE::INT);
        auto nestedInfo = BuildFieldInfo({nestedField0, nestedField1});
        auto fieldArray = std::make_shared<FieldArrayField>("nestedArray", FIELD_TYPE::FIELD_ARRAY, "", DATA_TYPE::UNKNOWN, 2, nestedInfo);

        msgDef->fieldInfo.emplace(kMsgCrc2, BuildFieldInfo({parentField0, parentField1, parentField2, fieldArray}));
        msgDef->latestMessageCrc = kMsgCrc2;
        return msgDef;
    }
};

TEST_F(MessageDatabaseTest, NoAlignByDefault)
{
    auto msgDef = CreateMessageDefinition(123U, "TESTMSG");

    MessageDatabase db({msgDef}, {});
    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_NE(retrievedMsgDef, nullptr);
    const auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 6U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 2U); // No alignment, int field starts immediately after the short field
}

TEST_F(MessageDatabaseTest, OEMAlign)
{
    auto msgDef = CreateMessageDefinition(123U, "TESTMSG");

    MessageDatabase db({msgDef}, {});
    db.SetMessageFamily("OEM");
    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_NE(retrievedMsgDef, nullptr);
    const auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 8U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 4U); // OEM alignment
}

TEST_F(MessageDatabaseTest, AppendMessagesCopies)
{
    auto msgDef = CreateMessageDefinition(124U, "TESTMSG");

    MessageDatabase db({}, {});
    db.SetMessageFamily("OEM");
    db.AppendMessages({msgDef});

    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_NE(retrievedMsgDef, nullptr);
    const auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 8U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 4U);          // OEM alignment
    ASSERT_EQ(msgDef->fieldInfo[kMsgCrc]->messageOrderedFields[1]->index, 2U); // Original message definition is unchanged
}

TEST_F(MessageDatabaseTest, MessageDefinitionsNotCopiedOnInit)
{
    auto msgDef = CreateMessageDefinition(124U, "TESTMSG");

    MessageDatabase db({msgDef}, {});

    auto retrievedMsgDef = db.GetMsgDef("TESTMSG");
    ASSERT_EQ(retrievedMsgDef, msgDef); // The message definition is not copied on init
}

TEST_F(MessageDatabaseTest, MessageDefinitionsCopiedOnMerge)
{
    auto msgDef1 = CreateMessageDefinition(123U, "TESTMSG1");
    auto msgDef2 = CreateMessageDefinition(124U, "TESTMSG2");

    MessageDatabase noAlignDb({msgDef1}, {});
    MessageDatabase oemDb({msgDef2}, {});
    oemDb.SetMessageFamily("OEM");
    oemDb.Merge(noAlignDb);
    auto retrievedMsgDef = noAlignDb.GetMsgDef("TESTMSG1");
    ASSERT_NE(retrievedMsgDef, nullptr);
    auto retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 6U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 2U); // No alignment, int field starts immediately after the short field

    retrievedMsgDef = oemDb.GetMsgDef("TESTMSG2");
    ASSERT_NE(retrievedMsgDef, nullptr);
    retrievedFieldInfo = retrievedMsgDef->GetMsgDefFromCrc(kMsgCrc);
    ASSERT_EQ(retrievedFieldInfo.fixedFieldBytes, 8U);
    ASSERT_EQ(retrievedFieldInfo.varFieldCount, 0U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields.size(), 2U);
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[0]->name, "short");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->name, "int");
    ASSERT_EQ(retrievedFieldInfo.messageOrderedFields[1]->index, 4U);           // OEM alignment
    ASSERT_EQ(msgDef2->fieldInfo[kMsgCrc]->messageOrderedFields[1]->index, 2U); // Original message definition is unchanged
}

TEST_F(MessageDatabaseTest, SetMessageFamilyRebuildsAllMessagesAndAllCrcEntries)
{
    auto msgDef1 = CreateMultiCrcMessageDefinition(200U, "TESTMSG1");
    auto msgDef2 = CreateMultiCrcMessageDefinition(201U, "TESTMSG2");
    MessageDatabase db({msgDef1, msgDef2}, {});

    ASSERT_EQ(db.GetMsgDef("TESTMSG1")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 2U);
    ASSERT_EQ(db.GetMsgDef("TESTMSG1")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 2U);
    ASSERT_EQ(db.GetMsgDef("TESTMSG2")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 2U);
    ASSERT_EQ(db.GetMsgDef("TESTMSG2")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 2U);

    db.SetMessageFamily("OEM");

    ASSERT_EQ(db.GetMsgDef("TESTMSG1")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 4U);
    ASSERT_EQ(db.GetMsgDef("TESTMSG1")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 4U);
    ASSERT_EQ(db.GetMsgDef("TESTMSG2")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 4U);
    ASSERT_EQ(db.GetMsgDef("TESTMSG2")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 4U);
}

TEST_F(MessageDatabaseTest, AppendMessagesReplacesExistingLogIdAndNameMappings)
{
    auto oldDef = CreateMessageDefinition(300U, "OLDMSG");
    auto newDef = CreateMessageDefinition(300U, "NEWMSG");
    MessageDatabase db({oldDef}, {});

    db.AppendMessages({newDef});

    ASSERT_EQ(db.MessageDefinitions().size(), 1U);
    ASSERT_EQ(db.GetMsgDef("OLDMSG"), nullptr);
    const auto replaced = db.GetMsgDef("NEWMSG");
    ASSERT_NE(replaced, nullptr);
    ASSERT_EQ(replaced->logID, 300U);
    ASSERT_EQ(db.GetMsgDef(300)->name, "NEWMSG");
}

TEST_F(MessageDatabaseTest, AppendMessagesRebuildsEveryCrcAndPreservesSourceDefinition)
{
    auto source = CreateComplexMultiCrcMessageDefinition(400U, "SRCMSG");
    MessageDatabase db({}, {});
    db.SetMessageFamily("OEM");

    db.AppendMessages({source});

    const auto appended = db.GetMsgDef("SRCMSG");
    ASSERT_NE(appended, nullptr);
    ASSERT_EQ(appended->fieldInfo.begin()->second->messageOrderedFields[1]->index, 4U);

    const auto& complexInfo = appended->GetMsgDefFromCrc(kMsgCrc2);

    ASSERT_EQ(complexInfo.fixedFieldBytes, 8U);
    ASSERT_EQ(complexInfo.varFieldCount, 2U);
    ASSERT_EQ(complexInfo.messageOrderedFields.size(), 4U);
    ASSERT_EQ(complexInfo.messageOrderedFields[3]->type, FIELD_TYPE::FIELD_ARRAY);

    const auto appendedFieldArray = std::dynamic_pointer_cast<const FieldArrayField>(complexInfo.messageOrderedFields[3]);
    const auto sourceFieldArray = std::dynamic_pointer_cast<const FieldArrayField>(source->GetMsgDefFromCrc(kMsgCrc2).messageOrderedFields[3]);
    ASSERT_NE(appendedFieldArray, nullptr);
    ASSERT_NE(sourceFieldArray, nullptr);
    ASSERT_EQ(appendedFieldArray->fieldInfo->messageOrderedFields.size(), 2U);
    ASSERT_NE(appendedFieldArray->fieldInfo->messageOrderedFields[0].get(), sourceFieldArray->fieldInfo->messageOrderedFields[0].get());
}

TEST_F(MessageDatabaseTest, AppendMessagesRebuildsFixedAndVariableIndices)
{
    auto source = CreateComplexMultiCrcMessageDefinition(400U, "SRCMSG");
    MessageDatabase db({}, {});
    db.SetMessageFamily("OEM");

    db.AppendMessages({source});

    const auto appended = db.GetMsgDef("SRCMSG");
    ASSERT_NE(appended, nullptr);

    const auto& complexInfo = appended->GetMsgDefFromCrc(kMsgCrc2);

    ASSERT_EQ(complexInfo.messageOrderedFields[0]->index, 0U); // rootShort
    ASSERT_EQ(complexInfo.messageOrderedFields[1]->index, 0U); // rootVar
    ASSERT_EQ(complexInfo.messageOrderedFields[2]->index, 4U); // rootInt
    ASSERT_EQ(complexInfo.messageOrderedFields[3]->index, 1U); // fieldArray
}

TEST_F(MessageDatabaseTest, MergeOverwritesConflictsUsingTargetFamilyAndPreservesSource)
{
    auto targetOriginal = CreateMessageDefinition(500U, "TARGET_ORIGINAL");
    auto sourceReplacement = CreateMessageDefinition(500U, "SOURCE_REPLACEMENT");
    auto sourceExtra = CreateMessageDefinition(501U, "SOURCE_EXTRA");

    MessageDatabase targetDb({targetOriginal}, {});
    targetDb.SetMessageFamily("OEM");

    MessageDatabase sourceDb({sourceReplacement, sourceExtra}, {});
    targetDb.Merge(sourceDb);

    ASSERT_EQ(targetDb.MessageDefinitions().size(), 2U);
    ASSERT_EQ(targetDb.GetMsgDef("TARGET_ORIGINAL"), nullptr);
    ASSERT_NE(targetDb.GetMsgDef("SOURCE_REPLACEMENT"), nullptr);
    ASSERT_NE(targetDb.GetMsgDef("SOURCE_EXTRA"), nullptr);
    ASSERT_EQ(targetDb.GetMsgDef("SOURCE_REPLACEMENT")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 4U);
    ASSERT_EQ(targetDb.GetMsgDef("SOURCE_EXTRA")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 4U);

    ASSERT_EQ(sourceDb.GetMsgDef("SOURCE_REPLACEMENT")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 2U);
    ASSERT_EQ(sourceDb.GetMsgDef("SOURCE_EXTRA")->fieldInfo.begin()->second->messageOrderedFields[1]->index, 2U);
}
