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

#include "trpc/filter/client_filter_manager.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/filter/filter_controller.h"
#include "trpc/filter/testing/client_filter_testing.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::testing {

TEST(ClientFilterManager, All) {
  trpc::ClientFilterManager* manager = trpc::ClientFilterManager::GetInstance();

  // Test add and get client filter by name
  std::vector<FilterPoint> points1 = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  auto client_filter = std::make_shared<MockClientFilter>();
  ASSERT_EQ(0, client_filter->Init());
  EXPECT_CALL(*client_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points1));
  EXPECT_CALL(*client_filter, Name()).WillRepeatedly(::testing::Return("client_filter1"));
  manager->AddMessageClientFilter(client_filter);
  ASSERT_TRUE(manager->GetMessageClientFilter("client_filter_1") == nullptr);
  ASSERT_TRUE(manager->GetMessageClientFilter("client_filter1") != nullptr);
  ASSERT_TRUE(manager->GetAllMessageClientFilters().size() == 1);

  // Test add and get client global filter
  auto global_client_filter = std::make_shared<MockClientFilter>();
  ASSERT_EQ(0, global_client_filter->Init());
  std::vector<FilterPoint> points2 = {
      FilterPoint::CLIENT_POST_RECV_MSG,
      FilterPoint::CLIENT_PRE_SEND_MSG,
      FilterPoint::CLIENT_PRE_SCHED_SEND_MSG,
      FilterPoint::CLIENT_POST_SCHED_SEND_MSG,
  };
  EXPECT_CALL(*global_client_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points2));
  manager->AddMessageClientGlobalFilter(global_client_filter);
  ASSERT_TRUE(manager->GetMessageClientGlobalFilter(points2[0]).size() == 1);

  manager->Clear();
  ASSERT_TRUE(manager->GetMessageClientGlobalFilter(points2[0]).size() == 0);
}

}  // namespace trpc::testing
