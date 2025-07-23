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

#include <gflags/gflags.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/redis/cmdgen.h"
#include "trpc/client/redis/redis_service_proxy.h"
#include "trpc/client/redis/reply.h"
#include "trpc/client/trpc_client.h"
#include "trpc/future/future_utility.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/log/trpc_log.h"

DEFINE_string(client_config, "future_client_client_config.yaml", "trpc cpp framework client_config file");

std::atomic<int> gProgramRet = 0;

void AsyncTestSet(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  trpc::ClientContextPtr set_ctx = trpc::MakeClientContext(proxy);
  auto fut = proxy->AsyncCommand(set_ctx, trpc::redis::cmdgen{}.set("trpc_cpp_redis_future", "support_redis"))
                 .Then([proxy](trpc::Future<trpc::redis::Reply>&& set_fut) {
                   if (set_fut.IsReady()) {
                     auto set_reply = std::get<0>(set_fut.GetValue());
                     if (set_reply.IsError()) {
                       // Redis-Server return error
                       std::cout << "Future Async call redis set command fail,error:" << set_reply.GetString()
                                 << std::endl;
                       return trpc::MakeExceptionFuture<>(trpc::CommonException(set_reply.GetString().c_str()));
                     }

                     // success
                     std::cout << "Future Async call redis set command success" << std::endl;
                     return trpc::MakeReadyFuture<>();
                   }

                   // RPC get error,maybe timeout
                   std::cout << "Future Async call redis set command fail,error:" << set_fut.GetException().what()
                             << std::endl;
                   return trpc::MakeExceptionFuture<>(trpc::CommonException(set_fut.GetException().what()));
                 });
  trpc::future::BlockingGet(std::move(fut));
}

void AsyncTestGet(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  trpc::ClientContextPtr get_ctx = trpc::MakeClientContext(proxy);
  auto fut =
      proxy->AsyncCommand(get_ctx, trpc::redis::cmdgen{}.get("trpc_cpp_redis_future"))
          .Then([proxy](trpc::Future<trpc::redis::Reply>&& get_fut) {
            if (get_fut.IsReady()) {
              auto get_reply = std::get<0>(get_fut.GetValue());
              if (get_reply.IsError()) {
                // Redis-Server return error
                std::cout << "Future Async call redis get command fail,error:" << get_reply.GetString() << std::endl;
                return trpc::MakeExceptionFuture<>(trpc::CommonException(get_reply.GetString().c_str()));
              } else if (get_reply.IsNil()) {
                // Redis-Server return Nil if not exists this redis-key
                std::cout << "Future Async call redis get command return Nil" << std::endl;
                return trpc::MakeExceptionFuture<>(trpc::CommonException("Return Nil"));
              }

              // success
              std::cout << "Future Async call redis get command success, reply:" << get_reply.GetString() << std::endl;
              return trpc::MakeReadyFuture<>();
            }

            // RPC get error,maybe timeout
            std::cout << "Future Async call redis get command fail,error:" << get_fut.GetException().what()
                      << std::endl;
            return trpc::MakeExceptionFuture<>(trpc::CommonException(get_fut.GetException().what()));
          });
  trpc::future::BlockingGet(std::move(fut));
}

void AsyncTest(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  AsyncTestSet(proxy);
  AsyncTestGet(proxy);
}

// Specify a connection for asynchronous parallel calls
void AsyncParalleTestFixedConnector(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  trpc::PreallocationOption option;
  auto connector_id = proxy->GetOrCreateConnector(option);
  if (!connector_id) {
    std::cout << "preallocate connector fail" << std::endl;
    gProgramRet = -1;
    return;
  }

  std::vector<trpc::Future<trpc::redis::Reply>> all_result;
  for (int i = 0; i < 5; i++) {
    trpc::ClientContextPtr get_ctx = trpc::MakeClientContext(proxy);
    get_ctx->SetFixedConnectorId(*connector_id);
    get_ctx->SetTimeout(1000);
    std::string key("trpc_cpp_redis_future_fix_connector_key");
    key.append(std::to_string(i));
    std::string cmd = trpc::redis::cmdgen{}.set(key, std::to_string(i));
    auto fut = proxy->AsyncCommand(get_ctx, cmd);
    all_result.emplace_back(std::move(fut));
  }
  std::vector<trpc::redis::Reply> replies;
  auto fut = trpc::WhenAll(all_result.begin(), all_result.end())
                 .Then([&replies](std::vector<trpc::Future<trpc::redis::Reply>>&& rlts) {
                   for (auto& result : rlts) {
                     if (result.IsFailed()) {
                       return trpc::MakeExceptionFuture<>(result.GetException());
                     } else {
                       replies.emplace_back(std::move(result.GetValue0()));
                     }
                   }

                   return trpc::MakeReadyFuture<>();
                 });
  fut = trpc::future::BlockingGet(std::move(fut));
  if (fut.IsFailed()) {
    std::cout << "AsyncParalleTestFixedConnector fail, error msg:" << fut.GetException().what() << std::endl;
  }
  for (auto&& reply : replies) {
    std::cout << "AsyncParalleTestFixedConnector success ."
              << "reply:" << reply << std::endl;
  }
  // Need release the connection regardless of whether the request was successful or not.
  proxy->ReleaseFixedConnector(*connector_id);
}

