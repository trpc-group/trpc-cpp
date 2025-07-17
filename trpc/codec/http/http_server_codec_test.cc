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

#include "trpc/codec/http/http_server_codec.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/http/http_protocol.h"
#include "trpc/server/server_context.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc::testing {

class HttpServerCodecTest : public ::testing::Test {
 protected:
  HttpServerCodec codec_;
};

void FillHttpRequest(http::RequestPtr& r) {
  r->SetMethodType(http::POST);
  r->SetUrl("/trpc.test.helloworld.Greeter/SayHello");
  r->SetVersion("1.1");
  r->SetHeader("trpc-call-type", std::to_string(kUnaryCall));
  r->SetHeader("trpc-request-id", std::to_string(1));
  r->SetHeader("trpc-timeout", std::to_string(1000));
  r->SetHeader("trpc-caller", "trpc.test.helloworld.client");
  r->SetHeader("trpc-callee", "trpc.test.helloworld.Greeter");
  r->SetHeader("trpc-message-type", std::to_string(TrpcMessageType::TRPC_DEFAULT));
  r->SetHeader("Content-Type", "application/json");
  // Add: trpc-trans-info: json
#ifdef TRPC_ENABLE_HTTP_TRANSINFO_BASE64
  r->SetHeader("trpc-trans-info", "{\"trpc-dyeing-key\": \"MTIz\",\"trpc-client-ip\": \"MC4wLjAuMA==\"}");
#else
  r->SetHeader("trpc-trans-info", "{\"trpc-dyeing-key\": \"123\",\"trpc-client-ip\": \"0.0.0.0\"}");
#endif
  r->SetContent("{\"msg\": \"helloworld\"}");
}

TEST_F(HttpServerCodecTest, Name) { ASSERT_EQ(kHttpCodecName, codec_.Name()); }

TEST_F(HttpServerCodecTest, Decode) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);

  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, context->GetRequestMsg()));
}

TEST_F(HttpServerCodecTest, Encode) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);

  ASSERT_EQ(trpc::compressor::kNone, context->GetReqCompressType());
  ASSERT_EQ(trpc::compressor::kNone, context->GetRspCompressType());

  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, context->GetRequestMsg()));
  dynamic_cast<HttpRequestProtocol*>(context->GetRequestMsg().get())->request = std::move(r);

  NoncontiguousBuffer out;
  ASSERT_TRUE(codec_.ZeroCopyEncode(context, context->GetResponseMsg(), out));
}

class TrpcOverHttpServerCodecTest : public ::testing::Test {
 protected:
  TrpcOverHttpServerCodec codec_;
};

TEST_F(TrpcOverHttpServerCodecTest, Name) { ASSERT_EQ(kTrpcOverHttpCodecName, codec_.Name()); }

TEST_F(TrpcOverHttpServerCodecTest, Decode) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);

  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, context->GetRequestMsg()));

  std::string http_header_trpc_trans_dyeing_key = "";
  auto it = context->GetPbReqTransInfo().find("trpc-dyeing-key");
  if (it != context->GetPbReqTransInfo().end()) {
    http_header_trpc_trans_dyeing_key = it->second;
  }
  ASSERT_STREQ(http_header_trpc_trans_dyeing_key.c_str(), "123");
}

TEST_F(TrpcOverHttpServerCodecTest, DecodeWithOkContentType) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);

  r->SetHeader("Content-Type", "application/json");
  ASSERT_EQ(r->GetHeader("Content-Type"), "application/json");
  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, context->GetRequestMsg()));

  r->SetHeader("Content-Type", "application/json; charset=utf-8");
  ASSERT_EQ(r->GetHeader("Content-Type"), "application/json; charset=utf-8");

  auto& out = context->GetRequestMsg();
  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, out));
  ASSERT_EQ(context->GetReqEncodeType(), TrpcContentEncodeType::TRPC_JSON_ENCODE);
  ASSERT_EQ(context->GetRspEncodeType(), TrpcContentEncodeType::TRPC_JSON_ENCODE);
}

