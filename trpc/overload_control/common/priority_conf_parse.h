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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include <string>

#include "yaml-cpp/yaml.h"

namespace trpc::overload_control {

/// @brief Decode the specific field values from config yaml file.
/// @tparam T Data type of field
/// @param node Yaml node
/// @param key Field name
/// @param value Field value
template <typename T>
void DecodeField(const YAML::Node& node, const std::string& key, T& value) {
  if (node[key]) {
    value = node[key].as<T>();
  }
}

/// @brief Decode configuration required for parsing priority algorithm.
/// @tparam T Data type of field
/// @param node Yaml node
/// @param config Config struct
/// @note Since the 'high_percentile` and `throttler` algorithms are used in conjunction with the priority algorithm,
///       the common part of the priority configuration is extracted here.
template <typename T>
void Decode(const YAML::Node& node, T& config) {
  DecodeField(node, "max_priority", config.max_priority);
  DecodeField(node, "dry_run", config.dry_run);
  DecodeField(node, "is_report", config.is_report);
  DecodeField(node, "lower_step", config.lower_step);
  DecodeField(node, "upper_step", config.upper_step);
  DecodeField(node, "fuzzy_ratio", config.fuzzy_ratio);
  DecodeField(node, "max_update_interval", config.max_update_interval);
  DecodeField(node, "max_update_size", config.max_update_size);
  DecodeField(node, "histograms", config.histograms);
}

/// @brief Encode configuration required for parsing priority algorithm.
/// @tparam T Data type of field
/// @param node Yaml node
/// @param config Config struct
/// @note Since the 'high_percentile` and `throttler` algorithms are used in conjunction with the priority algorithm,
///       the common part of the priority configuration is extracted here.
template <typename T>
void Encode(YAML::Node& node, const T& config) {
  node["max_priority"] = config.max_priority;
  node["dry_run"] = config.dry_run;
  node["is_report"] = config.is_report;
  node["lower_step"] = config.lower_step;
  node["upper_step"] = config.upper_step;
  node["fuzzy_ratio"] = config.fuzzy_ratio;
  node["max_update_interval"] = config.max_update_interval;
  node["max_update_size"] = config.max_update_size;
  node["histograms"] = config.histograms;
}

}  // namespace trpc::overload_control

#endif
