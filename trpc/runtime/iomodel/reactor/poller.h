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

#include <memory>

#include "trpc/runtime/iomodel/reactor/event_handler.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Base class for io multiplexed event poll
///        the event data registered to poller is the EventHandler pointer
class Poller {
 public:
  using WaitCallbackFunction = Function<void(int)>;

  // ms
  static constexpr int kPollerTimeout = 5;

 public:
  virtual ~Poller() = default;

  /// @brief Wait and dispatch event
  /// @param timeout_ms Wait timeout(ms)
  virtual void Dispatch(int timeout_ms) = 0;

  /// @brief Update the event status of eventhandler
  /// @param event_handler The concrete subclass pointer of EventHandler
  virtual void UpdateEvent(EventHandler* event_handler) = 0;

  /// @brief Set the callback function to be executed immediately
  ///        after `Dispatch` waits for completion
  void SetWaitCallback(WaitCallbackFunction&& func) {
    wait_callback_ = std::move(func);
  }

 protected:
  WaitCallbackFunction wait_callback_;
};

}  // namespace trpc
