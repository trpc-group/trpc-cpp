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

#pragma once

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/filter/client_filter_base.h"

namespace trpc::testing {

class MockClientFilter : public MessageClientFilter {
 public:
  MOCK_METHOD0(Name, std::string());
  MOCK_METHOD0(Type, FilterType());
  MOCK_METHOD0(GetFilterPoint, std::vector<FilterPoint>());
  MOCK_METHOD3(BracketOp, void(FilterStatus&, FilterPoint, const ClientContextPtr&));
  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override {
    BracketOp(status, point, context);
  }
};

}  // namespace trpc::testing
