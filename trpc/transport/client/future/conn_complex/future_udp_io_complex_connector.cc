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

#include "trpc/transport/client/future/conn_complex/future_udp_io_complex_connector.h"

#include "trpc/transport/client/future/common/utils.h"
#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc {

FutureUdpIoComplexConnector::FutureUdpIoComplexConnector(FutureConnectorOptions&& options)
    : FutureConnector(std::move(options)),
      msg_timeout_handler_(options_.group_options->trans_info->rsp_dispatch_function) {}

bool FutureUdpIoComplexConnector::Init() {
  auto conn_handler = FutureConnComplexConnectionHandlerFactory::GetIntance()->Create(
       options_.group_options->trans_info->protocol, options_, msg_timeout_handler_);
  connection_ = future::CreateUdpTransceiver(options_, std::move(conn_handler));
  if (connection_ == nullptr) {
    return false;
  }

  // Register timer id.
  auto timeout_check_interval = options_.group_options->trans_info->request_timeout_check_interval;
  timer_id_ = options_.group_options->reactor->AddTimerAfter(0, timeout_check_interval,
                                                             [this]() { this->HandleRequestTimeout(); });
  if (timer_id_ == kInvalidTimerId) {
    return false;
  }
  return true;
}

void FutureUdpIoComplexConnector::Stop() {
  if (timer_id_ != kInvalidTimerId) {
    options_.group_options->reactor->CancelTimer(timer_id_);
    timer_id_ = kInvalidTimerId;
  }

  if (connection_ != nullptr) {
    connection_->DoClose(false);
  }
}

void FutureUdpIoComplexConnector::Destroy() {
  if (connection_ != nullptr) {
    connection_.Reset();
  }
}

int FutureUdpIoComplexConnector::SendReqMsg(CTransportReqMsg* req_msg) {
  if (connection_) {
    return SendReqMsgImpl(req_msg);
  }

  // Create new connection in case of fd exhausted.
  if (Init()) {
    return SendReqMsgImpl(req_msg);
  }

  // Final error.
  std::string error_msg = "connection is unavailable, target:";
  error_msg += options_.group_options->peer_addr.ToString();
  future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error_msg),
                            options_.group_options->trans_info->rsp_dispatch_function);
  return -1;
}

int FutureUdpIoComplexConnector::SendOnly(CTransportReqMsg* req_msg) {
  if (TRPC_UNLIKELY(connection_ == nullptr)) {
    if (!Init()) {
      TRPC_FMT_ERROR("connection is unavailable, target:{}", options_.group_options->peer_addr.ToString());
      return -1;
    }
  }

  int ret = future::SendUdpMsg(req_msg, connection_.Get());
  if (TRPC_UNLIKELY(ret != 0)) {
    TRPC_FMT_ERROR("send error, peer addr: {}", options_.group_options->peer_addr.ToString());
  }

  return ret;
}

void FutureUdpIoComplexConnector::HandleRequestTimeout() {
  msg_timeout_handler_.DoTimeout();
}

int FutureUdpIoComplexConnector::SendReqMsgImpl(CTransportReqMsg* req_msg) {
  // Backup request is not support in udp.
  if (msg_timeout_handler_.Push(req_msg) != FutureConnComplexMessageTimeoutHandler::PushResult::kOk)
    return -1;

  uint32_t seq_id = req_msg->context->GetRequestId();
  int ret = future::SendUdpMsg(req_msg, connection_.Get());
  if (ret == 0) return 0;

  std::string error = "send error, peer addr:";
  error += options_.group_options->peer_addr.ToString();
  CTransportReqMsg* msg = msg_timeout_handler_.Pop(seq_id);
  // Message not found, do nothing.
  if (!msg) return 0;

  future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                            options_.group_options->trans_info->rsp_dispatch_function);
  return 0;
}

}  // namespace trpc
