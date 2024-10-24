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
// ! \file range_decompressor.hpp
// ===============================================================================

#ifndef RANGE_DECOMPRESSOR_HPP
#define RANGE_DECOMPRESSOR_HPP

#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/filter.hpp"
#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"
#include "novatel_edie/decoders/oem/rangecmp/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class RangeDecompressor
//! \brief Decompresses Range logs depending on the Range version (E.g. 2/3/4).
//============================================================================
class RangeDecompressor
{
  public:
    RangeDecompressor(JsonReader* pclJsonDB_ = nullptr);

    void LoadJsonDb(JsonReader* pclJsonDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger() { return pclMyLogger; }

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_  The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) { pclMyLogger->set_level(eLevel_); }

    //----------------------------------------------------------------------------
    //! \brief Reset the decompressor to handle new datasets
    //----------------------------------------------------------------------------
    void Reset()
    {
        ammmMyRangeCmp2LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY)].clear();
        ammmMyRangeCmp2LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::SECONDARY)].clear();
        ammmMyRangeCmp4LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY)].clear();
        ammmMyRangeCmp4LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::SECONDARY)].clear();
    }

    [[nodiscard]] STATUS Decompress(unsigned char* pucBuffer_, uint32_t uiBufferSize_, MetaDataStruct& stMetaData_,
                                    ENCODE_FORMAT eFormat_ = ENCODE_FORMAT::UNSPECIFIED);

  protected:
    std::map<ChannelTrackingStatus::SATELLITE_SYSTEM, std::map<ChannelTrackingStatus::SIGNAL_TYPE, std::map<uint32_t, RangeCmp2LockTimeInfo>>>
        ammmMyRangeCmp2LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];
    std::map<ChannelTrackingStatus::SATELLITE_SYSTEM, std::map<ChannelTrackingStatus::SIGNAL_TYPE, std::map<uint32_t, RangeCmp4LocktimeInfo>>>
        ammmMyRangeCmp4LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

    template <typename T> T ExtractBitfield(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_, uint32_t uiBitsInBitfield_);

  private:
    Filter clMyRangeCmpFilter;
    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    JsonReader* pclMyMsgDB{};

    // Store the last primary reference blocks for each measurement source.
    RangeCmp4MeasurementSignalBlock astMyLastPrimaryReferenceBlocks[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

    // This is an array of map of map of maps. Indexed by SYSTEM, rangecmp4::SIGNAL_TYPE, then PRN
    // (uint32_t). This will store a header and its reference block for whenever we find
    // differential data for the System, Signal type and PRN. We must keep track of which
    // measurement source the reference block came from so any subsequent differential blocks are
    // correctly decompressed.
    std::map<SYSTEM,
             std::map<rangecmp4::SIGNAL_TYPE, std::map<uint32_t, std::pair<RangeCmp4MeasurementBlockHeader, RangeCmp4MeasurementSignalBlock>>>>
        ammmMyReferenceBlocks[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

    static double GetSignalWavelength(const ChannelTrackingStatus& stChannelStatus_, int16_t sGLONASSFrequency_);
    float GetRangeCmp2LockTime(const MetaDataStruct& stMetaData_, uint32_t uiLockTimeBits_, ChannelTrackingStatus::SATELLITE_SYSTEM eSystem_,
                               ChannelTrackingStatus::SIGNAL_TYPE eSignal_, uint16_t usPRN_);
    float GetRangeCmp4LockTime(const MetaDataStruct& stMetaData_, uint8_t ucLockTimeBits_, ChannelTrackingStatus::SATELLITE_SYSTEM eSystem_,
                               ChannelTrackingStatus::SIGNAL_TYPE eSignal_, uint32_t uiPRN_);
    template <bool bSecondary>
    void DecompressReferenceBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                  RangeCmp4MeasurementSignalBlock& stRefBlock_, MEASUREMENT_SOURCE eMeasurementSource_);
    template <bool bSecondary>
    void DecompressDifferentialBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                     RangeCmp4MeasurementSignalBlock& stDiffBlock_, const RangeCmp4MeasurementSignalBlock& stRefBlock_,
                                     double dSecondOffset_);
    void PopulateNextRangeData(RangeData& stRangeData_, const RangeCmp4MeasurementSignalBlock& stBlock_, const MetaDataStruct& stMetaData_,
                               const ChannelTrackingStatus& stChannelStatus_, uint32_t uiPRN_, char cGLONASSFrequencyNumber_);

    static void RangeCmpToRange(const RangeCmp& stRangeCmpMessage_, Range& stRangeMessage_);
    void RangeCmp2ToRange(const RangeCmp2& stRangeCmp2Message_, Range& stRangeMessage_, const MetaDataStruct& stMetaData_);
    void RangeCmp4ToRange(unsigned char* pucData_, Range& stRangeMessage_, const MetaDataStruct& pstMetaData_);
};

} // namespace novatel::edie::oem

#endif // RANGE_DECOMPRESSOR_HPP
