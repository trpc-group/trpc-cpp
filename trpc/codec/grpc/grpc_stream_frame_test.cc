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

#include "trpc/codec/grpc/grpc_stream_frame.h"

#include "gtest/gtest.h"

namespace trpc::testing {

using namespace trpc::stream;

TEST(GrpcStreamFrameTest, InitFrameTest) {
  uint32_t stream_id = 123;
  http2::RequestPtr req = http2::CreateRequest();
  GrpcStreamInitFrame frame(stream_id, req);
  ASSERT_EQ(StreamMessageCategory::kStreamInit, frame.GetFrameType());
  ASSERT_TRUE(frame.GetRequest());
  ASSERT_EQ(stream_id, frame.GetStreamId());
}

TEST(GrpcStreamFrameTest, DataFrameTest) {
  uint32_t stream_id = 123;

  const char msg[] = "hello";
  NoncontiguousBufferBuilder builder;
  builder.Append(msg, strlen(msg));
  NoncontiguousBuffer buffer = builder.DestructiveGet();

  GrpcStreamDataFrame frame(stream_id, std::move(buffer));
  ASSERT_EQ(StreamMessageCategory::kStreamData, frame.GetFrameType());
  NoncontiguousBuffer data = frame.GetData();
  ASSERT_EQ(data.ByteSize(), strlen(msg));
  ASSERT_EQ(stream_id, frame.GetStreamId());
}

TEST(GrpcStreamFrameTest, CloseFrameTest) {
  uint32_t stream_id = 123;
  GrpcStreamCloseFrame frame(stream_id);
  ASSERT_EQ(StreamMessageCategory::kStreamClose, frame.GetFrameType());
  ASSERT_EQ(stream_id, frame.GetStreamId());
}

TEST(GrpcStreamFrameTest, ResetFrameTest) {
  uint32_t stream_id = 123;
  uint32_t error_code = 456;
  GrpcStreamResetFrame frame(stream_id, error_code);
  ASSERT_EQ(StreamMessageCategory::kStreamReset, frame.GetFrameType());
  ASSERT_EQ(frame.GetErrorCode(), error_code);
  ASSERT_EQ(stream_id, frame.GetStreamId());
}

}  // namespace trpc::testing
