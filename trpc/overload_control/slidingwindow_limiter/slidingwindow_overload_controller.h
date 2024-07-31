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

class SlidingWindowOverloadController : public ServerOverloadController {
 public:

  /// @brief 
  SlidingWindowOverloadController(size_t max_requests, std::chrono::seconds window_duration);

  // ~SlidingWindowOverloadController() override;

  std::string Name() override;

  bool Init() override;

  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

 private:
  size_t max_requests_;
  std::chrono::seconds window_duration_;
  std::deque<std::chrono::steady_clock::time_point> timestamps_;
  std::mutex mutex_;
};

using SlidingWindowOverloadControllerPtr = std::shared_ptr<SlidingWindowOverloadController>;

}  // namespace trpc::overload_control

#endif