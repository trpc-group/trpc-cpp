
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

#include "examples/unittest/client/greeter_client.h"

#include "trpc/client/make_client_context.h"
#include "trpc/log/trpc_log.h"

namespace test {
namespace unittest {

bool GreeterClient::SayHello() {
  auto client_context = ::trpc::MakeClientContext(proxy_);
  ::trpc::test::unittest::HelloRequest req;
  ::trpc::test::unittest::HelloReply rep;
  req.set_msg("trpc");
  auto status = proxy_->SayHello(client_context, req, &rep);
  if (status.OK()) {
    TRPC_FMT_DEBUG("Invoke success, response msg = {}", rep.msg());
    // do something when success
    // ...
    return true;
  }

  TRPC_FMT_ERROR("Invoke fail, error = {}", status.ErrorMessage());
  // do something when fail
  // ...
  return false;
}

::trpc::Future<> GreeterClient::AsyncSayHello() {
  auto client_context = MakeClientContext(proxy_);
  ::trpc::test::unittest::HelloRequest req;
  req.set_msg("trpc");
  return proxy_->AsyncSayHello(client_context, req).Then([](::trpc::Future<::trpc::test::unittest::HelloReply>&& fut) {
    if (fut.IsReady()) {
      auto reply = fut.GetValue0();
      TRPC_FMT_DEBUG("Async invoke success, response msg = {}", reply.msg());
      // do something when success
      // ...
      return ::trpc::MakeReadyFuture<>();
    }

    auto ex = fut.GetException();
    TRPC_FMT_ERROR("Async invoke fail, error = {}", ex.what());
    // do something when fail
    // ...
    return ::trpc::MakeExceptionFuture<>(ex);
  });
}

}  // namespace unittest
}  // namespace test
