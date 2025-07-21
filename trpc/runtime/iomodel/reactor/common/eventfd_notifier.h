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

#include <functional>

#include "trpc/runtime/iomodel/reactor/common/notifier.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"

namespace trpc {

/// @brief Eventfd implementation for notifier
class EventFdNotifier final : public Notifier {
 public:
  explicit EventFdNotifier(Reactor* reactor_);

  ~EventFdNotifier() override;

  /// @brief Set the execution function after wakeup
  void SetWakeupFunction(NotifierWakeupFunction&& func) {
    wakeup_function_ = std::move(func);
  }

  void EnableNotify() override;

  void DisableNotify() override;

  void WakeUp() override;

 protected:
  int HandleReadEvent() override;

 private:
  Reactor* reactor_{nullptr};

  // eventfd
  int fd_{-1};

  // whether to enable read event
  bool enable_{false};

  // execute function after notifier wakeup
  NotifierWakeupFunction wakeup_function_;
};

}  // namespace trpc
