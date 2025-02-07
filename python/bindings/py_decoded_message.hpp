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

//============================================================================
//! \class PyField
//! \brief A python representation for a log header.
//============================================================================
struct PyHeader : public IntermediateHeader
{
    nb::dict to_dict() const;
};

//============================================================================
//! \class PyField
//! \brief A python representation for a single log message or message field.
//! 
//! Contains a vector of FieldContainer objects, which behave like attributes 
//! within the Python API.
//============================================================================
struct PyField
{
    std::string name;
    explicit PyField(std::vector<FieldContainer> message_, PyMessageDatabase::ConstPtr parent_db_, std::string name_);
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


//============================================================================
//! \class PyMessage
//! \brief A python representation for a single log message.
//! 
//! Extends PyField with reference to the Python represenation of a Header.
//============================================================================
struct PyMessage : public PyField
{
  public:
    PyHeader header;

    PyMessage(std::vector<FieldContainer> fields_, PyMessageDatabase::ConstPtr parent_db_, std::string name_, PyHeader header_)
        : PyField(std::move(fields_), std::move(parent_db_), std::move(name_)), header(std::move(header_)) {}
};

} // namespace novatel::edie::oem
