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

#include "trpc/runtime/iomodel/reactor/common/unix_address.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

namespace trpc {

UnixAddress::UnixAddress() {
  addr_.sun_family = AF_UNIX;
  static std::string sun_path = "trpc_unix_domain_" + std::to_string(getpid()) + ".socket";
  strncpy(addr_.sun_path, sun_path.c_str(), sun_path.size());
}

UnixAddress::UnixAddress(const struct sockaddr_un* addr) { addr_ = *addr; }

UnixAddress::UnixAddress(const char* path) {
  addr_.sun_family = AF_UNIX;
  if (strlen(path) >= sizeof(addr_.sun_path) || strlen(path) == 0) {
    snprintf(addr_.sun_path, sizeof(addr_.sun_path), "trpc_unix_domain_%d.socket", getpid());
    return;
  }
  snprintf(addr_.sun_path, sizeof(addr_.sun_path), "%s", path);
}

}  // namespace trpc
