#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace nb = nanobind;
using namespace novatel::edie;

struct PyGpsTime
{
    PyGpsTime() = default;
    PyGpsTime(uint16_t week_, double milliseconds_, TIME_STATUS time_status_) : week(week_), milliseconds(milliseconds_), time_status(time_status_) {}
    PyGpsTime(const oem::MetaDataStruct& meta_) : week(meta_.usWeek), milliseconds(meta_.dMilliseconds), time_status(meta_.eTimeStatus) {}

    uint16_t week{0};
    double milliseconds{0.0};
    TIME_STATUS time_status{TIME_STATUS::UNKNOWN};
};

struct PyDecodedMessage
{
    explicit PyDecodedMessage(std::vector<FieldContainer> message_, const oem::MetaDataStruct& meta_);
    nb::dict& get_values() const;
    nb::dict& get_fields() const;
    nb::dict to_dict() const;
    nb::object getattr(nb::str field_name) const;
    nb::object getitem(nb::str field_name) const;
    bool contains(nb::str field_name) const;
    size_t len() const;
    std::string repr() const;

    std::vector<FieldContainer> fields;

    // MetaDataStruct with all fields that are no longer relevant after decoding dropped.
    uint16_t message_id;
    uint32_t message_crc;
    std::string message_name;
    PyGpsTime time;
    MEASUREMENT_SOURCE measurement_source{MEASUREMENT_SOURCE::PRIMARY};
    oem::CONSTELLATION constellation{oem::CONSTELLATION::UNKNOWN};

  private:
    mutable nb::dict cached_values_;
    mutable nb::dict cached_fields_;
};
