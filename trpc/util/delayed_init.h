//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/delayed_init.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <optional>
#include <utility>

namespace trpc {

/// @brief This class allows you to delay initialization of an object.
template <class T>
class DelayedInit {
 public:
  template <class... Args>
  void Init(Args&&... args) {
    value_.emplace(std::forward<Args>(args)...);
  }

  void Destroy() { value_ = std::nullopt; }

  T* operator->() {
    TRPC_DCHECK(value_);
    return &*value_;
  }

  const T* operator->() const {
    TRPC_DCHECK(value_);
    return &*value_;
  }

  T& operator*() {
    TRPC_DCHECK(value_);
    return *value_;
  }

  const T& operator*() const {
    TRPC_DCHECK(value_);
    return *value_;
  }

  explicit operator bool() const { return !!value_; }

 private:
  std::optional<T> value_;
};

}  // namespace trpc
