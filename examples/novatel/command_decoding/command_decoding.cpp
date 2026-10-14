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
// ! \file command_decoding.cpp
// ===============================================================================

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <novatel_edie/common/logger.hpp>
#include <novatel_edie/decoders/common/json_db_reader.hpp>
#include <novatel_edie/decoders/oem/command_decoder.hpp>
#include <novatel_edie/decoders/oem/encoder.hpp>

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

int main(int argc, char* argv[])
{
    // This example uses the default pclLogger config, but you can also pass a config file to InitLogger()
    // Example config file: pclLogger\example_logger_config.toml
    LOGGER_MANAGER->InitLogger();
    auto pclLogger = CREATE_LOGGER();
    pclLogger->set_level(spdlog::level::debug);
    LOGGER_MANAGER->AddConsoleLogging(pclLogger);
    LOGGER_MANAGER->AddRotatingFileLogger(pclLogger);

    if (argc < 4)
    {
        pclLogger->error("Format: command_decoding <path to Json DB> <output format> <abbreviated ascii command>\n");
        pclLogger->error("Example: command_decoding database/database.json ASCII \"LOG BESTPOSA\"\n");
        return 1;
    }

    // Json DB
    const fs::path pathJsonDb = argv[1];
    if (!fs::exists(pathJsonDb))
    {
        pclLogger->error("File \"{}\" does not exist", pathJsonDb.string());
        return 1;
    }

    // Encode format
    std::string strEncodeFormat = argv[2];
    ENCODE_FORMAT eEncodeFormat = StringToEncodeFormat(strEncodeFormat);
    if (eEncodeFormat != ENCODE_FORMAT::ASCII && eEncodeFormat != ENCODE_FORMAT::BINARY)
    {
        pclLogger->error("Unsupported output format. Choose from:\n\tASCII\n\tBINARY");
        return 1;
    }

    auto tStart = std::chrono::high_resolution_clock::now();
    pclLogger->info("Loading Database... ");
    MessageDatabase::Ptr clJsonDb = LoadJsonDbFile(pathJsonDb.string());
    pclLogger->info("Done in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    CommandDecoder clCommandDecoder(clJsonDb);
    Encoder clEncoder(clJsonDb);

    // Decode the command, filling in any parameters that were left out from the database defaults
    IntermediateHeader stHeader;
    CompositeField stCommand;
    STATUS eDecoderStatus = clCommandDecoder.Decode(argv[3], static_cast<uint32_t>(strlen(argv[3])), stHeader, stCommand);
    if (eDecoderStatus != STATUS::SUCCESS)
    {
        pclLogger->info("Failed to decode the command ({})", eDecoderStatus);
        return -1;
    }

    // The receiver's response to the command will have the same message ID
    pclLogger->info("Decoded \"{}\", the response will have message ID {}", argv[3], stHeader.usMessageId);

    unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucEncodeBuffer = acEncodeBuffer;
    MessageDataStruct stMessageData;

    pclLogger->info("Converting \"{}\" to {}", argv[3], strEncodeFormat);
    STATUS eEncoderStatus = clEncoder.Encode(&pucEncodeBuffer, sizeof(acEncodeBuffer), stHeader, stCommand, stMessageData, eEncodeFormat);
    if (eEncoderStatus != STATUS::SUCCESS)
    {
        pclLogger->info("Failed to encode the command ({})", eEncoderStatus);
        return -1;
    }

    std::string sOutFilename = std::string("COMMAND.").append(strEncodeFormat);
    std::ofstream ofs(sOutFilename.c_str(), std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);
    LOGGER_MANAGER->Shutdown();
    return 0;
}
