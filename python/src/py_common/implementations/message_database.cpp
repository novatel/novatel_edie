#include "py_common/message_database.hpp"

#include <functional>
#include <iomanip>
#include <sstream>

#include <nanobind/eval.h>
#include <nanobind/stl/unordered_map.h>

#include "novatel_edie/decoders/common/message_database.hpp"
#include "py_common/bindings_core.hpp"
#include "py_common/field_objects.hpp"

namespace nb = nanobind;
using namespace nb::literals;
using namespace novatel::edie;

PYCOMMON_EXPORT std::unordered_map<std::string, py_common::MessageFamilyRegistration>& py_common::GetMessageFamilyRegistrations()
{
    static std::unordered_map<std::string, py_common::MessageFamilyRegistration> registrations;
    return registrations;
}

PYCOMMON_EXPORT const py_common::MessageFamilyRegistration* py_common::GetMessageFamilyRegistration(const std::string& message_family)
{
    auto& registrations = GetMessageFamilyRegistrations();
    auto registrationIt = registrations.find(message_family);
    if (registrationIt == registrations.end()) { return nullptr; }
    return &registrationIt->second;
}

PYCOMMON_EXPORT py_common::PyMessageDatabase::PyMessageDatabase(MessageDatabase&& message_db)
    : core_(std::make_shared<MessageDatabase>(std::move(message_db)))
{
}

