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

#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_conf.h"
#include "trpc/overload_control/server_overload_controller.h"
#include "trpc/server/server_context.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/tvar/common/atomic_type.h"
#include "trpc/util/time.h"

namespace trpc::overload_control {

/// @brief Overload protection controller based on token bucket algorithm.
class TokenBucketOverloadController : public ServerOverloadController {
public:
  TokenBucketOverloadController(const TokenBucketLimiterControlConf& conf);

  /// @brief Name of controller
  std::string Name() override { return "TokenBucketOverloadController"; };

  /// @brief Initialization function.
  bool Init() override;

  /// @brief Register the controller plugin by the conf.
  void Register(const TokenBucketLimiterControlConf& conf);

  bool CheckLimit(uint32_t current_concurrency);

  void AddToken();

  void ConsumeToken(uint32_t consume_count);

  uint64_t GetBeginTimeStamp();

  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

private:
  void refill();

public:
  uint32_t capacity;
  uint32_t rate;
  uint32_t current_token;  ///< Current token count in token bucket.
  uint64_t begin_timestamp;	///< System begin timestamp(millisecond).
};

using TokenBucketOverloadControllerPtr = std::shared_ptr<TokenBucketOverloadController>;

}  // namespace trpc::overload_control

#endif
