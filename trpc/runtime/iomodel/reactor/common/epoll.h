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

#pragma once

#include <sys/epoll.h>

#include <memory>

#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief Encapsulation implementation of linux epoll
class Epoll {
 public:
  explicit Epoll(uint32_t max_events);

  ~Epoll();

  /// @brief  Add fd and its events and data to epoll
  void Add(int fd, uint64_t data, uint32_t event);

  /// @brief Modify fd and its events and data to epoll
  void Mod(int fd, uint64_t data, uint32_t event);

  /// @brief Remove fd and its events and data from epoll
  void Del(int fd, uint64_t data, uint32_t event);

  /// @brief Execute epoll_wait to wait for events
  int Wait(int millsecond);

  /// @brief Get the triggered event by epoll
  struct epoll_event& Get(uint32_t i) {
    TRPC_ASSERT(i <= max_events_);
    return events_[i];
  }

 private:
  void Ctrl(int fd, uint64_t data, uint32_t events, int op);

 private:
  int epoll_fd_;

  // The maximum number of events passed in each execution of epoll_wait
  uint32_t max_events_;

  // Collection of events
  std::unique_ptr<struct epoll_event[]> events_;
};

}  // namespace trpc
