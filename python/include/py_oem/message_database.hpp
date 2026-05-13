#pragma once

#include <memory>

#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"
#include "py_common/message_database.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_oem {

struct DatabaseExtras : public py_common::MessageDBExtrasBase
{
    std::unique_ptr<oem::Encoder> encoder;
    std::unique_ptr<oem::RxConfigHandler> rxConfigHandler;
};

std::unique_ptr<py_common::MessageDBExtrasBase> AllocateDatabaseExtras(MessageDatabase::Ptr database);

const DatabaseExtras& GetDatabaseExtras(const py_common::PyMessageDatabase& database);

void ThrowRXConfigConstructionError();

} // namespace novatel::edie::py_oem
