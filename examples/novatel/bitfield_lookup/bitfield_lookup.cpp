// ===============================================================================
// |                                                                             |
// |  COPYRIGHT NovAtel Inc, 2026. All rights reserved.                          |
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
// ! \file bitfield_lookup.cpp
// ===============================================================================

#include <cstring>
#include <filesystem>
#include <stdexcept>

#include <novatel_edie/common/logger.hpp>
#include <novatel_edie/decoders/common/json_db_reader.hpp>
#include <novatel_edie/decoders/oem/header_decoder.hpp>
#include <novatel_edie/decoders/oem/message_decoder.hpp>

namespace fs = std::filesystem;

using namespace novatel::edie;
using namespace novatel::edie::oem;

// A single framed BESTPOS log. ext_sol_stat is the 0x02 near the end.
constexpr char acBestPos[] =
    "#BESTPOSA,COM1,0,83.5,FINESTEERING,2163,329760.000,02400000,b1f6,65535;SOL_COMPUTED,SINGLE,51.15043874397,-114.03066788586,"
    "1097.6822,-17.0000,WGS84,1.3648,1.1806,3.1112,\"\",0.000,0.000,18,18,18,0,00,02,11,01*c3194e35\r\n";

// Decode a single framed log into its message body. Throws if the header or body fails to decode.
CompositeField DecodeLog(const HeaderDecoder& clHeaderDecoder_, const MessageDecoder& clMessageDecoder_, const char* pcLog_)
{
    const auto* pucLog = reinterpret_cast<const unsigned char*>(pcLog_);
    MetaDataStruct stMetaData;
    stMetaData.uiLength = static_cast<uint32_t>(std::strlen(pcLog_));

    IntermediateHeader stHeader;
    if (clHeaderDecoder_.Decode(pucLog, stHeader, stMetaData) != STATUS::SUCCESS) { throw std::runtime_error("Failed to decode header"); }

    CompositeField stMessage;
    if (clMessageDecoder_.Decode(pucLog + stMetaData.uiHeaderLength, stMessage, stMetaData) != STATUS::SUCCESS)
    {
        throw std::runtime_error("Failed to decode message");
    }
    return stMessage;
}

int main(int argc, char* argv[])
{
    LOGGER_MANAGER->InitLogger();
    auto pclLogger = CREATE_LOGGER();
    LOGGER_MANAGER->AddConsoleLogging(pclLogger);

    if (argc < 2)
    {
        pclLogger->error("Usage: bitfield_lookup <path to Json DB>");
        return 1;
    }

    const fs::path pathJsonDb = argv[1];
    if (!fs::exists(pathJsonDb))
    {
        pclLogger->error("File \"{}\" does not exist", pathJsonDb.string());
        return 1;
    }

    MessageDatabase::Ptr clJsonDb = LoadJsonDbFile(pathJsonDb.string());

    // Look up ext_sol_stat and its bitmask entry in the database.
    const MessageDefinition::ConstPtr pclBestPosDef = clJsonDb->GetMsgDef("BESTPOS");
    const FieldInfo& clBestPosFields = pclBestPosDef->GetMsgDefFromCrc(pclBestPosDef->latestMessageCrc);
    const BaseField::ConstPtr pclExtSolStat = clBestPosFields.GetFieldDefByName("ext_sol_stat");
    const auto& [pclPsrInnoCorrectionEnum, stPsrInnoCorrectionMask] = pclExtSolStat->bitMasks->masks.at("pseudorange_inno_correction");

    // Decode the log
    const HeaderDecoder clHeaderDecoder(clJsonDb);
    const MessageDecoder clMessageDecoder(clJsonDb);
    const CompositeField stMessage = DecodeLog(clHeaderDecoder, clMessageDecoder, acBestPos);

    // Read ext_sol_stat as a plain integer. It is a UCHAR.
    const auto ucExtSolStat = stMessage.GetFieldValueByName<uint8_t>("ext_sol_stat");
    // Apply the mask.
    const uint32_t uiPsrInnoCorrection = ExtractMaskedValue(stPsrInnoCorrectionMask, ucExtSolStat);
    // Get a human readable representation.
    const std::string_view svPsrInnoCorrection = pclPsrInnoCorrectionEnum->valueName.at(uiPsrInnoCorrection);

    pclLogger->info("ext_sol_stat = 0x{:02X}", ucExtSolStat);
    pclLogger->info("pseudorange_inno_correction = {} ({})", uiPsrInnoCorrection, svPsrInnoCorrection);

    LOGGER_MANAGER->Shutdown();
    return 0;
}
