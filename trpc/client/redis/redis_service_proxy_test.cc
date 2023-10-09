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

#include "trpc/client/redis/redis_service_proxy.h"

#include <memory>
#include <sstream>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/redis/cmdgen.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/codec/redis/redis_client_codec.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/future/future_utility.h"

namespace trpc::testing {

class MockRedisCodec : public RedisClientCodec {
 public:
  MOCK_METHOD(bool, FillRequest, (const ClientContextPtr&, const ProtocolPtr&, void*), (override));

  MOCK_METHOD(bool, FillResponse, (const ClientContextPtr&, const ProtocolPtr&, void*), (override));

  MOCK_METHOD(bool, ZeroCopyDecode, (const ClientContextPtr&, std::any&&, ProtocolPtr&), (override));
};

class MockRedisServiceProxy : public trpc::redis::RedisServiceProxy {
 public:
  MOCK_METHOD(trpc::redis::Reply, GetReply, ());

  void UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) override {
    auto redis_rsp = GetReply();
    ProtocolPtr redis_protocol = codec_->CreateResponsePtr();
    codec_->ZeroCopyDecode(context, redis_rsp, redis_protocol);
    rsp = redis_protocol;
  }

  Future<ProtocolPtr> AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                const ProtocolPtr& req_protocol) override {
    auto redis_rsp = GetReply();
    ProtocolPtr redis_protocol = codec_->CreateResponsePtr();
    if (!codec_->ZeroCopyDecode(context, redis_rsp, redis_protocol)) {
      return MakeExceptionFuture<ProtocolPtr>(Exception());
    }
    if (context->GetStatus().OK()) {
      return MakeReadyFuture<ProtocolPtr>(std::move(redis_protocol));
    } else {
      return MakeExceptionFuture<ProtocolPtr>(Exception());
    }
  }

  void SetMockServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) {
    SetServiceProxyOptionInner(option);
  }

  void SetMockCodec(ClientCodecPtr&& codec) { codec_ = codec; }
};

using MockRedisServiceProxyPtr = std::shared_ptr<MockRedisServiceProxy>;

class RedisServiceProxyTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    TrpcPlugin::GetInstance()->RegisterPlugins();

    trpc::detail::SetDefaultOption(option_);
    option_->name = "default_redis_service";
    option_->caller_name = "";
    option_->codec_name = "redis";
    option_->conn_type = "long";
    option_->network = "tcp";
    option_->timeout = 1000;
    option_->target = "127.0.0.1:6379";
    option_->selector_name = "direct";
    option_->redis_conf.enable = true;
    option_->redis_conf.password = "my_redis123";
  }

  static void TearDownTestCase() { TrpcPlugin::GetInstance()->UnregisterPlugins(); }

 protected:
  void SetUp() override {
    mock_redis_service_proxy_ = std::make_shared<MockRedisServiceProxy>();
    mock_redis_service_proxy_->SetMockServiceProxyOption(option_);
  }

  void TearDown() {
    mock_redis_service_proxy_->Stop();
    mock_redis_service_proxy_->Destroy();
  }

  ClientContextPtr GetClientContext() {
    auto ctx = MakeClientContext(mock_redis_service_proxy_);
    ctx->SetTimeout(option_->timeout);
    ctx->SetAddr("127.0.0.1", 6379);
    return ctx;
  }

 protected:
  static std::shared_ptr<ServiceProxyOption> option_;
  MockRedisServiceProxyPtr mock_redis_service_proxy_{nullptr};
};

std::shared_ptr<ServiceProxyOption> RedisServiceProxyTest::option_ = std::make_shared<ServiceProxyOption>();

TEST_F(RedisServiceProxyTest, CommandCmdgen) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  trpc::redis::Reply rep;
  std::string cmd = trpc::redis::cmdgen{}.get("trpc");
  Status st = mock_redis_service_proxy_->Command(client_context, &rep, cmd);
  ASSERT_TRUE(st.OK());
  ASSERT_TRUE(rep.IsString());
  ASSERT_EQ("redis", rep.GetString());
}

TEST_F(RedisServiceProxyTest, CommandCmdgenMove) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  trpc::redis::Reply rep;
  Status st = mock_redis_service_proxy_->Command(client_context, &rep, std::move(trpc::redis::cmdgen{}.get("trpc")));
  ASSERT_TRUE(st.OK());
  ASSERT_TRUE(rep.IsString());
  ASSERT_EQ("redis", rep.GetString());
}

