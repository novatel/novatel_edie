#include <vector>

#include <nanobind/nanobind.h>

#include "novatel_edie/stream_interface/common.hpp"
#include "novatel_edie/stream_interface/filestream.hpp"
#include "novatel_edie/stream_interface/inputfilestream.hpp"
#include "novatel_edie/stream_interface/multioutputfilestream.hpp"
#include "novatel_edie/stream_interface/outputfilestream.hpp"

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(hw_interface_bindings, m)
{
    // # stream_interface/common.hpp

    nb::enum_<FileSplitMethodEnum>(m, "FileSplitMethodEnum")
        .value("SPLIT_SIZE", FileSplitMethodEnum::SPLIT_SIZE)
        .value("SPLIT_LOG", FileSplitMethodEnum::SPLIT_LOG)
        .value("SPLIT_TIME", FileSplitMethodEnum::SPLIT_TIME)
        .value("SPLIT_NONE", FileSplitMethodEnum::SPLIT_NONE)
        .export_values();

    nb::class_<ReadDataStructure>(m, "ReadDataStructure")
        .def(nb::init<>())
        .def_ro("size", &ReadDataStructure::uiDataSize)
        .def_ro("data", &ReadDataStructure::cData);

    nb::class_<StreamReadStatus>(m, "StreamReadStatus")
        .def(nb::init<>())
        .def_ro("percent_read", &StreamReadStatus::uiPercentStreamRead)
        .def_ro("current_read", &StreamReadStatus::uiCurrentStreamRead)
        .def_ro("length", &StreamReadStatus::ullStreamLength)
        .def_ro("eos", &StreamReadStatus::bEOS);

    // # stream_interface/filestream.hpp

    nb::enum_<FileStream::FILE_MODES>(m, "FILE_MODES")
        .value("APPEND", FileStream::FILE_MODES::APPEND)
        .value("INSERT", FileStream::FILE_MODES::INSERT)
        .value("INPUT", FileStream::FILE_MODES::INPUT)
        .value("OUTPUT", FileStream::FILE_MODES::OUTPUT)
        .value("TRUNCATE", FileStream::FILE_MODES::TRUNCATE)
        .export_values();

    nb::class_<FileStream>(m, "FileStream")
        .def(nb::init<const std::u32string&>(), "file_name"_a)
        .def("open", &FileStream::OpenFile, "mode"_a)
        .def("close", &FileStream::CloseFile)
        .def("calculate_percentage", &FileStream::CalculatePercentage, "read_length"_a)
        .def(
            "read",
            [](FileStream& self, size_t size) {
                std::vector<char> buffer(size);
                StreamReadStatus status = self.ReadFile(buffer.data(), size);
                return nb::make_tuple(nb::bytes(buffer.data(), status.uiCurrentStreamRead), status);
            },
            "n"_a)
        .def("readline",
             [](FileStream& self) {
                 std::string line;
                 StreamReadStatus status = self.ReadLine(line);
                 return nb::make_tuple(nb::bytes(line.c_str(), line.size()), status);
             })
        .def(
            "write", [](FileStream& self, nb::bytes& data) { return self.WriteFile(data.c_str(), data.size()); }, "data"_a)
        .def_prop_ro("size", &FileStream::GetFileSize)
        .def("flush", &FileStream::FlushFile)
        .def("set_position", &FileStream::SetFilePosition)
        .def_prop_ro("length", &FileStream::GetFileLength)
        .def_prop_ro("file_name", &FileStream::Get32StringFileName)
        .def_prop_ro("current_size", &FileStream::GetCurrentFileSize)
        .def_prop_ro("my_file_stream", &FileStream::GetMyFileStream)
        .def("set_current_offset", &FileStream::SetCurrentFileOffset)
        .def_prop_ro("current_offset", &FileStream::GetCurrentFileOffset);

    // # stream_interface/multioutputfilestream.hpp

    nb::class_<MultiOutputFileStream>(m, "MultiOutputFileStream")
        .def(nb::init<>())
        .def("select_file_stream", nb::overload_cast<const std::u32string&>(&MultiOutputFileStream::SelectFileStream), "file_name"_a)
        .def("clear_file_stream_map", &MultiOutputFileStream::ClearWCFileStreamMap)
        .def("configure_base_file_name", nb::overload_cast<const std::u32string&>(&MultiOutputFileStream::ConfigureBaseFileName), "file_name"_a)
        .def("select_log_file", &MultiOutputFileStream::SelectWCLogFile, "log_name"_a)
        .def("select_size_file", &MultiOutputFileStream::SelectWCSizeFile, "size"_a)
        .def("select_time_file", &MultiOutputFileStream::SelectWCTimeFile, "status"_a, "week"_a, "milliseconds"_a)
        .def(
            "write_data", [](MultiOutputFileStream& self, nb::bytes& data) { return self.WriteData(data.c_str(), data.size()); }, "data"_a)
        .def(
            "write_data",
            [](MultiOutputFileStream& self, nb::bytes& data, std::string msg_name, uint32_t size, novatel::edie::TIME_STATUS status, uint16_t week,
               double milliseconds) { return self.WriteData(data.c_str(), data.size(), msg_name, size, status, week, milliseconds); },
            "data"_a, "msg_name"_a, "size"_a, "status"_a, "week"_a, "milliseconds"_a)
        .def("configure_split_by_log", &MultiOutputFileStream::ConfigureSplitByLog, "status"_a)
        .def("configure_split_by_size", &MultiOutputFileStream::ConfigureSplitBySize, "size"_a)
        .def("configure_split_by_time", &MultiOutputFileStream::ConfigureSplitByTime, "time"_a)
        .def_prop_ro("file_map", &MultiOutputFileStream::Get32FileMap)
        .def("set_extension_name", nb::overload_cast<const std::u32string&>(&MultiOutputFileStream::SetExtensionName), "ext"_a);

    // # stream_interface/outputfilestream.hpp

    nb::class_<OutputFileStream>(m, "OutputFileStream")
        .def(nb::init<const std::u32string>(), "file_name"_a)
        .def(
            "write", [](OutputFileStream& self, nb::bytes& data) { return self.WriteData(data.c_str(), data.size()); }, "data"_a)
        .def_ro("file_stream", &OutputFileStream::pOutFileStream);
}
