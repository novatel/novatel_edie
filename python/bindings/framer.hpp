#include "novatel_edie/decoders/oem/framer.hpp"

#include "bindings_core.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {
class PyFramer : public Framer
{
  public:
    PyFramer() : Framer() {};

    nb::tuple PyGetFrame(uint32_t buffer_size);

    nb::tuple PyIterGetFrame();
};
}
