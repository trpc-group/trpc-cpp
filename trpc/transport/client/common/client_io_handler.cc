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

#include "trpc/transport/client/common/client_io_handler.h"

namespace trpc {

ClientIoHandler::ClientIoHandler(Connection* conn, TransInfo* trans_info) : conn_(conn) {
  if (trans_info->custom_io_handler_function != nullptr) {
    this->auth_handler_ = trans_info->custom_io_handler_function(this);
  }
}

ClientIoHandler::~ClientIoHandler() {
  if (auth_handler_ != nullptr) {
    delete auth_handler_;
    auth_handler_ = nullptr;
  }
}

IoHandler::HandshakeStatus ClientIoHandler::Handshake(bool is_read_event) {
  if (auth_handler_ == nullptr) {
    return IoHandler::HandshakeStatus::kSucc;
  }

  IoHandler::HandshakeStatus ret = IoHandler::HandshakeStatus::kFailed;
  if (is_read_event) {
    do {
      ret = this->auth_handler_->Handshake(is_read_event);
    } while (ret != IoHandler::HandshakeStatus::kSucc && ret != IoHandler::HandshakeStatus::kFailed &&
             ret != IoHandler::HandshakeStatus::kNeedRead);

  } else {
    ret = this->auth_handler_->Handshake(is_read_event);
  }
  return ret;
}

int ClientIoHandler::Read(void* buff, uint32_t length) {
  return ::recv(conn_->GetFd(), buff, length, 0);
}

int ClientIoHandler::Writev(const iovec* iov, int iovcnt) {
  return ::writev(conn_->GetFd(), iov, iovcnt);
}

}  // namespace trpc
