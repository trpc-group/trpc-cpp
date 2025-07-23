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

#include "trpc/util/http/mime_types.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(MimeType, ExtensionToType) {
  EXPECT_EQ("application/json", trpc::http::ExtensionToType("json"));
  EXPECT_EQ("image/jpeg", trpc::http::ExtensionToType("jpg"));
  EXPECT_EQ("application/octet-stream", trpc::http::ExtensionToType("bin"));
  EXPECT_EQ("application/proto", trpc::http::ExtensionToType("proto"));
  EXPECT_EQ("text/plain", trpc::http::ExtensionToType("not exist"));
}

}  // namespace trpc::testing
