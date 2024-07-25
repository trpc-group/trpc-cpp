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

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include "trpc/overload_control/server_overload_controller.h"

namespace trpc::overload_control {

class FixedTimeWindowOverloadController : public ServerOverloadController {
 public:

  /// @brief 
  FixedTimeWindowOverloadController(size_t limit, std::chrono::seconds window);

  // ~FixedTimeWindowOverloadController() override;

  std::string Name() override;

  bool Init() override;

  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

 private:
  size_t limit_;  // request quota
  std::chrono::seconds window_;  // time window size
  std::atomic<size_t> request_count_;  
  std::chrono::steady_clock::time_point last_reset_time_;  // Last reset time
  std::mutex mutex_;  
};

using FixedTimeWindowOverloadControllerPtr = std::shared_ptr<FixedTimeWindowOverloadController>;

}  // namespace trpc::overload_control

#endif