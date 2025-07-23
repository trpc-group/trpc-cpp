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

#include "gtest/gtest.h"

#include "test/end2end/common/subprocess.h"
#include "test/end2end/common/util.h"
#include "test/end2end/unary/redis/redis_server.h"
#include "trpc/client/redis/cmdgen.h"
#include "trpc/client/redis/formatter.h"
#include "trpc/client/redis/redis_service_proxy.h"
#include "trpc/future/future_utility.h"

int test_argc;
char** client_argv;
char** server_argv;

namespace trpc::testing {

class RedisClientTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    // run before first testcase
    test_signal_ = std::make_unique<TestSignaller>();
    server_process_ = std::make_unique<SubProcess>([]() {
      RedisServer server(test_signal_.get());
      server.Main(test_argc, server_argv);
      server.Wait();
    });

    test_signal_->ClientWaitToContinue();

    InitializeRuntimeEnv(test_argc, client_argv);
    proxy_ = trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("redis_client");
  }

  static void TearDownTestSuite() {
    // run after last testcase

    DestroyRuntimeEnv();
    server_process_.reset();
  }

  void SetUp() override {}

  void TearDown() override {}

 protected:
  static std::unique_ptr<TestSignaller> test_signal_;
  static std::unique_ptr<SubProcess> server_process_;

 public:
  static std::shared_ptr<::trpc::redis::RedisServiceProxy> proxy_;
};

std::unique_ptr<TestSignaller> RedisClientTest::test_signal_;
std::unique_ptr<SubProcess> RedisClientTest::server_process_;
std::shared_ptr<::trpc::redis::RedisServiceProxy> RedisClientTest::proxy_;

// valid redis server ip/port
constexpr auto valid_redis_server_address_ip = "0.0.0.0";
constexpr uint16_t valid_redis_server_address_port = 6379;

// invalid redis server ip/port
constexpr auto invalid_redis_server_address_ip = "0.0.0.0";
constexpr uint16_t invalid_redis_server_address_port = 8080;

std::shared_ptr<ServiceProxyOption> MakeAuthedRedisProxyOption() {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = std::make_shared<ServiceProxyOption>();

  authed_redis_proxy_option->name = "authed_redis_client";
  authed_redis_proxy_option->codec_name = "redis";
  authed_redis_proxy_option->timeout = 1000;
  authed_redis_proxy_option->selector_name = "direct";
  authed_redis_proxy_option->target =
      std::string(valid_redis_server_address_ip) + ":" + std::to_string(valid_redis_server_address_port);
  authed_redis_proxy_option->redis_conf.enable = true;
  authed_redis_proxy_option->redis_conf.password = "my_redis";

  return authed_redis_proxy_option;
}

std::shared_ptr<ServiceProxyOption> MakeDisableAuthedRedisProxyOption() {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = std::make_shared<ServiceProxyOption>();

  disable_authed_redis_proxy_option->name = "disable_auth_redis_client";
  disable_authed_redis_proxy_option->codec_name = "redis";
  disable_authed_redis_proxy_option->timeout = 1000;
  disable_authed_redis_proxy_option->selector_name = "direct";
  disable_authed_redis_proxy_option->target =
      std::string(valid_redis_server_address_ip) + ":" + std::to_string(valid_redis_server_address_port);
  disable_authed_redis_proxy_option->redis_conf.enable = false;

  return disable_authed_redis_proxy_option;
}

std::shared_ptr<ServiceProxyOption> MakeInvalidAuthedRedisProxyOption() {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = std::make_shared<ServiceProxyOption>();

  invalid_authed_redis_proxy_option->name = "invalid_auth_redis_client";
  invalid_authed_redis_proxy_option->codec_name = "redis";
  invalid_authed_redis_proxy_option->timeout = 1000;
  invalid_authed_redis_proxy_option->selector_name = "direct";
  invalid_authed_redis_proxy_option->target =
      std::string(valid_redis_server_address_ip) + ":" + std::to_string(valid_redis_server_address_port);
  invalid_authed_redis_proxy_option->redis_conf.enable = true;
  invalid_authed_redis_proxy_option->redis_conf.password = "invalid_redis_auth";

  return invalid_authed_redis_proxy_option;
}

