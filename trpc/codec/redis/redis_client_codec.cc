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

#include "trpc/codec/redis/redis_client_codec.h"

#include <sstream>

#include "trpc/client/redis/reply.h"
#include "trpc/client/redis/request.h"
#include "trpc/codec/redis/redis_proto_checker.h"
#include "trpc/codec/redis/redis_protocol.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int32_t RedisClientCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  return RedisZeroCopyCheckResponse(conn, in, out);
}

bool RedisClientCodec::FillRequest(const ClientContextPtr& context, const ProtocolPtr& in, void* body) {
  auto* redis_req_protocol = static_cast<RedisRequestProtocol*>(in.get());

  redis::Request* redis_raw_req = static_cast<redis::Request*>(body);

  redis_req_protocol->redis_req = std::move(*redis_raw_req);

  return true;
}

bool RedisClientCodec::ZeroCopyEncode(const ClientContextPtr& context, const ProtocolPtr& in,
                                      NoncontiguousBuffer& out) {
  return in->ZeroCopyEncode(out);
}

bool RedisClientCodec::ZeroCopyDecode(const ClientContextPtr& context, std::any&& in, ProtocolPtr& out) {
  bool ret = true;
  try {
    // Construct redis::Reply when checker,so get from param in
    redis::Reply&& decode_reply = std::any_cast<redis::Reply&&>(std::move(in));
    auto* redis_rsp_protocol = static_cast<RedisResponseProtocol*>(out.get());
    redis_rsp_protocol->redis_rsp = std::move(decode_reply);
  } catch (std::exception& ex) {
    TRPC_LOG_ERROR("Redis decode throw exception: " << ex.what());
    ret = false;
  }

  return ret;
}

bool RedisClientCodec::FillResponse(const ClientContextPtr& context, const ProtocolPtr& in, void* out) {
  auto* redis_rsp_protocol = static_cast<RedisResponseProtocol*>(in.get());
  redis::Reply* redis_raw_rsp = static_cast<redis::Reply*>(out);

  *redis_raw_rsp = std::move(redis_rsp_protocol->redis_rsp);
  return true;
}

ProtocolPtr RedisClientCodec::CreateRequestPtr() { return std::make_shared<RedisRequestProtocol>(); }

ProtocolPtr RedisClientCodec::CreateResponsePtr() { return std::make_shared<RedisResponseProtocol>(); }

}  // namespace trpc
