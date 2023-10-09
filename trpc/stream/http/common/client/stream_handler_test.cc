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

#include "trpc/stream/http/common/client/stream_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::testing {

using namespace trpc::stream;

class HttpCommonStreamHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StreamOptions options;
    stream_handler_ = MakeRefCounted<HttpClientCommonStreamHandler>(std::move(options));
  }
  void TearDown() override {}

 protected:
  RefPtr<HttpClientCommonStreamHandler> stream_handler_{nullptr};
};

NoncontiguousBuffer BuildBuffer(std::string buf) {
  NoncontiguousBufferBuilder builder;
  builder.Append(buf);
  return builder.DestructiveGet();
}

TEST_F(HttpCommonStreamHandlerTest, TestParseResponseStartLineOk) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("HTTP/1.0 202 OK2\r\n");
  ASSERT_EQ(stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 1);
  auto rsp_line = static_pointer_cast<HttpStreamStatusLine>(std::any_cast<HttpStreamFramePtr>(out.front()));
  ASSERT_EQ(rsp_line->GetMutableHttpStatusLine()->status_code, 202);
  ASSERT_EQ(rsp_line->GetMutableHttpStatusLine()->status_text, "OK2");
  ASSERT_EQ(rsp_line->GetMutableHttpStatusLine()->version, "1.0");
}

TEST_F(HttpCommonStreamHandlerTest, TestParseResponseStartLineFailed) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("xHTxTP/1x.0 2x02 OK2\r\n");
  ASSERT_EQ(stream_handler_->ParseMessage(&buf, &out), -1);
  ASSERT_EQ(out.size(), 0);
}

}  // namespace trpc::testing
