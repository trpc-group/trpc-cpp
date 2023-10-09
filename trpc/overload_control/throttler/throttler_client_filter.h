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

#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include "trpc/filter/filter.h"
#include "trpc/overload_control/overload_control_defs.h"
#include "trpc/overload_control/throttler/throttler_conf.h"
#include "trpc/overload_control/throttler/throttler_overload_controller.h"
#include "trpc/overload_control/throttler/throttler_priority_impl.h"

namespace trpc::overload_control {

/// @brief Customer overload protection filter based on downstream success rate.
class ThrottlerClientFilter : public MessageClientFilter {
 public:
  /// @brief Name of filter
  std::string Name() override { return kThrottlerName; }

  /// @brief Initialization function.
  int Init() override;

  /// @brief Get the collection of tracking points
  std::vector<FilterPoint> GetFilterPoint() override;

  /// @brief Execute the logic corresponding to the tracking point.
  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override;

 private:
  // Process requests by algorithm the result of which determine whether this request is allowed.
  void OnRequest(FilterStatus& status, const ClientContextPtr& context);

  // Process the response of current request to update algorithm data for the next request processing.
  void OnResponse(FilterStatus& status, const ClientContextPtr& context);

  // Get or create the overload protection controller instance corresponding to the IP-port.
  ThrottlerOverloadControllerPtr GetOrCreateOverloadControl(const ClientContextPtr& context);

  // Submit a periodic check and cleanup task.
  void SubmitClearTaskThreadUnsafe(const std::string& ip_port);

  // Periodic cleanup task.
  void ClearIfExpired(const std::string& ip_port);

  // Check if the instance corresponding to ip_port has expired and needs to be cleaned up.
  bool CheckIfExpiredThreadUnsafe(const std::string& ip_port);

 private:
  // Config object
  ThrottlerConf config_;

  // RWLock
  std::shared_mutex mutex_;

  // key: "${ip}:${port}"
  std::unordered_map<std::string, ThrottlerOverloadControllerPtr> controllers_;

  // key: "${ip}:${port}"
  // value: task_id
  std::unordered_map<std::string, uint64_t> task_ids_;
};

}  // namespace trpc::overload_control

#endif
