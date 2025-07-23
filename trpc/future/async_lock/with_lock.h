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

#include <utility>

#include "trpc/future/future.h"

namespace trpc {

class LockException : public ExceptionBase {
 public:
  explicit LockException(const char* msg, int ret = ExceptionCode::LOCK_EXCEPT) {
    ret_code_ = ret;
    msg_ = msg;
  }

  const char* what() const noexcept override { return msg_.c_str(); }

  enum ExceptionCode {
    LOCK_EXCEPT = 1,
  };
};

template <typename... T> struct IsFuture : std::false_type {};
template <typename... T> struct IsFuture<Future<T...>> : std::true_type {};

template <typename T>
struct FuturizeBase {
  using Type = Future<T>;

  static inline Type Convert(T&& value) {
    return MakeReadyFuture<T>(std::move(value));
  }

  static inline Type Convert(Type&& value) {
    return std::move(value);
  }
};

template<>
struct FuturizeBase<void> {
  using Type = Future<>;

  static inline Type Convert(Type&& value) {
    return std::move(value);
  }
};

template <typename T>
struct FuturizeBase<Future<T>> : public FuturizeBase<T> {};

template <>
struct FuturizeBase<Future<>> : public FuturizeBase<void> {};

template <typename T>
struct Futurize : public FuturizeBase<T> {
  using Base = FuturizeBase<T>;
  using Type = typename Base::Type;
  using Base::Convert;

  template <typename Func, typename... FuncArgs>
  static inline Type Invoke(Func&& func, FuncArgs&&... args) noexcept {
    try {
      using ret_t = decltype(func(std::forward<FuncArgs>(args)...));
      if constexpr (std::is_void_v<ret_t>) {
        // When func returns void.
        func(std::forward<FuncArgs>(args)...);
        // Return Future<>
        return MakeReadyFuture<>();
      } else if constexpr (IsFuture<ret_t>::value) {
        // When func returns Future<T>.
        return func(std::forward<FuncArgs>(args)...);
      } else {
        // When func returns T, then return Future<T>.
        return Convert(func(std::forward<FuncArgs>(args)...));
      }
    } catch (...) {
      // Exception thrown in func, then return Future with exception.
      return Type(LockException("WithLock invoke err", LockException::LOCK_EXCEPT));
    }
  }
};

template<typename Func, typename... Args>
auto FuturizeInvoke(Func&& func, Args&&... args) noexcept {
  return Futurize<std::invoke_result_t<Func, Args&&...>>::Invoke(
          std::forward<Func>(func), std::forward<Args>(args)...);
}

/// @brief Execute func with lock, and unlock when it's done.
/// @param lock: Reference to a Read/Write lock object, who offers Lock()/UnLock() syntax.
/// @param func: the lambda of user's work.
/// @return Return whatever func returns, if func's return type is not Future<>,
//          WithLock will wrap it up, e.g. int -> Future<int>.
template<typename Lock, typename Func>
inline auto WithLock(Lock* lock, Func&& func) {
  return lock->Lock().Then([lock, func = std::forward<Func>(func)] () mutable {
    auto ret = FuturizeInvoke(func);
    using futurized_type = decltype(ret);
    return ret.Then([lock] (futurized_type&& ret) {
      lock->UnLock();
      return std::move(ret);
    });
  });
}

}  // namespace trpc
