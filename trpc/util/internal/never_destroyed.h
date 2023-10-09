//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/never_destroyed.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <new>
#include <type_traits>
#include <utility>

namespace trpc {

namespace detail {

/// @brief Resident class implements, never released during process running
template <class T>
class NeverDestroyedImpl {
 public:
  // Noncopyable / nonmovable.
  NeverDestroyedImpl(const NeverDestroyedImpl&) = delete;
  NeverDestroyedImpl& operator=(const NeverDestroyedImpl&) = delete;

  /// @brief Get a pointer to a template type of `T`.
  /// @return the pointer of `T`
  T* Get() noexcept { return reinterpret_cast<T*>(&storage_); }

  /// @brief Get a `const` pointer to a template type of `T`.
  /// @return the pointer of `T`
  const T* Get() const noexcept { return reinterpret_cast<const T*>(&storage_); }

  /// @brief Get a reference to the object.
  /// @return the reference of `T`
  T& GetReference() noexcept { return *reinterpret_cast<T*>(&storage_); }

  /// @brief Get a const reference to the object.
  /// @return the reference of `T`
  const T& GetReference() const noexcept { return *reinterpret_cast<const T*>(&storage_); }

  /// @brief Pointer operator.
  /// @return the pointer of `T`
  T* operator->() noexcept { return Get(); }

  /// @brief Pointer operator.
  /// @return the const pointer of `T`
  const T* operator->() const noexcept { return Get(); }

  /// @brief Dereference operator.
  /// @return the reference of `T`
  T& operator*() noexcept { return *Get(); }

  /// @brief Dereference operator.
  /// @return the const reference of `T`
  const T& operator*() const noexcept { return *Get(); }

 protected:
  NeverDestroyedImpl() = default;

 protected:
  std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
};

}  // namespace detail

namespace internal {

/// @brief `NeverDestroyed<T>` helps you create objects that are never destroyed (without incuring heap memory
///         allocation.).
/// @note In certain cases (e.g., singleton), not destroying object can save you from
///       dealing with destruction order issues.
/// @note Caveats:
///       - Be caution when declaring `NeverDestroyed<T>` as `thread_local`, this may cause memory leak.
///       - To construct `NeverDestroyed<T>`, you might have to declare this class as
///         your friend (if the constructor being called is not publicly accessible).
///       - By declaring `NeverDestroyed<T>` as your friend, it's impossible to
///         guarantee `T` is used as a singleton as now anybody can construct a new
///         `NeverDestroyed<T>`. You can use `NeverDestroyedSingleton<T>` in this case.
/// Usage:
/// @code{.cpp}
///   void CreateWorld() {
///      static NeverDestroyed<std::mutex> lock;  // Destructor won't be called.
///      std::scoped_lock _(*lock);
///       // ...
///    }
/// @endcode
template <class T>
class NeverDestroyed final : private detail::NeverDestroyedImpl<T> {
  using Impl = detail::NeverDestroyedImpl<T>;

 public:
  template <class... Ts>
  explicit NeverDestroyed(Ts&&... args) {
    new (&this->storage_) T(std::forward<Ts>(args)...);
  }

  using Impl::Get;
  using Impl::GetReference;
  using Impl::operator->;
  using Impl::operator*;
};

/// @brief Same as `NeverDestroyed`, except that it's constructor is only accessible to
///        `T`. This class is useful when `T` is intended to be used as singleton.
template <class T>
class NeverDestroyedSingleton final : private trpc::detail::NeverDestroyedImpl<T> {
  using Impl = detail::NeverDestroyedImpl<T>;

 public:
  using Impl::Get;
  using Impl::GetReference;
  using Impl::operator->;
  using Impl::operator*;

 private:
  friend T;

  template <class... Ts>
  explicit NeverDestroyedSingleton(Ts&&... args) {
    new (&this->storage_) T(std::forward<Ts>(args)...);
  }
};

}  // namespace internal

}  // namespace trpc
