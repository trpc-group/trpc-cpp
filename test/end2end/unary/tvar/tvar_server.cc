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

#include "test/end2end/unary/tvar/tvar_server.h"

#include "test/end2end/common/util.h"
#include "test/end2end/unary/tvar/tvar.trpc.pb.h"
#include "trpc/tvar/tvar.h"

namespace trpc::testing {

class TvarServiceImpl final : public trpc::testing::tvar::Tvar {
 public:
  trpc::Status FireDynamicCounter(trpc::ServerContextPtr context,
                                  const trpc::testing::tvar::TestRequest* request,
                                  trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    counter_dynamic_ptr_ = std::make_unique<trpc::tvar::Counter<uint64_t>>(parent, "counter/dynamic");
    return trpc::kSuccStatus;
  }

  trpc::Status FireCounterAdd(trpc::ServerContextPtr context,
                              const trpc::testing::tvar::TestRequest* request,
                              trpc::testing::tvar::TestReply* response) override {
    counter_add_.Add(2);
    return trpc::kSuccStatus;
  }

  trpc::Status FireCounterIncrement(trpc::ServerContextPtr context,
                                    const trpc::testing::tvar::TestRequest* request,
                                    trpc::testing::tvar::TestReply* response) override {
    counter_increment_.Increment();
    return trpc::kSuccStatus;
  }

