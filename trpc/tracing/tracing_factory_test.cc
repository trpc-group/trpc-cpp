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

#include "trpc/tracing/tracing_factory.h"

#include "gtest/gtest.h"

namespace trpc::testing {

constexpr char kTestTracingName[] = "test_tracing";

class TestTracing : public trpc::Tracing {
 public:
  std::string Name() const override { return kTestTracingName; }

  int Init() noexcept override { return 0; }
};

TEST(TracingFactory, TestTracingFactory) {
  TracingPtr p = MakeRefCounted<TestTracing>();
  TracingFactory::GetInstance()->Register(p);
  ASSERT_EQ(p.Get(), TracingFactory::GetInstance()->Get(kTestTracingName).Get());

  ASSERT_EQ(1, TracingFactory::GetInstance()->GetAllPlugins().size());

  TracingFactory::GetInstance()->Clear();
  ASSERT_TRUE(TracingFactory::GetInstance()->Get(kTestTracingName) == nullptr);
}

}  // namespace trpc::testing
