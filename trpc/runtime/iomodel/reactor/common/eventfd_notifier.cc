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

#include "trpc/runtime/iomodel/reactor/common/eventfd_notifier.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include "trpc/util/log/logging.h"

namespace trpc {

EventFdNotifier::EventFdNotifier(Reactor* reactor)
    : reactor_(reactor),
      fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      enable_(false) {
  TRPC_ASSERT(reactor_);
  TRPC_ASSERT(fd_ > 0);
  SetFd(fd_);
}

EventFdNotifier::~EventFdNotifier() {
  TRPC_ASSERT(!enable_ && "EventFdNotifier enabled");

  ::close(fd_);
}

void EventFdNotifier::EnableNotify() {
  if (enable_) {
    return;
  }

  EnableEvent(EventHandler::EventType::kReadEvent);

  Ref();

  reactor_->Update(this);

  enable_ = true;
}

void EventFdNotifier::DisableNotify() {
  if (!enable_) {
    return;
  }

  enable_ = false;

  DisableEvent(EventHandler::EventType::kReadEvent);

  reactor_->Update(this);

  Deref();
}

void EventFdNotifier::WakeUp() {
  uint64_t value = 1;

  ssize_t n = write(fd_, &value, sizeof(value));

  TRPC_ASSERT(n == sizeof(n));
}

int EventFdNotifier::HandleReadEvent() {
  uint64_t value = 1;

  ssize_t n = read(fd_, &value, sizeof(value));
  TRPC_ASSERT(n == sizeof(n));

  if (wakeup_function_) {
    wakeup_function_();
  }

  return 0;
}

}  // namespace trpc
