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

#include "gmock/gmock.h"

#include "trpc/stream/stream_handler.h"
#include "trpc/transport/client/client_transport.h"

namespace trpc::testing {

class MockClientTransport : public ClientTransport {
 public:
  std::string Name() const override { return "mock_client_transport"; }

  std::string Version() const override { return "0.1"; }

  MOCK_METHOD(int, SendRecv, (CTransportReqMsg*, CTransportRspMsg*), (override));

  MOCK_METHOD(Future<CTransportRspMsg>, AsyncSendRecv, (CTransportReqMsg*), (override));

  MOCK_METHOD(int, SendOnly, (CTransportReqMsg*), (override));

  MOCK_METHOD(stream::StreamReaderWriterProviderPtr, CreateStream, (const NodeAddr&, stream::StreamOptions&&),
              (override));

  MOCK_METHOD(std::optional<uint64_t>, GetOrCreateConnector, (const PreallocationOption&), (override));

  MOCK_METHOD(bool, ReleaseFixedConnector, (uint64_t), (override));

  MOCK_METHOD(void, Disconnect, (const std::string&), (override));
};

}  // namespace trpc::testing
