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
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/util/combiner.h"

#include "gtest/gtest.h"

#include "trpc/rpcz/collector.h"
#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/reducer.h"
#include "trpc/rpcz/util/rpcz_fixture.h"

namespace trpc::testing {

using BlockCombinerType = trpc::rpcz::BlockCombiner<trpc::rpcz::Span*,
                                                    trpc::rpcz::Span*,
                                                    trpc::rpcz::CombinerCollected>;
using BlockGroupType = BlockCombinerType::RpczBlockGroup;

/// @brief Test basic operations for BlockCombiner.
TEST(ElementContainerTest, StoreLoadExchangeModify) {
  // Get.
  BlockCombinerType combiner;
  auto* block = combiner.GetOrCreateTlsBlock();
  ASSERT_NE(block, nullptr);
  ASSERT_NE(block->combiner_, nullptr);

  // Store.
  trpc::rpcz::Span* span_ptr = PackServerSpan(0);
  block->element_.Store(span_ptr);

  // Load.
  trpc::rpcz::Span* span;
  block->element_.Load(&span);
  ASSERT_EQ(span->SpanId(), 0);

  // Exchange.
  trpc::rpcz::Span* null_span{nullptr};
  trpc::rpcz::Span* new_span;

  block->element_.Exchange(&new_span, null_span);
  ASSERT_EQ(new_span->SpanId(), 0);

  trpc::rpcz::Span* temp_span;
  block->element_.Load(&temp_span);
  ASSERT_EQ(temp_span, nullptr);

  // Modify.
  trpc::rpcz::Span* first_span_ptr = PackServerSpan(1);
  block->element_.Modify(combiner.GetOp(), first_span_ptr);
  trpc::rpcz::Span* second_span_ptr1 = PackServerSpan(2);
  block->element_.Modify(combiner.GetOp(), second_span_ptr1);

  trpc::rpcz::Span* check_span;
  block->element_.Load(&check_span);
  ASSERT_NE(check_span->next(), check_span);
}

/// @brief Test ResetAllBlocks.
TEST(BlockCombinerTest, ResetAllBlocks) {
  BlockCombinerType combiner_;
  auto* block = combiner_.GetOrCreateTlsBlock();
  trpc::rpcz::Span* span_ptr = PackServerSpan(99);
  block->element_.Store(span_ptr);

  auto* span = combiner_.ResetAllBlocks();

  ASSERT_EQ(span->SpanId(), 99);

  trpc::rpcz::Span* check_span;
  block->element_.Load(&check_span);
  ASSERT_EQ(check_span, nullptr);
}

/// @brief Test CallOpReturningVoid.
TEST(CombinerTest, CallOpReturningVoid) {
  trpc::rpcz::CombinerCollected combiner_collect;
  trpc::rpcz::Span* temp_span{nullptr};
  trpc::rpcz::Span* span_ptr = PackServerSpan(4);
  trpc::rpcz::CallOpReturningVoid(combiner_collect, temp_span, span_ptr);

  ASSERT_EQ(temp_span->SpanId(), 4);
}

}  // namespace trpc::testing
#endif
