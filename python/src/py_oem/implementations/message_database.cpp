#include "py_oem/message_database.hpp"

#include <memory>

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void py_oem::ThrowRXConfigConstructionError() { throw py_common::FailureException("RXConfig definition in database cannot be handled."); }

std::unique_ptr<py_common::MessageDBExtrasBase> py_oem::AllocateDatabaseExtras(py_common::PyMessageDatabaseCore::Ptr database)
{
    auto extras = std::make_unique<DatabaseExtras>();
    extras->encoder = std::make_unique<oem::Encoder>(database);

    try
    {
        extras->rxConfigHandler = std::make_unique<oem::RxConfigHandler>(database);
    }
    catch (const std::invalid_argument&)
    {
        ThrowRXConfigConstructionError();
    }

    return extras;
}

const py_oem::DatabaseExtras& py_oem::GetDatabaseExtras(const py_common::PyMessageDatabase& database)
{
    const auto* typedExtras = dynamic_cast<const DatabaseExtras*>(database.GetMessageFamilyExtras());
    if (!typedExtras) { throw py_common::FailureException("Database extras are only available for OEM message families."); }
    return *typedExtras;
}
