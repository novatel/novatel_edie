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
    RangeDecompressor(JsonReader::Ptr pclJsonDb_ = nullptr);

    void LoadJsonDb(JsonReader::Ptr pclJsonDb_);

    //----------------------------------------------------------------------------
    //! \brief Get the internal logger.
    //
    //! \return A shared_ptr to the spdlog::logger.
    //----------------------------------------------------------------------------
    std::shared_ptr<spdlog::logger> GetLogger();

    //----------------------------------------------------------------------------
    //! \brief Set the level of detail produced by the internal logger.
    //
    //! \param[in] eLevel_  The logging level to enable.
    //----------------------------------------------------------------------------
    void SetLoggerLevel(spdlog::level::level_enum eLevel_);

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

    [[nodiscard]] STATUS Decompress(unsigned char* pucRangeMessageBuffer_, uint32_t uiRangeMessageBufferSize_, MetaDataStruct& stMetaData_,
                                    ENCODE_FORMAT eFormat_ = ENCODE_FORMAT::UNSPECIFIED);

  private:
    Filter clMyRangeCmpFilter;
    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    JsonReader::Ptr pclMyMsgDb{nullptr};

    // Store the last primary reference blocks for each measurement source.
    RangeCmp4MeasurementSignalBlockStruct astMyLastPrimaryReferenceBlocks[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

    // TODO: Is there a better way to map all this information together?
    // This is an array of map of map of maps. Indexed by SYSTEM, RangeCmp4::SIGNAL_TYPE, then PRN
    // (uint32_t). This will store a header and its reference block for whenever we find
    // differential data for the System, Signal type and PRN. We must keep track of which
    // measurement source the reference block came from so any subsequent differential blocks are
    // correctly decompressed.
    std::map<SYSTEM, std::map<RangeCmp4::SIGNAL_TYPE,
                              std::map<uint32_t, std::pair<RangeCmp4MeasurementBlockHeaderStruct, RangeCmp4MeasurementSignalBlockStruct>>>>
        ammmMyReferenceBlocks[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

    // Protected members to be accessed by test child classes.
  protected:
    std::map<ChannelTrackingStatusStruct::SATELLITE_SYSTEM,
             std::map<ChannelTrackingStatusStruct::SIGNAL_TYPE, std::map<uint32_t, RangeCmp2LockTimeInfoStruct>>>
        ammmMyRangeCmp2LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];
    std::map<ChannelTrackingStatusStruct::SATELLITE_SYSTEM,
             std::map<ChannelTrackingStatusStruct::SIGNAL_TYPE, std::map<uint32_t, RangeCmp4LocktimeInfoStruct>>>
        ammmMyRangeCmp4LockTimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

  private:
    static double GetSignalWavelength(const ChannelTrackingStatusStruct& stChannelTrackingStatus_, int16_t sGLONASSFrequency_);
    double DetermineRangeCmp2ObservationLockTime(const MetaDataStruct& stMetaData_, uint32_t uiLockTimeBits_,
                                                 ChannelTrackingStatusStruct::SATELLITE_SYSTEM eSystem_,
                                                 ChannelTrackingStatusStruct::SIGNAL_TYPE eSignal_, uint16_t usPRN_);
    double DetermineRangeCmp4ObservationLockTime(const MetaDataStruct& stMetaData_, uint8_t ucLockTimeBits_,
                                                 ChannelTrackingStatusStruct::SATELLITE_SYSTEM eSystem_,
                                                 ChannelTrackingStatusStruct::SIGNAL_TYPE eSignal_, uint32_t uiPRN_);
    template <bool bIsSecondary>
    void DecompressReferenceBlock(uint8_t** ppucDataPointer_, RangeCmp4MeasurementSignalBlockStruct& stReferenceBlock_,
                                  MEASUREMENT_SOURCE eMeasurementSource_);
    template <bool bIsSecondary>
    void DecompressDifferentialBlock(uint8_t** ppucDataPointer_, RangeCmp4MeasurementSignalBlockStruct& stDifferentialBlock_,
                                     const RangeCmp4MeasurementSignalBlockStruct& stReferenceBlock_, double dSecondOffset_);
    void PopulateNextRangeData(RangeDataStruct& stRangeData_, const RangeCmp4MeasurementSignalBlockStruct& stBlock_,
                               const MetaDataStruct& stMetaData_, const ChannelTrackingStatusStruct& stChannelTrackingStatus_, uint32_t uiPRN_,
                               char cGLONASSFrequencyNumber_);

    static void RangeCmpToRange(const RangeCmpStruct& stRangeCmpMessage_, RangeStruct& stRangeMessage_);
    void RangeCmp2ToRange(const RangeCmp2Struct& stRangeCmp2Message_, RangeStruct& stRangeMessage_, const MetaDataStruct& stMetaData_);
    void RangeCmp4ToRange(uint8_t* pucCompressedData_, RangeStruct& stRangeMessage_, const MetaDataStruct& pstMetaData_);

    // Protected members to be accessed by test child classes.
  protected:
    uint32_t uiMyBytesRemaining{0U};
    uint32_t uiMyBitOffset{0U};
    uint64_t GetBitfieldFromBuffer(uint8_t** ppucDataBuffer_, uint32_t uiBitsInBitfield_);
};

} // namespace novatel::edie::oem

#endif // RANGE_DECOMPRESSOR_HPP
