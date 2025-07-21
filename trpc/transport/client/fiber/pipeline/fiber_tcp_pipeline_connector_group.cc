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

#include "trpc/transport/client/fiber/pipeline/fiber_tcp_pipeline_connector_group.h"

#include "trpc/coroutine/fiber_event.h"
#include "trpc/util/log/logging.h"

namespace trpc {

FiberTcpPipelineConnectorGroup::FiberTcpPipelineConnectorGroup(const FiberConnectorGroup::Options& options)
    : options_(options),
      impl_{std::make_unique<Impl>().release()} {
  max_conn_num_ = options_.trans_info->max_conn_num;

  TRPC_ASSERT(max_conn_num_ > 0);

  std::vector<RefPtr<FiberTcpPipelineConnector>> tcp_conns;
  tcp_conns.resize(max_conn_num_);

  for (size_t i = 0; i < max_conn_num_; ++i) {
    tcp_conns[i] = nullptr;
  }

  impl_.load(std::memory_order_relaxed)->tcp_conns = std::move(tcp_conns);
}

FiberTcpPipelineConnectorGroup::~FiberTcpPipelineConnectorGroup() {}

void FiberTcpPipelineConnectorGroup::Stop() {
  Impl* imp = nullptr;
  {
    std::scoped_lock lock(mutex_);
    Hazptr hazptr;
    imp = hazptr.Keep(&impl_);
  }

  if (!imp) {
    return;
  }

  for (std::size_t i = 0; i < imp->tcp_conns.size(); i++) {
    auto connector = imp->tcp_conns[i];
    if (connector != nullptr) {
      connector->Stop();
    }
  }
}

void FiberTcpPipelineConnectorGroup::Destroy() {
  Impl* imp = nullptr;
  {
    std::scoped_lock lock(mutex_);
    imp = impl_.exchange(nullptr, std::memory_order_relaxed);
  }

  if (!imp) {
    return;
  }

  for (std::size_t i = 0; i < imp->tcp_conns.size(); i++) {
    auto connector = imp->tcp_conns[i];
    if (connector != nullptr) {
      connector->Destroy();
    }
  }

  imp->tcp_conns.clear();
  imp->Retire();
}

int FiberTcpPipelineConnectorGroup::SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  RefPtr<FiberTcpPipelineConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    FiberEvent e;
    // save request context
    bool flag = connector->SaveCallContext(req_msg, rsp_msg,
                               [&e, &ret](uint32_t err_code, std::string&& err_msg) {
                                 ret = err_code;
                                 if (ret != 0) {
                                   TRPC_LOG_WARN(err_msg);
                                 }
                                 e.Set();
                               });
    if (flag) {
      connector->SendReqMsg(req_msg);

      e.Wait();
    }

    Reclaim(ret, connector);
  }

  return ret;
}

Future<CTransportRspMsg> FiberTcpPipelineConnectorGroup::AsyncSendRecv(CTransportReqMsg* req_msg) {
  RefPtr<FiberTcpPipelineConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    Promise<CTransportRspMsg>& promise = req_msg->extend_info->promise;
    auto fut = promise.GetFuture();
    CTransportRspMsg* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
    // save request context
    bool flag = connector->SaveCallContext(
        req_msg, rsp_msg, [promise_ptr = &promise, rsp_msg](int err_code, std::string&& err_msg) mutable {
          if (err_code == 0) {
            promise_ptr->SetValue(std::move(*rsp_msg));
          } else {
            TRPC_LOG_WARN(err_msg);
            promise_ptr->SetException(CommonException(err_msg.c_str(), err_code));
          }
          trpc::object_pool::Delete(rsp_msg);
        });
    if (flag) {
      connector->SendReqMsg(req_msg);
    }

    return fut.Then([this, connector = std::move(connector)](Future<CTransportRspMsg>&& fut_rsp) mutable {
      int ret = fut_rsp.IsReady() ? 0 : -1;
      this->Reclaim(ret, connector);

      return std::move(fut_rsp);
    });
  }

  return MakeExceptionFuture<CTransportRspMsg>(CommonException("network error.",
                                               TrpcRetCode::TRPC_CLIENT_NETWORK_ERR));
}

int FiberTcpPipelineConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  RefPtr<FiberTcpPipelineConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    connector->SendOnly(req_msg);
    return 0;
  }

  return TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
}

void FiberTcpPipelineConnectorGroup::SendRecvForBackupRequest(
    CTransportReqMsg* req_msg,
    CTransportRspMsg* rsp_msg,
    OnCompletionFunction&& cb) {
  RefPtr<FiberTcpPipelineConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    auto func = [this, connector, cb = std::move(cb)](int err_code,
                                                      std::string&& err_msg) mutable {
      cb(err_code, std::move(err_msg));
      Reclaim(err_code, connector);
    };
    bool flag = connector->SaveCallContext(req_msg, rsp_msg, std::move(func));
    if (flag) {
      connector->SendReqMsg(req_msg);
    }
  } else {
    int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
    std::string err_msg("get connector failed, remote: ");
    err_msg += options_.peer_addr.ToString();

    cb(ret, std::move(err_msg));
  }
}

RefPtr<FiberTcpPipelineConnector> FiberTcpPipelineConnectorGroup::GetOrCreate() {
  thread_local size_t tls_index = 0;

  size_t uid = tls_index % max_conn_num_;
  RefPtr<FiberTcpPipelineConnector> connector{nullptr};
  RefPtr<FiberTcpPipelineConnector> idle_connector{nullptr};

  {
    Hazptr hazptr;
    auto ptr = hazptr.Keep(&impl_);
    connector = ptr->tcp_conns.at(uid);
    if (connector.Get() != nullptr && connector->IsHealthy()) {
      if (!connector->IsConnIdleTimeout()) {
        ++tls_index;
        return connector;
      }
    }
  }

  {
    auto new_impl = std::make_unique<Impl>();

    std::scoped_lock _(mutex_);
    {
      Hazptr hazptr;
      new_impl->tcp_conns = hazptr.Keep(&impl_)->tcp_conns;
    }

    connector = new_impl->tcp_conns.at(uid);

    uint64_t conn_id = 0;
    if (connector != nullptr) {
      if (connector->IsHealthy()) {
        if (!connector->IsConnIdleTimeout()) {
          ++tls_index;
          return connector;
        } else {
          conn_id = connector->GetConnId() + max_conn_num_;
          connector->SetHealthy(false);
          idle_connector = connector;
        }
      } else {
        conn_id = connector->GetConnId() + max_conn_num_;
      }
    } else {
      conn_id = uid;
    }

    connector = CreateTcpPipelineConnector(conn_id);
    if (connector->Init()) {
      new_impl->tcp_conns[uid] = connector;
    } else {
      new_impl->tcp_conns[uid] = nullptr;
      connector = nullptr;
    }
    impl_.exchange(new_impl.release(), std::memory_order_relaxed)->Retire();
  }

  if (idle_connector) {
    idle_connector->CloseConnection();
  }

  return connector;
}

RefPtr<FiberTcpPipelineConnector> FiberTcpPipelineConnectorGroup::CreateTcpPipelineConnector(uint64_t conn_id) {
  FiberTcpPipelineConnector::Options options;
  options.conn_id = conn_id;
  options.trans_info = options_.trans_info;
  options.peer_addr = &(options_.peer_addr);
  options.connector_group = this;

  return MakeRefCounted<FiberTcpPipelineConnector>(options);
}

void FiberTcpPipelineConnectorGroup::Reclaim(int ret, RefPtr<FiberTcpPipelineConnector>& connector) {
  if (ret != 0) {
    connector->CloseConnection();
  }
}

}  // namespace trpc
