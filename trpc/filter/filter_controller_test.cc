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

#include "trpc/filter/filter_controller.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/filter/testing/client_filter_testing.h"
#include "trpc/filter/testing/server_filter_testing.h"
#include "trpc/server/server_context.h"
#include "trpc/transport/common/transport_message.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::testing {

struct FilterTestData {
  std::vector<std::string> filter_names;  // used to save the name of the executed filters
};

const uint32_t kTestFilterIndex = 99;

template <class T>
static void ExecuteFilter(const std::string& name, FilterStatus& status, FilterStatus expect_status, const T& context) {
  auto* filter_data = context->template GetFilterData<FilterTestData>(kTestFilterIndex);
  filter_data->filter_names.push_back(name);
  status = expect_status;
}

class FilterControllerTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    filter_status_["server_global_filter"] = FilterStatus::CONTINUE;
    filter_status_["server_global_reject_filter"] = FilterStatus::REJECT;
    filter_status_["server_service_filter"] = FilterStatus::CONTINUE;
    filter_status_["server_service_reject_filter"] = FilterStatus::REJECT;
    filter_status_["client_global_filter"] = FilterStatus::CONTINUE;
    filter_status_["client_global_reject_filter"] = FilterStatus::REJECT;
    filter_status_["client_service_filter"] = FilterStatus::CONTINUE;
    filter_status_["client_service_reject_filter"] = FilterStatus::REJECT;
  }

  static void TearDownTestCase() {}

 protected:
  void SetUp() override {
    MessageServerFilterPtr global_server_filter(new MockServerFilter());
    MockFilterInit<MessageServerFilterPtr, MockServerFilter, ServerContextPtr>(
        global_server_filter, "server_global_filter",
        {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE});

    auto* filter_manager = FilterManager::GetInstance();
    filter_manager->AddMessageServerGlobalFilter(global_server_filter);

    ASSERT_TRUE(filter_manager->GetMessageServerGlobalFilter(global_server_filter->GetFilterPoint()[0]).size() > 0);

    MessageClientFilterPtr global_client_filter(new MockClientFilter());

    MockFilterInit<MessageClientFilterPtr, MockClientFilter, ClientContextPtr>(
        global_client_filter, "client_global_filter",
        {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE});

    ASSERT_EQ(0, global_client_filter->Init());
    filter_manager->AddMessageClientGlobalFilter(global_client_filter);
    ASSERT_TRUE(filter_manager->GetMessageClientGlobalFilter(global_client_filter->GetFilterPoint()[0]).size() > 0);
  }

  void TearDown() override { FilterManager::GetInstance()->Clear(); }

  template <class T, class U, class C>
  static void MockFilterInit(const T& filter, const std::string& name, const std::vector<FilterPoint>& points) {
    U* mock_server_filter = static_cast<U*>(filter.get());
    EXPECT_CALL(*mock_server_filter, GetFilterPoint()).WillRepeatedly(::testing::Return(points));
    EXPECT_CALL(*mock_server_filter, Name()).WillRepeatedly(::testing::Return(name));
    EXPECT_CALL(*mock_server_filter, BracketOp(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([s = filter_status_[name], name](FilterStatus& status, FilterPoint point, const C& context) {
          ExecuteFilter<C>(name, status, s, context);
          if (name == "server_service_reject_filter") {
            if (point == FilterPoint::SERVER_PRE_RPC_INVOKE) {
              status = FilterStatus::REJECT;
            } else {
              status = FilterStatus::CONTINUE;
            }
          }

          if (name == "client_service_reject_filter") {
            if (point == FilterPoint::CLIENT_PRE_RPC_INVOKE) {
              status = FilterStatus::REJECT;
            } else {
              status = FilterStatus::CONTINUE;
            }
          }
        });
  }

 protected:
  static std::unordered_map<std::string, FilterStatus> filter_status_;
};

std::unordered_map<std::string, FilterStatus> FilterControllerTestFixture::filter_status_;

TEST_F(FilterControllerTestFixture, RunMessageServerFilters) {
  MessageServerFilterPtr server_filter(new MockServerFilter());
  MockFilterInit<MessageServerFilterPtr, MockServerFilter, ServerContextPtr>(
      server_filter, "server_service_filter",
      {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE});

  FilterController filter_controller;
  filter_controller.AddMessageServerFilter(server_filter);

  ServerContextPtr ctx = MakeRefCounted<ServerContext>();
  ctx->SetFilterData<FilterTestData>(kTestFilterIndex, FilterTestData());
  FilterStatus status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  auto* filter_data = ctx->GetFilterData<FilterTestData>(kTestFilterIndex);
  ASSERT_EQ(filter_data->filter_names.size(), 4);
  ASSERT_EQ(filter_data->filter_names[0], "server_global_filter");
  ASSERT_EQ(filter_data->filter_names[1], "server_service_filter");
  ASSERT_EQ(filter_data->filter_names[2], "server_service_filter");
  ASSERT_EQ(filter_data->filter_names[3], "server_global_filter");
}

TEST_F(FilterControllerTestFixture, RunMessageServerFiltersWithGlobalReject) {
  MessageServerFilterPtr server_filter(new MockServerFilter());
  MockFilterInit<MessageServerFilterPtr, MockServerFilter, ServerContextPtr>(
      server_filter, "server_global_reject_filter",
      {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE});
  FilterManager::GetInstance()->AddMessageServerGlobalFilter(server_filter);

  MessageServerFilterPtr server_filter1(new MockServerFilter());
  MockFilterInit<MessageServerFilterPtr, MockServerFilter, ServerContextPtr>(
      server_filter1, "server_service_filter",
      {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE});

  FilterController filter_controller;
  filter_controller.AddMessageServerFilter(server_filter1);

  ServerContextPtr ctx = MakeRefCounted<ServerContext>();
  ctx->SetFilterData<FilterTestData>(kTestFilterIndex, FilterTestData());
  FilterStatus status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  auto* filter_data = ctx->GetFilterData<FilterTestData>(kTestFilterIndex);
  ASSERT_EQ(filter_data->filter_names.size(), 3);
  ASSERT_EQ(filter_data->filter_names[0], "server_global_filter");
  ASSERT_EQ(filter_data->filter_names[1], "server_global_reject_filter");
  ASSERT_EQ(filter_data->filter_names[2], "server_global_filter");
}

TEST_F(FilterControllerTestFixture, RunMessageServerFiltersWithServiceReject) {
  FilterController filter_controller;
  MessageServerFilterPtr server_filter(new MockServerFilter());
  MockFilterInit<MessageServerFilterPtr, MockServerFilter, ServerContextPtr>(
      server_filter, "server_service_reject_filter",
      {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE});
  filter_controller.AddMessageServerFilter(server_filter);

  MessageServerFilterPtr server_filter1(new MockServerFilter());
  MockFilterInit<MessageServerFilterPtr, MockServerFilter, ServerContextPtr>(
      server_filter1, "server_service_filter",
      {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE});
  filter_controller.AddMessageServerFilter(server_filter1);

  ServerContextPtr ctx = MakeRefCounted<ServerContext>();
  ctx->SetFilterData<FilterTestData>(kTestFilterIndex, FilterTestData());
  FilterStatus status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  auto* filter_data = ctx->GetFilterData<FilterTestData>(kTestFilterIndex);
  ASSERT_EQ(filter_data->filter_names.size(), 3);
  ASSERT_EQ(filter_data->filter_names[0], "server_global_filter");
  ASSERT_EQ(filter_data->filter_names[1], "server_service_reject_filter");
  ASSERT_EQ(filter_data->filter_names[2], "server_global_filter");
}

TEST_F(FilterControllerTestFixture, RunMessageClientFilters) {
  MessageClientFilterPtr client_filter(new MockClientFilter());
  MockFilterInit<MessageClientFilterPtr, MockClientFilter, ClientContextPtr>(
      client_filter, "client_service_filter",
      {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE});
  ASSERT_EQ(0, client_filter->Init());
  FilterController filter_controller;
  filter_controller.AddMessageClientFilter(client_filter);

  auto trpc_codec = std::make_shared<TrpcClientCodec>();
  ClientContextPtr ctx = MakeRefCounted<ClientContext>(trpc_codec);
  ctx->SetFilterData<FilterTestData>(kTestFilterIndex, FilterTestData());
  FilterStatus status = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  status = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  auto* filter_data = ctx->GetFilterData<FilterTestData>(kTestFilterIndex);
  ASSERT_EQ(filter_data->filter_names.size(), 4);
  ASSERT_EQ(filter_data->filter_names[0], "client_global_filter");
  ASSERT_EQ(filter_data->filter_names[1], "client_service_filter");
  ASSERT_EQ(filter_data->filter_names[2], "client_service_filter");
  ASSERT_EQ(filter_data->filter_names[3], "client_global_filter");
}

TEST_F(FilterControllerTestFixture, RunMessageClientFiltersWithSGlobalReject) {
  MessageClientFilterPtr client_filter(new MockClientFilter());
  MockFilterInit<MessageClientFilterPtr, MockClientFilter, ClientContextPtr>(
      client_filter, "client_global_reject_filter",
      {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE});
  ASSERT_EQ(0, client_filter->Init());

  FilterController filter_controller;
  FilterManager::GetInstance()->AddMessageClientGlobalFilter(client_filter);

  MessageClientFilterPtr client_filter1(new MockClientFilter());
  MockFilterInit<MessageClientFilterPtr, MockClientFilter, ClientContextPtr>(
      client_filter1, "client_service_filter",
      {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE});
  ASSERT_EQ(0, client_filter1->Init());
  filter_controller.AddMessageClientFilter(client_filter1);

  auto trpc_codec = std::make_shared<TrpcClientCodec>();
  ClientContextPtr ctx = MakeRefCounted<ClientContext>(trpc_codec);
  ctx->SetFilterData<FilterTestData>(kTestFilterIndex, FilterTestData());
  FilterStatus status = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  status = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  auto* filter_data = ctx->GetFilterData<FilterTestData>(kTestFilterIndex);
  ASSERT_EQ(filter_data->filter_names.size(), 3);
  ASSERT_EQ(filter_data->filter_names[0], "client_global_filter");
  ASSERT_EQ(filter_data->filter_names[1], "client_global_reject_filter");
  ASSERT_EQ(filter_data->filter_names[2], "client_global_filter");
}

TEST_F(FilterControllerTestFixture, RunMessageClientFiltersWithServiceReject) {
  MessageClientFilterPtr client_filter(new MockClientFilter());
  MockFilterInit<MessageClientFilterPtr, MockClientFilter, ClientContextPtr>(
      client_filter, "client_service_reject_filter",
      {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE});
  ASSERT_EQ(0, client_filter->Init());

  FilterController filter_controller;
  filter_controller.AddMessageClientFilter(client_filter);

  MessageClientFilterPtr client_filter1(new MockClientFilter());
  MockFilterInit<MessageClientFilterPtr, MockClientFilter, ClientContextPtr>(
      client_filter1, "client_service_filter",
      {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE});
  ASSERT_EQ(0, client_filter1->Init());

  filter_controller.AddMessageClientFilter(client_filter1);

  auto trpc_codec = std::make_shared<TrpcClientCodec>();
  ClientContextPtr ctx = MakeRefCounted<ClientContext>(trpc_codec);
  ctx->SetFilterData<FilterTestData>(kTestFilterIndex, FilterTestData());
  FilterStatus status = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::REJECT);
  status = filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
  ASSERT_EQ(status, FilterStatus::CONTINUE);

  auto* filter_data = ctx->GetFilterData<FilterTestData>(kTestFilterIndex);
  ASSERT_EQ(filter_data->filter_names.size(), 3);
  ASSERT_EQ(filter_data->filter_names[0], "client_global_filter");
  ASSERT_EQ(filter_data->filter_names[1], "client_service_reject_filter");
  ASSERT_EQ(filter_data->filter_names[2], "client_global_filter");
}

}  // namespace trpc::testing
