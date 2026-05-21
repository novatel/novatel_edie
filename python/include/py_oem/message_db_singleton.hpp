#pragma once

#include "py_common/message_database.hpp"

namespace novatel::edie::py_oem {

//============================================================================
//! \class MessageDbSingleton
//! \brief The singular default MessageDatabase based on the JSON database
//!        file included in the novatel_edie package.
//!
//! This is OEM-specific because the built-in database is an OEM database.
//! If the package does not contain a JSON database file or the C++ bindings
//! submodule is imported outside of the package, the default database will
//! be empty.
//============================================================================
class MessageDbSingleton
{
  public:
    MessageDbSingleton() = delete;

    //----------------------------------------------------------------------------
    //! \brief Method to get the MessageDbSingleton.
    //!
    //! If the instance does not yet exist, it will be created and returned.
    //----------------------------------------------------------------------------
    [[nodiscard]] static py_common::PyMessageDatabase::Ptr& get();

    //----------------------------------------------------------------------------
    //! \brief Frees the singleton database.
    //!
    //! Must be called at Python interpreter exit; relying on static destruction
    //! leads to this being done too late.
    //----------------------------------------------------------------------------
    static void cleanup();

    static py_common::PyMessageDatabase::Ptr json_db;
};

} // namespace novatel::edie::py_oem
