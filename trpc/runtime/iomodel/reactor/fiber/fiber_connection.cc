//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/descriptor.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/fiber_connection.h"

#include <iostream>
#include <utility>

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_timer.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/likely.h"
#include "trpc/util/thread/internal/memory_barrier.h"
#include "trpc/util/time.h"

using namespace std::literals;

namespace trpc {

struct FiberConnection::SeldomlyUsed {
  std::string name;

  std::atomic<bool> cleanup_queued{false};

  // Incremented whenever `EPOLLERR` is seen.
  //
  // FIXME: Can we really see more than one `EPOLLERR` in practice?
  std::atomic<std::size_t> error_events{};
  std::atomic<bool> error_seen{false};  // Prevent multiple `EPOLLERR`s.

  // Set to non-`kNone` once a cleanup event is pending. If multiple events
  // triggered cleanup (e.g., an error occurred and the descriptor is
  // concurrently being removed from the `EventLoop`), the first one wins.
  std::atomic<CleanupReason> cleanup_reason{CleanupReason::kNone};

  // For implementing `WaitForCleanup()`.
  FiberMutex cleanup_lk;
  FiberConditionVariable cleanup_cv;
  bool cleanup_completed{false};
};

FiberConnection::FiberConnection(Reactor* reactor)
    : read_mostly_{.reactor = reactor, .enabled = false, .seldomly_used = std::make_unique<SeldomlyUsed>()} {}

FiberConnection::~FiberConnection() {}

void FiberConnection::EnableEvent(int event_type) {
  SetSetEvents(event_type);

  restart_read_count_ = (event_type & EventType::kReadEvent) ? 1 : 0;
  restart_write_count_ = (event_type & EventType::kWriteEvent) ? 1 : 0;
}

void FiberConnection::AttachReactor() {
  Ref();

  SetEnabled(true);

  GetReactor()->Update(this);
}

void FiberConnection::DisableRead() {
  GetReactor()->SubmitTask([this, ref = RefPtr(ref_ptr, this)] {
    if (Enabled()) {
      DisableEvent(EventHandler::EventType::kReadEvent);
      GetReactor()->Update(this);
      TRPC_FMT_DEBUG("FiberConnection::DisableRead ip {}, port: {}, is_client {}, Disable Read.",
        GetPeerIp(), GetPeerPort(), IsClient());
    }
  });
}

int FiberConnection::HandleReadEvent() {
  if (read_events_.fetch_add(1, std::memory_order_acquire) == 0) {
    bool start_fiber = StartFiberDetached([this] {
      RefPtr self_ref(ref_ptr, this);

      do {
        auto rc = OnReadable();
        if (TRPC_LIKELY(rc == EventAction::kReady)) {
          continue;
        } else if (TRPC_UNLIKELY(rc == EventAction::kLeaving)) {
          TRPC_ASSERT(read_mostly_.seldomly_used->cleanup_reason.load(std::memory_order_relaxed) !=
                      CleanupReason::kNone);
          // We can only reset the counter in reactor's context.
          //
          // As `Kill()` as been called, by the time our task is run by the
          // reactor, this descriptor has been disabled, and no more call to
          // `FireReadEvent()` (the only one who increments `read_events_`), so
          // it's safe to reset the counter.
          //
          // Meanwhile, `QueueCleanupCallbackCheck()` is called after our task,
          // by the time it checked the counter, it's zero as expected.
          GetReactor()->SubmitTask([this] {
            read_events_.store(0, std::memory_order_relaxed);
            QueueCleanupCallbackCheck();
          });
          break;
        } else if (rc == EventAction::kSuppress) {
          SuppressReadAndClearReadEvent();

          // CAUTION: Here we break out before `read_events_` is drained. This
          // is safe though, as `SuppressReadAndClearReadEvent()` will
          // reset `read_events_` to zero after is has disabled the event.
          break;
        }
      } while (read_events_.fetch_sub(1, std::memory_order_release) !=
               1);  // Loop until we decremented `read_events_` to zero. If more
                    // data has come before `OnReadable()` returns, the loop
                    // condition will hold.
      QueueCleanupCallbackCheck();
    });

    if (TRPC_UNLIKELY(!start_fiber)) {
      TRPC_ASSERT(false && "StartFiberDetached to execute HandleReadEvent faild,abort!!!");
    }
  }

  return 0;
}

int FiberConnection::HandleWriteEvent() {
  if (write_events_.fetch_add(1, std::memory_order_acquire) == 0) {
    bool start_fiber = StartFiberDetached([this] {
      RefPtr self_ref(ref_ptr, this);
      do {
        auto rc = OnWritable();
        if (TRPC_LIKELY(rc == EventAction::kReady)) {
          continue;
        } else if (TRPC_UNLIKELY(rc == EventAction::kLeaving)) {
          TRPC_ASSERT(read_mostly_.seldomly_used->cleanup_reason.load(std::memory_order_relaxed) !=
                      CleanupReason::kNone);
          GetReactor()->SubmitTask([this] {
            write_events_.store(0, std::memory_order_relaxed);
            QueueCleanupCallbackCheck();
          });
          break;
        } else if (rc == EventAction::kSuppress) {
          SuppressAndClearWriteEvent();
          break;
        }
      } while (write_events_.fetch_sub(1, std::memory_order_release) != 1);
      QueueCleanupCallbackCheck();
    });

    if (TRPC_UNLIKELY(!start_fiber)) {
      TRPC_ASSERT(false && "StartFiberDetached to execute HandleWriteEvent faild,abort!!!");
    }
  }

  return 0;
}

void FiberConnection::HandleCloseEvent() {
  if (!Enabled()) {
    return;
  }

  if (read_mostly_.seldomly_used->error_seen.exchange(true, std::memory_order_relaxed)) {
    TRPC_FMT_ERROR_EVERY_SECOND(
        "FiberConnection::HandleCloseEvent ip {}, port: {}, is_client {}, Unexpected: Multiple `EPOLLERR` received.",
        GetPeerIp(), GetPeerPort(), IsClient());
    return;
  }

  if (read_mostly_.seldomly_used->error_events.fetch_add(1, std::memory_order_acquire) == 0) {
    bool start_fiber = StartFiberDetached([this] {
      RefPtr self_ref(ref_ptr, this);
      OnError(-1);
      TRPC_ASSERT(read_mostly_.seldomly_used->error_events.fetch_sub(1, std::memory_order_release) == 1);
      QueueCleanupCallbackCheck();
    });

    if (TRPC_UNLIKELY(!start_fiber)) {
      TRPC_ASSERT(false && "StartFiberDetached to execute HandleCloseEvent faild,abort!!!");
    }
  } else {
    TRPC_ASSERT(false);
  }
}

void FiberConnection::RestartReadIn(std::chrono::nanoseconds after) {
  if (after != 0ns) {
    auto timer = CreateFiberTimer(ReadSteadyClock() + after, [this, ref = RefPtr(ref_ptr, this)](auto timer_id) {
      KillFiberTimer(timer_id);
      RestartReadNow();
    });
    EnableFiberTimer(timer);
  } else {
    RestartReadNow();
  }
}

void FiberConnection::RestartWriteIn(std::chrono::nanoseconds after) {
  if (after != 0ns) {
    auto timer = CreateFiberTimer(ReadSteadyClock() + after, [this, ref = RefPtr(ref_ptr, this)](auto timer_id) {  //
      KillFiberTimer(timer_id);
      RestartWriteNow();
    });
    EnableFiberTimer(timer);
  } else {
    RestartWriteNow();
  }
}

void FiberConnection::SuppressReadAndClearReadEvent() {
  // This must be done in `EventLoop`. Otherwise order of calls to
  // `ModEventHandler` is nondeterministic.
  GetReactor()->SubmitTask([this, ref = RefPtr(ref_ptr, this)] {  //
    // We reset `read_events_` to zero first, as it's left non-zero when we
    // leave `HandleReadEvent()`.
    //
    // No race should occur. `HandleReadEvent()` itself is called in `EventLoop`
    // (where we're running), so it can't race with us. The only other one who
    // can change `read_events_` is the fiber who called us, and it should
    // have break out the `while` loop immediately after calling us without
    // touching `read_events_` any more.
    //
    // So we're safe here.
    read_events_.store(0, std::memory_order_release);

    // This is needed in case the descriptor is going to leave and its
    // `OnReadable()` returns `kSuppress`.
    QueueCleanupCallbackCheck();

    if (Enabled()) {
      int reached = restart_read_count_.fetch_sub(1, std::memory_order_relaxed) - 1;
      // If `reached` (i.e., `restart_read_count_` after decrement) reaches 0,
      // we're earlier than `RestartReadIn()`.
      TRPC_ASSERT(reached != -1);
      TRPC_ASSERT((GetSetEvents() & EventType::kReadEvent) != 0);  // Were `EventType::kReadEvent` to be
                                                                   // removed, it's us who remove it.
      if (reached == 0) {
        SetSetEvents(GetSetEvents() & (~EventType::kReadEvent));
        GetReactor()->Update(this);
      } else {
        // Otherwise things get tricky. In this case we left system's buffer
        // un-drained, and `RestartRead` happens before us. From system's
        // perspective, this scenario just looks like we haven't drained its
        // buffer yet, so it won't return a `EventType::kReadEvent` again.
        //
        // We have to either emulate one or remove and re-add the connection to
        // the reactor in this case.
        HandleReadEvent();
      }
    }
  });
}

void FiberConnection::SuppressAndClearWriteEvent() {
  GetReactor()->SubmitTask([this, ref = RefPtr(ref_ptr, this)] {
    write_events_.store(0, std::memory_order_relaxed);
    QueueCleanupCallbackCheck();

    if (Enabled()) {
      auto reached = restart_write_count_.fetch_sub(1, std::memory_order_relaxed) - 1;
      TRPC_ASSERT(reached == 0 || reached == 1);
      TRPC_ASSERT((GetSetEvents() & EventType::kWriteEvent) != 0);

      if (reached == 0) {
        SetSetEvents(GetSetEvents() & (~EventType::kWriteEvent));
        GetReactor()->Update(this);
      } else {
        HandleWriteEvent();
      }
    }
  });
}

void FiberConnection::RestartReadNow() {
  GetReactor()->SubmitTask([this, ref = RefPtr(ref_ptr, this)] {
    if (Enabled()) {
      auto count = restart_read_count_.fetch_add(1, std::memory_order_relaxed);
      if (count == 0) {  // We changed it from 0 to 1.
        TRPC_ASSERT((GetSetEvents() & EventType::kReadEvent) == 0);
        SetSetEvents(GetSetEvents() | EventType::kReadEvent);
        GetReactor()->Update(this);
      }
    }
  });
}

void FiberConnection::RestartWriteNow() {
  GetReactor()->SubmitTask([this, ref = RefPtr(ref_ptr, this)] {
    if (Enabled()) {
      auto count = restart_write_count_.fetch_add(1, std::memory_order_relaxed);
      TRPC_ASSERT(count == 0 || count == 1);

      if (count == 0) {
        TRPC_ASSERT((GetSetEvents() & EventType::kWriteEvent) == 0);
        SetSetEvents(GetSetEvents() | EventType::kWriteEvent);
        GetReactor()->Update(this);
      }
    }
  });
}

void FiberConnection::WaitForCleanup() {
  std::unique_lock lk(read_mostly_.seldomly_used->cleanup_lk);
  read_mostly_.seldomly_used->cleanup_cv.wait(lk, [this] { return read_mostly_.seldomly_used->cleanup_completed; });
}

void FiberConnection::Kill(CleanupReason reason) {
  TRPC_LOG_DEBUG("FiberConnection::Kill ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                                             << ", is_client:" << IsClient() << ", reason:" << (int)reason);
  TRPC_ASSERT(reason != CleanupReason::kNone);
  auto expected = CleanupReason::kNone;
  if (read_mostly_.seldomly_used->cleanup_reason.compare_exchange_strong(expected, reason, std::memory_order_relaxed)) {
    GetReactor()->SubmitTask([this, ref = RefPtr(ref_ptr, this)] {
      TRPC_ASSERT(Enabled());

      DisableAllEvent();

      GetReactor()->Update(this);

      SetEnabled(false);

      cleanup_pending_.store(true, std::memory_order_relaxed);

      QueueCleanupCallbackCheck();
    });
  }
}

void FiberConnection::QueueCleanupCallbackCheck() {
  // Full barrier, hurt performance.
  //
  // Here we need it to guarantee that:
  //
  // - For `Kill()`, it's preceding store to `cleanup_pending_` cannot be
  //   reordered after reading `xxx_events_`;
  //
  // - For `HandleXxxEvent()`, it's load of `cleanup_pending_` cannot be reordered
  //   before its store to `xxx_events_`.
  //
  // Either case, reordering leads to falsely treating the descriptor in use.
  internal::MemoryBarrier();

  if (TRPC_LIKELY(!cleanup_pending_.load(std::memory_order_relaxed))) {
    return;
  }

  if (read_events_.load(std::memory_order_relaxed) == 0 && write_events_.load(std::memory_order_relaxed) == 0 &&
      read_mostly_.seldomly_used->error_events.load(std::memory_order_relaxed) == 0) {
    // Consider queue a call to `OnCleanup()` then.
    if (!read_mostly_.seldomly_used->cleanup_queued.exchange(true, std::memory_order_release)) {
      // No need to take a reference to us, `OnCleanup()` has not been called.
      GetReactor()->SubmitTask([this] {
        // The load below acts as a fence (paired with `exchange` above). (But
        // does it make sense?)
        (void)read_mostly_.seldomly_used->cleanup_queued.load(std::memory_order_acquire);

        // They can't have changed.
        TRPC_ASSERT(read_events_.load(std::memory_order_relaxed) == 0);
        TRPC_ASSERT(write_events_.load(std::memory_order_relaxed) == 0);
        TRPC_ASSERT(read_mostly_.seldomly_used->error_events.load(std::memory_order_relaxed) == 0);

        // Hold a reference to us, might be the last one.
        RefPtr self_ref(ref_ptr, this);

        TRPC_ASSERT(!Enabled());

        Deref();

        TRPC_LOG_DEBUG("FiberConnection::QueueCleanupCallbackCheck ip:" << GetPeerIp() << ", port:" << GetPeerPort()
                                                                        << ", is_client:" << IsClient()
                                                                        << ", conn_id:" << GetConnId());

        OnCleanup(read_mostly_.seldomly_used->cleanup_reason.load(std::memory_order_relaxed));

        // Wake up any waiters on `OnCleanup()`.
        std::scoped_lock _(read_mostly_.seldomly_used->cleanup_lk);
        read_mostly_.seldomly_used->cleanup_completed = true;
        read_mostly_.seldomly_used->cleanup_cv.notify_one();
      });
    }
  }
}

}  // namespace trpc
