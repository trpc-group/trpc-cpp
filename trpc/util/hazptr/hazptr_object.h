//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/hazptr/hazptr_object.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <memory>
#include <utility>

#include "trpc/util/check.h"
#include "trpc/util/hazptr/hazptr_domain.h"
#include "trpc/util/log/logging.h"

namespace trpc {

template <class T, class D = std::default_delete<T>>
class HazptrObject;
class HazptrDomain;

namespace hazptr {

// "Real" base class for `Hazptr`-enabled classes.
//
// FOR INTERNAL USE. USE `HazptrObject<T, D>` INSTEAD.
class Object {
 private:
  template <class T, class D>
  friend class trpc::HazptrObject;
  friend class trpc::HazptrDomain;

  constexpr Object() = default;
  virtual ~Object() = default;  // It making dtor `virtual` acceptable?

  // Nothing is copied / moved.
  constexpr Object(const Object&) {}
  constexpr Object(Object&&) noexcept {}
  constexpr Object& operator=(const Object&) { return *this; }
  constexpr Object& operator=(Object&&) noexcept { return *this; }

  // Free resources allocated for this object.
  virtual void DestroySelf() noexcept = 0;

  // Check for double free. For debugging purpose only.
  void PreRetireCheck() { TRPC_CHECK_EQ(next_, this); }
  void PushRetired(HazptrDomain* domain);
  void PushReclaim(HazptrDomain* domain);

 private:
  Object* next_ = {this};
};

}  // namespace hazptr

// For your objects to be hazard-pointer-enabled, you need to inherit from this
// base class.
template <class T, class D>
class HazptrObject : public hazptr::Object {
 public:  // protected?
  // Retire the object. The object will be destroyed by `deleter` (after an
  // nondeterministic time period.).
  //
  // Note that before calling this method, you must make sure that no MORE
  // reference to it can be made. (Normally you store the pointer in an
  // `std::atomic<...>` variable, and calls `Retire` after change the atomic
  // variable points to another object.)
  void Retire(D deleter = {}, HazptrDomain* domain = GetDefaultHazptrDomain()) {
    PreRetireCheck();
    deleter_ = std::move(deleter);
    PushRetired(domain);
  }

  // Reclaim the object. The object will be destroyed by `deleter` immediately
  void Reclaim(D deleter = {}, HazptrDomain* domain = GetDefaultHazptrDomain()) {
    PreRetireCheck();
    deleter_ = std::move(deleter);
    PushReclaim(domain);
  }

 private:
  void DestroySelf() noexcept override { deleter_(static_cast<T*>(this)); }

 private:
  /* [[no_unique_address]] */ D deleter_;
};

}  // namespace trpc
