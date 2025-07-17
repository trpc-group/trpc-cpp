//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/thread_local.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include "trpc/util/function.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/thread/internal/object_array.h"

namespace trpc {

namespace internal {

// Same as `ThreadLocal<T>` except that objects are initialized eagerly (also
// nondeterministically.). Note that `T`'s constructor may not touch other TLS
// variables, otherwise the behavior is undefined.
//
// Performs slightly better. For internal use only.
//
// Instances of `T` in different threads are guaranteed to reside in different
// cache line. However, if `T` itself allocates memory, there's no guarantee on
// how memory referred by `T` in different threads are allocated.
//
template <class T>
class ThreadLocalAlwaysInitialized {
 public:
  ThreadLocalAlwaysInitialized();
  ~ThreadLocalAlwaysInitialized();

  // Initialize object with a customized initializer.
  template <class F>
  explicit ThreadLocalAlwaysInitialized(F&& initializer);

  // Noncopyable / nonmovable.
  ThreadLocalAlwaysInitialized(const ThreadLocalAlwaysInitialized&) = delete;
  ThreadLocalAlwaysInitialized& operator=(const ThreadLocalAlwaysInitialized&) = delete;

  // Accessors.
  T* Get() const;
  T* operator->() const { return Get(); }
  T& operator*() const { return *Get(); }

  // Traverse through all instances among threads.
  //
  // CAUTION: Called with internal lock held. You may not touch TLS in `f`.
  // (Maybe we should name it as `ForEachUnsafe` or `ForEachLocked`?)
  template <class F>
  void ForEach(F&& f) const;

 private:
  // Placed as the first member to keep accessing it quick.
  //
  // Always a multiple of `kEntrySize`.
  std::ptrdiff_t offset_;
  Function<void(void*)> initializer_;
};

}  // namespace details

// Performance note:
//
// In some versions of Google's gperftools (tcmalloc), allocating memory from
// different threads often results in adjacent addresses (within a cacheline
// boundary). This allocation scheme CAN EASILY LEAD TO FALSE-SHARING AND HURT
// PERFORMANCE. As we often use `ThreadLocal<T>` for perf. optimization, this (I
// would say it's a bug) totally defeat the reason why we want a "thread-local"
// copy in the first place.
//
// Due to technical reasons (we don't have control on how instances of `T` are
// allocated), we can't workaround this quirk for you automatically, you need to
// annotate your `T` with `alignas` yourself.
//

// Support thread-local storage, with extra capability to traverse all instances
// among threads.
template <class T>
class ThreadLocal {
 public:
  constexpr ThreadLocal() : constructor_([]() { return new T(); }) {}

  template <typename F, std::enable_if_t<std::is_invocable_r_v<T*, F>, int> = 0>
  explicit ThreadLocal(F&& constructor) : constructor_(std::forward<F>(constructor)) {}

  T* Get() const {
    // NOT locked, I'm not sure if this is right. (However, it nonetheless
    // should work without problem and nobody else should make it non-null.)
    auto&& ptr = tlp_->get();
    return TRPC_LIKELY(!!ptr) ? ptr : MakeTlp();
  }
  T* operator->() const { return Get(); }
  T& operator*() const { return *Get(); }

  T* Leak() noexcept {
    std::scoped_lock _(init_lock_);
    return tlp_->release();
  }

  void Reset(T* newPtr = nullptr) noexcept {
    std::scoped_lock _(init_lock_);
    tlp_->reset(newPtr);
  }

  // `ForEach` calls `f` with pointer (i.e. `T*`) to each thread-local
  // instances.
  //
  // CAUTION: Called with internal lock held. You may not touch TLS in `f`.
  template <class F>
  void ForEach(F&& f) const {
    std::scoped_lock _(init_lock_);
    tlp_.ForEach([&](auto* p) {
      if (*p) {
        f(p->get());
      }
    });
  }

 private:
  // non-copyable
  ThreadLocal(const ThreadLocal&) = delete;
  ThreadLocal& operator=(const ThreadLocal&) = delete;
  ThreadLocal(const ThreadLocal&&) = delete;
  ThreadLocal& operator=(const ThreadLocal&&) = delete;

  [[gnu::noinline]] T* MakeTlp() const {
    std::scoped_lock _(init_lock_);
    auto const ptr = constructor_();
    tlp_->reset(ptr);
    return ptr;
  }

  mutable internal::ThreadLocalAlwaysInitialized<std::unique_ptr<T>> tlp_;

  // Synchronizes between `ForEach` and other member methods operating on
  // `tlp_`.
  mutable std::mutex init_lock_;
  Function<T*()> constructor_;
};

///////////////////////////////////////
// Implementation goes below.        //
///////////////////////////////////////

namespace internal {

template <class T>
ThreadLocalAlwaysInitialized<T>::ThreadLocalAlwaysInitialized()
    : ThreadLocalAlwaysInitialized([](void* ptr) { new (ptr) T(); }) {}

template <class T>
template <class F>
ThreadLocalAlwaysInitialized<T>::ThreadLocalAlwaysInitialized(F&& initializer)
    : initializer_(std::forward<F>(initializer)) {
  // Initialize the slot in every thread (that has allocated the slot in its
  // thread-local object array.).
  auto slot_initializer = [&](auto index) {
    // Called with layout lock held. Nobody else should be resizing its own
    // object array or mutating the (type-specific) global layout.

    // Initialize all slots (if the slot itself has already been allocated in
    // corresponding thread's object array) immediately so that we don't need to
    // check for initialization on `Get()`.
    tls::detail::ObjectArrayRegistry<T>::Instance()->ForEachLocked(
        index, [&](auto* p) { p->objects.InitializeAt(index, initializer_); });
  };

  // Allocate a slot.
  offset_ = tls::detail::ObjectArrayLayout<T>::Instance()->CreateEntry(&initializer_, slot_initializer) * sizeof(T);
}

template <class T>
ThreadLocalAlwaysInitialized<T>::~ThreadLocalAlwaysInitialized() {
  TRPC_ASSERT(offset_ % sizeof(T) == 0);
  auto index = offset_ / sizeof(T);

  // The slot is freed after we have destroyed all instances.
  tls::detail::ObjectArrayLayout<T>::Instance()->FreeEntry(index, [&] {
    // Called with layout lock held.

    // Destroy all instances. (We actually reconstructed a new one at the place
    // we were at.)
    tls::detail::ObjectArrayRegistry<T>::Instance()->ForEachLocked(index,
                                                                   [&](auto* p) { p->objects.DestroyAt(index); });
  });
}

template <class T>
inline T* ThreadLocalAlwaysInitialized<T>::Get() const {
  return tls::detail::GetLocalObjectArrayAt<T>(offset_);
}

template <class T>
template <class F>
void ThreadLocalAlwaysInitialized<T>::ForEach(F&& f) const {
  TRPC_ASSERT(offset_ % sizeof(T) == 0);
  auto index = offset_ / sizeof(T);

  tls::detail::ObjectArrayRegistry<T>::Instance()->ForEachLocked(index, [&](auto* p) { f(p->objects.GetAt(index)); });
}

}  // namespace internal

}  // namespace trpc
