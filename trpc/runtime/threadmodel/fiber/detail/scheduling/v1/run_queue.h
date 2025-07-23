//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/run_queue.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cstddef>
#include <cstdint>

#include <atomic>
#include <memory>

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/scheduling.h"
#include "trpc/util/align.h"
#include "trpc/util/likely.h"

namespace trpc::fiber::detail::v1 {

// Thread-safe queue for storing runnable fibers.
class alignas(hardware_destructive_interference_size) RunQueue {
 public:
  RunQueue() = default;
  ~RunQueue() = default;

  // Initialize a queue whose capacity is `capacity`.
  bool Init(size_t capacity);

  // Push a fiber into the run queue.
  //
  // `instealable` should be `e.scheduling_group_local`. Internally we store
  // this value separately for `Steal` to use. This is required since `Steal`
  // cannot access `RunnableEntity` without claim ownership of it. In the meantime,
  // once the ownership is claimed (and subsequently to find the `RunnableEntity`
  // cannot be stolen), it can't be revoked easily. So we treat `RunnableEntity` as
  // opaque, and avoid access `RunnableEntity` it at all.
  //
  // Returns `false` on overrun.
  bool Push(RunnableEntity* e, bool instealable = false) {
    auto head = head_seq_.load(std::memory_order_relaxed);
    auto&& n = nodes_[head & mask_];
    auto nseq = n.seq.load(std::memory_order_acquire);
    if (TRPC_LIKELY(nseq == head &&
                     head_seq_.compare_exchange_weak(
                         head, head + 1, std::memory_order_relaxed))) {
      n.fiber = e;
      n.instealable.store(instealable, std::memory_order_relaxed);
      n.seq.store(head + 1, std::memory_order_release);
      return true;
    }
    return PushSlow(e, instealable);
  }

  // Push fibers in batch into the run queue.
  //
  // Returns `false` on overrun.
  bool BatchPush(RunnableEntity** start, RunnableEntity** end, bool instealable);

  // Pop a fiber from the run queue.
  //
  // Returns `nullptr` if the queue is empty.
  RunnableEntity* Pop() {
    auto tail = tail_seq_.load(std::memory_order_relaxed);
    auto&& n = nodes_[tail & mask_];
    auto nseq = n.seq.load(std::memory_order_acquire);
    if (TRPC_LIKELY(nseq == tail + 1 &&
                     tail_seq_.compare_exchange_weak(
                         tail, tail + 1, std::memory_order_relaxed))) {
      (void)n.seq.load(std::memory_order_acquire);  // We need this fence.
      auto rc = n.fiber;
      n.seq.store(tail + capacity_, std::memory_order_release);
      return rc;
    }
    return PopSlow();
  }

  // Steal a fiber from this run queue.
  //
  // If the first fibers in the queue was pushed with `instealable` set,
  // `nullptr` will be returned.
  //
  // Returns `nullptr` if the queue is empty.
  RunnableEntity* Steal();

  // Test if the queue is empty. The result might be inaccurate.
  bool UnsafeEmpty() const;

  // The size of queue
  std::size_t UnsafeSize() const;

 private:
  struct alignas(hardware_destructive_interference_size) Node {
    RunnableEntity* fiber;
    std::atomic<bool> instealable;
    std::atomic<std::uint64_t> seq;
  };

  bool PushSlow(RunnableEntity* e, bool instealable);
  RunnableEntity* PopSlow();

  template <class F>
  RunnableEntity* PopIf(F&& f);

  std::size_t capacity_;
  std::size_t mask_;
  std::unique_ptr<Node[]> nodes_;
  alignas(hardware_destructive_interference_size)
      std::atomic<std::size_t> head_seq_;
  alignas(hardware_destructive_interference_size)
      std::atomic<std::size_t> tail_seq_;
};

}  // namespace trpc::fiber::detail::v1
