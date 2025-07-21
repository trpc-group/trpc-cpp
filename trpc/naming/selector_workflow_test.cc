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

#include "trpc/naming/selector_workflow.h"

#include <any>
#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/naming/selector.h"
#include "trpc/naming/selector_factory.h"

namespace trpc::testing {

namespace {

/// @brief Mock service discovery plugin
class MockSelector : public Selector {
 public:
  struct Option {
    std::function<int(const SelectorInfo* info, TrpcEndpointInfo* endpoint)> select;
    std::function<Future<TrpcEndpointInfo>(const SelectorInfo* info)> async_select;
    std::function<int(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints)> select_batch;
    std::function<Future<std::vector<TrpcEndpointInfo>>(const SelectorInfo* info)> async_select_batch;
    std::function<int(const InvokeResult* result)> report_invoke_result;
    std::function<int(const RouterInfo* info)> set_endpoints;
  };

 public:
  explicit MockSelector(Option option) : option_(std::move(option)) {}

  /// @brief plugin name
  std::string Name() const override { return "mock_selector"; };

  /// @brief plugin version
  std::string Version() const override { return "0.0.1"; };

  /// @brief initialization
  /// @return Returns 0 on success, -1 on failure
  int Init() noexcept override { return 0; }

  /// @brief Gets the routing interface of the node being called
  int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override {
    if (option_.select != nullptr) {
      return option_.select(info, endpoint);
    }
    return 0;
  }

  /// @brief fetches a moderated point interface asynchronously
  Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override {
    if (option_.async_select != nullptr) {
      return option_.async_select(info);
    }
    assert(false && "bad async_select");
  }

  /// @brief interface to fetch nodes' routing information in batches by policy
  int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override {
    if (option_.select_batch != nullptr) {
      return option_.select_batch(info, endpoints);
    }
    return 0;
  }

  /// @brief async interface to fetch nodes' routing information in batches by policy
  Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override {
    if (option_.async_select_batch != nullptr) {
      return option_.async_select_batch(info);
    }
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("bad async_select_batch"));
  }

  /// @brief call result reporting interface
  int ReportInvokeResult(const InvokeResult* result) override {
    if (option_.report_invoke_result != nullptr) {
      return option_.report_invoke_result(result);
    }
    return 0;
  }

  /// @brief Sets the route information interface for the called service
  int SetEndpoints(const RouterInfo* info) override {
    if (option_.set_endpoints != nullptr) {
      return option_.set_endpoints(info);
    }
    return 0;
  }

 private:
  Option option_;
};

}  // namespace

class SelectorWorkFlowTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  SelectorPtr DefaultMockSelector() { return MakeRefCounted<MockSelector>(MockSelector::Option{}); }
};

TEST_F(SelectorWorkFlowTest, Init) {
  {
    SelectorWorkFlow work_flow{"mock_selector", false, false};
    ASSERT_EQ(work_flow.Init(), -1);
  }
  {
    SelectorFactory::GetInstance()->Register(DefaultMockSelector());
    SelectorWorkFlow work_flow{"mock_selector", false, false};
    ASSERT_EQ(work_flow.Init(), 0);
    SelectorFactory::GetInstance()->Clear();
  }
}

TEST_F(SelectorWorkFlowTest, SelectTarget) {
  // ServiceProxyOption is stored inside the ServiceProxy object and is shared by the ClientContext
  ServiceProxyOption option;
  option.name = "foo.test";
  option.target = "foo.test";
  option.selector_name = "mock_selector";

  MockSelector::Option mock_selector_option{
    select : [](const SelectorInfo* info, TrpcEndpointInfo* endpoint) -> int {
      endpoint->host = info->name + "-127.0.0.1";
      endpoint->port = 12345;
      return 0;
    }
  };
  auto selector = MakeRefCounted<MockSelector>(mock_selector_option);
  SelectorFactory::GetInstance()->Register(selector);
  SelectorWorkFlow work_flow{"mock_selector", false, false};
  auto trpc_codec = std::make_shared<TrpcClientCodec>();
  {
    // Use the target in ServiceProxy
    auto context = trpc::MakeRefCounted<ClientContext>(trpc_codec);
    context->SetServiceProxyOption(&option);
    work_flow.SelectTarget(context);
    ASSERT_EQ(context->GetIp(), "foo.test-127.0.0.1");
    ASSERT_EQ(context->GetPort(), 12345);
  }
  {
    // Address using the Target set in the Context
    auto context = trpc::MakeRefCounted<ClientContext>(trpc_codec);
    context->SetServiceProxyOption(&option);
    context->SetServiceTarget("bar.test");
    ASSERT_EQ(context->GetServiceTarget(), "bar.test");

    work_flow.SelectTarget(context);
    ASSERT_EQ(context->GetIp(), "bar.test-127.0.0.1");
    ASSERT_EQ(context->GetPort(), 12345);
  }

  SelectorFactory::GetInstance()->Clear();
}

}  // namespace trpc::testing
