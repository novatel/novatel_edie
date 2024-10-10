#pragma once

#include "novatel_edie/decoders/common/json_reader.hpp"

class JsonDbSingleton
{
  public:
    static novatel::edie::JsonReader::Ptr& get();

  private:
    JsonDbSingleton() = default;
};
