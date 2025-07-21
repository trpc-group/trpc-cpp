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

#include "trpc/transport/client/future/conn_pool/future_tcp_conn_pool_connector.h"

#include <string>
#include <utility>

#include "trpc/filter/filter.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"
#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler_factory.h"
#include "trpc/util/time.h"

namespace trpc {

bool FutureTcpConnPoolConnector::Init() {
  auto conn_event_handler = FutureConnPoolConnectionHandlerFactory::GetIntance()->Create(
      options_.group_options->trans_info->protocol, options_, msg_timeout_handler_);

  FutureTcpConnPoolConnectionHandler* handler =
      dynamic_cast<FutureTcpConnPoolConnectionHandler*>(conn_event_handler.get());
  TRPC_ASSERT(handler && "dynamic_cast to FutureTcpConnPoolConnectionHandler error");
  handler->SetConnPool(conn_pool_);

  connection_ = future::CreateTcpConnection(options_, std::move(conn_event_handler));
  if (connection_ == nullptr) {
    return false;
  }

  return true;
}

void FutureTcpConnPoolConnector::Stop() {
  if (!connection_) return;

  connection_->DoClose(false);
}

void FutureTcpConnPoolConnector::Destroy() {
  if (!connection_) return;

  connection_.Reset();
}

int FutureTcpConnPoolConnector::SendReqMsg(CTransportReqMsg* req_msg) {
  if (msg_timeout_handler_.IsSendQueueEmpty()) {
    return SendReqMsgImpl(req_msg);
  }

  bool ret = msg_timeout_handler_.PushToPendingQueue(req_msg);
  if (ret) return 0;

  return -1;
}

int FutureTcpConnPoolConnector::SendOnly(CTransportReqMsg* req_msg) {
  if (!connection_) return -1;

  int ret = future::SendTcpMsg(req_msg, connection_.Get());
  if (!IsFixedConnection()) {
    conn_pool_->RecycleConnectorId(GetConnectorId());
  }

  return ret;
}

void FutureTcpConnPoolConnector::HandleSendQueueTimeout(CTransportReqMsg* req_msg) {
  // If request timeout, must close connection to avoid ambiguity.
  if (msg_timeout_handler_.HandleSendQueueTimeout(req_msg)) {
    connection_->DoClose(true);
  }
}

void FutureTcpConnPoolConnector::HandlePendingQueueTimeout() { msg_timeout_handler_.HandlePendingQueueTimeout(); }

void FutureTcpConnPoolConnector::HandleIdleConnection(uint64_t now_ms) {
  if (connection_ == nullptr || options_.group_options->trans_info->connection_idle_timeout == 0) {
    return;
  }

  if (now_ms > connection_->GetConnActiveTime() &&
      now_ms - connection_->GetConnActiveTime() >= options_.group_options->trans_info->connection_idle_timeout) {
    if (msg_timeout_handler_.IsSendQueueEmpty()) {
      connection_->DoClose(true);
    }
  }
}

int FutureTcpConnPoolConnector::SendReqMsgImpl(CTransportReqMsg* req_msg) {
  if (GetConnectionState() == ConnectionState::kConnecting) {
    if (kConnectInterval + connection_->GetDoConnectTimestamp() < trpc::time::GetMilliSeconds()) {
      if (!IsFixedConnection()) {
        conn_pool_->PushToPendingQueue(req_msg);
      } else {
        msg_timeout_handler_.PushToPendingQueue(req_msg);
      }
      connection_->DoClose(true);
      return 0;
    }
  }

  msg_timeout_handler_.PushToSendQueue(req_msg);
  int ret = future::SendTcpMsg(req_msg, connection_.Get());
  if (!ret) return 0;

  std::string error = "network send error, peer addr:";
  error += options_.group_options->peer_addr.ToString();
  CTransportReqMsg* msg = msg_timeout_handler_.PopFromSendQueue();
  if (msg) {
    DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error));
  }

  return -1;
}

}  // namespace trpc
