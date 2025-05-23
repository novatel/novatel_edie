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
#include "novatel_edie/decoders/oem/rangecmp/channel_tracking_status.hpp"
#include "novatel_edie/decoders/oem/rangecmp/common.hpp"

namespace novatel::edie::oem {

//============================================================================
//! \class RangeDecompressor
//! \brief Decompresses Range logs depending on the Range version (E.g. 2/3/4).
//============================================================================
class RangeDecompressor
{
  public:
    //! Default constructor.
    RangeDecompressor(MessageDatabase::Ptr pclJsonDb_ = nullptr);

    //! Load the JSON database for the decompressor.
    void LoadJsonDb(MessageDatabase::Ptr pclJsonDb_);

    //! Get the internal logger.
    std::shared_ptr<spdlog::logger> GetLogger() { return pclMyLogger; }

    //! Set the level of detail produced by the internal logger.
    void SetLoggerLevel(spdlog::level::level_enum eLevel_) { pclMyLogger->set_level(eLevel_); }

    //! Reset the decompressor to handle new datasets.
    void Reset()
    {
        for (auto& it : mMyRangeCmp2LockTimes) { it.second = {}; }
        for (auto& it : mMyRangeCmp4LockTimes) { it.second = {}; }
    }

    //! Decompresses a RANGECMP message provided in a buffer and overwrites it with the equivalent RANGE message.
    [[nodiscard]] STATUS Decompress(unsigned char* pucBuffer_, uint32_t uiBufferSize_, MetaDataStruct& stMetaData_,
                                    ENCODE_FORMAT eFormat_ = ENCODE_FORMAT::UNSPECIFIED);

  private:
    //! Get the lock time for a RANGECMP2 message.
    double GetRangeCmp2LockTime(const MetaDataStruct& stMetaData_, uint32_t uiLockTimeBits_, uint64_t key_);

    //! Get the lock time for a RANGECMP4 message.
    double GetRangeCmp4LockTime(const MetaDataStruct& stMetaData_, uint8_t ucLockTimeBits_, uint64_t key_);

    //! Decompresses a RANGECMP4 reference measurement block and populates the reference block struct.
    template <bool Secondary>
    void DecompressReferenceBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                  rangecmp4::MeasurementSignalBlock& stRefBlock_, double primaryPsr, double primaryDoppler) const;

    //! Decompresses a RANGECMP4 differential measurement block and populates the provided reference block struct.
    template <bool Secondary>
    void DecompressDifferentialBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_,
                                     rangecmp4::MeasurementSignalBlock& stDiffBlock_, const rangecmp4::MeasurementSignalBlock& stRefBlock_,
                                     double dSecondOffset_) const;

    //! Populates a provided RangeData structure from the RANGECMP4 blocks provided.
    void PopulateNextRangeData(RangeData& stRangeData_, const rangecmp4::MeasurementSignalBlock& stBlock_, const MetaDataStruct& stMetaData_,
                               const ChannelTrackingStatus& stCtStatus_, uint32_t uiPrn_, char cGlonassFrequencyNumber_);

    //! Populates a provided RangeData structure from the RANGECMP5 blocks provided.
    void PopulateNextRangeData(RangeData& stRangeData_, const rangecmp5::MeasurementSignalBlock& stBlock_, const MetaDataStruct& stMetaData_,
                               const ChannelTrackingStatus& stCtStatus_, uint32_t uiPrn_, char cGlonassFrequencyNumber_);

    //! Decompresses a RANGECMP5 reference measurement block and populates the reference block struct.
    template <bool Secondary>
    void DecompressBlock(unsigned char** ppucData_, uint32_t& uiBytesLeft_, uint32_t& uiBitOffset_, rangecmp5::MeasurementSignalBlock& stBlock_,
                         double primaryPsr, double primaryDoppler) const;

    //! Convert a RANGECMP message into RANGE message.
    static void RangeCmpToRange(const rangecmp::RangeCmp& stRangeCmpMessage_, Range& stRangeMessage_);

    //! Convert a RANGECMP2 message into RANGE message.
    void RangeCmp2ToRange(const rangecmp2::RangeCmp& stRangeCmpMessage_, Range& stRangeMessage_, const MetaDataStruct& stMetaData_);

    //! Convert a RANGECMP4 message into RANGE message.
    void RangeCmp4ToRange(unsigned char* pucData_, Range& stRangeMessage_, const MetaDataStruct& pstMetaData_);

    //! Convert a RANGECMP5 message into RANGE message.
    void RangeCmp5ToRange(unsigned char* pucData_, Range& stRangeMessage_, const MetaDataStruct& pstMetaData_);

    Filter clMyRangeCmpFilter;
    HeaderDecoder clMyHeaderDecoder;
    MessageDecoder clMyMessageDecoder;
    Encoder clMyEncoder;

    std::shared_ptr<spdlog::logger> pclMyLogger;
    MessageDatabase::Ptr pclMyMsgDB{nullptr};

    std::unordered_map<uint64_t, rangecmp2::LockTimeInfo> mMyRangeCmp2LockTimes;
    std::unordered_map<uint64_t, rangecmp4::LockTimeInfo> mMyRangeCmp4LockTimes;
    std::unordered_map<uint64_t, std::pair<rangecmp4::MeasurementBlockHeader, rangecmp4::MeasurementSignalBlock>> mMyReferenceBlocks;
};

} // namespace novatel::edie::oem

#endif // RANGE_DECOMPRESSOR_HPP
