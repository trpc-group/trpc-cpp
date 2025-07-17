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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "test/end2end/unary/rpcz/rpcz_server.h"

#include <chrono>
#include <thread>

#include "test/end2end/common/util.h"
#include "test/end2end/unary/rpcz/real_server.trpc.pb.h"
#include "test/end2end/unary/rpcz/rpcz.trpc.pb.h"
#include "trpc/rpcz/trpc_rpcz.h"
#include "trpc/runtime/runtime.h"

namespace {

constexpr int kErrorCodeFuture = -5;

// Have to reduce duplicate code.
#define CALL_RPC_FUNC(client_ctx, fiber_func, future_func) \
  trpc::testing::rpcz::RealServerRequest rs_request; \
  if (IsFiberEnv()) { \
    trpc::testing::rpcz::RealServerReply rs_response; \
    trpc::Status status = fiber_func(client_ctx, rs_request, &rs_response); \
    return status; \
  } \
  context->SetResponse(false); \
  future_func(client_ctx, rs_request) \
    .Then([context](trpc::Future<trpc::testing::rpcz::RealServerReply>&& fut) mutable { \
      trpc::Status status; \
      trpc::testing::rpcz::RpczReply reply; \
      if (fut.IsFailed()) { \
        status.SetFuncRetCode(kErrorCodeFuture); \
      } \
      context->SendUnaryResponse(status, reply); \
    }); \
  return trpc::kSuccStatus

/// @brief To help decide the way to request rs.
bool IsFiberEnv() {
  return ::trpc::runtime::GetRuntimeType() == ::trpc::runtime::kFiberRuntime;
}

}  // namespace

namespace trpc::testing {

class RpczServiceImpl final : public trpc::testing::rpcz::Rpcz {
 public:
  RpczServiceImpl() {
    proxy_ = trpc::GetTrpcClient()->GetProxy<::trpc::testing::rpcz::RealServerServiceProxy>("real_service");
  }

  trpc::Status ClientGetRpczOk(trpc::ServerContextPtr context,
                               const trpc::testing::rpcz::RpczRequest* request,
                               trpc::testing::rpcz::RpczReply* response) override {
    // To trigger client rpcz routine.
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyClientGetRpczOk, proxy_->AsyncProxyClientGetRpczOk);
  }

