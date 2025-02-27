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
// ! \file proprietary_test.cpp
// ===============================================================================

#include <filesystem>

#include <gtest/gtest.h>

#include "novatel_edie/common/framer_manager.hpp"
#include "novatel_edie/decoders/oem/framer.hpp"

using namespace novatel::edie;
using namespace novatel::edie::oem;

class ProprietaryFramerTest : public ::testing::Test
{
  protected:
    static std::unique_ptr<std::istream> pclMyIFS;
    static std::unique_ptr<unsigned char[]> pucMyTestFrameBuffer;

    // Per-test-suite setup
    static void SetUpTestSuite()
    {
        novatel::edie::FramerManager& clMyFramerManager = FramerManager::GetInstance();
        clMyFramerManager.SetLoggerLevel(spdlog::level::info);
        clMyFramerManager.SetReportUnknownBytes(true);

        clMyFramerManager.RegisterFramer("NOVATEL", std::make_unique<oem::Framer>(), std::make_unique<oem::MetaDataStruct>());

        pucMyTestFrameBuffer = std::make_unique<unsigned char[]>(131071); // 128kB
    }

    // Per-test-suite teardown
    static void TearDownTestSuite() { Logger::Shutdown(); }

    // Per-test setup
    void SetUp() override
    {
        FlushTestFixture();
        pucMyTestFrameBuffer = std::make_unique<unsigned char[]>(131071); // 128k
        novatel::edie::FramerManager& clMyFramerManager = FramerManager::GetInstance();
        clMyFramerManager.ResetAllFramerStates();
        clMyFramerManager.ResetAllMetaDataStates();
    }

    // Per-test teardown
    void TearDown() override
    {
        FlushTestFixture();
        novatel::edie::FramerManager& clMyFramerManager = FramerManager::GetInstance();
        clMyFramerManager.ResetAllFramerStates();
        clMyFramerManager.ResetAllMetaDataStates();
    }

  public:
    template <HEADER_FORMAT F, STATUS S> static void FramerHelper(uint32_t uiLength_, uint32_t uiFrameLength_, int& id_);

    template <HEADER_FORMAT F, STATUS S> static void FramerHelper(uint32_t uiLength_, uint32_t uiFrameLength_);

    void WriteFileStreamToFramer(std::string sFilename_)
    {
        pclMyIFS = std::make_unique<std::ifstream>(std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / sFilename_, std::ios::binary);

        std::array<char, MAX_ASCII_MESSAGE_LENGTH> cData;
        uint32_t uiBytesWritten = 0;

        FramerManager& clMyFramerManager = FramerManager::GetInstance();

        while (!pclMyIFS->eof())
        {
            pclMyIFS->read(cData.data(), cData.size());
            uiBytesWritten = clMyFramerManager.Write(reinterpret_cast<unsigned char*>(cData.data()), pclMyIFS->gcount());
            ASSERT_NE(uiBytesWritten, 0);
            ASSERT_EQ(uiBytesWritten, pclMyIFS->gcount());
        }
    }

    static void WriteBytesToFramer(unsigned char* pucBytes_, uint32_t uiNumBytes_)
    {
        FramerManager& clMyFramerManager = FramerManager::GetInstance();
        ASSERT_EQ(clMyFramerManager.Write(pucBytes_, uiNumBytes_), uiNumBytes_);
    }

    static void FlushTestFixture()
    {
        FramerManager& clMyFramerManager = FramerManager::GetInstance();
        while (clMyFramerManager.Flush(pucMyTestFrameBuffer.get(), MAX_ASCII_MESSAGE_LENGTH) > 0) {}
        pucMyTestFrameBuffer.reset();
    }
};

template <HEADER_FORMAT F, STATUS S> void ProprietaryFramerTest::FramerHelper(uint32_t uiLength_, uint32_t uiFrameLength_, int& id_)
{
    FramerManager& clMyFramerManager = FramerManager::GetInstance();
    MetaDataStruct stExpectedMetaData(F, uiLength_);
    ASSERT_EQ(S, clMyFramerManager.GetFrame(pucMyTestFrameBuffer.get(), uiFrameLength_, id_));
    MetaDataStruct* stTestMetaData = dynamic_cast<MetaDataStruct*>(clMyFramerManager.GetMetaData(clMyFramerManager.idMap["NOVATEL"]));
    ASSERT_EQ(*stTestMetaData, stExpectedMetaData);
}

template <HEADER_FORMAT F, STATUS S> void ProprietaryFramerTest::FramerHelper(uint32_t uiLength_, uint32_t uiFrameLength_)
{
    FramerManager& clMyFramerManager = FramerManager::GetInstance();
    int id = clMyFramerManager.idMap["UNKNOWN"];
    FramerHelper<F, S>(uiLength_, uiFrameLength_, id);
}

std::unique_ptr<std::istream> ProprietaryFramerTest::pclMyIFS = nullptr;
std::unique_ptr<unsigned char[]> ProprietaryFramerTest::pucMyTestFrameBuffer = nullptr;

