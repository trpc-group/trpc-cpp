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

#include "trpc/codec/grpc/http2/request.h"

#include <utility>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::testing {

class Http2RequestTest : public ::testing::Test {
 protected:
  void SetUp() override { request_ = http2::CreateRequest(); }

  void TearDown() override {}

 protected:
  http2::RequestPtr request_{nullptr};
};

TEST_F(Http2RequestTest, ReadContentOk) {
  std::array<uint8_t, 1024> read_buffer{};
  ASSERT_TRUE(request_!=nullptr);
  // Content is empty, read EOF
  ssize_t read_len = request_->ReadContent(read_buffer.data(), read_buffer.size());
  EXPECT_EQ(0, read_len);

  // Set content
  std::vector<uint8_t> hello_world{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
  for (ssize_t content_len = 0;;) {
    request_->OnData(hello_world.data(), hello_world.size());
    content_len += hello_world.size();

    if (content_len > 4096) {
      // EOF
      request_->OnData(nullptr, 0);
      break;
    }
  }

  int64_t content_length = request_->ContentLength();
  ssize_t read_bytes{0};
  for (;;) {
    ssize_t read_len = request_->ReadContent(read_buffer.data(), read_buffer.size());
    if (read_len > 0) {
      read_bytes += read_len;
    } else if (read_len == 0) {
      // EOF
      break;
    } else {
      // Error
      break;
    }
  }

  EXPECT_EQ(content_length, read_bytes);
  EXPECT_EQ(0, request_->ContentLength());
}

TEST_F(Http2RequestTest, OnResponseReadyOK) {
  bool response_ready_flag{false};
  auto on_response_ready_cb = [&response_ready_flag](http2::ResponsePtr&&) { response_ready_flag = true; };

  request_->SetOnResponseReadyCallback(std::move(on_response_ready_cb));
  request_->OnResponseReady(nullptr);

  EXPECT_TRUE(response_ready_flag);
}

}  // namespace trpc::testing