// Case: Invalid Redis Server ip/port
// Expected: Failed
bool DoInvokeInvalidIpPort(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(invalid_redis_server_address_ip), invalid_redis_server_address_port);
  ::trpc::Status status =
      RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.set(redis_key, redis_value));

  return status.OK() && !reply.IsError();
}

// Case: send redis 'set' with interface Command to redis-server
// Expected: OK
bool DoSet(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status =
      RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.set(redis_key, redis_value));

  return status.OK() && !reply.IsError();
}

// Case: send redis 'set' with interface Command(right-value) to redis-server
// Expected: OK
bool DoSetRightValue(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  std::string cmd = ::trpc::redis::cmdgen{}.set(redis_key, redis_value);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, std::move(cmd));

  return status.OK() && !reply.IsError();
}

// Case: send redis 'set' with interface AsyncCommand to redis-server
// Expected: OK
bool DoAsyncSet(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.set(redis_key, redis_value))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis 'set' with interface AsyncCommand(right-value) to redis-server
// Expected: OK
bool DoAsyncRightValueSet(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  std::string cmd = ::trpc::redis::cmdgen{}.set(redis_key, redis_value);
  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, std::move(cmd))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid set command with interface Command to redis-server
// Expected: Failed
bool DoInvalidSetCommand(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, "invalid redis set command");

  return status.OK() && !reply.IsError();
}

// Case: send redis invalid set command with interface AsyncCommand to redis-server
// Expected: Failed
bool DoAsyncInvalidSetCommand(const std::string& redis_key, const std::string& redis_value,
                              ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, "invalid redis set command")
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with auth-info to redis-server
// Expected: OK
bool DoSetWithValidAuth(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status =
      RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.set(redis_key, redis_value));

  return status.OK() && !reply.IsError();
}

// Case: send redis command with auth-info to redis-server
// Expected: OK
bool DoAsyncSetWithValidAuth(const std::string& redis_key, const std::string& redis_value,
                             ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.set(redis_key, redis_value))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with disable-auth-info to redis-server
// Expected: Failed
bool DoSetWithDisableAuth(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();
  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status =
      RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.set(redis_key, redis_value));
  return status.OK() && !reply.IsError();
}

// Case: send redis command with disable-auth-info to redis-server
// Expected: Failed
bool DoAsyncSetWithDisableAuth(const std::string& redis_key, const std::string& redis_value,
                               ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.set(redis_key, redis_value))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with invalid-auth-info to redis-server
// Expected: Failed
bool DoSetWithInvalidAuth(const std::string& redis_key, const std::string& redis_value, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status =
      RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.set(redis_key, redis_value));
  return status.OK() && !reply.IsError();
}

// Case: send redis command with invalid-auth-info to redis-server
// Expected: Failed
bool DoAsyncSetWithInvalidAuth(const std::string& redis_key, const std::string& redis_value,
                               ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.set(redis_key, redis_value))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis get command to redis-server
// Expected: OK
bool DoGet(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.get(redis_key));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis get command to redis-server
// Expected: OK
bool DoAsyncGet(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.get(redis_key))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid get command to redis-server
// Expected: Failed
bool DoInvalidGetCommand(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, "invalid get command");

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis invalid get command to redis-server
// Expected: Failed
bool DoAsyncInvalidGetCommand(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, "invalid get command")
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with auth-info to redis-server
// Expected: OK
bool DoGetWithValidAuth(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.get(redis_key));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis command with auth-info to redis-server
// Expected: OK
bool DoAsyncGetWithValidAuth(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.get(redis_key))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with disable-auth-info to redis-server
// Expected: Failed
bool DoGetWithDisableAuth(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.get(redis_key));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis command with disable-auth-info to redis-server
// Expected: Failed
bool DoAsyncGetWithDisableAuth(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.get(redis_key))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with invalid-auth-info to redis-server
// Expected: Failed
bool DoGetWithInvalidAuth(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.get(redis_key));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis command with invalid-auth-info to redis-server
// Expected: Failed
bool DoAsyncGetWithInvalidAuth(const std::string& redis_key, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.get(redis_key))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis mset command to redis-server
// Expected: OK
bool DoMset(std::vector<std::pair<std::string, std::string>>& key_values, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mset(key_values));

  return status.OK() && !reply.IsError();
}

