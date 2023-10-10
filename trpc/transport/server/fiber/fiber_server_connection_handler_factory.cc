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

#include "trpc/transport/server/fiber/fiber_server_connection_handler_factory.h"

namespace trpc {

bool FiberServerConnectionHandlerFactory::Register(const std::string& protocol,
                                                   FiberServerConnectionHandlerCreator&& func) {
  auto it = connection_handlers_.find(protocol);
  if (it != connection_handlers_.end()) {
    return false;
  }

  connection_handlers_.emplace(std::make_pair(protocol, std::move(func)));
  return true;
}

std::unique_ptr<FiberServerConnectionHandler> FiberServerConnectionHandlerFactory::Create(
    Connection* conn, FiberBindAdapter* bind_adapter, BindInfo* bind_info) {
  auto it = connection_handlers_.find(bind_info->protocol);
  if (it != connection_handlers_.end()) {
    return (it->second)(conn, bind_adapter, bind_info);
  }

  return std::make_unique<FiberServerConnectionHandler>(conn, bind_adapter, bind_info);
}

void FiberServerConnectionHandlerFactory::Clear() { connection_handlers_.clear(); }

}  // namespace trpc
