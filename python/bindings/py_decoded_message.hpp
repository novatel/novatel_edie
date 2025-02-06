#pragma once

#include "bindings_core.hpp"
#include "novatel_edie/decoders/oem/message_decoder.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {

struct PyGpsTime
{
    PyGpsTime() = default;
    PyGpsTime(uint16_t week_, double milliseconds_, TIME_STATUS time_status_) : week(week_), milliseconds(milliseconds_), time_status(time_status_) {}
    explicit PyGpsTime(const MetaDataStruct& meta_) : week(meta_.usWeek), milliseconds(meta_.dMilliseconds), time_status(meta_.eTimeStatus) {}

    uint16_t week{0};
    double milliseconds{0.0};
    TIME_STATUS time_status{TIME_STATUS::UNKNOWN};
};

struct PyMessageBody
{
    std::string name;
    explicit PyMessageBody(std::vector<FieldContainer> message_, PyMessageDatabase::ConstPtr parent_db_, std::string name);
    nb::dict& get_values() const;
    nb::dict& get_fields() const;
    nb::dict to_dict() const;
    nb::object getattr(nb::str field_name) const;
    nb::object getitem(nb::str field_name) const;
    bool contains(nb::str field_name) const;
    size_t len() const;
    std::string repr() const;

    std::vector<FieldContainer> fields;

  private:
    mutable nb::dict cached_values_;
    mutable nb::dict cached_fields_;

    PyMessageDatabase::ConstPtr parent_db_;
};

struct PyMessage
{
    nb::object message_body;
    nb::object header;
    std::string name;

    PyMessage(nb::object message_body_, nb::object header_, std::string name_)
        : message_body(message_body_), header(header_), name(name_) {}

    std::string repr() const
    {
        return "<" + name + " Message>";
    }
};



} // namespace novatel::edie::oem
