////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file proprietarytest.hpp
//! \brief Unit tests for proprietary NovAtel logs.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <gtest/gtest.h>

#include "decoders/novatel/api/framer.hpp"
#include "hw_interface/stream_interface/api/inputfilestream.hpp"

using namespace std;
using namespace novatel::edie;
using namespace novatel::edie::oem;

class ProprietaryFramerTest : public ::testing::Test
{
  protected:
    static std::unique_ptr<Framer> pclMyFramer;
    static std::unique_ptr<InputFileStream> pclMyIFS;
    static std::unique_ptr<unsigned char[]> pucMyTestFrameBuffer;

    // Per-test-suite setup
    static void SetUpTestSuite()
    {
        pclMyFramer = std::make_unique<Framer>();
        pclMyFramer->SetReportUnknownBytes(true);
        pclMyFramer->SetPayloadOnly(false);
        pucMyTestFrameBuffer = std::make_unique<unsigned char[]>(131071); // 128kB
    }

    // Per-test-suite teardown
    static void TearDownTestSuite() { pclMyFramer->ShutdownLogger(); }

    // Per-test setup
    void SetUp() { FlushFramer(); }

    // Per-test teardown
    void TearDown() { FlushFramer(); }

  public:
    void WriteFileStreamToFramer(std::string sFilename_)
    {
        pclMyIFS = std::make_unique<InputFileStream>((std::filesystem::path(std::getenv("TEST_RESOURCE_PATH")) / sFilename_).string().c_str());

        StreamReadStatus stReadStatus;
        ReadDataStructure stReadData;
        stReadData.cData = new char[MAX_ASCII_MESSAGE_LENGTH];
        stReadData.uiDataSize = MAX_ASCII_MESSAGE_LENGTH;
        uint32_t uiBytesWritten = 0;

        while (!stReadStatus.bEOS)
        {
            stReadStatus = pclMyIFS->ReadData(stReadData);
            uiBytesWritten = pclMyFramer->Write(reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);
            ASSERT_EQ(uiBytesWritten, stReadStatus.uiCurrentStreamRead);
        }

        delete[] stReadData.cData;
        stReadData.cData = nullptr;
    }

    void WriteBytesToFramer(unsigned char* pucBytes_, uint32_t uiNumBytes_) { ASSERT_EQ(pclMyFramer->Write(pucBytes_, uiNumBytes_), uiNumBytes_); }

    void FlushFramer()
    {
        uint32_t uiBytes = 0;
        do {
            uiBytes = pclMyFramer->Flush(pucMyTestFrameBuffer.get(), MAX_ASCII_MESSAGE_LENGTH);
        } while (uiBytes > 0);
    }
};

