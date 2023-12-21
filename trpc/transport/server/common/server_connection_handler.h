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

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/transport/server/server_transport_def.h"

namespace trpc {

constexpr int kStreamOk{0};
constexpr int kStreamError{-1};
constexpr int kStreamNonStreamMsg{-2};

class ServerTransport;

template <class T>
class ServerConnectionHandler : public ConnectionHandler {
 public:
  ServerConnectionHandler(Connection* conn, T* bind_adapter, BindInfo* bind_info)
      : connection_(conn), bind_adapter_(bind_adapter), bind_info_(bind_info) {}

  T* GetBindAdapter() const { return bind_adapter_; }

  BindInfo* GetBindInfo() const { return bind_info_; }

  ServerTransport* GetTransport() const { return bind_adapter_->GetTransport(); }

  Connection* GetConnection() const override { return connection_; }

  int CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    if (TRPC_LIKELY(bind_info_->checker_function)) {
      return bind_info_->checker_function(conn, in, out);
    }

    return -1;
  }

  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override {
    if (TRPC_LIKELY(bind_info_->msg_handle_function)) {
      return bind_info_->msg_handle_function(conn, msg);
    }

    return false;
  }

  void CleanResource() override { bind_adapter_->CleanConnectionResource(connection_); }

  void UpdateConnection() override { bind_adapter_->UpdateConnection(connection_); }

  void ConnectionEstablished() override {
    if (TRPC_UNLIKELY(bind_info_->conn_establish_function)) {
      bind_info_->conn_establish_function(connection_);
    }
  }

  void ConnectionClosed() override {
    if (TRPC_UNLIKELY(bind_info_->conn_close_function)) {
      bind_info_->conn_close_function(connection_);
    }
  }

  void MessageWriteDone(const IoMessage& message) override {
    TRPC_FMT_DEBUG("MessageWriteDone request_id: {}", message.seq_id);
    if (bind_info_->run_io_filters_function) {
      bind_info_->run_io_filters_function(FilterPoint::SERVER_POST_IO_SEND_MSG, message.msg);
    }
  }

 private:
  Connection* connection_{nullptr};
  T* bind_adapter_{nullptr};
  BindInfo* bind_info_{nullptr};
};

}  // namespace trpc
