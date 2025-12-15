#pragma once

#include <filesystem>
#include <iostream>

#include "novatel_edie/decoders/oem/file_parser.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/exceptions.hpp"
#include "py_common/py_message_data.hpp"
#include "py_oem/message_db_singleton.hpp"
#include "py_oem/py_message_objects.hpp"


namespace nb = nanobind;

namespace novatel::edie::py_oem {

class PyFileParser : public oem::FileParser
{
  private:
    py_oem::PyMessageDatabase::Ptr pclPyMessageDb;
    void SetStreamByPath(const std::filesystem::path& filepath_)
    {
        auto ifs = std::make_shared<std::ifstream>(filepath_, std::ios::binary);
        if (!ifs) { throw std::runtime_error("Failed to open file"); }
        if (!SetStream(ifs)) { throw std::runtime_error("Input stream could not be set to the provided file."); }
    }

  public:
    PyFileParser(const std::filesystem::path& filepath_) : PyFileParser(filepath_, py_oem::MessageDbSingleton::get()) {};

    PyFileParser(const std::filesystem::path& filepath_, const py_common::PyMessageDatabaseCore::Ptr& message_db_pointer)
        : FileParser(message_db_pointer), pclPyMessageDb(std::make_shared<py_oem::PyMessageDatabase>(message_db_pointer))
    {
        SetStreamByPath(filepath_);
    }

    nb::object PyRead();

    nb::object PyIterRead();

    nb::object PyConvert(ENCODE_FORMAT fmt);
};

class FileConversionIterator
{
    PyFileParser& parser;
    ENCODE_FORMAT fmt;

  public:
    FileConversionIterator(PyFileParser& parser_, ENCODE_FORMAT fmt_) : parser(parser_), fmt(fmt_) {}

    nb::object PyIterConvert();
};
} // namespace novatel::edie::py_oem