std::unique_ptr<Framer> ProprietaryFramerTest::pclMyFramer = nullptr;
std::unique_ptr<InputFileStream> ProprietaryFramerTest::pclMyIFS = nullptr;
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

    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.uiLength = 12;
    stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
    ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    stExpectedMetaData.uiLength = 76;
    stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
    ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_INCOMPLETE)
{
    // "<incomplete binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19,
                         0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5,
                         0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A};
    WriteBytesToFramer(aucData, sizeof(aucData));

    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.uiLength = 59;
    stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
    ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_SYNC_ERROR)
{
    WriteFileStreamToFramer("proprietary_binary_sync_error.BIN");

    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.uiLength = MAX_BINARY_MESSAGE_LENGTH;
    stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
    ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_BAD_CRC)
{
    // "<encrypted binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xFF};
    WriteBytesToFramer(aucData, sizeof(aucData));

    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.uiLength = 30; // Unknown bytes up to 0x24 ('$') should be returned (NMEA sync was found mid-log)
    stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;
    ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_RUN_ON_CRC)
{
    // "<encrypted binary bestpos log>FF"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7, 0x19,
                         0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5,
                         0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87, 0x8F, 0x8A, 0x35,
                         0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC, 0xFF};
    WriteBytesToFramer(aucData, sizeof(aucData));

    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.uiLength = 76;
    stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
    ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_INADEQUATE_BUFFER)
{
    // "<encrypted binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    WriteBytesToFramer(aucData, sizeof(aucData));

    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.uiLength = 76;
    stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
    ASSERT_EQ(STATUS::BUFFER_FULL, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), 38, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), 76, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_BYTE_BY_BYTE)
{
    // "<binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    uint32_t uiLogSize = sizeof(aucData);
    uint32_t uiRemainingBytes = uiLogSize;

    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

    while (true)
    {
        WriteBytesToFramer(&aucData[uiLogSize - uiRemainingBytes], 1);
        uiRemainingBytes--;
        stExpectedMetaData.uiLength = uiLogSize - uiRemainingBytes;

        if (stExpectedMetaData.uiLength == OEM4_BINARY_SYNC_LENGTH - 1) { stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY; }

        if (uiRemainingBytes > 0)
        {
            ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
            ASSERT_EQ(stTestMetaData, stExpectedMetaData);
        }
        else { break; }
    }
    stExpectedMetaData.uiLength = uiLogSize;
    ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_SEGMENTED)
{
    // "<binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    uint32_t uiLogSize = sizeof(aucData);
    uint32_t uiBytesWritten = 0;
    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;

    WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_BINARY_SYNC_LENGTH);
    uiBytesWritten += OEM4_BINARY_SYNC_LENGTH;
    stExpectedMetaData.uiLength = uiBytesWritten;
    ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    WriteBytesToFramer(&aucData[uiBytesWritten], (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH));
    uiBytesWritten += (OEM4_BINARY_HEADER_LENGTH - OEM4_BINARY_SYNC_LENGTH);
    stExpectedMetaData.uiLength = uiBytesWritten;
    ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    WriteBytesToFramer(&aucData[uiBytesWritten], 44);
    uiBytesWritten += 44;
    stExpectedMetaData.uiLength = uiBytesWritten;
    ASSERT_EQ(STATUS::INCOMPLETE, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    WriteBytesToFramer(&aucData[uiBytesWritten], OEM4_BINARY_CRC_LENGTH);
    uiBytesWritten += OEM4_BINARY_CRC_LENGTH;
    stExpectedMetaData.uiLength = uiBytesWritten;
    ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
    ASSERT_EQ(uiLogSize, uiBytesWritten);
}

TEST_F(ProprietaryFramerTest, PROPRIETARY_BINARY_TRICK)
{
    // "<binary syncs><binary sync + half header><binary sync byte 1><binary bestpos log>"
    uint8_t aucData[] = {0xAA, 0x45, 0x12, 0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0xAA,
                         0xAA, 0x45, 0x12, 0x1C, 0x3A, 0x09, 0x00, 0xE0, 0x2C, 0x00, 0x00, 0x00, 0xB8, 0xB4, 0x82, 0x08, 0xF8, 0xC6, 0xC7,
                         0x19, 0x00, 0x00, 0x00, 0x02, 0x01, 0xA3, 0x00, 0x41, 0x00, 0x00, 0x24, 0x00, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59,
                         0xE3, 0xF5, 0x14, 0xDC, 0x79, 0xB4, 0x16, 0xE9, 0xFA, 0x4C, 0xBF, 0x34, 0x0E, 0xD8, 0xCF, 0x59, 0xE3, 0xF5, 0x87,
                         0x8F, 0x8A, 0x35, 0xFF, 0xB1, 0x94, 0x64, 0x6B, 0xA4, 0xBD, 0xA8, 0x6C, 0x27, 0x91, 0x27, 0x6F, 0x8E, 0x0B, 0xCC};
    uint32_t uiLogSize = sizeof(aucData);
    MetaDataStruct stExpectedMetaData, stTestMetaData;
    stExpectedMetaData.eFormat = HEADERFORMAT::UNKNOWN;

    WriteBytesToFramer(aucData, uiLogSize);
    stExpectedMetaData.uiLength = 3;
    ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    stExpectedMetaData.uiLength = 15;
    ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    stExpectedMetaData.uiLength = 1;
    ASSERT_EQ(STATUS::UNKNOWN, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);

    stExpectedMetaData.uiLength = 76;
    stExpectedMetaData.eFormat = HEADERFORMAT::PROPRIETARY_BINARY;
    ASSERT_EQ(STATUS::SUCCESS, pclMyFramer->GetFrame(pucMyTestFrameBuffer.get(), MAX_BINARY_MESSAGE_LENGTH, stTestMetaData));
    ASSERT_EQ(stTestMetaData, stExpectedMetaData);
}
