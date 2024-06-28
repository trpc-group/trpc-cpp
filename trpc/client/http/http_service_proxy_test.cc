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

#include "trpc/client/http/http_service_proxy.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/future/future_utility.h"
#include "trpc/naming/trpc_naming_registry.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/runtime/runtime.h"
#include "trpc/serialization/trpc_serialization.h"

namespace trpc::testing {

class TestClientFilter : public trpc::MessageClientFilter {
 public:
  std::string Name() override { return "test_filter"; }

  std::vector<trpc::FilterPoint> GetFilterPoint() override {
    std::vector<FilterPoint> points = {trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE,
                                       trpc::FilterPoint::CLIENT_POST_RPC_INVOKE};
    return points;
  }

  void operator()(trpc::FilterStatus& status, trpc::FilterPoint point, const trpc::ClientContextPtr& context) override {
    status = FilterStatus::CONTINUE;
    // record the status upon entering the CLIENT_POST_RPC_INVOKE point
    if (point == FilterPoint::CLIENT_POST_RPC_INVOKE) {
      status_ = context->GetStatus();
    }
  }

  void SetStatus(trpc::Status&& status) { status_ = std::move(status); }

  Status GetStatus() { return status_; }

 private:
  trpc::Status status_;
};

class HttpServiceProxyTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_TRUE(runtime::StartRuntime());
    trpc::detail::SetDefaultOption(option_);
    option_->name = "default_http_service";
    option_->caller_name = "";
    option_->codec_name = "http";
    option_->conn_type = "long";
    option_->network = "tcp";
    option_->timeout = 1000;
    option_->target = "127.0.0.1:10002";
    option_->selector_name = "direct";
    option_->threadmodel_instance_name = "trpc_separate_admin_instance";

    codec::Init();
    serialization::Init();
    naming::Init();

    filter_ = std::make_shared<TestClientFilter>();
    trpc::FilterManager::GetInstance()->AddMessageClientFilter(filter_);
    option_->service_filters.push_back(filter_->Name());
  }

  static void TearDownTestCase() {
    runtime::TerminateRuntime();
    codec::Destroy();
    serialization::Destroy();
    naming::Stop();
    naming::Destroy();
  }

 protected:
  static std::shared_ptr<ServiceProxyOption> option_;
  static std::shared_ptr<TestClientFilter> filter_;
};

std::shared_ptr<ServiceProxyOption> HttpServiceProxyTest::option_ = std::make_shared<ServiceProxyOption>();
std::shared_ptr<TestClientFilter> HttpServiceProxyTest::filter_;

class MockHttpServiceProxy : public trpc::http::HttpServiceProxy {
 public:
  void UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) override {
    if (is_error_) {
      is_error_ = false;
      SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
      return;
    }

    auto reply = std::any_cast<trpc::http::HttpResponse>(reply_);
    auto http_reply = std::move(reply);
    ProtocolPtr http_rsp = codec_->CreateResponsePtr();
    if (codec_->ZeroCopyDecode(context, http_reply, http_rsp)) {
      rsp = http_rsp;
      return;
    }

    SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
  }

  Future<ProtocolPtr> AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                const ProtocolPtr& req_protocol) override {
    if (is_error_) {
      is_error_ = false;
      SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
      return MakeExceptionFuture<ProtocolPtr>(Exception());
    }
    auto reply = std::any_cast<trpc::http::HttpResponse>(reply_);
    auto http_reply = std::move(reply);
    ProtocolPtr http_rsp = codec_->CreateResponsePtr();

    if (codec_->ZeroCopyDecode(context, http_reply, http_rsp)) {
      return MakeReadyFuture<ProtocolPtr>(std::move(http_rsp));
    }

    SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
    return MakeExceptionFuture<ProtocolPtr>(Exception());
  }

  void SetMockServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) {
    SetServiceProxyOptionInner(option);
  }

  void SetReply(const std::any& r) { reply_ = r; }

  void SetReplyError(bool is_error) { is_error_ = is_error; }

  void ConstructGetRequest(const ClientContextPtr& context, const std::string& url, HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructGetRequest(context, url, req);
  }
  void ConstructHeadRequest(const ClientContextPtr& context, const std::string& url, HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructHeadRequest(context, url, req);
  }
  void ConstructOptionsRequest(const ClientContextPtr& context, const std::string& url, HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructOptionsRequest(context, url, req);
  }
  void ConstructPostRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                            HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructPostRequest(context, url, data, req);
  }
  void ConstructPostRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                            HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructPostRequest(context, url, data, req);
  }
  void ConstructPutRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                           HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructPutRequest(context, url, data, req);
  }
  void ConstructPutRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                           HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructPutRequest(context, url, data, req);
  }
  void ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                              HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructDeleteRequest(context, url, data, req);
  }
  void ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                              HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructDeleteRequest(context, url, data, req);
  }
  void ConstructPatchRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                             HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructPatchRequest(context, url, data, req);
  }
  void ConstructPatchRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                             HttpRequestProtocol* req) {
    return HttpServiceProxy::ConstructPatchRequest(context, url, data, req);
  }
  void ConstructPBRequest(const ClientContextPtr& context, const std::string& url, std::string&& content,
                          HttpRequestProtocol* req) {
    HttpServiceProxy::ConstructPBRequest(context, url, std::move(content), req);
  }

 protected:
  void SetErrorStatus(const ClientContextPtr& context, int err_code) {
    Status status;
    status.SetFrameworkRetCode(err_code);
    context->SetStatus(std::move(status));
  }

  std::any reply_;
  bool is_error_{false};
};

class MockReturnAllRspHttpServiceProxy : public trpc::http::HttpServiceProxy {
 public:
  void UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) override {
    if (is_error_) {
      is_error_ = false;
      SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
      return;
    }

    auto reply = std::any_cast<trpc::http::HttpResponse>(reply_);
    auto http_reply = std::move(reply);
    ProtocolPtr http_rsp = codec_->CreateResponsePtr();
    if (codec_->ZeroCopyDecode(context, http_reply, http_rsp)) {
      rsp = http_rsp;
      return;
    }

    SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
  }

  Future<ProtocolPtr> AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                const ProtocolPtr& req_protocol) override {
    if (is_error_) {
      is_error_ = false;
      SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
      return MakeExceptionFuture<ProtocolPtr>(Exception());
    }
    auto reply = std::any_cast<trpc::http::HttpResponse>(reply_);
    auto http_reply = std::move(reply);
    ProtocolPtr http_rsp = codec_->CreateResponsePtr();

    if (codec_->ZeroCopyDecode(context, http_reply, http_rsp)) {
      return MakeReadyFuture<ProtocolPtr>(std::move(http_rsp));
    }

    SetErrorStatus(context, trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
    return MakeExceptionFuture<ProtocolPtr>(Exception());
  }

  void SetMockServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) {
    SetServiceProxyOptionInner(option);
  }

  void SetReply(const std::any& r) { reply_ = r; }

  void SetReplyError(bool is_error) { is_error_ = is_error; }

  bool CheckHttpResponse(const ClientContextPtr& context, const ProtocolPtr& http_rsp) override { return true; }

 protected:
  void SetErrorStatus(const ClientContextPtr& context, int err_code) {
    Status status;
    status.SetFrameworkRetCode(err_code);
    context->SetStatus(std::move(status));
  }

  std::any reply_;
  bool is_error_{false};
};

