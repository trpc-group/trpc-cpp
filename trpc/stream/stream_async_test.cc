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

#include "trpc/stream/stream_async.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/serialization/serialization_factory.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/stream/testing/mock_stream_provider.h"
#include "trpc/stream/testing/stream.pb.h"

namespace trpc::testing {

// For testing purposes only.
using namespace ::trpc::test::helloworld;

serialization::Serialization* kPbSerialization = []() {
  serialization::Init();
  serialization::SerializationType pb = serialization::kPbType;

  return serialization::SerializationFactory::GetInstance()->Get(pb);
}();

class AsyncStreamTest : public ::testing::Test {
 public:
  using R = HelloReply;
  using W = HelloRequest;

  void SetUp() override {
    provider_ = CreateMockStreamReaderWriterProvider();
    content_codec_ = {
        .serialization = kPbSerialization,
        // protobuf.
        .content_type = serialization::kPbMessage,
    };
  }

  void TearDown() override { provider_.Reset(); }

 protected:
  MockStreamReaderWriterProviderPtr provider_;
  stream::MessageContentCodecOptions content_codec_;
};

TEST_F(AsyncStreamTest, Reader) {
  HelloRequest req;
  req.set_msg("nihao");
  NoncontiguousBuffer buff;
  ASSERT_TRUE(content_codec_.serialization->Serialize(content_codec_.content_type, &req, &buff));

  stream::AsyncReader<HelloRequest> reader(provider_, content_codec_);

  EXPECT_CALL(*provider_, AsyncRead(::testing::_))
      .WillOnce(  // get value
          ::testing::Return(::testing::ByMove(
              MakeReadyFuture<std::optional<NoncontiguousBuffer>>(std::make_optional(std::move(buff))))))
      .WillOnce(  // get eof
          ::testing::Return(::testing::ByMove(
              MakeReadyFuture<std::optional<NoncontiguousBuffer>>(std::optional<NoncontiguousBuffer>()))))
      .WillOnce(  // get exception
          ::testing::Return(::testing::ByMove(
              MakeExceptionFuture<std::optional<NoncontiguousBuffer>>(CommonException("invalid read")))));

  EXPECT_TRUE(reader.Read()
                  .Then([](std::optional<HelloRequest>&& req) {
                    EXPECT_TRUE(req);
                    EXPECT_EQ(req.value().msg(), "nihao");
                    return MakeReadyFuture<>();
                  })
                  .IsReady());

  EXPECT_TRUE(reader.Read()
                  .Then([](std::optional<HelloRequest>&& req) {
                    EXPECT_FALSE(req);
                    return MakeReadyFuture<>();
                  })
                  .IsReady());

  EXPECT_TRUE(reader.Read().IsFailed());

  EXPECT_CALL(*provider_, AsyncFinish())
      .WillOnce(  // once
          ::testing::Return(::testing::ByMove(MakeReadyFuture<>())))
      .WillOnce(  // get exception
          ::testing::Return(::testing::ByMove(MakeExceptionFuture<>(CommonException("invalid finish")))));

  EXPECT_TRUE(reader.Finish().IsReady());

  EXPECT_TRUE(reader.Finish().IsFailed());
}

MATCHER_P(ExpectedRsp, expected, "Serialized Buffer") { return FlattenSlow(arg) == FlattenSlow(expected); }

TEST_F(AsyncStreamTest, Writer) {
  HelloReply rsp;
  rsp.set_msg("nihao");
  NoncontiguousBuffer buff;
  ASSERT_TRUE(content_codec_.serialization->Serialize(content_codec_.content_type, &rsp, &buff));

  stream::AsyncWriter<HelloReply> writer(provider_, content_codec_);

  EXPECT_CALL(*provider_, AsyncWrite(ExpectedRsp(buff)))
      .WillOnce(  // get value
          ::testing::Return(::testing::ByMove(MakeReadyFuture<>())))
      .WillOnce(  // get exception
          ::testing::Return(::testing::ByMove(MakeExceptionFuture<>(CommonException("invalid write")))));

  EXPECT_TRUE(writer.Write(rsp).IsReady());

  EXPECT_TRUE(writer.Write(rsp).IsFailed());

  EXPECT_CALL(*provider_, AsyncFinish())
      .WillOnce(  // once
          ::testing::Return(::testing::ByMove(MakeReadyFuture<>())))
      .WillOnce(  // get exception
          ::testing::Return(::testing::ByMove(MakeExceptionFuture<>(CommonException("invalid finish")))));

  EXPECT_TRUE(writer.Finish().IsReady());

  EXPECT_TRUE(writer.Finish().IsFailed());
}

}  // namespace trpc::testing
