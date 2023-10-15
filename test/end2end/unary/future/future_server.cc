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

#include "test/end2end/unary/future/future_server.h"

#include <chrono>
#include <thread>

#include "test/end2end/common/util.h"
#include "test/end2end/unary/future/future.trpc.pb.h"

namespace trpc::testing {

class FutureServiceImpl final : public trpc::testing::future::Future {
 public:
  trpc::Status SayHello(trpc::ServerContextPtr context,
                        const trpc::testing::future::TestRequest* request,
                        trpc::testing::future::TestReply* response) override {
    return trpc::kSuccStatus;
  }

  trpc::Status SayHelloTimeout(trpc::ServerContextPtr context,
                               const trpc::testing::future::TestRequest* request,
                               trpc::testing::future::TestReply* response) override {
    context->SetResponse(false);
    trpc::Promise<> pr;
    trpc::Future<> fut = pr.GetFuture();
    // Do NOT sleep in framework thread, to avoid affecting after coming requests.
    std::thread th([p = std::move(pr)]() mutable {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    p.SetValue();
                  });
    th.detach();
    fut.Then([context]() {
          trpc::Status status = trpc::kSuccStatus;
          trpc::testing::future::TestReply response;
          context->SendUnaryResponse(status, response);
          return trpc::MakeReadyFuture<>();
        });
    return trpc::kSuccStatus;
  }
};

int FutureServer::Initialize() {
  RegisterService("tcp_service", std::make_shared<FutureServiceImpl>());
  RegisterService("udp_service", std::make_shared<FutureServiceImpl>());
  server_->Start();
  test_signal_->SignalClientToContinue();
  return 0;
}

void FutureServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