using MockHttpServiceProxyPtr = std::shared_ptr<MockHttpServiceProxy>;
using MockReturnAllRspHttpServiceProxyPtr = std::shared_ptr<MockReturnAllRspHttpServiceProxy>;

TEST_F(HttpServiceProxyTest, SuccessRspGet) {
  std::string str{R"({"age":"18","height":180})"};

  MockHttpServiceProxyPtr proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("hello", "world");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  rapidjson::Document doc;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Get(ctx, "http://127.0.0.1:10002/hello", &doc);
  EXPECT_TRUE(st.OK());
  EXPECT_STREQ("18", doc["age"].GetString());
  EXPECT_EQ(180, doc["height"].GetInt());

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  std::string rsp_str;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  st = proxy->GetString(ctx, "http://127.0.0.1:10002/hello", &rsp_str);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(str, rsp_str);

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_fut = proxy->AsyncGet(ctx, "http://127.0.0.1:10002/hello").Then([](rapidjson::Document&& d) {
    EXPECT_STREQ("18", d["age"].GetString());
    EXPECT_EQ(180, d["height"].GetInt());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_get_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_string_fut =
      proxy->AsyncGetString(ctx, "http://127.0.0.1:10002/hello").Then([](std::string&& rsp_str) {
        EXPECT_EQ("{\"age\":\"18\",\"height\":180}", rsp_str);
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_string_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
}

TEST_F(HttpServiceProxyTest, SuccessRspGetJsonEmpty) {
  std::string str;

  MockHttpServiceProxyPtr proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);
  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("hello", "world");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  rapidjson::Document doc;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Get(ctx, "http://127.0.0.1:10002/hello", &doc);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(doc.HasParseError());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_fut = proxy->AsyncGet(ctx, "http://127.0.0.1:10002/hello").Then([](rapidjson::Document&& d) {
    EXPECT_TRUE(d.HasParseError());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_get_fut));
}

TEST_F(HttpServiceProxyTest, InvalidRspGet) {
  MockHttpServiceProxyPtr proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  rapidjson::Document doc;
  Status st = proxy->Get(ctx, "http://127.0.0.1:10002/hello", &doc);
  EXPECT_TRUE(!st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  proxy->SetReply(reply);
  std::string rsp_str;
  st = proxy->GetString(ctx, "http://127.0.0.1:10002/hello", &rsp_str);
  EXPECT_TRUE(!st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_fut = proxy->AsyncGet(ctx, "http://127.0.0.1:10002/hello").Then([](Future<rapidjson::Document>&& d) {
    EXPECT_TRUE(d.IsFailed());

    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_get_fut));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_string_fut =
      proxy->AsyncGetString(ctx, "http://127.0.0.1:10002/hello").Then([](Future<std::string>&& d) {
        EXPECT_TRUE(d.IsFailed());

        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_string_fut));
}

TEST_F(HttpServiceProxyTest, ReturnAllRspGet) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Get 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  trpc::http::HttpResponse reply_json;
  reply_json.SetVersion("1.1");
  reply_json.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(R"({"error":"Get 404 not found"})");
    reply_json.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply_json);
  rapidjson::Document doc_json;
  Status st = proxy->Get(ctx, "http://127.0.0.1:10002/hello", &doc_json);
  EXPECT_TRUE(st.OK());
  EXPECT_STREQ("Get 404 not found", doc_json["error"].GetString());

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  rapidjson::Document doc;
  st = proxy->Get(ctx, "http://127.0.0.1:10002/hello", &doc);
  EXPECT_FALSE(st.OK());
  EXPECT_EQ(st.GetFrameworkRetCode(), GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  std::string rsp_str;
  st = proxy->GetString(ctx, "http://127.0.0.1:10002/hello", &rsp_str);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(rsp_str, "Get 404 not found");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_fut = proxy->AsyncGet(ctx, "http://127.0.0.1:10002/hello").Then([](Future<rapidjson::Document>&& d) {
    EXPECT_TRUE(d.IsFailed());

    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_get_fut));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_string_fut =
      proxy->AsyncGetString(ctx, "http://127.0.0.1:10002/hello").Then([](Future<std::string>&& d) {
        EXPECT_FALSE(d.IsFailed());

        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_string_fut));
}

TEST_F(HttpServiceProxyTest, SuccessRspGetFull) {
  std::string str{"trpc"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("trpc", "cpp");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  trpc::http::HttpResponse rsp;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Get2(ctx, "http://127.0.0.1:10002/hello", &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(rsp.IsOK());
  EXPECT_EQ(str, rsp.GetContent());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_fut =
      proxy->AsyncGet2(ctx, "http://127.0.0.1:10002/hello").Then([&reply, &str](http::HttpResponse&& rsp) {
        EXPECT_TRUE(rsp.IsOK());
        EXPECT_EQ(str, rsp.GetContent());
        EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
        EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
        EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
}

TEST_F(HttpServiceProxyTest, InvalidRspGetFull) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");
  reply.SetHeader("trpc", "cpp");

  proxy->SetReply(reply);
  http::HttpResponse rsp;
  Status st = proxy->Get2(ctx, "http://127.0.0.1:10002/hello", &rsp);
  EXPECT_TRUE(!st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_fut =
      proxy->AsyncGet2(ctx, "http://127.0.0.1:10002/hello").Then([](Future<http::HttpResponse>&& future) {
        EXPECT_TRUE(future.IsFailed());

        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_fut));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
}

TEST_F(HttpServiceProxyTest, ReturnAllRspGetFull) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetHeader("trpc", "cpp");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Get 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);
  http::HttpResponse rsp;
  Status st = proxy->Get2(ctx, "http://127.0.0.1:10002/hello", &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_FALSE(rsp.IsOK());
  EXPECT_EQ("Get 404 not found", rsp.GetContent());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_fut = proxy->AsyncGet2(ctx, "http://127.0.0.1:10002/hello").Then([&reply](http::HttpResponse&& rsp) {
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Get 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_get_fut));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
}

TEST_F(HttpServiceProxyTest, SuccessRspHead) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("trpc", "cpp");

  proxy->SetReply(reply);
  http::HttpResponse rsp;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Head(ctx, "http://127.0.0.1:10002/hello", &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(rsp.IsOK());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::HEAD);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_head_fut = proxy->AsyncHead(ctx, "http://127.0.0.1:10002/hello").Then([&reply](http::HttpResponse&& rsp) {
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_head_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::HEAD);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
}

TEST_F(HttpServiceProxyTest, InvalidRspHead) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetHeader("X-ret-msg", "error");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    Status status = proxy->Head(ctx, "http://127.0.0.1:10002/hello", &rsp);
    EXPECT_TRUE(!status.OK());
    EXPECT_TRUE(status.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::HEAD);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    auto async_head_fut =
        proxy->AsyncHead(ctx, "http://127.0.0.1:10002/hello").Then([](Future<http::HttpResponse>&& future) {
          EXPECT_TRUE(future.IsFailed());

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_head_fut));

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::HEAD);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspHead) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetHeader("X-ret-msg", "error");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Head 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    Status status = proxy->Head(ctx, "http://127.0.0.1:10002/hello", &rsp);
    EXPECT_TRUE(status.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ(rsp.GetContent(), "Head 404 not found");
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("X-ret-msg"), reply.GetHeader("X-ret-msg"));

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::HEAD);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    auto async_head_fut =
        proxy->AsyncHead(ctx, "http://127.0.0.1:10002/hello").Then([&reply](http::HttpResponse&& rsp) {
          EXPECT_FALSE(rsp.IsOK());
          EXPECT_EQ(rsp.GetContent(), "Head 404 not found");
          EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
          EXPECT_EQ(rsp.GetHeader("X-ret-msg"), reply.GetHeader("X-ret-msg"));

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_head_fut));

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::HEAD);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
}

TEST_F(HttpServiceProxyTest, SuccessRspOptions) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("trpc", "cpp");

  proxy->SetReply(reply);
  http::HttpResponse rsp;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Options(ctx, "http://127.0.0.1:10002/hello", &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::OPTIONS);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_options_fut =
      proxy->AsyncOptions(ctx, "http://127.0.0.1:10002/hello").Then([&reply](http::HttpResponse&& rsp) {
        EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
        EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_options_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::OPTIONS);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
}

TEST_F(HttpServiceProxyTest, InvalidRspOptions) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetHeader("X-ret-msg", "error");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    Status status = proxy->Options(ctx, "http://127.0.0.1:10002/hello", &rsp);
    EXPECT_TRUE(!status.OK());
    EXPECT_TRUE(status.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::OPTIONS);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    auto async_options_fut =
        proxy->AsyncOptions(ctx, "http://127.0.0.1:10002/hello").Then([](Future<http::HttpResponse>&& future) {
          EXPECT_TRUE(future.IsFailed());

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_options_fut));

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::OPTIONS);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspOptions) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetHeader("X-ret-msg", "error");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Options 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    Status status = proxy->Options(ctx, "http://127.0.0.1:10002/hello", &rsp);
    EXPECT_TRUE(status.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Options 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("X-ret-msg"), reply.GetHeader("X-ret-msg"));

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::OPTIONS);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    auto async_options_fut =
        proxy->AsyncOptions(ctx, "http://127.0.0.1:10002/hello").Then([&reply](http::HttpResponse&& rsp) {
          EXPECT_FALSE(rsp.IsOK());
          EXPECT_EQ(rsp.GetContent(), "Options 404 not found");
          EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
          EXPECT_EQ(rsp.GetHeader("X-ret-msg"), reply.GetHeader("X-ret-msg"));

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_options_fut));

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::OPTIONS);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(req->request->GetContent(), "");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), "");
  }
}

