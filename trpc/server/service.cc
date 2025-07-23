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

#include "trpc/server/service.h"

#include "trpc/server/service_adapter.h"
#include "trpc/transport/server/server_transport.h"

namespace trpc {

Service::~Service() {
  auto it_rpc = rpc_service_methods_.begin();
  while (it_rpc != rpc_service_methods_.end()) {
    delete it_rpc->second;
    ++it_rpc;
  }

  auto it_non_rpc = non_rpc_service_methods_.begin();
  while (it_non_rpc != non_rpc_service_methods_.end()) {
    delete it_non_rpc->second;
    ++it_non_rpc;
  }
}

const ServiceAdapterOption& Service::GetServiceAdapterOption() const {
  return adapter_->GetServiceAdapterOption();
}

int Service::SendMsg(STransportRspMsg* msg) {
  if (transport_) {
    return transport_->SendMsg(msg);
  }

  return -1;
}

bool Service::IsConnected(uint64_t conn_id) {
  if (transport_) {
    return transport_->IsConnected(conn_id);
  }
  return false;
}

void Service::CloseConnection(const CloseConnectionInfo& conn) {
  if (transport_) {
    transport_->DoClose(conn);
  }
}

void Service::ThrottleConnection(uint64_t conn_id, bool set) {
  if (transport_) {
    transport_->ThrottleConnection(conn_id, set);
  }
}

}  // namespace trpc
