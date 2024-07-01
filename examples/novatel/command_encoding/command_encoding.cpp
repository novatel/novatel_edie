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
// ! \file command_encoding.cpp
// ===============================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include <novatel_edie/decoders/commander.hpp>
#include <novatel_edie/stream_interface/outputfilestream.hpp>

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

int main(int argc, char* argv[])
{
    // This example uses the default logger config, but you can also pass a config file to InitLogger()
    // Example config file: logger\example_logger_config.toml
    Logger::InitLogger();
    std::shared_ptr<spdlog::logger> logger = Logger::RegisterLogger("command_encoder");
    logger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(logger);
    Logger::AddRotatingFileLogger(logger);

    if (argc < 3)
    {
        logger->error("Format: command_encoding <path to Json DB> <output format> <abbreviated ascii command>\n");
        logger->error("Example: command_encoding database/messages_public.json ASCII \"RTKTIMEOUT 30\"\n");
        return 1;
    }

    // Json DB
    const fs::path pathJsonDb = argv[1];
    if (!fs::exists(pathJsonDb))
    {
        logger->error("File \"{}\" does not exist", pathJsonDb.string());
        return 1;
    }

    // Encode format
    std::string strEncodeFormat = argv[2];
    ENCODE_FORMAT eEncodeFormat = StringToEncodeFormat(strEncodeFormat);
    if (eEncodeFormat == ENCODE_FORMAT::UNSPECIFIED)
    {
        logger->error("Unsupported output format. Choose from:\n\tASCII\n\tBINARY");
        return 1;
    }

    JsonReader clJsonDb;
    auto tStart = std::chrono::high_resolution_clock::now();
    logger->info("Loading Database... ");
    clJsonDb.LoadFile(pathJsonDb.string());
    logger->info("Done in {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    Commander clCommander(&clJsonDb);

    char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    char* pcEncodedMessageBuffer = acEncodeBuffer;
    uint32_t uiEncodeBufferLength = MAX_ASCII_MESSAGE_LENGTH;

    logger->info("Converting \"{}\" to {}", argv[3], strEncodeFormat);
    STATUS eCommanderStatus =
        clCommander.Encode(argv[3], static_cast<uint32_t>(strlen(argv[3])), pcEncodedMessageBuffer, uiEncodeBufferLength, eEncodeFormat);
    if (eCommanderStatus != STATUS::SUCCESS)
    {
        logger->info("Failed to formulate a command ({})", static_cast<uint32_t>(eCommanderStatus));
        return -1;
    }

    std::string sOutFilename = std::string("COMMAND.").append(strEncodeFormat);
    OutputFileStream clOutputFs(sOutFilename.c_str());
    clOutputFs.WriteData(pcEncodedMessageBuffer, uiEncodeBufferLength);
    return 0;
}