  trpc::Status ClientGetSpanIdOk(trpc::ServerContextPtr context,
                                 const trpc::testing::rpcz::RpczRequest* request,
                                 trpc::testing::rpcz::RpczReply* response) override {
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyClientGetSpanIdOk, proxy_->AsyncProxyClientGetSpanIdOk);
  }

  trpc::Status ClientProbableSample(trpc::ServerContextPtr context,
                                    const trpc::testing::rpcz::RpczRequest* request,
                                    trpc::testing::rpcz::RpczReply* response) override {
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyClientProbableSample, proxy_->AsyncProxyClientProbableSample);
  }

  trpc::Status ClientSpanExpired(trpc::ServerContextPtr context,
                                 const trpc::testing::rpcz::RpczRequest* request,
                                 trpc::testing::rpcz::RpczReply* response) override {
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyClientSpanExpired, proxy_->AsyncProxyClientSpanExpired);
  }

  trpc::Status ChangeClientSampleFunction(trpc::ServerContextPtr context,
                                          const trpc::testing::rpcz::RpczRequest* request,
                                          trpc::testing::rpcz::RpczReply* response) override {
    // Always return false;
    trpc::rpcz::SetClientSampleFunction([] (const trpc::ClientContextPtr& context) {
      return false;
    });
    return trpc::kSuccStatus;
  }

  trpc::Status RestoreClientSampleFunction(trpc::ServerContextPtr context,
                                           const trpc::testing::rpcz::RpczRequest* request,
                                           trpc::testing::rpcz::RpczReply* response) override {
    trpc::rpcz::DelClientSampleFunction();
    return trpc::kSuccStatus;
  }

  trpc::Status FireClientSampleFunction(trpc::ServerContextPtr context,
                                        const trpc::testing::rpcz::RpczRequest* request,
                                        trpc::testing::rpcz::RpczReply* response) override {
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyFireClientSampleFunction, proxy_->AsyncProxyFireClientSampleFunction);
  }

  trpc::Status ServerGetRpczOk(trpc::ServerContextPtr context,
                               const trpc::testing::rpcz::RpczRequest* request,
                               trpc::testing::rpcz::RpczReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ServerGetSpanIdOk(trpc::ServerContextPtr context,
                                 const trpc::testing::rpcz::RpczRequest* request,
                                 trpc::testing::rpcz::RpczReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ServerProbableSample(trpc::ServerContextPtr context,
                                    const trpc::testing::rpcz::RpczRequest* request,
                                    trpc::testing::rpcz::RpczReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ServerSpanExpired(trpc::ServerContextPtr context,
                                 const trpc::testing::rpcz::RpczRequest* request,
                                 trpc::testing::rpcz::RpczReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ChangeServerSampleFunction(trpc::ServerContextPtr context,
                                          const trpc::testing::rpcz::RpczRequest* request,
                                          trpc::testing::rpcz::RpczReply* response) override {
    // Always return false;
    trpc::rpcz::SetServerSampleFunction([] (const trpc::ServerContextPtr& context) {
      return false;
    });
    return trpc::kSuccStatus;
  }

  trpc::Status RestoreServerSampleFunction(trpc::ServerContextPtr context,
                                           const trpc::testing::rpcz::RpczRequest* request,
                                           trpc::testing::rpcz::RpczReply* response) override {
    trpc::rpcz::DelServerSampleFunction();
    return trpc::kSuccStatus;
  }

  trpc::Status FireServerSampleFunction(trpc::ServerContextPtr context,
                                        const trpc::testing::rpcz::RpczRequest* request,
                                        trpc::testing::rpcz::RpczReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ServerRpczPrint(trpc::ServerContextPtr context,
                               const trpc::testing::rpcz::RpczRequest* request,
                               trpc::testing::rpcz::RpczReply* response) override {
    TRPC_RPCZ_PRINT(context, "USER RPCZ PRINT");
    return trpc::kSuccStatus;
  }

  trpc::Status RouteGetRpczOk(trpc::ServerContextPtr context,
                              const trpc::testing::rpcz::RpczRequest* request,
                              trpc::testing::rpcz::RpczReply* response) override {
    // To trigger route rpcz routine.
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(context, proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyRouteGetRpczOk, proxy_->AsyncProxyRouteGetRpczOk);
  }

  trpc::Status RouteGetSpanIdOk(trpc::ServerContextPtr context,
                                const trpc::testing::rpcz::RpczRequest* request,
                                trpc::testing::rpcz::RpczReply* response) override {
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(context, proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyRouteGetSpanIdOk, proxy_->AsyncProxyRouteGetSpanIdOk);
  }

  trpc::Status RouteProbableSample(trpc::ServerContextPtr context,
                                  const trpc::testing::rpcz::RpczRequest* request,
                                  trpc::testing::rpcz::RpczReply* response) override {
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(context, proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyRouteProbableSample, proxy_->AsyncProxyRouteProbableSample);
  }

  trpc::Status RouteSpanExpired(trpc::ServerContextPtr context,
                                const trpc::testing::rpcz::RpczRequest* request,
                                trpc::testing::rpcz::RpczReply* response) override {
    trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(context, proxy_);
    CALL_RPC_FUNC(client_ctx, proxy_->ProxyRouteSpanExpired, proxy_->AsyncProxyRouteSpanExpired);
  }

  trpc::Status UserGetSpanIdOk(trpc::ServerContextPtr context,
                               const trpc::testing::rpcz::RpczRequest* request,
                               trpc::testing::rpcz::RpczReply* response) override {
    // Create user span by user.
    auto* root_span_ptr = trpc::rpcz::CreateUserRpczSpan("root_span");
    // Manipulate root span by adding attributes.
    root_span_ptr->AddAttribute("name", "root");
    root_span_ptr->AddAttribute("city", "shenzhen");

    // Do some user things here.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Create sub span of root span.
    auto* son_span_a_ptr = root_span_ptr->CreateSubSpan("son_span_a");
    // Manipulate sub span by adding attributes.
    son_span_a_ptr->AddAttribute("name", "son_a");

    // Do some user things here.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Create sub span of sub span.
    auto* grandson_span_a_ptr = son_span_a_ptr->CreateSubSpan("grandson_span_a");
    // Manipulate grandson span by adding attributes.
    grandson_span_a_ptr->AddAttribute("name", "grandson_a");
    // Do some user things here.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // Must end the span by user.
    grandson_span_a_ptr->End();

    // Must end the span by user.
    son_span_a_ptr->End();

    // Create another sub span of root span.
    auto* son_span_b_ptr = root_span_ptr->CreateSubSpan("son_span_b");
    // Manipulate sub span by adding attributes.
    son_span_b_ptr->AddAttribute("name", "son_b");
    // Do some user things here.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // Must end the span by user.
    son_span_b_ptr->End();

    // Do some user things here.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    // Must submit root span to framework, in order to see it through admin service.
    trpc::rpcz::SubmitUserRpczSpan(root_span_ptr);

    return trpc::kSuccStatus;
  }

 private:
  std::shared_ptr<trpc::testing::rpcz::RealServerServiceProxy> proxy_;
};

int RpczServer::Initialize() {
  RegisterService("rpcz_service", std::make_shared<RpczServiceImpl>());
  server_->Start();
  test_signal_->SignalClientToContinue();
  return 0;
}

void RpczServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
#endif
