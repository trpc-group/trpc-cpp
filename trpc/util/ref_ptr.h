//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/ref_ptr.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>

#include "trpc/util/likely.h"

namespace trpc {

// @sa: Constructor of `RefPtr`.
constexpr struct ref_ptr_t {
  explicit ref_ptr_t() = default;
} ref_ptr;
constexpr struct adopt_ptr_t {
  explicit adopt_ptr_t() = default;
} adopt_ptr;

// You need to specialize this traits unless your type is inherited from
// `RefCounted<T>`.
template <class T, class = void>
struct RefTraits {
  // Increment reference counter on `ptr` with `std::memory_order_relaxed`.
  static void Reference(T* ptr) noexcept {
    static_assert(sizeof(T) == 0,
                  "To use `RefPtr<T>`, you need either inherit from `RefCounted<T>` "
                  "(which you didn't), or specialize `RefTraits<T>`.");
  }

  // Decrement reference counter on `ptr` with `std::memory_order_acq_rel`.
  //
  // If the counter after decrement reaches zero, resource allocated for `ptr`
  // should be released.
  static void Dereference(T* ptr) noexcept {
    static_assert(sizeof(T) == 0,
                  "To use `RefPtr<T>`, you need either inherit from `RefCounted<T>` "
                  "(which you didn't), or specialize `RefTraits<T>`.");
  }
};

// For types inherited from `RefCounted`, they're supported by `RefPtr` (or
// `RefTraits<T>`, to be accurate) by default.
//
// Default constructed one has a reference count of one (every object whose
// reference count reaches zero should have already been destroyed). Should you
// want to construct a `RefPtr`, use overload that accepts `adopt_ptr`.
//
// IMPLEMENTATION DETAIL: For the sake of implementation simplicity, at the
// moment `Deleter` is declared as a friend to `RefCounted<T>`. This declaration
// is subject to change.
template <class T, class Deleter = std::default_delete<T>>
class RefCounted {
 public:
  // Increment ref-count.
  constexpr void Ref() noexcept;

  // Decrement ref-count, if it reaches zero after the decrement, the pointer is
  // freed by `Deleter`.
  constexpr void Deref() noexcept;

  // Get current ref-count.
  //
  // It's unsafe, as by the time the ref-count is returned, it may well have
  // changed. You cannot rely on it for accurate ref-count even if it
  // returns 1, because the implementation loads with memory_order_relaxed.
  // This method is similiar to `std::shared_ptr<T>::use_count`.
  constexpr std::uint32_t UnsafeRefCount() const noexcept;

 protected:
  // `RefCounted` must be inherited.
  RefCounted() = default;

  // Destructor is NOT defined as virtual on purpose, we always cast `this` to
  // `T*` before calling `delete`.

 private:
  friend Deleter;

