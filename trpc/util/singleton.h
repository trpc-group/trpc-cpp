//
//
// Copyright (C) 2016Tencent. All rights reserved.
// TarsCpp is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/TarsCloud/TarsCpp/blob/v1.0.0/util/include/util/tc_singleton.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <mutex>

namespace trpc {

/// @brief Use `new` to create singleton object
template <typename T>
class CreateUsingNew {
 public:
  static T *Create() { return new T; }

  static void Destroy(T *t) { delete t; }
};

/// @brief Use `static` to create singleton object
template <typename T>
class CreateStatic {
 public:
  union MaxAlign {
    char t_[sizeof(T)];
    long double longDouble_;
  };

  static T *Create() {
    static MaxAlign t;
    return new (&t) T;
  }

  static void Destroy(T *t) { t->~T(); }
};

/// @brief Release the singleton object when the program exits
template <typename T>
class DefaultLifetime {
 public:
  static void DeadReference() { assert(false); }

  static void ScheduleDestruction(T *, void (*pFun)()) { std::atexit(pFun); }
};

/// @brief Never Destroy
template <typename T>
struct NoDestroyLifetime {
  static void ScheduleDestruction(T *, void (*)()) {}

  static void DeadReference() {}
};

/// @brief The implementation for singleton 
///
/// class A : public TC_Singleton<A, CreateStatic,  DefaultLifetime> {
///  public:
///   A() = default;
///
///   ~A() = default;
///
///   void Test() { cout << "test A" << endl; }
///
/// };
///
template <typename T, template <class> class CreatePolicy = CreateUsingNew,
          template <class> class LifetimePolicy = DefaultLifetime>
class Singleton {
 public:
  typedef T instance_type;
  typedef volatile T volatile_type;

  static T* GetInstance() {
    T *tmp = instance_.load(std::memory_order_relaxed);
    if (tmp == nullptr) {
      std::lock_guard<std::mutex> lock(mutex_);
      tmp = instance_.load(std::memory_order_relaxed);
      if (tmp == nullptr) {
        if (destroyed_.load(std::memory_order_acquire)) {
          LifetimePolicy<T>::DeadReference();
          destroyed_.store(false, std::memory_order_release);
        }
        tmp = CreatePolicy<T>::Create();
        std::atomic_thread_fence(std::memory_order_release);
        instance_.store(tmp, std::memory_order_relaxed);
        LifetimePolicy<T>::ScheduleDestruction(tmp, &DestroySingleton);
      }
    }
    return tmp;
  }

  virtual ~Singleton() {}

 protected:
  static void DestroySingleton() {
    assert(!destroyed_.load(std::memory_order_acquire));
    CreatePolicy<T>::Destroy(instance_.load(std::memory_order_acquire));
    instance_.store(nullptr, std::memory_order_release);
    destroyed_.store(true, std::memory_order_release);
  }

 protected:
  static std::mutex mutex_;
  static std::atomic<T*> instance_;
  static std::atomic<bool> destroyed_;

 protected:
  Singleton() {}
  Singleton(const Singleton &) = delete;
  Singleton& operator=(const Singleton&) = delete;
};

template <class T, template <class> class CreatePolicy, template <class> class LifetimePolicy>
std::mutex Singleton<T, CreatePolicy, LifetimePolicy>::mutex_;

template <class T, template <class> class CreatePolicy, template <class> class LifetimePolicy>
std::atomic<bool> Singleton<T, CreatePolicy, LifetimePolicy>::destroyed_{false};

template <class T, template <class> class CreatePolicy, template <class> class LifetimePolicy>
std::atomic<T*> Singleton<T, CreatePolicy, LifetimePolicy>::instance_{nullptr};

}  // namespace trpc
