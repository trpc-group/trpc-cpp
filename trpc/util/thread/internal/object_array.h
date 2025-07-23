//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/thread_local/object_array.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cstdint>

#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

#include "trpc/util/align.h"
#include "trpc/util/function.h"
#include "trpc/util/internal/never_destroyed.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::tls::detail {

// FIXME: I'm not sure if this is equivalent to `using Entry = ...`.
template <class T>
struct Entry {
  std::aligned_storage_t<sizeof(T), alignof(T)> storage;
  static_assert(sizeof(T) == sizeof(storage));
};

// Performance note: ANY MODIFICATION TO THIS STRUCTURE MUST KEEP IT TRIVIAL.
// We'll be accessing it as thread-local variables often, if it's not trivial,
// we'd pay a dynamic initialization penalty on each access. (For now it's
// statically initialized as it's trivial and initialized to all zero.).
template <class T>
struct ObjectArrayCache {
  std::size_t limit{};
  T* objects{};
};

template <class T>
struct ObjectArray;

// Stores a series of objects, either initialized or not.
//
// Here objects are stored in "structure of arrays" instead of "array of
// structures" as the fields are accessed unequally frequent.
template <class T>
class LazyInitObjectArray {
  struct EntryDeleter {
    void operator()(Entry<T>* ptr) noexcept { operator delete(ptr); }
  };

  using AlignedArray = std::unique_ptr<Entry<T>[], EntryDeleter>;

 public:
  LazyInitObjectArray() = default;

  ~LazyInitObjectArray() {
    for (size_t i = 0; i != initialized_.size(); ++i) {
      if (initialized_[i]) {
        reinterpret_cast<T*>(&objects_[i])->~T();
      }
    }
  }

  // Move-only.
  LazyInitObjectArray(LazyInitObjectArray&&) = default;
  LazyInitObjectArray& operator=(LazyInitObjectArray&) = default;

  // Expands internal storage.
  //
  // Throwing inside leads to crash.
  void UninitializedExpand(std::size_t new_size) noexcept {
    TRPC_ASSERT(new_size > size());
    auto new_entries = AllocateAtLeastNEntries(new_size);

    // Move initialized objects.
    for (size_t i = 0; i != size(); ++i) {
      if (initialized_[i]) {
        auto from = reinterpret_cast<T*>(&objects_[i]);
        auto to = &new_entries[i];

        new (to) T(std::move(*from));
        from->~T();
      }  // It was uninitialized otherwise, skip it.
    }

    objects_ = std::move(new_entries);
    initialized_.resize(new_size, false);
  }

  template <class F>
  void InitializeAt(std::size_t index, F&& f) {
    TRPC_ASSERT(index < initialized_.size());
    TRPC_ASSERT(!initialized_[index]);
    initialized_[index] = true;
    std::forward<F>(f)(&objects_[index]);
  }

  void DestroyAt(std::size_t index) {
    TRPC_ASSERT(index < initialized_.size());
    TRPC_ASSERT(initialized_[index]);
    initialized_[index] = false;
    reinterpret_cast<T*>(&objects_[index])->~T();
  }

  bool IsInitializedAt(std::size_t index) const noexcept {
    TRPC_ASSERT(index < initialized_.size());
    return initialized_[index];
  }

  T* GetAt(std::size_t index) noexcept {
    TRPC_ASSERT(index < initialized_.size());
    TRPC_ASSERT(initialized_[index]);
    return reinterpret_cast<T*>(&objects_[index]);
  }

  T* GetObjectsMaybeUninitialized() noexcept {
    static_assert(sizeof(T) == sizeof(Entry<T>));
    return reinterpret_cast<T*>(objects_.get());
  }

  std::size_t size() const noexcept { return initialized_.size(); }

 private:
  AlignedArray AllocateAtLeastNEntries(std::size_t desired) {
    // Some memory allocator (e.g., some versions of gperftools) can allocate
    // adjacent memory region (even within a cache-line) to different threads.
    //
    // So as to avoid false-sharing, we align the memory size ourselves.
    auto aligned_size = sizeof(Entry<T>) * desired;
    aligned_size = (aligned_size + hardware_destructive_interference_size - 1) /
                   hardware_destructive_interference_size * hardware_destructive_interference_size;
    // FIXME: Do we have to call `Entry<T>`'s ctor? It's POD anyway.
    return AlignedArray(reinterpret_cast<Entry<T>*>(operator new(aligned_size)), {});
  }

 private:
  AlignedArray objects_;
  std::vector<bool> initialized_;  // `std::vector<bool>` is hard..
};

// This allows us to traverse through all thread's local object array.
template <class T>
class ObjectArrayRegistry {
 public:
  static ObjectArrayRegistry* Instance() {
    static trpc::internal::NeverDestroyedSingleton<ObjectArrayRegistry> instance;
    return instance.Get();
  }

  void Register(ObjectArray<T>* array) {
    std::scoped_lock _(lock_);
    TRPC_DEBUG_ASSERT(std::find(arrays_.begin(), arrays_.end(), array) == arrays_.end());
    arrays_.push_back(array);
  }

