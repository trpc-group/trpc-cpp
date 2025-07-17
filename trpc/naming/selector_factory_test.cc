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

#include "trpc/naming/selector_factory.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/naming/testing/trpc_selector_testing.h"

namespace trpc::testing {

TEST(SelectorFactory, TestSelectorFactory) {
  SelectorPtr p = MakeRefCounted<TestTrpcSelector>();
  EXPECT_TRUE(p->Type() == PluginType::kSelector);

  SelectorFactory::GetInstance()->Register(p);

  SelectorPtr ret = SelectorFactory::GetInstance()->Get(p->Name());
  EXPECT_EQ(p.get(), ret.get());

  SelectorFactory::GetInstance()->Clear();
  ret = SelectorFactory::GetInstance()->Get(p->Name());
  EXPECT_EQ(nullptr, ret.Get());
}

}  // namespace trpc::testing
