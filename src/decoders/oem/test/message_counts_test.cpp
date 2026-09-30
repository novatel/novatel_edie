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
// ! \file message_counts_test.cpp
// ===============================================================================

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <unordered_set>

#include <gtest/gtest.h>

#include "novatel_edie/decoders/common/json_db_reader.hpp"
#include "novatel_edie/decoders/oem/file_parser.hpp"
#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/parser.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

namespace {

//! The message ID of BESTPOS. Every log literal below is a BESTPOS log.
constexpr uint16_t usBestPosId = 42;

//! The message IDs of the two logs in the BESTUTMBIN.GPS test file.
//! The name of the file says BIN, but the file holds ASCII logs.
constexpr uint16_t usVersionId = 37;
constexpr uint16_t usBestUtmId = 726;

constexpr unsigned char aucAsciiBestPos[] =
    "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,"
    "WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";

//! The same message and format as aucAsciiBestPos, with the sibling suffix _1 on the name.
constexpr unsigned char aucAsciiBestPosSibling1[] =
    "#BESTPOSA_1,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,1097.6822,-17.0000,"
    "WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";

//! The same message as aucAsciiBestPos, in abbreviated ASCII format.
constexpr unsigned char aucAbbrevAsciiBestPos[] =
    "<BESTPOS COM1 0 72.0 FINESTEERING 2215 148248.000 02000020 cdba 32768\r\n"
    "<     SOL_COMPUTED SINGLE 51.15043711386 -114.03067767000 1097.2099 -17.0000 WGS84 0.9038 0.8534 1.7480 \"\" 0.000 0.000 35 30 30 30 00 06 39 "
    "33\r\n";

//! A log whose name is not in the message database. The decoder cannot resolve an ID for it.
constexpr unsigned char aucAsciiUnknownName[] = "#NOTALOGA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;0*ffffffff\r\n";

//! \brief Add the counts of every key in the map.
uint64_t TotalCounts(const HeaderDecoder::MessageCountsMap& mapCounts_)
{
    return std::accumulate(mapCounts_.begin(), mapCounts_.end(), uint64_t{0},
                           [](uint64_t ullSum_, const auto& itCount_) { return ullSum_ + itCount_.second; });
}

} // namespace

// -------------------------------------------------------------------------------------------------------
// MessageCountsKeyHash tests
// -------------------------------------------------------------------------------------------------------
TEST(MessageCountsKeyHashTest, EACH_FIELD_GETS_ITS_OWN_BITS)
{
    const HeaderDecoder::MessageCountsKeyHash clHash;
    constexpr std::size_t ullBinary = static_cast<std::size_t>(DECODE_FORMAT::BINARY) << 24;

    // The sibling ID holds bits 0 to 7. The message ID holds bits 8 to 23.
    ASSERT_EQ(clHash({0, DECODE_FORMAT::BINARY, 0}), ullBinary);
    ASSERT_EQ(clHash({1, DECODE_FORMAT::BINARY, 0}), ullBinary | (std::size_t{1} << 8));
    ASSERT_EQ(clHash({0, DECODE_FORMAT::BINARY, 1}), ullBinary | std::size_t{1});

    // The widest value of each field still stays inside the bits of that field.
    ASSERT_EQ(clHash({UINT16_MAX, DECODE_FORMAT::BINARY, UINT8_MAX}), ullBinary | (std::size_t{UINT16_MAX} << 8) | std::size_t{UINT8_MAX});
}

TEST(MessageCountsKeyHashTest, EVERY_KEY_HAS_ITS_OWN_HASH_VALUE)
{
    const HeaderDecoder::MessageCountsKeyHash clHash;
    std::unordered_set<std::size_t> setHashes;
    std::size_t ullKeyCount = 0;

    // Cover every format, every message ID the database can hold, and the low sibling IDs.
    for (auto eFormat = static_cast<uint32_t>(DECODE_FORMAT::UNKNOWN); eFormat <= static_cast<uint32_t>(DECODE_FORMAT::ALL); eFormat++)
    {
        for (uint16_t usMessageId = 0; usMessageId < 3000; usMessageId++)
        {
            for (uint8_t ucSiblingId = 0; ucSiblingId < 4; ucSiblingId++)
            {
                setHashes.insert(clHash({usMessageId, static_cast<DECODE_FORMAT>(eFormat), ucSiblingId}));
                ullKeyCount++;
            }
        }
    }

    // No two keys share a hash value, so the map never compares two unrelated keys.
    ASSERT_EQ(setHashes.size(), ullKeyCount);
}

