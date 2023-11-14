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

#include "trpc/transport/client/future/conn_complex/future_tcp_conn_complex_connector_group.h"

#include <utility>

#include "trpc/transport/client/fixed_connector_id.h"
#include "trpc/transport/client/future/future_connector_group_manager.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/time.h"

namespace trpc {

FutureTcpConnComplexConnectorGroup::FutureTcpConnComplexConnectorGroup(FutureConnectorGroupOptions&& options,
                                                                       FutureConnComplexMessageTimeoutHandler& handler)
    : FutureConnectorGroup(std::move(options)) {
  FutureConnectorOptions connector_options;
  connector_options.group_options = &options_;
  connector_ = std::make_unique<FutureTcpConnComplexConnector>(std::move(connector_options), handler);
  TRPC_ASSERT(connector_ != nullptr);
}

bool FutureTcpConnComplexConnectorGroup::Init() {
  if (!connector_->Init()) {
    return false;
  }

  idle_connection_timer_id_ = options_.reactor->AddTimerAfter(0, 10000, [this]() { this->HandleIdleConnection(); });
  if (idle_connection_timer_id_ == kInvalidTimerId) {
    return false;
  }

  return true;
}

void FutureTcpConnComplexConnectorGroup::Stop() {
  if (idle_connection_timer_id_ != kInvalidTimerId) {
    options_.reactor->CancelTimer(idle_connection_timer_id_);
    idle_connection_timer_id_ = kInvalidTimerId;
  }

  connector_->Stop();
}

void FutureTcpConnComplexConnectorGroup::Destroy() { connector_->Destroy(); }

stream::StreamReaderWriterProviderPtr FutureTcpConnComplexConnectorGroup::CreateStream(
    stream::StreamOptions&& stream_options) {
  return connector_->CreateStream(std::move(stream_options));
}

bool FutureTcpConnComplexConnectorGroup::ReleaseConnector(uint64_t fixed_connector_id) {
  FixedConnectorId* ptr = reinterpret_cast<FixedConnectorId*>(fixed_connector_id);
  fixed_connection_count_--;
  object_pool::Delete<FixedConnectorId>(ptr);
  return true;
}

bool FutureTcpConnComplexConnectorGroup::GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) {
  if (connector_->GetConnectionState() == ConnectionState::kUnconnected) {
    connector_->Stop();
    connector_->Destroy();
    if (!connector_->Init()) {
      return false;
    }
  }

  FixedConnectorId* connector_id_ptr = trpc::object_pool::New<FixedConnectorId>();
  // No need to set conn_id as one connector group has only one connector.
  connector_id_ptr->connector_group = this;
  uint64_t connector_id = reinterpret_cast<std::uint64_t>(connector_id_ptr);
  fixed_connection_count_++;
  promise.SetValue(connector_id);
  return true;
}

int FutureTcpConnComplexConnectorGroup::SendReqMsg(CTransportReqMsg* req_msg) {
  if (req_msg->context->GetFixedConnectorId() == 0) {
    return connector_->SendReqMsg(req_msg);
  }

  // Fixed connection.
  if (connector_->GetConnectionState() == ConnectionState::kUnconnected && !options_.trans_info->allow_reconnect) {
    std::string error_msg = "connection is unavailable, target:";
    error_msg += options_.peer_addr.ToString();
    future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error_msg),
                              options_.trans_info->rsp_dispatch_function);
    return -1;
  }

  return connector_->SendReqMsg(req_msg);
}

int FutureTcpConnComplexConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  if (req_msg->context->GetFixedConnectorId() == 0) {
    return connector_->SendOnly(req_msg);
  }

  // Fixed connection.
  if (connector_->GetConnectionState() == ConnectionState::kUnconnected && !options_.trans_info->allow_reconnect) {
    std::string error_msg = "connection is unavailable, target:";
    error_msg += options_.peer_addr.ToString();
    TRPC_LOG_ERROR(error_msg);
    return -1;
  }

  return connector_->SendOnly(req_msg);
}

void FutureTcpConnComplexConnectorGroup::HandleIdleConnection() {
  connector_->HandleIdleConnection(trpc::time::GetMilliSeconds());

  if (connector_->GetConnectionState() == ConnectionState::kUnconnected && fixed_connection_count_ == 0) {
    NodeAddr addr;
    addr.ip = options_.peer_addr.Ip();
    addr.port = options_.peer_addr.Port();
    options_.conn_group_manager->DelConnectorGroup(addr);
  }
}

}  // namespace trpc