  void Deregister(ObjectArray<T>* array) {
    std::scoped_lock _(lock_);
    auto iter = std::find(arrays_.begin(), arrays_.end(), array);
    TRPC_ASSERT(iter != arrays_.end());
    arrays_.erase(iter);
  }

  template <class F>
  void ForEachLocked(std::size_t index, F&& f) {
    std::scoped_lock _(lock_);
    for (auto&& e : arrays_) {
      std::scoped_lock __(e->lock);
      if (index < e->objects.size()) {
        f(e);
      }
    }
  }

 private:
  std::mutex lock_;
  std::vector<ObjectArray<T>*> arrays_;
};

// Stores dynamically allocated thread-local variables.
template <class T>
struct ObjectArray {
  std::mutex lock;  // Synchronizes between traversal and update.
  LazyInitObjectArray<T> objects;

  ObjectArray() { ObjectArrayRegistry<T>::Instance()->Register(this); }
  ~ObjectArray() { ObjectArrayRegistry<T>::Instance()->Deregister(this); }
};

// This class helps us to keep track of current (newest) layout of
// `ObjectArray<T>`.
template <class T>
class ObjectArrayLayout {
  using InitializerPtr = Function<void(void*)>*;

 public:
  // Access newest layout with internal lock held.
  template <class F>
  void WithNewestLayoutLocked(F&& f) const noexcept {
    std::scoped_lock _(lock_);
    std::forward<F>(f)(initializers_);
  }

  // Returns: [Version ID, Index in resulting layout].
  template <class F>
  std::size_t CreateEntry(Function<void(void*)>* initializer, F&& cb) {
    std::scoped_lock _(lock_);

    // Let's see if we can reuse a slot.
    if (unused_.empty()) {
      TRPC_ASSERT(max_index_ == initializers_.size());
      initializers_.emplace_back(nullptr);
      unused_.push_back(max_index_++);
    }

    auto v = unused_.back();
    unused_.pop_back();
    TRPC_ASSERT(!initializers_[v]);
    initializers_[v] = initializer;

    // Called after the slot is initialized.
    std::forward<F>(cb)(v);
    return v;
  }

  // Free an index. It's the caller's responsibility to free the slot
  // beforehand.
  template <class F>
  void FreeEntry(std::size_t index, F&& cb) noexcept {
    std::scoped_lock _(lock_);

    // Called before the slot is freed.
    std::forward<F>(cb)();

    TRPC_ASSERT(initializers_[index]);
    initializers_[index] = nullptr;
    unused_.push_back(index);
  }

  static ObjectArrayLayout* Instance() {
    static trpc::internal::NeverDestroyedSingleton<ObjectArrayLayout> layout;
    return layout.Get();
  }

 private:
  mutable std::mutex lock_;
  std::vector<InitializerPtr> initializers_;
  std::vector<std::size_t> unused_;
  std::size_t max_index_{};
};

template <class T>
inline T* Add2Ptr(T* ptr, std::ptrdiff_t offset) {
  TRPC_DEBUG_ASSERT(offset % sizeof(T) == 0);
  return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(ptr) + offset);
}

template <class T>
ObjectArray<T>* GetNewestLocalObjectArray() {
  thread_local ObjectArray<T> array;

  // Expand internal storage and initialize new slots.
  ObjectArrayLayout<T>::Instance()->WithNewestLayoutLocked([&](auto&& layout) {
    std::scoped_lock _(array.lock);

    // We shouldn't be called otherwise.
    TRPC_ASSERT(array.objects.size() < layout.size());

    // Expand the object array.
    auto was = array.objects.size();
    array.objects.UninitializedExpand(layout.size());

    // Initialize the new slots.
    for (size_t i = was; i != array.objects.size(); ++i) {
      if (layout[i]) {
        array.objects.InitializeAt(i, *layout[i]);
      }  // Otherwise the slot is freed before we have a chance to initialize
         // it.
    }
  });
  return &array;
}

template <class T>
[[gnu::noinline]] T* ReloadLocalObjectArrayCache(std::ptrdiff_t offset, ObjectArrayCache<T>* cache) {
  TRPC_ASSERT(offset % sizeof(T) == 0);
  auto&& obj_array = GetNewestLocalObjectArray<T>();
  cache->objects = obj_array->objects.GetObjectsMaybeUninitialized();
  cache->limit = obj_array->objects.size() * sizeof(T);
  return Add2Ptr(cache->objects, offset);
}

template <class T>
[[gnu::noinline]] T* GetLocalObjectArrayAt(std::ptrdiff_t offset) {
  thread_local ObjectArrayCache<T> cache;
  TRPC_DEBUG_ASSERT(offset % sizeof(T) == 0);
  TRPC_DEBUG_ASSERT(offset >= 0);
  if (TRPC_LIKELY(offset < static_cast<int64_t>(cache.limit))) {
    return Add2Ptr(cache.objects, offset);
  }
  return ReloadLocalObjectArrayCache<T>(offset, &cache);
}

}  // namespace trpc::tls::detail