// Case: send redis mset command to redis-server
// Expected: OK
bool DoAsyncMset(std::vector<std::pair<std::string, std::string>>& key_values, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mset(key_values))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid mset command to redis-server
// Expected: Failed
bool DoInvalidMsetCommand(std::vector<std::pair<std::string, std::string>>& key_values, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, "invalid redis mset command");

  return status.OK() && !reply.IsError();
}

// Case: send redis invalid mset command to redis-server
// Expected: Failed
bool DoAsyncInvalidMsetCommand(std::vector<std::pair<std::string, std::string>>& key_values,
                               ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, "invalid redis mset command")
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with auth-info to redis-server
// Expected: OK
bool DoMsetWithValidAuth(std::vector<std::pair<std::string, std::string>>& key_values, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mset(key_values));

  return status.OK() && !reply.IsError();
}

// Case: send redis command with auth-info to redis-server
// Expected: OK
bool DoAsyncMsetWithValidAuth(std::vector<std::pair<std::string, std::string>>& key_values,
                              ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mset(key_values))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with disable-auth-info to redis-server
// Expected: Failed
bool DoMsetWithDisableAuth(std::vector<std::pair<std::string, std::string>>& key_values, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mset(key_values));

  return status.OK() && !reply.IsError();
}

// Case: send redis command with disable-auth-info to redis-server
// Expected: Failed
bool DoAsyncMsetWithDisableAuth(std::vector<std::pair<std::string, std::string>>& key_values,
                                ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mset(key_values))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis command with invalid-auth-info to redis-server
// Expected: Failed
bool DoMsetWithInvalidAuth(std::vector<std::pair<std::string, std::string>>& key_values, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mset(key_values));

  return status.OK() && !reply.IsError();
}

// Case: send redis command with invalid-auth-info to redis-server
// Expected: Failed
bool DoAsyncMsetWithInvalidAuth(std::vector<std::pair<std::string, std::string>>& key_values,
                                ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mset(key_values))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis mget command to redis-server
