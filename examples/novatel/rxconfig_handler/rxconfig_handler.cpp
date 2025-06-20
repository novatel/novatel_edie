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
// ! \file rxconfig_example.cpp
// ===============================================================================

#include <chrono>
#include <filesystem>
#include <iostream>

#include <novatel_edie/common/logger.hpp>
#include <novatel_edie/decoders/common/common.hpp>
#include <novatel_edie/decoders/common/json_db_reader.hpp>
#include <novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp>
#include <novatel_edie/version.h>

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

int main(int argc, char* argv[])
{
    // This example uses the default logger config, but you can also pass a config file to InitLogger()
    // Example config file: logger\example_logger_config.toml
    LOGGER_MANAGER->InitLogger();
    auto pclLogger = CREATE_LOGGER();
    pclLogger->set_level(spdlog::level::debug);
    LOGGER_MANAGER->AddConsoleLogging(pclLogger);
    LOGGER_MANAGER->AddRotatingFileLogger(pclLogger);

    // Get command line arguments
    pclLogger->info("Decoder library information:\n{}", caPrettyPrint);

    std::string sEncodeFormat = "ASCII";
    if (argc == 2 && strcmp(argv[1], "-V") == 0) { return 0; }
    if (argc < 3)
    {
        pclLogger->error("ERROR: Need to specify a JSON message definitions DB, an input file and an output format.");
        pclLogger->error("Example: converter <path to Json DB> <path to input file> <output format>");
        return 1;
    }
    if (argc == 4) { sEncodeFormat = argv[3]; }

    // Check command line arguments
    const fs::path pathJsonDb = argv[1];
    if (!fs::exists(pathJsonDb))
    {
        pclLogger->error("File \"{}\" does not exist", pathJsonDb.string());
        return 1;
    }
    const fs::path pathInFilename = argv[2];
    if (!fs::exists(pathInFilename))
    {
        pclLogger->error("File \"{}\" does not exist", pathInFilename.string());
        return 1;
    }

    ENCODE_FORMAT eEncodeFormat = StringToEncodeFormat(sEncodeFormat);
    if (eEncodeFormat == ENCODE_FORMAT::UNSPECIFIED)
    {
        pclLogger->error("Unspecified output format.\n\tASCII\n\tBINARY\n\tFLATTENED_BINARY");
        return 1;
    }

    // Load the database
    pclLogger->info("Loading Database...");
    auto tStart = std::chrono::high_resolution_clock::now();
    MessageDatabase::Ptr clJsonDb = LoadJsonDbFile(pathJsonDb.string());
    pclLogger->info("Done in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    // Set up file streams
    std::ifstream ifs(pathInFilename, std::ios::binary);
    std::ofstream convertedOfs(pathInFilename.string() + "." + sEncodeFormat, std::ios::binary);
    std::ofstream strippedOfs(pathInFilename.string() + ".STRIPPED." + sEncodeFormat, std::ios::binary);

    RxConfigHandler clRxConfigHandler(clJsonDb);

    while (!ifs.eof())
    {
        std::array<char, MAX_ASCII_MESSAGE_LENGTH> cData;
        ifs.read(cData.data(), cData.size());
        clRxConfigHandler.Write(reinterpret_cast<unsigned char*>(cData.data()), ifs.gcount());

        MetaDataStruct stMetaData;
        MetaDataStruct stEmbeddedMetaData;
        MessageDataStruct stMessageData;
        MessageDataStruct stEmbeddedMessageData;

        STATUS eStatus = clRxConfigHandler.Convert(stMessageData, stMetaData, stEmbeddedMessageData, stEmbeddedMetaData, eEncodeFormat);

        while (eStatus != STATUS::BUFFER_EMPTY)
        {
            if (eStatus == STATUS::SUCCESS)
            {
                stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                pclLogger->info("Encoded: ({}) {}", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                convertedOfs.write(reinterpret_cast<char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);

                // Make the embedded message valid by flipping the CRC.
                if (eEncodeFormat == ENCODE_FORMAT::ASCII)
                {
                    // Flip the CRC at the end of the embedded message and add a CRLF, so it becomes a valid command.
                    auto* pcCrcBegin =
                        reinterpret_cast<char*>((stEmbeddedMessageData.pucMessage + stEmbeddedMessageData.uiMessageLength) - OEM4_ASCII_CRC_LENGTH);
                    uint32_t uiFlippedCrc = strtoul(pcCrcBegin, nullptr, 16) ^ 0xFFFFFFFF;
                    snprintf(pcCrcBegin, OEM4_ASCII_CRC_LENGTH + 1, "%08x", uiFlippedCrc);
                    strippedOfs.write(reinterpret_cast<char*>(stEmbeddedMessageData.pucMessage), stEmbeddedMessageData.uiMessageLength);
                    strippedOfs.write("\r\n", 2);
                }
                else if (eEncodeFormat == ENCODE_FORMAT::BINARY)
                {
                    // Flip the CRC at the end of the embedded message, so it becomes a valid command.
                    auto* puiCrcBegin = reinterpret_cast<uint32_t*>((stEmbeddedMessageData.pucMessage + stEmbeddedMessageData.uiMessageLength) -
                                                                    OEM4_BINARY_CRC_LENGTH);
                    *puiCrcBegin ^= 0xFFFFFFFF;
                    strippedOfs.write(reinterpret_cast<char*>(stEmbeddedMessageData.pucMessage), stEmbeddedMessageData.uiMessageLength);
                }
                else if (eEncodeFormat == ENCODE_FORMAT::JSON)
                {
                    // Write in a comma and CRLF to make the files parse-able by JSON readers.
                    convertedOfs.write(",\r\n", 3);
                    strippedOfs.write(",\r\n", 3);
                }
            }

            eStatus = clRxConfigHandler.Convert(stMessageData, stMetaData, stEmbeddedMessageData, stEmbeddedMetaData, eEncodeFormat);
        }
    }

    LOGGER_MANAGER->Shutdown();
    return 0;
}