  // Hopefully `std::uint32_t` is large enough to store a ref count.
  //
  // We tried using `std::uint_fast32_t` here, it uses 8 bytes. I'm not aware of
  // the benefit of using an 8-byte integer here (at least for the purpose of
  // reference-couting.)
  std::atomic<std::uint32_t> ref_count_{1};
};

// Keep the overhead the same as an atomic `std::uint3232_t`.
static_assert(sizeof(RefCounted<int>) == sizeof(std::atomic<std::uint32_t>));

// Utilities for internal use.
/// @private
namespace detail {

// This methods serves multiple purposes:
//
// - If `T` is explicitly specified, it tests if `T` inherits from
//   `RefCounted<T, ...>`, and returns `RefCounted<T, ...>`;
//
// - If `T` is not specified, it tests if `T` inherits from some `RefCounted<U,
//   ...>`, and returns `RefCounted<U, ...>`;
//
// - Otherwise `int` is returned, which can never inherits from `RefCounted<int,
//   ...>`.
template <class T, class... Us>
RefCounted<T, Us...> GetRefCountedType(const RefCounted<T, Us...>*);
template <class T>
int GetRefCountedType(...);
int GetRefCountedType(...);  // Match failure.

// Extract `T` from `RefCounted<T, ...>`.
//
// We're returning `std::common_type<T>` instead of `T` so as not to require `T`
// to be complete.
template <class T, class... Us>
std::common_type<T> GetRefeeType(const RefCounted<T, Us...>*);
std::common_type<int> GetRefeeType(...);

template <class T>
using refee_t = typename decltype(GetRefeeType(reinterpret_cast<const T*>(0)))::type;

// If `T` inherits from `RefCounted<T, ...>` directly, `T` is returned,
// otherwise `int` (which can never inherits `RefCounted<int, ...>`) is
// returned.
template <class T>
using direct_refee_t = refee_t<decltype(GetRefCountedType<T>(reinterpret_cast<const T*>(0)))>;

// Test if `T` is a subclass of some `RefCounted<T, ...>`.
template <class T>
constexpr auto is_ref_counted_directly_v = !std::is_same_v<int, direct_refee_t<T>>;

// If `T` is inheriting from some `RefCounted<U, ...>` unambiguously, this
// method returns `U`. Otherwise it returns `int`.
template <class T>
using indirect_refee_t = refee_t<decltype(GetRefCountedType(reinterpret_cast<const T*>(0)))>;

// Test if:
//
// - `T` is inherited from `RefCounted<U, ...>` indirectly;  (1)
// - `U` is inherited from `RefCounted<U, ...>` directly;    (2)
// - and `U` has a virtual destructor.                       (3)
//
// In this case, we can safely use `RefTraits<RefCounted<U, ...>>` on
// `RefCounted<T>`.
template <class T>
constexpr auto is_ref_counted_indirectly_safe_v = !is_ref_counted_directly_v<T> &&                     // 1
                                                  is_ref_counted_directly_v<indirect_refee_t<T>> &&    // 2
                                                  std::has_virtual_destructor_v<indirect_refee_t<T>>;  // 3

// Test if `RefTraits<as_ref_counted_t<T>>` is safe for `T`.
//
// We consider default traits as safe if either:
//
// - `T` is directly inheriting `RefCounted<T, ...>` (so its destructor does not
//   have to be `virtual`), or
// - `T` inherts from `RefCounted<U, ...>` and `U`'s destructor is virtual.
template <class T>
constexpr auto is_default_ref_traits_safe_v =
    detail::is_ref_counted_directly_v<T> || detail::is_ref_counted_indirectly_safe_v<T>;

template <class T>
using as_ref_counted_t = decltype(GetRefCountedType(reinterpret_cast<const T*>(0)));

}  // namespace detail

// Specialization for `RefCounted<T>` ...
template <class T, class Deleter>
struct RefTraits<RefCounted<T, Deleter>> {
  static void Reference(RefCounted<T, Deleter>* ptr) noexcept { ptr->Ref(); }
  static void Dereference(RefCounted<T, Deleter>* ptr) noexcept { ptr->Deref(); }
};

// ... and its subclasses.
template <class T>
struct RefTraits<
    T, std::enable_if_t<!std::is_same_v<detail::as_ref_counted_t<T>, T> && detail::is_default_ref_traits_safe_v<T>>>
    : RefTraits<detail::as_ref_counted_t<T>> {};

// Our own design of `retain_ptr`, with several naming changes to make it easier
// to understand.
//
// @sa: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0468r1.html
template <class T>
class RefPtr {
  using Traits = RefTraits<T>;

 public:
  using element_type = T;

  // Default constructed one does not own a pointer.
  constexpr RefPtr() noexcept : ptr_(nullptr) {}

  // Same as default-constructed one.
  /* implicit */ constexpr RefPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

  // Reference counter is decrement on destruction.
  ~RefPtr() {
    if (ptr_) {
      Traits::Dereference(ptr_);
    }
  }

  // Increment reference counter on `ptr` and hold it.
  //
  // P0468 declares `xxx_t` as the second argument, but I'd still prefer to
  // place it as the first one. `std::scoped_lock` revealed the shortcoming of
  // placing `std::adopt_lock_t` as the last parameter, I won't repeat that
  // error here.
  constexpr RefPtr(ref_ptr_t, T* ptr) noexcept;

  // Hold `ptr` without increasing its reference counter.
  constexpr RefPtr(adopt_ptr_t, T* ptr) noexcept : ptr_(ptr) {}

  // TBH this is a dangerous conversion constructor. Even if `T` does not have
  // virtual destructor.
  //
  // However, testing if `T` has a virtual destructor comes with a price: we'll
  // require `T` to be complete when defining `RefPtr<T>`, which is rather
  // annoying.
  //
  // Given that `std::unique_ptr` does not test destructor's virtualization
  // state either, we ignore it for now.
  template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
  constexpr RefPtr(RefPtr<U> ptr) noexcept : ptr_(ptr.Leak()) {}

  // Copyable, movable.
  constexpr RefPtr(const RefPtr& ptr) noexcept;
  constexpr RefPtr(RefPtr&& ptr) noexcept;
  constexpr RefPtr& operator=(const RefPtr& ptr) noexcept;
  constexpr RefPtr& operator=(RefPtr&& ptr) noexcept;

  // `boost::intrusive_ptr(T*)` increments reference counter, while
  // `std::ref_ptr(T*)` (as specified by the proposal) does not. The
  // inconsistency between them can be confusing. To be perfect clear, we do not
  // support `RefPtr(T*)`.
  //
  // RefPtr(T*) = delete;

  // Accessors.
  constexpr T* operator->() const noexcept { return Get(); }
  constexpr T& operator*() const noexcept { return *Get(); }
  constexpr T* Get() const noexcept { return ptr_; }
  // Compatible with the usage of std::shared_ptr.
  constexpr T* get() const noexcept { return Get(); }

  // Test if *this holds a pointer.
  constexpr explicit operator bool() const noexcept { return ptr_; }
  bool operator!=(std::nullptr_t) const noexcept { return ptr_ != nullptr; }
  bool operator==(std::nullptr_t) const noexcept { return ptr_ == nullptr; }

