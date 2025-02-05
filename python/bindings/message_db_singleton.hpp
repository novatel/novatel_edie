#pragma once

#include "novatel_edie/decoders/common/message_database.hpp"
#include "py_database.hpp"

namespace novatel::edie {

class MessageDbSingleton
{
  public:
    MessageDbSingleton() = delete;
    [[nodiscard]] static PyMessageDatabase::Ptr& get();
};

} // namespace novatel::edie
