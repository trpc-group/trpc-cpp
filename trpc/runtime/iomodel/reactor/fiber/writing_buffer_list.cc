//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/detail/writing_buffer_list.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/writing_buffer_list.h"

#include <algorithm>
#include <climits>
#include <limits>
#include <utility>

#include "trpc/util/chrono/chrono.h"
#include "trpc/util/likely.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

using namespace std::literals;

namespace trpc {

namespace object_pool {

template <>
struct ObjectPoolTraits<WritingBufferList::Node> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

WritingBufferList::~WritingBufferList() {
  Stop();

  // Free the list.
  object_pool::LwUniquePtr<Node> ptr;
  auto current = head_.load(std::memory_order_acquire);
  Node* next;
  while (current) {
    ptr.Reset(current);  // To be freed.
    next = current->next.load(std::memory_order_acquire);
    if (!next) {
      if (auto expected_tail = current;
          !tail_.compare_exchange_strong(expected_tail, nullptr, std::memory_order_relaxed)) {
        // Someone else is appending new node and has changed `tail_` to its
        // node. Here we wait for him until he finishes in updating `current`
        // (i.e., his `prev`)'s `next`. (@sa: `Append`.)
        do {
          ptr.Reset(current->next.load(std::memory_order_acquire));
        } while (!ptr);
      }
      next = ptr->next.load(std::memory_order_acquire);
    }
    current = next;
  }
}

void WritingBufferList::Stop() {
  stop_token_.store(true, std::memory_order_release);

  if (TRPC_LIKELY(fiber::detail::IsFiberContextPresent())) {
    writable_cv_.notify_all();
    Append({}, {}, std::numeric_limits<size_t>::max(), 0);
  }
}

ssize_t WritingBufferList::FlushTo(IoHandler* io, ConnectionHandler* conn_handler,
                                   size_t max_bytes, size_t max_capacity,
                                   bool support_pipeline, bool* emptied, bool* short_write) {
  thread_local iovec iov[IOV_MAX];
  std::size_t nv = 0;
  std::size_t flushing = 0;

  // Yes since we're running concurrently with `Append`, we could miss some
  // newly-added buffers, and it does cause some (small) performance
  // degradation. However, it won't affect the whole algorithm's correctness.
  auto head = head_.load(std::memory_order_acquire);
  auto current = head;
  TRPC_ASSERT(current);  // It can't be. `Append` should have already updated it.

  TRPC_CHECK(tail_.load(std::memory_order_relaxed), "The buffer is empty.");
  while (current && nv != IOV_MAX && flushing < max_bytes) {
    for (auto iter = current->buffer.begin(); iter != current->buffer.end() && nv != IOV_MAX && flushing < max_bytes;
         ++iter) {
      auto& e = iov[nv++];
      e.iov_base = iter->data();
      e.iov_len = iter->size();  // For the last iov, we revise its size later.
      flushing += e.iov_len;
    }
    PreSendMessage(support_pipeline, conn_handler, current);
    current = current->next.load(std::memory_order_acquire);
  }

  // See above. We might fill more bytes than allowed into `iov`, so we revise
  // the possible error here.
  if (TRPC_LIKELY(flushing > max_bytes)) {
    auto diff = flushing - max_bytes;
    iov[nv - 1].iov_len -= diff;
    flushing -= diff;
  }

  ssize_t rc = io->Writev(iov, nv);
  if (rc < 0 || (rc == 0 && flushing > 0)) {
    return rc;  // Nothing is really flushed then.
  }
  TRPC_ASSERT(static_cast<std::size_t>(rc) <= flushing);

  // We did write something out. Remove those buffers and update the result accordingly.
  auto flushed = static_cast<std::size_t>(rc);
  bool drained = false;
  if (size_.fetch_sub(flushed, std::memory_order_acq_rel) - flushed < max_capacity) {
    writable_cv_.notify_one();
  }

  // Rewind.
  //
  // We do not have to reload `head_`, it shouldn't have changed.
  current = head;
  while (current) {
    if (auto b = current->buffer.ByteSize(); b <= flushed) {
      // The entire buffer was written then.
      object_pool::LwUniquePtr<Node> destroying;
      destroying.Reset(current);  // To be freed.
      flushed -= b;

      conn_handler->MessageWriteDone(current->io_msg);
      conn_handler->SetCurrentContextExt(current->io_msg.context_ext);

      if (auto next = current->next.load(std::memory_order_acquire); !next) {
        // We've likely drained the list.
        TRPC_ASSERT(flushed == 0);  // Or we have written out more than what we
                                    // have?
        // If nothing has changed, `tail_` still points to was-the-last node,
        // i.e., `current`.
        auto expected_tail = current;
        if (tail_.compare_exchange_strong(expected_tail, nullptr, std::memory_order_relaxed)) {
          // We successfully marked the list as empty.
          drained = true;
          // `head_` will be reset by next `Append`.
        } else {
          // Someone else is appending new node and has changed `tail_` to its
          // node. Here we wait for him until he finishes in updating `current`
          // (i.e., his `prev`)'s `next`. (@sa: `Append`.)
          Node* ptr;
          do {
            ptr = current->next.load(std::memory_order_acquire);
          } while (!ptr);
          // As `tail_` was never `nullptr` (we failed in setting it to
          // `nullptr` above), `Append` won't update `head_`. However, the newly
          // appended one is really the new head, so we update it here.
          head_.store(ptr, std::memory_order_release);
        }
        // In either case, we've finished rewinding, up to where we've written out.
        break;
      } else {
        // Move to the next one.
        current = next;
      }
    } else {
      current->buffer.Skip(flushed);
      // We didn't drain the list, set `head_` to where we left off.
      head_.store(current, std::memory_order_release);
      break;
    }
  }

  *emptied = drained;
  *short_write = (static_cast<std::size_t>(rc) != flushing);
  return rc;
}

WritingBufferList::BufferAppendStatus WritingBufferList::Append(NoncontiguousBuffer buffer, IoMessage&& io_msg,
                                                                size_t max_capacity, int64_t timeout) {
  if (max_capacity != 0) {
    std::unique_lock lock{mutex_};
    if (!writable_cv_.wait_for(lock, std::chrono::milliseconds(timeout), [&] {
          return size_.load(std::memory_order_acquire) < max_capacity || stop_token_.load(std::memory_order_acquire);
        })) {
      return kTimeout;
    }
  }

  // `size_` must add before appending node, otherwise `FlushTo` might underflow on sub.
  size_.fetch_add(buffer.ByteSize(), std::memory_order_release);

  auto node = object_pool::MakeLwUnique<Node>();
  node->next.store(nullptr, std::memory_order_relaxed);
  node->buffer = std::move(buffer);
  node->io_msg = std::move(io_msg);

  // By an atomic exchange between `tail_` and `node`, we atomically set `node`
  // as the new tail.
  auto prev = tail_.exchange(node.Get(), std::memory_order_acq_rel);
  if (!prev) {
    // If `tail_` was (before `exchange`) `nullptr`, the list was empty. In this
    // case we're the head of the list, so we'll update `head_` to reflect this.
    //
    // We'll also return `true` in this case to tell the caller about this fact.
    // TRPC_FMT_INFO("tail_ is nullptr");
    head_.store(node.Get(), std::memory_order_release);
  } else {
    // Otherwise there was a node (the old tail), so set us as its successor.
    // TRPC_FMT_INFO("tail_ is not nullptr");
    TRPC_ASSERT(!prev->next.load(std::memory_order_acquire));
    // Here is a time window between *`tail_` is set to `node`* and *`node` is
    // chained as the successor of the old `tail_`*.
    //
    // This inconsistency is mitigated by spinning in consumer (`FlushTo`) side
    // when it fails to `compare_exchange` `tail_` to `nullptr`. In this case,
    // the consumer side will spinning until it sees a non-`nullptr` `next` of
    // the old `tail_`.
    prev->next.store(node.Get(), std::memory_order_release);
    // TRPC_FMT_INFO("prev->next.store");
  }
  (void)node.Leak();                        // It will be freed on dequeue.
  return prev ? kAppendTail : kAppendHead;  // We changed `head_`?
}

void WritingBufferList::PreSendMessage(bool support_pipeline, ConnectionHandler* conn_handler, Node* current) {
  if (TRPC_LIKELY(!support_pipeline) || current->pre_send_flag.load(std::memory_order_acquire)) {
    return;
  }

  if (!current->io_msg.is_oneway) {
    conn_handler->PreSendMessage(current->io_msg);
  }
  current->pre_send_flag.store(true, std::memory_order_release);
}

}  // namespace trpc
