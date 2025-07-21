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

#include "trpc/filter/server_filter_manager.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/filter/testing/server_filter_testing.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::testing {

TEST(ServerFilterManager, All) {
  trpc::ServerFilterManager* manager = trpc::ServerFilterManager::GetInstance();

  // Test add and get server filter by name
  std::vector<FilterPoint> points1 = {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE};
  auto server_filter = std::make_shared<MockServerFilter>();
  EXPECT_CALL(*server_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points1));
  EXPECT_CALL(*server_filter, Name()).WillRepeatedly(::testing::Return("server_filter1"));
  manager->AddMessageServerFilter(server_filter);
  ASSERT_TRUE(manager->GetMessageServerFilter("server_filter_1") == nullptr);
  ASSERT_TRUE(manager->GetMessageServerFilter("server_filter1") != nullptr);
  ASSERT_TRUE(manager->GetAllMessageServerFilters().size() == 1);

  // Test add and get server global filter
  auto global_server_filter = std::make_shared<MockServerFilter>();
  std::vector<FilterPoint> points2 = {
      FilterPoint::SERVER_POST_RECV_MSG,
      FilterPoint::SERVER_PRE_SEND_MSG,
      FilterPoint::SERVER_PRE_RPC_INVOKE,
      FilterPoint::SERVER_POST_RPC_INVOKE,
  };
  EXPECT_CALL(*global_server_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points2));
  manager->AddMessageServerGlobalFilter(global_server_filter);
  ASSERT_TRUE(manager->GetMessageServerGlobalFilter(points2[0]).size() == 1);

  manager->Clear();
  ASSERT_TRUE(manager->GetMessageServerGlobalFilter(points2[0]).size() == 0);
}

}  // namespace trpc::testing