// Expected: OK
bool DoMget(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mget(redis_keys));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis mget command to redis-server
// Expected: OK
bool DoAsyncMget(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mget(redis_keys))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid mget command to redis-server
// Expected: Failed
bool DoInvalidMgetCommand(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, "invalid mget command");

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis invalid mget command to redis-server
// Expected: Failed
bool DoAsyncInvalidMgetCommand(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, "invalid mget command")
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis mget command with auth-info to redis-server
// Expected: OK
bool DoMgetWithValidAuth(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mget(redis_keys));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis mget command with auth-info to redis-server
// Expected: OK
bool DoAsyncMgetWithValidAuth(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> authed_redis_proxy_option = MakeAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("authed_redis_client",
                                                                      authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mget(redis_keys))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis mget command with disable-auth-info to redis-server
// Expected: Failed
bool DoMgetWithDisableAuth(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mget(redis_keys));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis mget command with disable-auth-info to redis-server
// Expected: Failed
bool DoAsyncMgetWithDisableAuth(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> disable_authed_redis_proxy_option = MakeDisableAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("disable_auth_redis_client",
                                                                      disable_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mget(redis_keys))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis mget command with invalid-auth-info to redis-server
// Expected: Failed
bool DoMgetWithInvalidAuth(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, ::trpc::redis::cmdgen{}.mget(redis_keys));

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis mget command with invalid-auth-info to redis-server
// Expected: Failed
bool DoAsyncMgetWithInvalidAuth(std::vector<std::string> redis_keys, ::trpc::redis::Reply& reply) {
  std::shared_ptr<ServiceProxyOption> invalid_authed_redis_proxy_option = MakeInvalidAuthedRedisProxyOption();

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("invalid_auth_redis_client",
                                                                      invalid_authed_redis_proxy_option.get());

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, ::trpc::redis::cmdgen{}.mget(redis_keys))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoInvalidCommand(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, "invalid command");

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoAsyncInvalidCommand(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, "invalid command")
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send multi redis command as pipeline to redis-server
// Expected: OK
bool DoSupportPipeline(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  // pipeline start
  std::string support_pipeline_cmds;
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_1", "value_1"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_2", "value_2"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_3", "value_2"));

  ctx->SetPipelineCount(3);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, support_pipeline_cmds);

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send multi redis command as pipeline to redis-server
// Expected: OK
bool DoAsyncSupportPipeline(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  // pipeline start
  std::string support_pipeline_cmds;
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_1", "value_1"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_2", "value_2"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_3", "value_2"));

  ctx->SetPipelineCount(3);

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, support_pipeline_cmds)
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send multi redis command but not as pipeline to redis-server
// Expected: May failed
bool DoInvalidSupportPipeline(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  // pipeline start
  std::string support_pipeline_cmds;
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_1", "value_1"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_2", "value_2"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_3", "value_2"));

  // Donnot PipelineCount,such as ctx->SetPipelineCount(3);

  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, &reply, support_pipeline_cmds);

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send multi redis command but not as pipeline to redis-server
// Expected: May failed
bool DoAsyncInvalidSupportPipeline(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  // pipeline start
  std::string support_pipeline_cmds;
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_1", "value_1"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_2", "value_2"));
  support_pipeline_cmds.append(trpc::redis::cmdgen{}.set("support_pipeline_test_3", "value_2"));

  // Donnot PipelineCount,such as ctx->SetPipelineCount(3);
  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommand(ctx, support_pipeline_cmds)
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoInvalidCommandArgv(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  trpc::redis::Request redis_req;
  redis_req.do_RESP_ = false;
  redis_req.params_.push_back("invalid commandargv");
  ::trpc::Status status = RedisClientTest::proxy_->CommandArgv(ctx, redis_req, &reply);
  std::cout << "DoInvalidCommandArgv:" << reply.GetString() << std::endl;
  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoInvalidRightValueCommandArgv(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  trpc::redis::Request redis_req;
  redis_req.do_RESP_ = false;
  redis_req.params_.push_back("invalid commandargv");
  ::trpc::Status status = RedisClientTest::proxy_->CommandArgv(ctx, std::move(redis_req), &reply);

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoAsyncInvalidCommandArgv(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  trpc::redis::Request redis_req;
  redis_req.do_RESP_ = false;
  redis_req.params_.push_back("invalid commandargv");

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommandArgv(ctx, redis_req)
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoAsyncInvalidRightValueCommandArgv(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  trpc::redis::Request redis_req;
  redis_req.do_RESP_ = false;
  redis_req.params_.push_back("invalid commandargv");

  bool succ = false;
  auto fut = RedisClientTest::proxy_->AsyncCommandArgv(ctx, std::move(redis_req))
                 .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
                   succ = !fut_redis_reply.IsFailed();
                   if (succ) {
                     reply = std::get<0>(fut_redis_reply.GetValue());
                     succ = !reply.IsError();
                   }
                   return ::trpc::MakeReadyFuture<>();
                 });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoFormatCommand(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  ::trpc::Status status =
      RedisClientTest::proxy_->Command(ctx, &reply, "SET %s %s", "format_command_key", "format_command_value");

  if (!status.OK() || reply.IsError() || reply.IsNil()) {
    return false;
  }

  return true;
}

// Case: send redis invalid command to redis-server
// Expected: Failed
bool DoAsyncFormatCommand(::trpc::redis::Reply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);

  bool succ = false;
  auto fut =
      RedisClientTest::proxy_->AsyncCommand(ctx, "SET %s %s", "async_format_command_key", "async_format_command_value")
          .Then([&](trpc::Future<trpc::redis::Reply>&& fut_redis_reply) {
            succ = !fut_redis_reply.IsFailed();
            if (succ) {
              reply = std::get<0>(fut_redis_reply.GetValue());
              succ = !reply.IsError();
            }
            return ::trpc::MakeReadyFuture<>();
          });
  future::BlockingGet(std::move(fut));

  return succ;
}

// Case: send redis oneway command to redis-server
// Expected: OK
bool DoOnewayCommand(const std::string& redis_key, const std::string& redis_value) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  std::string cmd = ::trpc::redis::cmdgen{}.set(redis_key, redis_value);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, cmd);

  return status.OK();
}

// Case: send redis oneway command to redis-server
// Expected: OK
bool DoOnewayRightValueCommand(const std::string& redis_key, const std::string& redis_value) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx->SetAddr(std::string(valid_redis_server_address_ip), valid_redis_server_address_port);
  std::string cmd = ::trpc::redis::cmdgen{}.set(redis_key, redis_value);
  ::trpc::Status status = RedisClientTest::proxy_->Command(ctx, std::move(cmd));

  return status.OK();
}

