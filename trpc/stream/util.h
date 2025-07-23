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

#include <cstdint>

#include "trpc/codec/server_codec.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/server/server_context.h"

namespace trpc::stream {

/// @brief Internal implementations for stream sub-module.
/// @private For internal use purpose only.
namespace internal {

// @brief Builds server context from connection and server codec.
ServerContextPtr BuildServerContext(const ConnectionPtr& conn, const ServerCodecPtr& codec, uint64_t recv_timestamp_us);

}  // namespace internal

}  // namespace trpc::stream
