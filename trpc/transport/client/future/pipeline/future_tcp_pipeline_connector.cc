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

#include "trpc/transport/client/future/pipeline/future_tcp_pipeline_connector.h"

#include <string>
#include <utility>

#include "trpc/filter/filter.h"
#include "trpc/util/time.h"

namespace trpc {

FuturePipelineConnectionHandler::FuturePipelineConnectionHandler(
  const FutureConnectorOptions& options,
  FuturePipelineMessageTimeoutHandler& msg_timeout_handler)
    : FutureConnectionHandler(options, options.group_options->trans_info),
      msg_timeout_handler_(msg_timeout_handler) {
  SetPipelineSendNotifyFunc([this](const IoMessage& io_msg) {
    // Oneway request, do nothing.
    if (io_msg.is_oneway) return;

    this->NotifyPipelineSendMsg(io_msg.seq_id);
  });
}

bool FuturePipelineConnectionHandler::HandleMessage(const ConnectionPtr& conn,
                                                    std::deque<std::any>& rsp_list) {
  bool conn_reusable = true;
  auto& rsp_decode_function = options_.group_options->trans_info->rsp_decode_function;
  for (auto&& rsp_buf : rsp_list) {
    ProtocolPtr rsp_protocol;
    bool ret = rsp_decode_function(std::move(rsp_buf), rsp_protocol);
    std::optional<uint32_t> request_id = GetRequestId();
    if (TRPC_UNLIKELY(!request_id)) {
      TRPC_FMT_ERROR("can not find request id , maybe cleared");
      return false;
    }

    CTransportReqMsg* msg = msg_timeout_handler_.PopFromSendQueueById(*request_id);
    if (!msg) {
      TRPC_FMT_ERROR("can not find request id: {}, ,maybe timeout", *request_id);
      if (ret) continue;

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

    std::string error = "decode failed peer addr= ";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_DECODE_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
    return false;
  }

  return conn_reusable;
}

void FuturePipelineConnectionHandler::CleanResource() {
  CTransportReqMsg* msg = nullptr;
  while ((msg = msg_timeout_handler_.PopFromSendQueue()) != nullptr) {
    std::string error = "network error peer addr= ";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
  }

  // Must SubmitTask to delay the deletion of connection to avoid wild pointer.
  std::unique_ptr<FutureConnector> connector = conn_pool_->GetAndDelConnector(GetConnection()->GetConnId());
  options_.group_options->reactor->SubmitInnerTask([connector = std::move(connector)]() {
    if (connector) {
      connector->Stop();
      connector->Destroy();
    }
  });
}

void FuturePipelineConnectionHandler::NotifyPipelineSendMsg(uint32_t req_id) {
  send_request_id_.push(req_id);
}

std::optional<uint32_t> FuturePipelineConnectionHandler::GetRequestId() {
  if (send_request_id_.empty()) {
    return std::nullopt;
  }

  uint32_t request_id = send_request_id_.front();
  send_request_id_.pop();
  return request_id;
}

bool FutureTcpPipelineConnector::Init() {
  /// @note Here not goes through factory.
  auto conn_event_handler = std::make_unique<FuturePipelineConnectionHandler>(options_, msg_timeout_handler_);
  conn_event_handler->SetConnPool(conn_pool_);
  connection_ = future::CreateTcpConnection(options_, std::move(conn_event_handler));
  if (connection_ == nullptr) {
    return false;
  }
  return true;
}

void FutureTcpPipelineConnector::Stop() {
  if (connection_ != nullptr) {
    connection_->DoClose(false);
  }
}

void FutureTcpPipelineConnector::Destroy() {
  if (connection_ != nullptr) {
    connection_.Reset();
  }
}

int FutureTcpPipelineConnector::SendReqMsg(CTransportReqMsg* req_msg) {
  if (TRPC_UNLIKELY(GetConnectionState() == ConnectionState::kConnecting)) {
    if (kConnectInterval + connection_->GetDoConnectTimestamp() < trpc::time::GetMilliSeconds()) {
      msg_timeout_handler_.PushToSendQueue(req_msg);
      connection_->DoClose(true);
      return -1;
    }
  }

  bool push_ret = msg_timeout_handler_.PushToSendQueue(req_msg);
  if (!push_ret) return -1;

  int send_ret = future::SendTcpMsg(req_msg, connection_.Get());
  if (!send_ret) return 0;

  CTransportReqMsg* msg = msg_timeout_handler_.PopFromSendQueueById(req_msg->context->GetRequestId());
  if (msg) {
    std::string error = "network send error, peer addr:";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
  }
  return -1;
}

int FutureTcpPipelineConnector::SendOnly(CTransportReqMsg* req_msg) {
  if (TRPC_UNLIKELY(GetConnectionState() == ConnectionState::kConnecting)) {
    if (kConnectInterval + connection_->GetDoConnectTimestamp() < trpc::time::GetMilliSeconds()) {
      TRPC_FMT_WARN("connect timeout, peer addr = {}", options_.group_options->peer_addr.ToString());
      connection_->DoClose(true);
      return -1;
    }
  }

  int ret = future::SendTcpMsg(req_msg, connection_.Get(), true);
  if (!ret) return 0;

  TRPC_FMT_WARN("send failed, peer addr = {}", options_.group_options->peer_addr.ToString());
  return -1;
}

void FutureTcpPipelineConnector::HandleRequestTimeout() {
  // When a timeout occurs, close the connection to avoid packet collision.
  if (msg_timeout_handler_.HandleSendQueueTimeout()) {
    connection_->DoClose(true);
  }
}

void FutureTcpPipelineConnector::HandleIdleConnection(uint64_t now_ms) {
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

}  // namespace trpc
