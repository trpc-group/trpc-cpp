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

#include <limits>
#include <utility>

#include "trpc/future/exception.h"
#include "trpc/future/future.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/util/internal/singly_linked_list.h"

namespace trpc {

class SemaphoreException : public ExceptionBase {
 public:
  explicit SemaphoreException(const char* msg, int ret = ExceptionCode::SEMA_EXCEPT) {
    ret_code_ = ret;
    msg_ = msg;
  }

  const char* what() const noexcept override { return msg_.c_str(); }

  enum ExceptionCode {
    SEMA_EXCEPT = 1,
    LIST_ADD = 2,
    BROKEN = 3,
    NOT_THREAD_SAFE = 4,
  };
};

/// @brief Async semaphore based on future, you can deposit or withdraw units,
///        and withdrawl may wait if there is not enough units.
/// @note semaphore is not thread safe, but it checks the WorkeThreadId
///       to make sure object only can be operated on the same thread it's created on.
class Semaphore {
 private:
  struct entry {
    Promise<> pr;
    size_t num;
    trpc::internal::SinglyLinkedListEntry chain;
    entry(Promise<>&& p, size_t n) noexcept : pr(std::move(p)), num(n) {}
  };

  trpc::internal::SinglyLinkedList<entry, &entry::chain> wait_list_;

  size_t count_;

  Exception* ex_ = nullptr;

  int32_t worker_id_;

  int32_t doing_;

  bool HasAvailableUnits(size_t num) const noexcept {
    return count_ >= 0 && count_ >= num;
  }

  bool MayProceed(size_t num) const noexcept {
    return HasAvailableUnits(num) && wait_list_.empty();
  }

  int32_t GetThreadId() const noexcept {
    if (TRPC_UNLIKELY(nullptr == WorkerThread::GetCurrentWorkerThread())) {
      // If not in trpc thread model, does not check for thread safety, not recommended.
      return -1;
    }
    return static_cast<int32_t>(WorkerThread::GetCurrentWorkerThread()->Id());
  }

 public:
  static constexpr size_t MaxCounter() noexcept {
    return std::numeric_limits<decltype(count_)>::max();
  }

  explicit Semaphore(size_t count) noexcept : count_(count), doing_(0) {
    worker_id_ = GetThreadId();
  }

  ~Semaphore() noexcept {
    if (ex_) {
      delete ex_;
    }
  }

  /// @brief Wait until there is enough units, then make the returned Future ready
  ///        and reduce the counter. Waiters are stored in fifo wait list.
  /// @param num certain amount(num) of units.
  /// @return Future<> Ready:     There is enough units.
  ///                  Not-ready: No available units.
  ///                  Failed:    Semaphore is broken, or current worker thread is not
  ///                             the same as the one that lock object was created on.
  Future<> Wait(size_t num = 1) noexcept {
    if (TRPC_UNLIKELY(worker_id_ != GetThreadId())) {
      return MakeExceptionFuture<>(
              SemaphoreException("Not on the same worker thread",
                                 SemaphoreException::NOT_THREAD_SAFE));
    }

    // No need to wait
    if (MayProceed(num)) {
      count_ -= num;
      return MakeReadyFuture<>();
    }

    // It's broken
    if (ex_) {
      return MakeExceptionFuture<>(*ex_);
    }

    try {
      auto e = new entry(Promise<>(), num);
      wait_list_.push_back(e);
      return e->pr.GetFuture();
    } catch (...) {
      return MakeExceptionFuture<>(
              SemaphoreException("Failed to add promise to list",
                                 SemaphoreException::LIST_ADD));
    }
  }

  /// @brief Deposit units, increase count by num, if count can satisfy
  ///        waiters in wait list, then set value of the promises, and
  ///        count will be reduced according to the request(entry).
  /// @note If it does not pass thread check, this method will do nothing.
  /// @param num certain amount of units.
  /// @return Whether the deposit was successful.
  bool Signal(size_t num = 1) noexcept {
    if (TRPC_UNLIKELY(worker_id_ != GetThreadId())) {
      return false;
    }

    if (ex_) {
      return false;
    }

    count_ += num;

    if (doing_ > 0) {
      // some promises has not SetValue, DO NOT pop new waiter.
      return true;
    }

    decltype(wait_list_) ready_list;

    // FIFO, current count can meet the request, make them ready.
    while (!wait_list_.empty() && HasAvailableUnits(wait_list_.front().num)) {
      auto e = wait_list_.pop_front();
      count_ -= e->num;
      ready_list.push_back(e);
    }

    doing_ = ready_list.size();
    while (!ready_list.empty()) {
      auto e = ready_list.pop_front();
      doing_--;
      e->pr.SetValue();
      delete e;
    }

    return true;
  }

  /// @brief Synchronouse syntax, no waiters will be stored.
  /// @param num how many units it tries to withdraw.
  /// @return True:  when there is available units
  ///         False: no available units, and does not wait.
  bool TryWait(size_t num = 1) noexcept {
    if (TRPC_UNLIKELY(worker_id_ != GetThreadId())) {
      return false;
    }

    if (MayProceed(num)) {
      count_ -= num;
      return true;
    }
    return false;
  }

  /// @brief Notify all waiters that an error occured (set exception), and count
  ///        will be set to 0. This semaphore will not provide any service any more.
  //  @param msg will be used to construct exception, default to empty.
  void Broke(const char* msg = nullptr) noexcept {
    const char* m = "Broke the semaphore";
    if (msg) {
        m = msg;
    }
    ex_ = new Exception(SemaphoreException(m, SemaphoreException::BROKEN));
    count_ = 0;
    while (!wait_list_.empty()) {
      auto& e = wait_list_.front();
      e.pr.SetException(*ex_);
      delete wait_list_.pop_front();
    }
    return;
  }

  size_t AvailableUnits() const noexcept { return count_; }

  size_t Waiters() const noexcept { return wait_list_.size(); }

  uint32_t WorkerId() const noexcept { return worker_id_; }
};

}  // namespace trpc