// -------------------------------------------------------------------------------------------------------
// HeaderDecoder message count tests
// -------------------------------------------------------------------------------------------------------
class HeaderDecoderCountsTest : public ::testing::Test
{
  protected:
    static MessageDatabase::Ptr pclMyJsonDb;
    std::unique_ptr<HeaderDecoder> pclMyHeaderDecoder;

    static void SetUpTestSuite()
    {
        try
        {
            pclMyJsonDb = LoadJsonDbFile(std::getenv("TEST_DATABASE_PATH"));
        }
        catch (JsonDbReaderFailure& e)
        {
            std::cout << e.what() << '\n';
        }
    }

    static void TearDownTestSuite() { LOGGER_MANAGER->Shutdown(); }

    // Every test starts with a decoder that has no counts.
    void SetUp() override { pclMyHeaderDecoder = std::make_unique<HeaderDecoder>(pclMyJsonDb); }

    //! \brief Decode one header and return the status.
    STATUS DecodeHeader(const unsigned char* pucLog_)
    {
        IntermediateHeader stHeader;
        MetaDataStruct stMetaData;
        return pclMyHeaderDecoder->Decode(pucLog_, stHeader, stMetaData);
    }
};

MessageDatabase::Ptr HeaderDecoderCountsTest::pclMyJsonDb = nullptr;

TEST_F(HeaderDecoderCountsTest, NEW_DECODER_HAS_NO_COUNTS) { ASSERT_TRUE(pclMyHeaderDecoder->GetMessageCounts().empty()); }

TEST_F(HeaderDecoderCountsTest, EACH_DECODE_INCREMENTS_ONE_KEY)
{
    ASSERT_EQ(DecodeHeader(aucAsciiBestPos), STATUS::SUCCESS);
    ASSERT_EQ(DecodeHeader(aucAsciiBestPos), STATUS::SUCCESS);
    ASSERT_EQ(DecodeHeader(aucAsciiBestPos), STATUS::SUCCESS);

    const auto& mapCounts = pclMyHeaderDecoder->GetMessageCounts();
    ASSERT_EQ(mapCounts.size(), 1U);
    ASSERT_EQ(mapCounts.at({usBestPosId, DECODE_FORMAT::ASCII, 0}), 3U);
}

TEST_F(HeaderDecoderCountsTest, FORMAT_IS_PART_OF_THE_KEY)
{
    ASSERT_EQ(DecodeHeader(aucAsciiBestPos), STATUS::SUCCESS);
    ASSERT_EQ(DecodeHeader(aucAbbrevAsciiBestPos), STATUS::SUCCESS);

    // One message in two formats gives two keys, and therefore two counts.
    const auto& mapCounts = pclMyHeaderDecoder->GetMessageCounts();
    ASSERT_EQ(mapCounts.size(), 2U);
    ASSERT_EQ(mapCounts.at({usBestPosId, DECODE_FORMAT::ASCII, 0}), 1U);
    ASSERT_EQ(mapCounts.at({usBestPosId, DECODE_FORMAT::ABB_ASCII, 0}), 1U);
}

TEST_F(HeaderDecoderCountsTest, SIBLING_ID_IS_PART_OF_THE_KEY)
{
    ASSERT_EQ(DecodeHeader(aucAsciiBestPos), STATUS::SUCCESS);
    ASSERT_EQ(DecodeHeader(aucAsciiBestPosSibling1), STATUS::SUCCESS);

    // One message from two sources gives two keys, and therefore two counts.
    const auto& mapCounts = pclMyHeaderDecoder->GetMessageCounts();
    ASSERT_EQ(mapCounts.size(), 2U);
    ASSERT_EQ(mapCounts.at({usBestPosId, DECODE_FORMAT::ASCII, 0}), 1U);
    ASSERT_EQ(mapCounts.at({usBestPosId, DECODE_FORMAT::ASCII, 1}), 1U);
}

