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

#include "examples/features/future_forward/proxy/forward_service.h"

#include <memory>
#include <string>
#include <utility>

#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/future/future_utility.h"
#include "trpc/log/trpc_log.h"

namespace examples::forward {

ForwardServiceImpl::ForwardServiceImpl() {
  greeter_proxy_ =
      ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("trpc.test.helloworld.Greeter");
}

::trpc::Status ForwardServiceImpl::Route(::trpc::ServerContextPtr context,
                                         const ::trpc::test::helloworld::HelloRequest* request,
                                         ::trpc::test::helloworld::HelloReply* reply) {
  TRPC_FMT_INFO("Forward request:{}, req id:{}", request->msg(), context->GetRequestId());

  // use asynchronous response mode
  context->SetResponse(false);

  auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);

  ::trpc::test::helloworld::HelloRequest route_request;
  route_request.set_msg(request->msg());

  ::trpc::test::helloworld::HelloReply route_reply;

  greeter_proxy_->AsyncSayHello(client_context, route_request)
      .Then([context](::trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) {
        ::trpc::Status status;
        ::trpc::test::helloworld::HelloReply reply;

        if (fut.IsReady()) {
          std::string msg = fut.GetValue0().msg();
          reply.set_msg(msg);
          TRPC_FMT_INFO("Invoke success, route_reply:{}", msg);
        } else {
          auto exception = fut.GetException();
          status.SetErrorMessage(exception.what());
          status.SetFrameworkRetCode(exception.GetExceptionCode());
          TRPC_FMT_ERROR("Invoke failed, reason:{}", exception.what());
          reply.set_msg(exception.what());
        }

        context->SendUnaryResponse(status, reply);
        return ::trpc::MakeReadyFuture<>();
      });

  return ::trpc::kSuccStatus;
}

::trpc::Status ForwardServiceImpl::ParallelRoute(::trpc::ServerContextPtr context,
                                                 const ::trpc::test::helloworld::HelloRequest* request,
                                                 ::trpc::test::helloworld::HelloReply* reply) {
  TRPC_FMT_INFO("Forward request:{}, req id:{}", request->msg(), context->GetRequestId());

  // use asynchronous response mode
  context->SetResponse(false);

  // send two requests in parallel to helloworldserver

  int exec_count = 2;
  std::vector<::trpc::Future<::trpc::test::helloworld::HelloReply>> results;
  results.reserve(exec_count);
  for (int i = 0; i < exec_count; i++) {
    std::string msg = request->msg();
    msg += ", index";
    msg += std::to_string(i);

    trpc::test::helloworld::HelloRequest request;
    request.set_msg(msg);

    auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);
    results.emplace_back(greeter_proxy_->AsyncSayHello(client_context, request));
  }

  ::trpc::WhenAll(results.begin(), results.end())
      .Then([context](std::vector<trpc::Future<trpc::test::helloworld::HelloReply>>&& futs) {
        trpc::Status status;
        trpc::test::helloworld::HelloReply reply;
        std::string rsp = "parallel result:";
        int i = 0;
        for (auto&& fut : futs) {
          if (fut.IsReady()) {
            auto msg = fut.GetValue0().msg();
            rsp += " " + msg;
            TRPC_FMT_INFO("Invoke success, forward i: {}, route_reply:{}", i, msg);
          } else {
            auto exception = fut.GetException();
            status.SetErrorMessage(exception.what());
            status.SetFrameworkRetCode(exception.GetExceptionCode());
            TRPC_FMT_ERROR("Invoke failed, forward i: {}, reason:{}", i, exception.what());
            break;
          }

          i++;
        }

        if (status.OK()) {
          reply.set_msg(rsp);
        }

        context->SendUnaryResponse(status, reply);
        return trpc::MakeReadyFuture<>();
      });

  return trpc::kSuccStatus;
}

}  // namespace examples::forward
