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

#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <utility>

#include "trpc//future/exception.h"
#include "trpc//future/executor.h"
#include "trpc//future/function_traits.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Declaration of Promise.
/// @private
template <typename... T>
class Promise;

/// @brief Declaration of Future.
/// @private
template <typename... T>
class Future;

/// @brief Helper to Future constructor.
struct MakeReadyFutureHelper {};

/// @brief Get a ready future with value set by parameter v.
template <typename... T>
Future<T...> MakeReadyFuture(T&&... v) {
  return Future<T...>(MakeReadyFutureHelper(), std::move(v)...);
}

/// @brief Get a exceptional future with exception set parameter e.
template <typename... T>
Future<T...> MakeExceptionFuture(const Exception& e) {
  return Future<T...>(e);
}

/// @brief Declaration of Continuation for variable parameters.
/// @private
template <typename... T>
class Continuation;

/// @brief Declaration of Continuation for single parameter.
/// @private
template <typename T>
class Continuation<T>;

/// @brief Executor for continuation theory,
/// @note Corresponding promise must be set inside thread having reactor.
/// @private
struct ContinuationScheduler : public Executor {
  bool SubmitTask(Task&& task) override {
    return Reactor::GetCurrentTlsReactor()->SubmitTask(std::move(task));
  }
};

/// @brief Stateless executor may splendid into many entities.
/// @note Users should not directly reference this executor.
static inline ContinuationScheduler kDefaultScheduler;

/// @brief Get a executor depending on whether current thread has reactor.
static inline Executor* GetExecutor() {
  return Reactor::GetCurrentTlsReactor() ? static_cast<Executor*>(&kDefaultScheduler)
                                         : static_cast<Executor*>(&kDefaultInlineExecutor);
}

/// @brief Inherited by FutureState to stand for state of FutureImpl.
/// @private
struct FutureStateBase {
  // Future is ready or not.
  bool ready = false;

  // Future is failed or not.
  bool failed = false;

  // Future is either ready or failed when true.
  bool has_result = false;

  // Is callback set through Then to this Future.
  bool has_callback = false;

  // Is callback already been scheduled, to control only once may callback be called.
  std::atomic_flag schedule_flag = ATOMIC_FLAG_INIT;

  // Since FutureImpl may hold by Future and Promise, use ref to determin when to free FutureImpl.
  std::atomic<int> ref_count = 1;

  // Synchronize ready/failed/has_result/has_callback between maybe two threads holding current state.
  std::mutex mtx;

  // Store exception when Futuren failed.
  Exception exception;
};

/// @brief Variable parameters version of FutureState.
/// @private
template <typename... T>
struct FutureState : public FutureStateBase {
  FutureState() = default;

  explicit FutureState(std::tuple<T...>&& val) : value(std::move(val)) {}

  std::tuple<T...> value;

  Continuation<T...>* callback = nullptr;
};

/// @brief Single parameter version of FutureState.
/// @private
template <typename T>
struct FutureState<T> : public FutureStateBase {
  static_assert(std::is_default_constructible_v<T>, "Type must be constructible");

  static_assert(std::is_move_constructible<T>::value, "Type must be move constructible");

  FutureState() = default;

  explicit FutureState(T&& val) : value(std::move(val)) {}

  T value;

  Continuation<T>* callback = nullptr;
};

/// @brief Derived by Continuation.
/// @private
class ContinuationBase {
 public:
  /// @brief Destructor.
  virtual ~ContinuationBase() = default;

  /// @brief Need to implemented by subclass to determined the way future result handled.
  virtual void Run() = 0;

  /// @brief Bind executor to current continuation.
  inline void SetExecutor(Executor* executor) { executor_ = executor; }

  /// @brief Bind exception to current continuation.
  inline void SetException(Exception&& e) {
    failed_ = true;
    e_ = std::move(e);
  }

  /// @brief Try to submit current continuation task to certain executor.
  ///        Fall back to current thread if submit failed.
  /// @note Task only exeuted once.
  inline void Schedule(ContinuationBase* task) {
    // Use default executor if no one set by uses.
    auto executor = executor_ ? executor_ : GetExecutor();
    bool ret = executor->SubmitTask([task]() mutable {
      task->Run();
      delete task;
    });

    // Fall back to current thread.
    if (!ret) {
      task->Run();
      delete task;
    }
  }

  /// @brief Chain current Future/Promise to next Future/Promise.
  template <typename PromiseType, typename FutureType>
  static inline void ChainFuture(FutureType&& fut, PromiseType&& promise);

 protected:
  // Store executor set bu users, default to null.
  Executor* executor_ = nullptr;

  // Corresponding future is ready or not.
  bool ready_ = false;

  // Corresponding future is failed or not.
  bool failed_ = false;

  // Store exception if future failed.
  Exception e_;
};

/// @brief Multiple parameters version of Continuation.
/// @private
template <typename... T>
class Continuation : public ContinuationBase {
 public:
  ~Continuation() override = default;

  // Set the value callback will reference to when current future is ready.
  void SetValue(std::tuple<T...>&& value) noexcept {
    ready_ = true;
    value_ = std::move(value);
  }

 protected:
  // Value type with tuple.
  std::tuple<T...> value_;
};

