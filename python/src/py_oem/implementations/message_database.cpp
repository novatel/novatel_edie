#include "py_oem/message_database.hpp"

#include <memory>

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void py_oem::ThrowRXConfigConstructionError() { throw py_common::FailureException("RXConfig definition in database cannot be handled."); }

void* py_oem::AllocateDatabaseExtras(py_common::PyMessageDatabaseCore* database)
{
    // Use unique pointer for cleanup if method throws
    auto extras = std::make_unique<DatabaseExtras>();
    extras->database = database;

    auto ownedDb = database->GetSharedPtr();
    extras->encoder = std::make_unique<oem::Encoder>(ownedDb);

    try
    {
        extras->rxConfigHandler = std::make_unique<oem::RxConfigHandler>(ownedDb);
    }
    catch (const std::invalid_argument&)
    {
        ThrowRXConfigConstructionError();
    }

    return extras.release();
}

void py_oem::FreeDatabaseExtras(void* extras)
{
    auto* typedExtras = static_cast<DatabaseExtras*>(extras);
    delete typedExtras;
}

const py_oem::DatabaseExtras& py_oem::GetDatabaseExtras(const py_common::PyMessageDatabaseCore& database)
{
    // TODO: This check can be removed if we ever add safeguards around modifying database after initial use
    if (database.GetMessageFamily() != "OEM" && database.GetMessageFamily() != "")
    {
        throw py_common::FailureException("Database extras are only available for OEM message families.");
    }

    return *static_cast<const DatabaseExtras*>(database.GetMessageFamilyExtras());
}
