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

#include "trpc/transport/client/future/conn_pool/future_udp_io_pool_connector_group.h"

#include "trpc/transport/client/future/conn_pool/conn_pool.h"

namespace trpc {

FutureUdpIoPoolConnectorGroup::FutureUdpIoPoolConnectorGroup(FutureConnectorGroupOptions&& options)
    : FutureConnectorGroup(std::move(options)),
      conn_pool_(options_),
      shared_send_queue_(options_.trans_info->max_conn_num) {
  timeout_handle_function_ = [this](const internal::SharedSendQueue::DataIterator& iter) {
    uint64_t conn_id = shared_send_queue_.GetIndex(iter);
    FutureUdpIoPoolConnector* conn = conn_pool_.GetConnector(conn_id);
    CTransportReqMsg* msg = shared_send_queue_.GetAndPop(iter);
    conn->HandleSendQueueTimeout(msg);
  };
}

bool FutureUdpIoPoolConnectorGroup::Init() {
  auto timeout_check_interval = options_.trans_info->request_timeout_check_interval;
  timer_id_ = options_.reactor->AddTimerAfter(0, timeout_check_interval, [this]() { this->HandleReqTimeout(); });
  if (timer_id_ == kInvalidTimerId) {
    TRPC_FMT_ERROR("add request timeout timer failed.");
    return false;
  }

  return true;
}

void FutureUdpIoPoolConnectorGroup::Stop() {
  if (timer_id_ != kInvalidTimerId) {
    options_.reactor->CancelTimer(timer_id_);
    timer_id_ = kInvalidTimerId;
  }

  conn_pool_.Stop();
}

void FutureUdpIoPoolConnectorGroup::Destroy() {
  conn_pool_.Destroy();
}

FutureUdpIoPoolConnector* FutureUdpIoPoolConnectorGroup::GetOrCreateConnector(uint64_t conn_id) {
  FutureUdpIoPoolConnector* conn = conn_pool_.GetConnector(conn_id);
  if (conn != nullptr) {
    return conn;
  }

  FutureConnectorOptions connector_options;
  connector_options.conn_id = conn_id;
  connector_options.group_options = &options_;
  auto new_conn = std::make_unique<FutureUdpIoPoolConnector>(std::move(connector_options),
                                                             shared_send_queue_,
                                                             &conn_pool_);
  if (new_conn == nullptr) {
    return nullptr;
  }

  if (!new_conn->Init()) {
    TRPC_FMT_ERROR("init udp connector failed");
    return nullptr;
  }

  conn = new_conn.get();
  conn_pool_.AddConnector(conn_id, std::move(new_conn));
  return conn;
}

int FutureUdpIoPoolConnectorGroup::SendReqMsg(CTransportReqMsg* req_msg) {
  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  if (conn_id == UdpIoPool::kInvalidConnId) {
    return conn_pool_.PushToPendingQueue(req_msg) ? 0 : -1;
  }

  FutureUdpIoPoolConnector* connector = GetOrCreateConnector(conn_id);
  if (connector) return connector->SendReqMsg(req_msg);

  conn_pool_.RecycleConnectorId(conn_id);
  std::string error_msg = "Get or create connector failed, target:";
  error_msg += options_.peer_addr.ToString();
  TRPC_LOG_ERROR(error_msg);
  future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error_msg),
                            options_.trans_info->rsp_dispatch_function);
  return -1;
}

int FutureUdpIoPoolConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  if (conn_id == UdpIoPool::kInvalidConnId) {
    TRPC_FMT_ERROR("no valid udp connector in conn pool");
    return -1;
  }

  FutureUdpIoPoolConnector* connector = GetOrCreateConnector(conn_id);
  if (connector) return connector->SendOnly(std::move(req_msg));

  conn_pool_.RecycleConnectorId(conn_id);
  TRPC_FMT_ERROR("Get connector failed");
  return -1;
}

void FutureUdpIoPoolConnectorGroup::HandleReqTimeout() {
  shared_send_queue_.DoTimeout(trpc::time::GetMilliSeconds(), timeout_handle_function_);
  conn_pool_.HandlePendingQueueTimeout();
}

}  // namespace trpc
