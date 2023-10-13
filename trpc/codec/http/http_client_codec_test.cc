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

#include "trpc/codec/http/http_client_codec.h"

#include <list>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/codec/http/http_protocol.h"
#include "trpc/compressor/compressor.h"
#include "trpc/compressor/compressor_factory.h"
#include "trpc/compressor/testing/compressor_testing.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc::testing {

namespace {
// For testing purposes only.
using trpc::compressor::CompressorFactory;
using trpc::compressor::testing::MockCompressor;

}  // namespace

class HttpClientCodecTest : public ::testing::Test {
 public:
  static void SetUpTestCase() { ASSERT_TRUE(trpc::serialization::Init()); }

  static void TearDownTestCase() { trpc::serialization::Destroy(); }

 protected:
  HttpClientCodec codec_;
  TrpcOverHttpClientCodec trpc_over_http_codec_;
};

TEST_F(HttpClientCodecTest, Name) {
  ASSERT_EQ(kHttpCodecName, codec_.Name());
  ASSERT_EQ(kTrpcOverHttpCodecName, trpc_over_http_codec_.Name());
}

TEST_F(HttpClientCodecTest, Decode) {
  http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetContent("{\"age\":\"18\",\"height\":180}");

  ClientContextPtr ctx = MakeRefCounted<ClientContext>();
  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  ASSERT_TRUE(codec_.ZeroCopyDecode(ctx, reply, rsp_protocol));
  auto ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  ASSERT_EQ(ptr->response.GetVersion(), reply.GetVersion());
  ASSERT_EQ(ptr->response.GetStatus(), reply.GetStatus());
  ASSERT_EQ(ptr->response.GetContent(), reply.GetContent());
}

TEST_F(HttpClientCodecTest, DecodeFail) {
  int in = 0;
  ProtocolPtr rsp = codec_.CreateResponsePtr();
  ASSERT_FALSE(codec_.ZeroCopyDecode(nullptr, in, rsp));
}

TEST_F(HttpClientCodecTest, EncodeWithContext) {
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();
  HttpRequestProtocol* req = static_cast<HttpRequestProtocol*>(req_protocol.get());

  auto& r = *req->request;
  r.SetMethodType(http::GET);
  r.SetUrl("/test");
  r.SetVersion("1.1");
  r.AddHeader("Key1", "value1");
  r.AddHeader("Key2", "value2");

  NoncontiguousBuffer out;

  ClientContextPtr ctx = MakeRefCounted<ClientContext>();
  ctx->SetAddr("127.0.0.1", 80);
  ASSERT_TRUE(codec_.ZeroCopyEncode(ctx, req_protocol, out));

  ASSERT_EQ(
      "GET /test HTTP/1.1\r\n"
      "Host: 127.0.0.1:80\r\n"
      "Key1: value1\r\n"
      "Key2: value2\r\n\r\n",
      FlattenSlow(out, out.ByteSize()));
}

TEST_F(HttpClientCodecTest, FillRequestBasic) {
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetRequest(req_protocol);
  std::string test_url = "/testfuncname";
  context->SetFuncName(test_url);
  //  std::string test_k1 = "k1", test_v1 = "v1";
  //  std::string test_k2 = "k2", test_v2 = "v2";
  //  context->SetHttpHeader(test_k1, test_v1);
  //  context->SetHttpHeader(test_k2, test_v2);

  test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("hello");

  ASSERT_TRUE(codec_.FillRequest(context, req_protocol, reinterpret_cast<void*>(&hello_req)));

  HttpRequestProtocol* req_ptr = static_cast<HttpRequestProtocol*>(req_protocol.get());
  ASSERT_EQ(req_ptr->request->GetMethodType(), http::OperationType::POST);
  ASSERT_EQ(req_ptr->request->GetVersion(), "1.1");
  ASSERT_EQ(req_ptr->request->GetUrl(), test_url);
  //  ASSERT_TRUE(req_ptr->request->HasHeader(test_k1));
  //  ASSERT_EQ(req_ptr->request->GetHeader(test_k1), test_v1);
  //  ASSERT_TRUE(req_ptr->request->HasHeader(test_k2));
  //  ASSERT_EQ(req_ptr->request->GetHeader(test_k2), test_v2);
}

