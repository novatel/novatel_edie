#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "novatel_edie/stream_interface/common.hpp"
#include "novatel_edie/stream_interface/filestream.hpp"
#include "novatel_edie/stream_interface/inputfilestream.hpp"
#include "novatel_edie/stream_interface/multioutputfilestream.hpp"
#include "novatel_edie/stream_interface/outputfilestream.hpp"
#include "u32string.h"

namespace nb = nanobind;
using namespace nb::literals;

// For unit tests
class MultiOutputFileStreamTest : public MultiOutputFileStream
{
  public:
    static std::u32string GetBase32FileName(const MultiOutputFileStream& fs) { return cast(fs).s32MyBaseName; }
    static std::u32string Get32ExtensionName(const MultiOutputFileStream& fs) { return cast(fs).s32MyExtensionName; }
    static bool IsFileSplit(const MultiOutputFileStream& fs) { return cast(fs).bMyFileSplit; }
    static uint64_t GetFileSplitSize(const MultiOutputFileStream& fs) { return cast(fs).ullMyFileSplitSize; }
    static double GetTimeInSeconds(const MultiOutputFileStream& fs) { return cast(fs).dMyTimeInSeconds; }
    static double GetStartTimeInSecs(const MultiOutputFileStream& fs) { return cast(fs).dMyStartTimeInSeconds; }
    static uint32_t GetWeek(const MultiOutputFileStream& fs) { return cast(fs).ulMyWeek; }
    static uint32_t GetStartWeek(const MultiOutputFileStream& fs) { return cast(fs).ulMyStartWeek; }

  private:
    static const MultiOutputFileStreamTest& cast(const MultiOutputFileStream& fs) { return *static_cast<const MultiOutputFileStreamTest*>(&fs); }
};

