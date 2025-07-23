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

#include "trpc/filter/rpc_filter.h"

#include <vector>

#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/proto/testing/helloworld_generated.h"
#include "trpc/server/server_context.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

namespace trpc::testing {

ClientContextPtr MakeClientContext() {
  auto trpc_codec = std::make_shared<TrpcClientCodec>();
  return MakeRefCounted<ClientContext>(trpc_codec);
}

void TestServerWithPb(std::shared_ptr<RpcServerFilter> p, std::string req_check,
                      std::string rsp_check) {
  auto ctx = trpc::MakeRefCounted<trpc::ServerContext>();
  trpc::ProtocolPtr req_msg = std::make_shared<trpc::TrpcRequestProtocol>();
  ctx->SetRequestMsg(std::move(req_msg));
  trpc::ProtocolPtr rsp_msg = std::make_shared<trpc::TrpcResponseProtocol>();
  ctx->SetResponseMsg(std::move(rsp_msg));

  trpc::test::helloworld::HelloRequest pb_req;
  trpc::test::helloworld::HelloReply pb_rsp;
  ctx->SetRequestData(&pb_req);
  ctx->SetResponseData(&pb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(pb_req.msg(), req_check);

  p->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(pb_rsp.msg(), rsp_check);
}

void TestClientWithPb(std::shared_ptr<RpcClientFilter> p, std::string req_check,
                      std::string rsp_check) {
  ASSERT_EQ(0, p->Init());

  ClientContextPtr ctx = MakeClientContext();
  ctx->SetReqEncodeType(TrpcContentEncodeType::TRPC_PROTO_ENCODE);

  trpc::test::helloworld::HelloRequest pb_req;
  trpc::test::helloworld::HelloReply pb_rsp;
  ctx->SetRequestData(&pb_req);
  ctx->SetResponseData(&pb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(pb_req.msg(), req_check);

  p->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(pb_rsp.msg(), rsp_check);
}

void SetFbsRequest(flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* req,
                   std::string msg) {
  flatbuffers::trpc::MessageBuilder mb_;
  auto msg_offset = mb_.CreateString(msg);
  auto hello_offset = trpc::test::helloworld::CreateFbRequest(mb_, msg_offset);
  mb_.Finish(hello_offset);
  *req = mb_.ReleaseMessage<trpc::test::helloworld::FbRequest>();
}

void SetFbsResponse(flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* rsp,
                    std::string msg) {
  flatbuffers::trpc::MessageBuilder mb_;
  auto msg_offset = mb_.CreateString(msg);
  auto hello_offset = trpc::test::helloworld::CreateFbReply(mb_, msg_offset);
  mb_.Finish(hello_offset);
  *rsp = mb_.ReleaseMessage<trpc::test::helloworld::FbReply>();
}

void TestServerWithFbs(std::shared_ptr<RpcServerFilter> p, std::string req_check,
                       std::string rsp_check) {
  auto ctx = trpc::MakeRefCounted<trpc::ServerContext>();
  trpc::ProtocolPtr req_msg = std::make_shared<trpc::TrpcRequestProtocol>();
  ctx->SetRequestMsg(std::move(req_msg));
  ctx->SetReqEncodeType(TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE);
  trpc::ProtocolPtr rsp_msg = std::make_shared<trpc::TrpcResponseProtocol>();
  ctx->SetResponseMsg(std::move(rsp_msg));

  flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest> fb_req;
  flatbuffers::trpc::Message<trpc::test::helloworld::FbReply> fb_rsp;
  SetFbsRequest(&fb_req, "");
  SetFbsResponse(&fb_rsp, "");
  ctx->SetRequestData(&fb_req);
  ctx->SetResponseData(&fb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(fb_req.GetRoot()->message()->str(), req_check);

  p->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(fb_rsp.GetRoot()->message()->str(), rsp_check);
}

void TestClientWithFbs(std::shared_ptr<RpcClientFilter> p, std::string req_check,
                       std::string rsp_check) {
  ASSERT_EQ(0, p->Init());

  ClientContextPtr ctx = MakeClientContext();
  ctx->SetReqEncodeType(TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE);

  flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest> fb_req;
  flatbuffers::trpc::Message<trpc::test::helloworld::FbReply> fb_rsp;
  flatbuffers::trpc::MessageBuilder mb_;
  SetFbsRequest(&fb_req, "");
  SetFbsResponse(&fb_rsp, "");
  ctx->SetRequestData(&fb_req);
  ctx->SetResponseData(&fb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(fb_req.GetRoot()->message()->str(), req_check);

  p->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(fb_rsp.GetRoot()->message()->str(), rsp_check);
}

void TestClientWithJson(std::shared_ptr<RpcClientFilter> p, std::string req_check,
                        std::string rsp_check) {
  ASSERT_EQ(0, p->Init());

  ClientContextPtr ctx = MakeClientContext();
  ctx->SetReqEncodeType(TrpcContentEncodeType::TRPC_JSON_ENCODE);

  rapidjson::Document json_req;
  json_req.SetObject();
  json_req.AddMember("msg", rapidjson::Value(""), json_req.GetAllocator());
  rapidjson::Document json_rsp;
  json_rsp.SetObject();
  json_rsp.AddMember("msg", rapidjson::Value(""), json_rsp.GetAllocator());
  ctx->SetRequestData(&json_req);
  ctx->SetResponseData(&json_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(json_req["msg"].GetString(), req_check);

  p->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(json_rsp["msg"].GetString(), rsp_check);
}

void TestClientWithNoop(std::shared_ptr<RpcClientFilter> p, std::string req_check,
                        std::string rsp_check) {
  ASSERT_EQ(0, p->Init());

  ClientContextPtr ctx = MakeClientContext();
  ctx->SetReqEncodeType(TrpcContentEncodeType::TRPC_NOOP_ENCODE);

  std::string noop_req = "";
  std::string noop_rsp = "";
  ctx->SetRequestData(&noop_req);
  ctx->SetResponseData(&noop_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(noop_req, req_check);

  p->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(noop_rsp, rsp_check);
}

class ServerRpcBasicFilter : public RpcServerFilter {
 public:
  std::string Name() override { return "server_rpc_basic_filter"; }
};

class ClientRpcBasicFilter : public RpcClientFilter {
 public:
  std::string Name() override { return "client_rpc_basic_filter"; }
};

TEST(RpcServerFilter, ServerRpcFilterBasicTest) {
  std::shared_ptr<RpcServerFilter> p = std::make_shared<ServerRpcBasicFilter>();

  TestServerWithPb(p, "", "");
  TestServerWithFbs(p, "", "");
}

TEST(RpcClientFilter, ClientRpcFilterBasicTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcBasicFilter>();
  ASSERT_EQ(0, p->Init());

  TestClientWithPb(p, "", "");
  TestClientWithFbs(p, "", "");
  TestClientWithJson(p, "", "");
  TestClientWithNoop(p, "", "");
}

class ServerRpcFailedFilter : public RpcServerFilter {
 public:
  std::string Name() override { return "server_rpc_failed_filter"; }

 protected:
  Status BeforeRpcInvoke(ServerContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    return Status(TrpcRetCode::TRPC_SERVER_ENCODE_ERR, "Error server BeforeRpcInvokeTest");
  }

  Status AfterRpcInvoke(ServerContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    return Status(TrpcRetCode::TRPC_SERVER_ENCODE_ERR, "Error server AfterRpcInvoke");
  }

  using RpcServerFilter::BeforeRpcInvoke;
  using RpcServerFilter::AfterRpcInvoke;
};

class ClientRpcFailedFilter : public RpcClientFilter {
 public:
  std::string Name() override { return "client_rpc_failed_filter"; }

 protected:
  Status BeforeRpcInvoke(ClientContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    return Status(TrpcRetCode::TRPC_SERVER_ENCODE_ERR, "Error client AfterRpcInvoke");
  }

  Status AfterRpcInvoke(ClientContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    return Status(TrpcRetCode::TRPC_SERVER_ENCODE_ERR, "Error client AfterRpcInvoke");
  }

  using RpcClientFilter::BeforeRpcInvoke;
  using RpcClientFilter::AfterRpcInvoke;
};

TEST(RpcServerFilter, ServerRpcFilterEncodeTypeErrorTest) {
  std::shared_ptr<RpcServerFilter> p = std::make_shared<ServerRpcFailedFilter>();
  auto ctx = trpc::MakeRefCounted<trpc::ServerContext>();
  trpc::ProtocolPtr req_msg = std::make_shared<trpc::TrpcRequestProtocol>();
  ctx->SetRequestMsg(std::move(req_msg));
  ctx->SetReqEncodeType(99);
  trpc::ProtocolPtr rsp_msg = std::make_shared<trpc::TrpcResponseProtocol>();
  ctx->SetResponseMsg(std::move(rsp_msg));

  trpc::test::helloworld::HelloRequest pb_req;
  trpc::test::helloworld::HelloReply pb_rsp;
  ctx->SetRequestData(&pb_req);
  ctx->SetResponseData(&pb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_req.msg(), "");

  p->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_rsp.msg(), "");
}

TEST(RpcClientFilter, ClientRpcFilterEncodeTypeErrorTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcFailedFilter>();
  ASSERT_EQ(0, p->Init());

  ClientContextPtr ctx = MakeClientContext();
  ctx->SetReqEncodeType(99);

  trpc::test::helloworld::HelloRequest pb_req;
  trpc::test::helloworld::HelloReply pb_rsp;
  ctx->SetRequestData(&pb_req);
  ctx->SetResponseData(&pb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_req.msg(), "");

  p->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_rsp.msg(), "");
}

TEST(RpcServerFilter, ServerRpcFailedFilterTest) {
  std::shared_ptr<RpcServerFilter> p = std::make_shared<ServerRpcFailedFilter>();
  auto ctx = trpc::MakeRefCounted<trpc::ServerContext>();
  trpc::ProtocolPtr req_msg = std::make_shared<trpc::TrpcRequestProtocol>();
  ctx->SetRequestMsg(std::move(req_msg));
  trpc::ProtocolPtr rsp_msg = std::make_shared<trpc::TrpcResponseProtocol>();
  ctx->SetResponseMsg(std::move(rsp_msg));

  trpc::test::helloworld::HelloRequest pb_req;
  trpc::test::helloworld::HelloReply pb_rsp;
  ctx->SetRequestData(&pb_req);
  ctx->SetResponseData(&pb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_req.msg(), "");

  p->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_rsp.msg(), "");
}

TEST(RpcClientFilter, ClientRpcFailedFilterTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcFailedFilter>();
  ASSERT_EQ(0, p->Init());

  ClientContextPtr ctx = MakeClientContext();
  ctx->SetReqEncodeType(TrpcContentEncodeType::TRPC_PROTO_ENCODE);

  trpc::test::helloworld::HelloRequest pb_req;
  trpc::test::helloworld::HelloReply pb_rsp;
  ctx->SetRequestData(&pb_req);
  ctx->SetResponseData(&pb_rsp);

  FilterStatus status;

  p->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_req.msg(), "");

  p->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  ASSERT_EQ(pb_rsp.msg(), "");
}

TEST(RpcServerFilter, ServerRpcFilterPointTest) {
  std::shared_ptr<RpcServerFilter> p = std::make_shared<ServerRpcFailedFilter>();
  std::vector<FilterPoint> points = p->GetFilterPoint();
  ASSERT_EQ(points.size(), 2);
  ASSERT_EQ(points[0], FilterPoint::SERVER_PRE_RPC_INVOKE);
  ASSERT_EQ(points[1], FilterPoint::SERVER_POST_RPC_INVOKE);
}

TEST(RpcClientFilter, ClientRpcFilterPointTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcFailedFilter>();
  std::vector<FilterPoint> points = p->GetFilterPoint();
  ASSERT_EQ(points.size(), 2);
  ASSERT_EQ(points[0], FilterPoint::CLIENT_PRE_RPC_INVOKE);
  ASSERT_EQ(points[1], FilterPoint::CLIENT_POST_RPC_INVOKE);
}

class ServerRpcTestFilter : public RpcServerFilter {
 public:
  std::string Name() override { return "server_rpc_test_filter"; }

 protected:
  Status BeforeRpcInvoke(ServerContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    auto req = static_cast<trpc::test::helloworld::HelloRequest*>(pb_req);
    req->set_msg("BeforeRpcInvoke");
    return kSuccStatus;
  }

  Status AfterRpcInvoke(ServerContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    auto rsp = static_cast<trpc::test::helloworld::HelloReply*>(pb_rsp);
    rsp->set_msg("AfterRpcInvoke");
    return kSuccStatus;
  }

  Status BeforeRpcInvoke(ServerContextPtr context, FbsMessage* fbs_req,
                         FbsMessage* fbs_rsp) override {
    auto req = static_cast<flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>*>(fbs_req);
    SetFbsRequest(req, "BeforeRpcInvoke");
    return kSuccStatus;
  }

  Status AfterRpcInvoke(ServerContextPtr context, FbsMessage* fbs_req, FbsMessage* fbs_rsp) {
    auto rsp = static_cast<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>*>(fbs_rsp);
    SetFbsResponse(rsp, "AfterRpcInvoke");
    return kSuccStatus;
  }

  using RpcServerFilter::BeforeRpcInvoke;
  using RpcServerFilter::AfterRpcInvoke;
};

class ClientRpcTestFilter : public RpcClientFilter {
 public:
  std::string Name() override { return "client_rpc_test_filter"; }

 protected:
  Status BeforeRpcInvoke(ClientContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    auto req = static_cast<trpc::test::helloworld::HelloRequest*>(pb_req);
    req->set_msg("BeforeRpcInvoke");
    return kSuccStatus;
  }

  Status AfterRpcInvoke(ClientContextPtr context, PbMessage* pb_req, PbMessage* pb_rsp) override {
    auto rsp = static_cast<trpc::test::helloworld::HelloReply*>(pb_rsp);
    rsp->set_msg("AfterRpcInvoke");
    return kSuccStatus;
  }

  Status BeforeRpcInvoke(ClientContextPtr context, FbsMessage* fbs_req,
                         FbsMessage* fbs_rsp) override {
    auto req = static_cast<flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>*>(fbs_req);
    flatbuffers::trpc::MessageBuilder mb_;
    auto msg_offset = mb_.CreateString("BeforeRpcInvoke");
    auto hello_offset = trpc::test::helloworld::CreateFbRequest(mb_, msg_offset);
    mb_.Finish(hello_offset);
    *req = mb_.ReleaseMessage<trpc::test::helloworld::FbRequest>();
    return kSuccStatus;
  }

  Status AfterRpcInvoke(ClientContextPtr context, FbsMessage* fbs_req, FbsMessage* fbs_rsp) {
    auto rsp = static_cast<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>*>(fbs_rsp);
    flatbuffers::trpc::MessageBuilder mb_;
    auto msg_offset = mb_.CreateString("AfterRpcInvoke");
    auto hello_offset = trpc::test::helloworld::CreateFbReply(mb_, msg_offset);
    mb_.Finish(hello_offset);
    *rsp = mb_.ReleaseMessage<trpc::test::helloworld::FbReply>();
    return kSuccStatus;
  }

  Status BeforeRpcInvoke(ClientContextPtr context, rapidjson::Document* req,
                         rapidjson::Document* rsp) {
    (*req)["msg"].SetString("BeforeRpcInvoke");
    return kSuccStatus;
  }

  Status AfterRpcInvoke(ClientContextPtr context, rapidjson::Document* req,
                        rapidjson::Document* rsp) {
    (*rsp)["msg"].SetString("AfterRpcInvoke");
    return kSuccStatus;
  }

  Status BeforeRpcInvoke(ClientContextPtr context, std::string* req, std::string* rsp) {
    *req = "BeforeRpcInvoke";
    return kSuccStatus;
  }

  Status AfterRpcInvoke(ClientContextPtr context, std::string* req, std::string* rsp) {
    *rsp = "AfterRpcInvoke";
    return kSuccStatus;
  }

  using RpcClientFilter::BeforeRpcInvoke;
  using RpcClientFilter::AfterRpcInvoke;
};

TEST(RpcServerFilter, PbRpcServerFilterTest) {
  std::shared_ptr<RpcServerFilter> p = std::make_shared<ServerRpcTestFilter>();
  TestServerWithPb(p, "BeforeRpcInvoke", "AfterRpcInvoke");
}

TEST(RpcServerFilter, FbsRpcServerFilterTest) {
  std::shared_ptr<RpcServerFilter> p = std::make_shared<ServerRpcTestFilter>();
  TestServerWithFbs(p, "BeforeRpcInvoke", "AfterRpcInvoke");
}

TEST(RpcClientFilter, PbRpcClientFilterTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcTestFilter>();
  TestClientWithPb(p, "BeforeRpcInvoke", "AfterRpcInvoke");
}

TEST(RpcClientFilter, FbsRpcClientFilterTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcTestFilter>();
  TestClientWithFbs(p, "BeforeRpcInvoke", "AfterRpcInvoke");
}

TEST(RpcClientFilter, JsonRpcClientFilterTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcTestFilter>();
  TestClientWithJson(p, "BeforeRpcInvoke", "AfterRpcInvoke");
}

TEST(RpcClientFilter, NoopRpcClientFilterTest) {
  std::shared_ptr<RpcClientFilter> p = std::make_shared<ClientRpcTestFilter>();
  TestClientWithNoop(p, "BeforeRpcInvoke", "AfterRpcInvoke");
}

}  // namespace trpc::testing
