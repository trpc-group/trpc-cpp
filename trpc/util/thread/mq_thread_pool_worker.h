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
#include <random>

#include "trpc/util/thread/mq_thread_pool_task.h"
#include "trpc/util/thread/predicate_notifier.h"
#include "trpc/util/thread/unbounded_spmc_queue.h"

namespace trpc {

class MQThreadPool;

/// @brief The worker of MQThreadPool
class MQThreadPoolWorker {
  friend class MQThreadPool;

 private:
  size_t id_;
  size_t vtm_;
  MQThreadPool* mq_thread_pool_;
  PredicateNotifier::Waiter* waiter_{nullptr};
  std::default_random_engine rdgen_{std::random_device {}()};
  UnboundedSPMCQueue<MQThreadPoolTask*> task_queue_;
};

struct PerThreadWorker {
  MQThreadPoolWorker* worker{nullptr};

  PerThreadWorker() : worker(nullptr) {}

  PerThreadWorker(const PerThreadWorker&) = delete;
  PerThreadWorker(PerThreadWorker&&) = delete;

  PerThreadWorker& operator=(const PerThreadWorker&) = delete;
  PerThreadWorker& operator=(PerThreadWorker&&) = delete;
};

inline PerThreadWorker& ThisWorker() {
  thread_local PerThreadWorker worker;
  return worker;
}

}  // namespace trpc