// Case: send redis command use connection-support_pipeline to redis-server
// Expected: OK
bool DoConnectionPipeline() {
  std::shared_ptr<ServiceProxyOption> connection_pipeline_proxy_option = std::make_shared<ServiceProxyOption>();

  connection_pipeline_proxy_option->name = "connection_pipeline_client";
  connection_pipeline_proxy_option->codec_name = "redis";
  connection_pipeline_proxy_option->timeout = 60000;
  connection_pipeline_proxy_option->selector_name = "direct";
  connection_pipeline_proxy_option->target =
      std::string(valid_redis_server_address_ip) + ":" + std::to_string(valid_redis_server_address_port);
  connection_pipeline_proxy_option->support_pipeline = true;

  std::shared_ptr<::trpc::redis::RedisServiceProxy> authed_redis_proxy_ =
      trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("connection_pipeline_client",
                                                                      connection_pipeline_proxy_option.get());

  ::trpc::ClientContextPtr ctx1 = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx1->SetAddr(valid_redis_server_address_ip, valid_redis_server_address_port);
  trpc::redis::Reply reply1;
  ::trpc::Status status1 = RedisClientTest::proxy_->Command(
      ctx1, &reply1, ::trpc::redis::cmdgen{}.set("connection_pipeline_key_1", "connection_pipeline_value_1"));

  ::trpc::ClientContextPtr ctx2 = ::trpc::MakeClientContext(RedisClientTest::proxy_);
  ctx2->SetAddr(valid_redis_server_address_ip, valid_redis_server_address_port);
  trpc::redis::Reply reply2;
  ::trpc::Status status2 = RedisClientTest::proxy_->Command(
      ctx2, &reply2, ::trpc::redis::cmdgen{}.set("connection_pipeline_key_2", "connection_pipeline_value_2"));

  return status1.OK() && status2.OK();
}

TEST_F(RedisClientTest, TestDoInvokeInvalidIpPort) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_FALSE(DoInvokeInvalidIpPort("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoSet) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_TRUE(DoSet("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoSetRightValue) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_TRUE(DoSetRightValue("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncSet) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_TRUE(DoAsyncSet("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncRightValueSet) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_TRUE(DoAsyncRightValueSet("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoSetInvalidSetCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_FALSE(DoInvalidSetCommand("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidSetCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_FALSE(
        DoAsyncInvalidSetCommand("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoSetWithValidAuth) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_TRUE(DoSetWithValidAuth("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncSetWithValidAuth) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    ASSERT_TRUE(DoAsyncSetWithValidAuth("redis_client_end2end_test_key", "redis_client_end2end_test_value",
    set_reply));
  });
}

TEST_F(RedisClientTest, TestDoSetWithDisableAuth) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    DoSetWithDisableAuth("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply);
  });
}

TEST_F(RedisClientTest, TestDoAsyncSetWithDisableAuth) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    DoAsyncSetWithDisableAuth("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply);
  });
}

TEST_F(RedisClientTest, TestDoSetWithInvalidAuth) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    DoSetWithInvalidAuth("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply);
  });
}

TEST_F(RedisClientTest, TestDoAsyncSetWithInvalidAuth) {
  RunByEnv([&]() {
    trpc::redis::Reply set_reply;
    DoAsyncSetWithInvalidAuth("redis_client_end2end_test_key", "redis_client_end2end_test_value", set_reply);
  });
}

TEST_F(RedisClientTest, TestDoGet) {
  RunByEnv([&]() {
    trpc::redis::Reply get_reply;
    ASSERT_TRUE(DoGet("redis_client_end2end_test_key", get_reply));
    ASSERT_FALSE(get_reply.GetString().empty());
  });
}

