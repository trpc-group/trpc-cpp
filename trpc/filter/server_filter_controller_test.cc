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

#include "trpc/filter/server_filter_controller.h"

#include <iostream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/filter/server_filter_manager.h"
#include "trpc/filter/testing/server_filter_testing.h"
#include "trpc/server/server_context.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::testing {

TEST(ServerFilterController, AddMessageServerFilter) {
  // test the global filter
  std::vector<FilterPoint> points = {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE};
  auto server_filter = std::make_shared<MockServerFilter>();
  EXPECT_CALL(*server_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
  EXPECT_CALL(*server_filter, Name()).WillRepeatedly(::testing::Return("global_server_filter"));
  ASSERT_TRUE(trpc::ServerFilterManager::GetInstance()->AddMessageServerGlobalFilter(server_filter));
  // test duplicate addition
  ASSERT_FALSE(trpc::ServerFilterManager::GetInstance()->AddMessageServerGlobalFilter(server_filter));

  // test the service-level filter
  ServerFilterController controller;
  std::vector<FilterPoint> points1 = {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE};
  auto server_filter1 = std::make_shared<MockServerFilter>();
  EXPECT_CALL(*server_filter1, GetFilterPoint()).WillRepeatedly(::testing::Return(points1));
  // test duplicate addition
  {
    EXPECT_CALL(*server_filter1, Name()).WillRepeatedly(::testing::Return("global_server_filter"));
    ASSERT_FALSE(controller.AddMessageServerFilter(server_filter1));
  }
  // test no duplicate addition
  {
    EXPECT_CALL(*server_filter1, Name()).WillRepeatedly(::testing::Return("server_filter"));
    ASSERT_TRUE(controller.AddMessageServerFilter(server_filter1));
  }
  trpc::ServerFilterManager::GetInstance()->Clear();
}

TEST(ServerFilterController, RunMessageServerFilters) {
  // add global filter
  std::vector<FilterPoint> points = {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE};
  auto server_filter = std::make_shared<MockServerFilter>();
  EXPECT_CALL(*server_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
  EXPECT_CALL(*server_filter, Name()).WillRepeatedly(::testing::Return("global_server_filter"));
  trpc::ServerFilterManager::GetInstance()->AddMessageServerGlobalFilter(server_filter);

  // add service-level filter
  ServerFilterController filter_controller;
  std::vector<FilterPoint> points1 = {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE,
                                      FilterPoint::SERVER_PRE_SEND_MSG, FilterPoint::SERVER_POST_RECV_MSG};
  auto server_filter1 = std::make_shared<MockServerFilter>();
  EXPECT_CALL(*server_filter1, GetFilterPoint()).WillRepeatedly(::testing::Return(points1));
  EXPECT_CALL(*server_filter1, Name()).WillRepeatedly(::testing::Return("server_filter"));
  ASSERT_TRUE(filter_controller.AddMessageServerFilter(server_filter1));

  // 1. global filter executed failed
  {
    trpc::ServerContextPtr ctx = trpc::MakeRefCounted<trpc::ServerContext>();
    FilterStatus status = FilterStatus::REJECT;
    EXPECT_CALL(*server_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    FilterStatus ret = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::REJECT);

    int exec_index = ctx->GetFilterExecIndex(GetMatchPoint(FilterPoint::SERVER_POST_RPC_INVOKE));
    ASSERT_EQ(exec_index, 0);
  }

  // 2. global filter executed successfully, but the service-level filter executed failed
  {
    trpc::ServerContextPtr ctx = trpc::MakeRefCounted<trpc::ServerContext>();
    FilterStatus status = FilterStatus::CONTINUE;
    EXPECT_CALL(*server_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    FilterStatus status1 = FilterStatus::REJECT;
    EXPECT_CALL(*server_filter1, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status1), ::testing::Return()));

    FilterStatus ret = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::REJECT);

    int exec_index = ctx->GetFilterExecIndex(GetMatchPoint(FilterPoint::SERVER_POST_RPC_INVOKE));
    ASSERT_EQ(exec_index, 1);

    // execute the post-point of a filter that has already been successfully executed.
    FilterStatus status2 = FilterStatus::CONTINUE;
    EXPECT_CALL(*server_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(status2), ::testing::Return()));

    ret = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::CONTINUE);
  }

  // 3. all filters executed successfully
  {
    trpc::ServerContextPtr ctx = trpc::MakeRefCounted<trpc::ServerContext>();
    FilterStatus status = FilterStatus::CONTINUE;
    EXPECT_CALL(*server_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    EXPECT_CALL(*server_filter1, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::DoAll(::testing::SetArgReferee<0>(status), ::testing::Return()));

    FilterStatus ret = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::CONTINUE);

    //int exec_index = ctx->GetFilterExecIndex(GetMatchPoint(FilterPoint::SERVER_POST_RPC_INVOKE));
    // when executed successfully, exec_index == size of all filters
    //ASSERT_EQ(exec_index, 2);
    ret = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
    ASSERT_TRUE(ret == FilterStatus::CONTINUE);
  }
  trpc::ServerFilterManager::GetInstance()->Clear();
}

}  // namespace trpc::testing