TEST_F(HeaderDecoderCountsTest, A_MESSAGE_WITHOUT_AN_ID_IS_NOT_COUNTED)
{
    // The decoder cannot resolve the name, so the message ID stays 0.
    ASSERT_EQ(DecodeHeader(aucAsciiUnknownName), STATUS::SUCCESS);
    ASSERT_TRUE(pclMyHeaderDecoder->GetMessageCounts().empty());
}

TEST_F(HeaderDecoderCountsTest, RESET_CLEARS_THE_COUNTS)
{
    ASSERT_EQ(DecodeHeader(aucAsciiBestPos), STATUS::SUCCESS);
    ASSERT_EQ(pclMyHeaderDecoder->GetMessageCounts().size(), 1U);

    pclMyHeaderDecoder->ResetMessageCounts();
    ASSERT_TRUE(pclMyHeaderDecoder->GetMessageCounts().empty());

    // The decoder counts again after a reset, and it starts from 0.
    ASSERT_EQ(DecodeHeader(aucAsciiBestPos), STATUS::SUCCESS);
    ASSERT_EQ(pclMyHeaderDecoder->GetMessageCounts().at({usBestPosId, DECODE_FORMAT::ASCII, 0}), 1U);
}

// -------------------------------------------------------------------------------------------------------
// Parser message count tests
// -------------------------------------------------------------------------------------------------------
class ParserCountsTest : public ::testing::Test
{
  protected:
    std::unique_ptr<Parser> pclMyParser;

    void SetUp() override
    {
        try
        {
            pclMyParser = std::make_unique<Parser>(std::getenv("TEST_DATABASE_PATH"));
        }
        catch (JsonDbReaderFailure& e)
        {
            std::cout << e.what() << '\n';
        }
    }

    static void TearDownTestSuite() { LOGGER_MANAGER->Shutdown(); }

    //! \brief Write one log to the parser and read every message it produces.
    uint32_t ParseLog(const unsigned char* pucLog_, uint32_t uiLength_)
    {
        EXPECT_EQ(pclMyParser->Write(pucLog_, uiLength_), uiLength_);

        uint32_t uiSuccessCount = 0;
        MessageDataStruct stMessageData;
        MetaDataStruct stMetaData;
        while (pclMyParser->Read(stMessageData, stMetaData) == STATUS::SUCCESS) { uiSuccessCount++; }
        return uiSuccessCount;
    }
};

TEST_F(ParserCountsTest, PARSER_EXPOSES_THE_HEADER_DECODER_COUNTS)
{
    ASSERT_TRUE(pclMyParser->GetMessageCounts().empty());

    ASSERT_EQ(ParseLog(aucAsciiBestPos, sizeof(aucAsciiBestPos) - 1), 1U);
    ASSERT_EQ(ParseLog(aucAsciiBestPos, sizeof(aucAsciiBestPos) - 1), 1U);

    const auto& mapCounts = pclMyParser->GetMessageCounts();
    ASSERT_EQ(mapCounts.size(), 1U);
    ASSERT_EQ(mapCounts.at({usBestPosId, DECODE_FORMAT::ASCII, 0}), 2U);
}

TEST_F(ParserCountsTest, RESET_CLEARS_THE_COUNTS)
{
    ASSERT_EQ(ParseLog(aucAsciiBestPos, sizeof(aucAsciiBestPos) - 1), 1U);
    ASSERT_FALSE(pclMyParser->GetMessageCounts().empty());

    pclMyParser->ResetMessageCounts();
    ASSERT_TRUE(pclMyParser->GetMessageCounts().empty());
}

// -------------------------------------------------------------------------------------------------------
// FileParser message count tests
// -------------------------------------------------------------------------------------------------------
class FileParserCountsTest : public ::testing::Test
{
  protected:
    std::unique_ptr<FileParser> pclMyFileParser;

    void SetUp() override
    {
        try
        {
            pclMyFileParser = std::make_unique<FileParser>(std::getenv("TEST_DATABASE_PATH"));
        }
        catch (JsonDbReaderFailure& e)
        {
            std::cout << e.what() << '\n';
        }
    }

