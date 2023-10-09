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

#include "trpc/stream/http/common/server/stream_handler.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"

namespace trpc::testing {

using namespace trpc::stream;

class HttpCommonStreamHandlerTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() { trpc::codec::Init(); }

  static void TearDownTestSuite() { trpc::codec::Destroy(); }

  void SetUp() override {
    StreamOptions options;
    stream_handler_ = MakeRefCounted<HttpServerCommonStreamHandler>(std::move(options));
  }
  void TearDown() override {}

 protected:
  RefPtr<HttpServerCommonStreamHandler> stream_handler_{nullptr};
};

NoncontiguousBuffer BuildBuffer(std::string buf) {
  NoncontiguousBufferBuilder builder;
  builder.Append(buf);
  return builder.DestructiveGet();
}

TEST_F(HttpCommonStreamHandlerTest, ParseResponseStartLineOk) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("POST /hello HTTP/1.0\r\n");
  ASSERT_EQ(stream_handler_->ParseMessage(&buf, &out), 0);
  ASSERT_EQ(out.size(), 1);
  auto req_line = static_pointer_cast<HttpStreamRequestLine>(std::any_cast<HttpStreamFramePtr>(out.front()));
  ASSERT_EQ(req_line->GetMutableHttpRequestLine()->method, "POST");
  ASSERT_EQ(req_line->GetMutableHttpRequestLine()->request_uri, "/hello");
  ASSERT_EQ(req_line->GetMutableHttpRequestLine()->version, "1.0");
}

TEST_F(HttpCommonStreamHandlerTest, ParseResponseStartLineFailed) {
  std::deque<std::any> out;
  NoncontiguousBuffer buf = BuildBuffer("PxOxSxT x/hello HTxTP/1x.0\r\n");
  ASSERT_EQ(stream_handler_->ParseMessage(&buf, &out), -1);
  ASSERT_EQ(out.size(), 0);
}

}  // namespace trpc::testing
