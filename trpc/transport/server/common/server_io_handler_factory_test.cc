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

#include "trpc/transport/server/common/server_io_handler_factory.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"

namespace trpc::testing {

class FakeIoHandler : public DefaultIoHandler {
 public:
  explicit FakeIoHandler(Connection* conn) : DefaultIoHandler(conn) {}
};

TEST(ServerIoHandlerFactory, All) {
  ASSERT_TRUE(ServerIoHandlerFactory::GetInstance()->Register(
      "fake", [](Connection* conn, const BindInfo& bind_info) { return std::make_unique<FakeIoHandler>(conn); }));
  ASSERT_FALSE(ServerIoHandlerFactory::GetInstance()->Register(
      "fake", [](Connection* conn, const BindInfo& bind_info) { return std::make_unique<FakeIoHandler>(conn); }));

  Connection conn;
  BindInfo bind_info;
  std::unique_ptr<IoHandler> fake_io_handler =
      ServerIoHandlerFactory::GetInstance()->Create("fake", &conn, bind_info);
  ASSERT_EQ(typeid(*fake_io_handler), typeid(FakeIoHandler));

  std::unique_ptr<IoHandler> default_io_handler =
      ServerIoHandlerFactory::GetInstance()->Create("default",  &conn, bind_info);
  ASSERT_EQ(typeid(*default_io_handler), typeid(DefaultIoHandler));

  ServerIoHandlerFactory::GetInstance()->Clear();

  fake_io_handler = ServerIoHandlerFactory::GetInstance()->Create("fake",  &conn, bind_info);

  ASSERT_NE(typeid(*fake_io_handler), typeid(FakeIoHandler));
  ASSERT_EQ(typeid(*fake_io_handler), typeid(DefaultIoHandler));
}

}  // namespace trpc::testing
