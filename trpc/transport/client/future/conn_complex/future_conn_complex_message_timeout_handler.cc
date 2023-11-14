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

#include "trpc/transport/client/future/conn_complex/future_conn_complex_message_timeout_handler.h"

#include <utility>

#include "trpc/transport/client/future/common/utils.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

FutureConnComplexMessageTimeoutHandler::FutureConnComplexMessageTimeoutHandler(
    const TransInfo::RspDispatchFunction& rsp_dispatch_function)
    : timeout_queue_(kTimeoutQueueHashBucketSize), rsp_dispatch_function_(rsp_dispatch_function) {
  // Init timeout handler.
  timeout_handle_function_ = [this](const internal::TimingWheelTimeoutQueue::DataIterator& iter) {
    CTransportReqMsg* msg = this->timeout_queue_.GetAndPop(iter);

    /// @note backup_promise not empty, means a lot.
    ///       1. Current request is the original one.
    ///       2. User request for resend if original timeout.
    ///       3. Current request is pushed into timeout queue
    ///          by user rpc, not by below logic.
    if (msg->extend_info->backup_promise) {
      TRPC_LOG_WARN("request to " << msg->context->GetIp() << ":" << msg->context->GetPort()
                                  << " failed, resend to another channel now");
      Exception ex(CommonException("timeout queue resend timeout", TRPC_CLIENT_INVOKE_TIMEOUT_ERR));
      // Trigger backup requests sent here, and backup promise will be released then.
      future::NotifyBackupRequestResend(std::move(ex), msg->extend_info->backup_promise);

      // Put original request back into timeout queue, timeout minus delay.
      BackupRequestRetryInfo* retry_info_ptr = msg->context->GetBackupRequestRetryInfo();
      TRPC_ASSERT(msg->context->GetTimeout() > retry_info_ptr->delay);
      msg->context->SetTimeout(msg->context->GetTimeout() - retry_info_ptr->delay);

      if (this->Push(msg) != PushResult::kOk) {
        TRPC_FMT_ERROR("push failure when resend.");
      }
      return;
    }

    /// @note Three scenarios go into here.
    ///       1. Original request first timeout without user configuring backup request.
    ///       2. Original request second timeout with user configuring backup request.
    ///       3. All the backup requests timeout.
    std::string error = "future invoke timeout, peer addr = ";
    error += msg->context->GetIp().c_str();
    error += ":";
    error += std::to_string(msg->context->GetPort());
    future::DispatchException(msg, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, std::move(error),
                              this->rsp_dispatch_function_);
  };
}

FutureConnComplexMessageTimeoutHandler::PushResult FutureConnComplexMessageTimeoutHandler::Push(
    CTransportReqMsg* req_msg) {
  // Timeout queue overloaded.
  if (GetTimeoutQueueSize() > kTimeoutQueueLimitSize) {
    std::string err = "client overload in timeout queue, limit " + std::to_string(kTimeoutQueueLimitSize);
    future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR, std::move(err), rsp_dispatch_function_);
    return PushResult::kError;
  }

  BackupRequestRetryInfo* retry_info_ptr = req_msg->context->GetBackupRequestRetryInfo();
  int64_t timeout = req_msg->context->GetTimeout();
  /// @note When backup promise not empty, it is the original
  ///       request pushed here by user rpc, we need to timeout
  ///       in retry delay in order to trigger backup requests.
  if (req_msg->extend_info->backup_promise) {
    TRPC_ASSERT(retry_info_ptr);
    timeout = retry_info_ptr->delay;
  }

  timeout += trpc::time::GetMilliSeconds();
  bool ret = timeout_queue_.Push(req_msg->context->GetRequestId(), req_msg, timeout);
  if (TRPC_UNLIKELY(!ret)) {
    // Request id conflict in backup request scenario.
    if (retry_info_ptr && timeout_queue_.Contains(req_msg->context->GetRequestId())) {
      return PushResult::kExisted;
    }

    std::string err = "client request push to timeout queue failed, maybe seq_id is conflict, id = ";
    err += std::to_string(req_msg->context->GetRequestId());
    future::DispatchException(req_msg, TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, std::move(err), rsp_dispatch_function_);
    return PushResult::kError;
  }

  return PushResult::kOk;
}

CTransportReqMsg* FutureConnComplexMessageTimeoutHandler::Pop(uint32_t request_id) {
  return timeout_queue_.Pop(request_id);
}

CTransportReqMsg* FutureConnComplexMessageTimeoutHandler::Get(uint32_t request_id) {
  return timeout_queue_.Get(request_id);
}

void FutureConnComplexMessageTimeoutHandler::DoTimeout() {
  timeout_queue_.DoTimeout(trpc::time::GetMilliSeconds(), timeout_handle_function_);
}

}  // namespace trpc
