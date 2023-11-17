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

#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler.h"

#include <atomic>
#include <utility>

#include "trpc/filter/filter.h"
#include "trpc/util/check.h"
#include "trpc/util/time.h"
#include "trpc/util/log/logging.h"

namespace trpc {

bool FutureTcpConnComplexConnectionHandler::HandleMessage(const ConnectionPtr& conn,
                                                          std::deque<std::any>& rsp_list) {
  bool conn_reusable = true;
  auto& rsp_decode_function = options_.group_options->trans_info->rsp_decode_function;
  for (auto&& rsp_buf : rsp_list) {
    ProtocolPtr rsp_protocol;
    // Decode failed.
    if (!rsp_decode_function(std::move(rsp_buf), rsp_protocol)) {
      conn_reusable = false;
      TRPC_FMT_ERROR("rsp_decode_function decode error, destory connectin, conn_id: {}", conn->GetConnId());
      break;
    }

    conn_reusable &= rsp_protocol->IsConnectionReusable();
    // Get request id from response when conn complex.
    uint32_t seq_id = 0;
    rsp_protocol->GetRequestId(seq_id);
    CTransportReqMsg* msg = msg_timeout_handler_.Pop(seq_id);
    // Request not found in timeout queue, maybe response is too late.
    /// @note When backup request required, it may happen if response not comes
    ///       in retry delay time, which leading to backup request sent.
    if (!msg) {
      TRPC_FMT_INFO("can not find request id: {}, request maybe timeout", seq_id);
      continue;
    }

    // In backup request scenario, multiple connections may run the same request.
    if (TRPC_UNLIKELY(conn->GetConnId() != msg->extend_info->connection_id)) {
      BackupRequestRetryInfo* retry_info_ptr = msg->context->GetBackupRequestRetryInfo();
      if (retry_info_ptr) {
        /// @note Is this right or not.
        msg->context->SetAddr(conn->GetPeerIp(), conn->GetPeerPort());
        // Only one backup node supported.
        retry_info_ptr->succ_rsp_node_index = 1;
      }
    }

    // For rpcz.
    auto filter_function = options_.group_options->trans_info->run_client_filters_function;
    if (filter_function) {
      filter_function(FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, msg);
    }

    auto* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
    rsp_msg->msg = std::move(rsp_protocol);
    future::DispatchResponse(msg, rsp_msg, options_.group_options->trans_info->rsp_dispatch_function,
                             options_.group_options->trans_info->run_client_filters_function);
  }

  return conn_reusable;
}

void FutureTcpConnComplexConnectionHandler::CleanResource() {
  GetConnection()->GetConnectionHandler()->Stop();
  GetConnection()->GetConnectionHandler()->Join();

  if (del_conn_func_) {
    del_conn_func_();
  }
}

void FutureTcpConnComplexConnectionHandler::NotifyMessageFailed(const IoMessage& msg, ConnectionErrorCode code) {
  CTransportReqMsg* req_msg = msg_timeout_handler_.Pop(msg.seq_id);
  if (!req_msg) return;

  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  std::string error = "connection close, peer addr:";
  if (code == ConnectionErrorCode::kConnectingTimeout) {
    ret = TrpcRetCode::TRPC_CLIENT_CONNECT_ERR;
    error = "connection timeout, peer addr:";
  }

  error += req_msg->context->GetIp();
  error += ":";
  error += std::to_string(req_msg->context->GetPort());

  future::DispatchException(req_msg, ret, std::move(error),
                            options_.group_options->trans_info->rsp_dispatch_function);
}

bool FutureUdpIoComplexConnectionHandler::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) {
  auto& rsp_decode_function = options_.group_options->trans_info->rsp_decode_function;
  for (auto&& rsp_buf : rsp_list) {
    ProtocolPtr rsp_protocol;
    if (!rsp_decode_function(std::move(rsp_buf), rsp_protocol)) {
      TRPC_FMT_ERROR("rsp_decode_function decode error, destory conn, conn_id: {}", conn->GetConnId());
      return false;
    }

    // Decode success.
    TRPC_ASSERT(rsp_protocol);
    uint32_t seq_id = 0;
    rsp_protocol->GetRequestId(seq_id);
    CTransportReqMsg* msg = msg_timeout_handler_.Pop(seq_id);
    if (!msg) {
      TRPC_FMT_ERROR("can not find request id: {}, maybe timeout", seq_id);
    }

    // For rpcz.
    auto filter_function = options_.group_options->trans_info->run_client_filters_function;
    if (filter_function) {
      filter_function(FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, msg);
    }

    auto* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
    rsp_msg->msg = std::move(rsp_protocol);
    future::DispatchResponse(msg, rsp_msg, options_.group_options->trans_info->rsp_dispatch_function,
                             options_.group_options->trans_info->run_client_filters_function);
  }

  return true;
}

void FutureUdpIoComplexConnectionHandler::NotifyMessageFailed(const IoMessage& msg, ConnectionErrorCode code) {
  CTransportReqMsg* req_msg = msg_timeout_handler_.Pop(msg.seq_id);
  if (!req_msg) return;

  std::string error = "network send error, peer addr:";
  error += msg.ip.c_str();
  error += ":";
  error += std::to_string(msg.port);

  future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                            options_.group_options->trans_info->rsp_dispatch_function);
}

}  // namespace trpc
