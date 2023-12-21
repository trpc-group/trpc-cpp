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

LocalQueue::LocalQueue()
    : bottom_(1),
      capacity_(0),
      runnables_(nullptr),
      top_(1) {
}

bool LocalQueue::Init(size_t capacity) {
  if (capacity_ != 0) {
    return false;
  }

  if (capacity == 0) {
    return false;
  }

  if (capacity & (capacity - 1)) {
    return false;
  }

  runnables_ = new(std::nothrow) RunnableEntity*[capacity];
  if (!runnables_) {
    return false;
  }

  capacity_ = capacity;

  return true;
}

LocalQueue::~LocalQueue() {
  delete [] runnables_;
  runnables_ = nullptr;
}

bool LocalQueue::Push(RunnableEntity* entity) {
  assert(capacity_ > 0);
  size_t b = bottom_.load(std::memory_order_relaxed);
  size_t t = top_.load(std::memory_order_acquire);
  if (b >= t + capacity_) {
    return false;
  }

  runnables_[b & (capacity_ - 1)] = entity;
  bottom_.store(b + 1, std::memory_order_release);

  return true;
}

RunnableEntity* LocalQueue::Pop() {
  assert(capacity_ > 0);
  size_t b = bottom_.load(std::memory_order_relaxed);
  size_t t = top_.load(std::memory_order_relaxed);

  if (t >= b) {
    return nullptr;
  }

  size_t newb = b - 1;
  bottom_.store(newb, std::memory_order_relaxed);

  std::atomic_thread_fence(std::memory_order_seq_cst);

  t = top_.load(std::memory_order_relaxed);
  if (t > newb) {
    bottom_.store(b, std::memory_order_relaxed);
    return nullptr;
  }

  RunnableEntity* entity = runnables_[newb & (capacity_ - 1)];
  if (t != newb) {
    return entity;
  }

  bool popped = top_.compare_exchange_strong(
      t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed);
  bottom_.store(b, std::memory_order_relaxed);
  if (popped) {
    return entity;
  }

  return nullptr;
}

RunnableEntity* LocalQueue::Steal() {
  assert(capacity_ > 0);
  size_t t = top_.load(std::memory_order_acquire);
  size_t b = bottom_.load(std::memory_order_acquire);
  if (t >= b) {
    return nullptr;
  }

  RunnableEntity* entity{nullptr};

  do {
    std::atomic_thread_fence(std::memory_order_seq_cst);

    b = bottom_.load(std::memory_order_acquire);
    if (t >= b) {
      return nullptr;
    }

    entity = runnables_[t & (capacity_ - 1)];
  } while (!top_.compare_exchange_strong(t, t + 1,
      std::memory_order_seq_cst, std::memory_order_relaxed));

  return entity;
}

}  // namespace trpc::fiber::detail::v2
 