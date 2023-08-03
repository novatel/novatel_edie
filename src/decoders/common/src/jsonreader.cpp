////////////////////////////////////////////////////////////////////////
//
// COPYRIGHT NovAtel Inc, 2022. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////
//                            DESCRIPTION
//
//! \file jsonreader.cpp
//! \brief Class to parse JSON UI DB files from NovAtel.
////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------
#include "jsonreader.hpp"

namespace novatel::edie {

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::EnumDataType& f)
{
   f.value = j.at("value");
   f.name = j.at("name");
   f.description = j.at("description").is_null() ? "" : j.at("description");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::BaseDataType& f)
{
   auto itrDataTypeMapping = DataTypeEnumLookup.find(j.at("name"));
   f.name = itrDataTypeMapping != DataTypeEnumLookup.end() ? itrDataTypeMapping->second : DATA_TYPE_NAME::UNKNOWN;
   f.length = j.at("length");
   f.description = j.at("description").is_null() ? "" : j.at("description");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::SimpleDataType& f)
{
   from_json(j, static_cast<BaseDataType&>(f));

   if (j.find("enum") != j.end())
   {
      for (const auto& e : j.at("enum"))
      {
         f.enums[e.at("value")] = e;
      }
   }
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::BaseField& f)
{
   f.name = j.at("name");
   f.description = j.at("description").is_null() ? "" : j.at("description");

   auto itrFieldTypeMapping = FieldTypeEnumLookup.find(j.at("type"));
   f.type = itrFieldTypeMapping != FieldTypeEnumLookup.end() ? itrFieldTypeMapping->second : FIELD_TYPE::UNKNOWN;

   if (j.find("conversionString") != j.end())
   {
      if (j.at("conversionString").is_null())
         f.conversion = "";
      else
         f.setConversion(j.at("conversionString"));
   }

   f.dataType = j.at("dataType");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::EnumField& f)
{
   from_json(j, static_cast<BaseField&>(f));

   if (j.at("enumID").is_null())
      throw std::runtime_error("Invalid enum ID - cannot be NULL.  JsonDB file is likely corrupted.");

   f.enumID = j.at("enumID");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::ArrayField& fd)
{
   from_json(j, static_cast<BaseField&>(fd));

   fd.arrayLength = j.at("arrayLength");
   fd.dataType = j.at("dataType");
}

//-----------------------------------------------------------------------
void from_json(const json& j, novatel::edie::FieldArrayField& fd)
{
   from_json(j, static_cast<BaseField&>(fd));

   fd.arrayLength = j.at("arrayLength").is_null() ? 0 : static_cast<uint32_t>(j.at("arrayLength"));
   fd.fieldSize = fd.arrayLength * parse_fields(j.at("fields"), fd.fields);
}

//-----------------------------------------------------------------------
void from_json(const json& j, MessageDefinition& md)
{
   md._id = j.at("_id");
   md.logID = j.at("messageID"); // this was "logID"
   md.name = j.at("name");
   md.description = j.at("description").is_null() ? "" : j.at("description");
   md.latestMessageCrc = std::stoul(j.at("latestMsgDefCrc").get<std::string>());

   for (const auto& fields : j.at("fields").items()) {
      uint32_t defCRC = std::stoul(fields.key());
      md.fields[defCRC];
      parse_fields(fields.value(), md.fields[defCRC]);
   }
}

//-----------------------------------------------------------------------
void from_json(const json& j, EnumDefinition& ed)
{
   ed._id = j.at("_id");
   ed.name = j.at("name");
   parse_enumerators(j.at("enumerators"), ed.enumerators);
}