    static void TearDownTestSuite() { LOGGER_MANAGER->Shutdown(); }

    //! \brief Open the test file and give it to the FileParser.
    void SetTestStream()
    {
        const std::filesystem::path clTestFile = std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / "BESTUTMBIN.GPS";
        auto pclInputStream = std::make_shared<std::ifstream>(clTestFile.string().c_str(), std::ios::binary);
        ASSERT_TRUE(pclMyFileParser->SetStream(pclInputStream));
    }

    //! \brief Read the whole file and return the number of messages read.
    uint32_t ReadWholeFile()
    {
        uint32_t uiSuccessCount = 0;
        MessageDataStruct stMessageData;
        MetaDataStruct stMetaData;
        STATUS eStatus;
        while ((eStatus = pclMyFileParser->Read(stMessageData, stMetaData)) != STATUS::STREAM_EMPTY)
        {
            if (eStatus == STATUS::SUCCESS) { uiSuccessCount++; }
        }
        return uiSuccessCount;
    }
};

TEST_F(FileParserCountsTest, FILE_PARSER_COUNTS_EVERY_MESSAGE_IN_THE_FILE)
{
    ASSERT_TRUE(pclMyFileParser->GetMessageCounts().empty());
    SetTestStream();

    const uint32_t uiMessagesRead = ReadWholeFile();
    ASSERT_EQ(uiMessagesRead, 2U);

    // The file holds one VERSION log and one BESTUTM log, both in ASCII format.
    const auto& mapCounts = pclMyFileParser->GetMessageCounts();
    ASSERT_EQ(mapCounts.size(), 2U);
    ASSERT_EQ(mapCounts.at({usVersionId, DECODE_FORMAT::ASCII, 0}), 1U);
    ASSERT_EQ(mapCounts.at({usBestUtmId, DECODE_FORMAT::ASCII, 0}), 1U);

    // Every message that the parser read has a count.
    ASSERT_EQ(TotalCounts(mapCounts), uiMessagesRead);
}

TEST_F(FileParserCountsTest, RESET_CLEARS_THE_COUNTS)
{
    SetTestStream();
    ASSERT_EQ(ReadWholeFile(), 2U);
    ASSERT_FALSE(pclMyFileParser->GetMessageCounts().empty());

    // Reset() rewinds the stream, so the counts of the finished pass must not stay.
    ASSERT_TRUE(pclMyFileParser->Reset());
    ASSERT_TRUE(pclMyFileParser->GetMessageCounts().empty());

    // A second pass over the same file gives the same counts as the first pass.
    ASSERT_EQ(ReadWholeFile(), 2U);
    const auto& mapCounts = pclMyFileParser->GetMessageCounts();
    ASSERT_EQ(TotalCounts(mapCounts), 2U);
    ASSERT_EQ(mapCounts.at({usVersionId, DECODE_FORMAT::ASCII, 0}), 1U);
    ASSERT_EQ(mapCounts.at({usBestUtmId, DECODE_FORMAT::ASCII, 0}), 1U);
}

TEST_F(FileParserCountsTest, SET_STREAM_CLEARS_THE_COUNTS)
{
    SetTestStream();
    ASSERT_EQ(ReadWholeFile(), 2U);
    ASSERT_FALSE(pclMyFileParser->GetMessageCounts().empty());

    // A new stream starts a new set of counts. SetStream() calls Reset().
    SetTestStream();
    ASSERT_TRUE(pclMyFileParser->GetMessageCounts().empty());
}

TEST_F(FileParserCountsTest, RESET_MESSAGE_COUNTS_KEEPS_THE_STREAM)
{
    SetTestStream();
    ASSERT_EQ(ReadWholeFile(), 2U);

    // ResetMessageCounts() clears the counts only. It does not rewind the stream.
    pclMyFileParser->ResetMessageCounts();
    ASSERT_TRUE(pclMyFileParser->GetMessageCounts().empty());
    ASSERT_EQ(ReadWholeFile(), 0U);
    ASSERT_TRUE(pclMyFileParser->GetMessageCounts().empty());
}
