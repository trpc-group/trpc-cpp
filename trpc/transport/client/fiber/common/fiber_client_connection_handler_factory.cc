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

#include "trpc/transport/client/fiber/common/fiber_client_connection_handler_factory.h"

namespace trpc {

bool FiberClientConnectionHandlerFactory::Register(const std::string& protocol,
                                                   FiberClientConnectionHandlerCreator&& func) {
  auto it = connection_handlers_.find(protocol);
  if (it != connection_handlers_.end()) {
    return false;
  }

  connection_handlers_.emplace(std::make_pair(protocol, std::move(func)));
  return true;
}

std::unique_ptr<FiberClientConnectionHandler> FiberClientConnectionHandlerFactory::Create(const std::string& protocol,
                                                                                          Connection* conn,
                                                                                          TransInfo* trans_info) {
  auto it = connection_handlers_.find(protocol);
  if (it != connection_handlers_.end()) {
    return (it->second)(conn, trans_info);
  }

  return std::make_unique<FiberClientConnectionHandler>(conn, trans_info);
}

void FiberClientConnectionHandlerFactory::Clear() { connection_handlers_.clear(); }

}  // namespace trpc
