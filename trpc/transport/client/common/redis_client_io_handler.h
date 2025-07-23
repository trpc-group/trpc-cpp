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

#include <memory>
#include <string>

#include "trpc/common/config/redis_client_conf.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

enum class RedisClientStage {
  kInit = 0,             // Initial stage (waiting for the first request to trigger write event)
  kSendAuthSucc = 1,     // Authentication request packet sent successfully
  kSendAuthPart = 2,     // Authentication request packet partially sent successfully
  kSendAuthFail = 3,     // Authentication request packet failed to send
  kRecvAuthSucc = 4,     // Authentication response packet receive successfully
  kRecvAuthPart = 5,     // Authentication response packet partially receive successfully
  kRecvAuthFail = 6,     // Authentication response packet failed to receive
  kSendSelectSucc = 7,   // Select-DB request packet sent successfully
  kSendSelectPart = 8,   // Select-DB request packet partially sent successfully
  kSendSelectFail = 9,   // Select-DB request packet failed to send
  kRecvSelectSucc = 10,  // Select-DB response packet receive successfully
  kRecvSelectPart = 11,  // Select-DB response packet partially receive successfully
  kRecvSelectFail = 12,  // Select-DB response packet failed to receive
};

/// @brief The implementation for redis client io handler
class RedisClientIoHandler : public IoHandler {
 public:
  RedisClientIoHandler(Connection* conn, TransInfo* trans_info);

  ~RedisClientIoHandler() override;

  Connection* GetConnection() const override { return conn_; }

  HandshakeStatus Handshake(bool is_read_event) override;

  int Read(void* buffer, uint32_t size) override {
    TRPC_ASSERT(conn_ != nullptr && "conn_ cannot be nullptr");
    TRPC_ASSERT(buffer != nullptr && "buffer cannot be nullptr");

    return ::recv(conn_->GetFd(), buffer, size, 0);
  }

  int Writev(const iovec* iov, int iovcnt) override {
    TRPC_ASSERT(conn_ != nullptr && "conn_ cannot be nullptr");
    TRPC_ASSERT(iov != nullptr && "iov cannot be nullptr");

    return ::writev(conn_->GetFd(), iov, iovcnt);
  };

  using IoHandler::Read;

 private:
  IoHandler::HandshakeStatus HandleRedisHandshake(bool is_read_event);

  int UnPackageAuthResponse(char* recv_buff, const int recv_len);

  int UnPackageSelectResponse(char* recv_buff, const int recv_len);

  IoHandler::HandshakeStatus SendAuthRequest();

  IoHandler::HandshakeStatus SendSelectRequest();

  IoHandler::HandshakeStatus HandleSelectResponse(bool is_read_event);

  IoHandler::HandshakeStatus HandleAuthResponse(bool is_read_event);

  IoHandler::HandshakeStatus SendRequest(const std::string& cmd);

  int RecvResponse(char* recv_buff);

  int Reset();

 private:
  Connection* conn_{nullptr};

  std::string auth_cmd_;

  std::string select_cmd_;

  RedisClientStage current_stage_;

  RedisClientStage init_stage_;

  int pos_{0};

  uint32_t db_index_{0};

  RedisClientConf redis_conf_;
};
}  // namespace trpc
