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

#include "trpc/client/grpc/testing/grpc_client_call_testing.h"

#include <string>
#include <vector>

#include "trpc/client/make_client_context.h"

namespace trpc::testing {
namespace {
bool SayHello(const GreeterServiceProxyPtr& proxy, std::vector<std::string>&& request_msgs) {
  std::vector<std::string> msgs = std::move(request_msgs);
  for (const auto& msg : msgs) {
    auto ctx = MakeClientContext(proxy);
    ctx->SetTimeout(3000);
    trpc::test::helloworld::HelloRequest request;
    request.set_msg(msg);
    trpc::test::helloworld::HelloReply reply;
    auto status = proxy->SayHello(ctx, request, &reply);
    if (!status.OK()) {
      std::cerr << "bad status: " << status.ToString() << std::endl;
      return false;
    }
    if (reply.msg() != request.msg()) {
      std::cerr << "failed to check response" << std::endl;
      return false;
    }
    //    std::cout << "response: " << reply.msg() << std::endl;
  }
  return true;
}

bool SayHelloManyTimes(const GreeterServiceProxyPtr& proxy) {
  std::vector<std::string> msgs = {"gRPC", "930", "Cpp", "gRPC", "930", "Cpp"};
  return SayHello(proxy, std::move(msgs));
}
}  // namespace

bool TestGrpcClientUnaryRpcCall(const GreeterServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= SayHelloManyTimes(proxy);
  return ok;
}

}  // namespace trpc::testing
