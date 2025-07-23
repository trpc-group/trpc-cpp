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

#include "trpc/runtime/iomodel/reactor/common/epoll_poller.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void EPollPoller::Dispatch(int timeout_ms) {
  int event_num = epoll_.Wait(timeout_ms);
  if (wait_callback_) wait_callback_(event_num);

  for (int i = 0; i < event_num; ++i) {
    const epoll_event& ev = epoll_.Get(i);

    EventHandler* event_handler = reinterpret_cast<EventHandler*>(ev.data.u64);
    TRPC_ASSERT(event_handler != nullptr);

    int recv_events = EventToEventType(ev.events);

    event_handler->SetRecvEvents(recv_events);
    event_handler->HandleEvent();
  }
}

void EPollPoller::UpdateEvent(EventHandler* event_handler) {
  uint16_t state = event_handler->GetState();
  if (state == EventHandler::EventHandlerState::kCreate) {
    uint32_t events = EventTypeToEvent(event_handler->GetSetEvents());

    epoll_.Add(event_handler->GetFd(), event_handler->GetEventData(), events);

    event_handler->SetState(EventHandler::EventHandlerState::kMod);
  } else {
    if (event_handler->HasSetEvent()) {
      uint32_t events = EventTypeToEvent(event_handler->GetSetEvents());

      epoll_.Mod(event_handler->GetFd(), event_handler->GetEventData(), events);
    } else {
      epoll_.Del(event_handler->GetFd(), event_handler->GetEventData(), 0);

      event_handler->SetState(EventHandler::EventHandlerState::kCreate);
    }
  }
}

uint32_t EPollPoller::EventTypeToEvent(uint8_t event_type) {
  uint32_t events = 0;

  if (event_type & EventHandler::EventType::kReadEvent) {
    events |= EPOLLIN;
  }

  if (event_type & EventHandler::EventType::kWriteEvent) {
    events |= EPOLLOUT;
  }

  if (event_type & EventHandler::EventType::kCloseEvent) {
    events |= EPOLLRDHUP;
  }

  return events;
}

uint8_t EPollPoller::EventToEventType(uint32_t events) {
  uint8_t recv_events = 0;

  if (events & EPOLLIN) {
    recv_events |= EventHandler::EventType::kReadEvent;
  }

  if (events & EPOLLOUT) {
    recv_events |= EventHandler::EventType::kWriteEvent;
  }

  if (events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
    recv_events |= EventHandler::EventType::kCloseEvent;
  }

  return recv_events;
}

}  // namespace trpc
