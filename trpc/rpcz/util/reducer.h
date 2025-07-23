//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <type_traits>

#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/combiner.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::rpcz {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/reducer.h

/// @brief Represent initial forbidden operation.
/// @note Should be merged with tvar VoidOp, duplicate code here.
/// @private
struct VoidOp {
  template <typename T>
  T operator()(const T&, const T&) const {
    TRPC_ASSERT(false && "This function should never be called, abort");
  }
};

/// @brief Combine two list.
/// @private
struct CombinerCollected {
  /// @brief s1 before s2, and s1 always point to head of list.
  void operator()(Span*& s1, Span* s2) const {
    if (s2 == nullptr) {
      return;
    }
    if (s1 == nullptr) {
      s1 = s2;
      return;
    }
    s1->InsertBeforeAsList(s2);
  }
};

/// @brief Base class to merge thread local spans.
/// @private
template <typename T, typename Op, typename InvOp = VoidOp>
class Reducer {
 public:
  using CombinerType = BlockCombiner<T, T, Op>;
  using BlockType = typename CombinerType::Block;

  /// @brief Constructor.
  explicit Reducer(add_cr_non_integral_t<T> identity = T(), const Op& op = Op(), const InvOp& inv_op = InvOp())
      : combiner_(identity, identity, op) {}

  /// @brief Destructor.
  ~Reducer() = default;

  /// @brief Insert a new span into thread local list.
  Reducer& operator<<(add_cr_non_integral_t<T> value);

  /// @brief Reset all thread local blocks and return merged spans.
  T Reset() { return combiner_.ResetAllBlocks(); }

  /// @brief Get span operation.
  const Op& GetOp() const { return combiner_.GetOp(); }

 private:
  CombinerType combiner_;
};

/// @brief With op = CombinerCollected, operator<< simply insert new span into thread local list.
/// @private
template <typename T, typename Op, typename InvOp>
Reducer<T, Op, InvOp>& Reducer<T, Op, InvOp>::operator<<(add_cr_non_integral_t<T> value) {
  BlockType* block = combiner_.GetOrCreateTlsBlock();
  if (TRPC_UNLIKELY(!block)) {
    TRPC_FMT_ERROR("Fail to create block");
    return *this;
  }

  block->element_.Modify(combiner_.GetOp(), value);
  return *this;
}

// End of source codes that are from incubator-brpc.

}  // namespace trpc::rpcz
#endif
