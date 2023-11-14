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

#include "trpc/transport/client/future/common/utils.h"

#include <cstdint>
#include <utility>

#include "trpc/future/future.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/transport/client/common/client_io_handler_factory.h"
#include "trpc/transport/client/future/common/future_connector_options.h"
#include "trpc/transport/client/future/common/future_connection_handler.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc::future {

namespace {

void SetConnectionOption(Connection* conn, const FutureConnectorOptions& conn_options,
                         std::unique_ptr<FutureConnectionHandler>&& conn_handler,
                         std::unique_ptr<IoHandler>&& io_handler) {
  conn->SetMaxPacketSize(conn_options.group_options->trans_info->max_packet_size);
  conn->SetRecvBufferSize(conn_options.group_options->trans_info->recv_buffer_size);
  conn->SetSendQueueCapacity(conn_options.group_options->trans_info->send_queue_capacity);
  conn->SetSendQueueTimeout(conn_options.group_options->trans_info->send_queue_timeout);
  conn->SetCheckConnectTimeout(conn_options.group_options->trans_info->connect_timeout);
  conn->SetConnType(conn_options.group_options->trans_info->conn_type);
  conn->SetConnId(conn_options.conn_id);
  conn->SetClient();

  if (conn_options.group_options->trans_info->support_pipeline) {
    conn->SetSupportPipeline();
  }

  conn->SetPeerIp(conn_options.group_options->peer_addr.Ip());
  conn->SetPeerPort(conn_options.group_options->peer_addr.Port());
  conn->SetPeerIpType(conn_options.group_options->peer_addr.Type());
  conn->SetIoHandler(std::move(io_handler));

  conn_handler->SetConnection(conn);
  conn_handler->Init();
  conn->SetConnectionHandler(std::move(conn_handler));
}

}  // namespace

void DispatchResponse(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg,
                      const TransInfo::RspDispatchFunction& rsp_dispatch_function,
                      const TransInfo::RunClientFiltersFunction& run_client_filters_function) {
  object_pool::LwUniquePtr<MsgTask> task = object_pool::MakeLwUnique<MsgTask>();
  task->task_type = runtime::kResponseMsg;
  task->param = req_msg;
  task->handler = [&run_client_filters_function, req_msg, rsp_msg]() mutable {
    // For rpcz.
    if (run_client_filters_function) {
      run_client_filters_function(FilterPoint::CLIENT_POST_SCHED_RECV_MSG, req_msg);
    }

    auto* backup_promise = req_msg->extend_info->backup_promise;
    // Must set promise first before trigger backup request callback.
    req_msg->extend_info->promise.SetValue(std::move(*rsp_msg));

    if (backup_promise != nullptr) {
      future::NotifyBackupRequestOver(backup_promise);
    }

    trpc::object_pool::Delete(rsp_msg);
  };

  if (!rsp_dispatch_function(std::move(task))) {
    req_msg->extend_info->promise.SetException(CommonException("dispatch response failed"));
    // Release message if dispatch failed.
    trpc::object_pool::Delete(rsp_msg);
  }
}

void DispatchException(CTransportReqMsg* req_msg, int ret, std::string&& err_msg,
                       const TransInfo::RspDispatchFunction& rsp_dispatch_function) {
  object_pool::LwUniquePtr<MsgTask> task = object_pool::MakeLwUnique<MsgTask>();
  task->task_type = runtime::kResponseMsg;
  task->param = req_msg;
  task->handler = [req_msg, ret, msg = std::move(err_msg)]() mutable {
    auto* backup_promise = req_msg->extend_info->backup_promise;
    Exception ex(CommonException(msg.c_str(), ret));
    req_msg->extend_info->promise.SetException(ex);
    // Resend timeout or error, notify resend.
    if (backup_promise != nullptr) {
      Exception e(CommonException("Invoke failed", TRPC_INVOKE_UNKNOWN_ERR));
      future::NotifyBackupRequestResend(std::move(e), backup_promise);
    }
  };

  if (!rsp_dispatch_function(std::move(task))) {
    req_msg->extend_info->promise.SetException(CommonException("dispatch exception failed"));
  }
}

void NotifyBackupRequestOver(void*& promise) {
  auto* backup_promise = static_cast<Promise<bool>*>(promise);
  backup_promise->SetValue(true);
  delete backup_promise;
  promise = nullptr;
}

