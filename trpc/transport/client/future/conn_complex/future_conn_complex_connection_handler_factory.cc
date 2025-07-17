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

#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler_factory.h"

namespace trpc {

bool FutureConnComplexConnectionHandlerFactory::Register(const std::string& protocol,
                                                         FutureConnComplexConnectionHandlerCreator&& func) {
  auto it = creators_.find(protocol);
  if (it != creators_.end()) {
    return false;
  }

  creators_.emplace(std::make_pair(protocol, std::move(func)));
  return true;
}

void FutureConnComplexConnectionHandlerFactory::Clear() {
  creators_.clear();
}

std::unique_ptr<FutureConnectionHandler> FutureConnComplexConnectionHandlerFactory::Create(
  const std::string& protocol, const FutureConnectorOptions& options,
  FutureConnComplexMessageTimeoutHandler& handler) {

  auto it = creators_.find(protocol);
  if (it != creators_.end()) {
    return (it->second)(options, handler);
  }

  // Fallback is different between tcp and udp.
  ConnectionType conn_type = options.group_options->trans_info->conn_type;
  if (conn_type == kUdp) {
    return std::make_unique<FutureUdpIoComplexConnectionHandler>(options, handler);
  }

  return std::make_unique<FutureTcpConnComplexConnectionHandler>(options, handler);
}

}  // namespace trpc
