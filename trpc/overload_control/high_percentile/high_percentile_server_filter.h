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

#include <shared_mutex>

#include "trpc/filter/filter.h"
#include "trpc/overload_control/high_percentile/high_percentile_conf.h"
#include "trpc/overload_control/high_percentile/high_percentile_overload_controller.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::overload_control {

/// @brief Server-side overload protection filter based on high percentile monitoring metrics and concurrency.
class HighPercentileServerFilter : public MessageServerFilter {
 public:
  /// @brief Name of filter
  std::string Name() override { return kHighPercentileName; }

  /// @brief Initialization function.
  int Init() override;

  /// @brief Get the collection of tracking points
  std::vector<FilterPoint> GetFilterPoint() override;

  /// @brief Execute the logic corresponding to the tracking point.
  void operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) override;

 private:
  // Process requests by algorithm the result of which determine whether this request is allowed.
  void OnRequest(FilterStatus& status, const ServerContextPtr& context);

  // Process the response of current request to update algorithm data for the next request processing.
  void OnResponse(FilterStatus& status, const ServerContextPtr& context);

  // Create a corresponding controller based on the service name.
  HighPercentileOverloadControllerPtr CreateController(const std::string& service_func);

  // Get a corresponding controller based on the service name.
  HighPercentileOverloadControllerPtr& GetController(const std::string& service_func);

 private:
  // Configuration information related to overload protection.
  HighPercentileConf config_;

  // The default controller is used to handle global filter scenarios.
  HighPercentileOverloadControllerPtr default_controller_;

  // Used to protect dynamically added objects in controllers_.
  std::shared_mutex lock_;

  // Mapping between service names and controllers.
  std::unordered_map<std::string, HighPercentileOverloadControllerPtr> controllers_;
};

}  // namespace trpc::overload_control

#endif
