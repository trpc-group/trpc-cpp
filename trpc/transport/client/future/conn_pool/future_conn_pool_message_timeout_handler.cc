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

#include "trpc/transport/client/future/conn_pool/future_conn_pool_message_timeout_handler.h"

#include <utility>

#include "trpc/transport/client/future/common/utils.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

FutureConnPoolMessageTimeoutHandler::FutureConnPoolMessageTimeoutHandler(uint64_t conn_id,
  const TransInfo::RspDispatchFunction& rsp_dispatch_function,
  internal::SharedSendQueue& shared_send_queue)
    : conn_id_(conn_id),
    rsp_dispatch_function_(rsp_dispatch_function),
    shared_send_queue_(shared_send_queue) {

  timeout_handle_function_ = [this] (const internal::SmallCacheTimingWheelTimeoutQueue::DataIterator& iter) {
    CTransportReqMsg* msg = pending_queue_.GetAndPop(iter);

    if (msg->extend_info->backup_promise) {
      Exception ex(CommonException("Pending queue resend timeout", TRPC_CLIENT_INVOKE_TIMEOUT_ERR));
      future::NotifyBackupRequestResend(std::move(ex), msg->extend_info->backup_promise);

      BackupRequestRetryInfo* retry_info_ptr = msg->context->GetBackupRequestRetryInfo();
      TRPC_ASSERT(msg->context->GetTimeout() > retry_info_ptr->delay);
      msg->context->SetTimeout(msg->context->GetTimeout() - retry_info_ptr->delay);
      PushToPendingQueue(msg);
      return;
    }

    std::string error = "Future invoke timeout before send, peer addr:";
    error += msg->context->GetIp().c_str();
    error += ":";
    error += std::to_string(msg->context->GetPort());
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, std::move(error),
                              rsp_dispatch_function_);
  };
}

FutureConnPoolMessageTimeoutHandler::~FutureConnPoolMessageTimeoutHandler() {
  if (!need_check_queue_when_exit_) return;

  CTransportReqMsg* msg = nullptr;
  while ((msg = PopFromSendQueue()) != nullptr) {
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, "", rsp_dispatch_function_);
  }

  while ((msg = PopFromPendingQueue()) != nullptr) {
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, "", rsp_dispatch_function_);
  }
}

bool FutureConnPoolMessageTimeoutHandler::PushToSendQueue(CTransportReqMsg* req_msg) {
  TRPC_ASSERT(IsSendQueueEmpty());

  int64_t timeout = req_msg->context->GetTimeout();
  if (req_msg->extend_info->backup_promise) {
    BackupRequestRetryInfo* retry_info_ptr = req_msg->context->GetBackupRequestRetryInfo();
    timeout = retry_info_ptr->delay;
  }
  timeout += trpc::time::GetMilliSeconds();

  return shared_send_queue_.Push(conn_id_, req_msg, timeout);
}

CTransportReqMsg* FutureConnPoolMessageTimeoutHandler::PopFromSendQueue() {
  return shared_send_queue_.Pop(conn_id_);
}

const CTransportReqMsg* FutureConnPoolMessageTimeoutHandler::GetSendMsg() {
  return shared_send_queue_.Get(conn_id_);
}

bool FutureConnPoolMessageTimeoutHandler::PushToPendingQueue(CTransportReqMsg* req_msg) {
  size_t size = pending_queue_.Size();
  if (size > kPendingQueueSize) {
    std::string err = "Client Overload in Pending Queue";
    future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR, std::move(err), rsp_dispatch_function_);
    return false;
  }

  int64_t timeout = req_msg->context->GetTimeout();
  if (req_msg->extend_info->backup_promise) {
    BackupRequestRetryInfo* retry_info_ptr = req_msg->context->GetBackupRequestRetryInfo();
    timeout = retry_info_ptr->delay;
  }
  timeout += trpc::time::GetMilliSeconds();

  if (pending_queue_.Push(req_msg->context->GetRequestId(), req_msg, timeout)) {
    return true;
  }

  // Push failed.
  std::string err = "Client Request Push to Pending Queue Failed, Maybe Seq Id is conflict, id = ";
  err += std::to_string(req_msg->context->GetRequestId());
  future::DispatchException(req_msg, TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, std::move(err), rsp_dispatch_function_);
  return false;
}

CTransportReqMsg* FutureConnPoolMessageTimeoutHandler::PopFromPendingQueue() {
  return pending_queue_.Pop();
}

bool FutureConnPoolMessageTimeoutHandler::HandleSendQueueTimeout(CTransportReqMsg* msg) {
  if (!msg) return false;

  if (msg->extend_info->backup_promise) {
    TRPC_FMT_DEBUG("request {}:{} failed, resend to another channel",
                    msg->context->GetIp(),
                    msg->context->GetPort());
    Exception ex(CommonException("Send queue resend timeout", TRPC_CLIENT_INVOKE_TIMEOUT_ERR));
    future::NotifyBackupRequestResend(std::move(ex), msg->extend_info->backup_promise);
    BackupRequestRetryInfo* retry_info_ptr = msg->context->GetBackupRequestRetryInfo();

    TRPC_ASSERT(msg->context->GetTimeout() > retry_info_ptr->delay);
    msg->context->SetTimeout(msg->context->GetTimeout() - retry_info_ptr->delay);
    PushToSendQueue(msg);
    return false;
  }

  std::string error = "future invoke timeout, peer addr = ";
  error += msg->context->GetIp().c_str();
  error += ":";
  error += std::to_string(msg->context->GetPort());
  future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, std::move(error),
                            rsp_dispatch_function_);
  return true;
}

void FutureConnPoolMessageTimeoutHandler::HandlePendingQueueTimeout() {
  uint64_t now_ms = trpc::time::GetMilliSeconds();
  pending_queue_.DoTimeout(now_ms, timeout_handle_function_);
}

}  // namespace trpc
