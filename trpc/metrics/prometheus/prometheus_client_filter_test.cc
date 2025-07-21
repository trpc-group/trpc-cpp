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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/metrics/prometheus/prometheus_client_filter.h"

#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/prometheus/prometheus_metrics.h"

namespace trpc::testing {

class PrometheusClientFilterTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    int ret = TrpcConfig::GetInstance()->Init("./trpc/metrics/prometheus/testing/prometheus_metrics.yaml");
    ASSERT_EQ(0, ret);
    metrics_ = MakeRefCounted<PrometheusMetrics>();
    ASSERT_EQ(0, metrics_->Init());
    MetricsFactory::GetInstance()->Register(metrics_);
    client_filter_ = std::make_shared<PrometheusClientFilter>();
    ASSERT_EQ(0, client_filter_->Init());
  }
  static void TearDownTestCase() {}

 protected:
  static MessageClientFilterPtr client_filter_;
  static MetricsPtr metrics_;
};

MessageClientFilterPtr PrometheusClientFilterTest::client_filter_;
MetricsPtr PrometheusClientFilterTest::metrics_;

TEST_F(PrometheusClientFilterTest, Init) {
  ASSERT_EQ(trpc::prometheus::kPrometheusMetricsName, client_filter_->Name());
  std::vector<FilterPoint> points = client_filter_->GetFilterPoint();
  ASSERT_EQ(2, points.size());
  ASSERT_TRUE(std::find(points.begin(), points.end(), FilterPoint::CLIENT_POST_RPC_INVOKE) != points.end());
  ASSERT_TRUE(std::find(points.begin(), points.end(), FilterPoint::CLIENT_POST_RPC_INVOKE) != points.end());
}

TEST_F(PrometheusClientFilterTest, Report) {
  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  trpc::ClientContextPtr context = trpc::MakeRefCounted<trpc::ClientContext>(trpc_codec);
  context->SetCallerName("trpc.test.helloworld.client");
  context->SetCalleeName("trpc.test.helloworld.Greeter");
  context->SetFuncName("SayHello");

  FilterStatus status;
  client_filter_->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  ASSERT_EQ(FilterStatus::CONTINUE, status);

  trpc::Status frame_status;
  frame_status.SetFrameworkRetCode(101);
  context->SetStatus(frame_status);
  client_filter_->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  ASSERT_EQ(FilterStatus::CONTINUE, status);
}

}  // namespace trpc::testing
#endif
