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

#include <atomic>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "trpc/future/future.h"
#include "trpc/util/thread/latch.h"

namespace trpc {

/// @namespace trpc::future
/// @brief Namespace contains interfaces used for future.
namespace future {

/// @brief Block and wait for the future to return a result or exeception.
/// @param f The input future.
/// @return A future with the result or exception.
/// @note The input future will be moved, so you can't use it again after call this function.
template <class... Ts>
Future<Ts...> BlockingGet(Future<Ts...>&& f) {
  Latch latch(1);
  Future<Ts...> future;
  std::move(f).Then([&](Future<Ts...>&& fut) {
    future = std::move(fut);
    latch.count_down();
    return MakeReadyFuture<>();
  });
  latch.wait();

  return future;
}

/// @brief Block and wait for the future to return a result or exeception with timeout.
/// @param f The input future.
/// @return If the future is resolved within the timeout milliseconds, it will return a future with the result or
///         exception. Otherwise it will return std::nullopt.
/// @note The input future will be moved, so you can't use it again after call this function.
template <class... Ts>
std::optional<Future<Ts...>> BlockingTryGet(Future<Ts...>&& f, uint32_t timeout_ms) {
  struct State {
    Future<Ts...> future;
    Latch latch{1};
  };

  auto state = std::make_shared<State>();
  std::move(f).Then([state](Future<Ts...>&& fut) {
    state->future = std::move(fut);
    state->latch.count_down();
    return MakeReadyFuture<>();
  });

  bool ret = state->latch.wait_for(std::chrono::milliseconds(timeout_ms));
  if (!ret) {
    return std::nullopt;
  }

  return std::move(state->future);
}

}  // namespace future

namespace {

/// @brief To move value to heap.
/// @private
template <typename T>
class DoWithKeeper {
 public:
  explicit DoWithKeeper(T &&value) noexcept : value_(std::move(value)) {}
  T &GetValue() noexcept { return value_; }

 private:
  T value_;
};

/// @brief Store stop function, user function and user promise.
/// @private
template <typename Stop, typename Func, typename R = typename std::invoke_result_t<Func>>
struct DoUntilState {
  Stop stop;
  Func func;
  Promise<> pr;

  DoUntilState(Stop&& s, Func&& f) : stop(std::forward<Stop>(s)), func(std::forward<Func>(f)) {}

