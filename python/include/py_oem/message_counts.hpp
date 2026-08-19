#pragma once

#include <cstdint>
#include <map>
#include <tuple>

#include "novatel_edie/decoders/oem/header_decoder.hpp"
#include "py_common/bindings_core.hpp"

namespace novatel::edie::py_oem {

//! \brief The Python view of the message counts of a decoder.
//! \details The key is a tuple of (message ID, format, source). Filter.message_ids
//! uses the same tuple, so a caller can compare the two directly. The C++ key of
//! MessageCountsTracker holds the same three values in another order. A std::map
//! keeps its keys sorted, which gives the Python dictionary a stable order.
using PyMessageCountsMap = std::map<std::tuple<uint32_t, HEADER_FORMAT, uint8_t>, uint64_t>;

//! \brief Convert the counts of a decoder into the Python view of those counts.
inline PyMessageCountsMap CreatePyMessageCounts(const oem::HeaderDecoder::MessageCountsMap& counts_)
{
    PyMessageCountsMap mapPyCounts;
    for (const auto& [key, count] : counts_) { mapPyCounts[{std::get<1>(key), std::get<0>(key), std::get<2>(key)}] = count; }
    return mapPyCounts;
}

} // namespace novatel::edie::py_oem
