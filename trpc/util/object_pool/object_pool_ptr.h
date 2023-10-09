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

#include <atomic>
#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

#include "trpc/util/object_pool/object_pool.h"

namespace trpc::object_pool {

// We provide four smart pointer wrappers for allocating/releasing objects from the object pool, similar to
// std::unique_ptr and std::shared_ptr.
// They are LwUniquePtr, LwSharedPtr, UniquePtr, and SharedPtr. LwUniquePtr and LwSharedPtr do not support polymorphism
// and have high performance. UniquePtr and SharedPtr implement polymorphism using std::function and have some overhead.
// If there is no polymorphic scenario, please use LwUniquePtr and LwSharedPtr first. LwSharedPtr and SharedPtr are
// intrusive and require inheriting their respective base classes.
// For more details, please refer to the unit tests.

/// @brief LwUniquePtr is a nonshared smart pointer that does not support polymorphism.
///        Usage:
///        struct A {
///          int a;
///          std::string s;
///        };
///
///        template <>
///        struct ObjectPoolTraits<A> {
///          static constexpr auto kType = ObjectPoolType::kGlobal;
///        };
///
///        trpc::object_pool::LwUniquePtr<A> ptr1 = trpc::object_pool::MakeLwUnique<A>();
///
template <class T>
class LwUniquePtr {
 public:
  LwUniquePtr() noexcept = default;

  /* implicit */ constexpr LwUniquePtr(std::nullptr_t) noexcept  // NOLINT
      : LwUniquePtr() {}

  ~LwUniquePtr() noexcept { Reset(); }

  LwUniquePtr(LwUniquePtr&& ptr) noexcept
      : ptr_(ptr.ptr_) {
    ptr.ptr_ = nullptr;
  }

  LwUniquePtr& operator=(LwUniquePtr&& ptr) noexcept {
    if (this != &ptr) {
      Reset(ptr.Leak());
    }
    return *this;
  }

  explicit operator bool() const noexcept {
    return !!ptr_;
  }

  T* Get() const noexcept {
    return ptr_;
  }

  T* operator->() const noexcept {
    return ptr_;
  }

  T& operator*() const noexcept {
    return *ptr_;
  }

  LwUniquePtr& operator=(std::nullptr_t) noexcept {
    Reset(nullptr);
    return *this;
  }

  // `ptr` must be obtained from calling `Leak()` on another `LwUniquePtr`.
  void Reset(T* ptr = nullptr) noexcept {
    if (ptr_) {
      Delete<T>(ptr_);
    }
    ptr_ = ptr;
  }

  // Ownership is transferred to caller.
  [[nodiscard]] constexpr T* Leak() noexcept {
    return std::exchange(ptr_, nullptr);
  }

  template <typename U, typename... A>
  friend LwUniquePtr<U> MakeLwUnique(A&&... a);

 private:
  explicit LwUniquePtr(T* ptr) noexcept : ptr_(ptr) {}

 private:
  T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator!=(const LwUniquePtr<T>& x, const LwUniquePtr<U>& y) {
  return x.Get() != y.Get();
}

template <typename T>
inline bool operator!=(const LwUniquePtr<T>& x, std::nullptr_t) {
  return x.Get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, const LwUniquePtr<T>& y) {
  return nullptr != y.Get();
}

template <typename T, typename... A>
inline LwUniquePtr<T> MakeLwUnique(A&&... a) {
  return LwUniquePtr<T>(New<T>(std::forward<A>(a)...));
}

class LwSharedPtrCountBase {
 public:
  ~LwSharedPtrCountBase()  = default;

  std::atomic<uint32_t> count = 0;
};

template <class T>
class LwSharedPtr;

template <typename U, bool esft>
class LwSharedPtrMakeHelper;

template <typename T>
class EnableLwSharedFromThis : public LwSharedPtrCountBase {
 public:
  LwSharedPtr<T> SharedFromThis();

  LwSharedPtr<const T> SharedFromThis() const;

  [[nodiscard]] uint32_t UseCount() const noexcept { return count.load(std::memory_order_acquire); }

  /// @brief Increase/Decrease refcount
  constexpr void IncrCount() noexcept { count.fetch_add(1, std::memory_order_acq_rel); }
  constexpr void DecrCount() noexcept {
    if (count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      Delete<T>(static_cast<T*>(this));
    }
  }

  template <typename U>
  friend class LwSharedPtr;