//-----------------------------------------------------------------------
uint32_t parse_fields(const json& j, std::vector<novatel::edie::BaseField*>& vFields)
{
   uint32_t uiFieldSize = 0;
   for (const auto& field : j)
   {
      std::string sFieldType = field.at("type").get<std::string>();
      novatel::edie::BaseDataType stDataType = field.at("dataType").get<novatel::edie::BaseDataType>();

      if (sFieldType == "SIMPLE")
      {
         novatel::edie::BaseField* pstField = new novatel::edie::BaseField;
         *pstField = field;
         vFields.push_back(pstField);
         uiFieldSize += stDataType.length;
      }
      else if (sFieldType == "ENUM")
      {
         novatel::edie::EnumField* pstField = new novatel::edie::EnumField;
         *pstField = field;
         pstField->length = stDataType.length;
         vFields.push_back(pstField);
         uiFieldSize += stDataType.length;
      }
      else if (sFieldType == "FIXED_LENGTH_ARRAY" || sFieldType == "VARIABLE_LENGTH_ARRAY" || sFieldType == "STRING")
      {
         novatel::edie::ArrayField* pstField = new novatel::edie::ArrayField;
         *pstField = field;
         vFields.push_back(pstField);
         uint32_t uiArrayLength = field.at("arrayLength").get<uint32_t>();
         uiFieldSize += (stDataType.length * uiArrayLength);
      }
      else if (sFieldType == "FIELD_ARRAY")
      {
         novatel::edie::FieldArrayField* pstField = new novatel::edie::FieldArrayField;
         *pstField = field;
         vFields.push_back(pstField);
      }
      else
         throw std::runtime_error("Could not find field type");
   }
   return uiFieldSize;
}

//-----------------------------------------------------------------------
void parse_enumerators(const json& j, std::vector<novatel::edie::EnumDataType>& vEnumerators)
{
   for (const auto& enumerator : j)
   {
      vEnumerators.push_back(enumerator);
   }
}

}

//-----------------------------------------------------------------------
template<typename T>
void JsonReader::LoadFile(T filePath)
{
   try
   {
      std::fstream json_file;
      json_file.open(std::filesystem::path(filePath), std::ios::in);
      json jDefinitions = json::parse(json_file);

      vMessageDefinitions.clear();
      for (const auto& msg : jDefinitions["messages"])
      {
         vMessageDefinitions.push_back(msg);  // The JSON object is converted to a MessageDefinition object here
      }

      vEnumDefinitions.clear();
      for (const auto& enm : jDefinitions["enums"])
      {
         vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
      }

      GenerateMappings();
   }
   catch (std::exception& e)
   {
      throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath, e.what());
   }
}

//-----------------------------------------------------------------------
template <>
void JsonReader::LoadFile<std::string>(std::string filePath)
{
   try
   {
      std::fstream json_file;
      json_file.open(filePath, std::ios::in);
      json jDefinitions = json::parse(json_file);

      vMessageDefinitions.clear();

      for (auto& msg : jDefinitions["messages"])
      {
         vMessageDefinitions.push_back(msg);  // The JSON object is converted to a MessageDefinition object here
      }

      vEnumDefinitions.clear();
      for (auto& enm : jDefinitions["enums"])
      {
         vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
      }

      GenerateMappings();
   }
   catch (std::exception& e)
   {
      throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath.c_str(), e.what());
   }
}

//-----------------------------------------------------------------------
template<typename T>
void JsonReader::AppendMessages(T filePath_)
{
   try
   {
      std::fstream json_file;
      json_file.open(std::filesystem::path(filePath_));
      json jDefinitions = json::parse(json_file);

      for (const auto& msg : jDefinitions["messages"])
      {
         const novatel::edie::MessageDefinition msgdef = msg;

         RemoveMessage(msgdef.logID, false);

         vMessageDefinitions.push_back(msg);  // The JSON object is converted to a MessageDefinition object here
      }

      for (const auto& enm : jDefinitions["enums"])
      {
         const novatel::edie::EnumDefinition enmdef = enm;

         RemoveEnumeration(enmdef.name, false);

         vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
      }

      GenerateMappings();
   }
   catch (std::exception& e)
   {
      throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
   }
}

