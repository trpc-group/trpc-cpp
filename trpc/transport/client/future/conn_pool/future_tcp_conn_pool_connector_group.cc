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

#include "trpc/transport/client/future/conn_pool/future_tcp_conn_pool_connector_group.h"

#include "trpc/transport/client/fixed_connector_id.h"
#include "trpc/transport/client/future/future_connector_group_manager.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/time.h"

namespace trpc {

FutureTcpConnPoolConnectorGroup::FutureTcpConnPoolConnectorGroup(FutureConnectorGroupOptions&& options)
    : FutureConnectorGroup(std::move(options)),
      conn_pool_(options_),
      shared_send_queue_(options_.trans_info->max_conn_num) {
  timeout_handle_function_ = [this](const internal::SharedSendQueue::DataIterator& iter) {
    uint64_t conn_id = shared_send_queue_.GetIndex(iter);
    FutureTcpConnPoolConnector* conn = conn_pool_.GetConnector(conn_id);
    TRPC_ASSERT(conn != nullptr);
    CTransportReqMsg* msg = shared_send_queue_.GetAndPop(iter);
    conn->HandleSendQueueTimeout(msg);
  };
}

bool FutureTcpConnPoolConnectorGroup::Init() {
  uint64_t timer_id = options_.reactor->AddTimerAfter(0, 10000, [this]() { this->HandleIdleConnection(); });
  if (timer_id == kInvalidTimerId) {
    TRPC_FMT_ERROR("add idle connection timer failed.");
    return false;
  }
  timer_ids_.push_back(timer_id);

  auto timeout_check_interval = options_.trans_info->request_timeout_check_interval;
  TRPC_ASSERT(timeout_check_interval > 0);
  timer_id = options_.reactor->AddTimerAfter(0, timeout_check_interval, [this]() { this->HandleReqTimeout(); });
  if (timer_id == kInvalidTimerId) {
    TRPC_FMT_ERROR("add request timeout timer failed.");
    return false;
  }
  timer_ids_.push_back(timer_id);

  return true;
}

void FutureTcpConnPoolConnectorGroup::Stop() {
  for (auto& timer_id : timer_ids_) {
    options_.reactor->CancelTimer(timer_id);
  }

  timer_ids_.clear();
  conn_pool_.Stop();
}

void FutureTcpConnPoolConnectorGroup::Destroy() { conn_pool_.Destroy(); }

FutureTcpConnPoolConnector* FutureTcpConnPoolConnectorGroup::GetOrCreateConnector(uint64_t conn_id) {
  FutureTcpConnPoolConnector* conn = conn_pool_.GetConnector(conn_id);
  if (conn != nullptr) {
    return conn;
  }

  FutureConnectorOptions connector_options;
  connector_options.conn_id = conn_id;
  connector_options.group_options = &options_;
  auto new_conn =
      std::make_unique<FutureTcpConnPoolConnector>(std::move(connector_options), shared_send_queue_, &conn_pool_);
  if (new_conn == nullptr) {
    return nullptr;
  }

  if (new_conn->Init() == false) {
    TRPC_FMT_ERROR("Init connector failed");
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

bool FutureTcpConnPoolConnectorGroup::ReleaseConnector(uint64_t fixed_connector_id) {
  FixedConnectorId* ptr = reinterpret_cast<FixedConnectorId*>(fixed_connector_id);
  uint64_t conn_id = ptr->conn_id;
  auto* connector = conn_pool_.GetConnector(conn_id);
  if (!connector) {
    fixed_connection_count_--;
    object_pool::Delete<FixedConnectorId>(ptr);
    return true;
  }

  if (connector->IsConnectionIdle()) {
    connector->SetIsFixedConnection(false);
    conn_pool_.RecycleConnectorId(conn_id);
    fixed_connection_count_--;
    object_pool::Delete<FixedConnectorId>(ptr);
    return true;
  }

  TRPC_FMT_ERROR("Release connector failed");
  return false;
}

bool FutureTcpConnPoolConnectorGroup::GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) {
  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  if (conn_id == TcpConnPool::kInvalidConnId) {
    return false;
  }

  auto* connector = GetOrCreateConnector(conn_id);
  if (connector == nullptr) {
    conn_pool_.RecycleConnectorId(conn_id);
    return false;
  }

  connector->SetIsFixedConnection(true);
  fixed_connection_count_++;
  FixedConnectorId* connector_id_ptr = trpc::object_pool::New<FixedConnectorId>();
  connector_id_ptr->connector_group = this;
  connector_id_ptr->conn_id = conn_id;
  uint64_t connector_id = reinterpret_cast<std::uint64_t>(connector_id_ptr);
  promise.SetValue(connector_id);
  return true;
}

int FutureTcpConnPoolConnectorGroup::SendReqMsg(CTransportReqMsg* req_msg) {
  uint64_t fixed_connector_id = req_msg->context->GetFixedConnectorId();
  if (TRPC_UNLIKELY(fixed_connector_id != 0)) {
    FutureTcpConnPoolConnector* connector = GetConnectorByFixedId(fixed_connector_id);
    if (connector) return connector->SendReqMsg(req_msg);

    std::string error_msg = "connection is unavailable, target:";
    error_msg += options_.peer_addr.ToString();
    future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error_msg),
                              options_.trans_info->rsp_dispatch_function);
    return -1;
  }

  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  if (conn_id == TcpConnPool::kInvalidConnId) {
    return conn_pool_.PushToPendingQueue(req_msg) ? 0 : -1;
  }

