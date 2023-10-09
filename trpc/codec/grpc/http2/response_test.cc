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

#include "trpc/codec/grpc/http2/response.h"

#include <vector>

#include "gtest/gtest.h"

namespace trpc::testing {

class Http2ResponseTest : public ::testing::Test {
 protected:
  void SetUp() override { response_ = http2::CreateResponse(); }

  void TearDown() override {}

 protected:
  http2::ResponsePtr response_{nullptr};
};

TEST_F(Http2ResponseTest, OnDataOk) {
  std::array<uint8_t, 1024> read_buffer{};
  // Content is empty, read EOF
  ssize_t read_len = response_->ReadContent(read_buffer.data(), read_buffer.size());
  EXPECT_EQ(0, read_len);

  std::vector<uint8_t> hello_world{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
  size_t content_len{0};

  for (;;) {
    response_->OnData(hello_world.data(), hello_world.size());
    content_len += hello_world.size();

    if (content_len > 4096) {
      // EOF
      response_->OnData(nullptr, 0);
      break;
    }
  }

  EXPECT_EQ(content_len, response_->ContentLength());
}

}  // namespace trpc::testing
