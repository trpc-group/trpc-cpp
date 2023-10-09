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

#include "trpc/stream/grpc/util.h"

#include "limits.h"
#include "sys/uio.h"

#include "trpc/util/log/logging.h"

namespace trpc::stream::grpc {

int SendHandshakingMessage(IoHandler* io_handler, NoncontiguousBuffer&& buffer) {
  iovec iov[IOV_MAX];
  std::size_t nv = 0;
  int rc = 0;

  while (buffer.ByteSize() > 0) {
    nv = 0;
    for (auto iter = buffer.begin(); iter != buffer.end(); ++iter) {
      auto& e = iov[nv++];
      e.iov_base = iter->data();
      e.iov_len = iter->size();
    }
    rc = io_handler->Writev(iov, nv);
    if (rc > 0) {
      buffer.Skip(rc);
      continue;
    } else if (rc < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        continue;
      }
      TRPC_LOG_ERROR("Error write to conn, because errno: " << errno);
      return errno;
    } else {
      TRPC_ASSERT(false && "Impossible writing empty buffer");
    }
  }

  return 0;
}

}  // namespace trpc::stream::grpc