  /// @brief Run until stop return true or user function return exceptional future.
  void Run(std::unique_ptr<DoUntilState>&& sp, bool nostop = false) {
    if (!nostop && stop()) {
      pr.SetValue();
    } else {
      func().Then([sp = std::move(sp)](R&& ft) mutable {
        if (ft.IsFailed())
          sp->pr.SetException(ft.GetException());
        else
          sp->Run(std::move(sp));
        return MakeReadyFuture<>();
      });
    }
  }
};

template <typename Tuple, typename F, std::size_t... Indices>
constexpr void for_each_impl(Tuple &&tuple, F &&f, std::index_sequence<Indices...>) {
  int ignore[] = {1, (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...};
  (void)ignore;
}

template <typename Tuple, typename F>
constexpr void for_each(Tuple &&tuple, F &&f) {
  constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
  for_each_impl(std::forward<Tuple>(tuple), std::forward<F>(f), std::make_index_sequence<N>{});
}

template <typename Tuple>
class WhenAllFunction {
 public:
  WhenAllFunction(std::shared_ptr<Promise<Tuple>> pr, Tuple &futs,
                  const std::shared_ptr<std::atomic_uint32_t> &counter)
    : pr_(pr), futs_(futs), counter_(counter) {}

  template <typename Fut>
  void operator()(Fut &fut) {
    auto &futs = futs_;
    auto &input_fut = fut;
    DoWith(std::move(fut), [&futs, &input_fut, counter = counter_, pr = pr_](Fut &fut) mutable {
      return fut.Then(
          [&futs, &input_fut, counter, pr](std::remove_reference_t<Fut> &&result) mutable {
            input_fut = std::move(result);
            auto current = (*counter).fetch_add(1);
            if (current + 1 == std::tuple_size<Tuple>::value) {
              pr->SetValue(std::move(futs));
            }
            auto future = MakeReadyFuture<>();
            return future;
          });
    });
  }

 private:
  std::shared_ptr<Promise<Tuple>> pr_;

  Tuple &futs_;

  std::shared_ptr<std::atomic_uint32_t> counter_;
};

}  // namespace

/// @brief Move a object to heap memory, and hold it until func's future was resolved.
///        The object will be passed into func as lvalue reference.
template <typename T, typename Func, typename R = typename std::invoke_result<Func, T &>::type>
R DoWith(T &&value, Func &&func) {
  auto keeper = std::make_unique<DoWithKeeper<T>>(std::move(value));
  return func(keeper->GetValue()).Then([kp = std::move(keeper)](R &&future) {
    return std::move(future);
  });
}

/// @brief Execute func repeatedly until stop function return true.
///        If the func returns an exception future, the loop will terminate immediately and
///        return a same exception. Eachtime stop function runs before func.
/// @return Future<> Ready when stopped, otherwise fails when func failed.
template <typename Stop, typename Func, typename R = std::invoke_result_t<Func>,
          typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<Stop>, bool>>,
          typename = std::enable_if_t<is_future_v<R>>>
Future<> DoUntil(Stop&& stop, Func&& func) {
  if (stop()) {
    return MakeReadyFuture<>();
  }

  auto sp = std::make_unique<DoUntilState<Stop, Func>>(std::forward<Stop>(stop), std::forward<Func>(func));
  auto ft = sp->pr.GetFuture();
  sp->Run(std::move(sp), true);

  return ft;
}

/// @brief Execute func repeatedly until function return a ready Future<bool> with false.
///        If the func returns an exception future, the loop will terminate immediately and return a same exception.
/// @return Future<> Ready when stopped, otherwise fails when func failed.
template <typename Func, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<Func>, Future<bool>>>>
Future<> DoUntil(Func&& func) {
  auto stop = std::make_shared<bool>(false);
  return DoUntil(
    [stop]() { return *stop; },
    [func = std::forward<Func>(func), stop]() {
      return func().Then([stop](bool ok) {
        *stop = !ok;
        return MakeReadyFuture<>();
      });
    });
}

/// @brief Execute func repeatedly while cond function return true.
///        If the func returns an exception future, the loop will terminate immediately and return a same exception.
///        Eachtime cond function runs before func.
/// @return Future<> Ready when stopped, otherwise fails when func failed.
template <typename Condition, typename Func, typename R = typename std::invoke_result_t<Func>,
          typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<Condition>, bool>>,
          typename = std::enable_if_t<is_future_v<R>>>
Future<> DoWhile(Condition&& cond, Func&& func) {
  return DoUntil([cond = std::forward<Condition>(cond)]() { return !cond(); }, std::forward<Func>(func));
}

/// @brief Execute func repeatedly forever. If the func returns an exception future, the loop will terminate
///        immediately and return a same exception.
/// @return Future<> Fails when func fails, never be ready.
template <typename Func, typename R = std::invoke_result_t<Func>>
Future<> Repeat(Func&& func) {
  return DoWhile([]() { return true; }, std::forward<Func>(func));
}

/// @brief Execute func for every iterator in the range [begin, end), in order.
/// @param begin First iterator.
/// @param end Last iterator.
/// @param func User function.
/// @return Future<> Ready when completed, otherwise failed when func fails.
template <typename Iterator, typename Func>
Future<> DoForEach(Iterator&& begin, Iterator&& end, Func&& func) {
  if constexpr (std::is_invocable_v<Func, typename Iterator::value_type>) {
    auto state = std::make_shared<Iterator>(std::forward<Iterator>(begin));
    return DoUntil([state, end = std::forward<Iterator>(end)]() { return (*state) == end; },
                   [state, func = std::forward<Func>(func)]() { return func(*(*state)++); });
  } else {
    static_assert(std::is_invocable_v<Func>);
    return DoUntil([begin = std::forward<Iterator>(begin),
                    end = std::forward<Iterator>(end)]() mutable {
                      return begin++ == end;
                    },
                    std::forward<Func>(func));
  }
}

/// @brief Execute func with each element in a container, in order.
/// @tparam T For Partitial Specialization.
/// @param container Iterate over.
/// @param func User function.
/// @return Future<>
template <typename Container, typename Func, typename = std::invoke_result_t<Func, typename Container::value_type>>
Future<> DoForEach(Container& container, Func&& func) {
  return DoForEach(std::begin(container), std::end(container), std::forward<Func>(func));
}

/// @brief Execute func for several times.
/// @param n How many times do fun execute.
/// @param func User func.
/// @return Future<> Ready when completed, otherwise fails when func fails.
template <typename Func>
Future<> DoFor(std::size_t n, Func&& func) {
  if constexpr (std::is_invocable_v<Func, std::size_t>) {
    auto state = std::make_shared<std::size_t>(n);
    return DoUntil([state]() { return *state == 0; },
                   [n, state, func = std::forward<Func>(func)]() mutable { return func(n - (*state)--); });
  } else {
    static_assert(std::is_invocable_v<Func>);
    return DoUntil([n]() mutable { return n-- == 0; }, std::forward<Func>(func));
  }
}

/// @brief Waiting for multiple futures at same time.
///        When all futures were resolved(whatere ready or failed), this function's future
///        will become ready with a tuple of all futures.
/// @return Future<std::tuple<FutsType...>>
template <typename... FutsType>
Future<std::tuple<FutsType...>> WhenAll(FutsType &&... futs) {
  if constexpr (std::tuple_size<std::tuple<FutsType...>>::value == 0) {
    return MakeReadyFuture<std::tuple<>>(std::tuple<>());
  } else {
    auto input = std::make_tuple(std::move(futs)...);

    return DoWith(std::move(input), [](std::tuple<FutsType...> &futs) {
      auto promise_ptr = std::make_shared<Promise<std::tuple<FutsType...>>>();
      auto future = promise_ptr->GetFuture();
      auto counter = std::make_shared<std::atomic_uint32_t>(0);
      for_each(
        futs, WhenAllFunction<std::tuple<FutsType...>>(promise_ptr, futs, counter));
      return future;
    });
  }
}

/// @brief This function is as same as @ref WhenAll(FutsType &&... futs) , but accept
///        two interators to set input futures.
/// @param first First iterator.
/// @param last Last iterator.
/// @return Future<std::vector<FutureType>>
template <typename InputIterator, typename FutureType = typename std::iterator_traits<InputIterator>::value_type>
Future<std::vector<FutureType>> WhenAll(InputIterator first, InputIterator last) {
  std::vector<FutureType> result;
  if (last - first == 0) {
    return MakeReadyFuture<std::vector<FutureType>>(std::move(result));
  } else {
    return DoWith(std::move(result), [last, first](std::vector<FutureType> &result) mutable {
      result.resize(last - first);
      auto promise_ptr = std::make_shared<Promise<std::vector<FutureType>>>();
      auto future = promise_ptr->GetFuture();
      auto counter = std::make_shared<std::atomic_uint32_t>(0);
      uint32_t size = last - first;
      for (auto it = first; it != last; ++it) {
        uint32_t idx = it - first;
        it->Then([counter, idx, &result, size, promise = promise_ptr](FutureType &&fut) mutable {
          result[idx] = std::move(fut);
          std::atomic_thread_fence(std::memory_order_acq_rel);
          auto current = (*counter).fetch_add(1);
          if (current + 1 == size) {
            promise->SetValue(std::move(result));
          }
          return MakeReadyFuture<>();
        });
      }
      return future;
    });
  }
}

/// @brief Returned future turns resulted, when any input future ready or failed.
/// @tparam FutureValueType
/// @param first First iterator.
/// @param last Last iterator.
/// @return Future<size_t, FutureValueType>
template <typename InputIterator,
          typename FutureType = typename std::iterator_traits<InputIterator>::value_type,
          typename FutureValueType = typename FutureType::TupleValueType>
Future<size_t, FutureValueType> WhenAny(InputIterator first, InputIterator last) {
  struct InnerContext {
    Promise<size_t, FutureValueType> promise;
    std::atomic_bool done{false};
  };

  if (last - first == 0) {
    FutureValueType result;
    return MakeReadyFuture<size_t, FutureValueType>(0, std::move(result));
  } else {
    auto context = std::make_shared<InnerContext>();
    auto future = context->promise.GetFuture();
    for (auto it = first; it != last; ++it) {
      uint32_t idx = it - first;
      it->Then([context, idx](FutureType &&fut) mutable {
        auto has_done = context->done.exchange(true);
        if (!has_done) {
          if (fut.IsReady()) {
            context->promise.SetValue(idx, std::move(fut.GetValue()));
          } else {
            context->promise.SetException(fut.GetException());
          }
        }

        return MakeReadyFuture<>();
      });
    }
    return future;
  }
}

/// @brief Returned future turns resulted, when any input future ready or all input futures failed.
/// @param first First iterator.
/// @param last Last iterator.
/// @return Future<size_t, FutureValueType>
template <typename InputIterator,
          typename FutureType = typename std::iterator_traits<InputIterator>::value_type,
          typename FutureValueType = typename FutureType::TupleValueType>
Future<size_t, FutureValueType> WhenAnyWithoutException(InputIterator first, InputIterator last) {
  struct InnerContext {
    explicit InnerContext(size_t n) : total(n) {}
    Promise<size_t, FutureValueType> promise;
    std::atomic_uint32_t counter{0};
    std::atomic_bool done{false};
    size_t total;
  };

  if (last - first == 0) {
    FutureValueType result;
    return MakeReadyFuture<size_t, FutureValueType>(0, std::move(result));
  } else {
    size_t size = last - first;
    auto context = std::make_shared<InnerContext>(size);
    auto future = context->promise.GetFuture();
    for (auto it = first; it != last; ++it) {
      uint32_t idx = it - first;
      it->Then([context, idx](FutureType &&fut) mutable {
        if (fut.IsReady()) {
          auto has_done = context->done.exchange(true);
          if (!has_done) {
            context->promise.SetValue(idx, std::move(fut.GetValue()));
          }
        } else {
          auto current = context->counter.fetch_add(1);
          if (current + 1 == context->total) {
            context->promise.SetException(fut.GetException());
          }
        }

        return MakeReadyFuture<>();
      });
    }
    return future;
  }
}

}  // namespace trpc