  // Equivalent to `Reset()`.
  constexpr RefPtr& operator=(std::nullptr_t) noexcept;

  // Reset *this to an empty one.
  constexpr void Reset() noexcept;

  // Release whatever *this currently holds and hold `ptr` instead.
  constexpr void Reset(ref_ptr_t, T* ptr) noexcept;
  constexpr void Reset(adopt_ptr_t, T* ptr) noexcept;

  // Gives up ownership on its internal pointer, which is returned.
  //
  // The caller is responsible for calling `RefTraits<T>::Dereference` when it
  // sees fit.
  [[nodiscard]] constexpr T* Leak() noexcept { return std::exchange(ptr_, nullptr); }

 private:
  T* ptr_ = nullptr;
};

// Shorthand for `new`-ing an object and **adopt** (NOT *ref*, i.e., the
// initial ref-count must be 1) it into a `RefPtr`.
template <class T, class... Us>
RefPtr<T> MakeRefCounted(Us&&... args) {
  return RefPtr(adopt_ptr, new T(std::forward<Us>(args)...));
}

template <class T, class Deleter>
constexpr void RefCounted<T, Deleter>::Ref() noexcept {
  ref_count_.fetch_add(1, std::memory_order_relaxed);
}

template <class T, class Deleter>
constexpr void RefCounted<T, Deleter>::Deref() noexcept {
  // It seems that we can simply test if `ref_count_` is 1, and save an atomic
  // operation if it is (as we're the only reference holder). However I don't
  // see a perf. boost in implementing it, so I keep it unchanged. we might want
  // to take a deeper look later.
  if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    Deleter()(static_cast<T*>(this));
  }
}

template <class T, class Deleter>
constexpr std::uint32_t RefCounted<T, Deleter>::UnsafeRefCount() const noexcept {
  return ref_count_.load(std::memory_order_relaxed);
}

template <class T>
constexpr RefPtr<T>::RefPtr(ref_ptr_t, T* ptr) noexcept : ptr_(ptr) {
  if (ptr) {
    Traits::Reference(ptr_);
  }
}

template <class T>
constexpr RefPtr<T>::RefPtr(const RefPtr& ptr) noexcept : ptr_(ptr.ptr_) {
  if (ptr_) {
    Traits::Reference(ptr_);
  }
}

template <class T>
constexpr RefPtr<T>::RefPtr(RefPtr&& ptr) noexcept : ptr_(ptr.ptr_) {
  ptr.ptr_ = nullptr;
}

template <class T>
constexpr RefPtr<T>& RefPtr<T>::operator=(const RefPtr& ptr) noexcept {
  if (TRPC_UNLIKELY(&ptr == this)) {
    return *this;
  }
  if (ptr_) {
    Traits::Dereference(ptr_);
  }
  ptr_ = ptr.ptr_;
  if (ptr_) {
    Traits::Reference(ptr_);
  }
  return *this;
}

template <class T>
constexpr RefPtr<T>& RefPtr<T>::operator=(RefPtr&& ptr) noexcept {
  if (TRPC_UNLIKELY(&ptr == this)) {
    return *this;
  }
  if (ptr_) {
    Traits::Dereference(ptr_);
  }
  ptr_ = ptr.ptr_;
  ptr.ptr_ = nullptr;
  return *this;
}

template <class T>
constexpr RefPtr<T>& RefPtr<T>::operator=(std::nullptr_t) noexcept {
  Reset();
  return *this;
}

template <class T>
constexpr void RefPtr<T>::Reset() noexcept {
  if (ptr_) {
    Traits::Dereference(ptr_);
    ptr_ = nullptr;
  }
}

template <class T>
constexpr void RefPtr<T>::Reset(ref_ptr_t, T* ptr) noexcept {
  Reset();
  ptr_ = ptr;
  Traits::Reference(ptr_);
}

template <class T>
constexpr void RefPtr<T>::Reset(adopt_ptr_t, T* ptr) noexcept {
  Reset();
  ptr_ = ptr;
}

// refptr pointer cast operators like shared_ptr
template <typename _Tp, typename _Up>
inline RefPtr<_Tp> static_pointer_cast(const RefPtr<_Up>& __r) noexcept {
  using _Sp = RefPtr<_Tp>;
  return _Sp(ref_ptr, static_cast<typename _Sp::element_type*>(__r.get()));
}

template <typename _Tp, typename _Up>
inline RefPtr<_Tp> const_pointer_cast(const RefPtr<_Up>& __r) noexcept {
  using _Sp = RefPtr<_Tp>;
  return _Sp(ref_ptr, const_cast<typename _Sp::element_type*>(__r.get()));
}

template <typename _Tp, typename _Up>
inline RefPtr<_Tp> dynamic_pointer_cast(const RefPtr<_Up>& __r) noexcept {
  using _Sp = RefPtr<_Tp>;
  if (auto* __p = dynamic_cast<typename _Sp::element_type*>(__r.get())) {
    return _Sp(ref_ptr, __p);
  }
  return _Sp();
}

}  // namespace trpc
