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

#include "examples/unittest/helloworld.trpc.pb.h"

namespace test {
namespace unittest {

class GreeterServiceImpl : public ::trpc::test::unittest::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::unittest::HelloRequest* request,
                          ::trpc::test::unittest::HelloReply* reply) override;
};

}  // namespace unittest
}  // namespace test
