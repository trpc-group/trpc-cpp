//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/descriptor.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <chrono>
#include <memory>

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/util/align.h"

namespace trpc {

/// @brief Base class for fiber connection
class alignas(hardware_destructive_interference_size) FiberConnection : public Connection {
 public:
  explicit FiberConnection(Reactor* reactor);

  ~FiberConnection() override;

  /// @brief Enable the event
  void EnableEvent(int event_type);

  /// @brief Register this connection to the reactor
  void AttachReactor();

  /// @brief Get reactor
  Reactor* GetReactor() const { return read_mostly_.reactor; }

  enum class EventAction {
    // No special action will be taken.
    //
    // The implemented MUST saturate system's buffer before return.
    kReady,

    // The descriptor `Kill()`-ed itself in the callback.
    kLeaving,

    // kSuppress the event from happening in the future. It's the `Descriptor`'s
    // responsibility to re-enable the event via `RestartReadIn()` / `RestartWriteIn()`
    kSuppress
  };

  enum class CleanupReason {
    kNone,  // Placeholder, not actually used
    kHandshakeFailed,
    kDisconnect,
    UserInitiated,
    kClosing,
    kError
  };

  /// @brief The execution function for readable event
  virtual EventAction OnReadable() = 0;

  /// @brief The execution function for writable event
  virtual EventAction OnWritable() = 0;

  /// @brief The execution function for error event,
  ///        you should call `Kill()` in this method
  virtual void OnError(int err) = 0;

  /// @brief Stop the connection
  virtual void Stop() {}

  /// @brief wait for the operations of the connection to complete
  virtual void Join() {}

  /// @brief Prevent events from happening, `OnCleanup()` will be called on completion.
  ///        if the descriptor is killed for multiple times, only the first one take
  ///        effect.
  void Kill(CleanupReason reason);

  /// @brief This connection is in a quiescent state now,
  ///        it has been removed from the reactor,
  ///        it can be destroyed immediately upon returning from this method
  virtual void OnCleanup(CleanupReason reason) = 0;

  /// @brief Wait until `OnCleanup()` returns.
  ///        `Kill()` must be called prior to calling this method
  void WaitForCleanup();

  /// @brief These methods re-enable read / write events that was / will be disabled by
  ///        returning `Suppress` from `OnReadable` / `OnWritable`.
  ///        it's safe to call these methods even before `OnReadable` / `OnWritable`
  ///        returns, in this case, returning `Suppress` has no effect.
  /// @param after allows you to specify a delay after how long will read / write be re-enabled.
  void RestartReadIn(std::chrono::nanoseconds after);
  void RestartWriteIn(std::chrono::nanoseconds after);

  /// @brief Get/Set the state whether to be attached to reactor
  bool Enabled() const { return read_mostly_.enabled.load(std::memory_order_relaxed); }
  void SetEnabled(bool flag) { read_mostly_.enabled.store(flag, std::memory_order_relaxed); }

  /// @brief Get the reason why the connection was closed,
  ///        used for streaming to handle normal shutdown and abnormal shutdown
  ///        when closing the connection
  int GetCleanupReason() const { return static_cast<int>(cleanup_reason_); }

  /// @brief Disable read event
  void DisableRead();

 protected:
  int HandleReadEvent() override;
  int HandleWriteEvent() override;
  void HandleCloseEvent() override;

 protected:
  CleanupReason cleanup_reason_{CleanupReason::kNone};

  std::atomic<std::size_t> restart_read_count_{0}, restart_write_count_{0};

  // Connection closing cleanup and connection sending/receiving mutex lock, to protect connection cleanup and network
  // data transmission/reception thread safety.
  std::mutex mutex_;
  // Connection unavailability status flag.
  bool conn_unavailable_{false};

 private:
  void SuppressReadAndClearReadEvent();
  void SuppressAndClearWriteEvent();
  void RestartReadNow();
  void RestartWriteNow();
  void QueueCleanupCallbackCheck();

 private:
  struct SeldomlyUsed;
  struct alignas(hardware_destructive_interference_size) {
    Reactor* reactor{nullptr};
    std::atomic<bool> enabled{false};

    // These fields are seldomly used, so put them separately so as to save
    // precious cache space.
    std::unique_ptr<SeldomlyUsed> seldomly_used;
  } read_mostly_;

  std::atomic<std::size_t> read_events_{0}, write_events_{0};

  std::atomic<bool> cleanup_pending_{false};
};

using FiberConnectionPtr = RefPtr<FiberConnection>;

}  // namespace trpc