TEST_F(HttpServiceProxyTest, InnerUnaryInvokeReturnFail) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  proxy->SetReplyError(true);
  rapidjson::Document doc;
  Status st = proxy->Get(ctx, "http://127.0.0.1:10002/hello", &doc);
  EXPECT_TRUE(!st.OK());
  EXPECT_TRUE(st.GetFrameworkRetCode() == trpc::TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
}

TEST_F(HttpServiceProxyTest, AsyncInnerUnaryInvokeReturnException) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  proxy->SetReplyError(true);
  auto async_get_fut = proxy->AsyncGet(ctx, "http://127.0.0.1:10002/hello").Then([](Future<rapidjson::Document>&& d) {
    EXPECT_TRUE(d.IsFailed());

    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_get_fut));

  proxy->SetReplyError(true);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_get_string_fut =
      proxy->AsyncGetString(ctx, "http://127.0.0.1:10002/hello").Then([](Future<std::string>&& d) {
        EXPECT_TRUE(d.IsFailed());

        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_string_fut));
}

TEST_F(HttpServiceProxyTest, SuccessRspPostStr) {
  std::string str{R"({"age":"18","height":180})"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  ctx->SetStatus(Status());
  std::string data = "post data";
  std::string body;
  Status st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", data, &body);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(str, body);

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    trpc::NoncontiguousBuffer body;
    st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet(), &body);
    EXPECT_TRUE(st.OK());
    EXPECT_EQ(str, FlattenSlow(body));
  }

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", data).Then([&str](std::string&& body) {
    EXPECT_EQ(str, body);
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_post_fut1));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet())
                               .Then([&str](trpc::NoncontiguousBuffer&& body) {
                                 EXPECT_EQ(str, FlattenSlow(body));
                                 return MakeReadyFuture<>();
                               });
    future::BlockingGet(std::move(async_post_fut1));
  }

  proxy->SetReply(reply);
  auto copy_data = data;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", std::move(copy_data), &body);
  EXPECT_EQ(str, body);

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");

  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  std::string moved_data = data;
  auto async_post_fut2 =
      proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", std::move(data)).Then([](std::string&& body) {
        EXPECT_EQ("{\"age\":\"18\",\"height\":180}", body);
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_post_fut2));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(moved_data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
}

TEST_F(HttpServiceProxyTest, InvalidRspPostStr) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  std::string data = "post data";
  std::string body;
  Status st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", data, &body);
  EXPECT_FALSE(st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<std::string>&& d) {
    EXPECT_TRUE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_post_fut1));
  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut2 =
      proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", std::move(data)).Then([](Future<std::string>&& d) {
        EXPECT_TRUE(d.IsFailed());
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_post_fut2));

  trpc::http::HttpResponse reply1;
  reply1.SetVersion("1.1");
  reply1.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("404 not found");
    reply1.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply1);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    trpc::NoncontiguousBuffer body;
    st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet(), &body);
    EXPECT_TRUE(!st.OK());
  }

  proxy->SetReply(reply1);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet())
                               .Then([](Future<trpc::NoncontiguousBuffer>&& future) {
                                 EXPECT_TRUE(future.IsFailed());
                                 return MakeReadyFuture<>();
                               });
    future::BlockingGet(std::move(async_post_fut1));
  }
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspPostStr) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Post 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  std::string data = "post data";
  std::string body;
  Status st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", data, &body);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(body, "Post 404 not found");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<std::string>&& d) {
    EXPECT_FALSE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_post_fut1));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut2 =
      proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", std::move(data)).Then([](Future<std::string>&& d) {
        EXPECT_FALSE(d.IsFailed());
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_post_fut2));

  trpc::http::HttpResponse reply1;
  reply1.SetVersion("1.1");
  reply1.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Post 404 not found");
    reply1.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply1);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    trpc::NoncontiguousBuffer body;
    st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet(), &body);
    EXPECT_TRUE(st.OK());
    EXPECT_EQ("Post 404 not found", FlattenSlow(body));
  }

  proxy->SetReply(reply1);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet())
                               .Then([](trpc::NoncontiguousBuffer&& body) {
                                 EXPECT_EQ("Post 404 not found", FlattenSlow(body));
                                 return MakeReadyFuture<>();
                               });
    future::BlockingGet(std::move(async_post_fut1));
  }
}

