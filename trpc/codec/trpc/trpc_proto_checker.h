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

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief Decodes a complete request/response/streaming-frame protocol message from binary byte stream (zero-copy).
int CheckTrpcProtocolMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out);

/// @brief Extracts metadata of trpc protocol message (type of data-frame, stream ID, type of streaming-frame).
bool PickTrpcProtocolMessageMetadata(const std::any& message, std::any& data);

}  // namespace trpc