TEST_F(HttpClientCodecTest, FillRequestAsPB) {
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetRequest(req_protocol);
  context->SetReqEncodeType(serialization::kPbType);
  std::string test_k = "k", test_v = "v";
  context->AddReqTransInfo(test_k, test_v);

  test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("hello");

  ASSERT_TRUE(codec_.FillRequest(context, req_protocol, reinterpret_cast<void*>(&hello_req)));

  HttpRequestProtocol* req_ptr = static_cast<HttpRequestProtocol*>(req_protocol.get());
  auto body = req_ptr->request->GetNonContiguousBufferContent();
  ASSERT_EQ(body.ByteSize(), hello_req.ByteSizeLong());
  ASSERT_EQ(req_ptr->request->GetHeader("Content-Length"), std::to_string(hello_req.ByteSizeLong()));
  ASSERT_TRUE(req_ptr->request->HasHeader("Content-Type"));
  ASSERT_EQ(req_ptr->request->GetHeader("Content-Type"), "application/pb");
#ifdef TRPC_ENABLE_HTTP_TRANSINFO_BASE64
  ASSERT_EQ(req_ptr->request->GetHeader("trpc-trans-info"), "{\"k\":\"dg==\"}");
#else
  ASSERT_EQ(req_ptr->request->GetHeader("trpc-trans-info"), "{\"k\":\"v\"}");
#endif
}

TEST_F(HttpClientCodecTest, FillRequestAsJson) {
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetRequest(req_protocol);
  context->SetReqEncodeType(serialization::kJsonType);
  context->SetReqEncodeDataType(serialization::kRapidJson);

  rapidjson::Document json;
  std::string request_json = "{\"age\":\"18\",\"height\":180}";
  json.Parse(request_json);

  ASSERT_TRUE(codec_.FillRequest(context, req_protocol, reinterpret_cast<void*>(&json)));

  HttpRequestProtocol* req_ptr = static_cast<HttpRequestProtocol*>(req_protocol.get());
  ASSERT_TRUE(!req_ptr->request->GetContent().empty());
  ASSERT_EQ(req_ptr->request->GetHeader("Content-Length"), std::to_string(request_json.size()));
  ASSERT_TRUE(req_ptr->request->HasHeader("Content-Type"));
  ASSERT_EQ(req_ptr->request->GetHeader("Content-Type"), "application/json");
  ASSERT_TRUE(req_ptr->request->HasHeader("Accept"));
  ASSERT_EQ(req_ptr->request->GetHeader("Accept"), "application/json");
}

TEST_F(HttpClientCodecTest, FillRequestAsString) {
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetRequest(req_protocol);
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kStringNoop);

  std::string request_string = "this is request string";

  ASSERT_TRUE(codec_.FillRequest(context, req_protocol, reinterpret_cast<void*>(&request_string)));

  HttpRequestProtocol* req_ptr = static_cast<HttpRequestProtocol*>(req_protocol.get());
  ASSERT_TRUE(!req_ptr->request->GetContent().empty());
  ASSERT_EQ(req_ptr->request->GetHeader("Content-Length"), std::to_string(request_string.size()));
  ASSERT_EQ(req_ptr->request->GetHeader("Content-Type"), "application/octet-stream");
  ASSERT_EQ(req_ptr->request->GetHeader("Accept"), "application/octet-stream");
}

TEST_F(HttpClientCodecTest, FillRequestAsUnSupport) {
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetRequest(req_protocol);
  context->SetReqEncodeType(serialization::kMaxType);

  std::string request_string = "this is invalid encode type";

  ASSERT_FALSE(codec_.FillRequest(context, req_protocol, reinterpret_cast<void*>(&request_string)));
}