// Specify a connection for Redis Transaction
// Only support in Future/Promise.(Not support in Fiber)
void TestTransaction(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  // Specify a connection to complete Redis transaction requests
  // The combination of requests that make up a transaction can only be initiated in one thread.
  // Whether transaction are supported depends on the backend Redis Server
  trpc::ServiceProxyOption option = *(proxy->GetServiceProxyOption());
  option.name = "no_allow_reconnect_redis_server";
  option.allow_reconnect = false;
  auto transaction_proxy = trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>(option.name, &option);

  trpc::PreallocationOption pre_alloc_option;
  auto connector_id = transaction_proxy->GetOrCreateConnector(pre_alloc_option);

  if (!connector_id) {
    std::cout << "preallocate connector fail" << std::endl;
    gProgramRet = -1;
    return;
  }

  std::vector<trpc::Future<trpc::redis::Reply>> all_result;

  trpc::ClientContextPtr transaction_ctx = trpc::MakeClientContext(transaction_proxy);
  transaction_ctx->SetFixedConnectorId(*connector_id);
  auto fut1 = transaction_proxy->AsyncCommand(transaction_ctx, trpc::redis::cmdgen{}.multi());
  all_result.emplace_back(std::move(fut1));

  transaction_ctx = trpc::MakeClientContext(transaction_proxy);
  transaction_ctx->SetFixedConnectorId(*connector_id);
  auto fut2 = transaction_proxy->AsyncCommand(
      transaction_ctx, trpc::redis::cmdgen{}.set("trpc_cpp_redis_future_transaction_key1", "value_1"));
  all_result.emplace_back(std::move(fut2));

  transaction_ctx = trpc::MakeClientContext(transaction_proxy);
  transaction_ctx->SetFixedConnectorId(*connector_id);
  auto fut3 = transaction_proxy->AsyncCommand(
      transaction_ctx, trpc::redis::cmdgen{}.set("trpc_cpp_redis_future_transaction_key2", "value_2"));
  all_result.emplace_back(std::move(fut3));

  transaction_ctx = trpc::MakeClientContext(transaction_proxy);
  transaction_ctx->SetFixedConnectorId(*connector_id);
  auto fut4 = transaction_proxy->AsyncCommand(
      transaction_ctx, trpc::redis::cmdgen{}.set("trpc_cpp_redis_future_transaction_key3", "value_3"));
  all_result.emplace_back(std::move(fut4));

  transaction_ctx = trpc::MakeClientContext(transaction_proxy);
  transaction_ctx->SetFixedConnectorId(*connector_id);
  auto fut5 = transaction_proxy->AsyncCommand(
      transaction_ctx, trpc::redis::cmdgen{}.set("trpc_cpp_redis_future_transaction_key4", "value_4"));

  all_result.emplace_back(std::move(fut5));
  transaction_ctx = trpc::MakeClientContext(transaction_proxy);
  transaction_ctx->SetFixedConnectorId(*connector_id);
  auto fut6 = transaction_proxy->AsyncCommand(transaction_ctx,
                                              trpc::redis::cmdgen{}.get("trpc_cpp_redis_future_transaction_key3"));
  all_result.emplace_back(std::move(fut6));

  transaction_ctx = trpc::MakeClientContext(transaction_proxy);
  transaction_ctx->SetFixedConnectorId(*connector_id);
  auto fut7 = transaction_proxy->AsyncCommand(transaction_ctx, trpc::redis::cmdgen{}.exec());
  all_result.emplace_back(std::move(fut7));

  std::vector<trpc::redis::Reply> replies;
  auto fut = trpc::WhenAll(all_result.begin(), all_result.end())
                 .Then([&replies](std::vector<trpc::Future<trpc::redis::Reply>>&& rlts) {
                   for (auto& result : rlts) {
                     if (result.IsFailed()) {
                       return trpc::MakeExceptionFuture<>(result.GetException());
                     } else {
                       replies.emplace_back(std::move(result.GetValue0()));
                     }
                   }

                   return trpc::MakeReadyFuture<>();
                 });
  fut = trpc::future::BlockingGet(std::move(fut));
  if (fut.IsFailed()) {
    std::cout << "Transaction fail, error msg:" << fut.GetException().what() << std::endl;
    // Need release the connection regardless of whether the request was successful or not.
    proxy->ReleaseFixedConnector(*connector_id);
    gProgramRet = -1;
    return;
  }

  for (auto&& reply : replies) {
    std::cout << "Transaction success reply:" << reply << std::endl;
  }

  // Need release the connection regardless of whether the request was successful or not.
  proxy->ReleaseFixedConnector(*connector_id);
}

int Run() {
  // get RedisServiceProxy
  auto proxy = trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("redis_server");

  AsyncTest(proxy);

  AsyncParalleTestFixedConnector(proxy);

  TestTransaction(proxy);

  return gProgramRet;
}

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    exit(-1);
  }

  std::cout << "FLAGS_client_config:" << FLAGS_client_config << std::endl;

  int ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }
}

int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}
