#pragma once

#include <any>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <functional>

#include "json/json.h"
#include "toml.hpp"
#include "yaml-cpp/yaml.h"

namespace trpc::config::codec {

/// @brief Processes an int64_t value and returns it as an appropriate narrower integer type.
/// This function takes an int64_t value and checks its range to determine if it can be
/// represented by a narrower integer type (uint32_t, int32_t, or uint64_t). It then returns
/// the value as the appropriate narrower type wrapped in a std::any object.
/// @param value The int64_t value to be processed.
/// @return A std::any object containing the value as a narrower integer type, if possible.
std::any ProcessInteger(int64_t value) {
  // Check if the input value is non-negative.
  if (value >= 0) {
    // If the value is within the range of uint32_t, return it as uint32_t.
    if (value <= std::numeric_limits<uint32_t>::max()) {
      return static_cast<uint32_t>(value);
    } else {
      // Otherwise, return it as uint64_t.
      return static_cast<uint64_t>(value);
    }
  } else {
    // If the value is within the range of int32_t, return it as int32_t.
    if (value >= std::numeric_limits<int32_t>::min() && value <= std::numeric_limits<int32_t>::max()) {
      return static_cast<int32_t>(value);
    } else {
      // Otherwise, return the value as-is (int64_t).
      return value;
    }
  }
}

/// @brief Processes a Json::Value node and stores the result in the output map.
/// @param node The Json::Value node to be processed.
/// @param prefix The current key prefix for the node.
/// @param out The output map where the processed values are stored.
void ProcessValue(const Json::Value& node, const std::string& prefix, std::unordered_map<std::string, std::any>& out) {
  if (node.isInt64() || node.isUInt64()) {
    int64_t value = node.isUInt64() ? static_cast<int64_t>(node.asUInt64()) : node.asInt64();
    out[prefix] = ProcessInteger(value);
  } else if (node.isDouble()) {
    out[prefix] = node.asDouble();
  } else if (node.isBool()) {
    out[prefix] = node.asBool();
  } else if (node.isString()) {
    out[prefix] = node.asString();
  } else {
    TRPC_FMT_ERROR("unsupported value type at prefix '{}', codec: json", prefix);
  }
}

/// @brief Processes a toml::value node and stores the result in the output map.
/// @param node The toml::value node to be processed.
/// @param prefix The current key prefix for the node.
/// @param out The output map where the processed values are stored.
void ProcessValue(const toml::value& node, const std::string& prefix, std::unordered_map<std::string, std::any>& out) {
  if (node.is_integer()) {
    int64_t value = node.as_integer();
    out[prefix] = ProcessInteger(value);
  } else if (node.is_floating()) {
    out[prefix] = static_cast<float>(node.as_floating());
  } else if (node.is_boolean()) {
    out[prefix] = static_cast<bool>(node.as_boolean());
  } else if (node.is_string()) {
    out[prefix] = static_cast<std::string>(node.as_string());
  } else {
    TRPC_FMT_ERROR("unsupported value type at prefix '{}', codec: toml", prefix);
  }
}

/// @brief Processes a YAML::Node and stores the result in the output map.
/// @param node The YAML::Node to be processed.
/// @param prefix The current key prefix for the node.
/// @param out The output map where the processed values are stored.
void ProcessValue(const YAML::Node& node, const std::string& prefix, std::unordered_map<std::string, std::any>& out) {
  if (node.IsScalar()) {
    try {
      int64_t value = node.as<int64_t>();
      out[prefix] = ProcessInteger(value);
    } catch (...) {
      try {
        out[prefix] = node.as<double>();
      } catch (...) {
        try {
          out[prefix] = node.as<bool>();
        } catch (...) {
          out[prefix] = node.as<std::string>();
        }
      }
    }
  } else {
    TRPC_FMT_ERROR("unsupported value type at prefix '{}', codec: yaml", prefix);
  }
}

/// @brief Recursively processes a Json::Value node and stores the results in the output map.
/// @param node The Json::Value node to be processed.
/// @param prefix The current key prefix for the node.
/// @param out The output map where the processed values are stored.
void ProcessNode(const Json::Value& node, const std::string& prefix, std::unordered_map<std::string, std::any>& out) {
  if (node.isObject()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      std::string key = it.key().asString();
      ProcessNode(*it, prefix.empty() ? key : prefix + "." + key, out);
    }
  } else if (node.isArray()) {
    for (size_t i = 0; i < node.size(); ++i) {
      ProcessNode(node[static_cast<Json::Value::ArrayIndex>(i)], prefix + "[" + std::to_string(i) + "]", out);
    }
  } else {
    ProcessValue(node, prefix, out);
  }
}

/// @brief Recursively processes a toml::value node and stores the results in the output map.
/// @param node The toml::value node to be processed.
/// @param prefix The current key prefix for the node.
/// @param out The output map where the processed values are stored.
void ProcessNode(const toml::value& node, const std::string& prefix, std::unordered_map<std::string, std::any>& out) {
  if (node.is_table()) {
    const toml::table& tbl = node.as_table();
    for (const auto& [key, value_node] : tbl) {
      ProcessNode(value_node, prefix.empty() ? key : prefix + "." + key, out);
    }
  } else if (node.is_array()) {
    const toml::array& arr = node.as_array();
    for (size_t i = 0; i < arr.size(); ++i) {
      ProcessNode(arr.at(i), prefix + "[" + std::to_string(i) + "]", out);
    }
  } else {
    ProcessValue(node, prefix, out);
  }
}

/// @brief Recursively processes a YAML::Node and stores the results in the output map.
/// @param node The YAML::Node to be processed.
/// @param prefix The current key prefix for the node.
/// @param out The output map where the processed values are stored.
void ProcessNode(const YAML::Node& node, const std::string& prefix, std::unordered_map<std::string, std::any>& out) {
  if (node.IsMap()) {
    for (const auto& kv : node) {
      std::string key = kv.first.as<std::string>();
      ProcessNode(kv.second, prefix.empty() ? key : prefix + "." + key, out);
    }
  } else if (node.IsSequence()) {
    for (size_t i = 0; i < node.size(); ++i) {
      ProcessNode(node[i], prefix + "[" + std::to_string(i) + "]", out);
    }
  } else {
    ProcessValue(node, prefix, out);
  }
}

}