/// @brief Single parameter version of Continuation.
/// @private
template <typename T>
class Continuation<T> : public ContinuationBase {
 public:
  ~Continuation() override = default;

  // Set the value callback will reference to when current future is ready.
  void SetValue(T&& value) noexcept {
    ready_ = true;
    value_ = std::move(value);
  }

 protected:
  // Value type with Future template parameter, better performance.
  T value_;
};

/// @brief Firstly, current future is of multiple parameters.
///        Secondly, user call Then with callback parametered with value and return next Future.
/// @private
template <typename Func, typename PromiseType, typename... T>
class ContinuationWithValue : public Continuation<T...> {
 public:
  using BaseClass = Continuation<T...>;

  /// @brief Constructor.
  ContinuationWithValue(Func&& func, PromiseType&& promise)
      : func_(std::forward<Func>(func)), promise_(std::forward<PromiseType>(promise)) {}

  /// @brief Destructor.
  ~ContinuationWithValue() override = default;

  /// @brief Callback will not run if current future failed, only pass exception to next Future/Promise.
  void Run() override {
    if (BaseClass::failed_) {
      promise_.SetException(BaseClass::e_);
      return;
    }

    // Have to make tuple before pass value to callback.
    auto r = std::apply<Func, std::tuple<T...>>(std::move(func_), std::move(BaseClass::value_));
    ContinuationBase::ChainFuture(std::move(r), std::move(promise_));
  }

 private:
  // Store user register callback through Then.
  Func func_;

  // Pair Future is returned to user by Then.
  PromiseType promise_;
};

/// @brief Firstly, current future is of single parameter.
///        Secondly, user call Then with callback parametered with value and return next future.
/// @private
template <typename Func, typename PromiseType, typename T>
class ContinuationWithValue<Func, PromiseType, T> : public Continuation<T> {
 public:
  using BaseClass = Continuation<T>;

  /// @brief Constructor.
  ContinuationWithValue(Func&& func, PromiseType&& promise)
      : func_(std::forward<Func>(func)), promise_(std::forward<PromiseType>(promise)) {}

  /// @brief Destructor.
  ~ContinuationWithValue() override = default;

  /// @brief Callback will not run if current future failed, only pass exception to next Future/Promise.
  void Run() override {
    if (BaseClass::failed_) {
      promise_.SetException(BaseClass::e_);
      return;
    }

    // Just pass value to callback.
    auto r = std::invoke<Func, T>(std::move(func_), std::move(BaseClass::value_));
    ContinuationBase::ChainFuture(std::move(r), std::move(promise_));
  }

 private:
  // Store user register callback.
  Func func_;

  // Pair Future is returned to user by Then.
  PromiseType promise_;
};

/// @brief Firstly, current future is of multiple parameters.
///        Secondly, user call Then with callback parametered with value and return void.
/// @private
template <typename Func, typename... T>
class TerminalWithValue : public Continuation<T...> {
 public:
  using BaseClass = Continuation<T...>;

  /// @brief Constructor.
  explicit TerminalWithValue(Func&& func) : func_(std::forward<Func>(func)) {}

  /// @brief Destructor.
  ~TerminalWithValue() override = default;

  /// @brief Callback will not run if current future failed.
  void Run() override {
    if (BaseClass::failed_) {
      return;
    }

    // Have to make tuple before pass value to callback.
    std::apply<Func, std::tuple<T...>>(std::move(func_), std::move(BaseClass::value_));
  }

 private:
  // Store user register callback.
  Func func_;
};

/// @brief Firstly, current future is of single parameter.
///        Secondly, user call Then with callback parametered with value and return void.
/// @private
template <typename Func, typename T>
class TerminalWithValue<Func, T> : public Continuation<T> {
 public:
  using BaseClass = Continuation<T>;

  explicit TerminalWithValue(Func&& func) : func_(std::forward<Func>(func)) {}

  ~TerminalWithValue() override = default;

  /// @brief Callback will not run if current future failed.
  void Run() override {
    if (BaseClass::failed_) {
      return;
    }

    // Just pass value to callback.
    std::invoke<Func, T>(std::move(func_), std::move(BaseClass::value_));
  }

 private:
  // Store user register callback.
  Func func_;
};

/// @brief User call Then with callback parametered with future and return next Future.
///        Regardless of how many parameters current future have.
/// @private
template <typename Func, typename PromiseType, typename... T>
class ContinuationWithFuture : public Continuation<T...> {
 public:
  using BaseClass = Continuation<T...>;

  ContinuationWithFuture(Func&& func, PromiseType&& promise)
      : func_(std::forward<Func>(func)), promise_(std::forward<PromiseType>(promise)) {}

  ~ContinuationWithFuture() override = default;

  /// @brief Callback will be run no matter what state is current future.
  void Run() override {
    Promise<T...> promise;

    if (BaseClass::failed_) {
      promise.SetException(BaseClass::e_);
    } else {
      promise.SetValue(std::move(BaseClass::value_));
    }

    // Use intermediate Future/Promise to chain current future with next future.
    auto r = func_(std::move(promise.GetFuture()));
    ContinuationBase::ChainFuture(std::move(r), std::move(promise_));
  }

 private:
  // Store user register callback.
  Func func_;

  // Pair Future is returned to user by Then.
  PromiseType promise_;
};