PYCOMMON_EXPORT nb::object py_common::PyMessageDatabase::Create(MessageDatabase&& message_db)
{
    PyMessageDatabase::Ptr db(new PyMessageDatabase(std::move(message_db)));
    // Publish the Python wrapper into nanobind's instance map so nb::find(this)
    // returns it during Initialize(). The returned nb::object keeps the wrapper
    // alive for the caller; dropping it before Initialize would destroy the
    // wrapper and unregister it from the instance map.
    nb::object wrapper = nb::cast(db);
    db->Initialize();
    db->allocateExtras();
    return wrapper;
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::Initialize()
{
    ResolveBaseType();
    UpdatePythonEnums();
    UpdatePythonMessageTypes();
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::ResolveBaseType()
{
    if (!core_->GetDbMetadata()) { core_->SetMessageFamily(""); }

    const std::string family = GetMessageFamily();
    message_family_registration_ = py_common::GetMessageFamilyRegistration(family);
    if (!message_family_registration_ || !message_family_registration_->messageType.is_valid())
    {
        throw FailureException(family + " is not a recognized message type.");
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::allocateExtras()
{
    if (message_family_registration_ && message_family_registration_->allocateExtras)
    {
        extras_ = message_family_registration_->allocateExtras(core_);
    }
    else { extras_.reset(); }
}

PYCOMMON_EXPORT std::string py_common::PyMessageDatabase::GetMessageFamily() const
{
    auto metadata = core_->GetDbMetadata();
    return metadata ? metadata->messageFamily : "";
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::SetMessageFamily(const std::string& messageFamily)
{
    ThrowIfLocked();
    core_->SetMessageFamily(messageFamily);
    ResolveBaseType();
    UpdatePythonMessageTypes();
    allocateExtras();
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::Merge(const Ptr& other_)
{
    ThrowIfLocked();
    other_->Lock();
    core_->AppendEnumerations(other_->core_->EnumDefinitions());
    AppendEnumTypes(other_->core_->EnumDefinitions());
    core_->AppendMessages(other_->core_->MessageDefinitions());
    AppendMessageTypes(other_->core_->MessageDefinitions());
}

// Why clone on the way in:
// MessageDatabase::AppendMessages writes through `EnumField::enumDef` via
// MapMessageEnumFields; without an owned copy that mutation corrupts the
// source database's shared field defs. Python is the only client that
// legitimately reuses a def across databases, so the duplication is paid here
// rather than inside the C++ AppendMessages.
PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendMessages(const std::vector<MessageDefinition::ConstPtr>& vMessageDefinitions_)
{
    ThrowIfLocked();
    std::vector<MessageDefinition::ConstPtr> owned;
    owned.reserve(vMessageDefinitions_.size());
    for (const auto& msgDef : vMessageDefinitions_) { owned.push_back(std::make_shared<MessageDefinition>(*msgDef)); }
    core_->AppendMessages(owned);
    AppendMessageTypes(owned);
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendEnumerations(const std::vector<EnumDefinition::ConstPtr>& vEnumDefinitions_)
{
    ThrowIfLocked();
    core_->AppendEnumerations(vEnumDefinitions_);
    AppendEnumTypes(vEnumDefinitions_);
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveMessage(uint32_t iMsgId_)
{
    ThrowIfLocked();
    RemoveMessageType(iMsgId_);
    core_->RemoveMessage(iMsgId_);
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveEnumeration(std::string strEnumeration_)
{
    ThrowIfLocked();
    RemoveEnumType(strEnumeration_);
    core_->RemoveEnumeration(strEnumeration_);
}

<<<<<<< HEAD
PYCOMMON_EXPORT nb::object py_common::PyMessageDatabase::fork()
=======
PYCOMMON_EXPORT nb::object py_common::PyMessageDatabase::clone()
>>>>>>> ccf6dd64 (Initial)
{
    Lock();
    // Build a new wrapper around a shallow copy of the C++ database — same
    // MessageDefinition / EnumDefinition / BaseField pointers, just a different
    // MessageDatabase instance.
    PyMessageDatabase::Ptr db(new PyMessageDatabase(MessageDatabase(*core_)));
    nb::object wrapper = nb::cast(db);
    db->ResolveBaseType();
    // Share the Python type caches with the source. Map keys are raw pointers
    // into the shared core_, so the same keys identify the same defs in either
    // database. The dynamic Python types' `_owner_db` attr still points at the
    // source, which is fine: the type belongs to whichever DB built it, and
    // GetMessageTypeLookup works on either map.
    db->messages_types = messages_types;
    db->field_types = field_types;
    db->enum_types = enum_types;
    db->field_name_maps_ = field_name_maps_;
    db->message_field_name_maps_ = message_field_name_maps_;
    db->message_type_lookup_ = message_type_lookup_;
    db->field_type_lookup_ = field_type_lookup_;
    db->enum_type_lookup_ = enum_type_lookup_;
    db->allocateExtras();
    return wrapper;
}

static void cleanString(std::string& str)
{
    // Remove special characters from the string to make it a valid python attribute name
    for (char& c : str)
    {
        if (!isalnum(c)) { c = '_'; }
    }
    if (isdigit(str[0])) { str = "_" + str; }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::UpdatePythonEnums()
{
    enum_types.clear();
    enum_type_lookup_.clear();
    AppendEnumTypes(core_->EnumDefinitions());
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendEnumTypes(const std::vector<EnumDefinition::ConstPtr>& enum_defs)
{
    nb::object IntEnum = nb::module_::import_("enum").attr("IntEnum");
    for (const auto& enum_def : enum_defs)
    {
        // remove existing enum type if it exists to ensure clean overwrite
        RemoveEnumType(enum_def->name);

        nb::dict values;
        const char* enum_name = enum_def->name.c_str();
        for (const auto& enumerator : enum_def->enumerators)
        {
            std::string enumerator_name = enumerator.name;
            cleanString(enumerator_name);
            values[enumerator_name.c_str()] = enumerator.value;
        }
        values["EDIE_UNKNOWN"] = enum_def->unknownValue;
        nb::object enum_type = IntEnum(enum_name, values);
        enum_type.attr("_owner_db") = nb::find(this);
        enum_types[enum_def.get()] = enum_type;
        enum_type_lookup_[enum_type] = enum_def.get();
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveEnumType(const std::string& enum_name)
{
    // get the enum definition to retrieve the pointer
    EnumDefinition::ConstPtr enum_def = core_->GetEnumDefName(enum_name);
    if (!enum_def) { return; }
    auto enum_it = enum_types.find(enum_def.get());
    if (enum_it != enum_types.end())
    {
        enum_type_lookup_.erase(enum_it->second);
        enum_types.erase(enum_it);
    }
}

static py_common::FieldNameMap BuildFieldNameMapFromDefs(const std::vector<BaseField::Ptr>& fields)
{
    py_common::FieldNameMap map;
    for (size_t i = 0; i < fields.size(); i++)
    {
        const auto& def = fields[i];
        map[def->name] = {i, false};
        if (def->type == FIELD_TYPE::FIELD_ARRAY || def->type == FIELD_TYPE::VARIABLE_LENGTH_ARRAY) { map[def->name + "_length"] = {i, true}; }
    }
    return map;
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::UpdatePythonMessageTypes()
{
    // clear existing definitions
    messages_types.clear();
    field_name_maps_.clear();
    message_field_name_maps_.clear();
    message_type_lookup_.clear();
    field_type_lookup_.clear();

    // add message and message body types for each message definition
    AppendMessageTypes(core_->MessageDefinitions());
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AddFieldType(std::vector<std::shared_ptr<BaseField>> fields, std::string base_name,
                                                                std::string parent_message, nb::handle type_constructor)
{
    // rescursively add field types for each field array element within the provided vector
    for (const auto& field : fields)
    {
        if (field->type == FIELD_TYPE::FIELD_ARRAY)
        {
            auto* field_array_field = dynamic_cast<FieldArrayField*>(field.get());
            std::string field_name = base_name + "_" + field_array_field->name + "_Field";
            nb::object field_type = type_constructor(field_name);
            field_types[field.get()] = field_type;
            field_type_lookup_[field_type] = field.get();
            field_name_maps_[field.get()] = BuildFieldNameMapFromDefs(field_array_field->fields);
            AddFieldType(field_array_field->fields, field_name, parent_message, type_constructor);
        }
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::AppendMessageTypes(const std::vector<MessageDefinition::ConstPtr>& message_defs)
{
    if (!message_family_registration_ || !message_family_registration_->messageType.is_valid())
    {
        throw FailureException("Message family type is not initialized.");
    }

    // get type constructor
    nb::dict globals;
    // Initializing builtins appears necessary to access 'type' functions in older python versions
    globals["__builtins__"] = nb::module_::import_("builtins");
    globals["field_type"] = nb::type<py_common::PyField>();
    globals["message_type"] = message_family_registration_->messageType;
    globals["db"] = nb::find(this);

    nb::exec(R"(
    def message_type_cons(name):
        return type(name, (message_type,), {'__slots__': (), '_owner_db': db})

    def field_type_cons(name):
        return type(name, (field_type,), {'__slots__': (), '_owner_db': db})
    )",
             globals);
    nb::object msg_type_cons = globals["message_type_cons"];
    nb::object field_type_cons = globals["field_type_cons"];

    // add message and message body types for each message definition
    for (const auto& message_def : message_defs)
    {
        // Initialize message mapping
        // remove existing message type if it exists to ensure clean overwrite
        RemoveMessageType(message_def->logID);
        messages_types[message_def.get()] = std::map<uint32_t, nb::object>{};

        for (const auto& [crc, message_fields] : message_def->fields)
        {
            std::ostringstream ss;
            ss << message_def->name << "_" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << crc;
            std::string message_name = ss.str();

            // specify the python superclass for the message type
            nb::object msg_py_type = msg_type_cons(message_name);
            messages_types[message_def.get()][crc] = msg_py_type;
            message_type_lookup_[msg_py_type] = {message_def.get(), crc};
            message_field_name_maps_[message_def.get()][crc] = BuildFieldNameMapFromDefs(message_fields);

            AddFieldType(message_fields, message_name, message_name, field_type_cons);
        }
    }
}

PYCOMMON_EXPORT void py_common::PyMessageDatabase::RemoveMessageType(uint32_t message_id)
{
    // get the message definition to retrieve the name
    MessageDefinition::ConstPtr message_def = core_->GetMsgDef(message_id);
    if (!message_def) { return; }

    // remove the message type
    auto messages_it = messages_types.find(message_def.get());
    if (messages_it != messages_types.end())
    {
        for (const auto& [crc, type_obj] : messages_it->second) { message_type_lookup_.erase(type_obj); }
    }
    messages_types.erase(message_def.get());
    message_field_name_maps_.erase(message_def.get());

    // remove all field types associated with this message from field_types
    for (const auto& [crc, fields] : message_def->fields) { RemoveFieldTypes(fields); }
}

void py_common::PyMessageDatabase::RemoveFieldTypes(const std::vector<BaseField::Ptr>& fieldDefs)
{
    for (const auto& fieldDef : fieldDefs)
    {
        auto field_it = field_types.find(fieldDef.get());
        if (field_it != field_types.end())
        {
            field_type_lookup_.erase(field_it->second);
            field_types.erase(field_it);
        }
        field_name_maps_.erase(fieldDef.get());
        if (fieldDef->type == FIELD_TYPE::FIELD_ARRAY)
        {
            auto* fieldArrayFieldDef = dynamic_cast<FieldArrayField*>(fieldDef.get());
            RemoveFieldTypes(fieldArrayFieldDef->fields);
        }
    }
}
