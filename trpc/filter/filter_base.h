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
#include <vector>

#include "trpc/common/plugin.h"
#include "trpc/filter/filter_point.h"
#include "trpc/filter/filter_id_counter.h"

namespace trpc {

using FilterType = PluginType;

/// @brief filter execution status
enum class FilterStatus {
  CONTINUE = 0,  ///< Continue to execute next filter
  REJECT = 1,    ///< Interrupt the execution of the filter chain
};

/// @brief Filter template base class.
template <typename... Args>
class Filter {
 public:
  Filter() { filter_id_ = GetNextFilterID(); }

  virtual ~Filter() = default;

  /// @brief Name of filter
  /// @return A string representing the name of the filter
  virtual std::string Name() = 0;

  /// @brief Collection of filter points
  /// @return A vector of FilterPoint objects
  virtual std::vector<FilterPoint> GetFilterPoint() = 0;

  /// @brief Processing function of filter
  /// @param status A FilterStatus object reference
  /// @param point A FilterPoint object
  /// @param args Expansion of Args, any further arguments needed for the filter's processing function
  virtual void operator()(FilterStatus& status, FilterPoint point, Args&&... args) = 0;

  /// @brief Retrieve the filter ID stored in filter_id_ member variable
  /// @return The filter ID associated with the filter instance
  virtual uint16_t GetFilterID() { return filter_id_; }

 private:
  // Note that filter_id is allocated in the range [10000, 65535).
  uint16_t filter_id_ = 10000;
};

}  // namespace trpc
