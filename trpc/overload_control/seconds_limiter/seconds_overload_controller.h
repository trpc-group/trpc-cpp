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

constexpr int32_t DefaultSecondsWindowSize = 10;

/// @brief Overload controller using fixed window algorithm
class SecondsOverloadController : public ServerOverloadController {
 public:
  using SecondsLimiterPtr = std::shared_ptr<SecondsLimiter>;

  SecondsOverloadController(std::string name, int64_t limit, bool is_report = false,
                            int32_t window_size = DefaultSecondsWindowSize);
  /// @brief Name of this controller.
  std::string Name() { return name_; };

  /// @brief Initialize controller.
  /// @return bool true: succ; false: failed
  bool Init() { return true; };

  /// @brief Whether this request should be rejeted.
  ///        When reject this request, you should also set status with error code TRPC_SERVER_OVERLOAD_ERR
  ///        into context.
  /// @param context server context.
  /// @return bool true: this request should be rejected; false: this request will be handled.
  bool BeforeSchedule(const ServerContextPtr& context) override;

  /// @brief After this request being sheduled. At this point, it may be handled or rejected.
  //         You can check status from context to distinguish these 2 scenes when implement.
  /// @param context server context.
  /// @return bool true: failed; true: succ.
  bool AfterSchedule(const ServerContextPtr& context) override;

  /// @brief Stop controller.
  void Stop() override;

  /// @brief Destroy resources of controller.
  void Destroy() override;

 private:
  std::string name_;
  SecondsLimiterPtr seconds_limiter_;
};

using SecondsOverloadControllerPtr = std::shared_ptr<SecondsOverloadController>;

}  // namespace trpc::overload_control

#endif