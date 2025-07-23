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

#include "trpc/client/non_rpc_service_proxy.h"

#include <any>
#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/client/testing/service_proxy_testing.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/protocol.h"
#include "trpc/future/future_utility.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::testing {

void ConstructReqBuffer(NoncontiguousBuffer& buff) {
  std::string req_str("hello req test");
  size_t req_len = (req_str.size() + 1);

  NoncontiguousBufferBuilder builder;
  builder.Append(reinterpret_cast<const void*>(req_str.c_str()), req_len);
  buff = builder.DestructiveGet();
}

void ConstructRspBuffer(NoncontiguousBuffer& buff) {
  std::string rsp_str("hello rsp test");
  size_t rsp_len = (rsp_str.size() + 1);

  NoncontiguousBufferBuilder builder;
  builder.Append(reinterpret_cast<const void*>(rsp_str.c_str()), rsp_len);
  buff = builder.DestructiveGet();
}

// Self-defined request protocol
class SelfDefinedRequestProtocol : public Protocol {
 public:
  SelfDefinedRequestProtocol() {}
  ~SelfDefinedRequestProtocol() {}

  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override {
    buff = req_data;
    return true;
  }

  // Not used in the client, so just return false
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override { return false; }

  NoncontiguousBuffer GetNonContiguousProtocolBody() override { return std::move(req_data); }

  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buff) override { req_data = std::move(buff); }

 private:
  NoncontiguousBuffer req_data;
};

using SelfDefinedRequestProtocolPtr = std::shared_ptr<SelfDefinedRequestProtocol>;

// Self-defined response protocol
class SelfDefinedResponseProtocol : public Protocol {
 public:
  NoncontiguousBuffer GetNonContiguousProtocolBody() override { return std::move(rsp_data); }
  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buff) override { rsp_data = std::move(buff); }

  // Not used in the client, so just return false
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override { return false; }

  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override {
    rsp_data = buff;
    return true;
  }

 public:
  NoncontiguousBuffer rsp_data;
};

using SelfDefinedResponseProtocolPtr = std::shared_ptr<SelfDefinedResponseProtocol>;

// Self-defined codec
class SelfDefinedCodec : public ClientCodec {
 public:
  std::string Name() const { return "selfdefined"; }

  bool ZeroCopyDecode(const ClientContextPtr& context, std::any&& in, ProtocolPtr& out) override {
    auto buf = std::any_cast<NoncontiguousBuffer&&>(std::move(in));
    return out->ZeroCopyDecode(buf);
  }

  bool ZeroCopyEncode(const ClientContextPtr& context, const ProtocolPtr& in, NoncontiguousBuffer& out) override {
    return in->ZeroCopyEncode(out);
  }

  ProtocolPtr CreateRequestPtr() override { return std::make_shared<SelfDefinedRequestProtocol>(); }

  ProtocolPtr CreateResponsePtr() override { return std::make_shared<SelfDefinedResponseProtocol>(); }
};

class MockSelfProtocolServiceProxy : public NonRpcServiceProxy {
 public:
  MOCK_METHOD(Status, GetInvokeStatus, ());

  void UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) override {
    auto status = GetInvokeStatus();
    if (!status.OK()) {
      context->SetStatus(status);
      return;
    }

    ProtocolPtr rsp_protocol = codec_->CreateResponsePtr();
    NoncontiguousBuffer buff;
    ConstructRspBuffer(buff);
    codec_->ZeroCopyDecode(context, std::any(buff), rsp_protocol);
    rsp = rsp_protocol;
  }

  Future<ProtocolPtr> AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                const ProtocolPtr& req_protocol) override {
    auto status = GetInvokeStatus();
    if (!status.OK()) {
      context->SetStatus(status);
      return MakeExceptionFuture<ProtocolPtr>(CommonException(status.ErrorMessage().c_str()));
    }

    ProtocolPtr rsp_protocol = codec_->CreateResponsePtr();
    NoncontiguousBuffer buff;
    ConstructRspBuffer(buff);
    codec_->ZeroCopyDecode(context, std::any(buff), rsp_protocol);

    return MakeReadyFuture<ProtocolPtr>(std::move(rsp_protocol));
  }

  void SetServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) { SetServiceProxyOptionInner(option); }
};

using MockSelfProtocolServiceProxyPtr = std::shared_ptr<MockSelfProtocolServiceProxy>;

class NonRpcServiceProxyTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    // registry self-defined codec
    ClientCodecFactory::GetInstance()->Register(std::make_shared<SelfDefinedCodec>());
    RegisterPlugins();

    detail::SetDefaultOption(option_);
    option_->name = "thirdparty.test.helloworld.Greeter";
    option_->caller_name = "Test.BinaryClient";
    option_->codec_name = "selfdefined";
    option_->conn_type = "long";
    option_->network = "tcp";
    option_->timeout = 1000;
    option_->target = "127.0.0.1:10001";
    option_->selector_name = "direct";
  }

  static void TearDownTestCase() { UnregisterPlugins(); }

 protected:
  void SetUp() override {
    mock_service_proxy_ = std::make_shared<MockSelfProtocolServiceProxy>();
    mock_service_proxy_->SetServiceProxyOption(option_);
  }

  void TearDown() override {
    mock_service_proxy_->Stop();
    mock_service_proxy_->Destroy();
  }

  ClientContextPtr GetClientContext() {
    auto ctx = MakeClientContext(mock_service_proxy_);
    ctx->SetAddr("127.0.0.1", 10001);
    return ctx;
  }

 protected:
  static std::shared_ptr<ServiceProxyOption> option_;
  MockSelfProtocolServiceProxyPtr mock_service_proxy_{nullptr};
};

