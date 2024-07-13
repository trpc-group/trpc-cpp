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

#include <chrono>
#include <memory>
#include <optional>

#include "trpc/overload_control/server_overload_controller.h"
#include "trpc/overload_control/common/priority_adapter.h"
#include "trpc/server/server_context.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::overload_control {

/// @brief Overload protection controller based on token bucket algorithm.
class TokenBucketOverloadController : public ServerOverloadController {
public:
  /// @brief Name of controller
  std::string Name() override { return "TokenBucketOverloadController" };

  /// @brief Initialization function.
  bool Init() override;

  /// @brief Register the controller plugin by the conf.
  void Register(const TokenBucketLimiterControlConf& conf);

  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

private:
  void refill();

private:
  uint32 capacity;
  uint32 rate;
  AtomicType<uint32> current_token;  ///< Current token count in token bucket.
  std::thread refill_thread;
  bool stop;
};

using TokenBucketOverloadControllerPtr = std::shared_ptr<TokenBucketOverloadController>;

}  // namespace trpc::overload_control

#endif