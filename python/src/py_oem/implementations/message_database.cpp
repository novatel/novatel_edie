#include "py_oem/message_database.hpp"

#include <memory>

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

void py_oem::ThrowRXConfigConstructionError() { throw py_common::FailureException("RXConfig definition in database cannot be handled."); }

void* py_oem::AllocateDatabaseExtras(py_common::PyMessageDatabaseCore* database)
{
    auto* extras = new DatabaseExtras();
    extras->database = database;

    auto ownedDb = database->GetSharedPtr();
    extras->encoder = std::make_unique<oem::Encoder>(ownedDb);

    try
    {
        extras->rxConfigHandler = std::make_unique<oem::RxConfigHandler>(ownedDb);
    }
    catch (const std::invalid_argument&)
    {
        delete extras;
        ThrowRXConfigConstructionError();
    }

    return extras;
}

void py_oem::FreeDatabaseExtras(void* extras)
{
    auto* typedExtras = static_cast<DatabaseExtras*>(extras);
    delete typedExtras;
}

py_oem::DatabaseExtras& py_oem::GetDatabaseExtras(py_common::PyMessageDatabaseCore& database)
{
    if (database.GetMessageFamily() != "OEM") { throw py_common::FailureException("Database extras are only available for OEM message families."); }

    void* extras = database.GetMessageFamilyExtras();
    if (!extras) { throw py_common::FailureException("OEM database extras are not initialized for this database."); }

    return *static_cast<DatabaseExtras*>(extras);
}

const py_oem::DatabaseExtras& py_oem::GetDatabaseExtras(const py_common::PyMessageDatabaseCore& database)
{
    if (database.GetMessageFamily() != "OEM") { throw py_common::FailureException("Database extras are only available for OEM message families."); }

    void* extras = database.GetMessageFamilyExtras();
    if (!extras) { throw py_common::FailureException("OEM database extras are not initialized for this database."); }

    return *static_cast<const DatabaseExtras*>(extras);
}