TEST_F(HttpClientCodecTest, FillResponseAsPB) {
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kPbType);

  // In unit tests, RspEncodeType needs to be set here;
  // Normally, the codec is not involved, and RspEncodeType is automatically set by the RPC interface.
  context->SetRspEncodeType(serialization::kPbType);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  rsp.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  test::helloworld::HelloReply hello_reply;
  hello_reply.set_msg("hello");

  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);
  {
    NoncontiguousBufferBuilder builder;
    NoncontiguousBufferOutputStream nbos(&builder);
    auto ret = hello_reply.SerializePartialToZeroCopyStream(&nbos);
    ASSERT_EQ(ret, true);
    nbos.Flush();
    rsp_ptr->SetNonContiguousProtocolBody(builder.DestructiveGet());
  }

  test::helloworld::HelloReply reply;
  ASSERT_TRUE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));
  ASSERT_EQ(reply.msg(), "hello");
}

TEST_F(HttpClientCodecTest, FillResponseAsJSON) {
  std::string str{"{\"age\":\"18\",\"height\":180}"};

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kJsonType);
  context->SetReqEncodeDataType(serialization::kRapidJson);

  context->SetRspEncodeType(serialization::kJsonType);
  context->SetRspEncodeDataType(serialization::kRapidJson);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  rsp.SetStatus(trpc::http::HttpResponse::StatusCode::kCreated);

  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);
  {
    NoncontiguousBufferBuilder nb;
    nb.Append(str);
    rsp_ptr->SetNonContiguousProtocolBody(nb.DestructiveGet());
  }

  rapidjson::Document reply;
  ASSERT_TRUE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  reply.Accept(writer);
  ASSERT_EQ(buffer.GetString(), str);
}

TEST_F(HttpClientCodecTest, FillResponseAsString) {
  std::string str{"this is string"};

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kStringNoop);

  context->SetRspEncodeType(serialization::kNoopType);
  context->SetRspEncodeDataType(serialization::kStringNoop);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  rsp.SetStatus(trpc::http::HttpResponse::StatusCode::kAccepted);

  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);
  {
    NoncontiguousBufferBuilder nb;
    nb.Append(str);
    rsp_ptr->SetNonContiguousProtocolBody(nb.DestructiveGet());
  }

  std::string reply;
  ASSERT_TRUE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));
  ASSERT_EQ(reply, str);
}

TEST_F(HttpClientCodecTest, FillResponseAsUnSupport) {
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kMaxType);

  context->SetRspEncodeType(serialization::kMaxType);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  rsp.SetStatus(trpc::http::HttpResponse::StatusCode::kNoContent);
  rsp.SetContent("this is unsupport");

  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);

  std::string reply;
  ASSERT_FALSE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));
}

TEST_F(HttpClientCodecTest, FillResponseAsBadRsp) {
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kStringNoop);

  context->SetRspEncodeType(serialization::kNoopType);
  context->SetRspEncodeDataType(serialization::kStringNoop);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  rsp.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  rsp.SetContent("NOT_FOUND");

  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);

  std::string reply;
  ASSERT_FALSE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));
}

TEST_F(HttpClientCodecTest, FillResponseAsJsonParseError) {
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kStringNoop);

  context->SetRspEncodeType(serialization::kJsonType);
  context->SetRspEncodeDataType(serialization::kRapidJson);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("NOT_JSON");
    rsp_protocol->SetNonContiguousProtocolBody(builder.DestructiveGet());
  }

  rapidjson::Document reply;
  ASSERT_FALSE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));

  // content is empty
  ClientContextPtr context2 = MakeRefCounted<ClientContext>();
  context2->SetReqEncodeType(serialization::kNoopType);
  context2->SetReqEncodeDataType(serialization::kStringNoop);

  context2->SetRspEncodeType(serialization::kJsonType);
  context2->SetRspEncodeDataType(serialization::kRapidJson);

  rsp_protocol = codec_.CreateResponsePtr();
  context2->SetResponse(rsp_protocol);
  http::HttpResponse rsp2;
  rsp2.SetVersion("1.1");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("");
    rsp_protocol->SetNonContiguousProtocolBody(builder.DestructiveGet());
  }

  rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp2);

  rapidjson::Document reply2;
  ASSERT_TRUE(codec_.FillResponse(context2, rsp_protocol, reinterpret_cast<void*>(&reply2)));
  ASSERT_TRUE(reply2.HasParseError());
}

