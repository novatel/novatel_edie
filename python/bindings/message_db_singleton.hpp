#pragma once

#include "novatel_edie/decoders/common/message_database.hpp"

class MessageDbSingleton
{
  public:
    static novatel::edie::MessageDatabase::Ptr& get();

    static const std::unordered_map<std::string, nb::object>& getEnumsByIdMap();

  private:
    MessageDbSingleton() = default;
};
