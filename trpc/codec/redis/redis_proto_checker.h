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

#include <any>
#include <deque>
#include <memory>
#include <string>

#include "trpc/client/redis/reader.h"
#include "trpc/codec/redis/redis_protocol.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief Decodes redis response protocol messages from binary bytes (zero-copy).
/// @param conn the connection that received the binary bytes
/// @param in the response binary bytes
/// @param out[out] a list of successfully decoded redis responses which actual type is redis::reply
/// @return the result of packet parsing:
///         kPacketLess: the response packet was not received completely
///         kPacketFull: at least one response packet has been received
///         kPacketError: parsed protocol error
/// @private For internal use purpose only.
int RedisZeroCopyCheckResponse(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out);

}  // namespace trpc