/// @brief User call Then with callback parametered with future and return void.
///        Regardless of how many parameters current future have.
/// @private
template <typename Func, typename... T>
class TerminalWithFuture : public Continuation<T...> {
 public:
  using BaseClass = Continuation<T...>;

  explicit TerminalWithFuture(Func&& func) : func_(std::forward<Func>(func)) {}

  ~TerminalWithFuture() override = default;

  /// @brief Callback will be run no matter what state is current future.
  void Run() override {
    Promise<T...> promise;

    if (BaseClass::failed_) {
      promise.SetException(BaseClass::e_);
    } else {
      promise.SetValue(std::move(BaseClass::value_));
    }

    // Fill callback with intermediate future.
    func_(std::move(promise.GetFuture()));
  }

 private:
  // Store user register callback.
  Func func_;
};

/// @brief Derived by multiple parameters and single parameter version of FutureImpl.
/// @private
class FutureImplBase {
 public:
  FutureImplBase() noexcept = default;

  ~FutureImplBase() noexcept = default;

  /// @brief Set state for future.
  void Init(FutureStateBase* state_base) { state_base_ = state_base; }

  /// @brief Called before get value of future.
  inline bool IsReady() const noexcept { return state_base_->ready; }

  /// @brief Called before get exception of future.
  inline bool IsFailed() const noexcept { return state_base_->failed; }

  /// @brief Either future is ready or failed.
  inline bool HasResult() const noexcept { return state_base_->has_result; }

  /// @brief Wait for future ready or failed, return if timeout expired. It't deprecated.
  /// @note Do not call this inside framework threads or users threads, as it blocks everything.
  [[deprecated("Use Latch/BlockingGet/BlockingTryGet instead")]] bool Wait(const uint32_t timeout) noexcept {
    int64_t timeout_us = timeout * 1000;
    auto start = std::chrono::system_clock::now();

    while (true) {
      {
        std::lock_guard<std::mutex> lock(state_base_->mtx);
        if (state_base_->has_result) {
          break;
        }
      }
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      auto now = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - start);
      if (diff.count() >= timeout_us) {
        return false;
      }
    }

    return true;
  }

  /// @brief Should be called only once, as state changed after first call.
  Exception GetException() noexcept {
    state_base_->failed = false;
    return state_base_->exception;
  }

 protected:
  /// @brief Normally called by get pair future of promise, to avoid wild pointer.
  inline void Attach() noexcept { state_base_->ref_count.fetch_add(1, std::memory_order_relaxed); }

 private:
  FutureStateBase* state_base_ = nullptr;
};

/// @brief Multiple parameters version of FutureImpl.
/// @private
template <typename... T>
class FutureImpl : public FutureImplBase {
 public:
  friend class Promise<T...>;
  friend class Future<T...>;

  FutureImpl() noexcept { FutureImplBase::Init(&state_); }

  /// @brief Used by MakeReadyFuture to create ready future.
  explicit FutureImpl(std::tuple<T...>&& value) noexcept : state_(std::move(value)) {
    FutureImplBase::Init(&state_);
    state_.ready = true;
    state_.has_result = true;
  }

  ~FutureImpl() noexcept = default;

  /// @brief Make sure future is ready before calling.
  /// @note After calling, future is not ready any more.
  std::tuple<T...>&& GetValue() noexcept {
    assert(state_.ready);
    state_.ready = false;
    return std::move(state_.value);
  }

  /// @brief Make sure future is ready before calling.
  /// @note Future reserves ready after calling.
  const std::tuple<T...>& GetConstValue() const noexcept {
    assert(state_.ready);
    return state_.value;
  }

