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

#include "trpc/transport/client/fiber/conn_complex/fiber_tcp_conn_complex_connector_group.h"

#include "trpc/coroutine/fiber_event.h"
#include "trpc/stream/stream.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"
#include "trpc/util/log/logging.h"

namespace trpc {

FiberTcpConnComplexConnectorGroup::FiberTcpConnComplexConnectorGroup(const FiberConnectorGroup::Options& options)
    : options_(options) {
  max_conn_num_ = options_.trans_info->max_conn_num;
  if (max_conn_num_ == 64) {
    // If max_conn_num is the default value of 64, change it to 1
    max_conn_num_ = 1;
  }

  TRPC_LOG_DEBUG("conn_complex max_conn_num_:" << max_conn_num_ << ", protocol:" << options_.trans_info->protocol);

  conn_impl_ = std::make_unique<ConnectorImpl[]>(max_conn_num_);

  for (size_t i = 0; i < max_conn_num_; ++i) {
    conn_impl_[i].impl.store(std::make_unique<Impl>().release(), std::memory_order_relaxed);
  }
}

FiberTcpConnComplexConnectorGroup::~FiberTcpConnComplexConnectorGroup() {}

void FiberTcpConnComplexConnectorGroup::Stop() {
  destroy_tcp_conns_.clear();

  for (size_t i = 0; i < max_conn_num_; ++i) {
    std::scoped_lock lock(conn_impl_[i].mut);

    auto ptr = conn_impl_[i].impl.exchange(nullptr, std::memory_order_relaxed);
    if (ptr) {
      if (ptr->tcp_conn != nullptr) {
        TRPC_LOG_DEBUG("Stop i:" << i << ", ip:" << options_.peer_addr.Ip() << ", port:" << options_.peer_addr.Port());
        destroy_tcp_conns_.push_back(ptr->tcp_conn);
      }
      ptr->Retire();
    }
  }

  for (auto& item : destroy_tcp_conns_) {
    item->Stop();
  }
}

void FiberTcpConnComplexConnectorGroup::Destroy() {
  for (auto& item : destroy_tcp_conns_) {
    TRPC_LOG_DEBUG("Destroy ip:" << options_.peer_addr.Ip() << ", port:" << options_.peer_addr.Port());
    item->Destroy();
  }
}

int FiberTcpConnComplexConnectorGroup::SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  RefPtr<FiberTcpConnComplexConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    FiberEvent e;
    // save request context
    bool flag = connector->SaveCallContext(req_msg, rsp_msg, [&e, &ret](uint32_t err_code, std::string&& err_msg) {
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
  }

  return ret;
}

Future<CTransportRspMsg> FiberTcpConnComplexConnectorGroup::AsyncSendRecv(CTransportReqMsg* req_msg) {
  RefPtr<FiberTcpConnComplexConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    Promise<CTransportRspMsg> promise;
    auto fut = promise.GetFuture();
    CTransportRspMsg* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
    bool flag = connector->SaveCallContext(
        req_msg, rsp_msg, [promise = std::move(promise), rsp_msg](uint32_t err_code, std::string&& err_msg) mutable {
          if (err_code == 0) {
            promise.SetValue(std::move(*rsp_msg));
          } else {
            TRPC_LOG_WARN(err_msg);
            promise.SetException(CommonException(err_msg.c_str(), err_code));
          }
          trpc::object_pool::Delete(rsp_msg);
        });
    if (flag) {
      connector->SendReqMsg(req_msg);
    }

    return fut;
  }

  return MakeExceptionFuture<CTransportRspMsg>(CommonException("network error.", TrpcRetCode::TRPC_CLIENT_NETWORK_ERR));
}

int FiberTcpConnComplexConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  RefPtr<FiberTcpConnComplexConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    connector->SendReqMsg(req_msg);
    return 0;
  }

  return TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
}

