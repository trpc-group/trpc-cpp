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

#include "trpc/transport/common/io_handler_manager.h"

#include "trpc/stream/grpc/grpc_io_handler.h"
#include "trpc/transport/client/common/client_io_handler_factory.h"
#include "trpc/transport/client/common/redis_client_io_handler.h"
#include "trpc/transport/server/common/server_io_handler_factory.h"

namespace trpc {

bool InitIoHandler() {
  bool registry_ret = ServerIoHandlerFactory::GetInstance()->Register(
      "grpc", [](Connection* conn, const BindInfo& bind_info) {
        return std::make_unique<GrpcIoHandler>(conn); });
  TRPC_ASSERT(registry_ret && "Registry grpc server io handler failed");

  registry_ret = ClientIoHandlerFactory::GetInstance()->Register(
      "grpc", [](Connection* conn, TransInfo* trans_info) {
        return std::make_unique<GrpcIoHandler>(conn); });
  TRPC_ASSERT(registry_ret && "Registry grpc client io handler failed");

  registry_ret = ClientIoHandlerFactory::GetInstance()->Register("redis", [](Connection* conn, TransInfo* trans_info) {
    return std::make_unique<RedisClientIoHandler>(conn, trans_info);
  });
  TRPC_ASSERT(registry_ret && "Registry redis client io handler failed");

  return registry_ret;
}

void DestroyIoHandler() {
  ServerIoHandlerFactory::GetInstance()->Clear();
  ClientIoHandlerFactory::GetInstance()->Clear();
}

}  // namespace trpc
