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

#include "trpc/runtime/iomodel/reactor/event_handler.h"

namespace trpc {

using NotifierWakeupFunction = std::function<void()>;

/// @brief Base class for notify event
class Notifier : public EventHandler {
 public:
  /// @brief Enable the read event for the Notifier
  virtual void EnableNotify() = 0;

  /// @brief Disable the read event for the Notifier
  virtual void DisableNotify() = 0;

  /// @brief Wake up
  virtual void WakeUp() = 0;
};

}  // namespace trpc
