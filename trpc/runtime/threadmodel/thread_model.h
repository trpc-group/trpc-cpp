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

#pragma once

#include <string>
#include <string_view>

#include "trpc/runtime/threadmodel/common/msg_task.h"

namespace trpc {

constexpr std::string_view kSeparate = "separate";
constexpr std::string_view kMerge = "merge";
constexpr std::string_view kFiber = "fiber";

// for compatible
constexpr std::string_view kDefault = "default";

/// @brief the name of the threading model used by admin service
constexpr std::string_view kSeparateAdminInstance = "trpc_separate_admin_instance";

/// @brief Base class for implement a threading model.
class ThreadModel {
 public:
  virtual ~ThreadModel() = default;

  /// @brief the group id of the current threading model
  virtual uint16_t GroupId() const = 0;

  /// @brief the type of the current threading model
  virtual std::string_view Type() const = 0;

  /// @brief The group name of the current threading model
  virtual std::string GroupName() const = 0;

  /// @brief initialize and start the current threading model
  virtual void Start() noexcept = 0;

  /// @brief stop and destroy the current threading model
  virtual void Terminate() noexcept = 0;

  /// @brief Submit handle task
  /// @note handle_task is allocated by object pool
  virtual bool SubmitHandleTask(MsgTask* handle_task) noexcept { return false; }
};

}  // namespace trpc
