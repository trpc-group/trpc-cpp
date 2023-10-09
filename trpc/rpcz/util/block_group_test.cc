//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ

#include "trpc/rpcz/util/block_group.h"

#include <stdlib.h>
#include <time.h>

#include "gtest/gtest.h"

#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/combiner.h"
#include "trpc/rpcz/util/reducer.h"

namespace trpc::testing {

using BlockCombinerType = trpc::rpcz::BlockCombiner<trpc::rpcz::Span*,
                                                    trpc::rpcz::Span*,
                                                    trpc::rpcz::CombinerCollected>;
using BlockGroupType = BlockCombinerType::RpczBlockGroup;

/// @brief Test GetTlsBlock.
TEST(BlockGroupTest, GetTlsBlock) {
  auto* block = BlockGroupType::GetTlsBlock(0);
  ASSERT_EQ(block, nullptr);
}

/// @brief Test GetOrCreateTlsBlock.
TEST(BlockGroupTest, GetOrCreateTlsBlock) {
  auto* block = BlockGroupType::GetOrCreateTlsBlock(0);
  ASSERT_NE(block, nullptr);
  ASSERT_FALSE(block->combiner_);
  ASSERT_NE(BlockGroupType::GetTlsBlock(0), nullptr);
}

}  // namespace trpc::testing
#endif