void NotifyBackupRequestResend(Exception&& ex, void*& promise) {
  auto* backup_promise = static_cast<Promise<bool>*>(promise);
  backup_promise->SetException(std::move(ex));
  delete backup_promise;
  promise = nullptr;
}

int SendTcpMsg(CTransportReqMsg* req_msg, Connection* conn, bool is_oneway) {
  IoMessage message;
  message.buffer = std::move(req_msg->send_data);
  message.seq_id = req_msg->context->GetRequestId();
  message.is_oneway = is_oneway;
  message.context_ext = req_msg->context->GetCurrentContextExt();
  // Can not use std::move here.
  message.msg = req_msg->context;

  // Sending Non-Streaming Unary Requests.
  if (req_msg->context->GetStreamId() == 0) {
    return conn->Send(std::move(message));
  } else {
    // Sending Streaming Unary Responses, which may require additional encoding such as gRPC/HTTP2.
    if (TRPC_LIKELY(conn->GetConnectionHandler()->EncodeStreamMessage(&message))) {
      return conn->Send(std::move(message));
    }
  }
  return -1;
}

int SendUdpMsg(CTransportReqMsg* req_msg, Connection* conn) {
  IoMessage message;
  message.ip = req_msg->context->GetIp();
  message.port = req_msg->context->GetPort();
  message.buffer = std::move(req_msg->send_data);
  // Can not use std::move here.
  message.msg = req_msg->context;
  return conn->Send(std::move(message));
}

RefPtr<TcpConnection> CreateTcpConnection(const FutureConnectorOptions& conn_options,
                                          std::unique_ptr<FutureConnectionHandler>&& conn_handler) {
  auto socket = Socket::CreateTcpSocket(conn_options.group_options->peer_addr.IsIpv6());
  if (!socket.IsValid()) {
    TRPC_LOG_ERROR("create socket failed due to invalid socket"
                   << ", peer addr: " << conn_options.group_options->peer_addr.ToString());
    return nullptr;
  }

  socket.SetTcpNoDelay();
  socket.SetCloseWaitDefault();
  socket.SetBlock(false);
  socket.SetKeepAlive();

  if (conn_options.group_options->trans_info->set_socket_opt_function) {
    conn_options.group_options->trans_info->set_socket_opt_function(socket);
  }

  auto conn = MakeRefCounted<TcpConnection>(conn_options.group_options->reactor, socket);
  TransInfo* trans_info = conn_options.group_options->trans_info;
  auto io_handler = std::unique_ptr<IoHandler>(
    ClientIoHandlerFactory::GetInstance()->Create(trans_info->protocol, conn.Get(), trans_info));

  SetConnectionOption(conn.Get(), conn_options, std::move(conn_handler), std::move(io_handler));
  if (conn->DoConnect()) {
    return conn;
  } else {
    TRPC_LOG_ERROR("DoConnect fail, remote addr:" << conn_options.group_options->peer_addr.ToString());
  }

  return nullptr;
}

RefPtr<UdpTransceiver> CreateUdpTransceiver(const FutureConnectorOptions& conn_options,
                                            std::unique_ptr<FutureConnectionHandler>&& conn_handler) {
  bool is_ipv6 = conn_options.group_options->ip_type == NetworkAddress::IpType::kIpV6 ? true : false;
  auto socket = Socket::CreateUdpSocket(is_ipv6);
  if (!socket.IsValid()) {
    TRPC_LOG_ERROR("create udp socket failed due to invalid socket");
    return nullptr;
  }

  if (conn_options.group_options->trans_info->set_socket_opt_function) {
    conn_options.group_options->trans_info->set_socket_opt_function(socket);
  }

  auto conn = MakeRefCounted<UdpTransceiver>(conn_options.group_options->reactor, socket, true);
  TransInfo* trans_info = conn_options.group_options->trans_info;
  auto io_handler = std::unique_ptr<IoHandler>(
    ClientIoHandlerFactory::GetInstance()->Create(trans_info->protocol, conn.Get(), trans_info));
  SetConnectionOption(conn.get(), conn_options, std::move(conn_handler), std::move(io_handler));

  conn->EnableReadWrite();
  conn->StartHandshaking();
  return conn;
}

}  // namespace trpc::future
