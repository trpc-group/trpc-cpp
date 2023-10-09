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

#include "trpc/telemetry/trpc_telemetry.h"

#include "gtest/gtest.h"

#include "trpc/telemetry/telemetry_factory.h"

namespace trpc::testing {

constexpr char kTestTracingName[] = "test_tracing";

class TestTracing : public trpc::Tracing {
 public:
  std::string Name() const override { return kTestTracingName; }
};

constexpr char kTestTelemetryName[] = "test_telemetry";

class TestTelemetry : public trpc::Telemetry {
 public:
  std::string Name() const override { return kTestTelemetryName; }

  TracingPtr GetTracing() { return MakeRefCounted<TestTracing>(); }

  MetricsPtr GetMetrics() { return nullptr; }

  LoggingPtr GetLog() { return nullptr; }
};

class TrpcTelemetryTest : public ::testing::Test {
 public:
  static void SetUpTestCase() { TelemetryFactory::GetInstance()->Register(MakeRefCounted<TestTelemetry>()); }

  static void TearDownTestCase() { TelemetryFactory::GetInstance()->Clear(); }
};

TEST_F(TrpcTelemetryTest, Init) {
  ASSERT_TRUE(telemetry::Init());
  telemetry::Stop();
  telemetry::Destroy();
}

}  // namespace trpc::testing