TEST_F(RedisServiceProxyTest, CommandCmdgenAsync) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  std::string cmd = trpc::redis::cmdgen{}.get("trpc");
  auto fut = mock_redis_service_proxy_->AsyncCommand(client_context, cmd).Then([](trpc::redis::Reply&& r) {
    EXPECT_TRUE(r.IsString());
    EXPECT_EQ("redis", r.GetString());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, CommandCmdgenAsyncMove) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  auto fut = mock_redis_service_proxy_->AsyncCommand(client_context, std::move(trpc::redis::cmdgen{}.get("trpc")))
                 .Then([](trpc::redis::Reply&& r) {
                   EXPECT_TRUE(r.IsString());
                   EXPECT_EQ("redis", r.GetString());
                   return MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, CommandCmdgenOneWay) {
  auto client_context = GetClientContext();
  std::string cmd = trpc::redis::cmdgen{}.get("trpc");
  Status st = mock_redis_service_proxy_->Command(client_context, cmd);
  ASSERT_TRUE(st.OK());
}

TEST_F(RedisServiceProxyTest, CommandCmdgenOneWayMove) {
  auto client_context = GetClientContext();
  Status st = mock_redis_service_proxy_->Command(client_context, std::move(trpc::redis::cmdgen{}.get("trpc")));
  ASSERT_TRUE(st.OK());
}

TEST_F(RedisServiceProxyTest, Command) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  trpc::redis::Reply rep;
  Status st = mock_redis_service_proxy_->Command(client_context, &rep, "get %s", "trpc");
  ASSERT_TRUE(st.OK());
  ASSERT_TRUE(rep.IsString());
  ASSERT_EQ("redis", rep.GetString());
}

TEST_F(RedisServiceProxyTest, CommandAsync) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  auto fut = mock_redis_service_proxy_->AsyncCommand(client_context, "get %s", "trpc").Then([](trpc::redis::Reply&& r) {
    EXPECT_TRUE(r.IsString());
    EXPECT_EQ("redis", r.GetString());
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, CommandFailed) {
  auto client_context = GetClientContext();
  trpc::redis::Reply rep;
  Status st = mock_redis_service_proxy_->Command(client_context, &rep, "get %la", 100);
  ASSERT_FALSE(st.OK());
}

TEST_F(RedisServiceProxyTest, CommandFailedAsync) {
  auto client_context = GetClientContext();
  auto fut =
      mock_redis_service_proxy_->AsyncCommand(client_context, "get %la", 100).Then([](Future<trpc::redis::Reply>&& r) {
        EXPECT_TRUE(r.IsFailed());
        return MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, CommandArgv) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  trpc::redis::Reply rep;
  trpc::redis::Request req;
  Status st = mock_redis_service_proxy_->CommandArgv(client_context, req, &rep);
  ASSERT_TRUE(st.OK());
  ASSERT_TRUE(rep.IsString());
  ASSERT_EQ("redis", rep.GetString());
}

TEST_F(RedisServiceProxyTest, CommandArgvMove) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  trpc::redis::Reply rep;
  trpc::redis::Request req;
  Status st = mock_redis_service_proxy_->CommandArgv(client_context, std::move(req), &rep);
  ASSERT_TRUE(st.OK());
  ASSERT_TRUE(rep.IsString());
  ASSERT_EQ("redis", rep.GetString());
}

TEST_F(RedisServiceProxyTest, CommandArgvAsync) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  trpc::redis::Request req;
  auto fut = mock_redis_service_proxy_->AsyncCommandArgv(client_context, req).Then([](trpc::redis::Reply&& r) {
    EXPECT_TRUE(r.IsString());
    EXPECT_EQ("redis", r.GetString());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, CommandArgvAsyncMove) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  auto client_context = GetClientContext();
  trpc::redis::Request req;
  auto fut = mock_redis_service_proxy_->AsyncCommandArgv(client_context, std::move(req)).Then([](trpc::redis::Reply&& r) {
    EXPECT_TRUE(r.IsString());
    EXPECT_EQ("redis", r.GetString());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, FillRequestFail) {
  std::string cmd = trpc::redis::cmdgen{}.get("trpc");
  mock_redis_service_proxy_->SetMockCodec(std::make_shared<MockRedisCodec>());
  MockRedisCodec* mock_ptr = static_cast<MockRedisCodec*>(mock_redis_service_proxy_->GetClientCodec().get());
  EXPECT_CALL(*mock_ptr, FillRequest(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(false));
  auto client_context = GetClientContext();
  auto fut = mock_redis_service_proxy_->AsyncCommand(client_context, cmd).Then([](Future<trpc::redis::Reply>&& r) {
    EXPECT_TRUE(r.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, DecodeFail) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  std::string cmd = trpc::redis::cmdgen{}.get("trpc");
  mock_redis_service_proxy_->SetMockCodec(std::make_shared<MockRedisCodec>());
  MockRedisCodec* mock_ptr = static_cast<MockRedisCodec*>(mock_redis_service_proxy_->GetClientCodec().get());
  EXPECT_CALL(*mock_ptr, FillRequest(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(true));
  EXPECT_CALL(*mock_ptr, ZeroCopyDecode(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(false));
  auto client_context = GetClientContext();
  auto fut = mock_redis_service_proxy_->AsyncCommand(client_context, cmd).Then([](Future<trpc::redis::Reply>&& r) {
    EXPECT_TRUE(r.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));
}

TEST_F(RedisServiceProxyTest, FillResponseFail) {
  trpc::redis::Reply reply = trpc::redis::Reply(trpc::redis::StringReplyMarker{}, "redis");
  EXPECT_CALL(*mock_redis_service_proxy_, GetReply()).Times(1).WillOnce(::testing::Return(reply));

  std::string cmd = trpc::redis::cmdgen{}.get("trpc");
  mock_redis_service_proxy_->SetMockCodec(std::make_shared<MockRedisCodec>());
  MockRedisCodec* mock_ptr = static_cast<MockRedisCodec*>(mock_redis_service_proxy_->GetClientCodec().get());
  EXPECT_CALL(*mock_ptr, FillRequest(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(true));
  EXPECT_CALL(*mock_ptr, ZeroCopyDecode(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(true));
  EXPECT_CALL(*mock_ptr, FillResponse(::testing::_, ::testing::_, ::testing::_)).WillOnce(::testing::Return(false));
  auto client_context = GetClientContext();
  auto fut = mock_redis_service_proxy_->AsyncCommand(client_context, cmd).Then([](Future<trpc::redis::Reply>&& r) {
    EXPECT_TRUE(r.IsFailed());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));
}

}  // namespace trpc::testing
