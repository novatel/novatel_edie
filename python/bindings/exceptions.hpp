#pragma once

#include <exception>

#include "nanobind/nanobind.h"
#include "novatel_edie/decoders/common/common.hpp"

namespace nb = nanobind;

namespace novatel::edie {

class EdieException : public std::exception
{
  private:
    std::string message;

  public:
    explicit EdieException(const std::string& message_ = "novatel_edie has encountered an error.") : message(message_) {}

    const char* what() const noexcept override { return message.c_str(); }
};

class FailureException : public EdieException
{
  public:
    FailureException(const std::string& message_ = "An unexpected failure occurred.") : EdieException(message_) {}
};

class UnknownException : public EdieException
{
  public:
    UnknownException(const std::string& message_ = "Could not identify bytes as a protocol.") : EdieException(message_) {}
};

class IncompleteException : public EdieException
{
  public:
    IncompleteException(const std::string& message_ = "It is possible that a valid frame exists in the frame buffer, but more information is needed.")
        : EdieException(message_)
    {
    }
};

class IncompleteMoreDataException : public EdieException
{
  public:
    IncompleteMoreDataException(const std::string& message_ = "The current frame buffer is incomplete but more data is expected.")
        : EdieException(message_)
    {
    }
};

class NullProvidedException : public EdieException
{
  public:
    NullProvidedException(const std::string& message_ = "A null pointer was provided.") : EdieException(message_) {}
};

class NoDatabaseException : public EdieException
{
  public:
    NoDatabaseException(const std::string& message_ = "No database has been provided to the component.") : EdieException(message_) {}
};

class NoDefinitionException : public EdieException
{
  public:
    NoDefinitionException(const std::string& message_ = "No definition could be found in the database for the provided message.")
        : EdieException(message_)
    {
    }
};

class NoDefinitionEmbeddedException : public EdieException
{
  public:
    NoDefinitionEmbeddedException(
        const std::string& message_ = "No definition could be found in the database for the embedded message in the RXCONFIG log.")
        : EdieException(message_)
    {
    }
};

class BufferFullException : public EdieException
{
  public:
    BufferFullException(const std::string& message_ = "The destination buffer is not big enough to contain the provided data.")
        : EdieException(message_)
    {
    }
};

class BufferEmptyException : public EdieException
{
  public:
    BufferEmptyException(const std::string& message_ = "The internal circular buffer does not contain any unread bytes.") : EdieException(message_) {}
};

class StreamEmptyException : public EdieException
{
  public:
    StreamEmptyException(const std::string& message_ = "The input stream is empty.") : EdieException(message_) {}
};

class UnsupportedException : public EdieException
{
  public:
    UnsupportedException(const std::string& message_ = "An attempted operation is unsupported by this component.") : EdieException(message_) {}
};

class MalformedInputException : public EdieException
{
  public:
    MalformedInputException(const std::string& message_ = "The input is recognizable, but has unexpected formatting.") : EdieException(message_) {}
};

class DecompressionFailureException : public EdieException
{
  public:
    DecompressionFailureException(const std::string& message_ = "The RANGECMP log could not be decompressed.") : EdieException(message_) {}
};

void throw_exception_from_status(STATUS status);

void throw_detailed_exception_from_status(STATUS status, const std::string& message);


} // namespace novatel::edie