TEST_F(TrpcOverHttpServerCodecTest, DecodeWithEmptyContentType) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);
  r->SetHeader("Content-Type", "");

  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, context->GetRequestMsg()));
  // Default value: 0
  ASSERT_EQ(context->GetReqEncodeType(), 0);
  ASSERT_EQ(context->GetRspEncodeType(), 0);
}

TEST_F(TrpcOverHttpServerCodecTest, DecodeWithFormContentType) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);
  r->SetHeader("Content-Type", "application/x-www-form-urlencoded");

  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, context->GetRequestMsg()));
  // Default value: 0
  ASSERT_EQ(context->GetReqEncodeType(), 0);
  ASSERT_EQ(context->GetRspEncodeType(), 0);
}

TEST_F(TrpcOverHttpServerCodecTest, Encode) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());
  context->AddRspTransInfo("trpc-dyeing-key", "123");

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);

  ASSERT_EQ(trpc::compressor::kNone, context->GetReqCompressType());
  ASSERT_EQ(trpc::compressor::kNone, context->GetRspCompressType());

  ASSERT_TRUE(codec_.ZeroCopyDecode(context, r, context->GetRequestMsg()));
  dynamic_cast<HttpRequestProtocol*>(context->GetRequestMsg().get())->request = std::move(r);

  NoncontiguousBuffer out;
  ASSERT_TRUE(codec_.ZeroCopyEncode(context, context->GetResponseMsg(), out));
  ASSERT_EQ(trpc::compressor::kNone, context->GetReqCompressType());
  ASSERT_EQ(trpc::compressor::kNone, context->GetRspCompressType());
  std::string expect_out =
      "HTTP/1.1 200 OK\r\n"
      "trpc-ret: 0\r\n"
      "Content-Type: application/json\r\n"
      "trpc-func-ret: 0\r\n"
      "Content-Length: 0\r\n"
      "trpc-call-type: 0\r\n"
      "trpc-error-msg: \r\n"
      "trpc-request-id: 1\r\n"
#ifdef TRPC_ENABLE_HTTP_TRANSINFO_BASE64
      "trpc-trans-info: {\"trpc-dyeing-key\":\"MTIz\"}\r\n"
#else
      "trpc-trans-info: {\"trpc-dyeing-key\":\"123\"}\r\n"
#endif
      "trpc-message-type: 0\r\n"
      "\r\n";
  ASSERT_EQ(expect_out, FlattenSlow(out)) << "xxx compress type:" << context->GetReqCompressType();
}

TEST_F(TrpcOverHttpServerCodecTest, EncodeWithUserDefinedHttpHeaders) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(codec_.CreateRequestObject());
  context->SetResponseMsg(codec_.CreateResponseObject());

  http::RequestPtr r = std::make_shared<http::Request>();
  FillHttpRequest(r);

  dynamic_cast<HttpRequestProtocol*>(context->GetRequestMsg().get())->request = r;
  dynamic_cast<HttpResponseProtocol*>(context->GetResponseMsg().get())
      ->SetNonContiguousProtocolBody(CreateBufferSlow(r->GetContent()));

  std::string k1 = "Access-Control-Allow-Origin", v1 = "http://foo.example";
  std::string k2 = "X-Greetings", v2 = "Hello World";
  context->AddRspTransInfo(k1, v1);
  context->AddRspTransInfo(k2, v2);

  NoncontiguousBuffer out;
  ASSERT_TRUE(codec_.ZeroCopyEncode(context, context->GetResponseMsg(), out));
}

