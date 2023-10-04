#include "novatel_edie/decoders/oem/filter.hpp"

#include "bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_filter(nb::module_& m)
{
    nb::class_<oem::Filter>(m, "Filter")
        .def(nb::init<>())
        .def_prop_ro("logger", &oem::Filter::GetLogger)
        .def("set_lower_time_bound", &oem::Filter::SetIncludeLowerTimeBound, "week"_a, "seconds"_a)
        .def("set_upper_time_bound", &oem::Filter::SetIncludeUpperTimeBound, "week"_a, "seconds"_a)
        .def("invert_time_filter", &oem::Filter::InvertTimeFilter, "invert"_a)
        .def("set_include_decimation", &oem::Filter::SetIncludeDecimation, "period_sec"_a)
        .def("invert_decimation_filter", &oem::Filter::InvertDecimationFilter, "invert"_a)
        .def("include_time_status", nb::overload_cast<TIME_STATUS>(&oem::Filter::IncludeTimeStatus), "time_status"_a)
        .def("include_time_status", nb::overload_cast<std::vector<TIME_STATUS>>(&oem::Filter::IncludeTimeStatus), "time_statuses"_a)
        .def("invert_time_status_filter", &oem::Filter::InvertTimeStatusFilter, "invert"_a)
        .def("include_message_id", nb::overload_cast<uint32_t, novatel::edie::HEADER_FORMAT, MEASUREMENT_SOURCE>(&oem::Filter::IncludeMessageId),
             "id"_a, "format"_a = novatel::edie::HEADER_FORMAT::ALL, "source"_a = MEASUREMENT_SOURCE::PRIMARY)
        .def("include_message_id",
             nb::overload_cast<std::vector<std::tuple<uint32_t, novatel::edie::HEADER_FORMAT, MEASUREMENT_SOURCE>>&>(&oem::Filter::IncludeMessageId),
             "ids"_a)
        .def("invert_message_id_filter", &oem::Filter::InvertMessageIdFilter, "invert"_a)
        .def("include_message_name",
             nb::overload_cast<const std::string&, novatel::edie::HEADER_FORMAT, MEASUREMENT_SOURCE>(&oem::Filter::IncludeMessageName), "name"_a,
             "format"_a = novatel::edie::HEADER_FORMAT::ALL, "source"_a = MEASUREMENT_SOURCE::PRIMARY)
        .def("include_message_name",
             nb::overload_cast<std::vector<std::tuple<std::string, novatel::edie::HEADER_FORMAT, MEASUREMENT_SOURCE>>&>(
                 &oem::Filter::IncludeMessageName),
             "names"_a)
        .def("invert_message_name_filter", &oem::Filter::InvertMessageNameFilter, "invert"_a)
        .def("include_nmea_messages", &oem::Filter::IncludeNmeaMessages, "include"_a)
        .def("clear_filters", &oem::Filter::ClearFilters)
        .def("do_filtering", &oem::Filter::DoFiltering, "metadata"_a);
}
