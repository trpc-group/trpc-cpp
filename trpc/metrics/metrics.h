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

#include <any>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "trpc/common/plugin.h"

namespace trpc {

constexpr int kMetricsCallerSource = 0;
constexpr int kMetricsCalleeSource = 1;

/// @brief Metrics data for inter-module calls
struct ModuleMetricsInfo {
  /// Reporting source, where kMetricsCallerSource indicates reporting from the caller and kMetricsCalleeSource
  /// indicates reporting from the callee
  int source;

  /// Statistical information
  std::map<std::string, std::string> infos;

  /// Call's status, where 0 indicates success, 1 indicates exception, and 2 indicates timeout
  int state;

  /// Call's status code of framework
  int frame_ret_code;

  /// Call's status code of user interface
  int interface_ret_code;

  /// Call's duration
  uint32_t cost_time;

  /// Call's protocol
  std::string protocol;

  /// Extend information, can be used to provide more information when need
  std::any extend_info;
};

/// @brief Bucket for histogram statistics
using HistogramBucket = std::vector<double>;

/// @brief Quantiles for summary statistics
using SummaryQuantiles = std::vector<std::vector<double>>;

/// @brief Statistical strategy
enum MetricsPolicy {
  /// Overwrite the statistical value
  SET = 0,
  /// Calculate the sum value
  SUM = 1,
  /// Calculate the mean value
  AVG = 2,
  /// Calculate the maximum value
  MAX = 3,
  /// Calculate the minimum value
  MIN = 4,
  /// Calculate the median value
  MID = 5,
  /// Calculate the quartiles
  QUANTILES = 6,
  /// Calculate the histogram
  HISTOGRAM = 7,
};

/// @brief Metrics data with single-dimensional attribute
/// @note If the attribute label has only one key, you can directly use "name" or "dimension".
///       If the attribute label is a key-value pair, you can use "name" and "dimension" to form the key-value pair.
struct SingleAttrMetricsInfo {
  /// Metrics name
  std::string name;

  /// Metrics dimension
  std::string dimension;

  /// Metrics policy
  MetricsPolicy policy;

  /// Statistical value
  double value;

  /// The bucket used to gather histogram statistics
  HistogramBucket bucket;

  /// The quantiles used to gather summary statistics
  SummaryQuantiles quantiles;
};

/// @brief Metrics data with multi-dimensional attributes
/// @note If the attribute labels are composed of single keys, you can use the "dimensions" field.
///       If the attribute labels are composed of key-value pairs, you can use the "tags" field.
struct MultiAttrMetricsInfo {
  /// Metrics name
  std::string name;

  /// Metrics tags
  std::map<std::string, std::string> tags;

  /// Metrics dimensions
  std::vector<std::string> dimensions;

  /// Statistical values
  std::vector<std::pair<MetricsPolicy, double>> values;

  /// The bucket used to gather histogram statistics
  HistogramBucket bucket;

  /// The quantiles used to gather summary statistics
  SummaryQuantiles quantiles;
};

/// @brief The abstract interface definition for metrics plugins.
class Metrics : public Plugin {
 public:
  /// @brief Gets the plugin type
  /// @return plugin type
  PluginType Type() const override { return PluginType::kMetrics; }

  /// @brief Reports the metrics data for inter-module calls
  /// @param info metrics data
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  virtual int ModuleReport(const ModuleMetricsInfo& info) = 0;
  virtual int ModuleReport(ModuleMetricsInfo&& info) { return ModuleReport(info); }

  /// @brief Reports the metrics data with single-dimensional attribute
  /// @param info metrics data
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  virtual int SingleAttrReport(const SingleAttrMetricsInfo& info) = 0;
  virtual int SingleAttrReport(SingleAttrMetricsInfo&& info) { return SingleAttrReport(info); }

  /// @brief Reports the metrics data with multi-dimensional attributes
  /// @param info metrics data
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  virtual int MultiAttrReport(const MultiAttrMetricsInfo& info) = 0;
  virtual int MultiAttrReport(MultiAttrMetricsInfo&& info) { return MultiAttrReport(info); }
};

using MetricsPtr = RefPtr<Metrics>;

}  // namespace trpc
