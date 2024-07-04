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
  request.set_name("issueshooter");
  request.set_age(18);
  request.add_hobby("opensource project"); 
  request.add_hobby("movies");
  request.add_hobby("books"); 

  ::trpc::test::helloworld::HelloReply response;

  test::helloworld::GreeterServiceImpl svc_impl;
  ::trpc::Status status = svc_impl.SayHello(server_context, &request, &response);

  ASSERT_TRUE(status.OK());
  ASSERT_EQ(response.name(), "hello issueshooter");
  ASSERT_EQ(response.age(), 19); 
  ASSERT_EQ(response.hobby_size(), 3); 
  ASSERT_EQ(response.hobby(0), "opensource project");
  ASSERT_EQ(response.hobby(1), "movies");
  ASSERT_EQ(response.hobby(2), "books");
}

}  // namespace testing
}  // namespace helloworld
}  // namespace test
