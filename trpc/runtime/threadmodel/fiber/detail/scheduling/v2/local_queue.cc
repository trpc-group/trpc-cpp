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

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v2/local_queue.h"

namespace trpc::fiber::detail::v2 {

LocalQueue::LocalQueue(int64_t c) {
  TRPC_ASSERT(c && (!(c & (c - 1))));
  top_.store(0, std::memory_order_relaxed);
  bottom_.store(0, std::memory_order_relaxed);
  array_.store(new Array{c}, std::memory_order_relaxed);
}

LocalQueue::~LocalQueue() {
  delete array_.load();
}

bool LocalQueue::Push(RunnableEntity* entity) {
  int64_t b = bottom_.load(std::memory_order_relaxed);
  int64_t t = top_.load(std::memory_order_acquire);
  Array* a = array_.load(std::memory_order_relaxed);

  if (a->Capacity() - 1 < (b - t)) {
    return false;
  }

  a->Push(b, entity);
  std::atomic_thread_fence(std::memory_order_release);
  bottom_.store(b + 1, std::memory_order_relaxed);

  return true;
}

RunnableEntity* LocalQueue::Pop() {
  int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
  Array* a = array_.load(std::memory_order_relaxed);
  bottom_.store(b, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  int64_t t = top_.load(std::memory_order_relaxed);

  RunnableEntity* entity{nullptr};

  if (t <= b) {
    entity = a->Pop(b);
    if (t == b) {
      if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        entity = nullptr;
      }
      bottom_.store(b + 1, std::memory_order_relaxed);
    }
  } else {
    bottom_.store(b + 1, std::memory_order_relaxed);
  }

  return entity;
}

RunnableEntity* LocalQueue::Steal() {
  int64_t t = top_.load(std::memory_order_acquire);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  int64_t b = bottom_.load(std::memory_order_acquire);

  RunnableEntity* entity{nullptr};

  if (t < b) {
    Array* a = array_.load(std::memory_order_consume);
    entity = a->Pop(t);
    if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
      return nullptr;
    }
  }

  return entity;
}

}  // namespace trpc::fiber::detail::v2
