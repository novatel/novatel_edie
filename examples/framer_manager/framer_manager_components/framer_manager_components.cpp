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
// ! \file framer_manager_components.cpp
// ===============================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include <decoders/common/api/json_reader.hpp>
#include <decoders/common/api/logger.hpp>
#include <decoders/novatel/api/encoder.hpp>
#include <decoders/novatel/api/filter.hpp>
#include <decoders/novatel/api/framer.hpp>
#include <decoders/novatel/api/header_decoder.hpp>
#include <decoders/novatel/api/message_decoder.hpp>
#include <framer_manager/api/framer_manager.hpp>
#include <hw_interface/stream_interface/api/inputfilestream.hpp>
#include <hw_interface/stream_interface/api/outputfilestream.hpp>
#include <version.h>

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

constexpr uint32_t MAX_BUFFER_SIZE = 4096;

int main(int argc, char* argv[])
{
    // This example uses the default logger config, but you can also pass a config file to InitLogger()
    // Example config file: logger\example_logger_config.toml
    Logger::InitLogger();
    std::shared_ptr<spdlog::logger> pclLogger = Logger::RegisterLogger("converter");
    pclLogger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(pclLogger);
    Logger::AddRotatingFileLogger(pclLogger);

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
    JsonReader clJsonDb;
    pclLogger->info("Loading Database...");
    auto tStart = std::chrono::high_resolution_clock::now();
    clJsonDb.LoadFile(pathJsonDb.string());
    pclLogger->info("Done in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    clJsonDb.LoadFile(pathJsonDb.string());
    pclLogger->info("Done in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    FramerManager clFramerManager("framer_manager");
    clFramerManager.SetLoggerLevel(spdlog::level::debug);

    auto eFramerStatus = STATUS::UNKNOWN;

    unsigned char acFrameBuffer[MAX_ASCII_MESSAGE_LENGTH];

    StreamReadStatus stReadStatus;
    ReadDataStructure stReadData;

    unsigned char* acIfsReadBuffer[MAX_BUFFER_SIZE];
    stReadData.cData = reinterpret_cast<char*>(acIfsReadBuffer);
    stReadData.uiDataSize = sizeof(acIfsReadBuffer);

    InputFileStream clIfs(pathInFilename.string().c_str());
    OutputFileStream clConvertedLogsOfs((pathInFilename.string() + ".FRAMED").c_str());
    OutputFileStream clUnknownBytesOfs((pathInFilename.string() + ".UNKNOWN").c_str());

    // TODO: Move FRAMER_ID into public framer managerinterface
    // auto eActiveFramerId = FRAMER_ID::UNKNOWN;

    while (!stReadStatus.bEOS)
    {
        stReadStatus = clIfs.ReadData(stReadData);

        clFramerManager.Write(reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);

        eFramerStatus = STATUS::INCOMPLETE_MORE_DATA;
        while (eFramerStatus != STATUS::BUFFER_EMPTY && eFramerStatus != STATUS::INCOMPLETE)
        {
            unsigned char* pucFrameBuffer = acFrameBuffer;
            // TODO: Move FRAMER_ID into public framer manager interface
            // eFramerStatus = clInternalFramerManager.GetFrame(pucFrameBuffer, sizeof(acFrameBuffer), eActiveFramerId);
            eFramerStatus = clFramerManager.GetFrame(pucFrameBuffer, sizeof(acFrameBuffer));

            if (eFramerStatus == STATUS::SUCCESS)
            {
                if (clFramerManager.novatelMetaDataStruct.bResponse)
                {
                    clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), clFramerManager.novatelMetaDataStruct.uiLength);
                    continue;
                }
                pucFrameBuffer[clFramerManager.novatelMetaDataStruct.uiLength] = '\0';
                pclLogger->info("Framed: {}", reinterpret_cast<char*>(pucFrameBuffer));
            }
            else if (eFramerStatus == STATUS::UNKNOWN)
            {
                clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), clFramerManager.novatelMetaDataStruct.uiLength);
            }
            else if (eFramerStatus == STATUS::SYNC_BYTES_FOUND) { continue; }
            else { pclLogger->warn("Framer returned with status code {}", static_cast<int>(eFramerStatus)); }
        }
    }
}