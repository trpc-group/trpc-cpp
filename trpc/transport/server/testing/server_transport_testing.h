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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/transport/server/server_transport.h"

namespace trpc::testing {

class TestServerTransport : public ServerTransport {
 public:
  ~TestServerTransport() override {
    if (rsp_msg_) {
      object_pool::Delete<STransportRspMsg>(rsp_msg_);
    }
  }

  std::string Name() const override { return "test_server_transport"; }

  std::string Version() const  override { return "test_0.1"; }

  int Init(const std::any& params) override { return 0; }

  void Bind(const BindInfo& bind_info) override {}

  bool Listen()  override { return true; }

  void Stop() override {}

  void StopListen(bool clean_conn) override {}

  void Destroy() override {}

  int SendMsg(STransportRspMsg* msg) override {
    if (rsp_msg_) {
      object_pool::Delete<STransportRspMsg>(rsp_msg_);
    }
    rsp_msg_ = msg;
    return 0;
  }

  void DoClose(const CloseConnectionInfo& close_connection_info) override {}

  bool IsConnected(uint64_t connection_id) override { return true; }

  void ThrottleConnection(uint64_t conn_id, bool set) override {}

  STransportRspMsg* GetRspMsg() const { return rsp_msg_; }

 private:
  STransportRspMsg* rsp_msg_{nullptr};
};

}  // namespace trpc::testing
