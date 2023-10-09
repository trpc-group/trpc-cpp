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

#include "trpc/telemetry/telemetry_factory.h"

#include "gtest/gtest.h"

namespace trpc::testing {

constexpr char kTestTelemetryName[] = "test_telemetry";

class TestTelemetry : public trpc::Telemetry {
 public:
  std::string Name() const override { return kTestTelemetryName; }

  TracingPtr GetTracing() { return nullptr; }

  MetricsPtr GetMetrics() { return nullptr; }

  LoggingPtr GetLog() { return nullptr; }
};

TEST(TelemetryFactoryTest, TelemetryFactory) {
  TelemetryPtr p = MakeRefCounted<TestTelemetry>();
  TelemetryFactory::GetInstance()->Register(p);
  ASSERT_EQ(p.Get(), TelemetryFactory::GetInstance()->Get(kTestTelemetryName).Get());

  ASSERT_EQ(1, TelemetryFactory::GetInstance()->GetAllPlugins().size());

  TelemetryFactory::GetInstance()->Clear();
  ASSERT_TRUE(TelemetryFactory::GetInstance()->Get(kTestTelemetryName) == nullptr);
}

}  // namespace trpc::testing
