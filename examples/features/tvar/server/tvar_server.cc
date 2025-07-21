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

#include "examples/features/tvar/server/tvar_server.h"

#include "trpc/tvar/tvar.h"

#include "examples/features/tvar/server/tvar.trpc.pb.h"

namespace trpc::examples {

class TvarServiceImpl final : public ::trpc::examples::tvar::Tvar {
 public:
  trpc::Status Counter(trpc::ServerContextPtr context,
                       const trpc::examples::tvar::TestRequest* request,
                       trpc::examples::tvar::TestReply* response) override {
    counter_.Add(2);
    return trpc::kSuccStatus;
  }

  trpc::Status Gauge(trpc::ServerContextPtr context,
                     const trpc::examples::tvar::TestRequest* request,
                     trpc::examples::tvar::TestReply* response) override {
    gauge_.Add(2);
    return trpc::kSuccStatus;
  }

  trpc::Status Miner(trpc::ServerContextPtr context,
                     const trpc::examples::tvar::TestRequest* request,
                     trpc::examples::tvar::TestReply* response) override {
    miner_.Update(10);
    return trpc::kSuccStatus;
  }

  trpc::Status Maxer(trpc::ServerContextPtr context,
                     const trpc::examples::tvar::TestRequest* request,
                     trpc::examples::tvar::TestReply* response) override {
    maxer_.Update(10);
    return trpc::kSuccStatus;
  }

  trpc::Status Status(trpc::ServerContextPtr context,
                      const trpc::examples::tvar::TestRequest* request,
                      trpc::examples::tvar::TestReply* response) override {
    status_.Update(10);
    return trpc::kSuccStatus;
  }

  trpc::Status Averager(trpc::ServerContextPtr context,
                        const trpc::examples::tvar::TestRequest* request,
                        trpc::examples::tvar::TestReply* response) override {
    for (uint64_t i = 1; i < 100; i++)
      averager_.Update(i);

    return trpc::kSuccStatus;
  }

  trpc::Status IntRecorder(trpc::ServerContextPtr context,
                           const trpc::examples::tvar::TestRequest* request,
                           trpc::examples::tvar::TestReply* response) override {
    for (uint64_t i = 1; i < 100; i++)
      intrecorder_.Update(i);

    return trpc::kSuccStatus;
  }

  trpc::Status LatencyRecorder(trpc::ServerContextPtr context,
                               const trpc::examples::tvar::TestRequest* request,
                               trpc::examples::tvar::TestReply* response) override {
    for (uint32_t i = 1; i <= 10000; i++)
      latency_recorder_.Update(i);

    return trpc::kSuccStatus;
  }

 private:
  // Counter.
  trpc::tvar::Counter<uint64_t> counter_{"counter"};
  // Gauge.
  trpc::tvar::Gauge<uint64_t> gauge_{"gauge"};
  // Miner.
  trpc::tvar::Miner<uint64_t> miner_{"miner"};
  // Maxer.
  trpc::tvar::Maxer<uint64_t> maxer_{"maxer"};
  // Status.
  trpc::tvar::Status<uint64_t> status_{"status", 0};
  // PassiveStatus.
  trpc::tvar::PassiveStatus<uint64_t> passive_status_{"passivestatus", [] { return 10; }};
  // Averager.
  trpc::tvar::Averager<uint64_t> averager_{"averager"};
  // IntRecorder.
  trpc::tvar::IntRecorder intrecorder_{"intrecorder"};
  // Window.
  trpc::tvar::Window<trpc::tvar::Counter<uint64_t>> window_{"window", &counter_};
  // PerSecond.
  trpc::tvar::PerSecond<trpc::tvar::Counter<uint64_t>> persecond_{"persecond", &counter_};
  // LatencyRecorder.
  trpc::tvar::LatencyRecorder latency_recorder_{"latencyrecorder"};
};

int TvarServer::Initialize() {
  RegisterService("tvar_service", std::make_shared<TvarServiceImpl>());
  return 0;
}

void TvarServer::Destroy() {}

}  // namespace trpc::examples

int main(int argc, char** argv) {
  ::trpc::examples::TvarServer tvar_server;
  tvar_server.Main(argc, argv);
  tvar_server.Wait();

  return 0;
}
