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

#include "trpc/server/server_context.h"
#include "trpc/transport/server/server_transport.h"

namespace trpc::testing {

/// @brief Mock server transport that used to mock network response.
/// @note Only used for test
class MockServerTransport : public ::trpc::ServerTransport {
 public:
  std::string Name() const override { return ""; }

  std::string Version() const override { return ""; }

  void Bind(const ::trpc::BindInfo& bind_info) override {}

  bool Listen() override { return false; }

  void Stop() override {}

  void Destroy() override {}

  MOCK_METHOD(int, SendMsg, (ServerContextPtr& context, NoncontiguousBuffer&& buffer));

  int SendMsg(STransportRspMsg* msg) override {
    auto server_context = msg->context;
    int ret = SendMsg(server_context, std::move(msg->buffer));
    auto callback = server_context->GetSendMsgCallback();
    if (callback != nullptr) {
      callback();
    }

    return ret;
  }
};

}  // namespace trpc::testing