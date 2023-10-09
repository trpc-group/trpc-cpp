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

#include "trpc/transport/client/future/conn_complex/future_tcp_conn_complex_connector.h"

#include <atomic>
#include <utility>

#include "trpc/filter/filter.h"
#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler_factory.h"
#include "trpc/util/check.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace {

// Generate connector id (thread safe).
uint32_t GenerateConnectorId() {
  static std::atomic_uint32_t connector_id{0};
  return connector_id.fetch_add(1);
}

}  // namespace

namespace trpc {

FutureTcpConnComplexConnector::FutureTcpConnComplexConnector(FutureConnectorOptions&& options,
                                                             FutureConnComplexMessageTimeoutHandler& handler)
    : FutureConnector(std::move(options)),
      msg_timeout_handler_(handler),
      connect_interval_(FutureConnector::kConnectInterval) {
  options_.conn_id = GenerateConnectorId();
}

bool FutureTcpConnComplexConnector::Init() {
  // Connection maybe create failed.
  connection_ = CreateConnection();
  if (TRPC_UNLIKELY(connection_ == nullptr)) {
    return false;
  }

  SetReconnectionHandle([this] {
    connection_->DoClose(true);
    TRPC_ASSERT(connection_ == nullptr && "connection need to be nullptr after close");
    connection_ = CreateConnection();
    if (TRPC_UNLIKELY(connection_ == nullptr)) {
      TRPC_FMT_ERROR("create connection failed when reconnect to : {}", options_.group_options->peer_addr.ToString());
    }
  });

  connection_state_.store(ClientTransportState::kInitialized, std::memory_order_release);
  return true;
}

void FutureTcpConnComplexConnector::Stop() {
  // Only ClientTransportState::kInitialized can go further.
  if (connection_state_.load(std::memory_order_acquire) != ClientTransportState::kInitialized) {
    TRPC_FMT_ERROR("stop when invalid connection_state:{}", (int)connection_state_.load(std::memory_order_acquire));
    return;
  }

  if (connection_ != nullptr) {
    TRPC_FMT_TRACE("stop conn_id:{}", connection_->GetConnId());
    connection_->DoClose(false);
  }

  connection_state_.store(ClientTransportState::kStopped, std::memory_order_release);
}

void FutureTcpConnComplexConnector::Destroy() {
  // Only ClientTransportState::kStopped can go further.
  if (connection_state_.load(std::memory_order_acquire) != ClientTransportState::kStopped) {
    TRPC_FMT_ERROR("destroy when invalid connection_state:{}", (int)connection_state_.load(std::memory_order_acquire));
    return;
  }

  if (connection_ != nullptr) {
    TRPC_FMT_TRACE("destroy conn_id:{}", connection_->GetConnId());
    connection_.Reset();
  }

  connection_state_.store(ClientTransportState::kDestroyed, std::memory_order_release);
}

int FutureTcpConnComplexConnector::SendReqMsg(CTransportReqMsg* req_msg) {
  // Only ClientTransportState::kInitialized can go further.
  if (TRPC_UNLIKELY(connection_state_.load(std::memory_order_acquire) != ClientTransportState::kInitialized)) {
    TRPC_FMT_ERROR("SendReqMsg failed,invalid ConnectionState ,ConnectionState:{}",
                    (int)connection_state_.load(std::memory_order_acquire));
    DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_CONNECT_ERR, "invalid ConnectionState");
    return -1;
  }

  // If connection_ is empty, it means that the connection has been cleaned up or has not been established successfully,
  // and it needs to be re-established.
  if (TRPC_UNLIKELY(connection_ == nullptr)) {
    connection_ = CreateConnection();
    // If creating a connection fails, return an error.
    if (TRPC_UNLIKELY(connection_ == nullptr)) {
      std::string err = "Create connection to " + options_.group_options->peer_addr.ToString() + " failed";
      DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_CONNECT_ERR, std::move(err));
      return -1;
    }
  }

  req_msg->extend_info->connection_id = connection_->GetConnId();

  if (GetConnectionState() == ConnectionState::kConnected) {
    return SendReqMsgWhenIsConnected(req_msg);
  } else {
    Connect();
    return SendReqMsgInternal(req_msg);
  }
}

int FutureTcpConnComplexConnector::SendOnly(CTransportReqMsg* req_msg) {
  // Only ClientTransportState::kInitialized can go further.
  if (TRPC_UNLIKELY(connection_state_.load(std::memory_order_acquire) != ClientTransportState::kInitialized)) {
    TRPC_FMT_ERROR("SendOnly failed,invalid ConnectionState ,ConnectionState:{}",
                   (int)connection_state_.load(std::memory_order_acquire));
    return -1;
  }

  if (TRPC_UNLIKELY(connection_ == nullptr)) {
    connection_ = CreateConnection();
    // If creating a connection fails, return an error.
    if (TRPC_UNLIKELY(connection_ == nullptr)) {
      return -1;
    }
  }

  // When the connection status is disconnected, it needs to be reconnected once.
  if (GetConnectionState() == ConnectionState::kUnconnected) {
    TRPC_LOG_ERROR("connection closed, try connect, peer addr:" << options_.group_options->peer_addr.ToString());
    Connect();
    // If unable to reconnect or if the connection is empty after reconnecting, return failure directly.
    if (TRPC_UNLIKELY(GetConnectionState() == ConnectionState::kUnconnected)) {
      return -1;
    }
  }

  IoMessage message;
  message.buffer = std::move(req_msg->send_data);
  message.seq_id = req_msg->context->GetRequestId();

  return connection_->Send(std::move(message));
}

