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

#include "trpc/admin/js/viz_min_js.h"

#include <string.h>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(VizMinJsTest, Test) {
  const char *js = admin::VizMinJs();
  EXPECT_GE(strlen(js), 100);
}

}  // namespace trpc::testing
