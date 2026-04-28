#pragma once

#include <memory>

#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"
#include "py_common/message_database.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_oem {

struct DatabaseExtras
{
    const py_common::PyMessageDatabaseCore* database;
    std::unique_ptr<oem::Encoder> encoder;
    std::unique_ptr<oem::RxConfigHandler> rxConfigHandler;
};

void* AllocateDatabaseExtras(py_common::PyMessageDatabaseCore* database);
void FreeDatabaseExtras(void* extras);

const DatabaseExtras& GetDatabaseExtras(const py_common::PyMessageDatabaseCore& database);

void ThrowRXConfigConstructionError();

} // namespace novatel::edie::py_oem
