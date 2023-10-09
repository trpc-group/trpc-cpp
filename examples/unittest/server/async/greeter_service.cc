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

#include "examples/unittest/server/async/greeter_service.h"

#include <string>
#include <thread>

#include "trpc/common/status.h"
#include "trpc/log/trpc_log.h"

namespace test {
namespace unittest {

::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::unittest::HelloRequest* request,
                                            ::trpc::test::unittest::HelloReply* reply) {
  context->SetResponse(false);
  // do some async operation, for example use std::thread to start a async operation
  auto t = std::thread([context, request, reply]() {
    std::string response = "Hello, " + request->msg();
    reply->set_msg(response);

    context->SendUnaryResponse(::trpc::kSuccStatus, *reply);
  });
  t.detach();

  return ::trpc::kSuccStatus;
}

}  // namespace unittest
}  // namespace test
