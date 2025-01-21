#include "novatel_edie/decoders/common/json_db_reader.hpp"

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"
#include "novatel_edie/common/nexcept.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

namespace {
std::string default_json_db_path()
{
    // Does the following, but using nanobind:
    // import importlib_resources
    // path_ctx = importlib_resources.as_file(importlib_resources.files("novatel_edie").joinpath("messages_public.json"))
    // with path_ctx as path:
    //     return path
    nb::object ir = nb::module_::import_("importlib_resources");
    nb::object path_ctx = ir.attr("as_file")(ir.attr("files")("novatel_edie").attr("joinpath")("messages_public.json"));
    auto py_path = path_ctx.attr("__enter__")();
    if (!nb::cast<bool>(py_path.attr("is_file")()))
    {
        throw NExcept((std::string("Could not find the default JSON DB file at ") + nb::str(py_path).c_str()).c_str());
    }
    auto path = nb::cast<std::string>(nb::str(py_path));
    path_ctx.attr("__exit__")(nb::none(), nb::none(), nb::none());
    return path;
}
} // namespace

PyMessageDatabase::Ptr& MessageDbSingleton::get()
{
    static PyMessageDatabase::Ptr json_db = nullptr;
    if (!json_db) { json_db = std::make_shared<PyMessageDatabase>(*JsonDbReader::LoadFile(default_json_db_path())); }
    return json_db;
}

void init_common_json_db_reader(nb::module_& m)
{
    nb::exception<JsonDbReaderFailure>(m, "JsonDbReaderFailure"); // NOLINT

    nb::class_<JsonDbReader>(m, "JsonDbReader")
        .def_static(
            "load_file",
            [](const std::filesystem::path& file_path) { //
                return std::make_shared<PyMessageDatabase>(std::move(*JsonDbReader::LoadFile(file_path)));
            },
            "file_path"_a)
        .def_static(
            "parse",
            [](const std::string_view json_data) { //
                return std::make_shared<PyMessageDatabase>(std::move(*JsonDbReader::Parse(json_data)));
            },
            "json_data"_a)
        .def_static(
            "append_messages",
            [](const PyMessageDatabase::Ptr& messageDb_, const std::filesystem::path& filePath_) {
                JsonDbReader::AppendMessages(messageDb_, filePath_);
            },
            "message_db"_a, "file_path"_a)
        .def_static(
            "append_enumerations",
            [](const PyMessageDatabase::Ptr& messageDb_, const std::filesystem::path& filePath_) {
                JsonDbReader::AppendEnumerations(messageDb_, filePath_);
            },
            "message_db"_a, "file_path"_a);

    m.def("get_default_database", &MessageDbSingleton::get, "Get the default JSON database singleton");
}
