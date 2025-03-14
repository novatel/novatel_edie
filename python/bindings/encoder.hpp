#include "novatel_edie/decoders/oem/encoder.hpp"

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "py_message_objects.hpp"
#include "py_message_data.hpp"


namespace nb = nanobind;

namespace novatel::edie::oem {
    
class PyEncoder : public Encoder
{
  public:
    PyEncoder(PyMessageDatabase* pclMessageDb_) : Encoder(pclMessageDb_) {};
    PyEncoder(PyMessageDatabase::ConstPtr pclMessageDb_) : Encoder(pclMessageDb_) {};

    PyMessageData PyEncode(const oem::PyMessage& py_message, ENCODE_FORMAT format) const;
};

}
