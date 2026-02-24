#pragma once

#include "novatel_edie/decoders/oem/encoder.hpp"
#include "novatel_edie/decoders/oem/rxconfig/rxconfig_handler.hpp"
#include "py_common/message_database.hpp"

namespace nb = nanobind;

namespace novatel::edie::py_oem {

class PyMessageDatabase
{
  private:
    void Initialize();

  public:
    explicit PyMessageDatabase(py_common::PyMessageDatabaseCore::Ptr pclMessageDb_);

    [[nodiscard]] std::string MsgIdToMsgName(uint32_t uiMessageId_) const { return pclMessageDb->MsgIdToMsgName(uiMessageId_); };

    // MessageDatabase wrappers
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(std::string_view strMsgName_) const { return pclMessageDb->GetMsgDef(strMsgName_); }
    [[nodiscard]] MessageDefinition::ConstPtr GetMsgDef(int32_t iMsgId_) const { return pclMessageDb->GetMsgDef(iMsgId_); }

    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefId(std::string& strEnumId_) const { return pclMessageDb->GetEnumDefId(strEnumId_); }
    [[nodiscard]] EnumDefinition::ConstPtr GetEnumDefName(std::string& strEnumName_) const { return pclMessageDb->GetEnumDefName(strEnumName_); }

    // PyMessageDatabaseCore wrappers
    [[nodiscard]] const std::unordered_map<std::string, py_common::PyMessageType>& GetMessagesByNameDict() const
    {
        return pclMessageDb->GetMessagesByNameDict();
    }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetFieldsByNameDict() const { return pclMessageDb->GetFieldsByNameDict(); }

    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByIdDict() const { return pclMessageDb->GetEnumsByIdDict(); }
    [[nodiscard]] const std::unordered_map<std::string, nb::object>& GetEnumsByNameDict() const { return pclMessageDb->GetEnumsByNameDict(); }

  private:
    py_common::PyMessageDatabaseCore::Ptr pclMessageDb; // This is the MessageDatabase that this class wraps
    std::unique_ptr<oem::Encoder> pclEncoder;
    std::unique_ptr<oem::RxConfigHandler> pclRxConfigHandler;

    void ThrowRXConfigConstructionError() const { throw py_common::FailureException("RXConfig definition in database cannot be handled."); }

  public:
    using Ptr = std::shared_ptr<PyMessageDatabase>;
    using ConstPtr = std::shared_ptr<const PyMessageDatabase>;

    py_common::PyMessageDatabaseCore::Ptr GetCoreDatabase() const { return pclMessageDb; }
    const oem::Encoder* GetEncoder() const { return pclEncoder.get(); }
    oem::RxConfigHandler* GetRxConfigHandler() const { return pclRxConfigHandler.get(); }
};

} // namespace novatel::edie::py_oem
