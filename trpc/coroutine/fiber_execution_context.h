//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/execution_context.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "trpc/coroutine/fiber_local.h"
#include "trpc/util/deferred.h"
#include "trpc/util/internal/index_alloc.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

namespace detail {

struct ExecutionLocalIndexTag;

}  // namespace detail

/// @brief `FiberExecutionContext` serves as a container for all information relevant to a
/// logical fiber / a group of fibers of execution.
class FiberExecutionContext : public object_pool::EnableLwSharedFromThis<FiberExecutionContext> {
 public:
  template <class F>
  decltype(auto) Execute(F&& cb) {
    ScopedDeferred _([old = *current_] { *current_ = old; });
    *current_ = this;
    return std::forward<F>(cb)();
  }

  /// @brief Clear this execution context for reuse.
  void Clear();

  /// @brief Capture current execution context.
  static object_pool::LwSharedPtr<FiberExecutionContext> Capture();

  /// @brief Create a new execution context.
  static object_pool::LwSharedPtr<FiberExecutionContext> Create();

  /// @brief Get current execution context.
  static FiberExecutionContext* Current() { return *current_; }

 private:
  template <class>
  friend class FiberExecutionLocal;

  struct ElsEntry {
    std::atomic<void*> ptr{nullptr};
    void (*deleter)(void*);

    ~ElsEntry() {
      if (auto p = ptr.load(std::memory_order_acquire)) {
        deleter(p);
      }
    }
  };

  static constexpr auto kInlineElsSlots = 8;
  static FiberLocal<FiberExecutionContext*> current_;

  ElsEntry* GetElsEntry(std::size_t slot_index) {
    if (TRPC_LIKELY(slot_index < std::size(inline_els_))) {
      return &inline_els_[slot_index];
    }
    return GetElsEntrySlow(slot_index);
  }

  ElsEntry* GetElsEntrySlow(std::size_t slow_index);

 private:
  ElsEntry inline_els_[kInlineElsSlots];
  std::mutex external_els_lock_;
  std::unordered_map<std::size_t, ElsEntry> external_els_;

  std::mutex els_init_lock_;
};

/// @brief Local storage a given fiber execution context.
/// @note Note that since fiber execution context can be shared by multiple (possibly
// concurrently running) fibers, access to `T` must be synchronized.
// `FiberExecutionLocal` guarantees thread-safety when initialize `T`.
template <class T>
class FiberExecutionLocal {
 public:
  FiberExecutionLocal() : slot_index_(GetIndexAlloc()->Next()) {}
  ~FiberExecutionLocal() { GetIndexAlloc()->Free(slot_index_); }

  T* operator->() const noexcept { return Get(); }
  T& operator*() const noexcept { return *Get(); }

  void UnsafeInit(T* ptr, void (*deleter)(T*)) {
    auto&& entry = FiberExecutionContext::Current()->GetElsEntry(slot_index_);
    TRPC_CHECK(entry, "Initializing ELS must be done inside execution context.");
    TRPC_CHECK(entry->ptr.load(std::memory_order_relaxed) == nullptr,
                "Initializeing an already-initialized ELS?");
    entry->ptr.store(ptr, std::memory_order_release);

    entry->deleter = reinterpret_cast<void (*)(void*)>(deleter);
  }

  T* Get() const noexcept {
    auto&& current = FiberExecutionContext::Current();
    TRPC_DCHECK(current, "Getting ELS is only meaningful inside execution context.");

    auto&& entry = current->GetElsEntry(slot_index_);
    if (auto ptr = entry->ptr.load(std::memory_order_acquire); TRPC_LIKELY(ptr)) {
      return reinterpret_cast<T*>(ptr);
    }
    return UninitializedGetSlow();
  }

  T* UninitializedGetSlow() const noexcept;

  static trpc::internal::IndexAlloc* GetIndexAlloc() noexcept {
    return trpc::internal::IndexAlloc::For<detail::ExecutionLocalIndexTag>();
  }

 private:
  std::size_t slot_index_;
};

template <class T>
T* FiberExecutionLocal<T>::UninitializedGetSlow() const noexcept {
  auto ectx = FiberExecutionContext::Current();
  auto&& entry = ectx->GetElsEntry(slot_index_);
  std::scoped_lock _(ectx->els_init_lock_);
  if (entry->ptr.load(std::memory_order_acquire) == nullptr) {
    auto ptr = std::make_unique<T>();
    entry->deleter = [](auto* p) { delete reinterpret_cast<T*>(p); };
    entry->ptr.store(ptr.release(), std::memory_order_release);
  }

  return reinterpret_cast<T*>(entry->ptr.load(std::memory_order_relaxed));
}

namespace object_pool {

template <>
struct ObjectPoolTraits<FiberExecutionContext> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

}  // namespace trpc