TEST_F(HttpClientCodecTest, FillResponseAsBadRspDueToBadTrpcRetCode) {
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kStringNoop);

  context->SetRspEncodeType(serialization::kNoopType);
  context->SetRspEncodeDataType(serialization::kStringNoop);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  rsp.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  rsp.SetContent("NOT_FOUND");
  rsp.SetHeader("trpc-ret", "1");
  rsp.SetHeader("trpc-func-ret", "0");
  rsp.SetHeader("trpc-error-msg", "bad trpc status");

  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);

  std::string reply;
  ASSERT_FALSE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));
  ASSERT_EQ(1, context->GetStatus().GetFrameworkRetCode());
}

TEST_F(HttpClientCodecTest, FillResponseAsBadRspDueToBadTrpcFuncRetCode) {
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kStringNoop);

  context->SetRspEncodeType(serialization::kNoopType);
  context->SetRspEncodeDataType(serialization::kStringNoop);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  http::HttpResponse rsp;
  rsp.SetVersion("1.1");
  rsp.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  rsp.SetContent("NOT_FOUND");
  rsp.SetHeader("trpc-ret", "0");
  rsp.SetHeader("trpc-func-ret", "2");
  rsp.SetHeader("trpc-error-msg", "bad trpc status");

  HttpResponseProtocol* rsp_ptr = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp_ptr->response = std::move(rsp);

  std::string reply;
  ASSERT_FALSE(codec_.FillResponse(context, rsp_protocol, reinterpret_cast<void*>(&reply)));
  ASSERT_EQ(2, context->GetStatus().GetFuncRetCode());
}

TEST_F(HttpClientCodecTest, GetIntegerFromResponseHeaderOk) {
  http::HttpResponse reply;
  reply.SetHeader("trpc-ret", "1");
  reply.SetHeader("trpc-func-ret", "2");
  reply.SetHeader("trpc-empty-value", "");

  auto resp_ptr = codec_.CreateResponsePtr();
  auto* response = static_cast<HttpResponseProtocol*>(resp_ptr.get());
  response->response = std::move(reply);

  auto trpc_ret = internal::GetHeaderInteger(response->response.GetHeader(), "trpc-ret");
  ASSERT_EQ(trpc_ret, 1);

  auto trpc_func_ret = internal::GetHeaderInteger(response->response.GetHeader(), "trpc-func-ret");

  ASSERT_EQ(trpc_func_ret, 2);

  auto trpc_empty_value = internal::GetHeaderInteger(response->response.GetHeader(), "rpc-empty-value");
  ASSERT_EQ(trpc_empty_value, 0);

  auto not_exist = internal::GetHeaderInteger(response->response.GetHeader(), "trpc-not-exist-xx");
  ASSERT_EQ(not_exist, 0);
}

TEST_F(HttpClientCodecTest, GetStringFromResponseHeaderOk) {
  http::HttpResponse reply;
  reply.SetHeader("trpc-ret", "1");
  reply.SetHeader("trpc-func-ret", "2");
  reply.SetHeader("trpc-empty-value", "");

  auto resp_ptr = codec_.CreateResponsePtr();
  auto* response = static_cast<HttpResponseProtocol*>(resp_ptr.get());
  response->response = std::move(reply);

  auto trpc_ret = internal::GetHeaderString(response->response.GetHeader(), "trpc-ret");
  ASSERT_EQ(trpc_ret, "1");

  auto trpc_func_ret = internal::GetHeaderString(response->response.GetHeader(), "trpc-func-ret");

  ASSERT_EQ(trpc_func_ret, "2");

  auto trpc_empty_value = internal::GetHeaderString(response->response.GetHeader(), "trpc-empty-value");
  ASSERT_TRUE(trpc_empty_value.empty());

  auto not_exist = internal::GetHeaderString(response->response.GetHeader(), "trpc-not-exist-xx");
  ASSERT_TRUE(not_exist.empty());
}

