#include "bindings_core.hpp"
#include "novatel_edie/decoders/common/framer_manager.hpp"

namespace nb = nanobind;

namespace novatel::edie {
class PyFramerManager : public FramerManager
{
  public:
    PyFramerManager(const std::vector<std::string> selectedFramers) : FramerManager(selectedFramers) {};

    std::unique_ptr<PyFramerManager> MakeFramerManagerFromList(nb::list py_selected_framers);

    nb::tuple PyGetFrame(uint32_t buffer_size);
};
} // namespace novatel::edie
