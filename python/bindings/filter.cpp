#include "novatel_edie/decoders/oem/filter.hpp"

#include "bindings_core.hpp"
#include "filter.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void init_novatel_filter(nb::module_& m)
{
    nb::class_<oem::PyFilter>(m, "Filter")
        .def(nb::init(), "Initializes a filter with the default configuration which all messages.")
        .def_prop_rw("include_responses", &oem::PyFilter::PyResponsesIncluded, &oem::PyFilter::IncludeResponses, nb::arg("value").none(),
                     "Whether to include response messages.") // signature in stubgen_pattern.txt
        .def_prop_rw("include_non_responses", &oem::PyFilter::PyNonResponsesIncluded, &oem::PyFilter::IncludeNonResponses, nb::arg("value").none(),
                     "Whether to include non-response (regular) messages.") // signature in stubgen_pattern.txt
        .def_prop_rw("lower_time_bound", &oem::PyFilter::PyGetIncludeLowerTimeBound, &oem::PyFilter::PySetIncludeLowerTimeBound,
                     nb::arg("value").none(),
                     "The earliest time that messages can have without being filtered-out.") // signature in stubgen_pattern.txt
        .def_prop_rw("upper_time_bound", &oem::PyFilter::PyGetIncludeUpperTimeBound, &oem::PyFilter::PySetIncludeUpperTimeBound,
                     nb::arg("value").none(),
                     "The latest time that messages can have without being filtered-out.") // signature in stubgen_pattern.txt
        .def_prop_rw("time_bounds_inverted", &oem::PyFilter::PyTimeFilterInverted, &oem::PyFilter::InvertTimeFilter, "value"_a,
                     R"doc(
                        Whether the upper and lower time bounds should be inverted. 

                        If both are set, only messages outside of the range will be included.
        )doc")
        .def_prop_rw(
            "decimation_period", &oem::PyFilter::PyGetIncludeDecimation, &oem::PyFilter::PySetIncludeDecimation, nb::arg("value").none(),
            "Only messages whose time in milliseconds is cleanly divisible by this number will be included.") // signature in stubgen_pattern.txt
        .def_prop_rw("decimation_period_inverted", &oem::PyFilter::PyDecimationFilterInverted, &oem::PyFilter::InvertDecimationFilter,
                     "Whether to invert which messages are filtered by the decimation period.")
        .def_prop_ro("time_statuses", &oem::PyFilter::PyGetTimeStatuses,
                     R"doc(
                        The set of time statues to filter on. 

                        If None, messages with any time status will be included.
         )doc")
        .def_prop_rw("time_statuses_excluded", &oem::PyFilter::PyTimeStatusFilterInverted, &oem::PyFilter::InvertTimeStatusFilter, "value"_a,
                     R"doc(
                        Whether to exclude messages from the set of filtered time_statues. 

                        Otherwise only they will be included.
        )doc")
        .def("add_time_status", nb::overload_cast<TIME_STATUS>(&oem::PyFilter::IncludeTimeStatus), "time_status"_a,
             R"doc(
                Adds a new time status to the set to filter on.
                
                Args:
                    time_status: The additional time status to filter on.
        )doc")
        .def("extend_time_statuses", nb::overload_cast<std::vector<TIME_STATUS>>(&oem::PyFilter::IncludeTimeStatus), "time_statuses"_a,
             R"doc(
                Extends the set of time statuses to filter on.
                
                Args:
                    time_statuses: The additional times status to filter on.
        )doc")
        .def("remove_time_status", &oem::PyFilter::RemoveTimeStatus, "time_status"_a,
             R"doc(
                Removes a time status from the set to filter on.
                
                Args:
                    time_statuses: The time status to remove.
        )doc")
        .def("clear_time_statuses", &oem::PyFilter::ClearTimeStatuses, "Clears all time status filters.")
        .def_prop_ro("message_ids", &oem::PyFilter::PyGetMessageIds, "The set of message IDs to filter on.")
        .def_prop_rw("message_ids_excluded", &oem::PyFilter::PyMessageIdFilterInverted, &oem::PyFilter::InvertMessageIdFilter, "value"_a,
                     R"doc(
                        Whether to exclude messages from the set of filtered ids. 

                        Otherwise only they will be included.
        )doc")
        .def("add_message_id", nb::overload_cast<uint32_t, HEADER_FORMAT, uint8_t>(&oem::PyFilter::IncludeMessageId), "id"_a,
             "format"_a = HEADER_FORMAT::ALL, "source"_a = NULL_SIBLING_ID,
             R"doc(
                Adds a new message ID to the set to filter on.
                
                Args:
                    id: The message ID to filter on.
                    format: The message format it applies to. Defaults to all.
                    source: The antenna source it applies to. Defaults to primary.
        )doc")
        .def("extend_message_ids", nb::overload_cast<std::vector<std::tuple<uint32_t, HEADER_FORMAT, uint8_t>>&>(&oem::PyFilter::IncludeMessageId),
             "ids"_a,
             R"doc(
                Extends the set of message ids to filter on.
                
                Args:
                    time_statuses: A sequence of ids, formats, and sources.
        )doc")
        .def("remove_message_id", &oem::PyFilter::RemoveMessageId, "id"_a, "format"_a, "source"_a,
             R"doc(
                Removes a message ID from the set to filter on.
                
                Args:
                    id: The message ID to remove.
                    format: Which format to remove it for.
                    source: Which source to remove if for.
        )doc")
        .def("clear_message_ids", &oem::PyFilter::ClearMessageIds, "Clears all message ID filters.")
        .def_prop_ro("message_names", &oem::PyFilter::PyGetMessageNames, "The set of message names to filter on.")
        .def_prop_rw("message_names_excluded", &oem::PyFilter::PyMessageNameFilterInverted, &oem::PyFilter::InvertMessageNameFilter, "value"_a,
                     R"doc(
                        Whether to exclude messages from the set of filtered names. 

                        Otherwise only they will be included.
        )doc")
        .def("add_message_name", nb::overload_cast<std::string_view, HEADER_FORMAT, uint8_t>(&oem::PyFilter::IncludeMessageName), "name"_a,
             "format"_a = HEADER_FORMAT::ALL, "source"_a = NULL_SIBLING_ID,
             R"doc(
                Adds a new message name to the set to filter on.
                
                Args:
                    id: The message name to filter on.
                    format: The message format it applies to. Defaults to all.
                    source: The antenna source it applies to. Defaults to primary.
        )doc")
        .def("extend_message_names",
             nb::overload_cast<std::vector<std::tuple<std::string, HEADER_FORMAT, uint8_t>>&>(&oem::PyFilter::IncludeMessageName),
             "names"_a,
             R"doc(
                Extends the set of message name to filter on.
                
                Args:
                    time_statuses: A sequence of names, formats, and sources.
        )doc")
        .def("remove_message_name", &oem::PyFilter::RemoveMessageName, "name"_a, "format"_a, "source"_a,
             R"doc(
                Removes a message name from the set to filter on.
                
                Args:
                    id: The message name to remove.
                    format: Which format to remove it for.
                    source: Which source to remove if for.
        )doc")
        .def("clear_message_names", &oem::PyFilter::ClearMessageNames, "Clears all message name filters.")
        .def("reset", &oem::PyFilter::ClearFilters, "Reset the filter to the default configuration of allowing all messages.")
        .def("do_filtering", &oem::PyFilter::DoFiltering, "metadata"_a, R"doc(
            Determines whether a message should be filtered.

            Args:
                metadata: The metadata associated with a particular message.

            Returns:
                True if the message can be included, False if it should be filtered-out.
        )doc")
        .def(
            "__repr__",
            [](nb::object self) {
                std::vector<const char*> main_fields = {"lower_time_bound",  "upper_time_bound", "time_statuses",
                                                        "decimation_period", "message_ids",      "message_names"};
                nb::list field_reprs;
                for (const auto& field : main_fields)
                {
                    nb::object value = nb::getattr(self, field);
                    nb::bool_ is_set = nb::bool_(value);
                    if (bool(is_set)) { field_reprs.append(nb::str("{0}={1}").format(field, value)); }
                }
                if (field_reprs.size() == 0) { return nb::str("Filter()"); }
                nb::str field_repr = nb::str(", ");
                field_repr = nb::str(field_repr.attr("join")(field_reprs));
                return nb::str("Filter({0})").format(field_repr);
            },
            R"doc(
                Creates a string representation based on active filters.
            
                Returns: The string representation of the filter.
            )doc");
}
