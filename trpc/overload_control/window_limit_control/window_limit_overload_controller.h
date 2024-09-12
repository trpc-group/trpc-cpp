//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <unordered_map>

#include "trpc/util/function.h"
#include "trpc/overload_control/flow_control/smooth_limiter.h"
#include "trpc/overload_control/flow_control/flow_controller.h"
#include "trpc/overload_control/window_limit_control/server_overload_controller.h"

namespace trpc::overload_control {
  
const char WindowLimitOverloadControllerName[] = "WindowLimitOverloadController";

/// @brief Default number of time frames per second
constexpr int32_t kDefaultNumber = 100;

class WindowLimitOverloadController : public ServerOverloadController {
 public:
  std::string Name() { return WindowLimitOverloadControllerName; }

  /// @brief Initialize the sliding window current limiting plugin
  /// @param Name of plugin Current limit quantity Record monitoring logs or not Number of time slots
  explicit WindowLimitOverloadController();

  /// @note Do nothing, the entire plugin requires manual destruction of only thread resources
  /// @brief From the implementation of filter, it can be seen that before destruction
  /// the plugin's stop() and destroy() will be called first
  ~WindowLimitOverloadController();

  /// @note Func called before onrequest() is called at the buried point location, and checkpoint() is called internally
  /// @param Context represents the storage of status within the context service name、caller name
  /// @param By matching the factory object (singleton) with the name information, specific plugin strategies can be
  /// found
  bool BeforeSchedule(const ServerContextPtr& context);

  /// @note There is no need to call funcs for burying points here
  bool AfterSchedule(const ServerContextPtr& context) { return true; }

  ///@brief Initialize thread resources
  bool Init();

  /// @brief End the 'loop' of the scheduled thread while making it 'joinable'
  void Stop();

  /// @brief Destroy thread
  void Destroy();

  static WindowLimitOverloadController* GetInstance() {
    static WindowLimitOverloadController instance;
    return &instance;
  }

  void RegisterLimiter(const std::string& name, FlowControllerPtr limiter);

  FlowControllerPtr GetLimiter(const std::string& name);

 private:
  std::unordered_map<std::string, FlowControllerPtr> smooth_limits_;
};
}  // namespace trpc::overload_control
#endif
