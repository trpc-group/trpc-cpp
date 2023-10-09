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

#include <memory>

#include "examples/helloworld/helloworld.trpc.pb.h"
#include "examples/features/fiber_forward/proxy/forward.trpc.pb.h"

namespace examples::forward {

using GreeterProxyPtr = std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>;

class ForwardServiceImpl : public ::trpc::test::route::Forward {
 public:
  ForwardServiceImpl();

  ::trpc::Status Route(::trpc::ServerContextPtr context,
                       const ::trpc::test::helloworld::HelloRequest* request,
                       ::trpc::test::helloworld::HelloReply* reply) override;

  ::trpc::Status ParallelRoute(::trpc::ServerContextPtr context,
                               const ::trpc::test::helloworld::HelloRequest* request,
                               ::trpc::test::helloworld::HelloReply* reply) override;

 private:
  GreeterProxyPtr greeter_proxy_;
};

}  // namespace examples::forward
