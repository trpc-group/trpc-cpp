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

#include <memory>
#include <string>
#include <limits>

#include "trpc/future/exception.h"
#include "trpc/future/future.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"
#include "trpc/util/function.h"

namespace trpc {

namespace iotimer {

// for compatible
constexpr uint64_t InvalidID = kInvalidTimerId;

/// @brief The following timer interfaces use in io thread in merge threadmodel, for compatible

/// @brief Create timer on the io-thread(merge threadmodel)
/// @param delay The timeout of timer
/// @param interval Time interval for timer execution, if set `0`, run only once
/// @param task Timer execution function
/// @return uint64_t Timer id, it must be cancel by `Cancel` or detach by `Detach`.
///         Otherwise, it will cause resource leak of timer.
uint64_t Create(uint64_t delay, uint64_t interval, Function<void()>&& task);

// @brief Detach a timer by timer id
// @param timer_id It create by `Create`
// @return bool true: success, other: failed
bool Detach(uint64_t timer_id);

// @brief Cancel a timer by timer id
// @param timer_id It create by `Create`
// @return bool true: success, other: failed
bool Cancel(uint64_t timer_id);

/// @brief Pause a timer by timer id
/// @param timer_id It create by `Create`
/// @return bool true: success, other: failed
bool Pause(uint64_t timer_id);

// @brief Resume a timer by timer id
// @param timer_id It create by `Create`
// @return bool true: success, other: failed
bool Resume(uint64_t timer_id);

}  // namespace iotimer

class ThreadModel;

/// @brief Create Async timer, use in merge/separate threadmodel
/// @note not thread-safe, only use in framework thread
class AsyncTimer {
 public:
  class Unavailable : public ExceptionBase {
   public:
    const char* what() const noexcept override { return "timer unavailable"; }
  };

  class Cancelled : public ExceptionBase {
   public:
    const char* what() const noexcept override { return "timer cancelled"; }
  };

  class CreateFailed : public ExceptionBase {
   public:
    const char* what() const noexcept override { return "timer create failed"; }
  };

 public:
  explicit AsyncTimer(bool auto_cancel = true)
      : auto_cancel_(auto_cancel) {
  }

  ~AsyncTimer();

  /// @brief Create timer use in future
  /// @param delay How long after the timer starts executing
  /// @return Future<>
  /// @note Only use once
  Future<> After(uint64_t delay);

  /// @brief Create timer
  /// @param delay How long after the timer starts executing
  /// @param executor The execute function
  /// @param interval Time interval for timer execution, if set `0`, run only once
  /// @return bool true: create success, false: failed
  /// @note Only use once
  bool After(uint64_t delay, Function<void(void)>&& executor, int interval = 0);

  /// @brief Cancel timer
  /// @note Only use once
  void Cancel();

  /// @brief Detach timer, when this interface is called,
  /// the timer task registered by AsyncTimer using `After` will not be canceled
  /// even if the AsyncTimer object is destructed.
  /// @note Only use once
  void Detach();

  AsyncTimer(AsyncTimer&&) = default;
  AsyncTimer& operator=(AsyncTimer&&) = default;
  AsyncTimer(const AsyncTimer&) = delete;
  AsyncTimer& operator=(const AsyncTimer&) = delete;

 private:
  void AutoCancel();

 private:
  // timer id
  uint64_t timer_id_{kInvalidTimerId};
  // whether auto cancel when `AsyncTimer` instance destroy
  bool auto_cancel_ = true;
  // promise
  std::shared_ptr<Promise<>> promise_{nullptr};
};

}  // namespace trpc