  FutureTcpConnPoolConnector* connector = GetOrCreateConnector(conn_id);
  if (connector) return connector->SendReqMsg(req_msg);

  conn_pool_.RecycleConnectorId(conn_id);
  std::string error_msg = "get connector failed, target:";
  error_msg += options_.peer_addr.ToString();
  TRPC_FMT_ERROR(error_msg);
  future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error_msg),
                            options_.trans_info->rsp_dispatch_function);
  return -1;
}

int FutureTcpConnPoolConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  uint64_t fixed_connector_id = req_msg->context->GetFixedConnectorId();
  if (TRPC_UNLIKELY(fixed_connector_id != 0)) {
    FutureTcpConnPoolConnector* connector = GetConnectorByFixedId(fixed_connector_id);
    if (connector) return connector->SendReqMsg(req_msg);

    std::string error_msg = "connection is unavailable, target:";
    error_msg += options_.peer_addr.ToString();
    TRPC_FMT_ERROR(error_msg);
    return -1;
  }

  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  if (conn_id == TcpConnPool::kInvalidConnId) {
    TRPC_FMT_ERROR("no valid connector in conn pool");
    return -1;
  }

  FutureTcpConnPoolConnector* connector = GetOrCreateConnector(conn_id);
  if (connector) return connector->SendOnly(std::move(req_msg));

  conn_pool_.RecycleConnectorId(conn_id);
  TRPC_FMT_ERROR("get connector failed");
  return -1;
}

FutureTcpConnPoolConnector* FutureTcpConnPoolConnectorGroup::GetConnectorByFixedId(uint64_t fixed_connector_id) {
  uint64_t conn_id = reinterpret_cast<FixedConnectorId*>(fixed_connector_id)->conn_id;
  TRPC_ASSERT(conn_id != TcpConnPool::kInvalidConnId);
  FutureTcpConnPoolConnector* connector = nullptr;
  if (options_.trans_info->allow_reconnect) {
    connector = GetOrCreateConnector(conn_id);
    if (connector) {
      connector->SetIsFixedConnection(true);
    }
  } else {
    connector = conn_pool_.GetConnector(conn_id);
    if (connector != nullptr && !connector->IsFixedConnection()) {
      return nullptr;
    }
  }

  return connector;
}

void FutureTcpConnPoolConnectorGroup::HandleReqTimeout() {
  shared_send_queue_.DoTimeout(trpc::time::GetMilliSeconds(), timeout_handle_function_);

  conn_pool_.HandlePendingQueueTimeout();

  if (fixed_connection_count_ == 0) return;

  const auto& connectors = conn_pool_.GetConnectors();
  for (auto& conn : connectors) {
    if (conn) conn->HandlePendingQueueTimeout();
  }
}

void FutureTcpConnPoolConnectorGroup::HandleIdleConnection() {
  if (options_.trans_info->connection_idle_timeout == 0) {
    return;
  }

  uint32_t alive_connector_num = 0;
  const auto& connectors = conn_pool_.GetConnectors();
  uint64_t now_ms = trpc::time::GetMilliSeconds();
  for (auto& conn : connectors) {
    if (conn) {
      alive_connector_num++;
      conn->HandleIdleConnection(now_ms);
    }
  }

  if (alive_connector_num == 0 && fixed_connection_count_ == 0) {
    NodeAddr addr;
    addr.ip = options_.peer_addr.Ip();
    addr.port = options_.peer_addr.Port();
    options_.conn_group_manager->DelConnectorGroup(addr);
  }
}

stream::StreamReaderWriterProviderPtr FutureTcpConnPoolConnectorGroup::CreateStream(
    stream::StreamOptions&& stream_options) {
  uint64_t conn_id = conn_pool_.GenAvailConnectorId();
  if (conn_id == TcpConnPool::kInvalidConnId) {
    return nullptr;
  }

  FutureTcpConnPoolConnector* connector = GetOrCreateConnector(conn_id);
  if (connector == nullptr) {
    conn_pool_.RecycleConnectorId(conn_id);
    TRPC_FMT_ERROR("Get connector failed, target: {}", options_.peer_addr.ToString());
    return nullptr;
  }

  auto conn_handler = static_cast<FutureConnectionHandler*>(connector->GetConnection()->GetConnectionHandler());
  auto stream_handler = conn_handler->GetOrCreateStreamHandler();
  TRPC_CHECK_NE(stream_handler.Get(), nullptr);

  stream_options.stream_handler = stream_handler;
  stream_options.connection_id = conn_id;
  stream_options.fiber_mode = false;
  stream_options.server_mode = false;
  stream_options.callbacks.on_close_cb = [this, conn_id](int reason) {
    FutureTcpConnPoolConnector* connector = conn_pool_.GetConnector(conn_id);
    if (connector == nullptr) {
      TRPC_FMT_ERROR("get connector failed, target: {}", options_.peer_addr.ToString());
      return;
    }

    if (reason == 0) {
      TRPC_FMT_DEBUG("stream close, recycle connection");
      connector->SetIsFixedConnection(false);
      conn_pool_.RecycleConnectorId(conn_id);
      fixed_connection_count_--;
    } else {
      options_.reactor->SubmitTask([this, conn_id]() {
        TRPC_FMT_ERROR("stream abnormal, close connection");
        this->conn_pool_.DelConnector(conn_id);
        fixed_connection_count_--;
      });
    }
  };
  return stream_handler->CreateStream(std::move(stream_options));
}

}  // namespace trpc
