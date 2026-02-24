#include "py_oem/message_database.hpp"

#include <memory>

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void py_oem::PyMessageDatabase::Initialize()
{
    pclEncoder = std::make_unique<oem::Encoder>(this->pclMessageDb);
    try
    {
        pclRxConfigHandler = std::make_unique<oem::RxConfigHandler>(this->pclMessageDb);
    }
    catch (const std::invalid_argument&)
    {
        ThrowRXConfigConstructionError();
    }
}

py_oem::PyMessageDatabase::PyMessageDatabase(py_common::PyMessageDatabaseCore::Ptr message_db) : pclMessageDb(message_db) { Initialize(); }
