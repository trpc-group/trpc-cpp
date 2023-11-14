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

#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler.h"

#include <string>
#include <utility>

#include "trpc/filter/filter.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"
#include "trpc/util/time.h"

namespace trpc {

bool FutureTcpConnPoolConnectionHandler::HandleMessage(const ConnectionPtr& conn,
                                                       std::deque<std::any>& rsp_list) {
  bool conn_reusable = true;
  auto& rsp_decode_function = options_.group_options->trans_info->rsp_decode_function;
  for (auto&& rsp_buf : rsp_list) {
    ProtocolPtr rsp_protocol;
    bool ret = rsp_decode_function(std::move(rsp_buf), rsp_protocol);
    CTransportReqMsg* msg = msg_timeout_handler_.PopFromSendQueue();
    if (!msg) {
      TRPC_FMT_ERROR("can not find request msg, maybe timeout");
      // Decode success, continue handling.
      if (ret) continue;

      // Decode error, let caller close connection.
      return false;
    }

    // For rpcz.
    auto filter_function = options_.group_options->trans_info->run_client_filters_function;
    if (filter_function) {
      filter_function(FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, msg);
    }

    // Decode success.
    if (ret) {
      conn_reusable &= rsp_protocol->IsConnectionReusable();
      auto* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
      rsp_msg->msg = std::move(rsp_protocol);
      future::DispatchResponse(msg, rsp_msg, options_.group_options->trans_info->rsp_dispatch_function,
                               options_.group_options->trans_info->run_client_filters_function);
      continue;
    }

    std::string error = "decode failed peer addr = ";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_DECODE_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
    // Decode error, let caller close connection.
    return false;
  }

  // When there is no pending request to send, and current connection
  // is not fixed type, just recycle connection.
  if (!SendPendingMsg() && !options_.is_fixed_connection) {
    conn_pool_->RecycleConnectorId(conn->GetConnId());
  }

  return conn_reusable;
}

void FutureTcpConnPoolConnectionHandler::CleanResource() {
  CTransportReqMsg* msg = msg_timeout_handler_.PopFromSendQueue();
  if (msg) {
    std::string error = "Network error peer addr = ";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
  }

  while ((msg = msg_timeout_handler_.PopFromPendingQueue()) != nullptr) {
    std::string error = "Network error peer addr = ";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
  }

  msg_timeout_handler_.NeedCheckQueueWhenExit(false);

  // Must SubmitTask to delay the deletion of connection to avoid wild pointer.
  std::unique_ptr<FutureConnector> connector = conn_pool_->GetAndDelConnector(GetConnection()->GetConnId());
  options_.group_options->reactor->SubmitInnerTask([connector = std::move(connector)]() mutable {
    if (connector) {
      connector->Stop();
      connector->Destroy();
    }
  });
}

uint32_t FutureTcpConnPoolConnectionHandler::GetMergeRequestCount() {
  const CTransportReqMsg* msg = msg_timeout_handler_.GetSendMsg();
  if (!msg) return 1;

  return msg->context->GetPipelineCount();
}

bool FutureTcpConnPoolConnectionHandler::SendPendingMsg() {
  CTransportReqMsg* pend_msg = nullptr;
  if ((pend_msg = msg_timeout_handler_.PopFromPendingQueue()) || (pend_msg = conn_pool_->PopFromPendingQueue())) {
    msg_timeout_handler_.PushToSendQueue(pend_msg);

    int ret = future::SendTcpMsg(pend_msg, GetConnection());
    if (!ret) return true;

    std::string error = "network send error, peer addr:";
    error += options_.group_options->peer_addr.ToString();
    pend_msg = msg_timeout_handler_.PopFromSendQueue();
    if (pend_msg) {
      future::DispatchException(pend_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                                options_.group_options->trans_info->rsp_dispatch_function);
    }

    return false;
  }

  return false;
}


bool FutureUdpIoPoolConnectionHandler::HandleMessage(const ConnectionPtr& conn,
                                                     std::deque<std::any>& rsp_list) {
  for (auto&& rsp_buf : rsp_list) {
    ProtocolPtr rsp_protocol;
    CTransportReqMsg* msg = msg_timeout_handler_.PopFromSendQueue();
    if (!msg) {
      TRPC_LOG_ERROR("can not find request msg, maybe timeout");
      continue;
    }

    // For rpcz.
    if (options_.group_options->trans_info->run_client_filters_function) {
      options_.group_options->trans_info->run_client_filters_function(FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, msg);
    }

    bool ret = options_.group_options->trans_info->rsp_decode_function(std::move(rsp_buf), rsp_protocol);
    if (ret) {
      auto* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
      TRPC_ASSERT(rsp_protocol);
      rsp_msg->msg = std::move(rsp_protocol);
      future::DispatchResponse(msg, rsp_msg, options_.group_options->trans_info->rsp_dispatch_function,
                               options_.group_options->trans_info->run_client_filters_function);
      continue;
    }

    std::string error = "decode failed peer addr = ";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_DECODE_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
  }

  // When there is no pending request to send, just recycle connection.
  if (!SendPendingMsg()) {
    conn_pool_->RecycleConnectorId(conn->GetConnId());
  }

  return true;
}

uint32_t FutureUdpIoPoolConnectionHandler::GetMergeRequestCount() {
  const CTransportReqMsg* msg = msg_timeout_handler_.GetSendMsg();
  if (!msg) return 1;

  return msg->context->GetPipelineCount();
}

bool FutureUdpIoPoolConnectionHandler::SendPendingMsg() {
  CTransportReqMsg* pend_msg = nullptr;

  if ((pend_msg = conn_pool_->PopFromPendingQueue()) == nullptr) return false;

  msg_timeout_handler_.PushToSendQueue(pend_msg);
  if (future::SendUdpMsg(pend_msg, GetConnection()) != 0) {
    pend_msg = msg_timeout_handler_.PopFromSendQueue();
    if (pend_msg) {
      std::string error = "send failed peer addr = ";
      error += options_.group_options->peer_addr.ToString();
      future::DispatchException(pend_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                                options_.group_options->trans_info->rsp_dispatch_function);
    }

    return false;
  }

  return true;
}

}  // namespace trpc
