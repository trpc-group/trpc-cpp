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

#include "gtest/gtest.h"

#include "trpc/common/status.h"
#include "trpc/server/make_server_context.h"
#include "trpc/server/trpc_server.h"

#include "examples/helloworld/greeter_service.h"
#include "examples/helloworld/helloworld.trpc.pb.h"

namespace test {
namespace helloworld {
namespace testing {

::trpc::ServerContextPtr MakeServerContext() {
  ::trpc::ServerContextPtr ctx = ::trpc::MakeServerContext();
  // ...
  return ctx;
}

TEST(GreeterServiceTest, SayHelloOK) {
  auto server_context = MakeServerContext();

  ::trpc::test::helloworld::HelloRequest request;
  request.set_msg("tRPC");
  ::trpc::test::helloworld::HelloReply response;

  test::helloworld::GreeterServiceImpl svc_impl;
  ::trpc::Status status = svc_impl.SayHello(server_context, &request, &response);

  ASSERT_TRUE(status.OK());
  ASSERT_EQ(response.msg(), "Hello, tRPC");
}

}  // namespace testing
}  // namespace helloworld
}  // namespace test
