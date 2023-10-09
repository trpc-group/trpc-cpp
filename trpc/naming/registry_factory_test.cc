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

#include "trpc/naming/registry_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/naming/testing/trpc_registry_testing.h"

namespace trpc::testing {

TEST(RegistryFactory, TestRegistryFactory) {
  RegistryPtr p = MakeRefCounted<TestTrpcRegistry>();
  EXPECT_TRUE(p->Type() == PluginType::kRegistry);

  RegistryFactory::GetInstance()->Register(p);

  RegistryPtr ret = RegistryFactory::GetInstance()->Get(p->Name());
  EXPECT_EQ(p.get(), ret.get());

  RegistryFactory::GetInstance()->Clear();
  ret = RegistryFactory::GetInstance()->Get(p->Name());
  EXPECT_EQ(nullptr, ret.Get());
}

}  // namespace trpc::testing
