#pragma once

#include <nanobind/stl/optional.h>

#include "bindings_core.hpp"
#include "py_message_objects.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {
class PyFilter : public Filter
{
  public:
    // -------------------------------------------------------------------------------------------------------
    void PySetIncludeLowerTimeBound(PyGpsTime* value)
    {
        if (value == nullptr) { ClearLowerTimeBound(); }
        else { SetIncludeLowerTimeBound(value->week, value->milliseconds / 1000.0); }
    }
    void PySetIncludeUpperTimeBound(PyGpsTime* value)
    {
        if (value == nullptr) { ClearUpperTimeBound(); }
        else { SetIncludeUpperTimeBound(value->week, value->milliseconds / 1000.0); }
    }

    nb::object PyGetIncludeLowerTimeBound()
    {
        if (bMyFilterLowerTime) { return nb::cast(PyGpsTime(uiMyLowerWeek, uiMyLowerMSec)); }
        else { return nb::none(); }
    }
    nb::object PyGetIncludeUpperTimeBound()
    {
        if (bMyFilterUpperTime) { return nb::cast(PyGpsTime(uiMyUpperWeek, uiMyUpperMSec)); }
        else { return nb::none(); }
    }

    bool PyTimeFilterInverted() { return bMyInvertTimeFilter; }

    // -------------------------------------------------------------------------------------------------------
    void PySetIncludeDecimation(std::optional<uint32_t> period_)
    {
        if (period_.has_value()) { SetIncludeDecimationMs(period_.value()); }
        else { ClearDecimationFilter(); }
    }
    nb::object PyGetIncludeDecimation()
    {
        if (bMyDecimate) { return nb::cast(uiMyDecimationPeriodMilliSec); }
        else { return nb::none(); }
    }
    bool PyDecimationFilterInverted() { return bMyInvertDecimation; }

    // -------------------------------------------------------------------------------------------------------
    std::vector<TIME_STATUS> PyGetTimeStatuses() { return vMyTimeStatusFilters; }
    bool PyTimeStatusFilterInverted() { return bMyInvertTimeStatusFilter; }

    // -------------------------------------------------------------------------------------------------------
    std::vector<std::tuple<uint32_t, HEADER_FORMAT, MEASUREMENT_SOURCE>> PyGetMessageIds() { return vMyMessageIdFilters; }
    bool PyMessageIdFilterInverted() { return bMyInvertMessageIdFilter; }

    // -------------------------------------------------------------------------------------------------------
    std::vector<std::tuple<std::string, HEADER_FORMAT, MEASUREMENT_SOURCE>> PyGetMessageNames() { return vMyMessageNameFilters; }
    bool PyMessageNameFilterInverted() { return bMyInvertMessageNameFilter; }

  public:
    using Ptr = std::shared_ptr<PyFilter>;
    using ConstPtr = std::shared_ptr<const PyFilter>;
};
} // namespace novatel::edie::oem