std::shared_ptr<ServiceProxyOption> NonRpcServiceProxyTestFixture::option_ = std::make_shared<ServiceProxyOption>();

SelfDefinedRequestProtocolPtr ConstructReq() {
  SelfDefinedRequestProtocolPtr req_protocol(new SelfDefinedRequestProtocol());
  NoncontiguousBuffer buff;
  ConstructReqBuffer(buff);
  req_protocol->SetNonContiguousProtocolBody(std::move(buff));
  return req_protocol;
}

// unary invoke ok
TEST_F(NonRpcServiceProxyTestFixture, UnaryInvokeOK) {
  EXPECT_CALL(*mock_service_proxy_, GetInvokeStatus()).Times(1).WillOnce(::testing::Return(kSuccStatus));

  auto client_context = GetClientContext();
  SelfDefinedRequestProtocolPtr req = ConstructReq();
  SelfDefinedResponseProtocolPtr rsp;
  auto status = mock_service_proxy_->UnaryInvoke<SelfDefinedRequestProtocolPtr, SelfDefinedResponseProtocolPtr>(
      client_context, req, rsp);
  ASSERT_TRUE(status.OK());

  // Get the decoded data
  NoncontiguousBuffer buff = rsp->GetNonContiguousProtocolBody();
  char rsp_data[256];
  FlattenToSlow(buff, rsp_data, buff.ByteSize());

  EXPECT_STREQ("hello rsp test", rsp_data);
}

// unary invoke error
TEST_F(NonRpcServiceProxyTestFixture, UnaryInvokeFail) {
  EXPECT_CALL(*mock_service_proxy_, GetInvokeStatus())
      .Times(1)
      .WillOnce(::testing::Return(Status(TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, 0, "invoke error")));

  auto client_context = GetClientContext();
  SelfDefinedRequestProtocolPtr req = ConstructReq();
  SelfDefinedResponseProtocolPtr rsp;
  auto status = mock_service_proxy_->UnaryInvoke<SelfDefinedRequestProtocolPtr, SelfDefinedResponseProtocolPtr>(
      client_context, req, rsp);
  EXPECT_TRUE(!status.OK());
  EXPECT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
}

// async invoke ok
TEST_F(NonRpcServiceProxyTestFixture, AsyncUnaryInvokeOK) {
  EXPECT_CALL(*mock_service_proxy_, GetInvokeStatus()).Times(1).WillOnce(::testing::Return(kSuccStatus));

  auto client_context = GetClientContext();
  SelfDefinedRequestProtocolPtr req = ConstructReq();
  // invoke ok
  auto fut = mock_service_proxy_
                 ->AsyncUnaryInvoke<SelfDefinedRequestProtocolPtr, SelfDefinedResponseProtocolPtr>(client_context, req)
                 .Then([client_context](Future<SelfDefinedResponseProtocolPtr>&& fut) {
                   EXPECT_TRUE(fut.IsReady());
                   Status status = client_context->GetStatus();
                   EXPECT_TRUE(status.OK());
                   auto rsp = fut.GetValue0();
                   // Get the decoded data
                   NoncontiguousBuffer buff = rsp->GetNonContiguousProtocolBody();
                   char rsp_data[256];
                   FlattenToSlow(buff, rsp_data, buff.ByteSize());

                   EXPECT_STREQ("hello rsp test", rsp_data);
                   return MakeReadyFuture<>();
                 });
  fut = future::BlockingGet(std::move(fut));
  EXPECT_TRUE(fut.IsReady());
}

// async invoke fail
TEST_F(NonRpcServiceProxyTestFixture, AsyncUnaryInvokeFail) {
  EXPECT_CALL(*mock_service_proxy_, GetInvokeStatus())
      .Times(1)
      .WillOnce(::testing::Return(Status(TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, 0, "invoke error")));

  auto client_context = GetClientContext();
  SelfDefinedRequestProtocolPtr req = ConstructReq();
  auto fut =
      mock_service_proxy_
          ->AsyncUnaryInvoke<SelfDefinedRequestProtocolPtr, SelfDefinedResponseProtocolPtr>(client_context, req)
          .Then([client_context](Future<SelfDefinedResponseProtocolPtr>&& fut) {
            EXPECT_TRUE(fut.IsFailed());
            Status status = client_context->GetStatus();
            EXPECT_TRUE(!status.OK());
            EXPECT_EQ(status.GetFrameworkRetCode(), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
            return MakeExceptionFuture<>(CommonException(status.ErrorMessage().c_str(), status.GetFrameworkRetCode()));
          });
  fut = future::BlockingGet(std::move(fut));
  EXPECT_TRUE(fut.IsFailed());
}

}  // namespace trpc::testing
