#pragma once

#include "novatel_edie/decoders/oem/parser.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/message_database.hpp"
#include "py_common/py_message_data.hpp"
#include "py_oem/message_database.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/py_message_objects.hpp"


namespace nb = nanobind;

namespace novatel::edie::py_oem {

nb::object HandlePythonReadStatus(STATUS status_, MessageDataStruct& message_data_, py_oem::PyHeader& header_,
                                  std::vector<FieldContainer>&& message_fields_, oem::MetaDataStruct& metadata_,
                                  py_oem::PyMessageDatabase::ConstPtr database_);

class PyParser : public oem::Parser
{
  private:
    py_oem::PyMessageDatabase::Ptr pclPyMessageDb;

  public:
    // inherit constructors
    PyParser(py_common::PyMessageDatabaseCore::Ptr& pclMessageDb_)
        : Parser(pclMessageDb_), pclPyMessageDb(std::make_shared<PyMessageDatabase>(pclMessageDb_))
    {
    }

    nb::object PyRead(bool decode_incomplete);
    nb::object PyIterRead();
    nb::object PyConvert(ENCODE_FORMAT fmt, bool decode_incomplete);
};

class ConversionIterator
{
    PyParser& parser;
    ENCODE_FORMAT fmt;

  public:
    ConversionIterator(PyParser& parser_, ENCODE_FORMAT fmt_) : parser(parser_), fmt(fmt_) {}

    nb::object PyIterConvert();
};

}; // namespace novatel::edie::py_oem
