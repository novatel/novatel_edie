////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2021 NovAtel Inc.
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
////////////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file rxconfig_example.cpp
//! \brief Demonstrate how to use the C++ source for converting RXCONFIG
//! messages.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include <cstdio>
#include <cstdlib>

#include <chrono>

#include "src/decoders/common/api/common.hpp"
#include "src/decoders/novatel/api/rxconfig/rxconfig_handler.hpp"
#include "src/hw_interface/stream_interface/api/inputfilestream.hpp"
#include "src/hw_interface/stream_interface/api/outputfilestream.hpp"
#include "src/version.h"

using namespace std;
using namespace novatel::edie;
using namespace novatel::edie::oem;

inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

int main(int argc, char* argv[])
{
    // This example uses the default logger config, but you can also pass a config file to InitLogger()
    // Example config file: logger\example_logger_config.toml
    Logger::InitLogger();
    std::shared_ptr<spdlog::logger> pclLogger = Logger::RegisterLogger("rxconfig_converter");
    pclLogger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(pclLogger);
    Logger::AddRotatingFileLogger(pclLogger);

    // Get command line arguments
    pclLogger->info("Decoder library information:\n{}", caPrettyPrint);

    std::string sEncodeFormat = "ASCII";
    if (argc == 2 && strcmp(argv[1], "-V") == 0) { return 0; }
    if (argc < 3)
    {
        pclLogger->error("ERROR: Need to specify a JSON message definitions DB, an input file and an output "
                         "format.");
        pclLogger->error("Example: converter.exe <path to Json DB> <path to input file> <output format>");
        return 1;
    }
    if (argc == 4) { sEncodeFormat = argv[3]; }

    // Check command line arguments
    std::string sJsonDB = argv[1];
    if (!file_exists(sJsonDB))
    {
        pclLogger->error("File \"{}\" does not exist", sJsonDB);
        return 1;
    }

    std::string sInFilename = argv[2];
    if (!file_exists(sInFilename))
    {
        pclLogger->error("File \"{}\" does not exist", sInFilename);
        return 1;
    }

    ENCODEFORMAT eEncodeFormat = StringToEncodeFormat(sEncodeFormat);
    if (eEncodeFormat == ENCODEFORMAT::UNSPECIFIED)
    {
        pclLogger->error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY");
        return 1;
    }

    // Load the database
    JsonReader clJsonDb;
    pclLogger->info("Loading Database...");
    auto tStart = chrono::high_resolution_clock::now();
    clJsonDb.LoadFile(sJsonDB);
    pclLogger->info("Done in {}ms", chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - tStart).count());

    // Initialize FS structures and buffers
    StreamReadStatus stReadStatus;
    ReadDataStructure stReadData;
    unsigned char acIFSReadBuffer[MAX_ASCII_MESSAGE_LENGTH];
    stReadData.cData = reinterpret_cast<char*>(acIFSReadBuffer);
    stReadData.uiDataSize = sizeof(acIFSReadBuffer);

    // Setup filestreams
    InputFileStream clIFS(sInFilename.c_str());
    OutputFileStream clConvertedRxConfigOFS((sInFilename + std::string(".").append(sEncodeFormat)).c_str());
    OutputFileStream clStrippedRxConfigOFS((sInFilename + std::string(".STRIPPED.").append(sEncodeFormat)).c_str());

    MetaDataStruct stMetaData;
    MetaDataStruct stEmbeddedMetaData;
    MessageDataStruct stMessageData;
    MessageDataStruct stEmbeddedMessageData;

    RxConfigHandler clRxConfigHandler(&clJsonDb);
    STATUS eStatus = STATUS::UNKNOWN;

    while (!stReadStatus.bEOS)
    {
        stReadData.cData = reinterpret_cast<char*>(acIFSReadBuffer);
        stReadStatus = clIFS.ReadData(stReadData);
        clRxConfigHandler.Write(reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);

        do {
            eStatus = clRxConfigHandler.Convert(stMessageData, stMetaData, stEmbeddedMessageData, stEmbeddedMetaData, eEncodeFormat);
            if (eStatus == STATUS::SUCCESS)
            {
                stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                pclLogger->info("Encoded: ({}) {}", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                clConvertedRxConfigOFS.WriteData(reinterpret_cast<char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);

                // Make the embedded message valid by flipping the CRC.
                if (eEncodeFormat == ENCODEFORMAT::ASCII)
                {
                    // Flip the CRC at the end of the embedded message and add a CRLF so it becomes
                    // a valid command.
                    auto* pcCRCBegin =
                        reinterpret_cast<char*>((stEmbeddedMessageData.pucMessage + stEmbeddedMessageData.uiMessageLength) - OEM4_ASCII_CRC_LENGTH);
                    uint32_t uiFlippedCRC = strtoul(pcCRCBegin, nullptr, 16) ^ 0xFFFFFFFF;
                    snprintf(pcCRCBegin, OEM4_ASCII_CRC_LENGTH + 1, "%08x", uiFlippedCRC);
                    clStrippedRxConfigOFS.WriteData(reinterpret_cast<char*>(stEmbeddedMessageData.pucMessage), stEmbeddedMessageData.uiMessageLength);
                    clStrippedRxConfigOFS.WriteData(const_cast<char*>("\r\n"), 2);
                }
                else if (eEncodeFormat == ENCODEFORMAT::BINARY)
                {
                    // Flip the CRC at the end of the embedded message so it becomes a valid
                    // command.
                    auto* puiCRCBegin = reinterpret_cast<uint32_t*>((stEmbeddedMessageData.pucMessage + stEmbeddedMessageData.uiMessageLength) -
                                                                    OEM4_BINARY_CRC_LENGTH);
                    *puiCRCBegin ^= 0xFFFFFFFF;
                    clStrippedRxConfigOFS.WriteData(reinterpret_cast<char*>(stEmbeddedMessageData.pucMessage), stEmbeddedMessageData.uiMessageLength);
                }
                else if (eEncodeFormat == ENCODEFORMAT::JSON)
                {
                    // Write in a comma and CRLF to make the files parse-able by JSON readers.
                    clConvertedRxConfigOFS.WriteData(const_cast<char*>(",\r\n"), 3);
                    clStrippedRxConfigOFS.WriteData(const_cast<char*>(",\r\n"), 3);
                }
            }
        } while (eStatus != STATUS::BUFFER_EMPTY);
    }

    Logger::Shutdown();
    return 0;
}
