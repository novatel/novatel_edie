
#ifndef RANGEDECOMPRESSOR_HPP
#define RANGEDECOMPRESSOR_HPP

#include <cmath>
#include <cstring>
#include <list>
#include <vector>

#include "decoders/common/api/crc32.hpp"
#include "decoders/common/api/message_decoder.hpp"
#include "decoders/novatel/api/encoder.hpp"
#include "decoders/novatel/api/filter.hpp"
#include "decoders/novatel/api/framer.hpp"
#include "decoders/novatel/api/header_decoder.hpp"
#include "decoders/novatel/api/rangecmp/common.hpp"

namespace novatel::edie::oem {

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
        ammmMyRangeCmp2Locktimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY)].clear();
        ammmMyRangeCmp2Locktimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::SECONDARY)].clear();
        ammmMyRangeCmp4Locktimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::PRIMARY)].clear();
        ammmMyRangeCmp4Locktimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::SECONDARY)].clear();
    };

    //----------------------------------------------------------------------------
    //! \brief Shutdown the internal logger.
    //----------------------------------------------------------------------------
    void ShutdownLogger();

    [[nodiscard]] STATUS Decompress(unsigned char* pucRangeMessageBuffer_, uint32_t uiRangeMessageBufferSize_, MetaDataStruct& stMetaData_,
                                    ENCODEFORMAT eFormat_ = ENCODEFORMAT::UNSPECIFIED);

  private:
    Filter clMyRangeCmpFilter;
    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    JsonReader* pclMyMsgDB;

    // Store the last primary reference blocks for each measurement source.
    RangeCmp4MeasurementSignalBlockStruct astMyLastPrimaryReferenceBlocks[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

    // TODO: Is there a better way to map all this information together?
    // This is an array of map of map of maps.  Indexed by SYSTEM, RangeCmp4::SIGNAL_TYPE, then PRN
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
             std::map<ChannelTrackingStatusStruct::SIGNAL_TYPE, std::map<uint32_t, RangeCmp2LocktimeInfoStruct>>>
        ammmMyRangeCmp2Locktimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];
    std::map<ChannelTrackingStatusStruct::SATELLITE_SYSTEM,
             std::map<ChannelTrackingStatusStruct::SIGNAL_TYPE, std::map<uint32_t, RangeCmp4LocktimeInfoStruct>>>
        ammmMyRangeCmp4Locktimes[static_cast<uint32_t>(MEASUREMENT_SOURCE::MAX)];

  private:
    double GetSignalWavelength(const ChannelTrackingStatusStruct& stChannelTrackingStatus_, int16_t sGLONASSFrequency_);
    float DetermineRangeCmp2ObservationLocktime(const MetaDataStruct& stMetaData_, uint32_t uiLocktimeBits_,
                                                ChannelTrackingStatusStruct::SATELLITE_SYSTEM eSystem_,
                                                ChannelTrackingStatusStruct::SIGNAL_TYPE eSignal_, uint16_t usPRN_);
    float DetermineRangeCmp4ObservationLocktime(const MetaDataStruct& stMetaData_, uint8_t ucLocktimeBits_,
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

    void RangeCmpToRange(const RangeCmpStruct& stRangeCmpMessage_, RangeStruct& stRangeMessage_);
    void RangeCmp2ToRange(const RangeCmp2Struct& stRangeCmp2Message_, RangeStruct& stRangeMessage_, const MetaDataStruct& stMetaData_);
    void RangeCmp4ToRange(uint8_t* pucCompressedData_, RangeStruct& stRangeMessage_, const MetaDataStruct& pstMetaData_);

    // Protected members to be accessed by test child classes.
  protected:
    uint32_t uiMyBytesRemaining{0U};
    uint32_t uiMyBitOffset{0U};
    uint64_t GetBitfieldFromBuffer(uint8_t** ppucDataBuffer_, uint32_t uiBitsInBitfield_);
};

} // namespace novatel::edie::oem

#endif // RANGEDECOMPRESSOR_HPP
