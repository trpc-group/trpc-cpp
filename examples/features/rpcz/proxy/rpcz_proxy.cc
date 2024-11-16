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

#ifdef TRPC_BUILD_INCLUDE_RPCZ

#include "examples/features/rpcz/proxy/rpcz_proxy.h"

#include "trpc/rpcz/trpc_rpcz.h"

#include "examples/features/rpcz/proxy/rpcz.trpc.pb.h"
#include "examples/helloworld/helloworld.trpc.pb.h"

namespace trpc::examples {

constexpr int kErrorCodeFuture = -5;

class RpczServiceImpl final : public ::trpc::examples::rpcz::Rpcz {
 public:
  RpczServiceImpl() {
    proxy_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("real_service");
  }

  ::trpc::Status Normal(::trpc::ServerContextPtr context,
                        const ::trpc::test::helloworld::HelloRequest* request,
                        ::trpc::test::helloworld::HelloReply* response) override {
    TRPC_RPCZ_PRINT(context, "USER RPCZ PRINT Normal");
    // To trigger client rpcz routine.
    ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy_);
    ::trpc::test::helloworld::HelloRequest rs_request;
    context->SetResponse(false);
    proxy_->AsyncSayHello(client_ctx, rs_request)
      .Then([context](trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) mutable {
        ::trpc::Status status;
        ::trpc::test::helloworld::HelloReply proxy_response;
        if (fut.IsFailed()) {
          status.SetFuncRetCode(kErrorCodeFuture);
        }

        context->SendUnaryResponse(status, proxy_response);
      });
    return trpc::kSuccStatus;
  }

  ::trpc::Status Route(::trpc::ServerContextPtr context,
                       const ::trpc::test::helloworld::HelloRequest* request,
                       ::trpc::test::helloworld::HelloReply* response) override {
    TRPC_RPCZ_PRINT(context, "USER RPCZ PRINT Route");
    // To trigger route rpcz routine.
    ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(context, proxy_);
    ::trpc::test::helloworld::HelloRequest rs_request;
    context->SetResponse(false);
    proxy_->AsyncSayHello(client_ctx, rs_request)
      .Then([context](trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) mutable {
        ::trpc::Status status;
        ::trpc::test::helloworld::HelloReply proxy_response;
        if (fut.IsFailed()) {
          status.SetFuncRetCode(kErrorCodeFuture);
        }

        context->SendUnaryResponse(status, proxy_response);
      });
    return trpc::kSuccStatus;
  }

 private:
  std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy> proxy_;
};

int RpczServer::Initialize() {
  RegisterService("rpcz_service", std::make_shared<RpczServiceImpl>());
  return 0;
}

void RpczServer::Destroy() {}

}  // namespace trpc::examples

int main(int argc, char** argv) {
  ::trpc::examples::RpczServer rpcz_server;
  rpcz_server.Main(argc, argv);
  rpcz_server.Wait();

  return 0;
}

#else

int main(int argc, char** argv) {
  return 0;
}

#endif // TRPC_BUILD_INCLUDE_RPCZ