void init_stream_interface(nb::module_& m)
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
        .def_ro("last_count", &StreamReadStatus::uiCurrentStreamRead)
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
                return nb::make_tuple(status, nb::bytes(buffer.data(), status.uiCurrentStreamRead));
            },
            "n"_a)
        .def("readline",
             [](FileStream& self) {
                 std::string line;
                 StreamReadStatus status = self.ReadLine(line);
                 return nb::make_tuple(status, nb::bytes(line.c_str(), line.size()));
             })
        .def(
            "write", [](FileStream& self, nb::bytes& data) { return self.WriteFile(data.c_str(), data.size()); }, "data"_a)
        .def_prop_ro("size", &FileStream::GetFileSize)
        .def("flush", &FileStream::FlushFile)
        .def("set_position", &FileStream::SetFilePosition)
        .def_prop_ro("length", &FileStream::GetFileLength)
        .def_prop_ro("file_name", &FileStream::Get32StringFileName)
        .def_prop_ro("current_size", &FileStream::GetCurrentFileSize)
        //      .def_prop_ro("file_stream", &FileStream::GetMyFileStream)
        .def_prop_ro("stream_failed", [](FileStream& self) { return self.GetMyFileStream()->fail(); })
        .def("set_current_offset", &FileStream::SetCurrentFileOffset)
        .def_prop_ro("current_offset", &FileStream::GetCurrentFileOffset);

    // # stream_interface/multioutputfilestream.hpp

    nb::class_<MultiOutputFileStream>(m, "MultiOutputFileStream")
        .def(nb::init<>())
        .def("select_file_stream", nb::overload_cast<const std::u32string&>(&MultiOutputFileStream::SelectFileStream), "file_name"_a)
        .def("clear_file_stream_map", &MultiOutputFileStream::ClearWCFileStreamMap)
        .def("configure_base_file_name", nb::overload_cast<std::u32string_view>(&MultiOutputFileStream::ConfigureBaseFileName), "file_name"_a)
        .def("select_log_file", &MultiOutputFileStream::SelectWCLogFile, "log_name"_a)
        .def("select_size_file", &MultiOutputFileStream::SelectWCSizeFile, "size"_a)
        .def("select_time_file", &MultiOutputFileStream::SelectWCTimeFile, "status"_a, "week"_a, "milliseconds"_a)
        .def(
            "write", [](MultiOutputFileStream& self, nb::bytes& data) { return self.WriteData(data.c_str(), data.size()); }, "data"_a)
        .def(
            "write",
            [](MultiOutputFileStream& self, nb::bytes& data, std::string msg_name, uint32_t size, novatel::edie::TIME_STATUS status, uint16_t week,
               double milliseconds) { return self.WriteData(data.c_str(), data.size(), std::move(msg_name), size, status, week, milliseconds); },
            "data"_a, "msg_name"_a, "size"_a, "status"_a, "week"_a, "milliseconds"_a)
        .def("configure_split_by_log", &MultiOutputFileStream::ConfigureSplitByLog, "status"_a)
        .def("configure_split_by_size", &MultiOutputFileStream::ConfigureSplitBySize, "size"_a)
        .def("configure_split_by_time", &MultiOutputFileStream::ConfigureSplitByTime, "time"_a)
        .def_prop_ro("file_map",
                     [](MultiOutputFileStream& self) {
                         nb::dict py_map;
                         for (const auto& [key, value] : self.Get32FileMap()) py_map[nb::cast(key)] = nb::cast(value, nb::rv_policy::reference);
                         return py_map;
                     })
        .def("set_extension_name", nb::overload_cast<std::u32string_view>(&MultiOutputFileStream::SetExtensionName), "ext"_a)
        // For unit tests only
        .def_prop_ro("_base_name", &MultiOutputFileStreamTest::GetBase32FileName)
        .def_prop_ro("_extension_name", &MultiOutputFileStreamTest::Get32ExtensionName)
        .def_prop_ro("_is_file_split", &MultiOutputFileStreamTest::IsFileSplit)
        .def_prop_ro("_file_split_size", &MultiOutputFileStreamTest::GetFileSplitSize)
        .def_prop_ro("_time_in_seconds", &MultiOutputFileStreamTest::GetTimeInSeconds)
        .def_prop_ro("_start_time_in_seconds", &MultiOutputFileStreamTest::GetStartTimeInSecs)
        .def_prop_ro("_week", &MultiOutputFileStreamTest::GetWeek)
        .def_prop_ro("_start_week", &MultiOutputFileStreamTest::GetStartWeek);

    // # stream_interface/inputfilestream.hpp

    nb::class_<InputFileStream>(m, "InputFileStream")
        .def(nb::init<const std::u32string&>(), "file_name"_a)
        .def_rw("file_stream", &InputFileStream::pInFileStream)
        .def(
            "read",
            [](InputFileStream& self, size_t size) {
                std::vector<char> buffer(size);
                ReadDataStructure read_data;
                read_data.cData = buffer.data();
                read_data.uiDataSize = size;
                StreamReadStatus status = self.ReadData(read_data);
                return nb::make_tuple(status, nb::bytes(buffer.data(), status.uiCurrentStreamRead));
            },
            "n"_a)
        .def("readline",
             [](InputFileStream& self) {
                 std::string line;
                 StreamReadStatus status = self.ReadLine(line);
                 return nb::make_tuple(status, nb::bytes(line.c_str(), line.size()));
             })
        .def(
            "reset",
            [](InputFileStream& self, std::streamoff offset, int dir) {
                std::ios_base::seekdir seek_dir;
                switch (dir)
                {
                case 0: seek_dir = std::ios::beg; break;
                case 1: seek_dir = std::ios::cur; break;
                case 2: seek_dir = std::ios::end; break;
                default: seek_dir = std::ios::beg; break;
                }
                self.Reset(offset, seek_dir);
            },
            "offset"_a = 0, "whence"_a = 0)
        .def_prop_ro("file_name", &InputFileStream::GetFileName)
        .def_prop_ro("current_position", &InputFileStream::GetCurrentFilePosition)
        .def("set_current_offset", &InputFileStream::SetCurrentFileOffset)
        .def_prop_ro("current_offset", &InputFileStream::GetCurrentFileOffset);

    // # stream_interface/outputfilestream.hpp

    nb::class_<OutputFileStream>(m, "OutputFileStream")
        .def(nb::init<const std::u32string>(), "file_name"_a)
        .def(
            "write", [](OutputFileStream& self, nb::bytes& data) { return self.WriteData(data.c_str(), data.size()); }, "data"_a)
        .def_ro("file_stream", &OutputFileStream::pOutFileStream);
}
