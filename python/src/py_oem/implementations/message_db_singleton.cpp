#include "py_oem/message_db_singleton.hpp"

#include "novatel_edie/decoders/common/json_db_reader.hpp"
#include "py_common/bindings_core.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

py_common::PyMessageDatabase::Ptr py_oem::MessageDbSingleton::json_db;

py_common::PyMessageDatabase::Ptr& py_oem::MessageDbSingleton::get()
{
    // If the database has already been loaded, return it
    if (json_db) { return json_db; }

    // Import necessary modules for locating the default database
    nb::object builtins = nb::module_::import_("builtins");
    nb::object os_path = nb::module_::import_("os.path");
    nb::object import_lib_util = nb::module_::import_("importlib.util");

    // Determine if the novatel_edie package exists within the current Python environment
    nb::object module_spec = import_lib_util.attr("find_spec")("novatel_edie");
    if (!nb::cast<bool>(builtins.attr("bool")(module_spec)))
    {
        // If the package does not exist, return an empty database
        json_db = nb::cast<py_common::PyMessageDatabase::Ptr>(py_common::PyMessageDatabase::Create());
    }
    else
    {
        // Determine the path to the database file within the novatel_edie package
        nb::object module_origin = module_spec.attr("origin");
        if (!nb::cast<bool>(builtins.attr("bool")(module_origin)))
        {
            // Return an empty database if the package is empty
            json_db = nb::cast<py_common::PyMessageDatabase::Ptr>(py_common::PyMessageDatabase::Create());
        }
        else
        {
            nb::object novatel_edie_path = os_path.attr("dirname")(module_spec.attr("origin"));
            nb::object db_path = os_path.attr("join")(novatel_edie_path, "database.json");
            // If the database file does not exist, return an empty database
            bool db_exists = nb::cast<bool>(os_path.attr("isfile")(db_path));
            if (!db_exists) { json_db = nb::cast<py_common::PyMessageDatabase::Ptr>(py_common::PyMessageDatabase::Create()); }
            else
            {
                std::string default_json_db_path = nb::cast<std::string>(db_path);
                json_db = nb::cast<py_common::PyMessageDatabase::Ptr>(
                    py_common::PyMessageDatabase::Create(std::move(*LoadJsonDbFile(default_json_db_path))));
            }
        }
    }
    json_db->Lock();
    return json_db;
}

void py_oem::MessageDbSingleton::cleanup() { json_db = nullptr; }
