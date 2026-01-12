#include "bindings_core.hpp"
#include "novatel_edie/decoders/common/framer_manager.hpp"

namespace nb = nanobind;

namespace novatel::edie {
class PyFramerManager : public FramerManager
{
  public:
    PyFramerManager(const std::vector<std::string> selectedFramers) : FramerManager(selectedFramers) {};

    bool GetReportUnknownBytes() { return bMyReportUnknownBytes; }

    nb::tuple PyGetFrame(uint32_t buffer_size);

    nb::tuple PyIterGetFrame();
};
} // namespace novatel::edie
