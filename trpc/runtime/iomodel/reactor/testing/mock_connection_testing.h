
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

#include "trpc/runtime/iomodel/reactor/common/connection.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::testing {

class MockConnection : public Connection {
 public:
  MOCK_METHOD0(Initialize, bool());
  MOCK_METHOD0(StartHandshaking, void());
  MOCK_METHOD0(Established, void());
  MOCK_METHOD0(DoConnect, bool());
  MOCK_METHOD1(DoClose, void(bool));

  int Send(IoMessage&& msg) override { return 0; };

 protected:
  MOCK_METHOD0(HandleReadEvent, int());
  MOCK_METHOD0(HandleWriteEvent, int());
  MOCK_METHOD0(HandleCloseEvent, void());
};

}  // namespace trpc::testing
