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

#include <string>

#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

/// @brief The implementation for client io handler
class ClientIoHandler : public IoHandler {
 public:
  ClientIoHandler(Connection* conn, TransInfo* trans_info);

  ~ClientIoHandler() override;

  Connection* GetConnection() const override { return conn_; }

  IoHandler::HandshakeStatus Handshake(bool is_read_event) override;

  int Read(void* buff, uint32_t length) override;

  int Writev(const iovec* iov, int iovcnt) override;

 private:
  Connection* conn_{nullptr};

  IoHandler* auth_handler_{nullptr};
};

}  // namespace trpc
