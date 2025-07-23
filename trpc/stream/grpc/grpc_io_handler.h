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

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"

namespace trpc {

/// @brief IO handler of gRPC.
class GrpcIoHandler : public DefaultIoHandler {
 public:
  explicit GrpcIoHandler(Connection* conn) : DefaultIoHandler(conn) {}

  /// @brief Handshaking for HTTP2 preface.
  HandshakeStatus Handshake(bool is_read_event) override;
};

}  // namespace trpc
