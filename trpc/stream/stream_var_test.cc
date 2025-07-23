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

#include "trpc/stream/stream_var.h"

#include <string>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/metrics/metrics.h"
#include "trpc/metrics/metrics_factory.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

namespace {
const char kSayHelloVarPath[] = "trpc/stream_rpc/client/trpc.test.helloworld.Greeter/StreamSayHello";
const char kTestStreamVarMetricsName[] = "test_trpc_stream_var_metrics";
}  // namespace

namespace {
class TestStreamVarMetrics : public trpc::Metrics {
 public:
  TestStreamVarMetrics() = default;

  ~TestStreamVarMetrics() override = default;

  PluginType Type() const override { return PluginType::kMetrics; }

  std::string Name() const override { return kTestStreamVarMetricsName; }

  int Init() noexcept override { return 0; }

  void Start() noexcept override {}

  void Stop() noexcept override {}

  void Destroy() noexcept override {}

  int ModuleReport(const ModuleMetricsInfo& info) override { return 0; }

  int SingleAttrReport(const SingleAttrMetricsInfo& info) override {
    if (info.name.empty() || info.dimension.empty()) {
      return -1;
    }
    return 0; 
  }

  int MultiAttrReport(const MultiAttrMetricsInfo& info) override { return 0; }

  using trpc::Metrics::ModuleReport;
  using trpc::Metrics::SingleAttrReport;
  using trpc::Metrics::MultiAttrReport;
};

void RegisterTestStreamVarMetrics() {
  auto metrics_ptr = MakeRefCounted<TestStreamVarMetrics>();
  MetricsFactory::GetInstance()->Register(metrics_ptr);
}
}  // namespace

class StreamVarTest : public ::testing::Test {
 protected:
  void SetUp() override { stream_var_ = CreateStreamVar(kSayHelloVarPath); }

  void TearDown() override {}

 protected:
  StreamVarPtr stream_var_{nullptr};
};

TEST_F(StreamVarTest, AddRpcCallCountOk) {
  stream_var_->AddRpcCallCount(1);
  std::string var_path{""};
  ASSERT_EQ(1, stream_var_->GetRpcCallCountValue(&var_path));
  ASSERT_FALSE(var_path.empty());
}

TEST_F(StreamVarTest, AddRpcCallFailureCountOk) {
  stream_var_->AddRpcCallFailureCount(2);
  std::string var_path{""};
  ASSERT_EQ(2, stream_var_->GetRpcCallFailureCountValue(&var_path));
  ASSERT_FALSE(var_path.empty());
}

TEST_F(StreamVarTest, AddSendMessageCountOk) {
  stream_var_->AddSendMessageCount(3);
  std::string var_path{""};
  ASSERT_EQ(3, stream_var_->GetSendMessageCountValue(&var_path));
  ASSERT_FALSE(var_path.empty());
}

TEST_F(StreamVarTest, AddRecvMessageCountOk) {
  stream_var_->AddRecvMessageCount(4);
  std::string var_path{""};
  ASSERT_EQ(4, stream_var_->GetRecvMessageCountValue(&var_path));
  ASSERT_FALSE(var_path.empty());
  ASSERT_FALSE(var_path.empty());
}

TEST_F(StreamVarTest, AddSendMessageBytesOK) {
  stream_var_->AddSendMessageBytes(5);
  std::string var_path{""};
  ASSERT_EQ(5, stream_var_->GetSendMessageBytesValue(&var_path));
  ASSERT_FALSE(var_path.empty());
}

TEST_F(StreamVarTest, AddRecvMessageBytesOK) {
  stream_var_->AddRecvMessageBytes(6);
  std::string var_path{""};
  ASSERT_EQ(6, stream_var_->GetRecvMessageBytesValue(&var_path));
  ASSERT_FALSE(var_path.empty());
}

TEST_F(StreamVarTest, CaptureStreamVarSnapshotOk) {
  stream_var_->AddRpcCallCount(1);
  stream_var_->AddRpcCallFailureCount(2);
  stream_var_->AddSendMessageCount(3);
  stream_var_->AddRecvMessageCount(4);
  stream_var_->AddSendMessageBytes(5);
  stream_var_->AddRecvMessageBytes(6);

  std::unordered_map<std::string, uint64_t> stream_var_snapshot{};
  stream_var_->CaptureStreamVarSnapshot(&stream_var_snapshot);
  ASSERT_FALSE(stream_var_snapshot.empty());
  ASSERT_EQ(6, stream_var_snapshot.size());

  std::string var_path{""};
  uint64_t var_value{0};

  var_value = stream_var_->GetRpcCallCountValue(&var_path);
  ASSERT_EQ(var_value, stream_var_snapshot[var_path]);

  var_value = stream_var_->GetRpcCallFailureCountValue(&var_path);
  ASSERT_EQ(var_value, stream_var_snapshot[var_path]);

  var_value = stream_var_->GetSendMessageCountValue(&var_path);
  ASSERT_EQ(var_value, stream_var_snapshot[var_path]);

  var_value = stream_var_->GetRecvMessageCountValue(&var_path);
  ASSERT_EQ(var_value, stream_var_snapshot[var_path]);

  var_value = stream_var_->GetSendMessageBytesValue(&var_path);
  ASSERT_EQ(var_value, stream_var_snapshot[var_path]);

  var_value = stream_var_->GetRecvMessageBytesValue(&var_path);
  ASSERT_EQ(var_value, stream_var_snapshot[var_path]);
}