int FutureTcpConnComplexConnector::SendReqMsgWhenIsConnected(CTransportReqMsg* req_msg) {
  FutureConnComplexMessageTimeoutHandler::PushResult push_result = msg_timeout_handler_.Push(req_msg);
  if (push_result == FutureConnComplexMessageTimeoutHandler::PushResult::kOk) {
    uint32_t seq_id = req_msg->context->GetRequestId();
    int send_ok = future::SendTcpMsg(req_msg, connection_.Get());
    if (send_ok == 0) return send_ok;

    std::string error = "network send error, peer addr:";
    error += options_.group_options->peer_addr.ToString();
    // Message maybe freed in HandleClose when error happened.
    // Need to pop from timeout queue to make sure message is still effective.
    CTransportReqMsg* msg = msg_timeout_handler_.Pop(seq_id);
    if (!msg) return send_ok;

    // Message still effective.
    DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error));
  } else if (push_result == FutureConnComplexMessageTimeoutHandler::PushResult::kExisted) {
    int send_ok = future::SendTcpMsg(req_msg, connection_.Get());
    DispatchException(req_msg, TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, "");
    return send_ok;
  }

  return -1;
}

int FutureTcpConnComplexConnector::SendReqMsgInternal(CTransportReqMsg* req_msg) {
  if (GetConnectionState() == ConnectionState::kConnected || GetConnectionState() == ConnectionState::kConnecting) {
    return SendReqMsgWhenIsConnected(req_msg);
  }

  std::string error("network unconnected error, peer addr:");
  error += options_.group_options->peer_addr.Ip();
  error += ":";
  error += std::to_string(options_.group_options->peer_addr.Port());
  DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_CONNECT_ERR, std::move(error));

  return -1;
}

RefPtr<TcpConnection> FutureTcpConnComplexConnector::CreateConnection() {
  auto conn_event_handler = FutureConnComplexConnectionHandlerFactory::GetIntance()->Create(
      options_.group_options->trans_info->protocol, options_, msg_timeout_handler_);
  FutureTcpConnComplexConnectionHandler* handler =
      dynamic_cast<FutureTcpConnComplexConnectionHandler*>(conn_event_handler.get());
  TRPC_ASSERT(handler && "dynamic_cast to FutureTcpConnComplexConnectionHandler error");
  handler->SetDelConnectionFunc([this]() {
    // Note: must submit reactor task to delay connection deleting.
    options_.group_options->reactor->SubmitInnerTask([conn = std::move(connection_)]() mutable {});
  });

  auto conn = future::CreateTcpConnection(options_, std::move(conn_event_handler));
  if (conn == nullptr) {
    TRPC_FMT_ERROR("Create connection to {} failed", options_.group_options->peer_addr.ToString());
    return nullptr;
  }

  return conn;
}

void FutureTcpConnComplexConnector::HandleIdleConnection(uint64_t now_ms) {
  // 1. Connection idle disable.
  // 2. Connection not created.
  if (options_.group_options->trans_info->connection_idle_timeout == 0 || connection_ == nullptr) {
    return;
  }

  uint64_t conn_id = connection_->GetConnId();
  TRPC_FMT_TRACE("HandleIdleConnection conn_id: {}", conn_id);

  uint64_t now = trpc::time::GetMilliSeconds();
  uint64_t conn_active_time = connection_->GetConnActiveTime();
  TRPC_ASSERT(now >= conn_active_time);
  if (now - conn_active_time < options_.group_options->trans_info->connection_idle_timeout) return;

  // Close connection and reconnect.
  TRPC_FMT_TRACE(
      "HandleIdleConnection conn_id: {}, now: {}, conn_active_time: {},"
      "idle_timeout: {}, , connection_: {}, is_reconnection: {}",
      conn_id, now, conn_active_time, options_.group_options->trans_info->connection_idle_timeout,
      reinterpret_cast<void*>(connection_.Get()), options_.group_options->trans_info->is_reconnection);

  // Just close without reconnect.
  if (!options_.group_options->trans_info->is_reconnection) {
    connection_->DoClose(true);
    return;
  }

  reconn_handle_();
  TRPC_FMT_TRACE("HandleIdleConnection CreateConnection connection: {}, conn_id: {}",
                 reinterpret_cast<void*>(connection_.Get()), (connection_ != nullptr ? connection_->GetConnId() : -1));
}

bool FutureTcpConnComplexConnector::Connect() {
  uint64_t now = trpc::time::GetMilliSeconds();

  TRPC_FMT_TRACE("Connect conn_id: {}, now: {}, connection time: {}, interval: {}", connection_->GetConnId(), now,
                 connection_->GetDoConnectTimestamp(), connect_interval_);

  // To limit reconnect frequency.
  if (now - connection_->GetDoConnectTimestamp() < connect_interval_) {
    return false;
  }

  TRPC_FMT_TRACE("Connect need to reconnect conn_id: {}, now: {}, connection time: {}, interval: {}",
                 connection_->GetConnId(), now, connection_->GetDoConnectTimestamp(), connect_interval_);

  reconn_handle_();
  return true;
}

stream::StreamReaderWriterProviderPtr FutureTcpConnComplexConnector::CreateStream(
    stream::StreamOptions&& stream_options) {
  if (TRPC_UNLIKELY(connection_ == nullptr)) {
    connection_ = CreateConnection();
    if (TRPC_UNLIKELY(connection_ == nullptr)) return nullptr;
  }

  auto stream_handler =
      static_cast<FutureConnectionHandler*>(connection_->GetConnectionHandler())->GetOrCreateStreamHandler();
  TRPC_CHECK_NE(stream_handler.Get(), nullptr);
  stream_options.stream_handler = stream_handler;
  return stream_handler->CreateStream(std::move(stream_options));
}

}  // namespace trpc
