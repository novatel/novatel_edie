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
// ! \file converter_components.cpp
// ===============================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include <novatel_edie/common/json_reader.hpp>
#include <novatel_edie/common/logger.hpp>
#include <novatel_edie/decoders/encoder.hpp>
#include <novatel_edie/decoders/filter.hpp>
#include <novatel_edie/decoders/framer.hpp>
#include <novatel_edie/decoders/header_decoder.hpp>
#include <novatel_edie/decoders/message_decoder.hpp>
#include <novatel_edie/stream_interface/inputfilestream.hpp>
#include <novatel_edie/stream_interface/outputfilestream.hpp>
#include <novatel_edie/version.h>

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

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

    // Set up the EDIE components
    Framer clFramer;
    clFramer.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clFramer.GetLogger());
    Logger::AddRotatingFileLogger(clFramer.GetLogger());
    clFramer.SetReportUnknownBytes(true);
    clFramer.SetPayloadOnly(false);
    clFramer.SetFrameJson(false);

    HeaderDecoder clHeaderDecoder(&clJsonDb);
    clHeaderDecoder.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clHeaderDecoder.GetLogger());
    Logger::AddRotatingFileLogger(clHeaderDecoder.GetLogger());

    MessageDecoder clMessageDecoder(&clJsonDb);
    clMessageDecoder.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clMessageDecoder.GetLogger());
    Logger::AddRotatingFileLogger(clMessageDecoder.GetLogger());

    Encoder clEncoder(&clJsonDb);
    clEncoder.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clEncoder.GetLogger());
    Logger::AddRotatingFileLogger(clEncoder.GetLogger());

    Filter clFilter;
    clFilter.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clFilter.GetLogger());
    Logger::AddRotatingFileLogger(clFilter.GetLogger());

    // Set up buffers
    unsigned char acFrameBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucEncodedMessageBuffer = acEncodeBuffer;

    // Initialize structures and error codes
    auto eFramerStatus = STATUS::UNKNOWN;
    auto eDecoderStatus = STATUS::UNKNOWN;
    auto eEncoderStatus = STATUS::UNKNOWN;

    IntermediateHeader stHeader;
    std::vector<FieldContainer> stMessage;

    MetaDataStruct stMetaData;
    MessageDataStruct stMessageData;

    // Initialize FS structures and buffers
    StreamReadStatus stReadStatus;
    ReadDataStructure stReadData;
    unsigned char acIfsReadBuffer[MAX_ASCII_MESSAGE_LENGTH];
    stReadData.cData = reinterpret_cast<char*>(acIfsReadBuffer);
    stReadData.uiDataSize = sizeof(acIfsReadBuffer);

    // Setup file streams
    InputFileStream clIfs(pathInFilename.string().c_str());
    OutputFileStream clConvertedLogsOfs(pathInFilename.string().append(".").append(sEncodeFormat).c_str());
    OutputFileStream clUnknownBytesOfs(pathInFilename.string().append(".").append(sEncodeFormat).append(".UNKNOWN").c_str());

    tStart = std::chrono::high_resolution_clock::now();

    while (!stReadStatus.bEOS)
    {
        stReadStatus = clIfs.ReadData(stReadData);
        clFramer.Write(reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);
        // Clearing INCOMPLETE status when internal buffer needs more bytes.
        eFramerStatus = STATUS::INCOMPLETE_MORE_DATA;

        while (eFramerStatus != STATUS::BUFFER_EMPTY && eFramerStatus != STATUS::INCOMPLETE)
        {
            unsigned char* pucFrameBuffer = acFrameBuffer;
            eFramerStatus = clFramer.GetFrame(pucFrameBuffer, sizeof(acFrameBuffer), stMetaData);

            if (eFramerStatus == STATUS::SUCCESS)
            {
                if (stMetaData.bResponse)
                {
                    clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                    continue;
                }

                pucFrameBuffer[stMetaData.uiLength] = '\0';
                pclLogger->info("Framed: {}", reinterpret_cast<char*>(pucFrameBuffer));

                // Decode the header.  Get metadata here and populate the Intermediate header.
                eDecoderStatus = clHeaderDecoder.Decode(pucFrameBuffer, stHeader, stMetaData);

                if (eDecoderStatus == STATUS::SUCCESS)
                {
                    // Filter the log, pass over this log if we don't want it.
                    if (!clFilter.DoFiltering(stMetaData)) { continue; }

                    pucFrameBuffer += stMetaData.uiHeaderLength;
                    uint32_t uiBodyLength = stMetaData.uiLength - stMetaData.uiHeaderLength;
                    // Decode the Log, pass the metadata and populate the intermediate log.
                    eDecoderStatus = clMessageDecoder.Decode(pucFrameBuffer, stMessage, stMetaData);

                    if (eDecoderStatus == STATUS::SUCCESS)
                    {
                        eEncoderStatus = clEncoder.Encode(&pucEncodedMessageBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData,
                                                          stMetaData, eEncodeFormat);

                        if (eEncoderStatus == STATUS::SUCCESS)
                        {
                            clConvertedLogsOfs.WriteData(reinterpret_cast<char*>(stMessageData.pucMessage), stMessageData.uiMessageLength);
                            stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                            pclLogger->info("Encoded: ({}) {}", stMessageData.uiMessageLength, reinterpret_cast<char*>(pucEncodedMessageBuffer));
                        }
                        else
                        {
                            clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), uiBodyLength);
                            pclLogger->warn("Encoder returned with status code {}", static_cast<int>(eEncoderStatus));
                        }
                    }
                    else
                    {
                        clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), uiBodyLength);
                        pclLogger->warn("MessageDecoder returned with status code {}", static_cast<int>(eDecoderStatus));
                    }
                }
                else
                {
                    clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength);
                    pclLogger->warn("HeaderDecoder returned with status code {}", static_cast<int>(eDecoderStatus));
                }
            }
            else if (eFramerStatus == STATUS::UNKNOWN) { clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(pucFrameBuffer), stMetaData.uiLength); }
            else { pclLogger->warn("Framer returned with status code {}", static_cast<int>(eFramerStatus)); }
        }
    }

    // Clean up
    uint32_t uiBytes = clFramer.Flush(acFrameBuffer, sizeof(acFrameBuffer));
    clUnknownBytesOfs.WriteData(reinterpret_cast<char*>(acFrameBuffer), uiBytes);

    Logger::Shutdown();
    return 0;
}