  template <typename U, bool esft>
  friend class LwSharedPtrMakeHelper;
};

constexpr struct lw_shared_ref_ptr_t { explicit lw_shared_ref_ptr_t() = default; } lw_shared_ref_ptr;
constexpr struct lw_shared_adopt_ptr_t { explicit lw_shared_adopt_ptr_t() = default; } lw_shared_adopt_ptr;

/// @brief LwSharedPtr is an intrusive shared smart pointer that does not support polymorphism.
///        Usage:
///        struct A : public object_pool::LwSharedPtrCountBase {
///          int a;
///          std::string s;
///        };
///
///        template <>
///        struct ObjectPoolTraits<A> {
///          static constexpr auto kType = ObjectPoolType::kGlobal;
///        };
///
///        trpc::object_pool::LwSharedPtr<A> ptr1 = trpc::object_pool::MakeLwShared<A>();
///
///        For more usage, see object_pool_ptr_test.cc
///
template <class T>
class LwSharedPtr {
 public:
  LwSharedPtr() noexcept = default;

  /* implicit */ constexpr LwSharedPtr(std::nullptr_t) noexcept  // NOLINT
      : LwSharedPtr() {}

  constexpr LwSharedPtr(lw_shared_ref_ptr_t, T* ptr) noexcept : base_(ptr), ptr_(ptr) {
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  // Hold `ptr` without increasing its reference counter.
  constexpr LwSharedPtr(lw_shared_adopt_ptr_t, T* ptr) noexcept : base_(ptr), ptr_(ptr) {}

  LwSharedPtr(const LwSharedPtr& x) noexcept
      : base_(x.base_),
        ptr_(x.ptr_) {
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  LwSharedPtr(LwSharedPtr&& x) noexcept
      : base_(x.base_),
        ptr_(x.ptr_) {
    x.base_ = nullptr;
    x.ptr_ = nullptr;
  }

  ~LwSharedPtr() {
    Reset();
  }

  void Reset() {
    if (base_ && base_->count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      Delete<T>(ptr_);
    }
    ptr_ = nullptr;
    base_ = nullptr;
  }

  constexpr void Reset(lw_shared_ref_ptr_t, T* ptr) noexcept {
    Reset();
    ptr_ = ptr;
    base_ = ptr;
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  constexpr void Reset(lw_shared_adopt_ptr_t, T* ptr) noexcept {
    Reset();
    ptr_ = ptr;
    base_ = ptr;
  }

  LwSharedPtr& operator=(const LwSharedPtr& x) noexcept {
    if (this != &x) {
      this->~LwSharedPtr();
      new (this) LwSharedPtr(x);
    }
    return *this;
  }

  LwSharedPtr& operator=(LwSharedPtr&& x) noexcept {
    if (this != &x) {
      this->~LwSharedPtr();
      new (this) LwSharedPtr(std::move(x));
    }
    return *this;
  }

  LwSharedPtr& operator=(std::nullptr_t) noexcept {
    return *this = LwSharedPtr();
  }

  explicit operator bool() const noexcept {
    return !!ptr_;
  }

  T& operator*() const noexcept {
    return *ptr_;
  }

  T* operator->() const noexcept {
    return ptr_;
  }

  T* Get() const noexcept {
    return ptr_;
  }

  // Ownership is transferred to caller.
  [[nodiscard]] constexpr T* Leak() noexcept {
    base_ = nullptr;
    return std::exchange(ptr_, nullptr);
  }

  uint32_t UseCount() const noexcept {
    if (base_) {
      return base_->count.load(std::memory_order_acquire);
    } else {
      return 0;
    }
  }

  template <typename U, typename... A>
  friend LwSharedPtr<U> MakeLwShared(A&&... a);

  template <typename U, bool esft>
  friend class LwSharedPtrMakeHelper;

  template <typename U>
  friend class EnableLwSharedFromThis;

  template <typename V, typename U>
  friend LwSharedPtr<V> StaticPointerCast(const LwSharedPtr<U>& p);

 private:
  LwSharedPtr(LwSharedPtrCountBase* base, T* ptr) noexcept
      : base_(base), ptr_(ptr) {
    if (base_ && ptr_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  explicit LwSharedPtr(EnableLwSharedFromThis<std::remove_cv_t<T>>* ptr) noexcept
      : base_(ptr), ptr_(static_cast<T*>(ptr)) {
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

 private:
  mutable LwSharedPtrCountBase* base_ = nullptr;
  mutable T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator!=(const LwSharedPtr<T>& x, const LwSharedPtr<U>& y) {
  return x.Get() != y.Get();
}

template <typename T>
inline bool operator!=(const LwSharedPtr<T>& x, std::nullptr_t) {
  return x.Get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, const LwSharedPtr<T>& y) {
  return nullptr != y.Get();
}

template <typename T>
class LwSharedPtrMakeHelper<T, true> {
 public:
  template <typename... A>
  static LwSharedPtr<T> Make(A&&... a) {
    auto p = New<T>(std::forward<A>(a)...);
    return LwSharedPtr<T>(p, p);
  }
};

template <typename T, typename... A>
inline LwSharedPtr<T> MakeLwShared(A&&... a) {
  using helper = LwSharedPtrMakeHelper<T, std::is_base_of<LwSharedPtrCountBase, T>::value>;
  return helper::Make(std::forward<A>(a)...);
}

template <typename T>
inline LwSharedPtr<T> EnableLwSharedFromThis<T>::SharedFromThis() {
  auto unconst = reinterpret_cast<EnableLwSharedFromThis<std::remove_cv_t<T>>*>(this);
  return LwSharedPtr<T>(unconst);
}

template <typename T>
inline LwSharedPtr<const T> EnableLwSharedFromThis<T>::SharedFromThis() const {
  auto esft = const_cast<EnableLwSharedFromThis*>(this);
  auto unconst = reinterpret_cast<EnableLwSharedFromThis<std::remove_cv_t<T>>*>(esft);
  return LwSharedPtr<const T>(unconst);
}

template <typename T, typename U>
inline LwSharedPtr<T> StaticPointerCast(const LwSharedPtr<U>& p) {
  return LwSharedPtr<T>(p.base_, static_cast<T*>(p.ptr_));
}

/// @brief UniquePtr is an nonshared smart pointer that support polymorphism.
///        Usage:
///        struct A {
///          int a;
///          std::string s;
///        };
///
///        template <>
///        struct ObjectPoolTraits<A> {
///          static constexpr auto kType = ObjectPoolType::kGlobal;
///        };
///
///        trpc::object_pool::UniquePtr<A> ptr1 = trpc::object_pool::MakeUnique<A>();
///
template <class T>
class UniquePtr {
 public:
  using DeleteFunction = std::function<void(void*)>;

 public:
  UniquePtr() noexcept = default;

  /* implicit */ constexpr UniquePtr(std::nullptr_t) noexcept  // NOLINT
      : UniquePtr() {}

  ~UniquePtr() noexcept { Reset(); }

  template <class E>
  UniquePtr(UniquePtr<E>&& ptr) noexcept
      : ptr_(ptr.Leak()), deleter_(std::move(ptr.deleter_)) {
  }

  template <class E>
  UniquePtr& operator=(UniquePtr<E>&& ptr) noexcept {
    if (this != static_cast<void*>(const_cast<std::remove_cv_t<UniquePtr<E>>*>(&ptr))) {
      ReturnToPool();
      ptr_ = static_cast<T*>(const_cast<std::remove_cv_t<E>*>(ptr.Leak()));
      deleter_ = std::move(ptr.deleter_);
    }
    return *this;
  }

  explicit operator bool() const noexcept {
    return !!ptr_;
  }

  T* Get() const noexcept {
    return ptr_;
  }

  T* operator->() const noexcept {
    return ptr_;
  }

  T& operator*() const noexcept {
    return *ptr_;
  }

  UniquePtr& operator=(std::nullptr_t) noexcept {
    Reset(nullptr);
    return *this;
  }

  void Reset() {
    ReturnToPool();
    ptr_ = nullptr;
  }

  void Reset(std::nullptr_t) {
    Reset();
  }

  // `ptr` must be obtained from calling `Leak()` on another `UniquePtr`.
  template <class E>
  void Reset(E* ptr) noexcept {
    if (!ptr) {
      return Reset();
    }
    ReturnToPool();
    ptr_ = static_cast<T*>(const_cast<std::remove_cv_t<E>*>(ptr));
    deleter_ = [ptr](void* p) { Delete(static_cast<std::remove_pointer_t<decltype(ptr)>*>(p)); };
  }

  // Ownership is transferred to caller.
  [[nodiscard]] constexpr T* Leak() noexcept {
    return std::exchange(ptr_, nullptr);
  }

  template <typename U, typename... A>
  friend UniquePtr<U> MakeUnique(A&&... a);

  template <typename U>
  friend class UniquePtr;

 private:
  explicit UniquePtr(T* ptr) noexcept
      : ptr_(ptr), deleter_([ptr](void* p) {
    Delete(static_cast<std::remove_pointer_t<decltype(ptr)>*>(p));
  }) {}

  inline void ReturnToPool() {
    if (ptr_) {
      assert(deleter_);
      deleter_(const_cast<std::remove_cv_t<T>*>(ptr_));
    }
  }

 private:
  T* ptr_ = nullptr;
  DeleteFunction deleter_;
};

template <typename T, typename U>
inline bool operator!=(const UniquePtr<T>& x, const UniquePtr<U>& y) {
  return x.Get() != y.Get();
}

template <typename T>
inline bool operator!=(const UniquePtr<T>& x, std::nullptr_t) {
  return x.Get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, const UniquePtr<T>& y) {
  return nullptr != y.Get();
}

template <typename T, typename... A>
inline UniquePtr<T> MakeUnique(A&&... a) {
  return UniquePtr<T>(New<T>(std::forward<A>(a)...));
}

class SharedPtrCountBase {
 public:
  using DeleteFunction = std::function<void(void*)>;

 public:
  virtual ~SharedPtrCountBase() = default;

  std::atomic<uint32_t> count = 0;

  DeleteFunction deleter;
};

template <class T>
class SharedPtr;

template <typename U, bool esft>
class SharedPtrMakeHelper;

template <typename T>
class EnableSharedFromThis : public SharedPtrCountBase {
 public:
  SharedPtr<T> SharedFromThis();

  SharedPtr<const T> SharedFromThis() const;

  [[nodiscard]] uint32_t UseCount() const noexcept { return count.load(std::memory_order_acquire); }

  constexpr void IncrCount() noexcept { count.fetch_add(1, std::memory_order_acq_rel); }
  constexpr void DecrCount() noexcept {
    if (count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      deleter(const_cast<std::remove_cv_t<T>*>(reinterpret_cast<T*>(this)));
    }
  }

  template <typename U>
  friend class SharedPtr;

  template <typename U, bool esft>
  friend class SharedPtrMakeHelper;
};

constexpr struct shared_ref_ptr_t { explicit shared_ref_ptr_t() = default; } shared_ref_ptr;
constexpr struct shared_adopt_ptr_t { explicit shared_adopt_ptr_t() = default; } shared_adopt_ptr;


/// @brief SharedPtr is an intrusive shared smart pointer that support polymorphism.
///        Usage:
///        struct A : public object_pool::SharedPtrCountBase {
///          int a;
///          std::string s;
///        };
///
///        template <>
///        struct ObjectPoolTraits<A> {
///          static constexpr auto kType = ObjectPoolType::kGlobal;
///        };
///
///        trpc::object_pool::SharedPtr<A> ptr1 = trpc::object_pool::MakeShared<A>();
///
///        For more usage, see object_pool_ptr_test.cc
///
template <class T>
class SharedPtr {
 public:
  SharedPtr() noexcept = default;

  /* implicit */ constexpr SharedPtr(std::nullptr_t) noexcept  // NOLINT
      : SharedPtr() {}

  constexpr SharedPtr(shared_ref_ptr_t, T* ptr) noexcept : base_(ptr), ptr_(ptr) {
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  // Hold `ptr` without increasing its reference counter.
  constexpr SharedPtr(shared_adopt_ptr_t, T* ptr) noexcept : base_(ptr), ptr_(ptr) {}

  SharedPtr(const SharedPtr& x) noexcept
      : base_(x.base_),
        ptr_(x.ptr_) {
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  SharedPtr(SharedPtr&& x) noexcept
      : base_(x.base_),
        ptr_(x.ptr_) {
    x.base_ = nullptr;
    x.ptr_ = nullptr;
  }

  template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
  SharedPtr(const SharedPtr<U>& x) noexcept
      : base_(x.base_),
        ptr_(x.ptr_) {
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
  SharedPtr(SharedPtr<U>&& x) noexcept
      : base_(x.base_),
        ptr_(x.ptr_) {
    x.base_ = nullptr;
    x.ptr_ = nullptr;
  }

  ~SharedPtr() { Reset(); }

  void Reset() {
    if (base_ && base_->count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      assert(base_->deleter);
      // do not delete self
      auto deleter = base_->deleter;
      deleter(const_cast<std::remove_cv_t<T>*>(ptr_));
    }
    ptr_ = nullptr;
    base_ = nullptr;
  }

  /// @note ptr must be created using MakeShared, otherwise polymorphism will not work
  constexpr void Reset(shared_ref_ptr_t, T* ptr) noexcept {
    Reset();
    ptr_ = ptr;
    base_ = ptr;
    if (base_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  /// @note ptr must be created using MakeShared, otherwise polymorphism will not work
  constexpr void Reset(shared_adopt_ptr_t, T* ptr) noexcept {
    Reset();
    ptr_ = ptr;
    base_ = ptr;
  }

  SharedPtr& operator=(const SharedPtr& x) noexcept {
    if (this != &x) {
      this->~SharedPtr();
      new (this) SharedPtr(x);
    }
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& x) noexcept {
    if (this != &x) {
      this->~SharedPtr();
      new (this) SharedPtr(std::move(x));
    }
    return *this;
  }

  SharedPtr& operator=(std::nullptr_t) noexcept {
    return *this = SharedPtr();
  }

  template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
  SharedPtr& operator=(const SharedPtr<U>& x) noexcept {
    if (*this != x) {
      this->~SharedPtr();
      new (this) SharedPtr(x);
    }
    return *this;
  }

  template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
  SharedPtr& operator=(SharedPtr<U>&& x) noexcept {
    if (*this != x) {
      this->~SharedPtr();
      new (this) SharedPtr(std::move(x));
    }
    return *this;
  }

  explicit operator bool() const noexcept {
    return !!ptr_;
  }

  T& operator*() const noexcept {
    return *ptr_;
  }

  T* operator->() const noexcept {
    return ptr_;
  }

  // Ownership is transferred to caller.
  [[nodiscard]] constexpr T* Leak() noexcept {
    base_ = nullptr;
    return std::exchange(ptr_, nullptr);
  }

  T* Get() const noexcept {
    return ptr_;
  }

  uint32_t UseCount() const noexcept {
    if (base_) {
      return base_->count.load(std::memory_order_acquire);
    } else {
      return 0;
    }
  }

  template <typename U, typename... A>
  friend SharedPtr<U> MakeShared(A&&... a);

  template <typename U, bool esft>
  friend class SharedPtrMakeHelper;

  template <typename U>
  friend class EnableSharedFromThis;

  template <typename V, typename U>
  friend SharedPtr<V> StaticPointerCast(const SharedPtr<U>& p);

  template <typename U>
  friend class SharedPtr;

 private:
  SharedPtr(SharedPtrCountBase* base, T* ptr) noexcept
      : base_(base), ptr_(ptr) {
    if (base_ && ptr_) {
      base_->count.fetch_add(1, std::memory_order_relaxed);
    }
  }

  explicit SharedPtr(EnableSharedFromThis<std::remove_cv_t<T>>* ptr) noexcept
      : SharedPtr(static_cast<SharedPtrCountBase*>(ptr), static_cast<T*>(ptr)) {}

 private:
  mutable SharedPtrCountBase* base_ = nullptr;
  mutable T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator!=(const SharedPtr<T>& x, const SharedPtr<U>& y) {
  return x.Get() != y.Get();
}

template <typename T>
inline bool operator!=(const SharedPtr<T>& x, std::nullptr_t) {
  return x.Get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, const SharedPtr<T>& y) {
  return nullptr != y.Get();
}

template <typename T>
class SharedPtrMakeHelper<T, true> {
 public:
  template <typename... A>
  static SharedPtr<T> Make(A&&... a) {
    T* ptr = New<T>(std::forward<A>(a)...);
    if (ptr) {
      static_cast<SharedPtrCountBase*>(ptr)->deleter = [ptr](void* p) {
        Delete(static_cast<std::remove_pointer_t<decltype(ptr)>*>(p));
      };
    }
    return SharedPtr<T>(ptr, ptr);
  }
};

template <typename T, typename... A>
inline SharedPtr<T> MakeShared(A&&... a) {
  using helper = SharedPtrMakeHelper<T, std::is_base_of<SharedPtrCountBase, T>::value>;
  return helper::Make(std::forward<A>(a)...);
}

template <typename T>
inline SharedPtr<T> EnableSharedFromThis<T>::SharedFromThis() {
  auto* unconst = reinterpret_cast<EnableSharedFromThis<std::remove_cv_t<T>>*>(this);
  return SharedPtr<T>(unconst);
}

template <typename T>
inline SharedPtr<const T> EnableSharedFromThis<T>::SharedFromThis() const {
  auto* esft = const_cast<EnableSharedFromThis*>(this);
  auto* unconst = reinterpret_cast<EnableSharedFromThis<std::remove_cv_t<T>>*>(esft);
  return SharedPtr<const T>(unconst);
}

template <typename T, typename U>
inline SharedPtr<T> StaticPointerCast(const SharedPtr<U>& p) {
  return SharedPtr<T>(p.base_, static_cast<T*>(p.ptr_));
}

}  // namespace trpc::object_pool
