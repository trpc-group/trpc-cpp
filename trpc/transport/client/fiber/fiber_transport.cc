//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/transport/client/fiber/fiber_transport.h"

#include <string>

#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/fiber_runtime.h"
// #include "trpc/stream/fiber_stream_connection_handler.h"
#include "trpc/transport/client/fiber/common/fiber_backup_request_retry.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/thread/latch.h"

namespace trpc {

int FiberTransport::Init(Options&& params) {
  connector_group_manager_ = std::make_unique<FiberConnectorGroupManager>(std::move(params.trans_info));

  return 0;
}

void FiberTransport::Stop() {
  if (connector_group_manager_) {
    connector_group_manager_->Stop();
  }
}

void FiberTransport::Destroy() {
  if (connector_group_manager_) {
    connector_group_manager_->Destroy();
  }
}

int FiberTransport::SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  if (IsRunningInFiberWorker()) {
    if (!req_msg->context->IsBackupRequest()) {
      FiberConnectorGroup* connector_group = connector_group_manager_->Get(req_msg->context->GetNodeAddr());
      if (connector_group != nullptr) {
        return connector_group->SendRecv(req_msg, rsp_msg);
      }

      TRPC_FMT_ERROR("Can't get connector group of {}:{}", req_msg->context->GetNodeAddr().ip,
                                                           req_msg->context->GetNodeAddr().port);
      return TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR;
    }

    return SendRecvForBackupRequest(req_msg, rsp_msg);
  }

  return SendRecvFromOutSide(req_msg, rsp_msg);
}

int FiberTransport::SendRecvFromOutSide(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  Latch latch(1);
  int ret = 0;
  bool start_ret = StartFiberDetached([this, &latch, req_msg, rsp_msg, &ret]() {
    ret = SendRecv(req_msg, rsp_msg);
    latch.count_down();
  });

  if (start_ret) {
    latch.wait();
    return ret;
  }

  return TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR;
}

int FiberTransport::SendRecvForBackupRequest(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  int ret_code = TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR;

  BackupRequestRetryInfo* backup_info = req_msg->context->GetBackupRequestRetryInfo();
  backup_info->IncrCount();

  TRPC_ASSERT(backup_info->backup_addrs.size() == 2);

  auto sync_retry = object_pool::MakeLwShared<FiberBackupRequestRetry>();
  sync_retry->SetResourceDeleter([backup_info]() { backup_info->DecrCount(); });
  sync_retry->SetRetryTimes(backup_info->backup_addrs.size());

  backup_info->retry = sync_retry.Get();

  NoncontiguousBuffer buff_back(req_msg->send_data);

  for (int i = 0; i < 2; ++i) {
    auto cb = [&ret_code, i, backup_info, sync_retry](int err_code, std::string&& err_msg) {
      if (sync_retry->IsFinished()) {
        return;
      }

      if (err_code == 0) {
        ret_code = err_code;
        backup_info->succ_rsp_node_index = i;
        sync_retry->SetFinished();
      } else {
        if (sync_retry->IsFailedCountUpToAll()) {
          ret_code = err_code;
          backup_info->succ_rsp_node_index = -1;
          sync_retry->SetFinished();
        }
      }
    };

    auto& addr = backup_info->backup_addrs[i].addr;
    FiberConnectorGroup* connector_group = connector_group_manager_->Get(addr);
    if (connector_group != nullptr) {
      connector_group->SendRecvForBackupRequest(req_msg, rsp_msg, std::move(cb));
    } else {
      TRPC_FMT_ERROR("Can't get connector group of {}:{}", addr.ip, addr.port);
      cb(TrpcRetCode::TRPC_CLIENT_CONNECT_ERR, "connector_group is nullptr");
    }

    if (i == 0) {
      if (ret_code != TrpcRetCode::TRPC_CLIENT_CONNECT_ERR) {
        if (sync_retry->Wait(backup_info->delay) && ret_code == 0) {
          return ret_code;
        }
        uint32_t timeout = req_msg->context->GetTimeout();
        req_msg->context->SetTimeout(timeout - backup_info->delay);
        req_msg->send_data = std::move(buff_back);
      }
      backup_info->resend_count += 1;
    } else {
      sync_retry->Wait();
    }
  }

  return ret_code;
}

Future<CTransportRspMsg> FiberTransport::AsyncSendRecv(CTransportReqMsg* req_msg) {
  if (IsRunningInFiberWorker()) {
    if (!req_msg->context->IsBackupRequest()) {
      FiberConnectorGroup* connector_group = connector_group_manager_->Get(req_msg->context->GetNodeAddr());
      if (connector_group != nullptr) {
        return connector_group->AsyncSendRecv(req_msg);
      }

      TRPC_FMT_ERROR("Can't get connector group of {}:{}", req_msg->context->GetNodeAddr().ip,
                                                           req_msg->context->GetNodeAddr().port);
      return MakeExceptionFuture<CTransportRspMsg>(
          CommonException("not found connector group.", TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR));
    }

    return MakeExceptionFuture<CTransportRspMsg>(
        CommonException("not implement.", TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR));
  }

  return AsyncSendRecvFromOutSide(req_msg);
}

Future<CTransportRspMsg> FiberTransport::AsyncSendRecvFromOutSide(CTransportReqMsg* req_msg) {
  Promise<CTransportRspMsg> promise;
  auto fut = promise.GetFuture();
  bool start_ret = StartFiberDetached([this, req_msg, promise = std::move(promise)]() mutable {
    AsyncSendRecv(req_msg).Then([promise = std::move(promise)](Future<CTransportRspMsg>&& rsp_fut) mutable {
      if (rsp_fut.IsReady()) {
        promise.SetValue(rsp_fut.GetValue0());
      } else {
        promise.SetException(rsp_fut.GetException());
      }

      return MakeReadyFuture<>();
    });
  });

  if (start_ret) {
    return fut;
  }

  return MakeExceptionFuture<CTransportRspMsg>(
      CommonException("create fiber failed.", TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR));
}

int FiberTransport::SendOnly(CTransportReqMsg* req_msg) {
  int ret = 0;

  if (IsRunningInFiberWorker()) {
    FiberConnectorGroup* connector_group = connector_group_manager_->Get(req_msg->context->GetNodeAddr());
    if (connector_group != nullptr) {
      ret = connector_group->SendOnly(req_msg);
    } else {
      TRPC_FMT_ERROR("Can't get connector group of {}:{}", req_msg->context->GetNodeAddr().ip,
                                                           req_msg->context->GetNodeAddr().port);
      ret = TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR;
    }

    object_pool::Delete(req_msg);
  } else {
    ret = SendOnlyFromOutSide(req_msg);
  }

  return ret;
}

int FiberTransport::SendOnlyFromOutSide(CTransportReqMsg* req_msg) {
  Latch latch(1);
  int ret = 0;
  bool start_ret = StartFiberDetached([this, &latch, &ret, req_msg]() {
    ret = SendOnly(req_msg);
    latch.count_down();
  });

  if (start_ret) {
    latch.wait();

    return ret;
  }

  return TrpcRetCode::TRPC_CLIENT_OVERLOAD_ERR;
}

stream::StreamReaderWriterProviderPtr FiberTransport::CreateStream(const NodeAddr& addr,
                                                                   stream::StreamOptions&& stream_options) {
  FiberConnectorGroup* connector_group = connector_group_manager_->Get(addr);
  if (connector_group) {
    return connector_group->CreateStream(std::move(stream_options));
  }

  TRPC_FMT_ERROR("Can't get connector group of {}:{}", addr.ip, addr.port);
  return nullptr;
}

}  // namespace trpc
