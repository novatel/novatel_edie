#pragma once

#include "novatel_edie/decoders/common/message_database.hpp"

#include "py_oem/message_database.hpp"

namespace novatel::edie::py_common {

//============================================================================
//! \class MessageDbSingleton
//! \brief The singular default MessageDatabase based on the JSON database
//!        file included in the novatel_edie package.
//!
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
    [[nodiscard]] static py_common::PyMessageDatabaseCore::Ptr& get();
};

} // namespace novatel::edie
