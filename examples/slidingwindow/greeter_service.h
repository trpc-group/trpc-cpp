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

#include "examples/slidingwindow/slidingwindow.trpc.pb.h"

namespace test {
namespace slidingwindow {

class GreeterServiceImpl : public ::trpc::test::slidingwindow::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::slidingwindow::HelloRequest* request,
                          ::trpc::test::slidingwindow::HelloReply* reply) override;
};

}  // namespace slidingwindow
}  // namespace test
