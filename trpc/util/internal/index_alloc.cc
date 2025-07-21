//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/index_alloc.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/index_alloc.h"

namespace trpc::internal {

std::size_t IndexAlloc::Next() {
  std::scoped_lock _(lock_);
  if (!recycled_.empty()) {
    auto rc = recycled_.back();
    recycled_.pop_back();
    return rc;
  }
  return current_++;
}

void IndexAlloc::Free(std::size_t index) {
  std::scoped_lock _(lock_);
  recycled_.push_back(index);
}

}  // namespace trpc::internal
