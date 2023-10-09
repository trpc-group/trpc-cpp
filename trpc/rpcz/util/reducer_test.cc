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
#include "trpc/rpcz/util/reducer.h"

#include "gtest/gtest.h"

#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/rpcz_fixture.h"

namespace trpc::testing {

using ReducerType = trpc::rpcz::Reducer<trpc::rpcz::Span*, trpc::rpcz::CombinerCollected>;

/// @brief Test << operator and Reset.
TEST(ReducerTest, Reset) {
  ReducerType reducer;
  trpc::rpcz::Span* span_ptr = PackServerSpan(10);
  reducer << span_ptr;
  auto* span = reducer.Reset();
  ASSERT_EQ(span->SpanId(), 10);
}

}  // namespace trpc::testing
#endif
