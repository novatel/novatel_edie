#include "novatel_edie/decoders/oem/framer.hpp"

#include "py_common/bindings_core.hpp"

namespace nb = nanobind;

namespace novatel::edie::oem {
class PyFramer : public Framer
{
  public:
    PyFramer() : Framer() {};

    bool GetFrameJson() { return bMyFrameJson; }

    bool GetPayloadOnly() { return bMyPayloadOnly; }

    bool GetReportUnknownBytes() { return bMyReportUnknownBytes; }

    nb::tuple PyGetFrame(uint32_t buffer_size);

    nb::tuple PyIterGetFrame();
};
}
