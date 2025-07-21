//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/deferred.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <utility>

#include "trpc/util/function.h"

namespace trpc {

/// @brief Call `F` on destruction.
template <class F>
class ScopedDeferred {
 public:
  explicit ScopedDeferred(F&& f) : action_(std::move(f)) {}
  ~ScopedDeferred() { action_(); }

  // Noncopyable / nonmovable.
  ScopedDeferred(const ScopedDeferred&) = delete;
  ScopedDeferred& operator=(const ScopedDeferred&) = delete;

 private:
  F action_;
};

/// @brief Call action on destruction. Moveable. Dismissable.
class Deferred {
 public:
  Deferred() = default;

  template <class F>
  explicit Deferred(F&& f) : action_(std::forward<F>(f)) {}
  Deferred(Deferred&&) = default;
  Deferred& operator=(Deferred&&) = default;
  ~Deferred() {
    if (action_) {
      action_();
    }
  }

  explicit operator bool() const noexcept { return !!action_; }

  void Dismiss() noexcept { action_ = nullptr; }

 private:
  Function<void()> action_;
};

}  // namespace trpc
