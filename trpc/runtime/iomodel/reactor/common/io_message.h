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

#pragma once

#include <any>
#include <string>

#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

struct IoMessage {
  // Message unique id
  uint32_t seq_id;

  // Context flag, for special scene(http header request)
  uint32_t context_ext;

  // Peer ip
  std::string ip;

  // Peer port
  uint16_t port;

  // Whether oneway call
  bool is_oneway{false};

  // The buffer send to network
  NoncontiguousBuffer buffer;

  // It stores servercontext or clientcontext
  std::any msg;
};

}  // namespace trpc
