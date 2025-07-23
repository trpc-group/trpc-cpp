//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <map>
#include <optional>
#include <string>

#include "google/protobuf/message.h"
#include "json/json.h"
#include "yaml-cpp/yaml.h"

#include "trpc/common/config/yaml_parser.h"
#include "trpc/config/trpc_conf.h"
#include "trpc/util/log/logging.h"

namespace trpc::config {

/// This method is provided for compatibility with the legacy framework configuration loading method,
/// but is not recommended for use.
/// Usage: first get the configuration content, then use the specific interface to parse it:
/// For example, auto yaml_result = trpc::config::LoadConfig<YAML::Node>(my_provider, myconf.conf);
/// @brief Fetches the configuration from the configuration center.
/// @param plugin_name (old)The name of the configuration plugin.
/// @param config_name (old)The name of the configuration file to be loaded.
/// @param params Any type of expandable parameters.
/// @return Returns the configuration in the format of the template type T, such as Json::Value or std::map.
template <typename T>
std::optional<T> LoadConfig(const std::string& plugin_name, const std::string& config_name,
                            const std::any& params = nullptr) {
  std::optional<T> ret;
  // Determine the type of support
  if constexpr (std::negation_v<std::is_same<T, Json::Value>> && std::negation_v<std::is_same<T, YAML::Node>> &&
                std::negation_v<std::is_same<T, std::map<std::string, std::string>>>) {
    TRPC_LOG_ERROR("Unknown type!");
    return ret;
  }

  DefaultConfigPtr config = Load(config_name, trpc::config::WithProvider(plugin_name));
  if (!config) {
    TRPC_FMT_ERROR("get config fail, plugin_name:{}", plugin_name);
  }

  T dest_form;
  std::string raw_data = config->GetRawData();
  TRPC_FMT_DEBUG("raw_config:{} ", raw_data);
  if (!TransformConfig(raw_data, &dest_form)) {
    return ret;
  }
  ret = std::move(dest_form);

  return ret;
}

/// @brief Return the original string format obtained from the configuration center
/// @param from The original string corresponding to the configuration
/// @param to The target string
/// @return bool true: Conversion succeeded
///              false: Conversion failed
bool TransformConfig(const std::string& from, std::string* to);

/// @brief Convert the original string format obtained from the configuration center to JSON format
/// @param from The original string corresponding to the configuration
/// @param json_doc The configuration in JSON format
/// @return bool true: Conversion succeeded
///              false: Conversion failed
bool TransformConfig(const std::string& from, Json::Value* json_value);

/// @brief Convert the original string format obtained from the configuration center to YAML format
/// @param from The original string corresponding to the configuration
/// @param node The configuration in YAML format
/// @return bool true: Conversion succeeded
///              false: Conversion failed
bool TransformConfig(const std::string& from, YAML::Node* node);

/// @brief Convert the original string format obtained from the configuration center to map format
/// @param from The original string corresponding to the configuration
/// @param config_map The configuration in map format
/// @return bool true: Conversion succeeded
///              false: Conversion failed
bool TransformConfig(const std::string& from, std::map<std::string, std::string>* config_map);

/// @brief Convert the original string format obtained from the configuration center to protobuf message format
/// @param from The original string corresponding to the configuration
/// @param pb_msg The configuration in protobuf message format
/// @return bool true: Conversion succeeded
///              false: Conversion failed
bool TransformConfig(const std::string& from, google::protobuf::Message* pb_msg);

/// @brief Convert Json::Value data to map format
/// @param from The original Json::Value configuration
/// @param config_map The converted map
/// @param parent_name The name of the parent object in the JSON, such as "a" in {"a": {"b": "c"}}
/// @return bool true: Conversion succeeded
///              false: Conversion failed
bool TransformConfig(const Json::Value& from, std::map<std::string, std::string>* config_map,
                     const std::string& parent_name);

/// @brief Convert YAML::Node data to map format
/// @param root_node The original YAML::Node configuration
/// @param config_map The converted map
/// @return bool true: Conversion succeeded
///              false: Conversion failed
bool TransformConfig(const YAML::Node& root_node, std::map<std::string, std::string>* config_map);

/// @brief Find a value in a Json::Value object based on a given key
/// @param config_json The original Json::Value configuration
/// @param key The key to search for
/// @param result The result of the search
/// @return bool true: The value was found
///              false: The value was not found
bool FindValueFromJson(const Json::Value& config_json, const std::string& key, std::string& result);

/// @brief Search for a value in a YAML::Node object based on a given key
/// @param root_node The original YAML::Node configuration
/// @param key The key to search for (can contain dots to indicate nested objects)
/// @return std::string The value of the key
std::string GetYamlValue(const YAML::Node& root_node, const std::string& key);

/// @brief Extract all the keys from a YAML::Node object and store them in a vector
/// @param node The original YAML::Node configuration
/// @param parent_name The name of the parent object in the YAML
/// @param yaml_keys The vector to store the keys in
/// @return bool true: The keys were extracted successfully
///              false: The keys could not be extracted
bool ExtractYamlKeys(const YAML::Node& node, const std::string& parent_name, std::vector<std::string>& yaml_keys);

/// @brief Search for an int64_t value in a std::map<std::string, std::string> object based on a given key
/// @param config_map The original std::map<std::string, std::string> configuration
/// @param key The key to search for
/// @param result The result of the search
/// @return bool true: The value was found
///              false: The value was not found
bool GetInt64(const std::map<std::string, std::string>& config_map, const std::string& key, int64_t* result);

/// @brief Search for an int64_t value in a Json::Value object based on a given key
/// @param config_json The original Json::Value configuration
/// @param key The key to search for
/// @param result The result of the search
/// @return bool true: The value was found
///              false: The value was not found
bool GetInt64(const Json::Value& config_json, const std::string& key, int64_t* result);

/// @brief Search for an int64_t value in a YAML::Node object based on a given key
/// @param node The original YAML::Node configuration
/// @param key The key to search for
/// @param result The result of the search
/// @return bool true: The value was found
///              false: The value was not found
bool GetInt64(const YAML::Node& node, const std::string& key, int64_t* result);

/// @brief Finds the result based on the key in the configuration of type T.
/// @param raw_config The input parameter of type T, which can be json/map.
/// @param key The key value.
/// @param default_value The default value set by the user when the key is not found.
/// @return int64_t The int64_t value found.
template <typename T>
int64_t GetInt64(const T& raw_config, const std::string& key, int64_t default_value) {
  int64_t result;
  bool ret = GetInt64(raw_config, key, &result);
  if (ret) {
    return result;
  }
  return default_value;
}

/// @brief Finds the result based on the key in the map configuration.
/// @param config_map The map configuration.
/// @param key The key value.
/// @param result The uint64_t value found.
/// @return bool true: found
///              false: not found
bool GetUint64(const std::map<std::string, std::string>& config_map, const std::string& key, uint64_t* result);

/// @brief Finds the result based on the key in the JSON configuration.
/// @param config_json The JSON configuration.
/// @param key The key value.
/// @param result The uint64_t value found.
/// @return bool true: found
///              false: not found
bool GetUint64(const Json::Value& config_json, const std::string& key, uint64_t* result);

/// @brief Finds the result based on the key in the YAML configuration.
/// @param node The YAML configuration.
/// @param key The key value.
/// @param result The uint64_t value found.
/// @return bool true: found
///              false: not found
bool GetUint64(const YAML::Node& node, const std::string& key, uint64_t* result);

/// @brief Finds the result based on the key in the configuration of type T.
/// @param raw_config The input parameter of type T, which can be json/map.
/// @param key The key value.
/// @param default_value The default value set by the user when the key is not found.
/// @return uint64_t The uint64_t value found.
template <typename T>
uint64_t GetUint64(const T& raw_config, const std::string& key, uint64_t default_value) {
  uint64_t result;
  bool ret = GetUint64(raw_config, key, &result);
  if (ret) {
    return result;
  }
  return default_value;
}

/// @brief Finds the result based on the key in the map configuration.
/// @param config_map The map configuration.
/// @param key The key value.
/// @param result The double value found.
/// @return bool true: found
///              false: not found
bool GetDouble(const std::map<std::string, std::string>& config_map, const std::string& key, double* result);

/// @brief Finds the result based on the key in the JSON configuration.
/// @param config_json The JSON configuration.
/// @param key The key value.
/// @param result The double value found.
/// @return bool true: found
///              false: not found
bool GetDouble(const Json::Value& config_json, const std::string& key, double* result);

/// @brief Finds the result based on the key in the YAML configuration.
/// @param node The YAML configuration.
/// @param key The key value.
/// @param result The double value found.
/// @return bool true: found
///              false: not found
bool GetDouble(const YAML::Node& node, const std::string& key, double* result);

/// @brief Finds the result based on the key in the configuration of type T.
/// @param raw_config The input parameter of type T, which can be json/map.
/// @param key The key value.
/// @param default_value The default value set by the user when the key is not found.
/// @return double The double value found.
template <typename T>
double GetDouble(const T& raw_config, const std::string& key, double default_value) {
  double result;
  bool ret = GetDouble(raw_config, key, &result);
  if (ret) {
    return result;
  }
  return default_value;
}

/// @brief Finds the result based on the key in the map configuration.
/// @param config_map The map configuration.
/// @param key The key value.
/// @param result The boolean value found.
/// @return bool true: found
///              false: not found
bool GetBool(const std::map<std::string, std::string>& config_map, const std::string& key, bool* result);

/// @brief Finds the result based on the key in the JSON configuration.
/// @param config_json The JSON configuration.
/// @param key The key value.
/// @param result The boolean value found.
/// @return bool true: found
///              false: not found
bool GetBool(const Json::Value& config_json, const std::string& key, bool* result);

/// @brief Finds the result based on the key in the YAML configuration.
/// @param node The YAML configuration.
/// @param key The key value.
/// @param result The boolean value found.
/// @return bool true: found
///              false: not found
bool GetBool(const YAML::Node& node, const std::string& key, bool* result);

/// @brief Finds the result based on the key in the configuration of type T.
/// @param raw_config The input parameter of type T, which can be json/map.
/// @param key The key value.
/// @param default_value The default value set by the user when the key is not found.
/// @return bool The boolean value found.
template <typename T>
bool GetBool(const T& raw_config, const std::string& key, bool default_value) {
  bool result;
  bool ret = GetBool(raw_config, key, &result);
  if (ret) {
    return result;
  }
  return default_value;
}

/// @brief Finds the result based on the key in the map configuration.
/// @param config_map The map configuration.
/// @param key The key value.
/// @param result The string value found.
/// @return bool true: found
///              false: not found
bool GetString(const std::map<std::string, std::string>& config_map, const std::string& key, std::string* result);

/// @brief Finds the result based on the key in the JSON configuration.
/// @param config_json The JSON configuration.
/// @param key The key value.
/// @param result The string value found.
/// @return bool true: found
///              false: not found
bool GetString(const Json::Value& config_json, const std::string& key, std::string* result);

/// @brief Finds the result based on the key in the YAML configuration.
/// @param node The YAML configuration.
/// @param key The key value.
/// @param result The string value found.
/// @return bool true: found
///              false: not found
bool GetString(const YAML::Node& node, const std::string& key, std::string* result);

/// @brief Finds the result based on the key in the configuration of type T.
/// @param raw_config The input parameter of type T, which can be json/map.
/// @param key The key value.
/// @param default_value The default value set by the user when the key is not found.
/// @return The string value found.
template <typename T>
std::string GetString(T& raw_config, const std::string& key, const std::string& default_value) {
  std::string result;
  bool ret = GetString(raw_config, key, &result);
  if (ret) {
    return result;
  }
  return default_value;
}

/// @brief Parses a json configuration into a table.
/// @param config The json configuration, which contains several escaped strings of real data.
/// @param result The vector of Json::Value objects to be obtained.
void GetTable(const Json::Value& config, std::vector<Json::Value>* result);

/// @brief Parses a string configuration into a table.
/// @param config The string configuration, generated by Jsoncpp's toStyledString() method.
/// @param result The vector of Json::Value objects to be obtained.
void GetTable(const std::string& config, std::vector<Json::Value>* result);

/// @brief Parses a configuration of type T into a table.
/// @param config The configuration in string format, generated by Jsoncpp's toStyledString() method.
/// @param result The vector of Json::Value objects to be obtained.
template <typename T>
std::vector<Json::Value> GetTable(const T& config) {
  std::vector<Json::Value> res;
  GetTable(config, &res);
  return res;
}

}  // namespace trpc::config
