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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {

/// @brief Reporting data on the overload protection judgment result of a single request.
struct OverloadInfo {
  std::string attr_name;  ///< Attribute name.
  // Reporting keywords, where the server reports the name consisting of "service" and "func",
  // while the client reports "IP" and "port".
  std::string report_name;
  // overload information.
  std::unordered_map<std::string, double> tags;
};

/// @brief Reporting data for policy metric sampling.
struct StrategySampleInfo {
  std::string report_name;    ///< Reporting the name consisting of "service" and "func".
  std::string strategy_name;  ///< Strategy name
  uint64_t value;             ///< Report data
};

/// @brief Overload protection information reporting class, which is singleton.
class Report {
 public:
  /// @brief Get singleton pointer.
  /// @return Singleton pointer pointing to Report.
  static Report* GetInstance() {
    static Report instance;
    return &instance;
  }

  Report(const Report&) = delete;
  Report& operator=(const Report&) = delete;

  /// @brief Reporting data on policy metric sampling.
  /// @param overload_infos Request information
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  void ReportOverloadInfo(const OverloadInfo& overload_infos);

  /// @brief Reporting policy metric collection data.
  /// @param sample_infos Sample information
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  void ReportStrategySampleInfo(const StrategySampleInfo& sample_infos);

 private:
  Report();

 private:
  std::vector<std::string> metrics_plugin_names_;
};

}  // namespace trpc::overload_control

#endif
