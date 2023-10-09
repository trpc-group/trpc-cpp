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

#include "trpc/transport/client/fiber/conn_complex/fiber_udp_io_complex_connector_group.h"

#include "trpc/coroutine/fiber_event.h"

namespace trpc {

FiberUdpIoComplexConnectorGroup::FiberUdpIoComplexConnectorGroup(const FiberConnectorGroup::Options& options)
    : options_(options) {
  max_conn_num_ = options_.trans_info->max_conn_num;
  if (max_conn_num_ == 64) {
    // If max_conn_num is the default value of 64, change it to 1
    max_conn_num_ = 1;
  }

  udp_connectors_.resize(max_conn_num_);

  for (size_t i = 0; i < max_conn_num_; ++i) {
    udp_connectors_[i] = CreateUdpIoComplexConnector();
  }
}

RefPtr<FiberUdpIoComplexConnector> FiberUdpIoComplexConnectorGroup::CreateUdpIoComplexConnector() {
  FiberUdpIoComplexConnector::Options options;
  options.conn_id = 0;
  options.trans_info = options_.trans_info;
  options.is_ipv6 = options_.is_ipv6;

  auto connector = MakeRefCounted<FiberUdpIoComplexConnector>(options);
  if (connector->Init()) {
    return connector;
  }

  return nullptr;
}

FiberUdpIoComplexConnectorGroup::~FiberUdpIoComplexConnectorGroup() {}

void FiberUdpIoComplexConnectorGroup::Stop() {
  for (size_t i = 0; i < max_conn_num_; ++i) {
    if (udp_connectors_[i] != nullptr) {
      udp_connectors_[i]->Stop();
    }
  }
}

void FiberUdpIoComplexConnectorGroup::Destroy() {
  for (size_t i = 0; i < max_conn_num_; ++i) {
    if (udp_connectors_[i] != nullptr) {
      udp_connectors_[i]->Destroy();
    }
  }
}

int FiberUdpIoComplexConnectorGroup::SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  RefPtr<FiberUdpIoComplexConnector> connector = Get();
  if (connector != nullptr) {
    FiberEvent e;
    // save request context
    connector->SaveCallContext(req_msg, rsp_msg,
                               [&e, &ret](uint32_t err_code, std::string&& err_msg) {
                                 ret = err_code;
                                 if (ret != 0) {
                                   TRPC_LOG_WARN(err_msg);
                                 }
                                 e.Set();
                               });
    connector->SendReqMsg(req_msg);

    e.Wait();
  }

  return ret;
}

Future<CTransportRspMsg> FiberUdpIoComplexConnectorGroup::AsyncSendRecv(CTransportReqMsg* req_msg) {
  RefPtr<FiberUdpIoComplexConnector> connector = Get();
  if (connector != nullptr) {
    Promise<CTransportRspMsg> promise;
    auto fut = promise.GetFuture();
    CTransportRspMsg* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
    // save request context
    connector->SaveCallContext(
        req_msg, rsp_msg, [promise = std::move(promise), rsp_msg](uint32_t err_code, std::string&& err_msg) mutable {
          if (err_code == 0) {
            promise.SetValue(std::move(*rsp_msg));
          } else {
            TRPC_LOG_WARN(err_msg);
            promise.SetException(CommonException(err_msg.c_str(), err_code));
          }
          trpc::object_pool::Delete(rsp_msg);
        });
    connector->SendReqMsg(req_msg);

    return fut;
  }

  return MakeExceptionFuture<CTransportRspMsg>(CommonException("network error.",
                                               TrpcRetCode::TRPC_CLIENT_NETWORK_ERR));
}

int FiberUdpIoComplexConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  RefPtr<FiberUdpIoComplexConnector> connector = Get();
  if (connector.Get() != nullptr) {
    connector->SendReqMsg(req_msg);
    return 0;
  }

  return TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
}

RefPtr<FiberUdpIoComplexConnector> FiberUdpIoComplexConnectorGroup::Get() {
  thread_local size_t tls_index = 0;

  for (size_t i = 0; i < max_conn_num_; ++i) {
    size_t uid = (tls_index + i) % max_conn_num_;
    if (udp_connectors_[uid] != nullptr) {
      tls_index++;
      return udp_connectors_[uid];
    }
  }

  return nullptr;
}

}  // namespace trpc
