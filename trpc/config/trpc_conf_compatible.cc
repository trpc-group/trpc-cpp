//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/config/trpc_conf_compatible.h"

#include <utility>
#include <vector>

#include "trpc/util/string_util.h"

namespace trpc {

bool ConvertField(const std::string& value_str, bool* value) {
  if (strcasecmp(value_str.c_str(), "true") == 0) {
    *value = true;
    return true;
  } else if (strcasecmp(value_str.c_str(), "false") == 0) {
    *value = false;
    return true;
  }
  return false;
}

bool ConvertField(const std::string& value_str, int* value) {
  try {
    *value = std::stoi(value_str);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool ConvertField(const std::string& value_str, int64_t* value) {
  try {
    *value = std::stoll(value_str);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool ConvertField(const std::string& value_str, uint32_t* value) {
  try {
    *value = std::stoul(value_str);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool ConvertField(const std::string& value_str, uint64_t* value) {
  try {
    *value = std::stoull(value_str);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool ConvertField(const std::string& value_str, float* value) {
  try {
    *value = std::stof(value_str);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool ConvertField(const std::string& value_str, double* value) {
  try {
    *value = std::stod(value_str);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

template <typename T>
bool ConvertField(const std::string& key, const std::string& value_str, T* value) {
  if (!ConvertField(value_str, value)) {
    TRPC_LOG_ERROR("Value conversion failed with key: " << key << " with value: " << value_str);
    return false;
  }
  return true;
}

bool PopulateConfigField(const google::protobuf::FieldDescriptor& field_desc, const std::string& field_name,
                         const std::string& field_value, google::protobuf::Message* pb_msg) {
  // The reason why the code was not optimized using a map<type, handle> is that
  // functions inside a namespace cannot set a map object to store pair<type, handle>.
  switch (field_desc.cpp_type()) {
    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
      int value_int32;
      if (!ConvertField(field_name, field_value, &value_int32)) {
        return false;
      }
      pb_msg->GetReflection()->SetInt32(pb_msg, &field_desc, value_int32);
      break;

    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
      bool value_bool;
      if (!ConvertField(field_name, field_value, &value_bool)) {
        return false;
      }
      pb_msg->GetReflection()->SetBool(pb_msg, &field_desc, value_bool);
      break;

    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
      pb_msg->GetReflection()->SetString(pb_msg, &field_desc, field_value);
      break;

    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
      int64_t value_int64;
      if (!ConvertField(field_name, field_value, &value_int64)) {
        return false;
      }
      pb_msg->GetReflection()->SetInt64(pb_msg, &field_desc, value_int64);
      break;

    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
      uint32_t value_uint32;
      if (!ConvertField(field_name, field_value, &value_uint32)) {
        return false;
      }
      pb_msg->GetReflection()->SetUInt32(pb_msg, &field_desc, value_uint32);
      break;

    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
      uint64_t value_uint64;
      if (!ConvertField(field_name, field_value, &value_uint64)) {
        return false;
      }
      pb_msg->GetReflection()->SetUInt64(pb_msg, &field_desc, value_uint64);
      break;

    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
      double value_double;
      if (!ConvertField(field_name, field_value, &value_double)) {
        return false;
      }
      pb_msg->GetReflection()->SetDouble(pb_msg, &field_desc, value_double);
      break;

    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
      float value_float;
      if (!ConvertField(field_name, field_value, &value_float)) {
        return false;
      }
      pb_msg->GetReflection()->SetFloat(pb_msg, &field_desc, value_float);
      break;

    default:
      TRPC_LOG_ERROR("The protobuf field type is unsupported. Type: " << field_desc.cpp_type_name() << " Key: "
                                                                      << field_name << " Value: " << field_value);
      break;
  }

  return true;
}

namespace config {

bool TransformConfig(const std::string& from, std::string* to) {
  *to = from;
  return true;
}

bool TransformConfig(const std::string& from, std::map<std::string, std::string>* config_map) {
  Json::CharReaderBuilder builder;
  Json::Value array_value;
  std::string err_msg;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  try {
    if (!reader->parse(from.c_str(), from.c_str() + from.length(), &array_value, &err_msg)) {
      TRPC_FMT_ERROR("json parse error. content:{} ,error:{}", from, err_msg);
      return false;
    }

    // The configuration center's key-value type configuration returns a string,
    // which must be parsed into a JSON array type.
    if (array_value.type() != Json::arrayValue) {
      TRPC_LOG_ERROR("json type is not an array");
      return false;
    }

    // The elements in the array must be of object type, such as
    // [{"key":"bool_config_item_1","value":"true","remark":""}].
    for (Json::Value::ArrayIndex i = 0; i < array_value.size(); i++) {
      if (array_value[i].type() != Json::objectValue) {
        TRPC_LOG_ERROR("the member of array is not object");
        return false;
      }
      Json::Value::Members member_names = array_value[i].getMemberNames();
      auto key_iter = std::find(member_names.begin(), member_names.end(), "key");
      if (key_iter == member_names.end()) {
        TRPC_LOG_ERROR("JSON object doesn't contain field named key.");
        return false;
      }
      if (!array_value[i]["key"].isString()) {
        TRPC_LOG_ERROR("JSON object is not a string.");
        return false;
      }

      auto value_iter = std::find(member_names.begin(), member_names.end(), "value");
      if (value_iter == member_names.end()) {
        TRPC_LOG_ERROR("JSON object doesn't contain field named value.");
        return false;
      }
      if (!array_value[i]["value"].isString()) {
        TRPC_LOG_ERROR("JSON object is not a string.");
        return false;
      }
      std::string key = array_value[i]["key"].asString();
      std::string value = array_value[i]["value"].asString();
      config_map->emplace(key, value);
    }
  } catch (std::exception& ex) {
    TRPC_FMT_ERROR("parse json ex:{} ", ex.what());
    return false;
  }
  return true;
}

bool TransformConfig(const std::string& from, Json::Value* json_value) {
  Json::CharReaderBuilder builder;
  std::string err_msg;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  try {
    if (!reader->parse(from.c_str(), from.c_str() + from.length(), json_value, &err_msg)) {
      TRPC_FMT_ERROR("Json parse error. content:{}, error:{}", from, err_msg);
      return false;
    }
  } catch (std::exception& ex) {
    TRPC_FMT_ERROR("parse string to json ex:{} ", ex.what());
    return false;
  }
  return true;
}

bool TransformConfig(const std::string& from, YAML::Node* node) {
  YamlParser yaml_parser;
  try {
    yaml_parser.Load(from);
    node->reset(yaml_parser.GetYAML());
  } catch (std::exception& ex) {
    TRPC_FMT_ERROR("parse string to yaml error:{} ", ex.what());
    return false;
  }
  return true;
}

bool TransformConfig(const Json::Value& from, std::map<std::string, std::string>* config_map,
                     const std::string& parent_name) {
  try {
    Json::Value::Members members = from.getMemberNames();
    for (auto it = members.begin(); it != members.end(); it++) {
      std::string key_name;
      if (!parent_name.empty()) {
        key_name = parent_name;
        key_name.append(".");
      }
      key_name.append(*it);

      // There is no map&lt; type, handle&gt; The way to optimize the code is the reason
      // Functions in the namespace cannot set a map object to store pair&lt; type, handle
      Json::ValueType value_type = from[*it].type();
      switch (value_type) {
        case Json::intValue: {
          config_map->insert(std::pair<std::string, std::string>(key_name, std::to_string(from[*it].asInt64())));
          break;
        }
        case Json::uintValue: {
          config_map->insert(std::pair<std::string, std::string>(key_name, std::to_string(from[*it].asUInt64())));
          break;
        }
        case Json::realValue: {
          config_map->insert(std::pair<std::string, std::string>(key_name, std::to_string(from[*it].asDouble())));
          break;
        }
        case Json::booleanValue: {
          if (from[*it].asBool()) {
            config_map->insert(std::pair<std::string, std::string>(key_name, "1"));
          } else {
            config_map->insert(std::pair<std::string, std::string>(key_name, "0"));
          }
          break;
        }
        case Json::stringValue:
          config_map->insert(std::pair<std::string, std::string>(key_name, from[*it].asString()));
          break;
        case Json::arrayValue:
          TRPC_FMT_ERROR("json array type is not supported, Please check it");
          break;
        case Json::objectValue:
          TransformConfig(from[*it], config_map, key_name);
          break;
        default:
          TRPC_FMT_ERROR("transform failed, unknow json type");
          return false;
      }
    }
  } catch (std::exception& ex) {
    TRPC_FMT_ERROR("parse json ex:{} ", ex.what());
    return false;
  }
  return true;
}

bool ExtractYamlKeys(const YAML::Node& node, const std::string& parent_name, std::vector<std::string>& yaml_keys) {
  for (YAML::const_iterator iter = node.begin(); iter != node.end(); ++iter) {
    std::string key_name;
    if (!parent_name.empty()) {
      key_name = parent_name;
      key_name.append(".");
    }
    std::string key = iter->first.as<std::string>();
    key_name.append(key);

    YAML::Node value = iter->second;

    // The reason why a map<type, handle> was not used to optimize the code here is that
    // functions inside a namespace cannot set a map object to store pair<type, handle>.
    switch (value.Type()) {
      case YAML::NodeType::Scalar: {
        yaml_keys.emplace_back(std::move(key_name));
        break;
      }
      case YAML::NodeType::Sequence: {
        TRPC_FMT_ERROR("yaml array type is not supported, Please check it");
        break;
      }
      case YAML::NodeType::Map: {
        ExtractYamlKeys(value, key_name, yaml_keys);
        break;
      }
      case YAML::NodeType::Null: {
        TRPC_FMT_ERROR("key:{},  type is Null", key_name);
        break;
      }
      default:
        TRPC_FMT_ERROR("extract yaml failed, unknow yaml type");
        return false;
    }
  }
  return true;
}

std::string GetYamlValue(const YAML::Node& root_node, const std::string& key) {
  std::vector<std::string> sub_keys = trpc::util::SplitString(key, '.');
  std::string result;
  try {
    // The reason why a map<int, handle> was not used to optimize the code here is that
    // functions inside a namespace cannot set a map object to store pair<int, handle>.
    switch (sub_keys.size()) {
      case 1:
        result = root_node[sub_keys[0]].as<std::string>();
        break;
      case 2:
        result = root_node[sub_keys[0]][sub_keys[1]].as<std::string>();
        break;
      case 3:
        result = root_node[sub_keys[0]][sub_keys[1]][sub_keys[2]].as<std::string>();
        break;
      case 4:
        result = root_node[sub_keys[0]][sub_keys[1]][sub_keys[2]][sub_keys[3]].as<std::string>();
        break;
      case 5:
        result = root_node[sub_keys[0]][sub_keys[1]][sub_keys[2]][sub_keys[3]][sub_keys[4]].as<std::string>();
        break;
      case 6:
        result =
            root_node[sub_keys[0]][sub_keys[1]][sub_keys[2]][sub_keys[3]][sub_keys[4]][sub_keys[5]].as<std::string>();
        break;
      case 7:
        result = root_node[sub_keys[0]][sub_keys[1]][sub_keys[2]][sub_keys[3]][sub_keys[4]][sub_keys[5]][sub_keys[6]]
                     .as<std::string>();
        break;
      case 8:
        result = root_node[sub_keys[0]][sub_keys[1]][sub_keys[2]][sub_keys[3]][sub_keys[4]][sub_keys[5]][sub_keys[6]]
                          [sub_keys[7]]
                              .as<std::string>();
        break;
      default:
        TRPC_FMT_ERROR("yaml depth more than 8 error");
        break;
    }
  } catch (const std::exception& ex) {
    TRPC_FMT_ERROR("yaml Conversion error:{}", ex.what());
  }

  return result;
}

bool TransformConfig(const YAML::Node& root_node, std::map<std::string, std::string>* config_map) {
  std::vector<std::string> yaml_keys;
  bool extract_result = ExtractYamlKeys(root_node, "", yaml_keys);
  if (!extract_result) {
    return false;
  }

  for (auto& item : yaml_keys) {
    // contains a split of the dot string, like a.b
    if (item.find('.') != std::string::npos) {
      std::string result = GetYamlValue(root_node, item);
      config_map->insert(std::pair<std::string, std::string>(item, std::move(result)));
    } else {
      std::string value = root_node[item].as<std::string>();
      config_map->insert(std::pair<std::string, std::string>(item, std::move(value)));
    }
  }
  return true;
}

bool TransformConfig(const std::string& from, google::protobuf::Message* pb_msg) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(from, &config_map)) {
    return false;
  }

  bool has_at_least_one_config_field = false;
  for (const std::pair<const std::string, std::string>& kv : config_map) {
    const google::protobuf::FieldDescriptor* field_desc = pb_msg->GetDescriptor()->FindFieldByName(kv.first);
    if (field_desc == nullptr) {
      TRPC_LOG_DEBUG("Proto message doesn't contain a field named: " << kv.first);
      continue;
    }

    if (!PopulateConfigField(*field_desc, kv.first, kv.second, pb_msg)) {
      return false;
    }

    if (!has_at_least_one_config_field) {
      has_at_least_one_config_field = true;
    }
  }

  return true;
}

bool FindValueFromJson(const Json::Value& config_json, const std::string& key, std::string& result) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(config_json, &config_map, "")) {
    return false;
  }
  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }

  result = it->second;
  return true;
}

bool GetInt64(const std::map<std::string, std::string>& config_map, const std::string& key, int64_t* result) {
  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }

  try {
    *result = std::stoll(it->second);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetInt64(const Json::Value& config_json, const std::string& key, int64_t* result) {
  std::string target_value;
  bool find_result = FindValueFromJson(config_json, key, target_value);
  if (!find_result) {
    return false;
  }

  try {
    *result = std::stoll(target_value);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetInt64(const YAML::Node& node, const std::string& key, int64_t* result) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(node, &config_map)) {
    return false;
  }

  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in node", key);
    return false;
  }

  try {
    *result = std::stoll(it->second);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetUint64(const std::map<std::string, std::string>& config_map, const std::string& key, uint64_t* result) {
  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }

  try {
    *result = std::stoull(it->second);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetUint64(const Json::Value& config_json, const std::string& key, uint64_t* result) {
  std::string target_value;
  bool get_data = FindValueFromJson(config_json, key, target_value);
  if (!get_data) {
    return false;
  }

  try {
    *result = std::stoull(target_value);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetUint64(const YAML::Node& node, const std::string& key, uint64_t* result) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(node, &config_map)) {
    return false;
  }

  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in node", key);
    return false;
  }

  try {
    *result = std::stoull(it->second);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetDouble(const std::map<std::string, std::string>& config_map, const std::string& key, double* result) {
  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }

  try {
    *result = std::stold(it->second);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetDouble(const Json::Value& config_json, const std::string& key, double* result) {
  std::string target_value;
  bool get_data = FindValueFromJson(config_json, key, target_value);
  if (!get_data) {
    return false;
  }

  try {
    *result = std::stold(target_value);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetDouble(const YAML::Node& node, const std::string& key, double* result) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(node, &config_map)) {
    return false;
  }

  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in node", key);
    return false;
  }

  try {
    *result = std::stold(it->second);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetBool(const std::map<std::string, std::string>& config_map, const std::string& key, bool* result) {
  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }

  if (it->second == "true") {
    *result = true;
  } else {
    *result = false;
  }

  return true;
}

bool GetBool(const Json::Value& config_json, const std::string& key, bool* result) {
  std::string target_value;
  bool get_data = FindValueFromJson(config_json, key, target_value);
  if (!get_data) {
    return false;
  }

  try {
    int value = std::stoi(target_value);
    if (value == 1) {
      *result = true;
    } else {
      *result = false;
    }
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetBool(const YAML::Node& node, const std::string& key, bool* result) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(node, &config_map)) {
    return false;
  }

  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in node", key);
    return false;
  }

  try {
    if (it->second == "true") {
      *result = true;
    } else {
      *result = false;
    }
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR(e.what());
    return false;
  }
  return true;
}

bool GetString(const std::map<std::string, std::string>& config_map, const std::string& key, std::string* result) {
  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }
  *result = it->second;
  return true;
}

bool GetString(const Json::Value& config_json, const std::string& key, std::string* result) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(config_json, &config_map, "")) {
    return false;
  }

  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }
  *result = it->second;
  return true;
}

bool GetString(const YAML::Node& node, const std::string& key, std::string* result) {
  std::map<std::string, std::string> config_map;
  if (!TransformConfig(node, &config_map)) {
    return false;
  }

  auto it = config_map.find(key);
  if (it == config_map.end()) {
    TRPC_FMT_ERROR("key:{} not find in map", key);
    return false;
  }
  *result = it->second;
  return true;
}

void GetTable(const std::string& config, std::vector<Json::Value>* result) {
  Json::Reader reader;
  Json::Value out;
  reader.parse(config, out);
  for (auto& row : out) {
    Json::Value row_json_value;
    if (reader.parse(row.asString(), row_json_value)) {
      result->push_back(row_json_value);
    }
  }
}

void GetTable(const Json::Value& config, std::vector<Json::Value>* result) {
  Json::Reader reader;
  for (auto& row : config) {
    Json::Value row_json_value;
    if (reader.parse(row.asString(), row_json_value)) {
      result->push_back(row_json_value);
    }
  }
}

}  // namespace config

}  // namespace trpc
