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

#include "trpc/runtime/iomodel/reactor/event_handler.h"

namespace trpc {

void EventHandler::HandleEvent() {
  if (recv_events_ & EventType::kReadEvent) {
    if (HandleReadEvent() != 0) {
      return;
    }
  }

  if (recv_events_ & EventType::kWriteEvent) {
    if (HandleWriteEvent() != 0) {
      return;
    }
  }

  if (recv_events_ & EventType::kCloseEvent) {
    HandleCloseEvent();
  }
}

void EventHandler::EnableEvent(uint8_t event_type) {
  if (event_type & EventType::kReadEvent) {
    set_events_ |= EventType::kReadEvent;
  }

  if (event_type & EventType::kWriteEvent) {
    set_events_ |= EventType::kWriteEvent;
  }

  set_events_ |= EventType::kCloseEvent;
}

void EventHandler::DisableEvent(uint8_t event_type) {
  if (event_type & EventType::kReadEvent) {
    set_events_ &= ~EventType::kReadEvent;
  }

  if (event_type & EventType::kWriteEvent) {
    set_events_ &= ~EventType::kWriteEvent;
  }
}

void EventHandler::DisableAllEvent() {
  set_events_ = 0;
}

}  // namespace trpc
