#pragma once

#include <exception>

#include "nanobind/nanobind.h"
#include "novatel_edie/decoders/common/common.hpp"

namespace nb = nanobind;

namespace novatel::edie {

class FailureException : public std::exception
{
  public:
    const char* what() const noexcept override { return "An unexpected failure occurred."; }
};

class UnknownException : public std::exception
{
  public:
    const char* what() const noexcept override { return "Could not identify bytes as a protocol."; }
};

class IncompleteException : public std::exception
{
  public:
    const char* what() const noexcept override
    {
        return "It is possible that a valid frame exists in the frame buffer, but more information is needed.";
    }
};

class IncompleteMoreDataException : public std::exception
{
  public:
    const char* what() const noexcept override { return "The current frame buffer is incomplete but more data is expected."; }
};

class NullProvidedException : public std::exception
{
  public:
    const char* what() const noexcept override { return "A null pointer was provided."; }
};

class NoDatabaseException : public std::exception
{
  public:
    const char* what() const noexcept override { return "No database has been provided to the component."; }
};

class NoDefinitionException : public std::exception
{
  public:
    const char* what() const noexcept override { return "No definition could be found in the database for the provided message."; }
};

class NoDefinitionEmbeddedException : public std::exception
{
  public:
    const char* what() const noexcept override
    {
        return "No definition could be found in the database for the embedded message in the RXCONFIG log.";
    }
};

class BufferFullException : public std::exception
{
  public:
    const char* what() const noexcept override { return "The provided destination buffer is not big enough to contain the frame."; }
};

class BufferEmptyException : public std::exception
{
  public:
    const char* what() const noexcept override { return "The internal circular buffer does not contain any unread bytes."; }
};

class StreamEmptyException : public std::exception
{
  public:
    const char* what() const noexcept override { return "The input stream is empty."; }
};

class UnsupportedException : public std::exception
{
  public:
    const char* what() const noexcept override { return "An attempted operation is unsupported by this component."; }
};

class MalformedInputException : public std::exception
{
  public:
    const char* what() const noexcept override { return "The input is recognizable, but has unexpected formatting."; }
};

class DecompressionFailureException : public std::exception
{
  public:
    const char* what() const noexcept override { return "The RANGECMP log could not be decompressed."; }
};

void throw_exception_from_status(STATUS status);

} // namespace novatel::edie
