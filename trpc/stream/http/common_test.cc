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

#include "trpc/stream/http/common.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(CommonTest, GetHttpChunkHeader) {
  size_t http_context_len = 1024;
  std::string result = stream::HttpChunkHeader(http_context_len);
  ASSERT_EQ("400\r\n", result);
}

}  // namespace trpc::testing
