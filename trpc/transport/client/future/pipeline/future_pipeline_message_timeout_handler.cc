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

#include "trpc/transport/client/future/pipeline/future_pipeline_message_timeout_handler.h"

#include <utility>

#include "trpc/transport/client/future/common/utils.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

FuturePipelineMessageTimeoutHandler::FuturePipelineMessageTimeoutHandler(
    const TransInfo::RspDispatchFunction& rsp_dispatch_function)
    : rsp_dispatch_function_(rsp_dispatch_function) {
  timeout_handle_function_ = [this] (const internal::TimingWheelTimeoutQueue::DataIterator& iter) {
    CTransportReqMsg* msg = this->send_queue_.GetAndPop(iter);
    if (msg->extend_info->backup_promise) {
      TRPC_FMT_WARN("request to {}:{}  failed, resend to another channel", msg->context->GetIp(),
                     msg->context->GetPort());
      Exception ex(CommonException("send queue resend timeout", TRPC_CLIENT_INVOKE_TIMEOUT_ERR));
      future::NotifyBackupRequestResend(std::move(ex), msg->extend_info->backup_promise);

      BackupRequestRetryInfo* retry_info_ptr = msg->context->GetBackupRequestRetryInfo();
      TRPC_ASSERT(msg->context->GetTimeout() > retry_info_ptr->delay);
      msg->context->SetTimeout(msg->context->GetTimeout() - retry_info_ptr->delay);
      this->PushToSendQueue(msg);
      return;
    }

    std::string error = "Future invoke timeout before send, peer addr:";
    error += msg->context->GetIp().c_str();
    error += ":";
    error += std::to_string(msg->context->GetPort());
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, std::move(error),
                              this->rsp_dispatch_function_);
    has_timeout_ = true;
  };
}

FuturePipelineMessageTimeoutHandler::~FuturePipelineMessageTimeoutHandler() {
  CTransportReqMsg* msg = nullptr;
  while ((msg = PopFromSendQueue()) != nullptr) {
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, "", rsp_dispatch_function_);
  }
}

bool FuturePipelineMessageTimeoutHandler::PushToSendQueue(CTransportReqMsg* req_msg) {
  size_t size = send_queue_.Size();
  if (size > kSendQueueSize) {
    future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR,
                              "Queue overload", rsp_dispatch_function_);
    return false;
  }

  int64_t timeout = req_msg->context->GetTimeout();
  if (req_msg->extend_info->backup_promise) {
    BackupRequestRetryInfo* retry_info_ptr = req_msg->context->GetBackupRequestRetryInfo();
    timeout = retry_info_ptr->delay;
  }
  timeout += trpc::time::GetMilliSeconds();

  if (send_queue_.Push(req_msg->context->GetRequestId(), req_msg, timeout)) return true;

  std::string err = "push to send queue failed, maybe seq id is conflict, id = ";
  err += std::to_string(req_msg->context->GetRequestId());
  future::DispatchException(req_msg, TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, std::move(err), rsp_dispatch_function_);
  return false;
}

CTransportReqMsg* FuturePipelineMessageTimeoutHandler::PopFromSendQueue() {
  return send_queue_.Pop();
}

CTransportReqMsg* FuturePipelineMessageTimeoutHandler::PopFromSendQueueById(uint32_t request_id) {
  return send_queue_.Pop(request_id);
}

bool FuturePipelineMessageTimeoutHandler::HandleSendQueueTimeout() {
  has_timeout_ = false;
  send_queue_.DoTimeout(trpc::time::GetMilliSeconds(), timeout_handle_function_);
  return has_timeout_;
}

}  // namespace trpc