// Requests with compression.
TEST_F(TrpcOverHttpServerCodecTest, ContentEncodingTypeTest) {
  auto DoContentEncodingTypeTest = [](HttpServerCodec& codec, ::trpc::compressor::CompressType compress_type,
                                      ::trpc::compressor::CompressType accept_compress_type, std::string compress_word,
                                      std::string accept_compress_word, std::string check_key_word) {
    ServerContextPtr context = MakeRefCounted<ServerContext>();
    context->SetRequestMsg(codec.CreateRequestObject());
    context->SetResponseMsg(codec.CreateResponseObject());

    http::RequestPtr r = std::make_shared<http::Request>();
    r->SetHeader("Content-Encoding", compress_word);
    r->SetHeader("Accept-Encoding", accept_compress_word);
    NoncontiguousBuffer out;
    ASSERT_TRUE(codec.ZeroCopyDecode(context, r, context->GetRequestMsg()));
    ASSERT_EQ(context->GetReqCompressType(), compress_type);
    ASSERT_EQ(context->GetRspCompressType(), accept_compress_type);
    ASSERT_TRUE(codec.ZeroCopyEncode(context, context->GetResponseMsg(), out));
    std::string rsp_binary = FlattenSlow(out);
    if (check_key_word == "no_compress") {
      ASSERT_TRUE(rsp_binary.find("Content-Encoding") == std::string::npos);
    } else {
      ASSERT_TRUE(rsp_binary.find("Content-Encoding: " + check_key_word) != std::string::npos);
    }
  };

  // Compresses request, bud doesn't compress response.
  DoContentEncodingTypeTest(codec_, ::trpc::compressor::kNone, ::trpc::compressor::kNone, "identity", "identity",
                            "no_compress");
  // Compresses request and response with gzip.
  DoContentEncodingTypeTest(codec_, ::trpc::compressor::kGzip, ::trpc::compressor::kGzip, "gzip", "gzip", "gzip");
  // Compresses request and response with zlib.
  DoContentEncodingTypeTest(codec_, ::trpc::compressor::kZlib, ::trpc::compressor::kZlib, "deflate", "deflate",
                            "deflate");
  // Compresses request and response with snappy.
  DoContentEncodingTypeTest(codec_, ::trpc::compressor::kSnappy, ::trpc::compressor::kSnappy, "snappy", "snappy",
                            "snappy");
  // Fails to compresses request and doesn't compress response due to bad compression type in request and response.
  DoContentEncodingTypeTest(codec_, ::trpc::compressor::kMaxType, ::trpc::compressor::kNone,
                            "not-exist-compress-algorithm", "not-exist-compress-algorithm", "no_compress");

  // Doesn't to compresses request and compresses response with gzip due to there is compression type in request
  // but gzip type in  response.
  DoContentEncodingTypeTest(codec_, ::trpc::compressor::kNone, ::trpc::compressor::kGzip, "", "gzip", "gzip");
  // Doesn't to compresses request and compresses response with gzip due to there is compression type in request
  // but gzip type("compress1, identity, gzip , snappy") in  response.
  DoContentEncodingTypeTest(codec_, ::trpc::compressor::kNone, ::trpc::compressor::kGzip, "",
                            "compress1, identity, gzip , snappy", "gzip");
}

// Error code converts to HTTP status.
TEST_F(TrpcOverHttpServerCodecTest, ErrorStatusTest) {
  auto DoErrorStatusTest = [](HttpServerCodec codec, ::trpc::Status status, int http_status) {
    ServerContextPtr context = MakeRefCounted<ServerContext>();
    context->SetRequestMsg(codec.CreateRequestObject());
    context->SetResponseMsg(codec.CreateResponseObject());

    http::RequestPtr r = std::make_shared<http::Request>();
    NoncontiguousBuffer out;

    ASSERT_TRUE(codec.ZeroCopyDecode(context, r, context->GetRequestMsg()));
    context->SetStatus(status);
    ASSERT_TRUE(codec.ZeroCopyEncode(context, context->GetResponseMsg(), out));
    std::string rsp_binary = FlattenSlow(out);
    ASSERT_TRUE(rsp_binary.find("HTTP/1.1 " + std::to_string(http_status)) == std::string::npos);
  };

  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_DECODE_ERR, ""), 400);
  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_ENCODE_ERR, ""), 500);
  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_AUTH_ERR, ""), 500);
  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, ""), 500);
  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_LIMITED_ERR, ""), 500);
  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_NOFUNC_ERR, ""), 404);
  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_TIMEOUT_ERR, ""), 504);
  DoErrorStatusTest(codec_, ::trpc::Status(::trpc::TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, ""), 429);
}

}  // namespace trpc::testing