  trpc::Status GetCounterPath(trpc::ServerContextPtr context,
                              const trpc::testing::tvar::TestRequest* request,
                              trpc::testing::tvar::TestReply* response) override {
    response->set_msg(counter_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetCounterNotExposedPath(trpc::ServerContextPtr context,
                                        const trpc::testing::tvar::TestRequest* request,
                                        trpc::testing::tvar::TestReply* response) override {
    response->set_msg(counter_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicGauge(trpc::ServerContextPtr context,
                                const trpc::testing::tvar::TestRequest* request,
                                trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    gauge_dynamic_ptr_ = std::make_unique<trpc::tvar::Gauge<uint64_t>>(parent, "gauge/dynamic");
    return trpc::kSuccStatus;
  }

  trpc::Status FireGaugeAdd(trpc::ServerContextPtr context,
                            const trpc::testing::tvar::TestRequest* request,
                            trpc::testing::tvar::TestReply* response) override {
    gauge_add_.Add(2);
    return trpc::kSuccStatus;
  }

  trpc::Status FireGaugeSubtract(trpc::ServerContextPtr context,
                                 const trpc::testing::tvar::TestRequest* request,
                                 trpc::testing::tvar::TestReply* response) override {
    gauge_subtract_.Add(10);
    gauge_subtract_.Subtract(5);
    return trpc::kSuccStatus;
  }

  trpc::Status FireGaugeIncrement(trpc::ServerContextPtr context,
                                  const trpc::testing::tvar::TestRequest* request,
                                  trpc::testing::tvar::TestReply* response) override {
    gauge_increment_.Increment();
    return trpc::kSuccStatus;
  }

  trpc::Status FireGaugeDecrement(trpc::ServerContextPtr context,
                                  const trpc::testing::tvar::TestRequest* request,
                                  trpc::testing::tvar::TestReply* response) override {
    gauge_decrement_.Add(10);
    gauge_decrement_.Decrement();
    return trpc::kSuccStatus;
  }

  trpc::Status GetGaugePath(trpc::ServerContextPtr context,
                            const trpc::testing::tvar::TestRequest* request,
                            trpc::testing::tvar::TestReply* response) override {
    response->set_msg(gauge_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetGaugeNotExposedPath(trpc::ServerContextPtr context,
                                      const trpc::testing::tvar::TestRequest* request,
                                      trpc::testing::tvar::TestReply* response) override {
    response->set_msg(gauge_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicMiner(trpc::ServerContextPtr context,
                                const trpc::testing::tvar::TestRequest* request,
                                trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    miner_dynamic_ptr_ = std::make_unique<trpc::tvar::Miner<uint64_t>>(parent, "miner/dynamic", 100);
    return trpc::kSuccStatus;
  }

  trpc::Status FireMinerUpdate(trpc::ServerContextPtr context,
                               const trpc::testing::tvar::TestRequest* request,
                               trpc::testing::tvar::TestReply* response) override {
    miner_update_.Update(10);
    return trpc::kSuccStatus;
  }

  trpc::Status FireMinerNotUpdate(trpc::ServerContextPtr context,
                                  const trpc::testing::tvar::TestRequest* request,
                                  trpc::testing::tvar::TestReply* response) override {
    miner_not_update_.Update(20);
    return trpc::kSuccStatus;
  }

  trpc::Status GetMinerPath(trpc::ServerContextPtr context,
                            const trpc::testing::tvar::TestRequest* request,
                            trpc::testing::tvar::TestReply* response) override {
    response->set_msg(miner_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetMinerNotExposedPath(trpc::ServerContextPtr context,
                                      const trpc::testing::tvar::TestRequest* request,
                                      trpc::testing::tvar::TestReply* response) override {
    response->set_msg(miner_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicMaxer(trpc::ServerContextPtr context,
                                const trpc::testing::tvar::TestRequest* request,
                                trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    maxer_dynamic_ptr_ = std::make_unique<trpc::tvar::Maxer<uint64_t>>(parent, "maxer/dynamic");
    return trpc::kSuccStatus;
  }

  trpc::Status FireMaxerUpdate(trpc::ServerContextPtr context,
                               const trpc::testing::tvar::TestRequest* request,
                               trpc::testing::tvar::TestReply* response) override {
    maxer_update_.Update(10);
    return trpc::kSuccStatus;
  }

  trpc::Status FireMaxerNotUpdate(trpc::ServerContextPtr context,
                                  const trpc::testing::tvar::TestRequest* request,
                                  trpc::testing::tvar::TestReply* response) override {
    maxer_not_update_.Update(10);
    return trpc::kSuccStatus;
  }

  trpc::Status GetMaxerPath(trpc::ServerContextPtr context,
                            const trpc::testing::tvar::TestRequest* request,
                            trpc::testing::tvar::TestReply* response) override {
    response->set_msg(maxer_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetMaxerNotExposedPath(trpc::ServerContextPtr context,
                                      const trpc::testing::tvar::TestRequest* request,
                                      trpc::testing::tvar::TestReply* response) override {
    response->set_msg(maxer_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicStatus(trpc::ServerContextPtr context,
                                 const trpc::testing::tvar::TestRequest* request,
                                 trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    status_dynamic_ptr_ = std::make_unique<trpc::tvar::Status<uint64_t>>(parent, "status/dynamic", 0);
    return trpc::kSuccStatus;
  }

  trpc::Status FireStatusUpdate(trpc::ServerContextPtr context,
                                const trpc::testing::tvar::TestRequest* request,
                                trpc::testing::tvar::TestReply* response) override {
    status_update_.Update(10);
    return trpc::kSuccStatus;
  }

  trpc::Status GetStatusPath(trpc::ServerContextPtr context,
                             const trpc::testing::tvar::TestRequest* request,
                             trpc::testing::tvar::TestReply* response) override {
    response->set_msg(status_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetStatusNotExposedPath(trpc::ServerContextPtr context,
                                       const trpc::testing::tvar::TestRequest* request,
                                       trpc::testing::tvar::TestReply* response) override {
    response->set_msg(status_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicPassiveStatus(trpc::ServerContextPtr context,
                                        const trpc::testing::tvar::TestRequest* request,
                                        trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    passive_status_dynamic_ptr_ = std::make_unique<trpc::tvar::PassiveStatus<uint64_t>>(parent,
                                  "passivestatus/dynamic", [] { return 0; });
    return trpc::kSuccStatus;
  }

  trpc::Status GetPassiveStatusPath(trpc::ServerContextPtr context,
                                    const trpc::testing::tvar::TestRequest* request,
                                    trpc::testing::tvar::TestReply* response) override {
    response->set_msg(passive_status_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetPassiveStatusNotExposedPath(trpc::ServerContextPtr context,
                                              const trpc::testing::tvar::TestRequest* request,
                                              trpc::testing::tvar::TestReply* response) override {
    response->set_msg(passive_status_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicAverager(trpc::ServerContextPtr context,
                                   const trpc::testing::tvar::TestRequest* request,
                                   trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    averager_dynamic_ptr_ = std::make_unique<trpc::tvar::Averager<uint64_t>>(parent, "averager/dynamic");
    return trpc::kSuccStatus;
  }

  trpc::Status FireAveragerUpdate(trpc::ServerContextPtr context,
                                  const trpc::testing::tvar::TestRequest* request,
                                  trpc::testing::tvar::TestReply* response) override {
    for (uint64_t i = 1; i < 100; i++)
      averager_update_.Update(i);

    return trpc::kSuccStatus;
  }

  trpc::Status GetAveragerPath(trpc::ServerContextPtr context,
                               const trpc::testing::tvar::TestRequest* request,
                               trpc::testing::tvar::TestReply* response) override {
    response->set_msg(averager_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetAveragerNotExposedPath(trpc::ServerContextPtr context,
                                         const trpc::testing::tvar::TestRequest* request,
                                         trpc::testing::tvar::TestReply* response) override {
    response->set_msg(averager_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicIntRecorder(trpc::ServerContextPtr context,
                                      const trpc::testing::tvar::TestRequest* request,
                                      trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    intrecorder_dynamic_ptr_ = std::make_unique<trpc::tvar::IntRecorder>(parent, "intrecorder/dynamic");
    return trpc::kSuccStatus;
  }

  trpc::Status FireIntRecorderUpdate(trpc::ServerContextPtr context,
                                     const trpc::testing::tvar::TestRequest* request,
                                     trpc::testing::tvar::TestReply* response) override {
    for (uint64_t i = 1; i < 100; i++)
      intrecorder_update_.Update(i);

    return trpc::kSuccStatus;
  }

  trpc::Status GetIntRecorderPath(trpc::ServerContextPtr context,
                                  const trpc::testing::tvar::TestRequest* request,
                                  trpc::testing::tvar::TestReply* response) override {
    response->set_msg(intrecorder_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetIntRecorderNotExposedPath(trpc::ServerContextPtr context,
                                            const trpc::testing::tvar::TestRequest* request,
                                            trpc::testing::tvar::TestReply* response) override {
    response->set_msg(intrecorder_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicWindowCounter(trpc::ServerContextPtr context,
                                        const trpc::testing::tvar::TestRequest* request,
                                        trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    window_dynamic_ptr_ = std::make_unique<trpc::tvar::Window<trpc::tvar::Counter<uint64_t>>>(parent,
                          "window/dynamic", &counter_normal_);
    return trpc::kSuccStatus;
  }

  trpc::Status GetWindowCounterPath(trpc::ServerContextPtr context,
                                    const trpc::testing::tvar::TestRequest* request,
                                    trpc::testing::tvar::TestReply* response) override {
    response->set_msg(window_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetWindowCounterNotExposedPath(trpc::ServerContextPtr context,
                                              const trpc::testing::tvar::TestRequest* request,
                                              trpc::testing::tvar::TestReply* response) override {
    response->set_msg(window_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicPerSecondCounter(trpc::ServerContextPtr context,
                                           const trpc::testing::tvar::TestRequest* request,
                                           trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    persecond_dynamic_ptr_ = std::make_unique<trpc::tvar::PerSecond<trpc::tvar::Counter<uint64_t>>>(parent,
                             "persecond/dynamic", &counter_normal_);
    return trpc::kSuccStatus;
  }

  trpc::Status GetPerSecondCounterPath(trpc::ServerContextPtr context,
                                       const trpc::testing::tvar::TestRequest* request,
                                       trpc::testing::tvar::TestReply* response) override {
    response->set_msg(persecond_normal_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status GetPerSecondCounterNotExposedPath(trpc::ServerContextPtr context,
                                                 const trpc::testing::tvar::TestRequest* request,
                                                 trpc::testing::tvar::TestReply* response) override {
    response->set_msg(persecond_not_exposed_.GetAbsPath());
    return trpc::kSuccStatus;
  }

  trpc::Status FireDynamicLatencyRecorder(trpc::ServerContextPtr context,
                                          const trpc::testing::tvar::TestRequest* request,
                                          trpc::testing::tvar::TestReply* response) override {
    trpc::tvar::TrpcVarGroup* parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/");
    latency_recorder_dynamic_ptr_ = std::make_unique<trpc::tvar::LatencyRecorder>(parent, "latencyrecorder/dynamic");
    return trpc::kSuccStatus;
  }

  trpc::Status FireLatencyRecorderUpdate(trpc::ServerContextPtr context,
                                         const trpc::testing::tvar::TestRequest* request,
                                         trpc::testing::tvar::TestReply* response) override {
    for (uint32_t i = 1; i <= 10000; i++)
      latency_recorder_update_.Update(i);

    return trpc::kSuccStatus;
  }

 private:
  // Counter.
  trpc::tvar::Counter<uint64_t> counter_normal_{"counter/normal"};
  trpc::tvar::Counter<uint64_t> counter_add_{"counter/add"};
  trpc::tvar::Counter<uint64_t> counter_increment_{"counter/increment"};
  trpc::tvar::Counter<uint64_t> counter_not_exposed_;
  std::unique_ptr<trpc::tvar::Counter<uint64_t>> counter_dynamic_ptr_;
  // Gauge.
  trpc::tvar::Gauge<uint64_t> gauge_normal_{"gauge/normal"};
  trpc::tvar::Gauge<uint64_t> gauge_add_{"gauge/add"};
  trpc::tvar::Gauge<uint64_t> gauge_subtract_{"gauge/subtract"};
  trpc::tvar::Gauge<uint64_t> gauge_increment_{"gauge/increment"};
  trpc::tvar::Gauge<uint64_t> gauge_decrement_{"gauge/decrement"};
  trpc::tvar::Gauge<uint64_t> gauge_not_exposed_;
  std::unique_ptr<trpc::tvar::Gauge<uint64_t>> gauge_dynamic_ptr_;
  // Miner.
  trpc::tvar::Miner<uint64_t> miner_normal_{"miner/normal", 100};
  trpc::tvar::Miner<uint64_t> miner_update_{"miner/update"};
  trpc::tvar::Miner<uint64_t> miner_not_update_{"miner/notupdate", 10};
  trpc::tvar::Miner<uint64_t> miner_not_exposed_;
  std::unique_ptr<trpc::tvar::Miner<uint64_t>> miner_dynamic_ptr_;
  // Maxer.
  trpc::tvar::Maxer<uint64_t> maxer_normal_{"maxer/normal"};
  trpc::tvar::Maxer<uint64_t> maxer_update_{"maxer/update"};
  trpc::tvar::Maxer<uint64_t> maxer_not_update_{"maxer/notupdate", 100};
  trpc::tvar::Maxer<uint64_t> maxer_not_exposed_;
  std::unique_ptr<trpc::tvar::Maxer<uint64_t>> maxer_dynamic_ptr_;
  // Status.
  trpc::tvar::Status<uint64_t> status_normal_{"status/normal", 0};
  trpc::tvar::Status<uint64_t> status_update_{"status/update", 0};
  trpc::tvar::Status<uint64_t> status_not_exposed_;
  std::unique_ptr<trpc::tvar::Status<uint64_t>> status_dynamic_ptr_;
  // PassiveStatus.
  trpc::tvar::PassiveStatus<uint64_t> passive_status_normal_{"passivestatus/normal", [] { return 0; }};
  trpc::tvar::PassiveStatus<uint64_t> passive_status_not_exposed_{[] { return 0; }};
  std::unique_ptr<trpc::tvar::PassiveStatus<uint64_t>> passive_status_dynamic_ptr_;
  // Averager.
  trpc::tvar::Averager<uint64_t> averager_normal_{"averager/normal"};
  trpc::tvar::Averager<uint64_t> averager_update_{"averager/update"};
  trpc::tvar::Averager<uint64_t> averager_not_exposed_;
  std::unique_ptr<trpc::tvar::Averager<uint64_t>> averager_dynamic_ptr_;
  // IntRecorder.
  trpc::tvar::IntRecorder intrecorder_normal_{"intrecorder/normal"};
  trpc::tvar::IntRecorder intrecorder_update_{"intrecorder/update"};
  trpc::tvar::IntRecorder intrecorder_not_exposed_;
  std::unique_ptr<trpc::tvar::IntRecorder> intrecorder_dynamic_ptr_;
  // Window.
  trpc::tvar::Window<trpc::tvar::Counter<uint64_t>> window_normal_{"window/normal", &counter_normal_};
  trpc::tvar::Window<trpc::tvar::Counter<uint64_t>> window_not_exposed_{&counter_normal_};
  std::unique_ptr<trpc::tvar::Window<trpc::tvar::Counter<uint64_t>>> window_dynamic_ptr_;
  // PerSecond.
  trpc::tvar::PerSecond<trpc::tvar::Counter<uint64_t>> persecond_normal_{"persecond/normal", &counter_normal_};
  trpc::tvar::PerSecond<trpc::tvar::Counter<uint64_t>> persecond_not_exposed_{&counter_normal_};
  std::unique_ptr<trpc::tvar::PerSecond<trpc::tvar::Counter<uint64_t>>> persecond_dynamic_ptr_;
  // LatencyRecorder.
  trpc::tvar::LatencyRecorder latency_recorder_update_{"latencyrecorder/update"};
  trpc::tvar::LatencyRecorder latency_recorder_not_exposed_;
  std::unique_ptr<trpc::tvar::LatencyRecorder> latency_recorder_dynamic_ptr_;
};

int TvarServer::Initialize() {
  {
    trpc::tvar::Counter<uint64_t> counter_tmp("counter/tmp");
    trpc::tvar::Gauge<uint64_t> gauge_tmp("gauge/tmp");
    trpc::tvar::Miner<uint64_t> miner_tmp("miner/tmp");
    trpc::tvar::Maxer<uint64_t> maxer_tmp("maxer/tmp");
    trpc::tvar::Status<uint64_t> status_tmp("status/tmp", 0);
    trpc::tvar::PassiveStatus<uint64_t> passive_status_tmp("passivestatus/tmp", [] { return 0; });
    trpc::tvar::Averager<uint64_t> averager_tmp("averager/tmp");
    trpc::tvar::IntRecorder intrecorder_tmp("intrecorder/tmp");
    trpc::tvar::Window<trpc::tvar::Counter<uint64_t>> window_tmp("window/tmp", &counter_tmp);
    trpc::tvar::PerSecond<trpc::tvar::Counter<uint64_t>> persecond_tmp("persecond/tmp", &counter_tmp);
    trpc::tvar::LatencyRecorder latency_recorder_tmp{"latencyrecorder/tmp"};
  }
  RegisterService("tvar_service", std::make_shared<TvarServiceImpl>());
  server_->Start();
  test_signal_->SignalClientToContinue();
  return 0;
}

void TvarServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
