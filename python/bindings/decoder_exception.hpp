#pragma once

#include <exception>

#include "nanobind/nanobind.h"
#include "novatel_edie/decoders/common/common.hpp"

namespace nb = nanobind;

namespace novatel::edie {
class DecoderException : public std::exception
{
  public:
    DecoderException(nb::handle_t<STATUS> status, std::string message = "");

    const char* what() const noexcept override;

  private:
    std::string msg;
};
} // namespace novatel::edie
