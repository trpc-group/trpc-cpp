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
#include "test/end2end/unary/rpcz/real_server.h"

#include "test/end2end/unary/rpcz/real_server.trpc.pb.h"

namespace trpc::testing {

/// @note We have to design as many rpc functions as needed, in order to let
///       rpcz client distinguish each test case from general rpcz info.
class RealServerServiceImpl final : public trpc::testing::rpcz::RealServer {
 public:
  trpc::Status ProxyClientGetRpczOk(trpc::ServerContextPtr context,
                                    const trpc::testing::rpcz::RealServerRequest* request,
                                    trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyClientGetSpanIdOk(trpc::ServerContextPtr context,
                                      const trpc::testing::rpcz::RealServerRequest* request,
                                      trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyClientProbableSample(trpc::ServerContextPtr context,
                                         const trpc::testing::rpcz::RealServerRequest* request,
                                         trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyClientSpanExpired(trpc::ServerContextPtr context,
                                      const trpc::testing::rpcz::RealServerRequest* request,
                                      trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyFireClientSampleFunction(trpc::ServerContextPtr context,
                                             const trpc::testing::rpcz::RealServerRequest* request,
                                             trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyRouteGetRpczOk(trpc::ServerContextPtr context,
                                   const trpc::testing::rpcz::RealServerRequest* request,
                                   trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyRouteGetSpanIdOk(trpc::ServerContextPtr context,
                                     const trpc::testing::rpcz::RealServerRequest* request,
                                     trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyRouteProbableSample(trpc::ServerContextPtr context,
                                        const trpc::testing::rpcz::RealServerRequest* request,
                                        trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status ProxyRouteSpanExpired(trpc::ServerContextPtr context,
                                     const trpc::testing::rpcz::RealServerRequest* request,
                                     trpc::testing::rpcz::RealServerReply* response) override {
    return trpc::kSuccStatus;
  }
};

int RealServer::Initialize() {
  RegisterService("echo_service", std::make_shared<RealServerServiceImpl>());
  server_->Start();
  test_signal_->SignalClientToContinue();
  return 0;
}

}  // namespace trpc::testing
#endif