// -------------------------------------------------------------------------------------------------------
// Proprietary Binary Framer Unit Tests
// -------------------------------------------------------------------------------------------------------
TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_COMPLETE)
{
    // "GARBAGE_DATA<binary bestpos log>"
    uint8_t aucData[] = {0x47, 0x41, 0x52, 0x42, 0x41, 0x47, 0x45, 0x5F, 0x44, 0x41, 0x54, 0x41, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09,
                         0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19, 0x00, 0x00, 0x00, 0x02,
                         0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x14, 0xDC,
                         0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35,
                         0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    WriteBytesToFramer(aucData, sizeof(aucData));

    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(12, MAX_BINARY_MESSAGE_LENGTH);

    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::SUCCESS>(76, MAX_BINARY_MESSAGE_LENGTH);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_INCOMPLETE)
{
    // "<incomplete binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19,
                         0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5,
                         0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A};
    WriteBytesToFramer(aucData, sizeof(aucData));

    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::INCOMPLETE>(59, MAX_BINARY_MESSAGE_LENGTH);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_SYNC_ERROR)
{
    WriteFileStreamToFramer("proprietary_binary_sync_error.BIN");

    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(MAX_BINARY_MESSAGE_LENGTH, MAX_BINARY_MESSAGE_LENGTH);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_BAD_CRC)
{
    // "<encrypted binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xFF};
    WriteBytesToFramer(aucData, sizeof(aucData));

    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(3, MAX_BINARY_MESSAGE_LENGTH);
    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(73, MAX_BINARY_MESSAGE_LENGTH);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_RUN_ON_CRC)
{
    // "<encrypted binary bestpos log>FF"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19,
                         0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5,
                         0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35,
                         0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC, 0xFF};
    WriteBytesToFramer(aucData, sizeof(aucData));

    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::SUCCESS>(76, MAX_BINARY_MESSAGE_LENGTH);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_INADEQUATE_BUFFER)
{
    // "<encrypted binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    WriteBytesToFramer(aucData, sizeof(aucData));

    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::BUFFER_FULL>(sizeof(aucData), sizeof(aucData) - 1);
    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::SUCCESS>(sizeof(aucData), sizeof(aucData));
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_BYTE_BY_BYTE)
{
    FramerManager& clMyFramerManager = FramerManager::GetInstance();
    // "<binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    uint32_t uiLogSize = sizeof(aucData);
    uint32_t uiRemainingBytes = uiLogSize;

    MetaDataStruct stExpectedMetaData(HEADER_FORMAT::UNKNOWN);
    MetaDataStruct* stTestMetaData = dynamic_cast<MetaDataStruct*>(clMyFramerManager.GetMetaData(clMyFramerManager.idMap["NOVATEL"]));
    int id = clMyFramerManager.idMap["UNKNOWN"];

    while (true)
    {
        WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
        uiRemainingBytes--;
        stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

        if (stExpectedMetaData.uiLength == OEM4_BINARY_SYNC_LENGTH) { stExpectedMetaData.eFormat = HEADER_FORMAT::PROPRIETARY_BINARY; }

        if (stExpectedMetaData.uiLength > OEM4_BINARY_SYNC_LENGTH) { id = clMyFramerManager.idMap["NOVATEL"]; }

        if (uiRemainingBytes == 0) { break; }

        ASSERT_EQ(STATUS::INCOMPLETE, clMyFramerManager.GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, id));
        ASSERT_EQ(*stTestMetaData, stExpectedMetaData);
    }
    stExpectedMetaData.uiLength = uiLogSize;
    ASSERT_EQ(STATUS::SUCCESS, clMyFramerManager.GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, id));
    ASSERT_EQ(*stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_SEGMENTED)
{
    FramerManager& clMyFramerManager = FramerManager::GetInstance();
    // "<binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    uint32_t uiBytesWritten = 0;
    int id = clMyFramerManager.idMap["UNKNOWN"];

    WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_BINARY_SYNC_LENGTH);
    uiBytesWritten += OEM4_BINARY_SYNC_LENGTH;
    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::INCOMPLETE>(uiBytesWritten, MAX_BINARY_MESSAGE_LENGTH, id);

    WriteBytesToFramer(&aucData[uiBytesWritten], (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH));
    uiBytesWritten += (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH);
    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::INCOMPLETE>(uiBytesWritten, MAX_BINARY_MESSAGE_LENGTH, id);

    WriteBytesToFramer(&aucData[uiBytesWritten], 44);
    uiBytesWritten += 44;
    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::INCOMPLETE>(uiBytesWritten, MAX_BINARY_MESSAGE_LENGTH, id);

    WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_BINARY_CRC_LENGTH);
    uiBytesWritten += OEM4_BINARY_CRC_LENGTH;
    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::SUCCESS>(uiBytesWritten, MAX_BINARY_MESSAGE_LENGTH, id);
    ASSERT_EQ(sizeof(aucData), uiBytesWritten);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_TRICK)
{
    // "<binary syncs><binary sync + half header><binary sync byte 1><binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0xAA,
                         0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};

    WriteBytesToFramer(aucData, sizeof(aucData));

    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(3, MAX_BINARY_MESSAGE_LENGTH);
    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(3, MAX_BINARY_MESSAGE_LENGTH);
    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(12, MAX_BINARY_MESSAGE_LENGTH);
    FramerHelper<HEADER_FORMAT::UNKNOWN, STATUS::UNKNOWN>(1, MAX_BINARY_MESSAGE_LENGTH);
    FramerHelper<HEADER_FORMAT::PROPRIETARY_BINARY, STATUS::SUCCESS>(76, MAX_BINARY_MESSAGE_LENGTH);
}
