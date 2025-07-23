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

#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/util/log/default/default_log.h"
#include "trpc/util/log/log.h"

namespace trpc {

namespace testing {

class MockLog : public Log {
 public:
  void Start() override { ASSERT_TRUE(true); }

  void Stop() override { ASSERT_TRUE(true); }

  MOCK_CONST_METHOD2(ShouldLog, bool(const char*, Level));
  MOCK_CONST_METHOD6(LogIt, void(const char*, Level, const char*, int, const char*, std::string_view));

  MOCK_CONST_METHOD1(GetLevel, std::pair<Level, bool>(const char*));
  MOCK_METHOD2(SetLevel, std::pair<Level, bool>(const char*, Level level));
};

}  // namespace testing

}  // namespace trpc
