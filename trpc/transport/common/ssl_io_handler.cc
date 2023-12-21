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

#ifdef TRPC_BUILD_INCLUDE_SSL

#include "trpc/transport/common/ssl_io_handler.h"

#include "trpc/transport/common/ssl/core.h"
#include "trpc/transport/common/ssl/errno.h"
#include "trpc/util/log/logging.h"

namespace trpc::ssl {

SslIoHandler::~SslIoHandler() {
  ssl_ = nullptr;
  conn_ = nullptr;
}

IoHandler::HandshakeStatus SslIoHandler::Handshake(bool is_read_event) {
  if (handshaked_) {
    return HandshakeStatus::kSucc;
  }

  HandshakeStatus status = HandshakeStatus::kFailed;
  int n = ssl_->DoHandshake();
  if (kOk == n) {
    handshaked_ = true;
    status = HandshakeStatus::kSucc;
  } else {
    handshaked_ = false;
    switch (n) {
      case kError:
        status = HandshakeStatus::kFailed;
        break;
      case kWantRead:
        status = HandshakeStatus::kNeedRead;
        break;
      case kWantWrite:
        status = HandshakeStatus::kNeedWrite;
        break;
      default:
        status = HandshakeStatus::kFailed;
    }
  }

  return status;
}

int SslIoHandler::Read(void* buff, uint32_t length) {
  int n = ssl_->Recv(reinterpret_cast<char*>(buff), length);
  if (n > 0) {
    return n;
  } else if (n == kWantRead || n == kWantWrite) {
    errno = EAGAIN;
  }
  return n;
}

int SslIoHandler::Writev(const struct iovec* iov, int iovcnt) {
  int n = ssl_->Writev(iov, iovcnt);
  if (n > 0) {
    return n;
  } else if (n == kWantRead || n == kWantWrite) {
    errno = EAGAIN;
  }
  return n;
}

void SslIoHandler::Destroy() {
  if (ssl_) {
    ssl_->Shutdown();
    ssl_ = nullptr;
    handshaked_ = false;
  }
}

}  // namespace trpc::ssl

#endif
