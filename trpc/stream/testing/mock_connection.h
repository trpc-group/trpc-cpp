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

#include "trpc/runtime/iomodel/reactor/common/connection.h"

namespace trpc::testing {

class MockTcpConnection : public Connection {
 public:
  MOCK_METHOD(int, HandleReadEvent, (), (override));
  MOCK_METHOD(int, HandleWriteEvent, (), (override));
  MOCK_METHOD(void, HandleCloseEvent, (), (override));
  MOCK_METHOD(void, Established, (), (override));
  MOCK_METHOD(int, Send, (IoMessage &&), (override));
};

class MockConnectionHandler : public ConnectionHandler {
 public:
  MOCK_METHOD(Connection*, GetConnection, (), (const, override));
  MOCK_METHOD(int, CheckMessage, (const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&), (override));
  MOCK_METHOD(bool, HandleMessage, (const ConnectionPtr&, std::deque<std::any>&), (override));
  MOCK_METHOD(int, DoHandshake, (), (override));
};

}  // namespace trpc::testing
