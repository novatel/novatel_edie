#pragma once

#include "novatel_edie/decoders/common/json_reader.hpp"

class JsonDbSingleton
{
  public:
    static novatel::edie::JsonReader::Ptr& get();

    static const std::unordered_map<std::string, nb::object>& getEnumsByIdMap();

  private:
    JsonDbSingleton() = default;
};
