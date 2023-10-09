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

#pragma once

#include <memory>
#include <string>

#include "trpc/client/redis/reply.h"
#include "trpc/client/redis/request.h"
#include "trpc/codec/protocol.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

using RedisRequestPtr = std::shared_ptr<redis::Request>;
using RedisResponsePtr = std::shared_ptr<redis::Reply>;

/// @brief Redis request protocol message is mainly used to package the redis::Request to make the code consistent
/// @private For internal use purpose only.
class RedisRequestProtocol : public trpc::Protocol {
 public:
  /// @private For internal use purpose only.
  RedisRequestProtocol() = default;

  /// @private For internal use purpose only.
  ~RedisRequestProtocol() override = default;

  /// @brief Decodes or encodes redis request protocol message (zero copy).
  /// @private For internal use purpose only.
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override { return false; }
  /// @private For internal use purpose only.
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override;

  /// @private For internal use purpose only.
  bool EncodeImpl(NoncontiguousBufferBuilder& builder, NoncontiguousBuffer& buff) const;

 public:
  // Header of redis request protocol
  redis::Request redis_req;
};

/// @brief Redis response protocol message is mainly used to package the redis::Reply to make the code consistent
/// @private For internal use purpose only.
class RedisResponseProtocol : public trpc::Protocol {
 public:
  /// @private For internal use purpose only.
  RedisResponseProtocol() = default;
  /// @private For internal use purpose only.
  ~RedisResponseProtocol() override = default;

  /// @brief Decodes or encodes redis response protocol message (zero copy).
  /// @private For internal use purpose only.
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override { return false; }
  /// @private For internal use purpose only.
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override { return false; }

 public:
  // Header of  redis response protocol
  trpc::redis::Reply redis_rsp;
};

using RedisRequestProtocolPtr = std::shared_ptr<RedisRequestProtocol>;
using RedisResponseProtocolPtr = std::shared_ptr<RedisResponseProtocol>;

}  // namespace trpc
