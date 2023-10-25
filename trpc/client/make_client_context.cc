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

#include "trpc/client/make_client_context.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "trpc/util/hash_util.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

namespace {

// callbacks for makeclientcontext
static std::vector<MakeClientContextCallback> callbacks;

static constexpr uint16_t kMaxShardLocks = 64;
// A shared lock array to reduce lock granularity.
static std::unique_ptr<std::mutex[]> g_shard_locks_ = std::make_unique<std::mutex[]>(kMaxShardLocks);

void RunMakeClientContextCallbacks(const ServerContextPtr& server_context, ClientContextPtr& client_context) {
  for (auto& cb : callbacks) {
    cb(server_context, client_context);
  }
}

void ConstructClientContext(const ServerContextPtr& ctx, bool with_trans_info, ClientContextPtr& client_ctx) {
  client_ctx->SetMessageType(ctx->GetMessageType());
  client_ctx->SetCallerName(ctx->GetCalleeName());
  client_ctx->SetCallerFuncName(ctx->GetFuncName());

  if (with_trans_info) {
    const auto& trans_info = ctx->GetPbReqTransInfo();
    if (trans_info.size() > 0) {
      client_ctx->SetReqTransInfo(trans_info.begin(), trans_info.end());
    }
  }

  RunMakeClientContextCallbacks(ctx, client_ctx);

  // Calculate the remaining timeout and set it to the client context.
  int64_t now_ms = static_cast<int64_t>(trpc::time::GetMilliSeconds());
  int64_t cost_time = now_ms - ctx->GetRecvTimestamp();
  int64_t left_time = std::max<int64_t>(static_cast<int64_t>(ctx->GetTimeout()) - cost_time, 0);

  if (ctx->IsUseFullLinkTimeout()) {
    client_ctx->SetFullLinkTimeout(left_time);
  } else {
    client_ctx->SetTimeout(left_time);
  }
}

}  // namespace

void RegisterMakeClientContextCallback(MakeClientContextCallback&& callback) {
  callbacks.emplace_back(std::move(callback));
}

ClientContextPtr MakeClientContext(const ServiceProxyPtr& proxy) {
  TRPC_ASSERT(proxy && "proxy must be created before create a ClientContext obj");
  return MakeRefCounted<ClientContext>(proxy->GetClientCodec());
}

ClientContextPtr MakeClientContext(const ServiceProxyPtr& proxy, const ProtocolPtr& req, const ProtocolPtr& rsp) {
  auto ctx = MakeRefCounted<ClientContext>();
  ctx->SetRequest(req);
  ctx->SetResponse(rsp);

  return ctx;
}


ClientContextPtr MakeClientContext(const ServerContextPtr& ctx, const ServiceProxyPtr& prx, bool with_trans_info) {
  ClientContextPtr client_ctx = MakeClientContext(prx);

  ConstructClientContext(ctx, with_trans_info, client_ctx);

  return client_ctx;
}

ClientContextPtr MakeClientContext(const ServerContextPtr& ctx, const ServiceProxyPtr& proxy,
                                   const ProtocolPtr& req, const ProtocolPtr& rsp,
                                   bool with_trans_info) {
  ClientContextPtr client_ctx = MakeClientContext(proxy, req, rsp);

  ConstructClientContext(ctx, with_trans_info, client_ctx);

  return client_ctx;
}

ClientContextPtr MakeTransparentClientContext(const ServerContextPtr& ctx, const ServiceProxyPtr& proxy) {
  ClientContextPtr client_ctx = MakeClientContext(proxy);

  client_ctx->SetCallType(ctx->GetCallType());
  client_ctx->SetCallerName(ctx->GetCallerName());
  client_ctx->SetFuncName(ctx->GetFuncName());
  client_ctx->SetCallerFuncName(ctx->GetFuncName());
  client_ctx->SetMessageType(ctx->GetMessageType());
  client_ctx->SetReqEncodeType(ctx->GetReqEncodeType());
  client_ctx->SetReqCompressType(ctx->GetReqCompressType());
  client_ctx->SetTransparent(true);

  const auto& trans_info = ctx->GetPbReqTransInfo();
  if (trans_info.size() > 0) {
    client_ctx->SetReqTransInfo(trans_info.begin(), trans_info.end());
  }

  RunMakeClientContextCallbacks(ctx, client_ctx);

  int64_t now_ms = static_cast<int64_t>(trpc::time::GetMilliSeconds());
  int64_t cost_time = now_ms - ctx->GetRecvTimestamp();
  int64_t left_time = std::max<int64_t>(static_cast<int64_t>(ctx->GetTimeout()) - cost_time, 0);

  if (ctx->IsUseFullLinkTimeout()) {
    client_ctx->SetFullLinkTimeout(left_time);
  } else {
    client_ctx->SetTimeout(left_time);
  }

  return client_ctx;
}

void BackFillServerTransInfo(const ClientContextPtr& client_context, const ServerContextPtr& server_context) {
  uint32_t seq_id = server_context->GetRequestId();
  auto&& lock = g_shard_locks_[GetHashIndex(seq_id, kMaxShardLocks)];
  const auto& rsp_trans_info = client_context->GetPbRspTransInfo();
  std::scoped_lock _(lock);
  server_context->SetRspTransInfo(rsp_trans_info.begin(), rsp_trans_info.end());
}

}  // namespace trpc
