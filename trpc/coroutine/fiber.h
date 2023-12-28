//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/fiber.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <chrono>
#include <cstddef>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

#include "trpc/util/chrono/chrono.h"
#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

namespace fiber::detail {

class ExitBarrier;

}  // namespace fiber::detail

namespace fiber {

/// @brief Fiber create and running attributes
enum class Launch {
  /// @brief Default launch model,framework will manage
  Post,

  /// @brief If possible, yield current pthread worker to user's code.
  Dispatch
};

constexpr std::string_view kCreateFiberEntityError =
    "Create Fiber Entity fail,maybe too many. Check `/proc/[pid]/maps` to "
    "see if there are too many memory regions. There's a limit at around 64K "
    "by default. If you reached the limit, try either disabling guard page or "
    "increasing `vm.max_map_count` (suggested).";
}  // namespace fiber

class FiberExecutionContext;

/// @brief Analogous to `std::thread`, but it's for fiber.
/// @note  It only uses in fiber runtime.
///        If the program will create too many fibers,
///        it is recommended that you use the `StartFiberDetached` interface to
///        prevent possible failures when allocated fiber memory stack.
class Fiber {
 public:
  static constexpr auto kNearestSchedulingGroup = std::numeric_limits<std::size_t>::max() - 1;
  static constexpr auto kUnspecifiedSchedulingGroup = std::numeric_limits<std::size_t>::max();

  /// @brief Fiber create and running attributes
  struct Attributes {
    /// @brief How the fiber is launched.
    fiber::Launch launch_policy = fiber::Launch::Post;

    /// @brief Which scheduling group should the fiber be *initially* placed in.
    ///        Note that unless you also have `scheduling_group_local` set, the fiber can be
    ///        stolen by workers belonging to other scheduling group.
    std::size_t scheduling_group = kNearestSchedulingGroup;

    /// @brief If set, fiber's start procedure is run in this execution context.
    ///        `Fiber` will take a reference to the execution context, so you're safe to
    ///        release your own ref. once `Fiber` is constructed.
    FiberExecutionContext* execution_context = nullptr;

    /// @brief If set, `scheduling_group` is enforced.
    ///        (i.e., work-stealing is disabled on this fiber.)
    bool scheduling_group_local = false;

    /// @brief If Set, it is a reactor fiber
    /// @note  Only used inside the framework
    bool is_fiber_reactor = false;
  };

  /// @brief Create an empty (invalid) fiber.
  Fiber();

  /// @brief Create a fiber with attributes. It will run from `start`.
  Fiber(const Attributes& attr, Function<void()>&& start);

  /// @brief Create fiber by calling `f` with args.
  template <class F, class... Args, class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
  explicit Fiber(F&& f, Args&&... args)
      : Fiber(Attributes(), std::forward<F>(f), std::forward<Args>(args)...) {}

  /// @brief Create fiber by calling `f` with args, using launch policy `policy`.
  template <class F, class... Args, class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
  Fiber(fiber::Launch policy, F&& f, Args&&... args)
      : Fiber(Attributes{.launch_policy = policy}, std::forward<F>(f),
              std::forward<Args>(args)...) {}

  /// @brief Create fiber by calling `f` with args, using attributes `attr`.
  template <class F, class... Args, class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
  Fiber(const Attributes& attr, F&& f, Args&&... args)
      : Fiber(attr, [f = std::forward<F>(f),
                     // P0780R2 is not implemented as of now (GCC 8.2).
                     t = std::tuple(std::forward<Args>(args)...)] {
          std::apply(std::forward<F>(f)(std::move(t)));
        }) {}

  /// @brief Special case if no parameter is passed to `F`, in this case we don't need
  ///       an indirection (the extra lambda).
  template <class F, class = std::enable_if_t<std::is_invocable_v<F&&>>>
  Fiber(const Attributes& attr, F&& f) : Fiber(attr, Function<void()>(std::forward<F>(f))) {}

  /// @note If a `Fiber` object which owns a fiber is destructed with no prior call to
  ///       `join()` or `detach()`, it leads to abort.
  ~Fiber();

  /// @brief Detach the fiber.
  void Detach();

  /// @brief Wait for the fiber to exit.
  /// @note  Can be run in both pthread context and fiber context.
  void Join();

  /// @brief Test if we can call `join()` on this object.
  bool Joinable() const;

  Fiber(Fiber&&) noexcept;
  Fiber& operator=(Fiber&&) noexcept;

 private:
  object_pool::LwSharedPtr<fiber::detail::ExitBarrier> join_impl_;
};

/// @brief Create a fiber and run it
/// @note  If the program creates too many fibers, system has not memory, it will return failure.
/// @code
/// examples:
///
/// 1. create fiber and run it
/// bool start_fiber = trpc::StartFiberDetached([] { // do your work here });
/// if (!start_fiber) { // ... }
///
/// 2. use with `FiberLatch` in fiber runtime
///
/// trpc::FiberLatch l(1);
/// bool start_fiber = trpc::StartFiberDetached([] {
///   // ...
///   l.CountDown();
/// });
/// if (!start_fiber) {
///   l.CountDown();
///   std::cout << "StartFiberDetached to execute task faild,please retry latter";
/// }
/// l.Wait();
///
/// 3. use with `trpc::Latch` outside fiber runtime
///
/// trpc::Latch l(1);
/// bool start_fiber = trpc::StartFiberDetached([] {
///   // ...
///   l.count_down();
/// });
/// if (!start_fiber) {
///   l.count_down();
///   std::cout << "StartFiberDetached to execute task faild, please retry latter";
/// }
/// l.wait();
/// @endcode
bool StartFiberDetached(Function<void()>&& start_proc);

/// @brief Create a fiber by attrs and run it
bool StartFiberDetached(Fiber::Attributes&& attrs, Function<void()>&& start_proc);

/// @brief Batch Create fibers and run all
/// @note  It's all going to be all or none
bool BatchStartFiberDetached(std::vector<Function<void()>>&& start_procs);

/// @brief Yield execution.
///        If there's no other fiber is ready to run, the caller will be rescheduled immediately.
/// @note  It only uses in fiber runtime.
void FiberYield();

/// @brief Block calling pthread or calling fiber until `expires_at`.
/// @note  It can be used in pthread context and fiber context.
void FiberSleepUntil(const std::chrono::steady_clock::time_point& expires_at);

/// @brief Block calling pthread or calling fiber for `expires_in`.
/// @note  It can be used in pthread context and fiber context.
void FiberSleepFor(const std::chrono::nanoseconds& expires_in);

/// @brief `SleepUntil` for clocks other than `std::steady_clock`.
/// @note  It can be used in pthread context and fiber context.
template <class Clock, class Duration>
void FiberSleepUntil(const std::chrono::time_point<Clock, Duration>& expires_at) {
  return FiberSleepUntil(ReadSteadyClock() + (expires_at - Clock::now()));
}

/// @brief `SleepFor` for durations other than `std::chrono::nanoseconds`.
/// @note  It can be used in pthread context and fiber context.
template <class Rep, class Period>
void FiberSleepFor(const std::chrono::duration<Rep, Period>& expires_in) {
  return FiberSleepFor(static_cast<std::chrono::nanoseconds>(expires_in));
}

/// @brief Whether the currently running thread is a fiber worker thread
bool IsRunningInFiberWorker();

/// @brief Get Fiber Count .
std::size_t GetFiberCount();

/// @brief Get the size of the fibers that all be waiting to running in the run queue
std::size_t GetFiberQueueSize();

}  // namespace trpc