//-----------------------------------------------------------------------
template<typename T>
void JsonReader::AppendEnumerations(T filePath_)
{
   try
   {
      std::fstream json_file;
      json_file.open(std::filesystem::path(filePath_));
      json jDefinitions = json::parse(json_file);

      for (const auto& enm : jDefinitions["enums"])
      {
         vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
      }

      GenerateMappings();
   }
   catch (std::exception& e)
   {
      throw novatel::edie::JsonReaderFailure(__func__, __FILE__, __LINE__, filePath_, e.what());
   }
}

// explicit template instantiations
template void JsonReader::LoadFile<std::u32string>(std::u32string filePath);
template void JsonReader::LoadFile<char*>(char* filePath);

template void JsonReader::AppendMessages<std::string>(std::string filePath);
template void JsonReader::AppendMessages<std::u32string>(std::u32string filePath);
template void JsonReader::AppendMessages<char*>(char* filePath);

template void JsonReader::AppendEnumerations<std::string>(std::string filePath);
template void JsonReader::AppendEnumerations<std::u32string>(std::u32string filePath);
template void JsonReader::AppendEnumerations<char*>(char* filePath);

//-----------------------------------------------------------------------
template<typename T>
void JsonReader::RemoveMessage(T iMsgId_, bool bGenerateMappings_)
{
   std::vector<novatel::edie::MessageDefinition>::iterator iTer;
   
   iTer = GetMessageIt(iMsgId_);

   if (iTer != vMessageDefinitions.end())
   {
      RemoveMessageMapping(*iTer);
      vMessageDefinitions.erase(iTer);
   }

   if(bGenerateMappings_)
      GenerateMappings();
}

//-----------------------------------------------------------------------
template<typename T>
void JsonReader::RemoveEnumeration(T strEnumeration_, bool bGenerateMappings_)
{
   std::vector<novatel::edie::EnumDefinition>::iterator iTer;
   iTer = GetEnumIt(strEnumeration_);

   if (iTer != vEnumDefinitions.end())
   {
      RemoveEnumerationMapping(*iTer);
      vEnumDefinitions.erase(iTer);
   }

   if(bGenerateMappings_)
      GenerateMappings();
}

template void JsonReader::RemoveMessage<uint32_t>(uint32_t iMsgId_, bool bGenerateMappings_);
template void JsonReader::RemoveMessage<const std::string&>(const std::string& strMessage_, bool bGenerateMappings_);
template void JsonReader::RemoveEnumeration<const std::string&> (const std::string&, bool bGenerateMappings_);

//-----------------------------------------------------------------------
void JsonReader::ParseJson(const std::string& strJsonData_)
{
   json jDefinitions = json::parse(strJsonData_);

   vMessageDefinitions.clear();
   for (const auto& msg : jDefinitions["logs"])
   {
      vMessageDefinitions.push_back(msg);  // The JSON object is converted to a MessageDefinition object here
   }

   vEnumDefinitions.clear();
   for (const auto& enm : jDefinitions["enums"])
   {
      vEnumDefinitions.push_back(enm); // The JSON object is converted to a EnumDefinition object here
   }

   GenerateMappings();
}

//-----------------------------------------------------------------------
const novatel::edie::MessageDefinition* JsonReader::GetMsgDef(const std::string& strMsgName_)
{
   const auto it = mMessageName.find(strMsgName_);
   return it != mMessageName.end() ? it->second : nullptr;
}

//-----------------------------------------------------------------------
// TODO need to look into the map and find the right crc and return the msg def for that CRC
const novatel::edie::MessageDefinition* JsonReader::GetMsgDef(int32_t iMsgID)
{
   const auto it = mMessageID.find(iMsgID);
   return it != mMessageID.end() ? it->second : nullptr;
}

//-----------------------------------------------------------------------
novatel::edie::EnumDefinition* JsonReader::GetEnumDef(const std::string& sEnumNameOrID)
{
   // Check string against name map
   auto it = mEnumName.find(sEnumNameOrID);
   if (it != mEnumName.end())
      return it->second;

   // Check string against ID map
   it = mEnumID.find(sEnumNameOrID);
   if (it != mEnumID.end())
      return it->second;

   return nullptr;
}