void FiberTcpConnComplexConnectorGroup::SendRecvForBackupRequest(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg,
                                                                 OnCompletionFunction&& cb) {
  RefPtr<FiberTcpConnComplexConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    bool flag = connector->SaveCallContext(req_msg, rsp_msg, std::move(cb));
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

stream::StreamReaderWriterProviderPtr FiberTcpConnComplexConnectorGroup::CreateStream(
    stream::StreamOptions&& stream_options) {
  RefPtr<FiberTcpConnComplexConnector> connector = GetOrCreate();
  if (connector == nullptr) {
    TRPC_LOG_ERROR("create stream failed due to get connection failed"
                   << ", ip:" << options_.peer_addr.Ip() << ", port:" << options_.peer_addr.Port());
    return nullptr;
  }

  auto conn_handler =
      static_cast<stream::FiberClientStreamConnectionHandler*>(connector->GetConnection()->GetConnectionHandler());
  auto stream_handler = conn_handler->GetOrCreateStreamHandler();

  stream_options.connection_id = connector->GetConnId();
  stream_options.stream_handler = stream_handler;
  stream_options.fiber_mode = true;
  stream_options.server_mode = false;

  return stream_handler->CreateStream(std::move(stream_options));
}

RefPtr<FiberTcpConnComplexConnector> FiberTcpConnComplexConnectorGroup::GetOrCreate() {
  size_t index = index_.fetch_add(1, std::memory_order_relaxed);
  size_t uid = index % max_conn_num_;

  RefPtr<FiberTcpConnComplexConnector> connector{nullptr};
  {
    Hazptr hazptr;
    auto ptr = hazptr.Keep(&(conn_impl_[uid].impl));

    connector = ptr->tcp_conn;

    if (connector != nullptr && connector->IsHealthy()) {
      if (!connector->IsConnIdleTimeout()) {
        return connector;
      }
      TRPC_LOG_DEBUG("connection uid:" << uid << ", ip:" << options_.peer_addr.Ip()
                                       << ", port:" << options_.peer_addr.Port() << ", idle timeout.");
    }
  }

  TRPC_LOG_DEBUG("connection uid:" << uid << ", ip:" << options_.peer_addr.Ip()
                                   << ", port:" << options_.peer_addr.Port() << ", get nullptr.");

  RefPtr<FiberTcpConnComplexConnector> idle_connector{nullptr};

  {
    uint64_t conn_id = 0;

    std::scoped_lock _(conn_impl_[uid].mut);

    {
      Hazptr hazptr;
      auto ptr = hazptr.Keep(&(conn_impl_[uid].impl));
      connector = ptr->tcp_conn;

      if (connector != nullptr && connector->IsHealthy()) {
        if (!connector->IsConnIdleTimeout()) {
          return connector;
        } else {
          conn_id = connector->GetConnId() + max_conn_num_;
          connector->SetHealthy(false);
          idle_connector = connector;
        }
      } else {
        conn_id = uid;
      }
    }

    TRPC_LOG_DEBUG("connection uid:" << uid << ", ip:" << options_.peer_addr.Ip()
                                     << ", port:" << options_.peer_addr.Port() << ", get nullptr too.");

    auto new_impl = std::make_unique<Impl>();

    connector = CreateTcpConnComplexConnector(conn_id);
    if (connector->Init()) {
      new_impl->tcp_conn = connector;
    } else {
      new_impl->tcp_conn = nullptr;
      connector = nullptr;
      TRPC_LOG_DEBUG("connection uid:" << uid << ", ip:" << options_.peer_addr.Ip()
                                       << ", port:" << options_.peer_addr.Port() << ", init failed.");
    }

    conn_impl_[uid].impl.exchange(new_impl.release(), std::memory_order_relaxed)->Retire();
  }

  if (idle_connector) {
    TRPC_LOG_DEBUG("connection uid:" << idle_connector->GetConnId() << ", ip:" << options_.peer_addr.Ip()
                                     << ", port:" << options_.peer_addr.Port() << ", idle timeout.");
    idle_connector->CloseConnection();
  }

  return connector;
}

RefPtr<FiberTcpConnComplexConnector> FiberTcpConnComplexConnectorGroup::CreateTcpConnComplexConnector(
    uint64_t conn_id) {
  FiberTcpConnComplexConnector::Options options;
  options.conn_id = conn_id;
  options.trans_info = options_.trans_info;
  options.peer_addr = &(options_.peer_addr);
  options.connector_group = this;

  return MakeRefCounted<FiberTcpConnComplexConnector>(options);
}

bool FiberTcpConnComplexConnectorGroup::DelConnector(FiberTcpConnComplexConnector* connector) {
  size_t uid = connector->GetConnId() % max_conn_num_;

  std::scoped_lock lock(conn_impl_[uid].mut);

  bool is_found{false};
  {
    Hazptr hazptr;
    auto ptr = hazptr.Keep(&(conn_impl_[uid].impl));
    if (!ptr) {
      return false;
    }

    if (ptr->tcp_conn.Get() == connector) {
      is_found = true;
    }
  }

  auto new_impl = std::make_unique<Impl>();

  if (is_found) {
    new_impl->tcp_conn = nullptr;

    TRPC_LOG_DEBUG("DelConnector uid:" << connector->GetConnId() << ", ip:" << options_.peer_addr.Ip()
                                       << ", port:" << options_.peer_addr.Port());

    conn_impl_[uid].impl.exchange(new_impl.release(), std::memory_order_relaxed)->Retire();

    return true;
  }

  return false;
}

}  // namespace trpc
