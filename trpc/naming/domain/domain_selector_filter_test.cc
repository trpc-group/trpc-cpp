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

#include "trpc/naming/domain/domain_selector_filter.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/common/trpc_plugin.h"
#include "trpc/filter/filter.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/naming/common/common_defs.h"

namespace trpc {
class DomainSelectorFilterTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() { trpc::TrpcPlugin::GetInstance()->RegisterPlugins(); }

  static void TearDownTestCase() { trpc::TrpcPlugin::GetInstance()->UnregisterPlugins(); }

  virtual void SetUp() {
    selector_filter_ = trpc::FilterManager::GetInstance()->GetMessageClientFilter("domain");
    ASSERT_TRUE(selector_filter_ != nullptr);
    EXPECT_EQ(selector_filter_->Name(), "domain");
  }

  virtual void TearDown() {}

 protected:
  MessageClientFilterPtr selector_filter_;
};

TEST_F(DomainSelectorFilterTest, Type) {
  auto filter_type = selector_filter_->Type();
  EXPECT_EQ(filter_type, FilterType::kSelector);
}

TEST_F(DomainSelectorFilterTest, GetFilterPoint) {
  auto filter_points = selector_filter_->GetFilterPoint();
  ASSERT_EQ(filter_points.size(), 2);
  EXPECT_EQ(filter_points[0], FilterPoint::CLIENT_PRE_RPC_INVOKE);
  EXPECT_EQ(filter_points[1], FilterPoint::CLIENT_POST_RPC_INVOKE);
}

TEST_F(DomainSelectorFilterTest, operator_select_one) {
  auto context = trpc::MakeRefCounted<ClientContext>();
  ServiceProxyOption option;
  option.name = "test";
  option.selector_name = "domain";
  context->SetServiceProxyOption(&option);

  RouterInfo route_info;
  route_info.name = option.name;
  TrpcEndpointInfo endpoint;
  endpoint.host = "localhost";
  endpoint.port = 10001;
  route_info.info.emplace_back(endpoint);
  trpc::SelectorFactory::GetInstance()->Get(option.selector_name)->SetEndpoints(&route_info);

  // select test
  FilterStatus status;
  selector_filter_->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  EXPECT_EQ(status, FilterStatus::CONTINUE);
  EXPECT_EQ(context->GetIp(), "127.0.0.1");
  EXPECT_EQ(context->GetPort(), 10001);

  // report test
  selector_filter_->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  EXPECT_EQ(status, FilterStatus::CONTINUE);
}

}  // namespace trpc
