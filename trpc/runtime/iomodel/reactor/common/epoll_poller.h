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

#include "trpc/runtime/iomodel/reactor/common/epoll.h"
#include "trpc/runtime/iomodel/reactor/poller.h"

namespace trpc {

/// @brief Epoll multiplex implement
class EPollPoller final : public Poller {
 public:
  EPollPoller() : epoll_(10240) {}

  void Dispatch(int timeout_ms) override;

  void UpdateEvent(EventHandler* event_handler) override;

 private:
  // Convert specific epoll event types to defined generic event types
  uint32_t EventTypeToEvent(uint8_t event_type);

  // Convert defined generic event types to specific epoll event types
  uint8_t EventToEventType(uint32_t events);

 private:
  Epoll epoll_;
};

}  // namespace trpc
