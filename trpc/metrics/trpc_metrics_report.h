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

#pragma once

#include <string>

#include "trpc/metrics/metrics.h"

namespace trpc {

/// @brief Trpc metrics data for inter-module calls
struct TrpcModuleMetricsInfo {
  /// Metrics plugin name. Specifies which plugin to report the data to.
  std::string plugin_name;
  /// Metrics data
  ModuleMetricsInfo module_info;
};

/// @brief Trpc metrics data with single-dimensional attribute
struct TrpcSingleAttrMetricsInfo {
  /// Metrics plugin name. Specifies which plugin to report the data to.
  std::string plugin_name;
  /// Metrics data
  SingleAttrMetricsInfo single_attr_info;
};

/// @brief Trpc metrics data with multi-dimensional attributes
struct TrpcMultiAttrMetricsInfo {
  /// Metrics plugin name.  Specifies which plugin to report the data to.
  std::string plugin_name;
  /// Metrics data
  MultiAttrMetricsInfo multi_attr_info;
};

/// @brief Metrics interfaces for user programing
namespace metrics {

/// @brief Report the trpc metrics data for inter-module calls
/// @param info metrics data
/// @return the reported result, where 0 indicates success and non-zero indicates failure
int ModuleReport(const TrpcModuleMetricsInfo& info);
int ModuleReport(TrpcModuleMetricsInfo&& info);

/// @brief Report the trpc metrics data with single-dimensional attribute
/// @param info metrics data
/// @return the reported result, where 0 indicates success and non-zero indicates failure
int SingleAttrReport(const TrpcSingleAttrMetricsInfo& info);
int SingleAttrReport(TrpcSingleAttrMetricsInfo&& info);

/// @brief Report the metrics data with multi-dimensional attributes
/// @param info metrics data
/// @return the reported result, where 0 indicates success and non-zero indicates failure
int MultiAttrReport(const TrpcMultiAttrMetricsInfo& info);
int MultiAttrReport(TrpcMultiAttrMetricsInfo&& info);

}  // namespace metrics

}  // namespace trpc
