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

#include "trpc/stream/grpc/grpc_io_handler.h"

#include "trpc/util/log/logging.h"

namespace trpc {

IoHandler::HandshakeStatus GrpcIoHandler::Handshake(bool is_read_event) {
  // NOTE: After adding SSL features, you need to complete the SSL handshake first before the Preface stage.
  Connection* conn = GetConnection();
  TRPC_ASSERT(conn != nullptr && "Connection can't be nullptr");

  ConnectionHandler* conn_handler = conn->GetConnectionHandler();
  TRPC_ASSERT(conn_handler != nullptr && "ConnectionHandler can't be nullptr");

  return conn_handler->DoHandshake() == 0 ? HandshakeStatus::kSucc : HandshakeStatus::kFailed;
}

}  // namespace trpc
