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

#include "trpc/runtime/threadmodel/thread_model.h"

namespace trpc {

namespace testing {

class MockThreadModel : public ThreadModel {
 public:
  MOCK_CONST_METHOD0(GroupId, uint16_t());
  MOCK_CONST_METHOD0(Type, std::string());
  MOCK_CONST_METHOD0(GroupName, std::string_view());

  MOCK_METHOD(void, Start, (), (noexcept));
  MOCK_METHOD(void, Terminate, (), (noexcept));
};
}  // namespace testing
}  // namespace trpc
