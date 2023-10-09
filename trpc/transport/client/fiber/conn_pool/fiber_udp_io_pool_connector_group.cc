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

#include "trpc/transport/client/fiber/conn_pool/fiber_udp_io_pool_connector_group.h"

#include "trpc/coroutine/fiber_event.h"
#include "trpc/util/log/logging.h"

namespace trpc {

FiberUdpIoPoolConnectorGroup::FiberUdpIoPoolConnectorGroup(const FiberConnectorGroup::Options& options)
    : options_(options) {
  TRPC_ASSERT(available_ids_.Init(options_.trans_info->max_conn_num) == 0);

  udp_pool_.clear();
  udp_pool_.resize(options_.trans_info->max_conn_num + 1);

  for (uint64_t i = 1; i <= options_.trans_info->max_conn_num; ++i) {
    udp_pool_[i] = nullptr;
    TRPC_ASSERT(available_ids_.Enqueue(i) == 0);
  }
}

FiberUdpIoPoolConnectorGroup::~FiberUdpIoPoolConnectorGroup() {}

void FiberUdpIoPoolConnectorGroup::Stop() {
  while (true) {
    uint64_t id = 0;

    available_ids_.Dequeue(id);
    if (id == 0) {
      break;
    }

    RefPtr<FiberUdpIoPoolConnector> connector = udp_pool_[id];
    if (connector != nullptr) {
      connector->Stop();
    }
  }
}

void FiberUdpIoPoolConnectorGroup::Destroy() {
  while (true) {
    uint64_t id = 0;

    available_ids_.Dequeue(id);
    if (id == 0) {
      break;
    }

    RefPtr<FiberUdpIoPoolConnector> connector = udp_pool_[id];
    if (connector != nullptr) {
      connector->Destroy();
      connector.Reset();
    }
  }

  udp_pool_.clear();
}

int FiberUdpIoPoolConnectorGroup::SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  RefPtr<FiberUdpIoPoolConnector> connector = GetOrCreate();
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

    Reclaim(ret, connector);
  }

  return ret;
}

Future<CTransportRspMsg> FiberUdpIoPoolConnectorGroup::AsyncSendRecv(CTransportReqMsg* req_msg) {
  RefPtr<FiberUdpIoPoolConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    Promise<CTransportRspMsg> promise;
    auto fut = promise.GetFuture();
    CTransportRspMsg* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
    // save request context
    connector->SaveCallContext(
        req_msg, rsp_msg, [promise = std::move(promise), rsp_msg](int err_code, std::string&& err_msg) mutable {
          if (err_code == 0) {
            promise.SetValue(std::move(*rsp_msg));
          } else {
            TRPC_LOG_WARN(err_msg);
            promise.SetException(CommonException(err_msg.c_str(), err_code));
          }
          trpc::object_pool::Delete(rsp_msg);
        });
    connector->SendReqMsg(req_msg);

    return fut.Then([this, connector](Future<CTransportRspMsg>&& fut_rsp) mutable {
      // connector is recycled to the pool only after success
      int ret = fut_rsp.IsReady() ? 0 : -1;
      this->Reclaim(ret, connector);

      return std::move(fut_rsp);
    });
  }

  return MakeExceptionFuture<CTransportRspMsg>(CommonException("network error.",
                                                 TrpcRetCode::TRPC_CLIENT_NETWORK_ERR));
}

int FiberUdpIoPoolConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  RefPtr<FiberUdpIoPoolConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    connector->SendReqMsg(req_msg);

    Reclaim(0, connector);

    return 0;
  }

  return ret;
}

RefPtr<FiberUdpIoPoolConnector> FiberUdpIoPoolConnectorGroup::GetOrCreate() {
  uint32_t available_id = GetAvailableId();
  if (available_id > 0) {
    RefPtr<FiberUdpIoPoolConnector> connector = udp_pool_[available_id];
    if (connector) {
      return connector;
    }

    connector = CreateUdpIoPoolConnector(available_id);
    if (connector->Init()) {
      udp_pool_[available_id] = connector;
    } else {
      Reclaim(available_id);
      connector = nullptr;
    }

    return connector;
  }

  available_id = 0;
  RefPtr<FiberUdpIoPoolConnector> connector = CreateUdpIoPoolConnector(available_id);
  if (connector->Init()) {
    return connector;
  }

  return nullptr;
}

uint64_t FiberUdpIoPoolConnectorGroup::GetAvailableId() {
  uint64_t id = 0;

  available_ids_.Dequeue(id);

  return id;
}

void FiberUdpIoPoolConnectorGroup::Reclaim(uint64_t id) {
  available_ids_.Enqueue(id);
}

void FiberUdpIoPoolConnectorGroup::Reclaim(int ret, RefPtr<FiberUdpIoPoolConnector>& connector) {
  if (connector->GetConnId() == 0) {
    connector->Stop();
    connector->Destroy();
    return;
  }

  if (ret == 0) {
    Reclaim(connector->GetConnId());
  } else {
    connector->Stop();
    connector->Destroy();
    udp_pool_[connector->GetConnId()] = nullptr;
  }
}

RefPtr<FiberUdpIoPoolConnector> FiberUdpIoPoolConnectorGroup::CreateUdpIoPoolConnector(uint32_t id) {
  FiberUdpIoPoolConnector::Options options;
  options.conn_id = id;
  options.trans_info = options_.trans_info;
  options.is_ipv6 = options_.is_ipv6;

  return MakeRefCounted<FiberUdpIoPoolConnector>(options);
}

}  // namespace trpc