TEST_F(HttpServiceProxyTest, SuccessRspPostJson) {
  std::string str{"{\"age\":\"18\",\"height\":180}"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  rapidjson::Document data;
  std::string request_json = "{\"age\":\"18\",\"height\":180}";
  data.Parse(request_json);

  rapidjson::Document doc;
  Status st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", data, &doc);
  EXPECT_TRUE(st.OK());
  EXPECT_STREQ("18", doc["age"].GetString());
  EXPECT_EQ(180, doc["height"].GetInt());

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(request_json.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "application/json");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", data).Then([](rapidjson::Document&& d) {
    EXPECT_STREQ("18", d["age"].GetString());
    EXPECT_EQ(180, d["height"].GetInt());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_post_fut1));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(request_json.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "application/json");
}

TEST_F(HttpServiceProxyTest, SuccessRspPostJsonEmpty) {
  std::string str{""};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("hello", "world");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  rapidjson::Document data;
  std::string request_json = "{\"age\":\"18\",\"height\":180}";
  data.Parse(request_json);

  rapidjson::Document doc;
  Status st = proxy->Post(ctx, "http://127.0.0.1:10002/hello", data, &doc);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(doc.HasParseError());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPost(ctx, "http://127.0.0.1:10002/hello", data).Then([](rapidjson::Document&& d) {
    EXPECT_TRUE(d.HasParseError());
    return MakeReadyFuture<>();
  });
}

TEST_F(HttpServiceProxyTest, InvalidRspPostJson) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  std::string url = "http://127.0.0.1:10002/hello";

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  rapidjson::Document data;
  rapidjson::Document doc;
  Status st = proxy->Post(ctx, url, data, &doc);
  EXPECT_FALSE(st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPost(ctx, url, data).Then([](Future<rapidjson::Document>&& d) {
    EXPECT_TRUE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_post_fut1));
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspPostJson) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  std::string url = "http://127.0.0.1:10002/hello";

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Post Json Error");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);
  rapidjson::Document data;
  rapidjson::Document doc;
  Status st = proxy->Post(ctx, url, data, &doc);
  EXPECT_FALSE(st.OK());
  EXPECT_EQ(st.GetFrameworkRetCode(), codec::GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPost(ctx, url, data).Then([](Future<rapidjson::Document>&& d) {
    EXPECT_TRUE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_post_fut1));
}

