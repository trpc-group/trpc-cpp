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

#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/function.h"

namespace trpc::stream::grpc {

// @brief Write all non-contiguous buffer data to the I/O until it is completed.
// @note Can be used in scenarios where all data needs to be sent during the handshake stage, such as gRPC sending
// handshake preface.
// @return 0 means all writes were successful, >0 means a write error occurred (e.g. remote connection disconnect),
// with an error code consistent with errno, and <0 indicates an internal write failure.
int SendHandshakingMessage(IoHandler* io_handler, NoncontiguousBuffer&& buffer);

}  // namespace trpc::stream::grpc
