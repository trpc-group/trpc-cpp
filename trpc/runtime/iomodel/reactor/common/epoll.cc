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

#include "trpc/runtime/iomodel/reactor/common/epoll.h"

#include <unistd.h>

namespace trpc {

Epoll::Epoll(uint32_t max_events)
    : epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      max_events_(max_events),
      events_(std::make_unique<struct epoll_event[]>(max_events_ + 1)) {
  TRPC_ASSERT(epoll_fd_ > 0);
}

Epoll::~Epoll() {
  if (epoll_fd_ > 0) {
    ::close(epoll_fd_);
  }
}

void Epoll::Add(int fd, uint64_t data, uint32_t event) {
  Ctrl(fd, data, event, EPOLL_CTL_ADD);
}

void Epoll::Mod(int fd, uint64_t data, uint32_t event) {
  Ctrl(fd, data, event, EPOLL_CTL_MOD);
}

void Epoll::Del(int fd, uint64_t data, uint32_t event) {
  Ctrl(fd, data, event, EPOLL_CTL_DEL);
}

int Epoll::Wait(int millsecond) {
  return epoll_wait(epoll_fd_, events_.get(), max_events_ + 1, millsecond);
}

void Epoll::Ctrl(int fd, uint64_t data, uint32_t events, int op) {
  struct epoll_event ev;
  ev.data.u64 = data;
  ev.events = events | EPOLLET;  // Default ET

  epoll_ctl(epoll_fd_, op, fd, &ev);
}

}  // namespace trpc
