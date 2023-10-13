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

#include "trpc/transport/client/fiber/conn_pool/fiber_tcp_conn_pool_connector_group.h"

#include <cmath>

#include "trpc/coroutine/fiber_event.h"
#include "trpc/stream/stream.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"
#include "trpc/util/log/logging.h"

namespace trpc {

FiberTcpConnPoolConnectorGroup::FiberTcpConnPoolConnectorGroup(const FiberConnectorGroup::Options& options)
    : options_(options) {
  conn_shards_ = std::make_unique<Shard[]>(options_.trans_info->fiber_connpool_shards);
  max_conn_per_shard_ = std::ceil(options_.trans_info->max_conn_num / options_.trans_info->fiber_connpool_shards);
}

FiberTcpConnPoolConnectorGroup::~FiberTcpConnPoolConnectorGroup() {}

void FiberTcpConnPoolConnectorGroup::Stop() {
  for (uint32_t i = 0; i != options_.trans_info->fiber_connpool_shards; ++i) {
    auto&& shard = conn_shards_[i];

    std::list<RefPtr<FiberTcpConnPoolConnector>> tcp_conns;
    {
      std::scoped_lock _(shard.lock);
      tcp_conns = shard.tcp_conns;
    }

    for (const auto& connector : tcp_conns) {
      TRPC_ASSERT(connector != nullptr);
      connector->Stop();
    }
  }
}

void FiberTcpConnPoolConnectorGroup::Destroy() {
  for (uint32_t i = 0; i != options_.trans_info->fiber_connpool_shards; ++i) {
    auto&& shard = conn_shards_[i];

    std::list<RefPtr<FiberTcpConnPoolConnector>> tcp_conns;
    {
      std::scoped_lock _(shard.lock);
      tcp_conns.swap(shard.tcp_conns);
    }

    while (!tcp_conns.empty()) {
      RefPtr<FiberTcpConnPoolConnector> connector = tcp_conns.back();
      TRPC_ASSERT(connector != nullptr);
      tcp_conns.pop_back();

      connector->Destroy();
    }
  }
}

int FiberTcpConnPoolConnectorGroup::SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  RefPtr<FiberTcpConnPoolConnector> connector = GetOrCreate();
  if (connector != nullptr) {
    FiberEvent e;
    // save request context
    bool flag = connector->SaveCallContext(req_msg, rsp_msg, [&e, &ret](int err_code, std::string&& err_msg) {
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

    Reclaim(ret, std::move(connector));
  }

  return ret;
}

Future<CTransportRspMsg> FiberTcpConnPoolConnectorGroup::AsyncSendRecv(CTransportReqMsg* req_msg) {
  RefPtr<FiberTcpConnPoolConnector> connector = GetOrCreate();
  if (TRPC_LIKELY(connector != nullptr)) {
    Promise<CTransportRspMsg> promise;
    auto fut = promise.GetFuture();
    CTransportRspMsg* rsp_msg = trpc::object_pool::New<CTransportRspMsg>();
    // save request context
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

    return fut.Then([this, connector = std::move(connector)](Future<CTransportRspMsg>&& fut_rsp) mutable {
      // connector is recycled to the pool only after success
      int ret = fut_rsp.IsReady() ? 0 : -1;
      this->Reclaim(ret, std::move(connector));

      return std::move(fut_rsp);
    });
  }

  return MakeExceptionFuture<CTransportRspMsg>(CommonException("network error.", TrpcRetCode::TRPC_CLIENT_NETWORK_ERR));
}

int FiberTcpConnPoolConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  int ret = TrpcRetCode::TRPC_CLIENT_NETWORK_ERR;
  RefPtr<FiberTcpConnPoolConnector> connector = GetOrCreate();
  if (TRPC_LIKELY(connector != nullptr)) {
    connector->SendReqMsg(req_msg);

    Reclaim(0, std::move(connector));

    return 0;
  }

  return ret;
}

void FiberTcpConnPoolConnectorGroup::SendRecvForBackupRequest(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg,
                                                              OnCompletionFunction&& cb) {
  RefPtr<FiberTcpConnPoolConnector> connector = GetOrCreate();
  if (TRPC_LIKELY(connector != nullptr)) {
    auto func = [this, connector, cb = std::move(cb)](int err_code, std::string&& err_msg) mutable {
      cb(err_code, std::move(err_msg));
      Reclaim(err_code, std::move(connector));
    };
    bool flag = connector->SaveCallContext(req_msg, rsp_msg, std::move(func));
    if (flag) {
      connector->SendReqMsg(req_msg);
    }
  } else {
    std::string err_msg("get connector failed, remote:");
    err_msg += options_.peer_addr.ToString();

    cb(TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(err_msg));
  }
}

stream::StreamReaderWriterProviderPtr FiberTcpConnPoolConnectorGroup::CreateStream(
    stream::StreamOptions&& stream_options) {
  RefPtr<FiberTcpConnPoolConnector> connector = GetOrCreate();
  if (TRPC_UNLIKELY(connector == nullptr)) {
    TRPC_FMT_ERROR("create stream failed due to get connection failed, peer addr: {}", options_.peer_addr.ToString());
    return nullptr;
  }

  auto* conn_handler =
      static_cast<stream::FiberClientStreamConnectionHandler*>(connector->GetConnection()->GetConnectionHandler());
  auto stream_handler = conn_handler->GetOrCreateStreamHandler();

  stream_options.connection_id = connector->GetConnId();
  stream_options.stream_handler = stream_handler;
  stream_options.fiber_mode = true;
  stream_options.server_mode = false;

  stream_options.callbacks.on_close_cb = [this, connector](int reason) {
    TRPC_FMT_TRACE("on_close_cb, reason: {}", reason);

    if (reason == 0 && connector->IsHealthy()) {
      uint32_t shard_id = (connector->GetConnId() >> 32);
      auto& shard = conn_shards_[shard_id % options_.trans_info->fiber_connpool_shards];

      std::scoped_lock _(shard.lock);
      if ((shard.tcp_conns.size() <= max_conn_per_shard_) &&
          (long_conn_num_.load(std::memory_order_relaxed) <= options_.trans_info->max_conn_num)) {
        shard.tcp_conns.push_back(std::move(connector));
        return;
      }
    }

    long_conn_num_.fetch_sub(1, std::memory_order_relaxed);
    connector->CloseConnection();
  };

  return stream_handler->CreateStream(std::move(stream_options));
}

RefPtr<FiberTcpConnPoolConnector> FiberTcpConnPoolConnectorGroup::GetOrCreate() {
  RefPtr<FiberTcpConnPoolConnector> connector{nullptr};

  // short connection
  if (TRPC_UNLIKELY(options_.trans_info->conn_type == ConnectionType::kTcpShort)) {
    connector = CreateTcpConnPoolConnector(0);
    if (connector->Init()) {
      return connector;
    }

    return nullptr;
  }

  uint32_t shard_id = shard_id_gen_.fetch_add(1);

  // long connection
  RefPtr<FiberTcpConnPoolConnector> idle_connector{nullptr};
  int retry_num = 3;

  while (retry_num > 0) {
    auto& shard = conn_shards_[shard_id % options_.trans_info->fiber_connpool_shards];

    {
      std::scoped_lock _(shard.lock);
      if (shard.tcp_conns.size() > 0) {
        connector = shard.tcp_conns.back();
        TRPC_ASSERT(connector != nullptr);

        shard.tcp_conns.pop_back();

        if (connector->IsHealthy()) {
          if (!connector->IsConnIdleTimeout()) {
            return connector;
          } else {
            idle_connector = connector;
          }
        } else {
          idle_connector = connector;
        }
      } else {
        break;
      }
    }

    if (idle_connector != nullptr) {
      idle_connector->CloseConnection();
    }

    --retry_num;
  }

  connector = CreateTcpConnPoolConnector(shard_id);
  if (connector->Init()) {
    long_conn_num_.fetch_add(1, std::memory_order_relaxed);
    return connector;
  }
  return nullptr;
}

void FiberTcpConnPoolConnectorGroup::Reclaim(int ret, RefPtr<FiberTcpConnPoolConnector>&& connector) {
  if (TRPC_UNLIKELY(options_.trans_info->conn_type == ConnectionType::kTcpShort)) {
    connector->CloseConnection();
    return;
  }

  if (ret == 0) {
    uint32_t shard_id = (connector->GetConnId() >> 32);
    auto& shard = conn_shards_[shard_id % options_.trans_info->fiber_connpool_shards];

    std::scoped_lock _(shard.lock);
    if ((shard.tcp_conns.size() <= max_conn_per_shard_) &&
        (long_conn_num_.load(std::memory_order_relaxed) <= options_.trans_info->max_conn_num)) {
      shard.tcp_conns.push_back(std::move(connector));
      return;
    }
  }

  long_conn_num_.fetch_sub(1, std::memory_order_relaxed);
  connector->CloseConnection();
}

RefPtr<FiberTcpConnPoolConnector> FiberTcpConnPoolConnectorGroup::CreateTcpConnPoolConnector(uint32_t shard_id) {
  uint64_t conn_id = static_cast<uint64_t>(shard_id) << 32;
  conn_id |= connector_id_gen_.fetch_add(1, std::memory_order_relaxed);

  FiberTcpConnPoolConnector::Options options;
  options.conn_id = conn_id;
  options.trans_info = options_.trans_info;
  options.peer_addr = &(options_.peer_addr);

  return MakeRefCounted<FiberTcpConnPoolConnector>(options);
}

}  // namespace trpc
