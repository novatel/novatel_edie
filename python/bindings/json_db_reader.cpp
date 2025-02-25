#include "novatel_edie/decoders/common/json_db_reader.hpp"

#include "bindings_core.hpp"
#include "message_db_singleton.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

PyMessageDatabase::Ptr& MessageDbSingleton::get()
{
    static PyMessageDatabase::Ptr json_db = nullptr;

    // If the database has already been loaded, return it
    if (json_db) { return json_db; }

    // Import necessary modules for locating the default database
    nb::object builtins = nb::module_::import_("builtins");
    nb::object os_path = nb::module_::import_("os.path");
    nb::object import_lib_util = nb::module_::import_("importlib.util");

    // Determine if the novatel_edie package exists within the current Python environment
    nb::object module_spec = import_lib_util.attr("find_spec")("novatel_edie");
    bool module_exists = nb::cast<bool>(builtins.attr("bool")(import_lib_util.attr("find_spec")("novatel_edie")));
    // If the package does not exist, return an empty database
    if (!module_exists)
    {
        json_db = std::make_shared<PyMessageDatabase>(MessageDatabase());
        return json_db;
    }

    // Determine the path to the database file within the novatel_edie package
    nb::object novatel_edie_path = os_path.attr("dirname")(module_spec.attr("origin"));
    nb::object db_path = os_path.attr("join")(novatel_edie_path, "database.json");
    // If the database file does not exist, return an empty database
    bool db_exists = nb::cast<bool>(os_path.attr("isfile")(db_path));
    if (!db_exists)
    {
        json_db = std::make_shared<PyMessageDatabase>(MessageDatabase());
        return json_db;
    }
    // If the database file exists, load it
    std::string default_json_db_path = nb::cast<std::string>(db_path);
    json_db = std::make_shared<PyMessageDatabase>(*LoadJsonDbFile(default_json_db_path));
    return json_db;
}

void init_common_json_db_reader(nb::module_& m)
{
    nb::exception<JsonDbReaderFailure>(m, "JsonDbReaderFailure"); // NOLINT

    m.def(
        "load_json_db_file",
        [](const std::filesystem::path& file_path) { //
            return std::make_shared<PyMessageDatabase>(std::move(*LoadJsonDbFile(file_path)));
        },
        "file_path"_a);
    m.def(
        "parse_json_db",
        [](const std::string_view json_data) { //
            return std::make_shared<PyMessageDatabase>(std::move(*ParseJsonDb(json_data)));
        },
        "json_data"_a);
    m.def("get_default_database", &MessageDbSingleton::get, "Get the default JSON database singleton");
}
