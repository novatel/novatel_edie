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
// ! \file range_decompressor_example.cpp
// ===============================================================================

#include "novatel_edie/decoders/rangecmp/range_decompressor.hpp"

#include <chrono>
#include <filesystem>

#include <novatel_edie/common/message_decoder.hpp>
#include <novatel_edie/decoders/encoder.hpp>
#include <novatel_edie/decoders/filter.hpp>
#include <novatel_edie/decoders/framer.hpp>
#include <novatel_edie/decoders/header_decoder.hpp>
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
    std::shared_ptr<spdlog::logger> pclLogger = Logger::RegisterLogger("range_decompressor");
    pclLogger->set_level(spdlog::level::debug);
    Logger::AddConsoleLogging(pclLogger);
    Logger::AddRotatingFileLogger(pclLogger);

    if (argc == 2 && strcmp(argv[1], "-V") == 0)
    {
        pclLogger->info("Decoder library information:\n{}", caPrettyPrint);
        return 0;
    }

    std::string sEncodeFormat = "ASCII";

    if (argc < 3)
    {
        pclLogger->info("ERROR: Need to specify a JSON message definitions DB, an input file and an output format.\n");
        pclLogger->info("Example: converter <path to Json DB> <path to input file> <output format>\n");
        return -1;
    }
    if (argc == 4) { sEncodeFormat = argv[3]; }

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
        return -1;
    }

    pclLogger->info("Decoder library information:\n{}", caPrettyPrint);

    JsonReader clJsonDb;
    pclLogger->info("Loading Database... ");
    auto tStart = std::chrono::high_resolution_clock::now();
    clJsonDb.LoadFile(pathJsonDb.string());
    pclLogger->info("DONE ({}ms)", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tStart).count());

    Framer clFramer;
    HeaderDecoder clHeaderDecoder(&clJsonDb);
    MessageDecoder clMessageDecoder(&clJsonDb);
    RangeDecompressor clRangeDecompressor(&clJsonDb);
    Encoder clEncoder(&clJsonDb);

    clFramer.SetLoggerLevel(spdlog::level::debug);
    clHeaderDecoder.SetLoggerLevel(spdlog::level::debug);
    clMessageDecoder.SetLoggerLevel(spdlog::level::debug);
    clEncoder.SetLoggerLevel(spdlog::level::debug);
    Logger::AddConsoleLogging(clFramer.GetLogger());
    Logger::AddConsoleLogging(clHeaderDecoder.GetLogger());
    Logger::AddConsoleLogging(clMessageDecoder.GetLogger());
    Logger::AddRotatingFileLogger(clFramer.GetLogger());
    Logger::AddRotatingFileLogger(clHeaderDecoder.GetLogger());
    Logger::AddRotatingFileLogger(clMessageDecoder.GetLogger());

    clFramer.SetFrameJson(false);
    clFramer.SetPayloadOnly(false);
    clFramer.SetReportUnknownBytes(true);

    unsigned char acFileStreamBuffer[MAX_ASCII_MESSAGE_LENGTH];
    ReadDataStructure stReadData;
    stReadData.cData = reinterpret_cast<char*>(acFileStreamBuffer);
    stReadData.uiDataSize = sizeof(acFileStreamBuffer);

    unsigned char acFrameBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char acEncodeBuffer[MAX_ASCII_MESSAGE_LENGTH];
    unsigned char* pucEncodedMessageBuffer = acEncodeBuffer;

    InputFileStream clIfs(pathInFilename.string().c_str());
    OutputFileStream clOfs(pathInFilename.string().append(".DECOMPRESSED.").append(sEncodeFormat).c_str());
    StreamReadStatus stReadStatus;

    auto eStatus = STATUS::UNKNOWN;

    IntermediateHeader stHeader;
    std::vector<FieldContainer> stMessage;

    MetaDataStruct stMetaData;
    MessageDataStruct stMessageData;

    auto start = std::chrono::system_clock::now();
    uint32_t uiCompletedMessages = 0;
    while (true)
    {
        unsigned char* pucReadBuffer = acFrameBuffer;

        // Get frame, null-terminate.
        eStatus = clFramer.GetFrame(pucReadBuffer, MAX_ASCII_MESSAGE_LENGTH, stMetaData);
        if (eStatus == STATUS::SUCCESS)
        {
            // Decode the header.  Get metadata here and populate the Intermediate header.
            eStatus = clHeaderDecoder.Decode(pucReadBuffer, stHeader, stMetaData);
            if (eStatus == STATUS::SUCCESS)
            {
                eStatus = clRangeDecompressor.Decompress(pucReadBuffer, MAX_ASCII_MESSAGE_LENGTH, stMetaData, eEncodeFormat);
                if (eStatus == STATUS::SUCCESS)
                {
                    uiCompletedMessages++;
                    uint32_t uiBytesWritten = clOfs.WriteData(reinterpret_cast<char*>(pucReadBuffer), stMetaData.uiLength);
                    if (stMetaData.uiLength == uiBytesWritten)
                    {
                        pucReadBuffer[stMetaData.uiLength] = '\0';
                        pclLogger->info("Decompressed: ({}) {}", stMetaData.uiLength, reinterpret_cast<char*>(pucReadBuffer));
                    }
                    else { pclLogger->error("Could only write {}/{} bytes.", uiBytesWritten, stMessageData.uiMessageLength); }
                }
                else if (eStatus == STATUS::UNSUPPORTED)
                {
                    if (eStatus == STATUS::SUCCESS)
                    {
                        stHeader.usMessageId = stMetaData.usMessageId;
                        eStatus = clMessageDecoder.Decode((pucReadBuffer + stMetaData.uiHeaderLength), stMessage, stMetaData);
                        if (eStatus == STATUS::SUCCESS)
                        {
                            // Encode our message now that we have everything we need.
                            eStatus = clEncoder.Encode(&pucEncodedMessageBuffer, MAX_ASCII_MESSAGE_LENGTH, stHeader, stMessage, stMessageData,
                                                       stMetaData, eEncodeFormat);
                            if (eStatus == STATUS::SUCCESS)
                            {
                                stMessageData.pucMessage[stMessageData.uiMessageLength] = '\0';
                                pclLogger->info("Encoded: ({}) {}", stMessageData.uiMessageLength, reinterpret_cast<char*>(stMessageData.pucMessage));
                            }
                        }
                    }
                }
            }
        }
        else if (eStatus == STATUS::BUFFER_EMPTY || eStatus == STATUS::INCOMPLETE)
        {
            // Read from file, write to framer.
            stReadStatus = clIfs.ReadData(stReadData);
            if (stReadStatus.uiCurrentStreamRead == 0)
            {
                pclLogger->info("Stream finished");
                break;
            }

            clFramer.Write(reinterpret_cast<unsigned char*>(stReadData.cData), stReadStatus.uiCurrentStreamRead);
        }
    }

    std::chrono::duration<double> elapsedSeconds = std::chrono::system_clock::now() - start;
    pclLogger->info("Decoded {} messages in {}s. ({}msg/s)", uiCompletedMessages, elapsedSeconds.count(),
                    uiCompletedMessages / elapsedSeconds.count());
    Logger::Shutdown();
    return 0;
}
