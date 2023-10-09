//
//
// Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
// TarsCpp is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/TarsCloud/TarsCpp/blob/v1.0.0/util/include/util/tc_thread_cond.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cerrno>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace trpc {

/// @brief Thread condition variable class, where all locks can wait for a signal to occur on it.
/// @note It needs to be used in conjunction with ThreadMutex, and ThreadMonitor is usually used
///       instead of using ThreadCond directly.
class ThreadCond {
 public:
  ThreadCond() = default;
  ~ThreadCond() {}

  // Not implemented; prevents accidental use.
  ThreadCond(const ThreadCond&) = delete;
  ThreadCond& operator=(const ThreadCond&) = delete;

  /// @brief Sending a signal will wake up one thread waiting on this condition.
  void Signal() { cond_.notify_one(); }

  /// @brief All threads waiting on this condition will be awakened.
  void Broadcast() { cond_.notify_all(); }

  /// @brief Waiting without any time limit.
  /// @tparam Mutex: a type that has the characteristics of a mutex lock.
  /// @param mutex Mutex lock.
  template <typename Mutex>
  void Wait(const Mutex& mutex) const {
    int c = mutex.Count();
    std::unique_lock<std::mutex> lck(mutex.mutex_, std::adopt_lock);
    cond_.wait(lck);
    mutex.Count(c);
  }

  /// @brief Wait for the specified time, and if it times out, stop waiting.
  /// @tparam Mutex: a type that has the characteristics of a mutex lock.
  /// @param mutex Mutex lock.
  /// @param millsecond Waiting time in milliseconds.
  /// @return false: indicates a timeout; true: indicates being awakened.
  template <typename Mutex>
  bool TimedWait(const Mutex& mutex, int millsecond) const {
    int c = mutex.Count();
    std::unique_lock<std::mutex> lck(mutex.mutex_, std::adopt_lock);
    std::cv_status ret = cond_.wait_for(lck, std::chrono::milliseconds(millsecond));
    mutex.Count(c);
    return ret != std::cv_status::timeout;
  }

 private:
  mutable std::condition_variable cond_;
};

}  // namespace trpc
