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

#include <string>
#include "trpc/overload_control/flow_control/seconds_limiter.h"
#include "trpc/overload_control/server_overload_controller.h"

namespace trpc::overload_control {
/// @brief Default number of time frames per second
constexpr int32_t DefaultSecondsWindowSize = 10;
class SecondsOverloadController : public ServerOverloadController {
 public:
  using SecondsLimiterPtr = std::shared_ptr<SecondsLimiter>;
  /// @brief
  SecondsOverloadController(std::string name, int64_t limit, bool is_report = false,
                            int32_t window_size = DefaultSecondsWindowSize);

  // ~SecondsOverloadController() override;

  std::string Name() { return name_; };

  bool Init() { return true; };

  // 被拦截为true，不拦截为false
  bool BeforeSchedule(const ServerContextPtr& context) override;

  bool AfterSchedule(const ServerContextPtr& context) override;

  void Stop() override;

  void Destroy() override;

 private:
  std::string name_;
  SecondsLimiterPtr seconds_limiter_;
};

using SecondsOverloadControllerPtr = std::shared_ptr<SecondsOverloadController>;

}  // namespace trpc::overload_control

#endif