TEST_F(StreamVarTest, GetStreamVarMetricsOk) {
  stream_var_->AddRpcCallCount(1);
  stream_var_->AddRpcCallFailureCount(2);
  stream_var_->AddSendMessageCount(3);
  stream_var_->AddRecvMessageCount(4);
  stream_var_->AddSendMessageBytes(5);
  stream_var_->AddRecvMessageBytes(6);

  std::unordered_map<std::string, uint64_t> metrics{};
  stream_var_->GetStreamVarMetrics(&metrics);
  ASSERT_FALSE(metrics.empty());
  ASSERT_EQ(6, metrics.size());

  std::string var_path{""};

  // First-time invocation, metrics are the same as initial value.
  stream_var_->GetRpcCallCountValue(&var_path);
  ASSERT_EQ(1, metrics[var_path]);

  stream_var_->GetRpcCallFailureCountValue(&var_path);
  ASSERT_EQ(2, metrics[var_path]);

  stream_var_->GetSendMessageCountValue(&var_path);
  ASSERT_EQ(3, metrics[var_path]);

  stream_var_->GetRecvMessageCountValue(&var_path);
  ASSERT_EQ(4, metrics[var_path]);

  stream_var_->GetSendMessageBytesValue(&var_path);
  ASSERT_EQ(5, metrics[var_path]);

  stream_var_->GetRecvMessageBytesValue(&var_path);
  ASSERT_EQ(6, metrics[var_path]);

  // First-time invocation, metrics equal to zero .
  stream_var_->GetStreamVarMetrics(&metrics);

  stream_var_->GetRpcCallCountValue(&var_path);
  ASSERT_EQ(0, metrics[var_path]);

  stream_var_->GetRpcCallFailureCountValue(&var_path);
  ASSERT_EQ(0, metrics[var_path]);

  stream_var_->GetSendMessageCountValue(&var_path);
  ASSERT_EQ(0, metrics[var_path]);

  stream_var_->GetRecvMessageCountValue(&var_path);
  ASSERT_EQ(0, metrics[var_path]);

  stream_var_->GetSendMessageBytesValue(&var_path);
  ASSERT_EQ(0, metrics[var_path]);

  stream_var_->GetRecvMessageBytesValue(&var_path);
  ASSERT_EQ(0, metrics[var_path]);

  // Third call, there is a diff.
  stream_var_->AddRpcCallCount(1);
  stream_var_->AddRpcCallFailureCount(2);
  stream_var_->AddSendMessageCount(3);
  stream_var_->AddRecvMessageCount(4);
  stream_var_->AddSendMessageBytes(5);
  stream_var_->AddRecvMessageBytes(6);

  stream_var_->GetStreamVarMetrics(&metrics);

  stream_var_->GetRpcCallCountValue(&var_path);
  ASSERT_EQ(1, metrics[var_path]);

  stream_var_->GetRpcCallFailureCountValue(&var_path);
  ASSERT_EQ(2, metrics[var_path]);

  stream_var_->GetSendMessageCountValue(&var_path);
  ASSERT_EQ(3, metrics[var_path]);

  stream_var_->GetRecvMessageCountValue(&var_path);
  ASSERT_EQ(4, metrics[var_path]);

  stream_var_->GetSendMessageBytesValue(&var_path);
  ASSERT_EQ(5, metrics[var_path]);

  stream_var_->GetRecvMessageBytesValue(&var_path);
  ASSERT_EQ(6, metrics[var_path]);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TEST(StreamVarHelperTest, GetStreamVarOk) {
  RunAsFiber([]() {
    ASSERT_TRUE(StreamVarHelper::GetInstance()->GetOrCreateStreamVar(kSayHelloVarPath));
    ASSERT_TRUE(StreamVarHelper::GetInstance()->GetOrCreateStreamVar(kSayHelloVarPath));
  });
}

TEST(StreamVarHelperTest, ReportMetricsOk) {
  RunAsFiber([]() {
    auto stream_var = StreamVarHelper::GetInstance()->GetOrCreateStreamVar(kSayHelloVarPath);
    std::unordered_map<std::string, uint64_t> metrics{};
    stream_var->GetStreamVarMetrics(&metrics);
    RegisterTestStreamVarMetrics();
    ASSERT_FALSE(metrics.empty());
    ASSERT_TRUE(StreamVarHelper::GetInstance()->ReportMetrics(kTestStreamVarMetricsName, metrics));
  });
}

TEST(StreamVarHelperTest, ReportStreamVarMetricsOk) {
  RunAsFiber([]() {
    auto stream_var = StreamVarHelper::GetInstance()->GetOrCreateStreamVar(kSayHelloVarPath);
    RegisterTestStreamVarMetrics();
    ASSERT_TRUE(StreamVarHelper::GetInstance()->ReportStreamVarMetrics(kTestStreamVarMetricsName));
  });
}

TEST(StreamVarHelperTest, ActiveStreamCountOk) {
  RunAsFiber([]() {
    ASSERT_EQ(0, StreamVarHelper::GetInstance()->GetActiveStreamCount());

    StreamVarHelper::GetInstance()->IncrementActiveStreamCount();
    StreamVarHelper::GetInstance()->IncrementActiveStreamCount();
    ASSERT_EQ(2, StreamVarHelper::GetInstance()->GetActiveStreamCount());

    StreamVarHelper::GetInstance()->DecrementActiveStreamCount();
    StreamVarHelper::GetInstance()->DecrementActiveStreamCount();
    ASSERT_EQ(0, StreamVarHelper::GetInstance()->GetActiveStreamCount());
  });
}

}  // namespace trpc::testing
