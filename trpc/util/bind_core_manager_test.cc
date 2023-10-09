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

#include <algorithm>
#include <mutex>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/util/bind_core_manager.h"

TEST(BindCoreManagerTest, ParseBindCoreGroup) {
  const std::vector<uint32_t>& result = trpc::BindCoreManager::GetAffinity();

  ASSERT_FALSE(trpc::BindCoreManager::ParseBindCoreGroup(""));
  ASSERT_TRUE(result.empty());

  ASSERT_DEATH(trpc::BindCoreManager::ParseBindCoreGroup(" "), "");
  ASSERT_TRUE(result.empty());

  ASSERT_DEATH(trpc::BindCoreManager::ParseBindCoreGroup("xxx"), "");
  ASSERT_TRUE(result.empty());

  ASSERT_DEATH(trpc::BindCoreManager::ParseBindCoreGroup("1,xxx"), "");
  ASSERT_TRUE(result.empty());

  ASSERT_DEATH(trpc::BindCoreManager::ParseBindCoreGroup("1,1-xxx"), "");
  ASSERT_TRUE(result.empty());

  ASSERT_DEATH(trpc::BindCoreManager::ParseBindCoreGroup("1,xxx-1"), "");
  ASSERT_TRUE(result.empty());

  ASSERT_FALSE(trpc::BindCoreManager::ParseBindCoreGroup("1,1-2"));
  ASSERT_TRUE(result.empty());

  ASSERT_FALSE(trpc::BindCoreManager::ParseBindCoreGroup("1,0-2"));
  ASSERT_TRUE(result.empty());

  ASSERT_FALSE(trpc::BindCoreManager::ParseBindCoreGroup("9999999"));
  ASSERT_TRUE(result.empty());

  ASSERT_DEATH(trpc::BindCoreManager::ParseBindCoreGroup("1,3 -4"), "");
  ASSERT_TRUE(result.empty());

  ASSERT_DEATH(trpc::BindCoreManager::ParseBindCoreGroup("2,1,8-5"), "");
  ASSERT_TRUE(result.empty());

  ASSERT_TRUE(trpc::BindCoreManager::ParseBindCoreGroup("1,3-4,"));
  ASSERT_TRUE(std::equal(result.begin(), result.end(), (std::vector<unsigned>{1, 3, 4}).begin()));

  ASSERT_TRUE(trpc::BindCoreManager::ParseBindCoreGroup("1,3-5"));
  ASSERT_TRUE(std::equal(result.begin(), result.end(), (std::vector<unsigned>{1, 3, 4, 5}).begin()));

  ASSERT_TRUE(trpc::BindCoreManager::ParseBindCoreGroup("1,0,2-3"));
  ASSERT_TRUE(std::equal(result.begin(), result.end(), (std::vector<unsigned>{0, 1, 2, 3, 4}).begin()));
}

TEST(BindCoreManagerTest, BindCore) {
  std::thread t([] {
    ASSERT_TRUE(trpc::BindCoreManager::ParseBindCoreGroup("1,0,2-3"));
    ASSERT_EQ(trpc::BindCoreManager::BindCore(), 0);
    ASSERT_EQ(trpc::BindCoreManager::BindCore(), 1);
    ASSERT_EQ(trpc::BindCoreManager::BindCore(), 2);
    ASSERT_EQ(trpc::BindCoreManager::BindCore(), 3);
    ASSERT_EQ(trpc::BindCoreManager::BindCore(), 0);
    ASSERT_EQ(trpc::BindCoreManager::BindCore(), 1);
  });
  t.join();
}

TEST(BindCoreManagerTest, UserBindCore) {
  std::thread t([] {
    const std::vector<uint32_t>& result = trpc::BindCoreManager::GetAffinity();
    ASSERT_TRUE(trpc::BindCoreManager::UserParseBindCoreGroup("1,0,2-3"));
    ASSERT_TRUE(std::equal(result.begin(), result.end(), (std::vector<unsigned>{0, 1, 2, 3, 4}).begin()));
    ASSERT_EQ(trpc::BindCoreManager::UserBindCore(), 0);
    ASSERT_EQ(trpc::BindCoreManager::UserBindCore(), 1);
    ASSERT_EQ(trpc::BindCoreManager::UserBindCore(), 2);
    ASSERT_EQ(trpc::BindCoreManager::UserBindCore(), 3);
    ASSERT_EQ(trpc::BindCoreManager::UserBindCore(), 0);
    ASSERT_EQ(trpc::BindCoreManager::UserBindCore(), 1);
  });
  t.join();
}