TEST_F(RedisClientTest, TestDoGetNotExistKey) {
  RunByEnv([&]() {
    trpc::redis::Reply get_reply;
    ASSERT_FALSE(DoGet("redis_client_end2end_test_not_exist_key", get_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncGet) {
  RunByEnv([&]() {
    trpc::redis::Reply get_reply;
    ASSERT_TRUE(DoAsyncGet("redis_client_end2end_test_key", get_reply));
    ASSERT_FALSE(get_reply.GetString().empty());
  });
}

TEST_F(RedisClientTest, TestDoAsyncGetNotExistKey) {
  RunByEnv([&]() {
    trpc::redis::Reply get_reply;
    ASSERT_TRUE(DoAsyncGet("redis_client_end2end_test_not_exist_key", get_reply));
    ASSERT_TRUE(get_reply.IsNil());
  });
}

TEST_F(RedisClientTest, TestDoInvalidGetCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply get_reply;
    ASSERT_FALSE(DoInvalidGetCommand("redis_client_end2end_test_key", get_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidGetCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply get_reply;
    ASSERT_FALSE(DoAsyncInvalidGetCommand("redis_client_end2end_test_key", get_reply));
  });
}

TEST_F(RedisClientTest, TestDoMset) {
  RunByEnv([&]() {
    std::vector<std::pair<std::string, std::string>> key_values;
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_1", "redis_client_end2end_test_value_1"));
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_2", "redis_client_end2end_test_value_2"));
    trpc::redis::Reply mset_reply;
    ASSERT_TRUE(DoMset(key_values, mset_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncMset) {
  RunByEnv([&]() {
    std::vector<std::pair<std::string, std::string>> key_values;
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_1", "redis_client_end2end_test_value_1"));
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_2", "redis_client_end2end_test_value_2"));
    trpc::redis::Reply mset_reply;
    ASSERT_TRUE(DoAsyncMset(key_values, mset_reply));
  });
}

TEST_F(RedisClientTest, TestDoMsetInvalidSetCommand) {
  RunByEnv([&]() {
    std::vector<std::pair<std::string, std::string>> key_values;
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_1", "redis_client_end2end_test_value_1"));
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_2", "redis_client_end2end_test_value_2"));
    trpc::redis::Reply mset_reply;
    ASSERT_FALSE(DoInvalidMsetCommand(key_values, mset_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidMsetCommand) {
  RunByEnv([&]() {
    std::vector<std::pair<std::string, std::string>> key_values;
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_1", "redis_client_end2end_test_value_1"));
    key_values.push_back(std::make_pair("redis_client_end2end_test_key_2", "redis_client_end2end_test_value_2"));
    trpc::redis::Reply mset_reply;
    ASSERT_FALSE(DoAsyncInvalidMsetCommand(key_values, mset_reply));
  });
}

TEST_F(RedisClientTest, TestDoMget) {
  RunByEnv([&]() {
    std::vector<std::string> keys;
    keys.push_back("redis_client_end2end_test_key_1");
    keys.push_back("redis_client_end2end_test_key_2");
    trpc::redis::Reply mget_reply;
    ASSERT_TRUE(DoMget(keys, mget_reply));
    ASSERT_FALSE(mget_reply.GetArray().at(0).GetString().empty());
    ASSERT_FALSE(mget_reply.GetArray().at(1).GetString().empty());
  });
}

TEST_F(RedisClientTest, TestDoMgetNotExistKey) {
  RunByEnv([&]() {
    std::vector<std::string> keys;
    keys.push_back("redis_client_end2end_test_not_exist_key_1");
    keys.push_back("redis_client_end2end_test_not_exist_key_2");
    trpc::redis::Reply mget_reply;
    ASSERT_TRUE(DoMget(keys, mget_reply));
    ASSERT_TRUE(mget_reply.GetArray().at(0).IsNil());
    ASSERT_TRUE(mget_reply.GetArray().at(1).IsNil());
  });
}

TEST_F(RedisClientTest, TestDoAsyncMget) {
  RunByEnv([&]() {
    std::vector<std::string> keys;
    keys.push_back("redis_client_end2end_test_key_1");
    keys.push_back("redis_client_end2end_test_key_2");
    trpc::redis::Reply mget_reply;
    ASSERT_TRUE(DoAsyncMget(keys, mget_reply));
    ASSERT_FALSE(mget_reply.GetArray().at(0).GetString().empty());
    ASSERT_FALSE(mget_reply.GetArray().at(1).GetString().empty());
  });
}

TEST_F(RedisClientTest, TestDoAsyncMgetNotExistKey) {
  RunByEnv([&]() {
    std::vector<std::string> keys;
    keys.push_back("redis_client_end2end_test_not_exist_key_1");
    keys.push_back("redis_client_end2end_test_not_exist_key_2");
    trpc::redis::Reply mget_reply;
    ASSERT_TRUE(DoAsyncMget(keys, mget_reply));
    ASSERT_TRUE(mget_reply.GetArray().at(0).IsNil());
    ASSERT_TRUE(mget_reply.GetArray().at(1).IsNil());
  });
}

TEST_F(RedisClientTest, TestDoInvalidMgetCommand) {
  RunByEnv([&]() {
    std::vector<std::string> keys;
    keys.push_back("redis_client_end2end_test_key_1");
    keys.push_back("redis_client_end2end_test_key_2");
    trpc::redis::Reply mget_reply;
    ASSERT_FALSE(DoInvalidMgetCommand(keys, mget_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidMgetCommand) {
  RunByEnv([&]() {
    std::vector<std::string> keys;
    keys.push_back("redis_client_end2end_test_key_1");
    keys.push_back("redis_client_end2end_test_key_2");
    trpc::redis::Reply mget_reply;
    ASSERT_FALSE(DoAsyncInvalidMgetCommand(keys, mget_reply));
  });
}

TEST_F(RedisClientTest, TestDoInvalidCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply invalid_redis_command_reply;
    ASSERT_FALSE(DoInvalidCommand(invalid_redis_command_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply invalid_redis_command_reply;
    ASSERT_FALSE(DoAsyncInvalidCommand(invalid_redis_command_reply));
  });
}

TEST_F(RedisClientTest, TestDoSupportPipeline) {
  RunByEnv([&]() {
    trpc::redis::Reply support_pipeline_reply;
    ASSERT_TRUE(DoSupportPipeline(support_pipeline_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncSupportPipeline) {
  RunByEnv([&]() {
    trpc::redis::Reply support_pipeline_reply;
    ASSERT_TRUE(DoAsyncSupportPipeline(support_pipeline_reply));
  });
}

TEST_F(RedisClientTest, TestDoInvalidSupportPipeline) {
  RunByEnv([&]() {
    trpc::redis::Reply invalid_support_pipeline_reply;
    DoInvalidSupportPipeline(invalid_support_pipeline_reply);
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidSupportPipeline) {
  RunByEnv([&]() {
    trpc::redis::Reply invalid_support_pipeline_reply;
    DoAsyncInvalidSupportPipeline(invalid_support_pipeline_reply);
  });
}

TEST_F(RedisClientTest, TestDoInvalidRightValueCommandArgv) {
  RunByEnv([&]() {
    trpc::redis::Reply invalid_commandargv_reply;
    ASSERT_FALSE(DoInvalidRightValueCommandArgv(invalid_commandargv_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidCommandArgv) {
  RunByEnv([&]() {
    trpc::redis::Reply invalid_commandargv_reply;
    ASSERT_FALSE(DoAsyncInvalidCommandArgv(invalid_commandargv_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncInvalidRightValueCommandArgv) {
  RunByEnv([&]() {
    trpc::redis::Reply invalid_commandargv_reply;
    ASSERT_FALSE(DoAsyncInvalidRightValueCommandArgv(invalid_commandargv_reply));
  });
}

TEST_F(RedisClientTest, TestDoFormatCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply format_command_reply;
    ASSERT_TRUE(DoFormatCommand(format_command_reply));
  });
}

TEST_F(RedisClientTest, TestDoAsyncFormatCommand) {
  RunByEnv([&]() {
    trpc::redis::Reply format_command_reply;
    ASSERT_TRUE(DoAsyncFormatCommand(format_command_reply));
  });
}

TEST_F(RedisClientTest, TestDoOnewayCommand) {
  RunByEnv([&]() {
    ASSERT_TRUE(DoOnewayCommand("redis_client_end2end_test_oneway_key", "redis_client_end2end_test_oneway_value"));
  });
}

TEST_F(RedisClientTest, TestDoOnewayRightValueCommand) {
  RunByEnv([&]() {
    ASSERT_TRUE(
        DoOnewayRightValueCommand("redis_client_end2end_test_oneway_key", "redis_client_end2end_test_oneway_value"));
  });
}

TEST_F(RedisClientTest, TestDoConnectionPipeline) {
  RunByEnv([&]() { ASSERT_TRUE(DoConnectionPipeline()); });
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!::trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