  /// @brief Callback registered through Then with parameters type by Value, and returned result typed by Future.
  /// @tparam FutureType Next Future type.
  /// @tparam PromiseType Pair with FutureType.
  /// @tparam Func Funtion type which receive Value type and return Future type inside.
  /// @param promise Intermediate promise created by framework, which's pair future is returned to user.
  /// @param func User specified callback func.
  /// @param executor User specified executor, default to null.
  template <typename FutureType, typename PromiseType = typename FutureType::PromiseType,
            typename Func = Function<FutureType(T&&...)>>
  void SetCallback(PromiseType&& promise, Func&& func, Executor* executor) {
    state_.callback = new ContinuationWithValue<Func, PromiseType, T...>(std::forward<Func>(func),
                                                                         std::forward<PromiseType>(promise));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    // Got immediately executed.
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Callback registered through Then with parameters type by Value, and returned result typed by void.
  /// @tparam NotFutureType Usually void.
  /// @tparam Func Funtion type which receive Value type and return void type inside.
  /// @param func User specified callback func.
  /// @param executor User specified executor, default to null.
  template <typename NotFutureType, typename Func = Function<NotFutureType(T&&...)>>
  void SetTerminalCallback(Func&& func, Executor* executor) {
    state_.callback = new TerminalWithValue<Func, T...>(std::forward<Func>(func));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    // Got immediately executed.
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Callback registered through Then with parameters type by Future, and returned result typed by Future.
  /// @tparam FutureType Next Future type.
  /// @tparam PromiseType Pair with FutureType.
  /// @tparam Func Funtion type which receive Future type and return Future type inside.
  /// @param promise Intermediate promise created by framework, which's pair future is returned to user.
  /// @param func User specified callback func.
  /// @param executor User specified executor, default to null.
  template <typename FutureType, typename PromiseType = typename FutureType::PromiseType,
            typename Func = Function<FutureType(Future<T...>&&)>>
  void SetCallbackWrapped(PromiseType&& promise, Func&& func, Executor* executor) {
    state_.callback = new ContinuationWithFuture<Func, PromiseType, T...>(std::forward<Func>(func),
                                                                          std::forward<PromiseType>(promise));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    // Got immediately executed.
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Callback registered through Then with parameters type by Future, and returned result typed by void.
  /// @tparam NotFutureType Usually void.
  /// @tparam Func Funtion type which receive Future type and return void type inside.
  /// @param func User specified callback func.
  /// @param executor User specified executor, default to null.
  template <typename NotFutureType, typename Func = Function<NotFutureType(Future<T...>&&)>>
  void SetTerminalCallbackWrapped(Func&& func, Executor* executor) {
    state_.callback = new TerminalWithFuture<Func, T...>(std::forward<Func>(func));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    // Got immediately executed.
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Ready value set through promise.
  void SetValue(std::tuple<T...>&& value) {
    state_.value = std::move(value);
    state_.ready = true;
    // Result state may be looped by another thread.
    {
      std::lock_guard<std::mutex> lock(state_.mtx);
      state_.has_result = true;
    }
    TrySchedule();
  }

  /// @brief Exceptional value set through promise.
  void SetException(const Exception& e) {
    state_.exception = e;
    state_.failed = true;
    // Result state may be looped by another thread.
    {
      std::lock_guard<std::mutex> lock(state_.mtx);
      state_.has_result = true;
    }
    TrySchedule();
  }

  /// @brief Support non const parameter.
  void SetException(Exception&& e) {
    state_.exception = std::move(e);
    state_.failed = true;
    {
      std::lock_guard<std::mutex> lock(state_.mtx);
      state_.has_result = true;
    }
    TrySchedule();
  }

 private:
  /// @brief Future may or may not registered callback yet, check to inspire callback.
  /// @note Callback can only inspired once.
  void TrySchedule() {
    if (state_.has_callback) {
      if (IsReady()) {
        if (state_.schedule_flag.test_and_set() == false) {
          state_.callback->SetValue(GetValue());
          state_.callback->Schedule(state_.callback);
        }
      } else if (IsFailed()) {
        if (state_.schedule_flag.test_and_set() == false) {
          state_.callback->SetException(GetException());
          state_.callback->Schedule(state_.callback);
        }
      }
    }
  }

  /// @brief Protected from wild pointer.
  inline void Detach() noexcept {
    auto a = state_.ref_count.fetch_sub(1, std::memory_order_acq_rel);
    assert(a >= 1);
    if (a == 1) {
      delete this;
    }
  }

 private:
  FutureState<T...> state_;
};

/// @brief Single parameter version of FutureImpl.
/// @private
template <typename T>
class FutureImpl<T> : public FutureImplBase {
 public:
  friend class Promise<T>;
  friend class Future<T>;

  FutureImpl() noexcept { FutureImplBase::Init(&state_); }

  /// @brief Used by MakeReadyFuture to create ready future.
  explicit FutureImpl(T&& value) noexcept : state_(std::move(value)) {
    FutureImplBase::Init(&state_);
    state_.ready = true;
    state_.has_result = true;
  }

  ~FutureImpl() noexcept = default;

  /// @brief Same as multiple version.
  T&& GetValue() noexcept {
    assert(state_.ready);
    state_.ready = false;
    return std::move(state_.value);
  }

  /// @brief Same as multiple version.
  const T& GetConstValue() const noexcept {
    assert(state_.ready);
    return state_.value;
  }

  /// @brief Same as multiple version.
  template <typename FutureType, typename PromiseType = typename FutureType::PromiseType,
            typename Func = Function<FutureType(T&&)>>
  void SetCallback(PromiseType&& promise, Func&& func, Executor* executor) {
    state_.callback =
        new ContinuationWithValue<Func, PromiseType, T>(std::forward<Func>(func), std::forward<PromiseType>(promise));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Same as multiple version.
  template <typename FutureType, typename Func = Function<FutureType(T&&)>>
  void SetTerminalCallback(Func&& func, Executor* executor) {
    state_.callback = new TerminalWithValue<Func, T>(std::forward<Func>(func));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Same as multiple version.
  template <typename FutureType, typename PromiseType = typename FutureType::PromiseType,
            typename Func = Function<FutureType(Future<T>&&)>>
  void SetCallbackWrapped(PromiseType&& promise, Func&& func, Executor* executor) {
    state_.callback =
        new ContinuationWithFuture<Func, PromiseType, T>(std::forward<Func>(func), std::forward<PromiseType>(promise));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Same as multiple version.
  template <typename FutureType, typename Func = Function<FutureType(Future<T>&&)>>
  void SetTerminalCallbackWrapped(Func&& func, Executor* executor) {
    state_.callback = new TerminalWithFuture<Func, T>(std::forward<Func>(func));
    if (executor) state_.callback->SetExecutor(executor);
    state_.has_callback = true;

    std::lock_guard<std::mutex> lock(state_.mtx);
    if (HasResult()) {
      TrySchedule();
    }
  }

  /// @brief Same as multiple version.
  void SetValue(T&& value) {
    state_.value = std::move(value);
    state_.ready = true;
    {
      std::lock_guard<std::mutex> lock(state_.mtx);
      state_.has_result = true;
    }
    TrySchedule();
  }

  /// @brief Same as multiple version.
  void SetException(const Exception& e) {
    state_.exception = e;
    state_.failed = true;
    {
      std::lock_guard<std::mutex> lock(state_.mtx);
      state_.has_result = true;
    }
    TrySchedule();
  }

  /// @brief Same as multiple version.
  void SetException(Exception&& e) {
    state_.exception = std::move(e);
    state_.failed = true;
    {
      std::lock_guard<std::mutex> lock(state_.mtx);
      state_.has_result = true;
    }
    TrySchedule();
  }

 private:
  /// @brief Same as multiple version.
  void TrySchedule() {
    if (state_.has_callback) {
      if (IsReady()) {
        if (state_.schedule_flag.test_and_set() == false) {
          state_.callback->SetValue(GetValue());
          state_.callback->Schedule(state_.callback);
        }
      } else if (IsFailed()) {
        if (state_.schedule_flag.test_and_set() == false) {
          state_.callback->SetException(GetException());
          state_.callback->Schedule(state_.callback);
        }
      }
    }
  }

  /// @brief Same as multiple version.
  inline void Detach() noexcept {
    auto a = state_.ref_count.fetch_sub(1, std::memory_order_acq_rel);
    assert(a >= 1);
    if (a == 1) {
      delete this;
    }
  }

 private:
  FutureState<T> state_;
};

/// @brief Check a type is Future or not.
/// @private
template <typename... T>
struct is_future {
  static const bool value = false;
};

/// @brief Support non reference.
/// @private
template <typename... T>
struct is_future<Future<T...>> {
  static const bool value = true;
};

/// @brief Support right reference.
/// @private
template <typename... T>
struct is_future<Future<T...>&&> {
  static const bool value = true;
};

/// @brief Helper for is_future.
/// @private
template <typename... T>
constexpr bool is_future_v = is_future<T...>::value;

/// @brief Variable parameter version of Future.
template <typename... T>
class Future {
 public:
  /// @private
  using ValueType = std::tuple<T...>;
  /// @private
  using TupleValueType = std::tuple<T...>;
  /// @private
  using Type = Future<T...>;
  /// @private
  using PromiseType = Promise<T...>;

  Future() noexcept = default;

  ~Future() noexcept { Detach(); }

  /// @brief Used by promise to get pair future.
  explicit Future(FutureImpl<T...>* future) noexcept : future_(future) {
    if (future) {
      future_->Attach();
    }
  }

  /// @brief Used by MakeReadyFuture.
  explicit Future(MakeReadyFutureHelper no_sense, T&&... v) noexcept {
    future_ = new FutureImpl<T...>(std::make_tuple<T...>(std::move(v)...));
  }

  /// @brief Used by MakeExceptionFuture.
  explicit Future(const Exception& e) noexcept {
    future_ = new FutureImpl<T...>();
    future_->SetException(e);
  }

  Future(Future&& o) noexcept : future_(std::exchange(o.future_, nullptr)) {}

  Future& operator=(Future&& o) noexcept {
    Detach();
    future_ = std::exchange(o.future_, nullptr);
    return *this;
  }

  bool IsReady() const noexcept { return future_->IsReady(); }

  bool IsFailed() const noexcept { return future_->IsFailed(); }

  /// @brief Deprecated due to bad function name, use IsReady instead.
  [[deprecated("Use IsReady instead")]] bool is_ready() const noexcept { return IsReady(); }

  /// @brief Deprecated due to bad function name, use IsFailed instead.
  [[deprecated("Use IsFailed instead")]] bool is_failed() const noexcept { return IsFailed(); }

  /// @brief Add a callback to the future. If the callback only has one parameter and
  ///        its type is @ref Future<T...>, when the future becomes ready or failed,
  ///        the callback will be called and the future will be passed as pararemter
  ///        to callback. If the callback has more than one parameters or its parameter
  ///        type is not future<T...>, when this future is ready, this callback will be
  ///        called with future's value as parameters. And if this future is failed, this
  ///        callback will be ignored.
  template <typename Func>
  auto Then(Func&& func) {
    using func_type = std::decay_t<Func>;
    using tuple_type = typename function_traits<func_type>::tuple_type;
    if constexpr (function_traits<Func>::arity == 1) {
      using first_arg_type = typename std::tuple_element<0, tuple_type>::type;
      if constexpr (is_future<std::decay_t<first_arg_type>>::value) {
        return InnerThenWrapped(std::forward<Func>(func));
      } else {
        return InnerThen(std::forward<Func>(func));
      }
    } else {
      return InnerThen(std::forward<Func>(func));
    }
  }

  /// @brief Add a callback to future, and the callback will run in specified executor.
  /// @note Excutor's life time should be longer than future task execution time.
  template <typename Func>
  auto Then(Executor* executor, Func&& func) {
    using func_type = std::decay_t<Func>;
    using tuple_type = typename function_traits<func_type>::tuple_type;
    if constexpr (function_traits<Func>::arity == 1) {
      using first_arg_type = typename std::tuple_element<0, tuple_type>::type;
      if constexpr (is_future<std::decay_t<first_arg_type>>::value) {
        return InnerThenWrapped(std::forward<Func>(func), executor);
      } else {
        return InnerThen(std::forward<Func>(func), executor);
      }
    } else {
      return InnerThen(std::forward<Func>(func), executor);
    }
  }

  /// @brief Wait result the future to return.
  /// @note No recommend to use it due to the function has poor performance.
  /// @param timeout milliseconds that to be waited.
  /// @return false when timeout, true when result ready or exception occured.
  bool Wait(const uint32_t timeout = UINT32_MAX) noexcept { return future_->Wait(timeout); }

  /// @brief Get value from a ready future, after call this function, this future is not ready anymore.
  std::tuple<T...> GetValue() noexcept { return future_->GetValue(); }

  /// @brief Get the const reference of value from a ready future, after call this function,
  ///        this future is still ready.
  const std::tuple<T...>& GetConstValue() const noexcept { return future_->GetConstValue(); }

  /// @brief Get exception from a failed future, after call this function, this future is not failed anymore.
  Exception GetException() noexcept { return future_->GetException(); }

 private:
  void Detach() {
    if (future_) {
      future_->Detach();
      future_ = nullptr;
    }
  }

  /// @brief Add a callback to future. When this future is ready, this callback will run with
  ///        future value as parameters. If this future is failed, this callback will be ignored.
  template <typename Func, typename R = typename std::invoke_result<Func, T&&...>::type>
  typename std::enable_if<!std::is_void<R>::value, R>::type
  InnerThen(Func&& func, Executor* executor = nullptr) {
    typename R::PromiseType promise;  // transport a empty promise to callback
    auto future = promise.GetFuture();
    future_->template SetCallback<R>(std::move(promise), std::forward<Func>(func), executor);
    return future;
  }

  /// @brief Add a callback to future. When this future is ready, this callback will run with
  ///        future value as parameters but no return. If this future is failed, this callback
  ///        will be ignored.
  template <typename Func, typename R = typename std::invoke_result<Func, T&&...>::type>
  typename std::enable_if<std::is_void<R>::value, R>::type
  InnerThen(Func&& func, Executor* executor = nullptr) {
    future_->template SetTerminalCallback<Type>(std::forward<Func>(func), executor);
  }

  /// @brief Add a callback to future. When this future is ready or failed, this callback will run
  ///        with this future as parameter. Callback should call @ref IsFailed to check is ready
  ///        or failed first.
  template <typename Func, typename R = typename std::invoke_result<Func, Future<T...>&&>::type>
  typename std::enable_if<!std::is_void<R>::value, R>::type
  InnerThenWrapped(Func&& func, Executor* executor = nullptr) {
    typename R::PromiseType promise;
    auto future = promise.GetFuture();
    future_->template SetCallbackWrapped<R>(std::move(promise), std::forward<Func>(func), executor);
    return future;
  }

  /// @brief Add a callback to future. When this future is ready or failed, this callback will run
  ///        with this future as parameter but no return. Callback should call @ref IsFailed to
  ///        check is ready or failed first.
  template <typename Func, typename R = typename std::invoke_result<Func, Future<T...>&&>::type>
  typename std::enable_if<std::is_void<R>::value, R>::type
  InnerThenWrapped(Func&& func, Executor* executor = nullptr) {
    future_->template SetTerminalCallbackWrapped<Type>(std::forward<Func>(func), executor);
  }

 private:
  FutureImpl<T...>* future_ = nullptr;

  friend class ContinuationBase;
};

/// @brief Future's specialized version with single parameter.
///        In this version, use GetValue0 instead of GetValue for higher performance.
template <typename T>
class Future<T> {
 public:
  /// @private
  using ValueType = T;
  /// @private
  using TupleValueType = std::tuple<T>;
  /// @private
  using Type = Future<T>;
  /// @private
  using PromiseType = Promise<T>;

  Future() noexcept = default;

  ~Future() noexcept { Detach(); }

  /// @brief Used by getting future through promise.
  explicit Future(FutureImpl<T>* future) noexcept : future_(future) {
    if (future) {
      future_->Attach();
    }
  }

  /// @brief Used by MakeReadyFuture.
  explicit Future(MakeReadyFutureHelper no_sense, T&& v) noexcept { future_ = new FutureImpl<T>(std::move(v)); }

  /// @brief Used by MakeExceptionFuture.
  explicit Future(const Exception& e) noexcept {
    future_ = new FutureImpl<T>();
    future_->SetException(e);
  }

  Future(Future&& o) noexcept : future_(std::exchange(o.future_, nullptr)) {}

  Future& operator=(Future&& o) noexcept {
    Detach();
    future_ = std::exchange(o.future_, nullptr);
    return *this;
  }

  bool IsReady() const noexcept { return future_->IsReady(); }

  bool IsFailed() const noexcept { return future_->IsFailed(); }

  /// @brief Deprecated due to bad function name, use IsReady instead.
  [[deprecated("Use IsReady instead")]] bool is_ready() const noexcept { return IsReady(); }

  /// @brief Deprecated due to bad function name, use IsFailed instead.
  [[deprecated("Use IsFailed instead")]] bool is_failed() const noexcept { return IsFailed(); }

  /// @brief Add a callback to future.
  ///        If the callback only has one parameter and its type is @ref Future<T...>,
  ///        when the future becomes ready or failed, the callback will be called and
  ///        the future will be passed as pararemter to callback.
  ///        If the callback has more than one parameters or its parameter type is not
  ///        future<T...>, when this future is ready, this callback will be called with
  ///        future's value as parameters. And if this future is failed, this callback
  ///        will be ignored.
  template <typename Func>
  auto Then(Func&& func) {
    using func_type = std::decay_t<Func>;
    using tuple_type = typename function_traits<func_type>::tuple_type;
    if constexpr (function_traits<Func>::arity == 1) {
      using first_arg_type = typename std::tuple_element<0, tuple_type>::type;
      if constexpr (is_future<std::decay_t<first_arg_type>>::value) {
        return InnerThenWrapped(std::forward<Func>(func));
      } else {
        return InnerThen(std::forward<Func>(func));
      }
    } else {
      return InnerThen(std::forward<Func>(func));
    }
  }

  /// @brief Add a callback to future, and the callback will run in specified executor.
  /// @note Excutor's life time should be longer than future task execution time.
  template <typename Func>
  auto Then(Executor* executor, Func&& func) {
    using func_type = std::decay_t<Func>;
    using tuple_type = typename function_traits<func_type>::tuple_type;
    if constexpr (function_traits<Func>::arity == 1) {
      using first_arg_type = typename std::tuple_element<0, tuple_type>::type;
      if constexpr (is_future<std::decay_t<first_arg_type>>::value) {
        return InnerThenWrapped(std::forward<Func>(func), executor);
      } else {
        return InnerThen(std::forward<Func>(func), executor);
      }
    } else {
      return InnerThen(std::forward<Func>(func), executor);
    }
  }

  /// @brief Wait result the future to return.
  /// @note No recommend to use it due to poor performance.
  bool Wait(const uint32_t timeout = UINT32_MAX) noexcept { return future_->Wait(timeout); }

  /// @brief Get value from a ready future, after call this function, this future is not ready anymore.
  std::tuple<T> GetValue() noexcept { return std::make_tuple<T>(future_->GetValue()); }

  /// @brief Get the const reference of value from a ready future, after call this function,
  ///        this future is still ready.
  const T& GetConstValue() const noexcept { return future_->GetConstValue(); }

  /// @brief High performace method to get value from a ready future, after call this function,
  ///        this future is not ready anymore.
  T GetValue0() noexcept { return future_->GetValue(); }

  /// @brief Get exception from a failed future, after call this function, this future is not failed anymore.
  Exception GetException() noexcept { return future_->GetException(); }

 private:
  void Detach() {
    if (future_) {
      future_->Detach();
      future_ = nullptr;
    }
  }

  /// @brief Add a callback to future.
  ///        When this future is ready, this callback will run with future value as
  ///        parameters. If this future is failed, this callback will be ignored.
  template <typename Func, typename R = typename std::invoke_result<Func, T&&>::type>
  typename std::enable_if<!std::is_void<R>::value, R>::type
  InnerThen(Func&& func, Executor* executor = nullptr) {
    typename R::PromiseType promise;  // transport a empty promise to callback
    auto future = promise.GetFuture();
    future_->template SetCallback<R>(std::move(promise), std::forward<Func>(func), executor);
    return future;
  }

  /// @brief Add a callback to future.
  ///        When this future is ready, this callback will run with future value as
  ///        parameters but no return. If this future is failed, this callback will
  ///        be ignored.
  template <typename Func, typename R = typename std::invoke_result<Func, T &&>::type>
  typename std::enable_if<std::is_void<R>::value, R>::type
  InnerThen(Func&& func, Executor* executor = nullptr) {
    future_->template SetTerminalCallback<Type>(std::forward<Func>(func), executor);
  }

  /// @brief Add a callback to future.
  ///        When this future is ready or failed, this callback will run with this
  ///        future as parameter. Callback should call @ref IsFailed to check is ready
  ///        or failed first.
  template <typename Func, typename R = typename std::invoke_result<Func, Future<T> &&>::type>
  typename std::enable_if<!std::is_void<R>::value, R>::type
  InnerThenWrapped(Func&& func, Executor* executor = nullptr) {
    typename R::PromiseType promise;
    auto future = promise.GetFuture();
    future_->template SetCallbackWrapped<R>(std::move(promise), std::forward<Func>(func), executor);
    return future;
  }

  /// @brief Add a callback to future.
  ///        When this future is ready or failed, this callback will run with this
  ///        future as parameter. Callback should call @ref IsFailed to check is ready
  ///        or failed first.
  template <typename Func, typename R = typename std::invoke_result<Func, Future<T> &&>::type>
  typename std::enable_if<std::is_void<R>::value, R>::type
  InnerThenWrapped(Func&& func, Executor* executor = nullptr) {
    // PromiseType promise;
    future_->template SetTerminalCallbackWrapped<Type>(std::forward<Func>(func), executor);
  }

 private:
  FutureImpl<T>* future_ = nullptr;

  friend class ContinuationBase;
};

/// @brief Variable parameter version of Promise.
template <typename... T>
class Promise {
 public:
  /// @private
  using FutureType = Future<T...>;

  Promise() noexcept : retrieved_(false) { future_ = new FutureImpl<T...>(); }

  Promise(Promise&& o) noexcept
      : retrieved_(std::exchange(o.retrieved_, false)), future_(std::exchange(o.future_, nullptr)) {}

  Promise& operator=(Promise&& o) noexcept {
    if (this == &o) {
      return *this;
    }

    Detach();
    retrieved_ = std::exchange(o.retrieved_, false);
    future_ = std::exchange(o.future_, nullptr);
    return *this;
  }

  ~Promise() noexcept { Detach(); }

  void SetValue(T&&... value) {
    std::tuple<T...> tuple = std::make_tuple<T...>(std::move(value)...);
    future_->SetValue(std::move(tuple));
  }

  void SetValue(std::tuple<T...>&& value) { future_->SetValue(std::move(value)); }

  void SetException(const Exception& e) { future_->SetException(e); }

  void SetException(Exception&& e) { future_->SetException(e); }

  bool IsReady() { return future_->IsReady(); }

  bool IsFailed() { return future_->IsFailed(); }

  bool IsResolved() const { return future_->HasResult(); }

  /// @brief Deprecated due to bad function name, use IsReady instead.
  [[deprecated("Use IsReady instead")]] bool is_ready() { return IsReady(); }

  /// @brief Deprecated due to bad function name, use IsFailed instead.
  [[deprecated("Use IsFailed instead")]] bool is_failed() { return IsFailed(); }

  /// @brief Get pair future.
  Future<T...> GetFuture() noexcept {
    retrieved_ = true;
    return Future<T...>(future_);
  }

  /// @brief Deprecated due to bad function name, use GetFuture instead.
  [[deprecated("Use GetFuture instead")]] Future<T...> get_future() noexcept { return GetFuture(); }

 private:
  void Detach() {
    if (future_) {
      if (!retrieved_) {
        future_->Detach();
      } else {
        if (!future_->HasResult()) {
          future_->SetException(CommonException("Broken promise"));
        }
        future_->Detach();
      }
      future_ = nullptr;
    }
  }

 private:
  // Whether pair future is got by outter layer.
  bool retrieved_;

  FutureImpl<T...>* future_ = nullptr;

  friend class Future<T...>;
};

/// @brief Single parameter version of Promise.
template <typename T>
class Promise<T> {
 public:
  using FutureType = Future<T>;

  Promise() noexcept : retrieved_(false) { future_ = new FutureImpl<T>(); }

  Promise(Promise&& o) noexcept : retrieved_(std::exchange(o.retrieved_, false)),
                                  future_(std::exchange(o.future_, nullptr)) {}

  Promise& operator=(Promise&& o) noexcept {
    if (this == &o) {
      return *this;
    }

    Detach();
    retrieved_ = std::exchange(o.retrieved_, false);
    future_ = std::exchange(o.future_, nullptr);
    return *this;
  }

  ~Promise() noexcept { Detach(); }

  void SetValue(std::tuple<T>&& value) { future_->SetValue(std::move(std::get<0>(value))); }

  void SetValue(T&& value) { future_->SetValue(std::forward<T>(value)); }

  void SetException(const Exception& e) { future_->SetException(e); }

  void SetException(Exception&& e) { future_->SetException(e); }

  bool IsReady() const { return future_->IsReady(); }

  bool IsFailed() const { return future_->IsFailed(); }

  bool IsResolved() const { return future_->HasResult(); }

  /// @brief Deprecated due to bad function name, use IsReady instead.
  [[deprecated("Use IsReady instead")]] bool is_ready() { return IsReady(); }

  /// @brief Deprecated due to bad function name, use IsFailed instead.
  [[deprecated("Use IsFailed instead")]] bool is_failed() { return IsFailed(); }

  Future<T> GetFuture() noexcept {
    retrieved_ = true;
    return Future<T>(future_);
  }

  /// @brief Deprecated due to bad function name, use GetFuture instead.
  [[deprecated("Use GetFuture instead")]] Future<T> get_future() noexcept { return GetFuture(); }

 private:
  void Detach() {
    if (future_) {
      if (!retrieved_) {
        future_->Detach();
      } else {
        if (!future_->HasResult()) {
          future_->SetException(CommonException("Broken promise"));
        }
        future_->Detach();
      }
      future_ = nullptr;
    }
  }

 private:
  // Whether pair future is got by outter layer.
  bool retrieved_;

  FutureImpl<T>* future_ = nullptr;

  friend class Future<T>;
};

/// @brief Chain current future with next future returned by callback.
template <typename PromiseType, typename FutureType>
void ContinuationBase::ChainFuture(FutureType&& fut, PromiseType&& promise) {
  // Combine current promise with value or exception, make state clearly.
  if (fut.IsReady()) {
    promise.SetValue(fut.future_->GetValue());
  } else if (fut.IsFailed()) {
    promise.SetException(fut.GetException());
  } else {
    // When async, current promise's state may not clearly by now.
    using ResultFutureType = typename PromiseType::FutureType;
    // Use superior callback's return to register a new callback for setting state of superior then's promise.
    fut.Then([pr = std::forward<PromiseType>(promise)](ResultFutureType&& result) mutable {
      if (result.IsReady()) {
        pr.SetValue(result.future_->GetValue());
      } else {
        pr.SetException(result.GetException());
      }
      return MakeReadyFuture<>();
    });
  }
}

}  // namespace trpc
