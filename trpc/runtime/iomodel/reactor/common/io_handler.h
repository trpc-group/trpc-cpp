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

#include <sys/socket.h>
#include <sys/uio.h>

#include <cstdint>

namespace trpc {

class Connection;

/// @brief Base class for io send and receive on the connection
class IoHandler {
 public:
  enum class HandshakeStatus {
    kSucc,
    kFailed,
    kNeedRead,
    kNeedWrite,
  };

  virtual ~IoHandler() = default;

  /// @brief Get the connection which this iohandler belongs to
  virtual Connection* GetConnection() const = 0;

  /// @brief Handshake at the network transport layer, eg: ssl
  virtual HandshakeStatus Handshake(bool is_read_event) = 0;

  /// @brief Read data from the connection
  virtual int Read(void* buffer, uint32_t size) = 0;

  /// @brief write data to the connection
  virtual int Writev(const iovec* iov, int iovcnt) = 0;

  /// @brief Destroy IO handler.
  virtual void Destroy() {}
};

}  // namespace trpc
