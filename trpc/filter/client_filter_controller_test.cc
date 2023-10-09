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

#include "trpc/filter/client_filter_controller.h"

#include <iostream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/filter/client_filter_manager.h"
#include "trpc/filter/testing/client_filter_testing.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::testing {

TEST(ClientFilterController, AddMessageClientFilter) {
  // test the global filter
  std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  auto client_filter = std::make_shared<MockClientFilter>();
  ASSERT_EQ(0, client_filter->Init());
  EXPECT_CALL(*client_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
  EXPECT_CALL(*client_filter, Name()).WillRepeatedly(::testing::Return("global_client_filter"));
  ASSERT_TRUE(trpc::ClientFilterManager::GetInstance()->AddMessageClientGlobalFilter(client_filter));
  // test duplicate addition
  ASSERT_FALSE(trpc::ClientFilterManager::GetInstance()->AddMessageClientGlobalFilter(client_filter));

  // test the service-level filter
  ClientFilterController controller;
  std::vector<FilterPoint> points1 = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  auto client_filter1 = std::make_shared<MockClientFilter>();
  ASSERT_EQ(0, client_filter1->Init());
  EXPECT_CALL(*client_filter1, GetFilterPoint()).WillRepeatedly(::testing::Return(points1));
  // test duplicate addition
  {
    EXPECT_CALL(*client_filter1, Name()).WillRepeatedly(::testing::Return("global_client_filter"));
    ASSERT_FALSE(controller.AddMessageClientFilter(client_filter1));
  }
  // test no duplicate addition
  {
    EXPECT_CALL(*client_filter1, Name()).WillRepeatedly(::testing::Return("client_filter"));
    ASSERT_TRUE(controller.AddMessageClientFilter(client_filter1));
  }
  trpc::ClientFilterManager::GetInstance()->Clear();
}

TEST(ClientFilterController, RunMessageClientFilters) {
  // add global filter
  std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  auto client_filter = std::make_shared<MockClientFilter>();
  ASSERT_EQ(0, client_filter->Init());
  EXPECT_CALL(*client_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
  EXPECT_CALL(*client_filter, Name()).WillRepeatedly(::testing::Return("global_client_filter"));
  trpc::ClientFilterManager::GetInstance()->AddMessageClientGlobalFilter(client_filter);

  // add service-level filter
  ClientFilterController filter_controller;
  std::vector<FilterPoint> points1 = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE,
                                      FilterPoint::CLIENT_PRE_SEND_MSG, FilterPoint::CLIENT_POST_RECV_MSG};
  auto client_filter1 = std::make_shared<MockClientFilter>();
  ASSERT_EQ(0, client_filter1->Init());
  EXPECT_CALL(*client_filter1, GetFilterPoint()).WillRepeatedly(::testing::Return(points1));
  EXPECT_CALL(*client_filter1, Name()).WillRepeatedly(::testing::Return("client_filter"));
  ASSERT_TRUE(filter_controller.AddMessageClientFilter(client_filter1));

  auto trpc_codec = std::make_shared<trpc::TrpcClientCodec>();
  // 1. global filter executed failed
  {
    trpc::ClientContextPtr ctx = trpc::MakeRefCounted<trpc::ClientContext>(trpc_codec);
    FilterStatus status = FilterStatus::REJECT;
    EXPECT_CALL(*client_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    FilterStatus ret = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::REJECT);

    int exec_index = ctx->GetFilterExecIndex(GetMatchPoint(FilterPoint::CLIENT_POST_RPC_INVOKE));
    ASSERT_EQ(exec_index, 0);
  }
  // 2. global filter executed successfully, but the service-level filter executed failed
  {
    trpc::ClientContextPtr ctx = trpc::MakeRefCounted<trpc::ClientContext>(trpc_codec);
    FilterStatus status = FilterStatus::CONTINUE;
    EXPECT_CALL(*client_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    FilterStatus status1 = FilterStatus::REJECT;
    EXPECT_CALL(*client_filter1, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status1), ::testing::Return()));

    FilterStatus ret = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::REJECT);

    int exec_index = ctx->GetFilterExecIndex(GetMatchPoint(FilterPoint::CLIENT_POST_RPC_INVOKE));
    ASSERT_EQ(exec_index, 1);

    // execute the post-point of a filter that has already been successfully executed.
    FilterStatus status2 = FilterStatus::CONTINUE;
    EXPECT_CALL(*client_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status2), ::testing::Return()));

    ret = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::CONTINUE);
  }
  // 3. all filters executed successfully
  {
    trpc::ClientContextPtr ctx = trpc::MakeRefCounted<trpc::ClientContext>(trpc_codec);
    FilterStatus status = FilterStatus::CONTINUE;
    EXPECT_CALL(*client_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    EXPECT_CALL(*client_filter1, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    FilterStatus ret = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::CONTINUE);

    //int exec_index = ctx->GetFilterExecIndex(GetMatchPoint(FilterPoint::CLIENT_POST_RPC_INVOKE));
    // when executed successfully, exec_index == size of all filters
    //ASSERT_EQ(exec_index, 2);
    ret = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::CONTINUE);
  }
  trpc::ClientFilterManager::GetInstance()->Clear();
}

}  // namespace trpc::testing
