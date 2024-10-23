#include "novatel_edie/common/nexcept.hpp"

#include "bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;

void init_common_nexcept(nb::module_&)
{
    nb::register_exception_translator([](const std::exception_ptr& p, void*) {
        try
        {
            std::rethrow_exception(p);
        }
        catch (const NExcept& e)
        {
            PyErr_SetString(PyExc_OSError, e.buffer);
        }
    });
}
