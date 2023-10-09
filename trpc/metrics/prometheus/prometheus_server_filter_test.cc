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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/metrics/prometheus/prometheus_server_filter.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/prometheus/prometheus_metrics.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/rpc/rpc_service_impl.h"
#include "trpc/server/server_context.h"
#include "trpc/server/testing/server_context_testing.h"

namespace trpc::testing {

class PrometheusServerFilterTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    int ret = TrpcConfig::GetInstance()->Init("./trpc/metrics/prometheus/testing/prometheus_metrics.yaml");
    ASSERT_EQ(0, ret);

    codec::Init();
    serialization::Init();

    metrics_ = MakeRefCounted<PrometheusMetrics>();
    ASSERT_EQ(0, metrics_->Init());
    MetricsFactory::GetInstance()->Register(metrics_);
    server_filter_ = std::make_shared<PrometheusServerFilter>();
    ASSERT_EQ(0, server_filter_->Init());
  }

  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
  }

 protected:
  static MessageServerFilterPtr server_filter_;
  static MetricsPtr metrics_;
};

MessageServerFilterPtr PrometheusServerFilterTest::server_filter_;
MetricsPtr PrometheusServerFilterTest::metrics_;

TEST_F(PrometheusServerFilterTest, Init) {
  ASSERT_EQ(trpc::prometheus::kPrometheusMetricsName, server_filter_->Name());
  std::vector<FilterPoint> points = server_filter_->GetFilterPoint();
  ASSERT_EQ(2, points.size());
  ASSERT_TRUE(std::find(points.begin(), points.end(), FilterPoint::SERVER_POST_RECV_MSG) != points.end());
  ASSERT_TRUE(std::find(points.begin(), points.end(), FilterPoint::SERVER_PRE_SEND_MSG) != points.end());
}

TEST_F(PrometheusServerFilterTest, Report) {
  DummyTrpcProtocol req_data;
  trpc::test::helloworld::HelloRequest hello_req;
  NoncontiguousBuffer req_bin_data;
  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));
  std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();
  ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));

  FilterStatus status;
  server_filter_->operator()(status, FilterPoint::SERVER_POST_RECV_MSG, context);
  ASSERT_EQ(FilterStatus::CONTINUE, status);

  trpc::Status frame_status;
  frame_status.SetFrameworkRetCode(101);
  context->SetStatus(frame_status);
  server_filter_->operator()(status, FilterPoint::SERVER_PRE_SEND_MSG, context);
  ASSERT_EQ(FilterStatus::CONTINUE, status);
}

}  // namespace trpc::testing
#endif
