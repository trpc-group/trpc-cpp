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

#include "trpc/transport/client/future/pipeline/future_tcp_pipeline_connector_group.h"

#include "trpc/transport/client/future/future_connector_group_manager.h"
#include "trpc/util/time.h"

namespace trpc {

bool FutureTcpPipelineConnectorGroup::Init() {
  uint64_t timer_id = options_.reactor->AddTimerAfter(0, 10000, [this]() { this->HandleIdleConnection(); });
  if (timer_id == kInvalidTimerId) {
    TRPC_FMT_ERROR("Add idle connection timer failed.");
    return false;
  }
  timer_ids_.push_back(timer_id);

  auto timeout_check_interval = options_.trans_info->request_timeout_check_interval;
  timer_id = options_.reactor->AddTimerAfter(0, timeout_check_interval, [this]() { this->HandleReqTimeout(); });
  if (timer_id == kInvalidTimerId) {
    TRPC_FMT_ERROR("Add request timeout timer failed.");
    return false;
  }
  timer_ids_.push_back(timer_id);

  return true;
}

void FutureTcpPipelineConnectorGroup::Stop() {
  for (auto& timer_id : timer_ids_) {
    options_.reactor->CancelTimer(timer_id);
  }

  timer_ids_.clear();
  conn_pool_.Stop();
}

void FutureTcpPipelineConnectorGroup::Destroy() { conn_pool_.Destroy(); }

FutureTcpPipelineConnector* FutureTcpPipelineConnectorGroup::GetOrCreateConnector(uint64_t conn_id) {
  FutureTcpPipelineConnector* conn = conn_pool_.GetConnector(conn_id);
  if (conn != nullptr) return conn;

  FutureConnectorOptions connector_options;
  connector_options.conn_id = conn_id;
  connector_options.group_options = &options_;
  auto new_conn = std::make_unique<FutureTcpPipelineConnector>(std::move(connector_options), &conn_pool_);
  if (new_conn == nullptr) {
    return nullptr;
  }

  if (!new_conn->Init()) {
    TRPC_FMT_ERROR("init connector failed");
    return nullptr;
  }

  if (new_conn->GetConnectionState() != ConnectionState::kUnconnected) {
    conn = new_conn.get();
    conn_pool_.AddConnector(conn_id, std::move(new_conn));
    return conn;
  }

  new_conn->Stop();
  new_conn->Destroy();
  return nullptr;
}

bool FutureTcpPipelineConnectorGroup::GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) {
  TRPC_LOG_ERROR("pipeline cannot support fixed connector, peer addr: " << options_.peer_addr.ToString());
  return false;
}

int FutureTcpPipelineConnectorGroup::SendReqMsg(CTransportReqMsg* req_msg) {
  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  FutureTcpPipelineConnector* connector = GetOrCreateConnector(conn_id);
  if (connector) return connector->SendReqMsg(req_msg);

  std::string error_msg = "Get connector failed, target:";
  error_msg += options_.peer_addr.ToString();
  TRPC_FMT_ERROR(error_msg);
  future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error_msg),
                            options_.trans_info->rsp_dispatch_function);
  return -1;
}

int FutureTcpPipelineConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  FutureTcpPipelineConnector* connector = GetOrCreateConnector(conn_id);
  if (connector != nullptr) {
    return connector->SendOnly(std::move(req_msg));
  }

  TRPC_FMT_ERROR("get connector failed");
  return -1;
}

void FutureTcpPipelineConnectorGroup::HandleReqTimeout() {
  const auto& connectors = conn_pool_.GetConnectors();
  for (auto& conn : connectors) {
    if (conn) conn->HandleRequestTimeout();
  }
}

void FutureTcpPipelineConnectorGroup::HandleIdleConnection() {
  if (options_.trans_info->connection_idle_timeout == 0)
    return;

  uint32_t alive_connector_num = 0;
  const auto& connectors = conn_pool_.GetConnectors();
  uint64_t now_ms = trpc::time::GetMilliSeconds();
  for (auto& conn : connectors) {
    if (conn) {
      alive_connector_num++;
      conn->HandleIdleConnection(now_ms);
    }
  }

  if (alive_connector_num == 0) {
    NodeAddr addr;
    addr.ip = options_.peer_addr.Ip();
    addr.port = options_.peer_addr.Port();
    options_.conn_group_manager->DelConnectorGroup(addr);
  }
}

}  // namespace trpc
