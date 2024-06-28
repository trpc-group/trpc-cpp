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

#include "trpc/codec/http/http_protocol.h"

#include <arpa/inet.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc.pb.h"

namespace trpc::testing {

class HttpProtoFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {}
  static void TearDownTestCase() {}

  virtual void SetUp() {}
};

TEST_F(HttpProtoFixture, HttpRequestProtocolTest) {
  std::string test("hello test");
  HttpRequestProtocol req = HttpRequestProtocol(std::make_shared<http::Request>());
  req.request->SetContent(test);
  NoncontiguousBuffer buff;
  ASSERT_FALSE(req.ZeroCopyDecode(buff));
  ASSERT_FALSE(req.ZeroCopyEncode(buff));
  ASSERT_NE(0, req.GetMessageSize());
}

TEST_F(HttpProtoFixture, HttpResponseProtocolTest) {
  std::string test("{\"age\":\"18\",\"height\":180}");
  HttpResponseProtocol rsp = HttpResponseProtocol();
  auto& reply = rsp.response;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetContent("{\"age\":\"18\",\"height\":180}");
  NoncontiguousBuffer buff;
  ASSERT_FALSE(rsp.ZeroCopyDecode(buff));
  ASSERT_FALSE(rsp.ZeroCopyEncode(buff));
  ASSERT_NE(0, rsp.GetMessageSize());
}

TEST(HttpRequestProtocolTest, GetOkNonContiguousProtocolBody) {
  std::string greetings = "Hello World";
  HttpRequestProtocol request_protocol{std::make_shared<http::HttpRequest>()};
  NoncontiguousBufferBuilder builder;
  builder.Append(greetings);
  request_protocol.SetNonContiguousProtocolBody(builder.DestructiveGet());

  auto body_buffer = request_protocol.GetNonContiguousProtocolBody();
  ASSERT_EQ(greetings.size(), body_buffer.ByteSize());
}

TEST(HttpRequestProtocolTest, GetEmptyNonContiguousProtocolBody) {
  HttpRequestProtocol request_protocol{std::make_shared<http::HttpRequest>()};
  auto body_buffer = request_protocol.GetNonContiguousProtocolBody();
  ASSERT_EQ(0, body_buffer.size());
}

TEST(HttpResponseProtocolTest, GetOkNonContiguousProtocolBody) {
  std::string greetings = "Hello World";
  HttpResponseProtocol response_protocol{};

  response_protocol.SetNonContiguousProtocolBody(CreateBufferSlow(greetings));

  ASSERT_EQ(greetings.size(), response_protocol.GetNonContiguousProtocolBody().ByteSize());
  ASSERT_TRUE(response_protocol.response.GetContent().empty());
}

TEST(EncodeTypeToMimeTest, EncodeTypeToMime) {
  struct testing_args_t {
    std::string name;
    int encode_type;
    std::string expect;
  };

  std::vector<testing_args_t> testings{
      {"pb encode", TrpcContentEncodeType::TRPC_PROTO_ENCODE, "application/proto"},
      {"json encode", TrpcContentEncodeType::TRPC_JSON_ENCODE, "application/json"},
      {"raw bytes stream, no operation", TrpcContentEncodeType::TRPC_NOOP_ENCODE, "application/octet-stream"},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, EncodeTypeToMime(t.encode_type));
  }
}

TEST(MimeToEncodeTypeTest, MimeToEncodeType) {
  struct testing_args_t {
    std::string name;
    std::string mime;
    int expect;
  };

  std::vector<testing_args_t> testings{
      {"proto mime", "application/proto", TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"proto mime", "application/pb", TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"proto mime", "application/protobuf", TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"json mime", "application/json", TrpcContentEncodeType::TRPC_JSON_ENCODE},
      {"octet-stream", "application/octet-stream", TrpcContentEncodeType::TRPC_NOOP_ENCODE},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, MimeToEncodeType(t.mime));
  }
}

}  // namespace trpc::testing
