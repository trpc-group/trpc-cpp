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

#include <arpa/inet.h>
#include <sys/un.h>

namespace trpc {

/// @brief Encapsulation for unix address family
class UnixAddress {
 public:
  UnixAddress();

  explicit UnixAddress(const struct sockaddr_un* addr);

  explicit UnixAddress(const char* path);

  const struct sockaddr* SockAddr() const {
    return reinterpret_cast<const struct sockaddr *>(&addr_);
  }

  socklen_t Socklen() const { return sizeof(addr_); }

  const char* Path() const { return addr_.sun_path; }

 private:
  struct sockaddr_un addr_;
};

}  // namespace trpc
