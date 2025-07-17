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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/high_percentile/high_percentile_server_filter.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/filter/filter_manager.h"

namespace trpc::overload_control {
namespace testing {

class HighPercentileServerFilterTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/high_percentile/high_percentile.yaml");
    trpc::TrpcPlugin::GetInstance()->RegisterPlugins();
    InitHighPercentileServerFilter();
  }
  static void TearDownTestCase() { trpc::TrpcPlugin::GetInstance()->UnregisterPlugins(); }

  static ServerContextPtr MakeServerContext() {
    auto context = MakeRefCounted<ServerContext>();
    context->SetRequestMsg(std::make_shared<trpc::testing::TestProtocol>());
    return context;
  }

  static bool InitHighPercentileServerFilter() {
    MessageServerFilterPtr high_percentile_filter(new HighPercentileServerFilter());
    high_percentile_filter->Init();
    return FilterManager::GetInstance()->AddMessageServerFilter(high_percentile_filter);
  }
};

TEST_F(HighPercentileServerFilterTest, Init) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kHighPercentileName);
  ASSERT_NE(filter, nullptr);
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  ASSERT_EQ(filter->Name(), kHighPercentileName);
  ASSERT_EQ(points, (std::vector<FilterPoint>{
                        FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
                        FilterPoint::SERVER_POST_SCHED_RECV_MSG,
                        FilterPoint::SERVER_PRE_RPC_INVOKE,
                        FilterPoint::SERVER_POST_RPC_INVOKE,
                        FilterPoint::SERVER_POST_RECV_MSG,
                        FilterPoint::SERVER_PRE_SEND_MSG,
                    }));
}

TEST_F(HighPercentileServerFilterTest, OneWay) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kHighPercentileName);
  ASSERT_NE(filter, nullptr);

  ServerContextPtr context = MakeServerContext();
  auto req = std::make_shared<trpc::testing::TestProtocol>();
  context->SetRequestMsg(std::move(req));

  // One-way mode, no action is performed.
  context->SetCallType(kOnewayCall);
  FilterStatus status = FilterStatus::CONTINUE;
  filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

TEST_F(HighPercentileServerFilterTest, Operator) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kHighPercentileName);
  ASSERT_NE(filter, nullptr);

  ServerContextPtr context = MakeServerContext();
  auto req = std::make_shared<trpc::testing::TestProtocol>();
  context->SetRequestMsg(std::move(req));
  context->SetStatus(Status(-1, ""));
  context->SetCallType(kUnaryCall);

  FilterStatus status = FilterStatus::CONTINUE;
  filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  filter->operator()(status, FilterPoint::SERVER_PRE_SEND_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  filter->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

TEST_F(HighPercentileServerFilterTest, OnRequest) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kHighPercentileName);
  ASSERT_NE(filter, nullptr);

  ServerContextPtr context = MakeServerContext();
  auto req = std::make_shared<trpc::testing::TestProtocol>();
  context->SetRequestMsg(std::move(req));
  context->SetStatus(Status(0, ""));

  context->AddReqTransInfo("trpc-priority", "0");
  context->SetCallType(kUnaryCall);
  FilterStatus status = FilterStatus::CONTINUE;
  filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

TEST_F(HighPercentileServerFilterTest, OnResponse) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kHighPercentileName);
  ASSERT_NE(filter, nullptr);

  ServerContextPtr context = MakeServerContext();
  auto req = std::make_shared<trpc::testing::TestProtocol>();
  context->SetRequestMsg(std::move(req));
  context->SetStatus(Status(0, ""));

  context->AddReqTransInfo("trpc-priority", "0");
  context->SetCallType(kUnaryCall);
  FilterStatus status = FilterStatus::CONTINUE;
  filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  context->AddReqTransInfo("trpc-priority", "0");
  context->SetCallType(kUnaryCall);
  status = FilterStatus::CONTINUE;
  filter->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  filter->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
