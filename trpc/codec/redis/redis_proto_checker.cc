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

#include "trpc/codec/redis/redis_proto_checker.h"

#include <utility>
#include <vector>

#include "trpc/client/redis/reader.h"

namespace trpc {

// Only native Redis supports pipeline, istore does not support it
int RedisZeroCopyCheckResponse(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  if (in.Empty() || in.ByteSize() < 1) {
    return PacketChecker::PACKET_LESS;
  }

  redis::Reader reader;
  // Allow parsing multiple packets at once when SupportPipeline is true.
  if (conn->SupportPipeline()) {
    while (in.ByteSize() > 0) {
      if (!reader.GetReply(in, out)) {
        break;
      }
    }
    if (reader.IsProtocolError()) {
      return PacketChecker::PACKET_ERR;
    }
    return out.empty() ? PacketChecker::PACKET_LESS : PacketChecker::PACKET_FULL;
  }

  // Backward compatibility with previous redis-level-pipeline usage
  uint32_t pipeline_count = conn->GetMergeRequestCount();
  reader.GetReply(in, out, pipeline_count);
  if (reader.IsProtocolError()) {
    return PacketChecker::PACKET_ERR;
  }
  return out.empty() ? PacketChecker::PACKET_LESS : PacketChecker::PACKET_FULL;
}

}  // namespace trpc