TEST_F(HttpClientCodecTest, CompressSucc) {
  compressor::Init();

  ProtocolPtr req_protocol = codec_.CreateRequestPtr();
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetRequest(req_protocol);
  context->SetReqCompressType(compressor::kZlib);
  HttpRequestProtocol* req = static_cast<HttpRequestProtocol*>(req_protocol.get());
  test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("hello");
  ASSERT_TRUE(codec_.FillRequest(context, req_protocol, &hello_req));

  ASSERT_EQ(req->request->GetHeader("Content-Encoding"), "deflate");
  ASSERT_EQ(req->request->GetHeader("Accept-Encoding"), "deflate");

  auto req_buf = req_protocol->GetNonContiguousProtocolBody();
  compressor::DecompressIfNeeded(compressor::kZlib, req_buf);
  NoncontiguousBufferInputStream nis(&req_buf);
  test::helloworld::HelloRequest hello_req_check;
  hello_req_check.ParsePartialFromZeroCopyStream(&nis);
  ASSERT_EQ("hello", hello_req_check.msg());

  test::helloworld::HelloReply hello_rsp;
  hello_rsp.set_msg("hello");
  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream nbos(&builder);
  hello_rsp.SerializePartialToZeroCopyStream(&nbos);
  nbos.Flush();
  auto buf = builder.DestructiveGet();
  compressor::CompressIfNeeded(compressor::kZlib, buf);

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  context->SetResponse(rsp_protocol);
  rsp_protocol->SetNonContiguousProtocolBody(std::move(buf));
  HttpResponseProtocol* rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp->response.SetHeader("Content-Encoding", "deflate");
  test::helloworld::HelloReply reply;
  ASSERT_TRUE(codec_.FillResponse(context, rsp_protocol, &reply));
  ASSERT_EQ("hello", reply.msg());
  ASSERT_EQ(context->GetRspCompressType(), compressor::kZlib);
}

TEST_F(HttpClientCodecTest, CompressFailed) {
  auto compressor = MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*compressor, Type()).WillOnce(::testing::Return(compressor::kZlib));
  CompressorFactory::GetInstance()->Register(compressor);

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetReqCompressType(compressor::kZlib);
  test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("hello");
  ProtocolPtr req_protocol = codec_.CreateRequestPtr();

  EXPECT_CALL(*compressor, DoCompress(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(false));
  bool ret = codec_.FillRequest(context, req_protocol, &hello_req);
  ASSERT_FALSE(ret);
  CompressorFactory::GetInstance()->Clear();
}

TEST_F(HttpClientCodecTest, DeCompressFailed) {
  auto compressor = MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*compressor, Type()).WillOnce(::testing::Return(compressor::kZlib));
  CompressorFactory::GetInstance()->Register(compressor);

  EXPECT_CALL(*compressor, DoDecompress(::testing::_, ::testing::_)).WillOnce(::testing::Return(false));

  test::helloworld::HelloReply hello_rsp;
  hello_rsp.set_msg("hello");
  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream nbos(&builder);
  hello_rsp.SerializePartialToZeroCopyStream(&nbos);
  nbos.Flush();

  ProtocolPtr rsp_protocol = codec_.CreateResponsePtr();
  rsp_protocol->SetNonContiguousProtocolBody(builder.DestructiveGet());
  HttpResponseProtocol* rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
  rsp->response.SetHeader("Content-Encoding", "deflate");

  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetResponse(rsp_protocol);

  ASSERT_FALSE(codec_.FillResponse(context, rsp_protocol, &hello_rsp));
  ASSERT_EQ(context->GetStatus().ErrorMessage(), "http input buffer which was decompressed failed");
  CompressorFactory::GetInstance()->Clear();
}

}  // namespace trpc::testing
