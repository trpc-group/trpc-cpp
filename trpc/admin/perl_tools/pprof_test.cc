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

#include <gtest/gtest.h>
#include <string.h>

#include "trpc/admin/perl_tools/pprof.h"

namespace trpc {

namespace testing {

TEST(PprofTest, Test) {
  std::string cont;
  admin::PprofPerl(&cont);
  EXPECT_GE(cont.size(), 100);
}

}  // namespace testing

}  // namespace trpc
