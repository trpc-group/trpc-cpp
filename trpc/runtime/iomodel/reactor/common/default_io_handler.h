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

#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief Default implementation for io send and receive on the connection
class DefaultIoHandler : public IoHandler {
 public:
  explicit DefaultIoHandler(Connection* conn)
      : conn_(conn) {
    TRPC_ASSERT(conn);
    fd_ = conn_->GetFd();
  }

  HandshakeStatus Handshake(bool is_read_event) override { return HandshakeStatus::kSucc; }

  int Read(void* buff, uint32_t length) override { return ::recv(fd_, buff, length, 0); }

  int Writev(const iovec* iov, int iovcnt) override {
    int ret = ::writev(fd_, iov, iovcnt);
#ifdef TRPC_DISABLE_TCP_CORK
    detail::FlushTcpCorkedData(fd_);
#endif
    return ret;
  }

  Connection* GetConnection() const override { return conn_; }

 private:
  Connection* conn_;
  int fd_;
};

}  // namespace trpc
