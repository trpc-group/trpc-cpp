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

#include "trpc/tracing/trpc_tracing.h"

#include "gtest/gtest.h"

#include "trpc/tracing/tracing_factory.h"

namespace trpc::testing {

constexpr char kTestTracingName[] = "test_tracing";

class TestTracing : public trpc::Tracing {
 public:
  std::string Name() const override { return kTestTracingName; }

  int Init() noexcept override { return 0; }
};

class TrpcTracingTest : public ::testing::Test {
 public:
  static void SetUpTestCase() { TracingFactory::GetInstance()->Register(MakeRefCounted<TestTracing>()); }

  static void TearDownTestCase() { TracingFactory::GetInstance()->Clear(); }
};

TEST_F(TrpcTracingTest, All) {
  ASSERT_TRUE(tracing::Init());
  tracing::Stop();
  tracing::Destroy();
}

}  // namespace trpc::testing