TEST_F(HttpServiceProxyTest, SuccessRspPostFull) {
  std::string str{"trpc"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("trpc", "cpp");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  trpc::http::HttpResponse rsp;
  std::string data = "post data";
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Post2(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(rsp.IsOK());
  EXPECT_EQ(str, rsp.GetContent());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut =
      proxy->AsyncPost2(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply, &str](http::HttpResponse&& rsp) {
        EXPECT_TRUE(rsp.IsOK());
        EXPECT_EQ(str, rsp.GetContent());
        EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
        EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
        EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_post_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  {
    trpc::http::HttpResponse rsp;
    std::string data = "post data";
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    st = proxy->Post2(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_TRUE(rsp.IsOK());
    EXPECT_EQ(str, rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    data = "post data";
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "post data";
    auto async_post_fut2 = proxy->AsyncPost2(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                               .Then([&reply, &str](http::HttpResponse&& rsp) {
                                 EXPECT_TRUE(rsp.IsOK());
                                 EXPECT_EQ(str, rsp.GetContent());
                                 EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
                                 EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
                                 EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
                                 return MakeReadyFuture<>();
                               });
    future::BlockingGet(std::move(async_post_fut2));

    data = "post data";
    req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, InvalidRspPostFull) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "post data";
    Status st = proxy->Post2(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "post data";
    Status st = proxy->Post2(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "post data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "post data";
    auto async_post_fut =
        proxy->AsyncPost2(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<http::HttpResponse>&& future) {
          EXPECT_TRUE(future.IsFailed());

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_post_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "post data";
    auto async_post_fut = proxy->AsyncPost2(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                              .Then([](Future<http::HttpResponse>&& future) {
                                EXPECT_TRUE(future.IsFailed());

                                return MakeReadyFuture<>();
                              });
    future::BlockingGet(std::move(async_post_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "post data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspPostFull) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Post 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "post data";
    Status st = proxy->Post2(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Post 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "post data";
    Status st = proxy->Post2(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Post 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "post data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "post data";
    auto async_post_fut =
        proxy->AsyncPost2(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply](http::HttpResponse&& rsp) {
          EXPECT_FALSE(rsp.IsOK());
          EXPECT_EQ("Post 404 not found", rsp.GetContent());
          EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
          EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
          EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_post_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "post data";
    auto async_post_fut = proxy->AsyncPost2(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                              .Then([&reply](http::HttpResponse&& rsp) {
                                EXPECT_FALSE(rsp.IsOK());
                                EXPECT_EQ("Post 404 not found", rsp.GetContent());
                                EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
                                EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
                                EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

                                return MakeReadyFuture<>();
                              });
    future::BlockingGet(std::move(async_post_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "post data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::POST);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, SuccessRspPutStr) {
  std::string str{"{\"age\":\"18\",\"height\":180}"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  ctx->SetStatus(Status());
  std::string data = "put data";
  std::string body;
  Status st = proxy->Put(ctx, "http://127.0.0.1:10002/hello", data, &body);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(str, body);

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    trpc::NoncontiguousBuffer body;
    st = proxy->Put(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet(), &body);
    EXPECT_TRUE(st.OK());
    EXPECT_EQ(str, FlattenSlow(body));
  }

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut1 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", data).Then([&str](std::string&& body) {
    EXPECT_EQ(str, body);
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut1));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    trpc::NoncontiguousBufferBuilder nb;
    nb.Append(data);
    auto async_put_fut1 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", nb.DestructiveGet())
                              .Then([&str](trpc::NoncontiguousBuffer&& body) {
                                EXPECT_EQ(str, FlattenSlow(body));
                                return MakeReadyFuture<>();
                              });
    future::BlockingGet(std::move(async_put_fut1));
  }

  proxy->SetReply(reply);
  auto copy_data = data;
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  st = proxy->Put(ctx, "http://127.0.0.1:10002/hello", std::move(copy_data), &body);
  EXPECT_EQ(str, body);

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");

  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  std::string moved_data = data;
  auto async_put_fut2 =
      proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", std::move(data)).Then([](std::string&& body) {
        EXPECT_EQ("{\"age\":\"18\",\"height\":180}", body);
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_put_fut2));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(moved_data.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
}

TEST_F(HttpServiceProxyTest, InvalidRspPutStr) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  std::string data = "put data";
  std::string body;
  Status st = proxy->Put(ctx, "http://127.0.0.1:10002/hello", data, &body);
  EXPECT_FALSE(st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut1 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<std::string>&& d) {
    EXPECT_TRUE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut1));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut2 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<std::string>&& d) {
    EXPECT_TRUE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut2));
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspPutStr) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Put 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);
  std::string data = "put data";
  std::string body;
  Status st = proxy->Put(ctx, "http://127.0.0.1:10002/hello", data, &body);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(body, "Put 404 not found");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut1 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", data).Then([](std::string&& d) {
    EXPECT_EQ(d, "Put 404 not found");
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut1));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut2 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", data).Then([](std::string&& d) {
    EXPECT_EQ(d, "Put 404 not found");
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut2));
}

TEST_F(HttpServiceProxyTest, SuccessRspPutJson) {
  std::string str{"{\"age\":\"18\",\"height\":180}"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  rapidjson::Document data;
  std::string request_json = "{\"age\":\"18\",\"height\":180}";
  data.Parse(request_json);

  rapidjson::Document doc;
  Status st = proxy->Put(ctx, "http://127.0.0.1:10002/hello", data, &doc);
  EXPECT_TRUE(st.OK());
  EXPECT_STREQ("18", doc["age"].GetString());
  EXPECT_EQ(180, doc["height"].GetInt());

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(request_json.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "application/json");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut1 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", data).Then([](rapidjson::Document&& d) {
    EXPECT_STREQ("18", d["age"].GetString());
    EXPECT_EQ(180, d["height"].GetInt());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut1));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(request_json.size()));
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "application/json");
}

TEST_F(HttpServiceProxyTest, SuccessRspPutJsonEmpty) {
  std::string str{""};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("hello", "world");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  rapidjson::Document data;
  std::string request_json = "{\"age\":\"18\",\"height\":180}";
  data.Parse(request_json);

  rapidjson::Document doc;
  Status st = proxy->Put(ctx, "http://127.0.0.1:10002/hello", data, &doc);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(doc.HasParseError());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_post_fut1 = proxy->AsyncPut(ctx, "http://127.0.0.1:10002/hello", data).Then([](rapidjson::Document&& d) {
    EXPECT_TRUE(d.HasParseError());
    return MakeReadyFuture<>();
  });
}

TEST_F(HttpServiceProxyTest, InvalidRspPutJson) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  std::string url = "http://127.0.0.1:10002/hello";

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  rapidjson::Document data;
  rapidjson::Document doc;
  Status st = proxy->Put(ctx, url, data, &doc);
  EXPECT_FALSE(st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut1 = proxy->AsyncPut(ctx, url, data).Then([](Future<rapidjson::Document>&& d) {
    EXPECT_TRUE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut1));
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspPutJson) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  std::string url = "http://127.0.0.1:10002/hello";

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Put Json Error");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);
  rapidjson::Document data;
  rapidjson::Document doc;
  Status st = proxy->Put(ctx, url, data, &doc);
  EXPECT_FALSE(st.OK());
  EXPECT_EQ(st.GetFrameworkRetCode(), codec::GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut1 = proxy->AsyncPut(ctx, url, data).Then([](Future<rapidjson::Document>&& d) {
    EXPECT_TRUE(d.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(async_put_fut1));
}

TEST_F(HttpServiceProxyTest, SuccessRspPutFull) {
  std::string str{"trpc"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("trpc", "cpp");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  trpc::http::HttpResponse rsp;
  std::string data = "put data";
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Put2(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(rsp.IsOK());
  EXPECT_EQ(str, rsp.GetContent());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_put_fut =
      proxy->AsyncPut2(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply, &str](http::HttpResponse&& rsp) {
        EXPECT_TRUE(rsp.IsOK());
        EXPECT_EQ(str, rsp.GetContent());
        EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
        EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
        EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_put_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  {
    trpc::http::HttpResponse rsp;
    std::string data = "put data";
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    st = proxy->Put2(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_TRUE(rsp.IsOK());
    EXPECT_EQ(str, rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "put data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "put data";
    auto async_put_fut2 = proxy->AsyncPut2(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                              .Then([&reply, &str](http::HttpResponse&& rsp) {
                                EXPECT_TRUE(rsp.IsOK());
                                EXPECT_EQ(str, rsp.GetContent());
                                EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
                                EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
                                EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
                                return MakeReadyFuture<>();
                              });
    future::BlockingGet(std::move(async_put_fut2));

    req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "put data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, InvalidRspPutFull) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "put data";
    Status st = proxy->Put2(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "put data";
    Status st = proxy->Put2(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "put data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "put data";
    auto async_put_fut =
        proxy->AsyncPut2(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<http::HttpResponse>&& future) {
          EXPECT_TRUE(future.IsFailed());

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_put_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "put data";
    auto async_put_fut = proxy->AsyncPut2(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                             .Then([](Future<http::HttpResponse>&& future) {
                               EXPECT_TRUE(future.IsFailed());

                               return MakeReadyFuture<>();
                             });
    future::BlockingGet(std::move(async_put_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "put data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspPutFull) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Put 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "put data";
    Status st = proxy->Put2(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Put 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "put data";
    Status st = proxy->Put2(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Put 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "put data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "put data";
    auto async_put_fut =
        proxy->AsyncPut2(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply](http::HttpResponse&& rsp) {
          EXPECT_FALSE(rsp.IsOK());
          EXPECT_EQ("Put 404 not found", rsp.GetContent());
          EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
          EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
          EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_put_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "put data";
    auto async_put_fut =
        proxy->AsyncPut2(ctx, "http://127.0.0.1:10002/hello", std::move(data)).Then([&reply](http::HttpResponse&& rsp) {
          EXPECT_FALSE(rsp.IsOK());
          EXPECT_EQ("Put 404 not found", rsp.GetContent());
          EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
          EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
          EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_put_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "put data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PUT);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, SuccessRspPatch) {
  std::string str{"trpc"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("trpc", "cpp");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  trpc::http::HttpResponse rsp;
  std::string data = "patch data";
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Patch(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(rsp.IsOK());
  EXPECT_EQ(str, rsp.GetContent());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_patch_fut =
      proxy->AsyncPatch(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply, &str](http::HttpResponse&& rsp) {
        EXPECT_TRUE(rsp.IsOK());
        EXPECT_EQ(str, rsp.GetContent());
        EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
        EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
        EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_patch_fut));

  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  {
    trpc::http::HttpResponse rsp;
    std::string data = "patch data";
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    st = proxy->Patch(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_TRUE(rsp.IsOK());
    EXPECT_EQ(str, rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "patch data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "patch data";
    auto async_patch_fut2 = proxy->AsyncPatch(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                                .Then([&reply, &str](http::HttpResponse&& rsp) {
                                  EXPECT_TRUE(rsp.IsOK());
                                  EXPECT_EQ(str, rsp.GetContent());
                                  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
                                  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
                                  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
                                  return MakeReadyFuture<>();
                                });
    future::BlockingGet(std::move(async_patch_fut2));

    req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "patch data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, InvalidRspPatch) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "patch data";
    Status st = proxy->Patch(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "patch data";
    Status st = proxy->Patch(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "patch data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "put data";
    auto async_patch_fut =
        proxy->AsyncPatch(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<http::HttpResponse>&& future) {
          EXPECT_TRUE(future.IsFailed());

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_patch_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "patch data";
    auto async_patch_fut = proxy->AsyncPatch(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                               .Then([](Future<http::HttpResponse>&& future) {
                                 EXPECT_TRUE(future.IsFailed());

                                 return MakeReadyFuture<>();
                               });
    future::BlockingGet(std::move(async_patch_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "patch data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspPatch) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Patch 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "patch data";
    Status st = proxy->Patch(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Patch 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "patch data";
    Status st = proxy->Patch(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Patch 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "patch data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "put data";
    auto async_patch_fut =
        proxy->AsyncPatch(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply](http::HttpResponse&& rsp) {
          EXPECT_FALSE(rsp.IsOK());
          EXPECT_EQ("Patch 404 not found", rsp.GetContent());
          EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
          EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
          EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_patch_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "patch data";
    auto async_patch_fut = proxy->AsyncPatch(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                               .Then([&reply](http::HttpResponse&& rsp) {
                                 EXPECT_FALSE(rsp.IsOK());
                                 EXPECT_EQ("Patch 404 not found", rsp.GetContent());
                                 EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
                                 EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
                                 EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

                                 return MakeReadyFuture<>();
                               });
    future::BlockingGet(std::move(async_patch_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "patch data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::PATCH);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, SuccessRspDelete) {
  std::string str{"trpc"};

  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  ctx->SetHttpHeader("hello", "world");
  EXPECT_EQ("world", ctx->GetHttpHeader("hello"));

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  reply.SetHeader("trpc", "cpp");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);

  trpc::http::HttpResponse rsp;
  std::string data = "delete data";
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  Status st = proxy->Delete(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
  EXPECT_TRUE(st.OK());
  EXPECT_TRUE(rsp.IsOK());
  EXPECT_EQ(str, rsp.GetContent());
  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

  auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_delete_fut =
      proxy->AsyncDelete(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply, &str](http::HttpResponse&& rsp) {
        EXPECT_TRUE(rsp.IsOK());
        EXPECT_EQ(str, rsp.GetContent());
        EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
        EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
        EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_delete_fut));
  req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
  EXPECT_EQ(req->request->GetVersion(), "1.1");
  EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
  EXPECT_EQ(req->request->GetContent(), data);
  EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
  EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));

  {
    trpc::http::HttpResponse rsp;
    std::string data = "delete data";
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    st = proxy->Delete(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_TRUE(rsp.IsOK());
    EXPECT_EQ(str, rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "delete data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "delete data";
    auto async_delete_fut2 = proxy->AsyncDelete(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                                 .Then([&reply, &str](http::HttpResponse&& rsp) {
                                   EXPECT_TRUE(rsp.IsOK());
                                   EXPECT_EQ(str, rsp.GetContent());
                                   EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
                                   EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
                                   EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");
                                   return MakeReadyFuture<>();
                                 });
    future::BlockingGet(std::move(async_delete_fut2));

    req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "delete data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, InvalidRspDelete) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "delete data";
    Status st = proxy->Delete(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "delete data";
    Status st = proxy->Delete(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(!st.OK());
    EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "delete data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "delete data";
    auto async_delete_fut =
        proxy->AsyncDelete(ctx, "http://127.0.0.1:10002/hello", data).Then([](Future<http::HttpResponse>&& future) {
          EXPECT_TRUE(future.IsFailed());

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_delete_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "delete data";
    auto async_delete_fut = proxy->AsyncDelete(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                                .Then([](Future<http::HttpResponse>&& future) {
                                  EXPECT_TRUE(future.IsFailed());

                                  return MakeReadyFuture<>();
                                });
    future::BlockingGet(std::move(async_delete_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "delete data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, ReturnlAllRspDelete) {
  auto proxy = std::make_shared<MockReturnAllRspHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append("Delete 404 not found");
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }
  reply.SetHeader("trpc", "cpp");
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "delete data";
    Status st = proxy->Delete(ctx, "http://127.0.0.1:10002/hello", data, &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Delete 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    http::HttpResponse rsp;
    std::string data = "delete data";
    Status st = proxy->Delete(ctx, "http://127.0.0.1:10002/hello", std::move(data), &rsp);
    EXPECT_TRUE(st.OK());
    EXPECT_FALSE(rsp.IsOK());
    EXPECT_EQ("Delete 404 not found", rsp.GetContent());
    EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
    EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
    EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "delete data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "delete data";
    auto async_delete_fut =
        proxy->AsyncDelete(ctx, "http://127.0.0.1:10002/hello", data).Then([&reply](http::HttpResponse&& rsp) {
          EXPECT_FALSE(rsp.IsOK());
          EXPECT_EQ("Delete 404 not found", rsp.GetContent());
          EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
          EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
          EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

          return MakeReadyFuture<>();
        });
    future::BlockingGet(std::move(async_delete_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
  {
    proxy->SetReply(reply);
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    std::string data = "delete data";
    auto async_delete_fut = proxy->AsyncDelete(ctx, "http://127.0.0.1:10002/hello", std::move(data))
                                .Then([&reply](http::HttpResponse&& rsp) {
                                  EXPECT_FALSE(rsp.IsOK());
                                  EXPECT_EQ("Delete 404 not found", rsp.GetContent());
                                  EXPECT_EQ(rsp.GetStatus(), reply.GetStatus());
                                  EXPECT_EQ(rsp.GetHeader("trpc"), reply.GetHeader("trpc"));
                                  EXPECT_EQ(rsp.GetHeader("trpc"), "cpp");

                                  return MakeReadyFuture<>();
                                });
    future::BlockingGet(std::move(async_delete_fut));
    auto* req = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
    data = "delete data";
    EXPECT_EQ(req->request->GetMethodType(), http::OperationType::DELETE);
    EXPECT_EQ(req->request->GetVersion(), "1.1");
    EXPECT_EQ(*req->request->GetMutableUrl(), "/hello");
    EXPECT_EQ(req->request->GetContent(), data);
    EXPECT_EQ(req->request->GetHeader("Host"), "127.0.0.1:10002");
    EXPECT_EQ(req->request->GetHeader("Content-Type"), "application/json");
    EXPECT_EQ(req->request->GetHeader("Accept"), "*/*");
    EXPECT_EQ(req->request->GetHeader("Content-Length"), std::to_string(data.size()));
  }
}

TEST_F(HttpServiceProxyTest, PBInvoke) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::test::helloworld::HelloRequest req;
  req.set_msg("pb_req");
  trpc::test::helloworld::HelloReply rep;
  rep.set_msg("pb_rsp");

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(rep.SerializeAsString());
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);
  Status st = proxy->PBInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(
      ctx, "http://127.0.0.1:10002/pb_hello", req, &rep);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ("pb_rsp", rep.msg());

  HttpRequestProtocol* http_req_protocol = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(http_req_protocol->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(http_req_protocol->request->GetVersion(), "1.1");
  EXPECT_EQ(*http_req_protocol->request->GetMutableUrl(), "/pb_hello");
  //  EXPECT_EQ(http_req_protocol->request->GetContent(), "");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Length"), std::to_string(req.SerializeAsString().size()));
  EXPECT_EQ(http_req_protocol->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Type"), "application/pb");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Accept"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_pb_invoke_fut =
      proxy
          ->AsyncPBInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(
              ctx, "http://127.0.0.1:10002/pb_hello", req)
          .Then([](trpc::test::helloworld::HelloReply&& rep) {
            EXPECT_EQ("pb_rsp", rep.msg());
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(async_pb_invoke_fut));
  http_req_protocol = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(http_req_protocol->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(http_req_protocol->request->GetVersion(), "1.1");
  EXPECT_EQ(*http_req_protocol->request->GetMutableUrl(), "/pb_hello");
  //  EXPECT_EQ(http_req_protocol->request->GetContent(), "");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Length"), std::to_string(req.SerializeAsString().size()));
  EXPECT_EQ(http_req_protocol->request->GetHeader("Host"), "127.0.0.1:10002");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Type"), "application/pb");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Accept"), "");
}

TEST_F(HttpServiceProxyTest, PBInvokeFailed) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::test::helloworld::HelloRequest req;
  trpc::test::helloworld::HelloReply rep;

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  Status st = proxy->PBInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(
      ctx, "/pb_hello", req, &rep);
  EXPECT_FALSE(st.OK());
  EXPECT_TRUE(st.GetFuncRetCode() == reply.GetStatus());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_pb_invoke_fut =
      proxy
          ->AsyncPBInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(ctx, "/pb_hello",
                                                                                                    req)
          .Then([](Future<trpc::test::helloworld::HelloReply>&& rep) {
            EXPECT_TRUE(rep.IsFailed());
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(async_pb_invoke_fut));
}

TEST_F(HttpServiceProxyTest, HttpUnaryInvoke) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpRequest req;
  std::string unary_data = "{\"name\":\"tom\",\"height\":180}";
  req.SetMethodType(http::POST);
  req.SetVersion("1.1");
  req.SetUrl("/UnaryInvokeTest");
  req.SetHeader("Content-Type", "application/json");
  req.SetHeader("Content-Length", std::to_string(unary_data.size()));
  req.SetHeader("Accept", "*/*");
  req.SetContent(unary_data);

  trpc::http::HttpResponse rep;

  std::string str{"{\"age\":\"30\",\"height\": 180}"};
  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(str);
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  std::cout << "request: " << ctx->GetRequest() << std::endl;
  proxy->SetReply(reply);
  Status st = proxy->HttpUnaryInvoke<trpc::http::HttpRequest, trpc::http::HttpResponse>(ctx, req, &rep);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(reply.GetVersion(), rep.GetVersion());
  EXPECT_EQ(reply.GetStatus(), rep.GetStatus());
  EXPECT_EQ(str, rep.GetContent());

  HttpRequestProtocol* http_req_protocol = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(http_req_protocol->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(http_req_protocol->request->GetVersion(), "1.1");
  EXPECT_EQ(*http_req_protocol->request->GetMutableUrl(), "/UnaryInvokeTest");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Host"), "");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Length"), std::to_string(unary_data.size()));
  EXPECT_EQ(http_req_protocol->request->GetHeader("Accept"), "*/*");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_unary_invoke_fut =
      proxy->AsyncHttpUnaryInvoke<trpc::http::HttpRequest, trpc::http::HttpResponse>(ctx, req).Then(
          [&reply, &str](trpc::http::HttpResponse&& rep) {
            EXPECT_EQ(reply.GetVersion(), rep.GetVersion());
            EXPECT_EQ(reply.GetStatus(), rep.GetStatus());
            EXPECT_EQ(str, rep.GetContent());
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(async_unary_invoke_fut));

  http_req_protocol = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(http_req_protocol->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(http_req_protocol->request->GetVersion(), "1.1");
  EXPECT_EQ(*http_req_protocol->request->GetMutableUrl(), "/UnaryInvokeTest");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Host"), "");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Type"), "application/json");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Length"), std::to_string(unary_data.size()));
  EXPECT_EQ(http_req_protocol->request->GetHeader("Accept"), "*/*");
}

TEST_F(HttpServiceProxyTest, HttpUnaryInvokeFailed) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpRequest req;
  std::string unary_data = "{\"name\":\"tom\",\"height\":180}";
  req.SetMethodType(http::POST);
  req.SetVersion("1.1");
  req.SetUrl("/UnaryInvokeTest");
  req.SetHeader("Content-Type", "application/json");
  req.SetHeader("Content-Length", std::to_string(unary_data.size()));
  req.SetHeader("Accept", "*/*");
  req.SetContent(unary_data);

  trpc::http::HttpResponse rep;

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  Status st = proxy->HttpUnaryInvoke<trpc::http::HttpRequest, trpc::http::HttpResponse>(ctx, req, &rep);
  EXPECT_FALSE(st.OK());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  auto async_unary_invoke_fut =
      proxy->AsyncHttpUnaryInvoke<trpc::http::HttpRequest, trpc::http::HttpResponse>(ctx, req).Then(
          [&reply](Future<trpc::http::HttpResponse>&& d) {
            EXPECT_TRUE(d.IsFailed());
            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(async_unary_invoke_fut));
}

TEST_F(HttpServiceProxyTest, UnaryInvoke) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeRefCounted<ClientContext>(proxy->GetClientCodec());
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::test::helloworld::HelloRequest req;
  req.set_msg("http rpc unaryinvoke request");
  trpc::test::helloworld::HelloReply rep;

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  trpc::test::helloworld::HelloReply http_rep;
  http_rep.set_msg("http rpc unaryinvoke reply");
  {
    NoncontiguousBufferBuilder builder;
    builder.Append(http_rep.SerializeAsString());
    reply.SetNonContiguousBufferContent(builder.DestructiveGet());
  }

  proxy->SetReply(reply);
  std::cout << "request obj:" << ctx->GetRequest() << std::endl;
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
  ctx->SetHttpHeader("RpcUnaryInvoke_header_k_1", "RpcUnaryInvoke_header_v_1");
  Status st =
      proxy->UnaryInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(ctx, req, &rep);
  EXPECT_TRUE(st.OK());
  EXPECT_EQ(http_rep.msg(), rep.msg());

  auto* http_req_protocol = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(http_req_protocol->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(http_req_protocol->request->GetVersion(), "1.1");
  EXPECT_EQ(*http_req_protocol->request->GetMutableUrl(), "/trpc.test.helloworld.Greeter/SayHello");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Host"), "");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Type"), "application/pb");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Length"), std::to_string(req.SerializeAsString().size()));
  EXPECT_EQ(http_req_protocol->request->GetHeader("Accept"), "");

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
  auto async_unary_invoke_fut =
      proxy->AsyncUnaryInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(ctx, req).Then(
          [&http_rep](trpc::test::helloworld::HelloReply&& rep) {
            EXPECT_EQ(http_rep.msg(), rep.msg());

            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(async_unary_invoke_fut));

  http_req_protocol = static_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  EXPECT_EQ(http_req_protocol->request->GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(http_req_protocol->request->GetVersion(), "1.1");
  EXPECT_EQ(*http_req_protocol->request->GetMutableUrl(), "/trpc.test.helloworld.Greeter/SayHello");
  EXPECT_EQ(http_req_protocol->request->GetContent(), req.SerializeAsString());
  EXPECT_EQ(http_req_protocol->request->GetHeader("Host"), "");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Type"), "application/pb");
  EXPECT_EQ(http_req_protocol->request->GetHeader("Content-Length"), std::to_string(req.SerializeAsString().size()));
  EXPECT_EQ(http_req_protocol->request->GetHeader("Accept"), "");
}

TEST_F(HttpServiceProxyTest, UnaryInvokeFailed) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeRefCounted<ClientContext>(proxy->GetClientCodec());
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::test::helloworld::HelloRequest req;
  req.set_msg("http rpc unaryinvoke request");

  trpc::test::helloworld::HelloReply rep;

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kNotFound);
  reply.SetContent("404 not found");

  proxy->SetReply(reply);
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
  Status st =
      proxy->UnaryInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(ctx, req, &rep);
  EXPECT_FALSE(st.OK());

  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
  auto async_unary_invoke_fut =
      proxy->AsyncUnaryInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(ctx, req).Then(
          [&reply](Future<trpc::test::helloworld::HelloReply>&& d) {
            EXPECT_TRUE(d.IsFailed());

            return MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(async_unary_invoke_fut));
}

TEST_F(HttpServiceProxyTest, ConstructGetRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeRefCounted<ClientContext>(proxy->GetClientCodec());
  ctx->SetAddr("127.0.0.1", 10002);
  {
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructGetRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::GET, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
  {
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructGetRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::GET, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
  {
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructGetRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::GET, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructGetRequestUrlWithoutPort) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  // URL: Path + Query
  {
    std::string url = "http://127.0.0.1/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructGetRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::GET, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1", req.GetHeader("Host").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructHeadRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  // URL: Path + Query
  {
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructHeadRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::HEAD, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
  // URL: Path
  {
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructHeadRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::HEAD, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
  // Url: Path
  {
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructHeadRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::HEAD, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructOptionsRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructOptionsRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::OPTIONS, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
  {
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructOptionsRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::OPTIONS, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
  {
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructOptionsRequest(ctx, url, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;

    EXPECT_EQ(http::OperationType::OPTIONS, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructPostRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructPostRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::POST, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructPostRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::POST, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructPostRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::POST, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructPutRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructPutRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::PUT, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructPutRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::PUT, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructPutRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::PUT, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructDeleteRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructDeleteRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::DELETE, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructDeleteRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::DELETE, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructDeleteRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::DELETE, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructPatchRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;
    proxy->ConstructPatchRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::PATCH, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructPatchRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::PATCH, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructPatchRequest(ctx, url, data, &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(trpc::http::PATCH, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/json", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
    EXPECT_STREQ("*/*", req.GetHeader("Accept").c_str());
  }
}

TEST_F(HttpServiceProxyTest, ConstructPBRequest) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetAddr("127.0.0.1", 10002);
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello?a=b&c=d#ref";
    trpc::HttpRequestProtocol request_protocol;

    proxy->ConstructPBRequest(ctx, url, std::move(data), &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(http::OperationType::POST, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello?a=b&c=d#ref", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/pb", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/hello";
    trpc::HttpRequestProtocol request_protocol;
    ctx = MakeClientContext(proxy);
    ctx->SetAddr("127.0.0.1", 10002);
    proxy->ConstructPBRequest(ctx, url, std::move(data), &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(http::OperationType::POST, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/hello", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/pb", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
  }
  {
    std::string data;
    std::string url = "http://127.0.0.1:10002/";
    trpc::HttpRequestProtocol request_protocol;

    proxy->ConstructPBRequest(ctx, url, std::move(data), &request_protocol);
    trpc::http::HttpRequest& req = *request_protocol.request;
    EXPECT_EQ(http::OperationType::POST, req.GetMethodType());
    EXPECT_STREQ("1.1", req.GetVersion().c_str());
    EXPECT_STREQ("/", req.GetMutableUrl()->c_str());
    EXPECT_STREQ("127.0.0.1:10002", req.GetHeader("Host").c_str());
    EXPECT_STREQ("application/pb", req.GetHeader("Content-Type").c_str());
    EXPECT_STREQ("0", req.GetHeader("Content-Length").c_str());
  }
}

TEST_F(HttpServiceProxyTest, FilterExecWhenSuccess) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  proxy->SetReplyError(false);
  proxy->SetReply(reply);

  std::string rspStr;
  filter_->SetStatus(Status());
  auto st = proxy->GetString(ctx, "http://127.0.0.1:10002/hello", &rspStr);
  ASSERT_TRUE(st.OK());
  // verify that the status is OK upon entering the RPC post-filter point
  ASSERT_TRUE(filter_->GetStatus().OK());

  proxy->SetReplyError(false);
  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);
  filter_->SetStatus(Status());
  auto async_get_string_fut =
      proxy->AsyncGetString(ctx, "http://127.0.0.1:10002/hello").Then([](Future<std::string>&& future) {
        EXPECT_TRUE(future.IsReady());
        // verify that the status is OK upon entering the RPC post-filter point
        EXPECT_TRUE(filter_->GetStatus().OK());

        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_string_fut));
}

TEST_F(HttpServiceProxyTest, FilterExecWithFrameworkErr) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  proxy->SetReplyError(true);
  std::string rspStr;
  filter_->SetStatus(Status());
  auto st = proxy->GetString(ctx, "http://127.0.0.1:10002/hello", &rspStr);
  ASSERT_FALSE(st.OK());
  // verify that the status is already failed upon entering the RPC post-filter point
  ASSERT_FALSE(filter_->GetStatus().OK());

  proxy->SetReplyError(true);
  ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);
  filter_->SetStatus(Status());
  auto async_get_string_fut =
      proxy->AsyncGetString(ctx, "http://127.0.0.1:10002/hello").Then([](Future<std::string>&& future) {
        EXPECT_TRUE(future.IsFailed());
        // verify that the status is already failed upon entering the RPC post-filter point
        EXPECT_FALSE(filter_->GetStatus().OK());
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_string_fut));
}

TEST_F(HttpServiceProxyTest, FilterExecWithHttpErr) {
  auto proxy = std::make_shared<MockHttpServiceProxy>();
  proxy->SetMockServiceProxyOption(option_);
  auto ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);

  trpc::http::HttpResponse reply;
  reply.SetVersion("1.1");
  reply.SetStatus(trpc::http::HttpResponse::StatusCode::kForbidden);
  proxy->SetReplyError(false);
  proxy->SetReply(reply);

  std::string rspStr;
  filter_->SetStatus(Status());
  auto st = proxy->GetString(ctx, "http://127.0.0.1:10002/hello", &rspStr);
  ASSERT_FALSE(st.OK());
  // verify that the status is already failed upon entering the RPC post-filter point
  ASSERT_FALSE(filter_->GetStatus().OK());

  proxy->SetReplyError(false);
  proxy->SetReply(reply);
  ctx = MakeClientContext(proxy);
  ctx->SetStatus(Status());
  ctx->SetAddr("127.0.0.1", 10002);
  filter_->SetStatus(Status());
  auto async_get_string_fut =
      proxy->AsyncGetString(ctx, "http://127.0.0.1:10002/hello").Then([](Future<std::string>&& future) {
        EXPECT_TRUE(future.IsFailed());
        // verify that the status is already failed upon entering the RPC post-filter point
        EXPECT_FALSE(filter_->GetStatus().OK());
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(async_get_string_fut));
}

}  // namespace trpc::testing
