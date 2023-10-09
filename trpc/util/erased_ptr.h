//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/erased_ptr.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <utility>

namespace trpc {

/// @brief RAII wrapper for holding type erased pointers. Type-safety is your own
///        responsibility.
class ErasedPtr {
 public:
  using Deleter = void (*)(void*);

  // A default constructed one is an empty one.
  // implicit
  constexpr ErasedPtr(std::nullptr_t = nullptr)  // NOLINT
      : ptr_(nullptr), deleter_(nullptr) {}

  // Ownership taken.
  template <class T>
  constexpr explicit ErasedPtr(T* ptr) noexcept : ptr_(ptr), deleter_([](void* ptr) { delete static_cast<T*>(ptr); }) {}
  template <class T, class D>
  constexpr ErasedPtr(T* ptr, D deleter) noexcept : ptr_(ptr), deleter_(deleter) {}

  // Movable
  constexpr ErasedPtr(ErasedPtr&& ptr) noexcept : ptr_(ptr.ptr_), deleter_(ptr.deleter_) { ptr.ptr_ = nullptr; }

  ErasedPtr& operator=(ErasedPtr&& ptr) noexcept {
    if (&ptr != this) {
      Reset();
    }
    std::swap(ptr_, ptr.ptr_);
    std::swap(deleter_, ptr.deleter_);
    return *this;
  }

  // But not copyable.
  ErasedPtr(const ErasedPtr&) = delete;
  ErasedPtr& operator=(const ErasedPtr&) = delete;

  // Any resource we're holding is freed in dtor.
  ~ErasedPtr() {
    if (ptr_) {
      deleter_(ptr_);
    }
  }

  /// @brief Accessor.
  /// @return void*
  constexpr void* Get() const noexcept { return ptr_; }

  /// @brief It's your responsibility to check if the type match.
  /// @return T*
  template <class T>
  T* UnsafeGet() const noexcept {  // Or `UncheckedGet()`?
    return reinterpret_cast<T*>(Get());
  }

  /// @brief ITest if this object holds a valid pointer.
  constexpr explicit operator bool() const noexcept { return !!ptr_; }

  /// @brief IFree any resource this class holds and reset its internal pointer to `nullptr`.
  constexpr void Reset(std::nullptr_t = nullptr) noexcept {
    if (ptr_) {
      deleter_(ptr_);
      ptr_ = nullptr;
    }
  }

  /// @brief Release ownership of its internal object.
  [[nodiscard]] void* Leak() noexcept { return std::exchange(ptr_, nullptr); }

  /// @brief his is the only way you can destroy the pointer you obtain from `Leak()`.
  constexpr Deleter GetDeleter() const noexcept { return deleter_; }

 private:
  void* ptr_;
  Deleter deleter_;
};

/// @brief Construct an `ErasedPtr`
template <class T, class... Args>
ErasedPtr MakeErased(Args&&... args) {
  return ErasedPtr(new T(std::forward<Args>(args)...));
}

}  // namespace trpc
