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

#ifdef TRPC_BUILD_INCLUDE_SSL

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/transport/common/ssl/ssl.h"

namespace trpc::ssl {

/// @brief Io handler for ssl
class SslIoHandler : public IoHandler {
 public:
  SslIoHandler(Connection* conn, SslPtr&& ssl) : conn_(conn), ssl_(std::move(ssl)), handshaked_(false) {}
  ~SslIoHandler() override;

  Connection* GetConnection() const override { return conn_; }

  IoHandler::HandshakeStatus Handshake(bool is_read_event) override;

  int Read(void* buff, uint32_t length) override;

  int Writev(const struct iovec* iov, int iovcnt) override;

  void Destroy() override;

 private:
  Connection* conn_{nullptr};
  SslPtr ssl_{nullptr};
  bool handshaked_{false};
};

}  // namespace trpc::ssl

#endif
